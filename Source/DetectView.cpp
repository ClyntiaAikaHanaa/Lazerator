#include "DetectView.h"
#include "LazerLook.h"
#include <cmath>

DetectView::DetectView (LazeratorAudioProcessor& p)
    : processor (p),
      threshAttachment (*p.apvts.getParameter ("Threshold"),
                        [this] (float v) { threshDb = v; repaint(); })
{
    fastDb.fill (kFloorDb);
    slowDb.fill (kFloorDb);
    threshAttachment.sendInitialUpdate();
    startTimerHz (30);
}

void DetectView::timerCallback()
{
    if (! isShowing())
        return;

    auto& dsp = processor.getDsp();

    auto toDb = [] (float v)
    {
        return juce::jlimit (kFloorDb, 0.0f, 20.0f * std::log10 (v + 1.0e-6f));
    };

    fastDb[(size_t) head] = toDb (dsp.consumeEnvFastPeak());
    slowDb[(size_t) head] = toDb (dsp.getEnvSlowValue());
    head = (head + 1) % kHistLen;

    const float dt = 1.0f / 30.0f;
    const int tc = dsp.getTriggerCount();
    for (int k = 0; k < juce::jmin (tc - lastTrigCount, 6); ++k)
        markers.push_back ({ (float) k * 0.01f, false });
    lastTrigCount = tc;

    const int rc = dsp.getRejectCount();
    for (int k = 0; k < juce::jmin (rc - lastRejectCount, 6); ++k)
        markers.push_back ({ (float) k * 0.01f, true });
    lastRejectCount = rc;

    for (auto& m : markers)
        m.age += dt;
    markers.erase (std::remove_if (markers.begin(), markers.end(),
                                   [] (const Marker& m) { return m.age > kTimeWindow; }),
                   markers.end());

    repaint();
}

float DetectView::dbToY (float db, juce::Rectangle<float> area) const
{
    return juce::jmap (juce::jlimit (kFloorDb, 0.0f, db),
                       kFloorDb, 0.0f, area.getBottom(), area.getY());
}

float DetectView::yToDb (float y, juce::Rectangle<float> area) const
{
    return juce::jmap (juce::jlimit (area.getY(), area.getBottom(), y),
                       area.getBottom(), area.getY(), kFloorDb, 0.0f);
}

void DetectView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (lazer::panel);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (lazer::ink.withAlpha (0.18f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    plotArea = bounds.reduced (6.0f, 6.0f);
    const auto plot = plotArea;

    // grid dB samar
    g.setFont (lazer::hudFont (8.0f));
    for (const float db : { -12.0f, -24.0f, -36.0f, -48.0f, -60.0f })
    {
        const float y = dbToY (db, plot);
        g.setColour (lazer::ink.withAlpha (0.06f));
        g.fillRect (plot.getX(), y, plot.getWidth(), 1.0f);
    }

    const float sens = processor.apvts.getRawParameterValue ("Sensitivity")->load();

    auto xAt = [&] (int k)
    {
        return plot.getX() + (float) k / (float) (kHistLen - 1) * plot.getWidth();
    };
    auto atHist = [&] (const std::array<float, (size_t) kHistLen>& a, int k)
    {
        return a[(size_t) ((head + k) % kHistLen)];   // k=0 tertua, k=len-1 terbaru
    };

    // envelope lambat: isian lembut teal
    {
        juce::Path slow;
        slow.startNewSubPath (plot.getX(), plot.getBottom());
        for (int k = 0; k < kHistLen; ++k)
            slow.lineTo (xAt (k), dbToY (atHist (slowDb, k), plot));
        slow.lineTo (plot.getRight(), plot.getBottom());
        slow.closeSubPath();
        g.setColour (lazer::teal.withAlpha (0.14f));
        g.fillPath (slow);
    }

    // ambang dinamis (slow + Sensitivity): putus-putus teal — di sinilah trigger lahir
    {
        juce::Path sensPath;
        bool first = true;
        for (int k = 0; k < kHistLen; ++k)
        {
            const float y = dbToY (atHist (slowDb, k) + sens, plot);
            if (first) { sensPath.startNewSubPath (xAt (k), y); first = false; }
            else         sensPath.lineTo (xAt (k), y);
        }
        juce::Path dashed;
        const float dashes[] = { 4.0f, 3.0f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, sensPath, dashes, 2);
        g.setColour (lazer::teal.withAlpha (0.75f));
        g.fillPath (dashed);
    }

    // envelope cepat: garis tinta
    {
        juce::Path fast;
        bool first = true;
        for (int k = 0; k < kHistLen; ++k)
        {
            const float y = dbToY (atHist (fastDb, k), plot);
            if (first) { fast.startNewSubPath (xAt (k), y); first = false; }
            else         fast.lineTo (xAt (k), y);
        }
        g.setColour (lazer::ink.withAlpha (0.85f));
        g.strokePath (fast, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Threshold absolut: garis pink draggable, ala kompresor
    {
        const float y = dbToY (threshDb, plot);
        g.setColour (lazer::pink.withAlpha (hoverThresh || draggingThresh ? 1.0f : 0.75f));
        g.fillRect (plot.getX(), y - 0.8f, plot.getWidth(), 1.6f);
        g.fillEllipse (plot.getRight() - 10.0f, y - 4.0f, 8.0f, 8.0f);

        g.setFont (lazer::hudFont (9.0f));
        g.drawText ("THR " + juce::String (threshDb, 1) + "dB",
                    (int) plot.getX() + 4, (int) (y - 13.0f), 90, 10,
                    juce::Justification::left);
    }

    // tick trigger di tepi atas
    for (const auto& m : markers)
    {
        const float x = plot.getRight() - (m.age / kTimeWindow) * plot.getWidth();
        const float a = 1.0f - m.age / kTimeWindow;
        g.setColour ((m.rejected ? lazer::amber : lazer::pink).withAlpha (a));
        g.fillRect (x - 1.0f, plot.getY(), 2.0f, m.rejected ? 5.0f : 9.0f);
    }
}

void DetectView::mouseDown (const juce::MouseEvent& e)
{
    if (hoverThresh || std::abs (e.position.y - dbToY (threshDb, plotArea)) < 9.0f)
    {
        draggingThresh = true;
        threshAttachment.beginGesture();
        threshAttachment.setValueAsPartOfGesture (
            juce::jlimit (-60.0f, 0.0f, yToDb (e.position.y, plotArea)));
    }
}

void DetectView::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingThresh)
        threshAttachment.setValueAsPartOfGesture (
            juce::jlimit (-60.0f, 0.0f, yToDb (e.position.y, plotArea)));
}

void DetectView::mouseUp (const juce::MouseEvent&)
{
    if (draggingThresh)
    {
        threshAttachment.endGesture();
        draggingThresh = false;
    }
}

void DetectView::mouseMove (const juce::MouseEvent& e)
{
    const bool hover = std::abs (e.position.y - dbToY (threshDb, plotArea)) < 9.0f;
    if (hover != hoverThresh)
    {
        hoverThresh = hover;
        setMouseCursor (hover ? juce::MouseCursor::UpDownResizeCursor
                              : juce::MouseCursor::NormalCursor);
        repaint();
    }
}
