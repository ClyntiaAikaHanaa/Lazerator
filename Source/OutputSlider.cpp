#include "OutputSlider.h"
#include "LazerLook.h"
#include <cmath>

namespace
{
    // Skala dB bersama: meter L/R dan garis ambang Drive memakai sumbu yang
    // sama, sehingga terlihat langsung kapan sinyal menembus ambang.
    constexpr float kScaleTopDb = 6.0f;
    constexpr float kScaleBotDb = -42.0f;

    constexpr float kLetterRow  = 12.0f;     // baris huruf L / R di bawah meter
    constexpr float kMeterW     = 8.0f;
    constexpr float kMeterGap   = 6.0f;
    constexpr float kTrackW     = 30.0f;
    constexpr float kHandleGrab = 8.0f;
    constexpr float kLabelGap   = 25.0f;     // jarak minimum antar-blok label

    float thresholdDbFor  (float drive) { return (float) LazerDSP::driveToThresholdDb (drive); }
    float driveForThresholdDb (float db) { return (float) LazerDSP::thresholdDbToDrive (db); }
}

OutputSlider::OutputSlider (LazeratorAudioProcessor& p)
    : processor (p),
      driveParam (p.apvts.getParameter ("Drive")),
      gainParam  (p.apvts.getParameter ("OutputGain")),
      driveAttachment (*p.apvts.getParameter ("Drive"),
                       [this] (float v) { driveValue = v; repaint(); }),
      gainAttachment  (*p.apvts.getParameter ("OutputGain"),
                       [this] (float v) { gainNorm = gainParam->convertTo0to1 (v); repaint(); })
{
    driveAttachment.sendInitialUpdate();
    gainAttachment.sendInitialUpdate();
    startTimerHz (30);
}

void OutputSlider::timerCallback()
{
    if (! isShowing())
        return;

    auto& dsp = processor.getDsp();
    auto update = [] (float& shown, float peak)
    {
        const float db = juce::jlimit (kScaleBotDb - 6.0f, kScaleTopDb,
                                       20.0f * std::log10 (peak + 1.0e-6f));
        shown = db > shown ? db : shown + (db - shown) * 0.25f;   // jatuh perlahan
    };

    update (meterLdb, dsp.consumeOutPeakL());
    update (meterRdb, dsp.consumeOutPeakR());
    repaint();
}

// Tata letak: [ label ][ track ][ meter L R ] — meter menempel di tepi kanan.
juce::Rectangle<float> OutputSlider::trackArea() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromBottom (kLetterRow);
    r.removeFromRight (kMeterW * 2.0f + kMeterGap + 9.0f);
    return r.removeFromRight (kTrackW).reduced (0.0f, 4.0f);
}

juce::Rectangle<float> OutputSlider::meterArea() const
{
    const auto t = trackArea();
    const float w = kMeterW * 2.0f + kMeterGap;
    return { getLocalBounds().toFloat().getRight() - w, t.getY(), w, t.getHeight() };
}

float OutputSlider::dbToY (float db) const
{
    const auto t = trackArea();
    return juce::jmap (juce::jlimit (kScaleBotDb, kScaleTopDb, db),
                       kScaleBotDb, kScaleTopDb, t.getBottom(), t.getY());
}

float OutputSlider::yToDb (float y) const
{
    const auto t = trackArea();
    return juce::jmap (juce::jlimit (t.getY(), t.getBottom(), y),
                       t.getBottom(), t.getY(), kScaleBotDb, kScaleTopDb);
}

float OutputSlider::driveY() const
{
    return dbToY (thresholdDbFor (driveValue));
}

float OutputSlider::gainY() const
{
    const auto t = trackArea();
    return juce::jmap (juce::jlimit (0.0f, 1.0f, gainNorm), 0.0f, 1.0f,
                       t.getBottom(), t.getY());
}

OutputSlider::Handle OutputSlider::hitTest (juce::Point<float> pos) const
{
    if (! trackArea().expanded (14.0f, kHandleGrab).contains (pos))
        return Handle::none;

    const float dDrive = std::abs (pos.y - driveY());
    const float dGain  = std::abs (pos.y - gainY());
    if (juce::jmin (dDrive, dGain) > kHandleGrab)
        return Handle::none;
    return dDrive <= dGain ? Handle::drive : Handle::gain;
}

//==============================================================================

void OutputSlider::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto track = trackArea();
    const auto meterCol = meterArea();

    const float dY = driveY();
    const float gY = gainY();

    // --- meter L / R pada skala bersama
    const float meters[] = { meterLdb, meterRdb };
    for (int i = 0; i < 2; ++i)
    {
        auto bar = juce::Rectangle<float> (meterCol.getX() + (float) i * (kMeterW + kMeterGap),
                                           meterCol.getY(), kMeterW, meterCol.getHeight());

        g.setColour (lazer::ink.withAlpha (0.10f));
        g.fillRoundedRectangle (bar, kMeterW * 0.5f);

        const float norm = juce::jlimit (0.0f, 1.0f,
                                         (meters[i] - kScaleBotDb) / (kScaleTopDb - kScaleBotDb));
        if (norm > 0.001f)
        {
            auto fill = bar.withTrimmedTop (bar.getHeight() * (1.0f - norm));
            // Bagian yang menembus ambang Drive digambar merah: itulah yang ter-clip
            g.setColour (lazer::teal);
            g.fillRoundedRectangle (fill, kMeterW * 0.5f);

            if (fill.getY() < dY)
            {
                g.setColour (lazer::warnCol);
                g.fillRoundedRectangle (fill.withBottom (juce::jmin (dY, fill.getBottom())),
                                        kMeterW * 0.5f);
            }
        }

        g.setColour (lazer::inkSoft);
        g.setFont (lazer::hudFont (8.5f));
        g.drawText (i == 0 ? "L" : "R",
                    bar.withY (bounds.getBottom() - kLetterRow).withHeight (kLetterRow).toNearestInt(),
                    juce::Justification::centred);
    }

    // --- track
    const float trackR = kTrackW * 0.35f;
    g.setColour (lazer::ink.withAlpha (0.08f));
    g.fillRoundedRectangle (track, trackR);

    // Zona di ATAS garis Drive adalah wilayah yang ter-clip
    {
        juce::Path clipPath;
        clipPath.addRoundedRectangle (track, trackR);
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (clipPath);
        g.setColour (lazer::pink.withAlpha (0.14f));
        g.fillRect (track.getX(), track.getY(), track.getWidth(), dY - track.getY());
    }

    // Penanda 0 dBFS, ditarik melintasi meter juga agar terbaca sebagai batas
    // skala penuh untuk keduanya
    {
        const float zeroY = dbToY (0.0f);
        g.setColour (lazer::ink.withAlpha (0.22f));
        g.fillRect (track.getX() + 3.0f, zeroY - 0.5f,
                    meterCol.getRight() - track.getX() - 3.0f, 1.0f);
    }

    g.setColour (lazer::ink.withAlpha (0.18f));
    g.drawRoundedRectangle (track, trackR, 1.0f);

    // --- garis Out Gain (hanya di atas track)
    {
        const bool active = hovering == Handle::gain || dragging == Handle::gain;
        const auto bar = juce::Rectangle<float> (track.getX() - 5.0f, gY - 2.0f,
                                                 track.getWidth() + 10.0f, 4.0f);
        if (active)
        {
            g.setColour (lazer::ink.withAlpha (0.28f));
            g.fillRoundedRectangle (bar.expanded (2.0f, 3.0f), 4.0f);
        }
        g.setColour (lazer::ink);
        g.fillRoundedRectangle (bar, 2.0f);
    }

    // --- garis ambang Drive: melintang sampai menutupi meter, seperti garis
    // threshold pada kompresor. Makin ke bawah, makin banyak yang ter-clip.
    {
        const bool active = hovering == Handle::drive || dragging == Handle::drive;
        const auto bar = juce::Rectangle<float> (track.getX() - 5.0f, dY - 1.5f,
                                                 meterCol.getRight() - track.getX() + 5.0f, 3.0f);
        if (active)
        {
            g.setColour (lazer::pink.withAlpha (0.28f));
            g.fillRoundedRectangle (bar.expanded (2.0f, 3.0f), 4.0f);
        }
        g.setColour (lazer::pink);
        g.fillRoundedRectangle (bar, 1.5f);
    }

    // --- label mengikuti garisnya, saling didorong bila terlalu rapat
    const float labelW = track.getX() - bounds.getX() - 10.0f;
    const float labelX = bounds.getX();

    const float gainVal = gainParam->convertFrom0to1 (gainNorm);
    const float threshDb = thresholdDbFor (driveValue);

    struct Label { float y; juce::Colour colour; juce::String name, value; };
    Label labels[2] =
    {
        { dY, lazer::pink,    "Clip",
          (threshDb > 0.0f ? "+" : "") + juce::String (threshDb, 1) + "dB" },
        { gY, lazer::inkSoft, "Out",
          (gainVal > 0.0f ? "+" : "") + juce::String (gainVal, 1) + "dB" }
    };
    if (labels[0].y > labels[1].y)
        std::swap (labels[0], labels[1]);

    if (labels[1].y - labels[0].y < kLabelGap)
    {
        const float mid = (labels[0].y + labels[1].y) * 0.5f;
        labels[0].y = mid - kLabelGap * 0.5f;
        labels[1].y = mid + kLabelGap * 0.5f;
    }

    for (const auto& l : labels)
    {
        const float top = juce::jlimit (track.getY() - 2.0f, track.getBottom() - 23.0f, l.y - 11.0f);
        g.setColour (l.colour);
        g.setFont (lazer::uiFont (10.0f));
        g.drawText (l.name, juce::Rectangle<float> (labelX, top, labelW, 11.0f).toNearestInt(),
                    juce::Justification::centredRight);
        g.setColour (lazer::ink);
        g.setFont (lazer::hudFont (11.0f));
        g.drawText (l.value, juce::Rectangle<float> (labelX, top + 10.0f, labelW, 13.0f).toNearestInt(),
                    juce::Justification::centredRight);
    }
}

//==============================================================================

void OutputSlider::mouseDown (const juce::MouseEvent& e)
{
    dragging = hitTest (e.position);
    if (dragging == Handle::none)
        return;

    (dragging == Handle::drive ? driveAttachment : gainAttachment).beginGesture();
    mouseDrag (e);
}

void OutputSlider::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == Handle::drive)
    {
        // Menarik ke bawah menurunkan ambang: makin banyak sinyal yang ter-clip
        driveAttachment.setValueAsPartOfGesture (driveForThresholdDb (yToDb (e.position.y)));
    }
    else if (dragging == Handle::gain)
    {
        const auto t = trackArea();
        const float norm = juce::jlimit (0.0f, 1.0f,
            juce::jmap (juce::jlimit (t.getY(), t.getBottom(), e.position.y),
                        t.getBottom(), t.getY(), 0.0f, 1.0f));
        gainAttachment.setValueAsPartOfGesture (gainParam->convertFrom0to1 (norm));
    }
}

void OutputSlider::mouseUp (const juce::MouseEvent&)
{
    if (dragging != Handle::none)
        (dragging == Handle::drive ? driveAttachment : gainAttachment).endGesture();
    dragging = Handle::none;
}

void OutputSlider::mouseMove (const juce::MouseEvent& e)
{
    const auto h = hitTest (e.position);
    if (h != hovering)
    {
        hovering = h;
        setMouseCursor (h == Handle::none ? juce::MouseCursor::NormalCursor
                                          : juce::MouseCursor::UpDownResizeCursor);
        repaint();
    }
}

void OutputSlider::mouseExit (const juce::MouseEvent&)
{
    if (hovering != Handle::none)
    {
        hovering = Handle::none;
        repaint();
    }
}

void OutputSlider::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto h = hitTest (e.position);
    if (h == Handle::none)
        return;

    auto& att = h == Handle::drive ? driveAttachment : gainAttachment;
    auto* par = h == Handle::drive ? driveParam : gainParam;
    att.setValueAsCompleteGesture (par->convertFrom0to1 (par->getDefaultValue()));
}
