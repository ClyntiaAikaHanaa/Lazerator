#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

// Mesin DSP Lazerator — implementasi PRD v1.1 (Dynamic Phase Smear).
// Rujukan bagian (§) mengacu pada dokumen PRD v1.1.
class LazerDSP
{
public:
    static constexpr int maxStages = 40;                     // §6.4: diturunkan dari 50

    enum class RetriggerMode { reset = 0, legato, lock };    // §6.6
    enum class StereoMode    { link  = 0, offset, midSide }; // §6.7

    struct Parameters
    {
        // Lazer Engine (§8.1)
        float baseFreq = 800.0f;
        float pinch    = 4.0f;
        int   amount   = 12;

        // Auto-Sweep Modulation (§8.2)
        float sweepDepth  = 3.0f;     // oktaf, bipolar
        float sweepTimeMs = 120.0f;
        float curve       = 1.0f;
        RetriggerMode retrigger = RetriggerMode::reset;

        // Detektor & Sidechain (§8.3)
        float thresholdDb   = -24.0f;
        float sensitivityDb = 6.0f;
        float holdMs        = 20.0f;
        float scHighPass    = 20.0f;
        float scLowPass     = 20000.0f;
        bool  scExternal    = false;
        bool  scListen      = false;

        // Stereo (§8.4)
        StereoMode stereoMode = StereoMode::link;
        float spreadSemitones = 0.0f;
        float timeOffsetPct   = 0.0f;
        float sideDepthPct    = 100.0f;

        // Master & Output (§8.5)
        float dryWetPct    = 100.0f;
        bool  phaseInvert  = false;
        bool  softClip     = true;
        float drive        = 2.0f;
        bool  autoGain     = true;
        float outputGainDb = 0.0f;
        bool  bypass       = false;
    };

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Dipanggil sekali per blok dari thread audio.
    void setParameters (const Parameters& p);
    void process (juce::AudioBuffer<float>& buffer, const float* const* sidechain, int numSidechainChannels);

    // Pembacaan untuk GUI / visualizer (aman lintas thread).
    float getEnvelopeValue()      const { return mEnvReadout.load(); }
    float getEffectiveStartFreq() const { return mStartFreqReadout.load(); }
    int   getTriggerCount()       const { return mTriggerCount.load(); }
    int   getRejectCount()        const { return mRejectCount.load(); }   // ditolak Hold / mode (§9.3)
    float getDisplayFrequency()   const { return mDisplayF0.load(); }
    float getCorrelation()        const { return mCorrelation.load(); }

    // Untuk detect view ala kompresor: puncak envelope cepat sejak pembacaan
    // terakhir (dikonsumsi GUI), dan nilai envelope lambat terkini.
    float consumeEnvFastPeak()          { return mEnvFastPeakA.exchange (0.0f); }
    float getEnvSlowValue()       const { return mEnvSlowA.load(); }

    // Puncak keluaran per kanal sejak pembacaan terakhir, untuk meter L/R.
    float consumeOutPeakL()             { return mOutPeakLA.exchange (0.0f); }
    float consumeOutPeakR()             { return mOutPeakRA.exchange (0.0f); }

    // Menarik sampel keluaran mono untuk spektrum visualizer (thread GUI).
    int pullVisualizerSamples (float* dest, int maxWanted);

    // Penguatan masuk shaper untuk sebuah nilai Drive (§6.10). Ambang clip
    // efektif adalah kebalikannya. Diekspos agar garis Drive pada GUI memakai
    // sumber kebenaran yang sama dengan DSP.
    static double driveToShaperGain (double drive)
    {
        return juce::jmax (0.1, drive * drive * 0.5);
    }

    // Ambang clip dalam dBFS. Dipakai GUI untuk menempatkan garis Drive pada
    // sumbu yang sama dengan meter: makin rendah garisnya, makin banyak sinyal
    // yang melampauinya dan ter-clip.
    static double driveToThresholdDb (double drive)
    {
        return -20.0 * std::log10 (driveToShaperGain (drive));
    }

    static double thresholdDbToDrive (double thresholdDb)
    {
        return juce::jlimit (1.0, 10.0, std::sqrt (2.0 * std::pow (10.0, -thresholdDb / 20.0)));
    }

    // Group delay digital eksak untuk pembacaan GUI (§6.4).
    static double groupDelayMs   (double f0, double q, int stages, double fs);
    static double groupDelayMsAt (double f0, double q, int stages, double fs, double evalHz);

private:
    struct Coeffs { double a1 = 0.0, a2 = 0.0; };

    // Biquad presisi ganda untuk jalur deteksi (§6.1), struktur TDF-II.
    struct BiquadD
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double s1 = 0.0, s2 = 0.0;

        void reset() { s1 = s2 = 0.0; }

        double process (double x)
        {
            const double y = b0 * x + s1;
            s1 = b1 * x - a1 * y + s2;
            s2 = b2 * x - a2 * y;
            return y;
        }

        void setHighpass (double f, double fs);
        void setLowpass  (double f, double fs);
    };

    enum class EnvState { idle, glide, sweep };

    double clampFreq (double f) const;                       // §6.8
    Coeffs makeAllpass (double f0, double q) const;          // §6.3
    double runCascade (int ch, double x, const Coeffs& c, double stageMix);
    bool   tryTrigger();                                     // §6.6
    void   tickEnvelope (double& gL, double& gR);            // §6.2
    double shape (double u) const;

    // --- konfigurasi
    double mFs = 48000.0, mInvFs = 1.0 / 48000.0;
    int    mMaxBlock = 0;
    Parameters mParams;

    // --- detektor (§6.1)
    static constexpr double kEps = 1.0e-12;
    double mAlphaFast = 0.0, mAlphaSlow = 0.0;
    double mEnvFast = 0.0, mEnvSlow = 0.0;
    double mSensLin = 2.0, mThreshLin = 0.063;
    bool   mPrevDetected = false;
    int    mSamplesSinceTrigger = 0, mHoldSamples = 0;
    BiquadD mScHpf, mScLpf;
    float  mLastHpf = -1.0f, mLastLpf = -1.0f;

    // --- envelope sweep (§6.2, §6.6)
    EnvState mEnvState = EnvState::idle;
    double mCurrentG = 0.0, mGlideStartG = 0.0;
    int    mGlideCount = 0, mGlideTotal = 96;
    double mSweepElapsed = 0.0;
    double mSweepSamplesL = 1.0, mSweepSamplesR = 1.0, mSweepSamplesBase = 1.0;
    double mDepthEff = 0.0, mDepthEffSide = 0.0;

    // --- cascade (§6.3), state presisi ganda (§6.8)
    double mS1[2][maxStages] = {}, mS2[2][maxStages] = {};
    int    mStagesFrom = 12, mStagesTo = 12;
    int    mStageFadeLeft = 0, mStageFadeLen = 1;            // jalur C (§6.9)

    // --- smoothing jalur A (§6.9)
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative> mBaseSm;
    juce::SmoothedValue<double> mQSm, mSpreadSm, mMixSm, mOutGainSm, mDriveSm, mBypassSm;

    // --- auto-gain (§6.5)
    double mPeakDecay = 0.0, mAgAttack = 0.0, mAgRelease = 0.0;
    double mDryPeak = 0.0, mWetPeak = 0.0, mAutoGain = 1.0;

    // --- soft clip oversampled (§6.10)
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler;

    // --- buffer kerja
    juce::AudioBuffer<float> mDry, mDet;

    // --- readout & tap visualizer (§9.3)
    static constexpr int vizFifoSize = 8192;
    juce::AbstractFifo mVizFifo { vizFifoSize };
    std::array<float, (size_t) vizFifoSize> mVizBuf {};
    double mCorrCoef = 0.0, mCorrLR = 0.0, mCorrLL = 0.0, mCorrRR = 0.0;

    std::atomic<float> mEnvReadout { 0.0f }, mStartFreqReadout { 800.0f };
    std::atomic<float> mDisplayF0 { 800.0f }, mCorrelation { 1.0f };
    std::atomic<float> mEnvFastPeakA { 0.0f }, mEnvSlowA { 0.0f };
    std::atomic<float> mOutPeakLA { 0.0f }, mOutPeakRA { 0.0f };
    std::atomic<int>   mTriggerCount { 0 }, mRejectCount { 0 };
};
