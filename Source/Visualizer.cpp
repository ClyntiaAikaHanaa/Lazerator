#include "Visualizer.h"
#include "LazerLook.h"
#include <cmath>

namespace
{
    constexpr float kTimeWindow = 3.0f;    // detik jendela tick trigger
    constexpr float kTrailLife  = 0.45f;   // umur jejak f0
    constexpr float kMinDb      = -100.0f;
    constexpr double kTauRefMs  = 4000.0;  // normalisasi tinggi kurva GD

    // Kemiringan tampilan spektrum: material musik cenderung meluruh ~3 dB/oktaf,
    // sehingga tanpa kompensasi hanya low end yang terlihat. Pivot di 1 kHz.
    constexpr float kTiltDbPerOct = 3.5f;
    constexpr float kTiltPivotHz  = 1000.0f;

    float tiltDb (float freq)
    {
        return kTiltDbPerOct * std::log2 (juce::jmax (20.0f, freq) / kTiltPivotHz);
    }
}

Visualizer::Visualizer (LazeratorAudioProcessor& p)
    : processor (p),
      baseAttachment   (*p.apvts.getParameter ("BaseFreq"),   [this] (float) { repaint(); }),
      amountAttachment (*p.apvts.getParameter ("Amount"),     [this] (float) { repaint(); }),
      depthAttachment  (*p.apvts.getParameter ("SweepDepth"), [this] (float) { repaint(); }),
      pinchAttachment  (*p.apvts.getParameter ("Pinch"),      [this] (float) { repaint(); })
{
    smoothDb.fill (kMinDb);
    startTimerHz (30);   // §9.3: maksimum 30 fps
}

void Visualizer::timerCallback()
{
    if (! isShowing())   // §9.3: berhenti saat tidak terlihat
        return;

    float tmp[512];
    for (;;)
    {
        const int n = processor.getDsp().pullVisualizerSamples (tmp, 512);
        if (n <= 0)
            break;
        for (int i = 0; i < n; ++i)
        {
            ring[(size_t) ringPos] = tmp[i];
            ringPos = (ringPos + 1) & (fftSize - 1);
        }
    }

    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = ring[(size_t) ((ringPos + i) & (fftSize - 1))];
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable (fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    for (int b = 0; b <= fftSize / 2; ++b)
    {
        const float mag = fftData[(size_t) b] * (2.0f / (float) fftSize);
        const float db  = juce::jlimit (kMinDb, 0.0f, 20.0f * std::log10 (mag + 1.0e-9f));
        auto& sm = smoothDb[(size_t) b];
        if (db > sm)
            sm = db * 0.6f + sm * 0.4f;
        else
            sm += (db - sm) * 0.12f;
    }

    const float dt = 1.0f / 30.0f;
    auto& dsp = processor.getDsp();

    const int tc = dsp.getTriggerCount();
    for (int k = 0; k < juce::jmin (tc - lastTrigCount, 6); ++k)
        hitAges.push_back ((float) k * 0.01f);
    if (tc != lastTrigCount)
        triggerFlash = 1.0f;
    lastTrigCount = tc;

    for (auto& a : hitAges)
        a += dt;
    hitAges.erase (std::remove_if (hitAges.begin(), hitAges.end(),
                                   [] (float a) { return a > kTimeWindow; }),
                   hitAges.end());

    trail.push_back ({ dsp.getDisplayFrequency(), 0.0f });
    for (auto& t : trail)
        t.age += dt;
    trail.erase (std::remove_if (trail.begin(), trail.end(),
                                 [] (const TrailPoint& t) { return t.age > kTrailLife; }),
                 trail.end());

    triggerFlash *= 0.85f;
    repaint();
}

float Visualizer::freqToX (float freq, juce::Rectangle<float> area) const
{
    const float norm = std::log (juce::jlimit (20.0f, 20000.0f, freq) / 20.0f)
                     / std::log (1000.0f);   // 20..20000 = 3 dekade
    return area.getX() + norm * area.getWidth();
}

float Visualizer::xToFreq (float x, juce::Rectangle<float> area) const
{
    const float norm = juce::jlimit (0.0f, 1.0f, (x - area.getX()) / area.getWidth());
    return 20.0f * std::pow (1000.0f, norm);
}

void Visualizer::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const double fs = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
    auto& ap = processor.apvts;

    const float baseFreq = ap.getRawParameterValue ("BaseFreq")->load();
    const float q        = ap.getRawParameterValue ("Pinch")->load();
    const int   stages   = (int) ap.getRawParameterValue ("Amount")->load();
    const int   mode     = (int) ap.getRawParameterValue ("StereoMode")->load();

    auto& dsp = processor.getDsp();
    const float f0  = dsp.getDisplayFrequency();
    const float env = dsp.getEnvelopeValue();

    // --- kartu panel terang
    lazer::drawCard (g, bounds.reduced (1.0f), 10.0f, lazer::panel);

    plotArea = bounds.reduced (12.0f, 10.0f);
    const auto plot = plotArea;

    // --- grid frekuensi & label
    g.setFont (lazer::hudFont (9.0f));
    for (const float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = freqToX (f, plot);
        const bool major = juce::exactlyEqual (f, 100.0f)
                        || juce::exactlyEqual (f, 1000.0f)
                        || juce::exactlyEqual (f, 10000.0f);
        g.setColour (lazer::ink.withAlpha (major ? 0.10f : 0.05f));
        g.fillRect (x, plot.getY(), 1.0f, plot.getHeight());
        if (major)
        {
            g.setColour (lazer::inkSoft);
            g.drawText (f >= 1000.0f ? juce::String ((int) (f / 1000.0f)) + "k"
                                     : juce::String ((int) f),
                        (int) x + 3, (int) plot.getBottom() - 11, 30, 10,
                        juce::Justification::left);
        }
    }

    // --- latar: spektrum keluaran sebagai sapuan tinta (§9.3)
    {
        juce::Path spec;
        spec.startNewSubPath (plot.getX(), plot.getBottom());
        for (float px = plot.getX(); px <= plot.getRight(); px += 3.0f)
        {
            const float f    = xToFreq (px, plot);
            const float binF = juce::jlimit (0.0f, (float) (fftSize / 2 - 1),
                                             f * (float) fftSize / (float) fs);
            const int   b0   = (int) binF;
            const float frac = binF - (float) b0;
            const float raw  = smoothDb[(size_t) b0] * (1.0f - frac)
                             + smoothDb[(size_t) b0 + 1] * frac;
            // Tilt hanya untuk bin yang benar-benar berisi sinyal. Tanpa ini,
            // lantai display ikut terangkat dan tampak sebagai garis miring.
            const float db   = raw <= kMinDb + 0.5f ? kMinDb : raw + tiltDb (f);
            spec.lineTo (px, juce::jmap (juce::jlimit (kMinDb, 0.0f, db),
                                         kMinDb, 0.0f, plot.getBottom(), plot.getY()));
        }
        spec.lineTo (plot.getRight(), plot.getBottom());
        spec.closeSubPath();

        g.setColour (lazer::ink.withAlpha (0.10f));
        g.fillPath (spec);
        g.setColour (lazer::ink.withAlpha (0.22f));
        g.strokePath (spec, juce::PathStrokeType (1.0f));
    }

    // --- jejak f0
    for (const auto& t : trail)
    {
        const float a = (1.0f - t.age / kTrailLife) * 0.16f;
        if (a <= 0.01f)
            continue;
        g.setColour (lazer::pink.withAlpha (a));
        g.fillRect (freqToX (t.freq, plot) - 0.5f, plot.getY(), 1.0f, plot.getHeight());
    }

    // --- kurva group delay (tinta tebal, bergerak mengikuti sweep)
    auto tauToH = [] (double tauMs)
    {
        return juce::jlimit (0.0f, 1.0f,
                             (float) (std::log10 (1.0 + tauMs) / std::log10 (1.0 + kTauRefMs)));
    };
    {
        juce::Path gd;
        bool first = true;
        for (float px = plot.getX(); px <= plot.getRight(); px += 3.0f)
        {
            const float f = xToFreq (px, plot);
            const float h = tauToH (LazerDSP::groupDelayMsAt (f0, q, stages, fs, f));
            const float y = plot.getBottom() - h * plot.getHeight();
            if (first) { gd.startNewSubPath (px, y); first = false; }
            else         gd.lineTo (px, y);
        }
        g.setColour (lazer::ink);
        g.strokePath (gd, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // --- garis f0 sesaat (menyala saat sweep berjalan)
    {
        const float x = freqToX (f0, plot);
        const float a = 0.20f + 0.60f * env;
        g.setColour (lazer::pink.withAlpha (a * 0.25f));
        g.fillRect (x - 2.5f, plot.getY(), 5.0f, plot.getHeight());
        g.setColour (lazer::pink.withAlpha (a));
        g.fillRect (x - 0.75f, plot.getY(), 1.5f, plot.getHeight());
    }

    // --- node interaktif
    {
        // node BASE: X = frekuensi, Y = jumlah tahap (tinggi bukit)
        const float bx = freqToX (baseFreq, plot);
        const float bh = tauToH (LazerDSP::groupDelayMs (baseFreq, q, stages, fs));
        baseNodePos = { bx, plot.getBottom() - bh * plot.getHeight() };

        // node START: posisi frekuensi awal sweep = Depth
        const float sx = freqToX (dsp.getEffectiveStartFreq(), plot);
        const float sy = plot.getY() + 26.0f;
        startNodePos = { sx, sy };

        // garis panah arah sweep (start -> base) putus-putus teal
        {
            juce::Path guide;
            guide.startNewSubPath (sx, sy);
            guide.lineTo (bx, sy);
            juce::Path dashed;
            const float dashes[] = { 3.0f, 3.0f };
            juce::PathStrokeType (1.1f).createDashedStroke (dashed, guide, dashes, 2);
            g.setColour (lazer::teal.withAlpha (0.55f));
            g.fillPath (dashed);

            const float dir = bx >= sx ? 1.0f : -1.0f;   // panah di ujung base
            juce::Path arrow;
            arrow.addTriangle (bx, sy, bx - dir * 6.0f, sy - 3.5f, bx - dir * 6.0f, sy + 3.5f);
            g.setColour (lazer::teal.withAlpha (0.7f));
            g.fillPath (arrow);
        }

        auto drawNode = [&] (juce::Point<float> c, juce::Colour col, float r, bool hover)
        {
            g.setColour (lazer::panel);
            g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
            if (hover)
            {
                g.setColour (col.withAlpha (0.25f));
                g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
            }
            g.setColour (col);
            g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 2.0f);
        };
        drawNode (startNodePos, lazer::teal, 6.0f, hoverStart || dragTarget == Drag::startNode);
        drawNode (baseNodePos, lazer::pink, 7.5f, hoverBase || dragTarget == Drag::baseNode);

        // Petunjuk saat kursor di node BASE: scroll mengubah Pinch
        if (hoverBase || dragTarget == Drag::baseNode)
        {
            const juce::String hint = "scroll: focus " + juce::String (q, 1);
            g.setFont (lazer::hudFont (9.0f));
            const float tw = (float) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), hint) + 10.0f;
            auto tip = juce::Rectangle<float> (bx - tw * 0.5f, baseNodePos.y + 12.0f, tw, 14.0f)
                           .constrainedWithin (plot);
            g.setColour (lazer::ink.withAlpha (0.88f));
            g.fillRoundedRectangle (tip, 4.0f);
            g.setColour (lazer::panel);
            g.drawText (hint, tip, juce::Justification::centred);
        }
    }

    // --- readout (§6.4, §6.8)
    {
        g.setFont (lazer::hudFont (10.5f).boldened());

        const double gdMs = LazerDSP::groupDelayMs (baseFreq, q, stages, fs);
        const bool warn = gdMs >= 500.0;
        const juce::String gdText = gdMs >= 1000.0
            ? juce::String (gdMs / 1000.0, 2) + " s"
            : juce::String (gdMs, 1) + " ms";
        g.setColour (warn ? lazer::warnCol : lazer::ink.withAlpha (0.8f));
        g.drawText ((warn ? "[!] " : "") + juce::String ("GD ") + gdText,
                    (int) plot.getX() + 4, (int) plot.getY() + 2, 160, 12,
                    juce::Justification::left);

        const double fStart = (double) dsp.getEffectiveStartFreq();
        const juce::String st = fStart >= 1000.0
            ? juce::String (fStart / 1000.0, 1) + " kHz"
            : juce::String ((int) fStart) + " Hz";
        g.setColour (lazer::inkSoft);
        g.drawText ("START " + st,
                    (int) plot.getRight() - 164, (int) plot.getY() + 2, 160, 12,
                    juce::Justification::right);
    }

    // --- meter korelasi, mode Offset / Mid-Side saja (§6.7)
    if (mode != 0)
    {
        const float corr = dsp.getCorrelation();
        auto meter = juce::Rectangle<float> (plot.getRight() - 96.0f,
                                             plot.getBottom() - 14.0f, 90.0f, 8.0f);
        g.setColour (lazer::ink.withAlpha (0.08f));
        g.fillRoundedRectangle (meter, 4.0f);
        g.setColour (lazer::inkSoft);
        g.fillRect (meter.getCentreX() - 0.5f, meter.getY(), 1.0f, meter.getHeight());

        const float nx = meter.getX() + (corr + 1.0f) * 0.5f * meter.getWidth();
        g.setColour (corr >= 0.0f ? lazer::teal : lazer::warnCol);
        g.fillRoundedRectangle (nx - 1.5f, meter.getY(), 3.0f, meter.getHeight(), 1.5f);

        g.setFont (lazer::hudFont (8.0f));
        g.setColour (lazer::inkSoft);
        g.drawText ("CORR", (int) meter.getX() - 34, (int) meter.getY() - 1, 32, 10,
                    juce::Justification::right);
    }

    // --- tick trigger diterima di dasar plot
    for (const float age : hitAges)
    {
        const float x = plot.getRight() - (age / kTimeWindow) * plot.getWidth();
        g.setColour (lazer::pink.withAlpha (1.0f - age / kTimeWindow));
        g.fillRect (x - 1.0f, plot.getBottom() - 8.0f, 2.0f, 8.0f);
    }

    // --- kilat bingkai saat trigger
    if (triggerFlash > 0.02f)
    {
        g.setColour (lazer::pink.withAlpha (0.55f * triggerFlash));
        g.drawRoundedRectangle (bounds.reduced (1.5f), 10.0f, 2.0f);
    }
}

//==============================================================================

void Visualizer::mouseDown (const juce::MouseEvent& e)
{
    if (e.position.getDistanceFrom (startNodePos) < 11.0f)
    {
        dragTarget = Drag::startNode;
        depthAttachment.beginGesture();
    }
    else if (e.position.getDistanceFrom (baseNodePos) < 13.0f)
    {
        dragTarget = Drag::baseNode;
        baseAttachment.beginGesture();
        amountAttachment.beginGesture();
    }
}

void Visualizer::mouseDrag (const juce::MouseEvent& e)
{
    const double fs = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
    const float baseFreq = processor.apvts.getRawParameterValue ("BaseFreq")->load();
    const float q        = processor.apvts.getRawParameterValue ("Pinch")->load();

    if (dragTarget == Drag::startNode)
    {
        // X -> Depth: posisi awal sweep relatif terhadap base, dalam oktaf
        const float f = xToFreq (e.position.x, plotArea);
        const float depth = juce::jlimit (-6.0f, 6.0f,
                                          std::log2 (f / juce::jmax (20.0f, baseFreq)));
        depthAttachment.setValueAsPartOfGesture (depth);
    }
    else if (dragTarget == Drag::baseNode)
    {
        // X -> Base Frequency
        const float f = juce::jlimit (20.0f, 20000.0f, xToFreq (e.position.x, plotArea));
        baseAttachment.setValueAsPartOfGesture (f);

        // Y -> Amount: balikkan pemetaan tinggi bukit ke jumlah tahap
        const float hNorm = juce::jlimit (0.0f, 1.0f,
                                          (plotArea.getBottom() - e.position.y) / plotArea.getHeight());
        const double tauTarget   = std::pow (10.0, (double) hNorm * std::log10 (1.0 + kTauRefMs)) - 1.0;
        const double tauPerStage = juce::jmax (1.0e-6, LazerDSP::groupDelayMsAt (f, q, 1, fs, f));
        const int stages = juce::jlimit (1, LazerDSP::maxStages,
                                         (int) std::lround (tauTarget / tauPerStage));
        amountAttachment.setValueAsPartOfGesture ((float) stages);
    }
}

void Visualizer::mouseUp (const juce::MouseEvent&)
{
    if (dragTarget == Drag::startNode)
        depthAttachment.endGesture();
    else if (dragTarget == Drag::baseNode)
    {
        baseAttachment.endGesture();
        amountAttachment.endGesture();
    }
    dragTarget = Drag::none;
}

void Visualizer::mouseMove (const juce::MouseEvent& e)
{
    const bool hb = e.position.getDistanceFrom (baseNodePos) < 13.0f;
    const bool hs = e.position.getDistanceFrom (startNodePos) < 11.0f;
    if (hb != hoverBase || hs != hoverStart)
    {
        hoverBase = hb;
        hoverStart = hs;
        setMouseCursor (hb || hs ? juce::MouseCursor::PointingHandCursor
                                 : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void Visualizer::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // Scroll di atas node BASE mengatur Focus (Q) — ketajaman bukit fase.
    // Multiplikatif agar terasa sama di seluruh rentang yang bertaper log.
    if (e.position.getDistanceFrom (baseNodePos) >= 13.0f)
        return;

    const float q = processor.apvts.getRawParameterValue ("Pinch")->load();
    const float step = wheel.deltaY * (wheel.isReversed ? -1.0f : 1.0f);
    pinchAttachment.setValueAsCompleteGesture (
        juce::jlimit (0.5f, 20.0f, q * std::exp2 (step * 2.0f)));
}
