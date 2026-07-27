#include "LazerDSP.h"
#include <cmath>
#include <complex>
#include <cstring>

//==============================================================================
// Filter sidechain (RBJ, Q = 1/sqrt(2))

void LazerDSP::BiquadD::setHighpass (double f, double fs)
{
    const double w0  = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, 0.45 * fs, f) / fs;
    const double cs  = std::cos (w0);
    const double al  = std::sin (w0) / juce::MathConstants<double>::sqrt2;
    const double inv = 1.0 / (1.0 + al);

    b0 = 0.5 * (1.0 + cs) * inv;
    b1 = -(1.0 + cs) * inv;
    b2 = b0;
    a1 = -2.0 * cs * inv;
    a2 = (1.0 - al) * inv;
}

void LazerDSP::BiquadD::setLowpass (double f, double fs)
{
    const double w0  = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, 0.45 * fs, f) / fs;
    const double cs  = std::cos (w0);
    const double al  = std::sin (w0) / juce::MathConstants<double>::sqrt2;
    const double inv = 1.0 / (1.0 + al);

    b0 = 0.5 * (1.0 - cs) * inv;
    b1 = (1.0 - cs) * inv;
    b2 = b0;
    a1 = -2.0 * cs * inv;
    a2 = (1.0 - al) * inv;
}

//==============================================================================

void LazerDSP::prepare (double sampleRate, int maxBlockSize)
{
    mFs = sampleRate;
    mInvFs = 1.0 / sampleRate;
    mMaxBlock = juce::jmax (16, maxBlockSize);

    // §6.1: tau_fast = 1 ms, tau_slow = 80 ms
    mAlphaFast = std::exp (-1.0 / (0.001 * mFs));
    mAlphaSlow = std::exp (-1.0 / (0.080 * mFs));

    mGlideTotal   = juce::jmax (1, (int) std::lround (0.002 * mFs)); // §6.6: glide 2 ms
    mStageFadeLen = juce::jmax (1, (int) std::lround (0.010 * mFs)); // §6.9 C: crossfade 10 ms

    // §6.5: jendela puncak ~100 ms, attack cepat, release ~300 ms
    mPeakDecay = std::exp (-1.0 / (0.100 * mFs));
    mAgAttack  = std::exp (-1.0 / (0.005 * mFs));
    mAgRelease = std::exp (-1.0 / (0.300 * mFs));

    mCorrCoef = std::exp (-1.0 / (0.250 * mFs));             // meter korelasi (§6.7)

    // §6.9 jalur A: 30 ms
    const double ramp = 0.03;
    mBaseSm.reset (mFs, ramp);
    mQSm.reset (mFs, ramp);
    mSpreadSm.reset (mFs, ramp);
    mMixSm.reset (mFs, ramp);
    mOutGainSm.reset (mFs, ramp);
    mDriveSm.reset (mFs, ramp);
    mBypassSm.reset (mFs, 0.02);                             // §8.5: bypass 20 ms

    mBaseSm.setCurrentAndTargetValue (juce::jmax (20.0, (double) mParams.baseFreq));
    mQSm.setCurrentAndTargetValue (mParams.pinch);
    mSpreadSm.setCurrentAndTargetValue (0.0);
    mMixSm.setCurrentAndTargetValue (mParams.dryWetPct * 0.01);
    mOutGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain ((double) mParams.outputGainDb));
    mDriveSm.setCurrentAndTargetValue (mParams.drive);
    mBypassSm.setCurrentAndTargetValue (0.0);

    // §6.10: 4x IIR polyphase half-band, latensi sub-sampel (dilaporkan nol)
    mOversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);
    mOversampler->initProcessing ((size_t) mMaxBlock);

    mDry.setSize (2, mMaxBlock);
    mDet.setSize (1, mMaxBlock);

    mLastHpf = mLastLpf = -1.0f;                             // paksa update filter SC
    setParameters (mParams);
    reset();
}

void LazerDSP::reset()
{
    mEnvFast = mEnvSlow = 0.0;
    mPrevDetected = false;
    mSamplesSinceTrigger = 1 << 30;
    mCorrLR = mCorrLL = mCorrRR = 0.0;
    mCorrelation.store (1.0f);
    mVizFifo.reset();

    mEnvState = EnvState::idle;
    mCurrentG = 0.0;
    mSweepElapsed = 0.0;
    mGlideCount = 0;

    mDryPeak = mWetPeak = 0.0;
    mAutoGain = 1.0;

    mStageFadeLeft = 0;
    mStagesFrom = mStagesTo;

    std::memset (mS1, 0, sizeof (mS1));
    std::memset (mS2, 0, sizeof (mS2));

    mScHpf.reset();
    mScLpf.reset();

    if (mOversampler != nullptr)
        mOversampler->reset();
}

void LazerDSP::setParameters (const Parameters& p)
{
    mParams = p;

    mBaseSm.setTargetValue (juce::jmax (20.0, (double) p.baseFreq));
    mQSm.setTargetValue (p.pinch);
    mSpreadSm.setTargetValue (p.stereoMode == StereoMode::offset ? (double) p.spreadSemitones : 0.0);
    mMixSm.setTargetValue (p.dryWetPct * 0.01);
    mOutGainSm.setTargetValue (juce::Decibels::decibelsToGain ((double) p.outputGainDb));
    mDriveSm.setTargetValue (p.drive);
    mBypassSm.setTargetValue (p.bypass ? 1.0 : 0.0);

    // §6.1: ambang dipindah ke domain linier — tanpa log10 pada tingkat sampel
    mSensLin   = std::pow (10.0, p.sensitivityDb / 20.0);
    mThreshLin = std::pow (10.0, p.thresholdDb / 20.0);
    mHoldSamples = juce::jmax (1, (int) std::lround (p.holdMs * 0.001 * mFs));

    // §6.8: titik awal sapuan diklem SEBELUM sapuan dimulai, bukan selama berjalan
    const double fMax   = juce::jmin (20000.0, 0.45 * mFs);
    const double fb     = juce::jlimit (20.0, fMax, (double) p.baseFreq);
    const double fStart = juce::jlimit (20.0, fMax, fb * std::exp2 ((double) p.sweepDepth));
    mDepthEff = std::log2 (fStart / fb);
    mStartFreqReadout.store ((float) fStart);

    const double fStartS = juce::jlimit (20.0, fMax, fb * std::exp2 ((double) p.sweepDepth * p.sideDepthPct * 0.01));
    mDepthEffSide = std::log2 (fStartS / fb);

    // §6.7: Time Offset menggeser durasi sapuan antar-kanal (mode Offset saja)
    const double t   = juce::jmax (0.005, (double) p.sweepTimeMs * 0.001);
    const double off = p.stereoMode == StereoMode::offset ? (double) p.timeOffsetPct * 0.01 : 0.0;
    mSweepSamplesBase = juce::jmax (1.0, t * mFs);
    mSweepSamplesL    = juce::jmax (1.0, t * (1.0 - 0.5 * off) * mFs);
    mSweepSamplesR    = juce::jmax (1.0, t * (1.0 + 0.5 * off) * mFs);

    // §6.9 jalur C: seluruh tahap sudah dialokasikan, perubahan N di-crossfade 10 ms
    const int stages = juce::jlimit (1, maxStages, p.amount);
    if (stages != mStagesTo)
    {
        mStagesFrom    = mStagesTo;
        mStagesTo      = stages;
        mStageFadeLeft = mStageFadeLen;
    }

    if (! juce::exactlyEqual (p.scHighPass, mLastHpf)) { mScHpf.setHighpass (p.scHighPass, mFs); mLastHpf = p.scHighPass; }
    if (! juce::exactlyEqual (p.scLowPass,  mLastLpf)) { mScLpf.setLowpass  (p.scLowPass,  mFs); mLastLpf = p.scLowPass; }
}

//==============================================================================

double LazerDSP::clampFreq (double f) const
{
    return juce::jlimit (20.0, juce::jmin (20000.0, 0.45 * mFs), f);
}

LazerDSP::Coeffs LazerDSP::makeAllpass (double f0, double q) const
{
    const double w0    = juce::MathConstants<double>::twoPi * f0 * mInvFs;
    const double alpha = std::sin (w0) / (2.0 * juce::jmax (0.5, q));
    const double inv   = 1.0 / (1.0 + alpha);
    return { -2.0 * std::cos (w0) * inv, (1.0 - alpha) * inv };
}

double LazerDSP::runCascade (int ch, double x, const Coeffs& c, double stageMix)
{
    double* s1 = mS1[ch];
    double* s2 = mS2[ch];
    const int nFrom = mStagesFrom, nTo = mStagesTo;
    const int nMax  = juce::jmax (nFrom, nTo);

    double v = x, tapFrom = x, tapTo = x;
    for (int k = 0; k < nMax; ++k)
    {
        const double y = c.a2 * v + s1[k];
        s1[k] = c.a1 * (v - y) + s2[k];                      // b1 == a1: satu perkalian dihemat (§6.3)
        s2[k] = v - c.a2 * y;
        v = y;
        if (k + 1 == nFrom) tapFrom = v;
        if (k + 1 == nTo)   tapTo   = v;
    }

    if (nFrom == nTo)
        return tapTo;
    return tapFrom + stageMix * (tapTo - tapFrom);
}

bool LazerDSP::tryTrigger()
{
    switch (mParams.retrigger)
    {
        case RetriggerMode::reset:
            break;
        case RetriggerMode::legato:                          // §6.6: terima hanya bila sudah lewat separuh
            if (mEnvState == EnvState::glide) return false;
            if (mEnvState == EnvState::sweep && mSweepElapsed < 0.5 * mSweepSamplesBase) return false;
            break;
        case RetriggerMode::lock:                            // §6.6: tunggu sampai tuntas
            if (mEnvState != EnvState::idle) return false;
            break;
    }

    // §6.6: hindari klik — g diluncurkan menuju 1,0 selama 2 ms, bukan dilompatkan
    mGlideStartG = mCurrentG;
    mGlideCount  = 0;
    mEnvState    = EnvState::glide;
    mTriggerCount.fetch_add (1, std::memory_order_relaxed);
    return true;
}

void LazerDSP::tickEnvelope (double& gL, double& gR)
{
    switch (mEnvState)
    {
        case EnvState::idle:
            mCurrentG = 0.0;
            gL = gR = 0.0;
            return;

        case EnvState::glide:
        {
            ++mGlideCount;
            const double f = (double) mGlideCount / (double) mGlideTotal;
            mCurrentG = mGlideStartG + (1.0 - mGlideStartG) * f;
            gL = gR = mCurrentG;
            if (mGlideCount >= mGlideTotal)
            {
                mEnvState = EnvState::sweep;
                mSweepElapsed = 0.0;
            }
            return;
        }

        case EnvState::sweep:
            mSweepElapsed += 1.0;
            gL = shape (mSweepElapsed / mSweepSamplesL);
            gR = shape (mSweepElapsed / mSweepSamplesR);
            mCurrentG = gL;
            if (mSweepElapsed >= mSweepSamplesL && mSweepElapsed >= mSweepSamplesR)
                mEnvState = EnvState::idle;
            return;
    }
}

double LazerDSP::shape (double u) const
{
    // §6.2: g(u) = (1 - u)^c, nol untuk u > 1
    if (u >= 1.0)
        return 0.0;
    const double base = 1.0 - u;
    return juce::exactlyEqual (mParams.curve, 1.0f) ? base : std::pow (base, (double) mParams.curve);
}

//==============================================================================

void LazerDSP::process (juce::AudioBuffer<float>& buffer, const float* const* sc, int numScCh)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, buffer.getNumChannels());
    if (numSamples == 0 || numChannels == 0)
        return;

    // Defensif: host yang melanggar ukuran blok yang dijanjikan
    if (mDry.getNumSamples() < numSamples)
    {
        mDry.setSize (2, numSamples, false, false, true);
        mDet.setSize (1, numSamples, false, false, true);
    }

    for (int ch = 0; ch < numChannels; ++ch)
        mDry.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    auto* const* data = buffer.getArrayOfWritePointers();
    const float* dryL = mDry.getReadPointer (0);
    const float* dryR = mDry.getReadPointer (numChannels > 1 ? 1 : 0);
    float* det = mDet.getWritePointer (0);

    const bool stereo = numChannels == 2;
    const bool useMS  = stereo && mParams.stereoMode == StereoMode::midSide;
    const bool useOff = stereo && mParams.stereoMode == StereoMode::offset;
    const bool extSC  = mParams.scExternal && sc != nullptr && numScCh > 0 && sc[0] != nullptr;

    double lastG = 0.0;
    double blockFastMax = 0.0;

    for (int n = 0; n < numSamples; ++n)
    {
        // ---- 1. DETEKSI (§6.1) — selalu jumlah L+R atau sidechain eksternal (§6.7)
        double d;
        if (extSC)
        {
            d = sc[0][n];
            if (numScCh > 1 && sc[1] != nullptr) d = 0.5 * (d + sc[1][n]);
        }
        else
        {
            d = data[0][n];
            if (stereo) d = 0.5 * (d + data[1][n]);
        }

        d = mScLpf.process (mScHpf.process (d));
        det[n] = (float) d;

        const double xr = std::abs (d);
        mEnvFast = mAlphaFast * mEnvFast + (1.0 - mAlphaFast) * xr;
        mEnvSlow = mAlphaSlow * mEnvSlow + (1.0 - mAlphaSlow) * xr;
        blockFastMax = juce::jmax (blockFastMax, mEnvFast);

        // Deteksi berbasis tepi: satu transien = satu keputusan, sehingga
        // penolakan oleh Hold dapat dihitung untuk indikator GUI (§9.3).
        const bool detected = (mEnvFast + kEps) >= (mEnvSlow + kEps) * mSensLin  // (1) cukup transien
                           && mEnvFast >= mThreshLin;                            // (2) cukup keras
        if (detected && ! mPrevDetected)
        {
            if (mSamplesSinceTrigger >= mHoldSamples)           // (3) hold antar-trigger
            {
                if (tryTrigger())
                    mSamplesSinceTrigger = 0;
                else
                    mRejectCount.fetch_add (1, std::memory_order_relaxed);  // ditolak mode retrigger
            }
            else
                mRejectCount.fetch_add (1, std::memory_order_relaxed);      // ditolak Hold (§9.3)
        }
        mPrevDetected = detected;
        if (mSamplesSinceTrigger < (1 << 30))
            ++mSamplesSinceTrigger;

        // ---- 2. MODULASI (§6.2, §6.6) — jalur B, tanpa smoothing (§6.9)
        double gL, gR;
        tickEnvelope (gL, gR);
        lastG = gL;

        // ---- 3. PEMETAAN FREKUENSI: f0 = smooth(f_base) * 2^(D_eff * g) (§6.9)
        const double fBase = mBaseSm.getNextValue();
        const double q     = mQSm.getNextValue();
        const double sprd  = mSpreadSm.getNextValue();

        double f0L = clampFreq (fBase * std::exp2 (mDepthEff * gL));
        double f0R = f0L;
        if (useOff)
            f0R = clampFreq (fBase * std::exp2 (mDepthEff * gR) * std::exp2 (sprd / 12.0));
        else if (useMS)
            f0R = clampFreq (fBase * std::exp2 (mDepthEffSide * gL));

        mDisplayF0.store ((float) f0L, std::memory_order_relaxed);

        // ---- koefisien: SEKALI per sampel, bukan per tahap (§7.2 #1)
        const Coeffs cL = makeAllpass (f0L, q);
        const Coeffs cR = juce::exactlyEqual (f0R, f0L) ? cL : makeAllpass (f0R, q);

        double stageMix = 1.0;
        if (mStageFadeLeft > 0)
            stageMix = 1.0 - (double) mStageFadeLeft / (double) mStageFadeLen;

        // ---- 4. CASCADE TDF-II serial (§6.3)
        double inL = data[0][n];
        double inR = stereo ? (double) data[1][n] : inL;
        if (useMS)
        {
            const double m = 0.5 * (inL + inR), s = 0.5 * (inL - inR);
            inL = m; inR = s;
        }

        double wetL = runCascade (0, inL, cL, stageMix);
        double wetR = stereo ? runCascade (1, inR, cR, stageMix) : wetL;

        if (mStageFadeLeft > 0 && --mStageFadeLeft == 0)
        {
            mStagesFrom = mStagesTo;
            for (int ch = 0; ch < 2; ++ch)
                for (int k = mStagesTo; k < maxStages; ++k)
                    mS1[ch][k] = mS2[ch][k] = 0.0;
        }

        if (useMS)
        {
            const double l = wetL + wetR, r = wetL - wetR;
            wetL = l; wetR = r;
        }

        // ---- 5. AUTO-GAIN (§6.5) lalu OUTPUT GAIN — keduanya sebelum soft clip.
        // Menyimpang dari urutan §4 secara sengaja: Output Gain ditempatkan
        // mendahului Drive supaya menaikkannya benar-benar menabrak clipper.
        const double dpk = juce::jmax (std::abs ((double) dryL[n]), std::abs ((double) dryR[n]));
        const double wpk = juce::jmax (std::abs (wetL), std::abs (wetR));
        mDryPeak = juce::jmax (dpk, mDryPeak * mPeakDecay);
        mWetPeak = juce::jmax (wpk, mWetPeak * mPeakDecay);

        double target = 1.0;
        if (mParams.autoGain)
            target = juce::jlimit (0.25, 2.0, (mDryPeak + kEps) / (mWetPeak + kEps));
        const double coeff = target < mAutoGain ? mAgAttack : mAgRelease;
        mAutoGain = coeff * mAutoGain + (1.0 - coeff) * target;

        const double stageGain = mAutoGain * mOutGainSm.getNextValue();

        data[0][n] = (float) (wetL * stageGain);
        if (stereo)
            data[1][n] = (float) (wetR * stageGain);
    }

    mEnvReadout.store ((float) lastG);
    mEnvFastPeakA.store (juce::jmax (mEnvFastPeakA.load(), (float) blockFastMax));
    mEnvSlowA.store ((float) mEnvSlow);

    // ---- 6. SOFT CLIP, 4x oversampling IIR polyphase (§6.10)
    // Menyimpang dari tanh pada §6.10: lututnya jauh terlalu lembut sehingga pada
    // Drive bawaan materi normal hanya menghasilkan ~2% THD — terdengar sekadar
    // lebih keras, bukan ter-clip. Kurva ini tetap linier lebih lama lalu membelok
    // tajam, masih mulus (turunannya kontinu) sehingga oversampling 4x cukup
    // menahan alias, dan lebih murah daripada tanh: hanya dua akar kuadrat.
    const double drive = mDriveSm.skip (numSamples);
    if (mParams.softClip && mOversampler != nullptr)
    {
        auto clip = [] (float u)
        {
            const float u2 = u * u;
            return u / std::sqrt (std::sqrt (1.0f + u2 * u2));
        };

        // Pemetaan kuadratik: ambang clip = 1/dr, jadi dengan dr linier 1..10
        // ambangnya berhenti di 0,1 dan ujung atas kenop nyaris tak terasa pada
        // materi bertingkat normal. dr = d^2/2 menahan setelan bawaan (d=2 -> 2,
        // tetap bersih sebagai jaring pengaman) namun membawa ujung atas ke 50.
        // Clipper murni tanpa makeup: y = T * clip(x / T) dengan T = 1/dr sebagai
        // ambang. Di bawah ambang lerengnya satu sehingga sinyal lewat apa adanya;
        // di atasnya dipotong menempel ke ambang. Tidak ada penguatan sama sekali.
        const float dr = (float) driveToShaperGain (drive);
        const float invDr = 1.0f / dr;

        juce::dsp::AudioBlock<float> full (buffer.getArrayOfWritePointers(),
                                           (size_t) numChannels, (size_t) numSamples);
        for (int start = 0; start < numSamples; start += mMaxBlock)
        {
            const int len = juce::jmin (mMaxBlock, numSamples - start);
            auto blk = full.getSubBlock ((size_t) start, (size_t) len);
            auto os  = mOversampler->processSamplesUp (blk);

            for (size_t ch = 0; ch < os.getNumChannels(); ++ch)
            {
                float* s = os.getChannelPointer (ch);
                for (size_t i = 0; i < os.getNumSamples(); ++i)
                    s[i] = clip (dr * s[i]) * invDr;
            }

            mOversampler->processSamplesDown (blk);
        }
    }

    // ---- 7. OUTPUT: invert -> dry/wet -> bypass (§8.5), SC Listen (§8.3).
    // Output Gain sudah diterapkan pada tahap 5, sebelum clipper.
    const bool inv    = mParams.phaseInvert;
    const bool listen = mParams.scListen;
    double outPeakL = 0.0, outPeakR = 0.0;

    for (int n = 0; n < numSamples; ++n)
    {
        const double mix = mMixSm.getNextValue();
        const double byp = mBypassSm.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const double dry = ch == 0 ? (double) dryL[n] : (double) dryR[n];
            double wet = data[ch][n];
            if (inv) wet = -wet;

            // §4: jalur dry TIDAK dikompensasi waktu — comb filtering disengaja
            double out = mix * wet + (1.0 - mix) * dry;
            if (listen) out = det[n];

            out = (1.0 - byp) * out + byp * dry;
            data[ch][n] = (float) out;
        }

        outPeakL = juce::jmax (outPeakL, std::abs ((double) data[0][n]));
        outPeakR = juce::jmax (outPeakR, std::abs ((double) data[stereo ? 1 : 0][n]));

        if (stereo)
        {
            const double L = data[0][n], R = data[1][n];
            mCorrLR = mCorrCoef * mCorrLR + (1.0 - mCorrCoef) * (L * R);
            mCorrLL = mCorrCoef * mCorrLL + (1.0 - mCorrCoef) * (L * L);
            mCorrRR = mCorrCoef * mCorrRR + (1.0 - mCorrCoef) * (R * R);
        }
    }

    mOutPeakLA.store (juce::jmax (mOutPeakLA.load(), (float) outPeakL));
    mOutPeakRA.store (juce::jmax (mOutPeakRA.load(), (float) outPeakR));

    mCorrelation.store (stereo
        ? (float) juce::jlimit (-1.0, 1.0, mCorrLR / std::sqrt (mCorrLL * mCorrRR + 1.0e-18))
        : 1.0f);

    // §9.3: dorong keluaran mono ke FIFO visualizer (ditarik oleh thread GUI)
    {
        const auto scope = mVizFifo.write (numSamples);
        int idx = 0;
        auto mono = [&] (int i) { return stereo ? 0.5f * (data[0][i] + data[1][i]) : data[0][i]; };
        for (int i = 0; i < scope.blockSize1; ++i, ++idx)
            mVizBuf[(size_t) (scope.startIndex1 + i)] = mono (idx);
        for (int i = 0; i < scope.blockSize2; ++i, ++idx)
            mVizBuf[(size_t) (scope.startIndex2 + i)] = mono (idx);
    }
}

int LazerDSP::pullVisualizerSamples (float* dest, int maxWanted)
{
    const int avail = juce::jmin (maxWanted, mVizFifo.getNumReady());
    if (avail <= 0)
        return 0;

    const auto scope = mVizFifo.read (avail);
    int idx = 0;
    for (int i = 0; i < scope.blockSize1; ++i, ++idx)
        dest[idx] = mVizBuf[(size_t) (scope.startIndex1 + i)];
    for (int i = 0; i < scope.blockSize2; ++i, ++idx)
        dest[idx] = mVizBuf[(size_t) (scope.startIndex2 + i)];
    return idx;
}

//==============================================================================

double LazerDSP::groupDelayMs (double f0, double q, int stages, double fs)
{
    return groupDelayMsAt (f0, q, stages, fs, f0);
}

double LazerDSP::groupDelayMsAt (double f0, double q, int stages, double fs, double evalHz)
{
    if (fs <= 0.0)
        fs = 48000.0;

    const double fMax = juce::jmin (20000.0, 0.45 * fs);
    f0     = juce::jlimit (20.0, fMax, f0);
    evalHz = juce::jlimit (20.0, fMax, evalHz);
    q      = juce::jmax (0.5, q);

    const double w0    = juce::MathConstants<double>::twoPi * f0 / fs;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double inv   = 1.0 / (1.0 + alpha);
    const double a1    = -2.0 * std::cos (w0) * inv;
    const double a2    = (1.0 - alpha) * inv;

    auto phaseAt = [a1, a2] (double w)
    {
        const std::complex<double> z1 = std::polar (1.0, -w);
        const std::complex<double> z2 = std::polar (1.0, -2.0 * w);
        return std::arg ((a2 + a1 * z1 + z2) / (1.0 + a1 * z1 + a2 * z2));
    };

    // Turunan numerik fase; dw kecil agar tidak melewati cabang -pi..pi
    const double dw = 1.0e-6;
    const double we = juce::MathConstants<double>::twoPi * evalHz / fs;
    double dphi = phaseAt (we + dw) - phaseAt (we - dw);
    while (dphi >  juce::MathConstants<double>::pi) dphi -= juce::MathConstants<double>::twoPi;
    while (dphi < -juce::MathConstants<double>::pi) dphi += juce::MathConstants<double>::twoPi;

    const double tauSamples = -dphi / (2.0 * dw);
    return (double) stages * tauSamples / fs * 1000.0;
}
