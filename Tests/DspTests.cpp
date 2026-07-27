#include <JuceHeader.h>
#include "../Source/LazerDSP.h"
#include <cmath>
#include <cstdio>

// Harness pengukuran DSP offline (§10.1). Menjalankan sinus 1 kHz melalui
// mesin dengan cascade netral, lalu melaporkan penguatan dan distorsi tahap
// soft clip pada berbagai setelan Drive.

namespace
{
    constexpr double fs = 48000.0;
    constexpr int    blockSize = 512;
    constexpr int    numBlocks = 40;
    // Jendela analisis = paruh kedua, harus memuat periode bulat agar tidak ada
    // kebocoran spektral yang menyamar sebagai distorsi.
    constexpr int    analysisLen = (numBlocks / 2) * blockSize;
    constexpr double freq = fs * 213.0 / (double) analysisLen;   // ~998,4 Hz

    struct Result { double peak, gainDb, thdPct; };

    Result measure (float drive, float amplitude, float outGainDb, bool softClip)
    {
        LazerDSP dsp;
        dsp.prepare (fs, blockSize);

        LazerDSP::Parameters p;
        p.amount       = 1;        // cascade sependek mungkin
        p.pinch        = 0.5f;
        p.sweepDepth   = 0.0f;     // filter statis: all-pass murni, magnitudo 1
        p.autoGain     = false;    // isolasi tahap clip
        p.softClip     = softClip;
        p.drive        = drive;
        p.outputGainDb = outGainDb;
        p.dryWetPct    = 100.0f;
        dsp.setParameters (p);

        juce::AudioBuffer<float> buf (2, blockSize);
        double phase = 0.0;
        const double inc = juce::MathConstants<double>::twoPi * freq / fs;

        double peak = 0.0, sumSq = 0.0, sumSin = 0.0, sumCos = 0.0;
        int counted = 0;

        for (int b = 0; b < numBlocks; ++b)
        {
            double startPhase = phase;
            for (int n = 0; n < blockSize; ++n)
            {
                const float s = (float) (amplitude * std::sin (phase));
                buf.setSample (0, n, s);
                buf.setSample (1, n, s);
                phase += inc;
            }

            dsp.process (buf, nullptr, 0);

            // Buang blok awal: smoothing parameter & state filter belum mapan
            if (b < numBlocks / 2)
                continue;

            double ph = startPhase;
            for (int n = 0; n < blockSize; ++n, ph += inc)
            {
                const double y = buf.getSample (0, n);
                peak = juce::jmax (peak, std::abs (y));
                sumSq += y * y;
                // Proyeksi kompleks: all-pass menggeser fase, jadi fundamental
                // harus diukur lewat komponen sin DAN cos sekaligus.
                sumSin += y * std::sin (ph);
                sumCos += y * std::cos (ph);
                ++counted;
            }
        }

        const double n = juce::jmax (1, counted);
        const double a1 = 2.0 * std::sqrt (sumSin * sumSin + sumCos * sumCos) / n;
        const double fundPow = 0.5 * a1 * a1 * n;
        const double distPow = juce::jmax (0.0, sumSq - fundPow);
        const double thd = std::sqrt (distPow / juce::jmax (1.0e-12, fundPow)) * 100.0;

        return { peak, juce::Decibels::gainToDecibels (peak / juce::jmax (1.0e-9, (double) amplitude)),
                 thd };
    }
}

namespace
{
    // Sinyal uji berisi transien: deret hentakan mirip kick agar detektor menyala
    // dan sapuan benar-benar berjalan — itulah kondisi kerja Auto Gain (§6.5).
    struct BurstResult { double peak, rms; };

    BurstResult runBurstTest (bool autoGain, bool phaseInvert, float dryWetPct)
    {
        LazerDSP dsp;
        dsp.prepare (fs, blockSize);

        LazerDSP::Parameters p;
        p.baseFreq    = 500.0f;
        p.pinch       = 5.0f;
        p.amount      = 16;
        p.sweepDepth  = 3.0f;
        p.sweepTimeMs = 150.0f;
        p.softClip    = false;          // isolasi dari clipper
        p.autoGain    = autoGain;
        p.phaseInvert = phaseInvert;
        p.dryWetPct   = dryWetPct;
        dsp.setParameters (p);

        juce::AudioBuffer<float> buf (2, blockSize);
        const int periodSamples = (int) (0.25 * fs);
        double peak = 0.0, sumSq = 0.0;
        int counted = 0, n0 = 0;

        for (int b = 0; b < 80; ++b)
        {
            for (int n = 0; n < blockSize; ++n, ++n0)
            {
                const double tInPeriod = (double) (n0 % periodSamples) / fs;
                const double env = std::exp (-tInPeriod / 0.03);
                const float s = (float) (0.8 * std::sin (juce::MathConstants<double>::twoPi * 60.0
                                                         * tInPeriod) * env);
                buf.setSample (0, n, s);
                buf.setSample (1, n, s);
            }

            dsp.process (buf, nullptr, 0);

            if (b < 8)                   // lewati transien awal
                continue;
            for (int n = 0; n < blockSize; ++n)
            {
                const double y = buf.getSample (0, n);
                peak = juce::jmax (peak, std::abs (y));
                sumSq += y * y;
                ++counted;
            }
        }

        return { peak, std::sqrt (sumSq / juce::jmax (1, counted)) };
    }

    // Kondisi yang menjadi alasan Auto Gain ada (§6.5): nada berkelanjutan yang
    // masuk mendadak sehingga memicu sapuan, lalu cascade termodulasi menumpuk
    // fase dan menghasilkan lonjakan puncak.
    BurstResult runSweepPeakTest (bool autoGain)
    {
        LazerDSP dsp;
        dsp.prepare (fs, blockSize);

        LazerDSP::Parameters p;
        p.baseFreq    = 200.0f;
        p.pinch       = 5.0f;
        p.amount      = 8;
        p.sweepDepth  = 5.3f;            // 200 Hz -> ~8 kHz
        p.sweepTimeMs = 250.0f;
        p.retrigger   = LazerDSP::RetriggerMode::lock;
        p.softClip    = false;
        p.autoGain    = autoGain;
        p.dryWetPct   = 100.0f;
        dsp.setParameters (p);

        juce::AudioBuffer<float> buf (2, blockSize);
        double peak = 0.0, sumSq = 0.0;
        int counted = 0, n0 = 0;
        const int limit = (int) (0.4 * fs);   // 400 ms pertama: sapuan berjalan

        while (n0 < limit)
        {
            for (int n = 0; n < blockSize; ++n, ++n0)
            {
                const float s = (float) (0.9 * std::sin (juce::MathConstants<double>::twoPi
                                                         * 1000.0 * (double) n0 / fs));
                buf.setSample (0, n, s);
                buf.setSample (1, n, s);
            }

            dsp.process (buf, nullptr, 0);

            for (int n = 0; n < blockSize; ++n)
            {
                const double y = buf.getSample (0, n);
                peak = juce::jmax (peak, std::abs (y));
                sumSq += y * y;
                ++counted;
            }
        }

        return { peak, std::sqrt (sumSq / juce::jmax (1, counted)) };
    }
}

int main()
{
    std::printf ("== Kurva transfer soft clip, sinus 1 kHz, Auto Gain OFF ==\n\n");

    std::printf ("Drive |  in 0.10          |  in 0.30          |  in 0.70          |  in 1.00\n");
    std::printf ("      |  peak   gain  THD |  peak   gain  THD |  peak   gain  THD |  peak   gain  THD\n");
    std::printf ("------+-------------------+-------------------+-------------------+------------------\n");

    for (const float d : { 1.0f, 2.0f, 4.0f, 7.0f, 10.0f })
    {
        std::printf ("%5.1f ", d);
        for (const float a : { 0.10f, 0.30f, 0.70f, 1.00f })
        {
            const auto r = measure (d, a, 0.0f, true);
            std::printf ("| %5.3f %+5.1f %4.1f%% ", r.peak, r.gainDb, r.thdPct);
        }
        std::printf ("\n");
    }

    std::printf ("\n== Pengaruh Out Gain sebelum clipper (Drive 2.0, input 0.30) ==\n\n");
    std::printf ("OutGain |  peak   THD\n");
    std::printf ("--------+-------------\n");
    for (const float g : { 0.0f, 6.0f, 12.0f, 18.0f, 24.0f })
    {
        const auto r = measure (2.0f, 0.30f, g, true);
        std::printf ("%+6.0f dB| %5.3f %5.1f%%\n", g, r.peak, r.thdPct);
    }

    std::printf ("\n== Pembanding: Soft Clip OFF (Drive 10, input 0.30) ==\n");
    const auto off = measure (10.0f, 0.30f, 0.0f, false);
    std::printf ("peak %.3f  THD %.2f%%\n", off.peak, off.thdPct);

    // --- Garis Drive sebagai ambang: makin ke bawah, makin banyak yang ter-clip
    std::printf ("\n== Garis ambang: turunkan garis -> clipping naik (input 0.30) ==\n\n");
    std::printf ("Ambang |  Drive |  peak |  THD    | gain  | round-trip\n");
    std::printf ("-------+--------+-------+---------+-------+-----------\n");

    bool monotonic = true, roundTripOk = true, ceilingOk = true;
    double prevThd = -1.0;

    for (const double thrDb : { 0.0, -6.0, -12.0, -18.0, -24.0, -30.0 })
    {
        const double drive = LazerDSP::thresholdDbToDrive (thrDb);
        const double back  = LazerDSP::driveToThresholdDb (drive);
        const auto r = measure ((float) drive, 0.30f, 0.0f, true);

        // Ambang di luar rentang Drive 1..10 akan diklem, jadi round-trip hanya
        // wajib tepat bila hasil klemnya tidak menempel di batas.
        const bool clamped = drive <= 1.0001 || drive >= 9.9999;
        const bool rt = clamped || std::abs (back - thrDb) < 0.01;
        roundTripOk = roundTripOk && rt;
        if (r.thdPct < prevThd - 0.01) monotonic = false;
        prevThd = r.thdPct;

        // Puncak tidak boleh melewati ambang lebih dari toleransi overshoot
        // rekonstruksi oversampling
        const double thrLin = std::pow (10.0, thrDb / 20.0);
        const double ceiling = juce::jmin (0.30, thrLin);
        if (r.peak > ceiling * 1.35 + 0.005) ceilingOk = false;

        std::printf ("%+5.1f dB| %6.2f | %5.3f | %6.2f%% | %+5.1f | %s\n",
                     thrDb, drive, r.peak, r.thdPct, r.gainDb, rt ? "ok" : "GAGAL");
    }

    // Klaim inti: clipper tidak pernah menaikkan level
    double maxGain = -99.0;
    for (const float d : { 1.0f, 2.0f, 4.0f, 7.0f, 10.0f })
        for (const float a : { 0.10f, 0.30f, 0.70f, 1.00f })
            maxGain = juce::jmax (maxGain, measure (d, a, 0.0f, true).gainDb);

    const bool noBoost = maxGain <= 0.1;

    // --- Apakah Auto Gain dan Phase Invert benar-benar berbuat sesuatu?
    std::printf ("\n== Auto Gain & Phase Invert pada deret hentakan ==\n\n");

    const auto agOff = runBurstTest (false, false, 100.0f);
    const auto agOn  = runBurstTest (true,  false, 100.0f);
    std::printf ("Auto Gain OFF : peak %.4f  rms %.4f\n", agOff.peak, agOff.rms);
    std::printf ("Auto Gain ON  : peak %.4f  rms %.4f  -> selisih peak %+.2f dB\n",
                 agOn.peak, agOn.rms,
                 juce::Decibels::gainToDecibels (agOn.peak / juce::jmax (1.0e-9, agOff.peak)));

    const auto spOff = runSweepPeakTest (false);
    const auto spOn  = runSweepPeakTest (true);
    std::printf ("Sapuan pada nada berkelanjutan (kondisi 6.5):\n");
    std::printf ("  A.Gain OFF: peak %.4f  rms %.4f\n", spOff.peak, spOff.rms);
    std::printf ("  A.Gain ON : peak %.4f  rms %.4f  -> selisih peak %+.2f dB\n",
                 spOn.peak, spOn.rms,
                 juce::Decibels::gainToDecibels (spOn.peak / juce::jmax (1.0e-9, spOff.peak)));

    const auto piWet100off = runBurstTest (false, false, 100.0f);
    const auto piWet100on  = runBurstTest (false, true,  100.0f);
    const auto piWet50off  = runBurstTest (false, false, 50.0f);
    const auto piWet50on    = runBurstTest (false, true,  50.0f);
    std::printf ("Ph.Inv @ Dry/Wet 100%% : rms %.4f -> %.4f  (selisih %+.2f dB)\n",
                 piWet100off.rms, piWet100on.rms,
                 juce::Decibels::gainToDecibels (piWet100on.rms / juce::jmax (1.0e-9, piWet100off.rms)));
    std::printf ("Ph.Inv @ Dry/Wet  50%% : rms %.4f -> %.4f  (selisih %+.2f dB)\n",
                 piWet50off.rms, piWet50on.rms,
                 juce::Decibels::gainToDecibels (piWet50on.rms / juce::jmax (1.0e-9, piWet50off.rms)));

    // Lokasi preset pengguna sebagaimana diselesaikan JUCE di mesin ini
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                .getChildFile ("Lazerator").getChildFile ("Presset");
        std::printf ("\nfolder preset: %s\n", folder.getFullPathName().toRawUTF8());
    }

    std::printf ("\nclipping menaik monoton saat ambang turun: %s\n", monotonic ? "YA" : "TIDAK");
    std::printf ("round-trip ambang<->Drive konsisten: %s\n", roundTripOk ? "YA" : "TIDAK");
    std::printf ("puncak tertahan di ambang: %s\n", ceilingOk ? "YA" : "TIDAK");
    std::printf ("tidak pernah menambah gain (maks %+.2f dB): %s\n",
                 maxGain, noBoost ? "YA" : "TIDAK");

    return (monotonic && roundTripOk && ceilingOk && noBoost) ? 0 : 1;
}
