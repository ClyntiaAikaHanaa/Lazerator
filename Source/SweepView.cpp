#include "SweepView.h"
#include "LazerLook.h"
#include <cmath>

SweepView::SweepView (LazeratorAudioProcessor& p)
    : processor (p),
      curveAttachment (*p.apvts.getParameter ("Curve"),
                       [this] (float v) { curve = v; repaint(); })
{
    curveAttachment.sendInitialUpdate();

    for (const auto* id : { "SweepDepth", "SweepTime", "StereoMode", "TimeOffset", "SideDepth" })
        shapeAttachments.push_back (std::make_unique<juce::ParameterAttachment> (
            *p.apvts.getParameter (id), [this] (float) { repaint(); }));

    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    startTimerHz (30);
}

void SweepView::timerCallback()
{
    if (! isShowing())
        return;

    const float e = processor.getDsp().getEnvelopeValue();
    if (! juce::approximatelyEqual (e, env))
    {
        env = e;
        repaint();
    }
}

void SweepView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (lazer::panel);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (lazer::ink.withAlpha (hovering || dragging ? 0.30f : 0.15f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    // Baris label disisihkan lebih dulu supaya kurva tidak pernah menimpanya
    auto inner = bounds.reduced (9.0f, 6.0f);
    auto labelRow = inner.removeFromBottom (11.0f);
    auto plot = inner;

    auto& ap = processor.apvts;
    const float depth  = ap.getRawParameterValue ("SweepDepth")->load();
    const float timeMs = ap.getRawParameterValue ("SweepTime")->load();
    const int   mode   = (int) ap.getRawParameterValue ("StereoMode")->load();
    const float tOff   = ap.getRawParameterValue ("TimeOffset")->load();
    const float side   = ap.getRawParameterValue ("SideDepth")->load();

    const bool rising = depth < 0.0f;                 // §6.2: D < 0 menyapu naik
    const float axisY = rising ? plot.getY() : plot.getBottom();

    // Kisi waktu samar, agar kurva lurus tetap terbaca sebagai plot
    for (int i = 1; i < 4; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i * 0.25f;
        g.setColour (lazer::ink.withAlpha (0.06f));
        g.fillRect (x, plot.getY(), 1.0f, plot.getHeight());
    }

    // Garis nol oktaf
    g.setColour (lazer::ink.withAlpha (0.18f));
    g.fillRect (plot.getX(), axisY - 0.5f, plot.getWidth(), 1.0f);

    // Kurva mana saja yang perlu digambar
    struct Trace { float timeScale, depthScale; juce::Colour colour; const char* tag; };
    std::vector<Trace> traces;

    if (mode == 1 && ! juce::approximatelyEqual (tOff, 0.0f))       // Offset: beda durasi
    {
        const float o = tOff * 0.01f;
        traces.push_back ({ 1.0f - 0.5f * o, 1.0f, lazer::pink, "L" });
        traces.push_back ({ 1.0f + 0.5f * o, 1.0f, lazer::teal, "R" });
    }
    else if (mode == 2 && ! juce::approximatelyEqual (side, 100.0f)) // Mid-Side: beda kedalaman
    {
        traces.push_back ({ 1.0f, 1.0f, lazer::pink, "M" });
        traces.push_back ({ 1.0f, side * 0.01f, lazer::teal, "S" });
    }
    else
        traces.push_back ({ 1.0f, 1.0f, lazer::pink, nullptr });

    // Skala dinormalisasi ke kurva terpanjang dan tertinggi, agar Side Depth
    // di atas 100% tidak terpotong tepi atas.
    float maxTime = 1.0f, maxDepth = 1.0f;
    for (const auto& t : traces)
    {
        maxTime  = juce::jmax (maxTime, t.timeScale);
        maxDepth = juce::jmax (maxDepth, t.depthScale);
    }
    const float span = plot.getHeight() / maxDepth;

    auto shape = [this] (float u)
    {
        return u >= 1.0f ? 0.0f : std::pow (1.0f - u, curve);
    };

    for (const auto& t : traces)
    {
        juce::Path path;
        const float widthFrac = t.timeScale / maxTime;
        bool first = true;

        for (float px = 0.0f; px <= 1.0001f; px += 0.02f)
        {
            const float u = juce::jmin (1.0f, px / widthFrac);
            const float y = axisY + (rising ? 1.0f : -1.0f) * shape (u) * t.depthScale * span;
            const float x = plot.getX() + juce::jmin (1.0f, px) * plot.getWidth();
            if (first) { path.startNewSubPath (x, y); first = false; }
            else         path.lineTo (x, y);
        }

        // Isian menuju sumbu memberi bobot pada bentuk; pada Curve 1,0 kurvanya
        // memang lurus (linier dalam oktaf) dan tanpa isian terlihat seperti cacat.
        juce::Path fill (path);
        fill.lineTo (plot.getRight(), axisY);
        fill.lineTo (plot.getX(), axisY);
        fill.closeSubPath();
        g.setColour (t.colour.withAlpha (0.13f));
        g.fillPath (fill);

        g.setColour (t.colour.withAlpha (traces.size() > 1 ? 0.9f : 1.0f));
        g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        if (t.tag != nullptr)
        {
            const float y = juce::jlimit (plot.getY() + 5.0f, plot.getBottom() - 5.0f,
                                          axisY + (rising ? 1.0f : -1.0f) * t.depthScale * span);
            g.setFont (lazer::hudFont (8.5f));
            g.setColour (t.colour);
            g.drawText (t.tag, (int) plot.getX() + 3, (int) y - 5, 12, 10,
                        juce::Justification::left);
        }
    }

    // Titik berjalan saat sweep aktif
    if (env > 0.001f)
    {
        const float u = curve > 0.0f ? 1.0f - std::pow (env, 1.0f / curve) : 0.0f;
        const float x = plot.getX() + juce::jlimit (0.0f, 1.0f, u) * plot.getWidth() / maxTime;
        const float y = axisY + (rising ? 1.0f : -1.0f) * env * span;
        g.setColour (lazer::pink);
        g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }

    // Pembacaan pada baris terpisah di bawah plot
    g.setFont (lazer::hudFont (8.5f));
    g.setColour (lazer::inkSoft);
    g.drawText ((depth > 0 ? "+" : "") + juce::String (depth, 1) + " oct",
                labelRow.toNearestInt(), juce::Justification::centredLeft);
    g.drawText (timeMs < 1000.0f ? juce::String ((int) timeMs) + " ms"
                                 : juce::String (timeMs / 1000.0f, 2) + " s",
                labelRow.toNearestInt(), juce::Justification::centredRight);
}

//==============================================================================

void SweepView::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    dragStartY = e.position.y;
    dragStartCurve = curve;
    curveAttachment.beginGesture();
}

void SweepView::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    // Menarik ke atas membuat kurva menggembung — peluruhan awal lebih lambat (c < 1)
    const float delta = e.position.y - dragStartY;
    curveAttachment.setValueAsPartOfGesture (
        juce::jlimit (0.25f, 4.0f, dragStartCurve * std::exp2 (delta * 0.02f)));
}

void SweepView::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
        curveAttachment.endGesture();
    dragging = false;
}

void SweepView::mouseEnter (const juce::MouseEvent&) { hovering = true;  repaint(); }
void SweepView::mouseExit  (const juce::MouseEvent&) { hovering = false; repaint(); }

void SweepView::mouseDoubleClick (const juce::MouseEvent&)
{
    curveAttachment.setValueAsCompleteGesture (1.0f);
}
