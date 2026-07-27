#include "LazerLook.h"

namespace lazer
{

void drawCard (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill)
{
    juce::Path p;
    p.addRoundedRectangle (r, radius);
    juce::DropShadow shadow (juce::Colour (0x26000000), 10, { 0, 3 });
    shadow.drawForPath (g, p);
    g.setColour (fill);
    g.fillPath (p);
}

juce::Font hudFont (float height)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                          height, juce::Font::plain));
}

juce::Font uiFont (float height, bool bold)
{
    auto options = juce::FontOptions (height);
    if (bold)
        options = options.withStyle ("Bold");
    return juce::Font (options);
}

} // namespace lazer

//==============================================================================

LazerLookAndFeel::LazerLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, lazer::ink);
    setColour (juce::Slider::thumbColourId, lazer::pink);
    setColour (juce::Label::textColourId, lazer::ink);

    setColour (juce::PopupMenu::backgroundColourId, lazer::panel);
    setColour (juce::PopupMenu::textColourId, lazer::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, lazer::ink.withAlpha (0.12f));
    setColour (juce::PopupMenu::highlightedTextColourId, lazer::ink);

    setColour (juce::ToggleButton::textColourId, lazer::ink);
    setColour (juce::TextButton::buttonColourId, lazer::panel);
    setColour (juce::TextButton::textColourOffId, lazer::ink);
    setColour (juce::TextButton::textColourOnId, lazer::ink);

    setColour (juce::TooltipWindow::backgroundColourId, lazer::panel);
    setColour (juce::TooltipWindow::textColourId, lazer::ink);
}

void LazerLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 4.0f;
    if (radius < 10.0f)
        return;

    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float thickness = juce::jmax (2.2f, radius * 0.085f);
    const float arcR = radius - thickness * 0.5f;

    // Jalur latar (tipis, tinta pudar)
    juce::Path track;
    track.addCentredArc (cx, cy, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (lazer::ink.withAlpha (0.14f));
    g.strokePath (track, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Busur nilai — kenop bipolar mengisi dari tengah
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float fromAngle = bipolar ? (rotaryStartAngle + rotaryEndAngle) * 0.5f : rotaryStartAngle;
    if (! juce::exactlyEqual (angle, fromAngle))
    {
        juce::Path value;
        value.addCentredArc (cx, cy, arcR, arcR, 0.0f,
                             juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (value, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Titik ujung sebagai penunjuk aksen
    const float dotR = juce::jmax (2.5f, thickness * 0.9f);
    const juce::Point<float> tip (cx + arcR * std::sin (angle),
                                  cy - arcR * std::cos (angle));
    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillEllipse (tip.x - dotR, tip.y - dotR, dotR * 2.0f, dotR * 2.0f);

    // Nilai di dalam kenop (gaya referensi: angka di tengah, label di bawah)
    const auto text = slider.getTextFromValue (slider.getValue());
    g.setColour (lazer::ink);
    g.setFont (lazer::uiFont (radius > 40.0f ? 15.0f : radius > 28.0f ? 12.0f : 10.5f));
    g.drawText (text, area.reduced (thickness * 2.0f).toNearestInt(),
                juce::Justification::centred);
}

void LazerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                         bool shouldDrawButtonAsHighlighted, bool)
{
    const auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const float rad = r.getHeight() * 0.5f;
    const bool on = b.getToggleState();

    if (on)
    {
        g.setColour (lazer::ink);
        g.fillRoundedRectangle (r, rad);
        g.setColour (lazer::panel);
    }
    else
    {
        g.setColour (lazer::ink.withAlpha (shouldDrawButtonAsHighlighted ? 0.55f : 0.35f));
        g.drawRoundedRectangle (r, rad, 1.2f);
        g.setColour (lazer::inkSoft);
    }

    g.setFont (lazer::uiFont (11.0f));
    g.drawText (b.getButtonText(), r.toNearestInt(), juce::Justification::centred);
}

void LazerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    const auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const float rad = r.getHeight() * 0.5f;

    g.setColour (shouldDrawButtonAsDown ? lazer::card : lazer::panel);
    g.fillRoundedRectangle (r, rad);
    g.setColour (lazer::ink.withAlpha (shouldDrawButtonAsHighlighted ? 0.55f : 0.30f));
    g.drawRoundedRectangle (r, rad, 1.2f);
}

juce::Font LazerLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return lazer::uiFont (11.0f);
}

//==============================================================================
// Menu preset

int LazerLookAndFeel::getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&)
{
    return 6;
}

void LazerLookAndFeel::drawPopupMenuBackgroundWithOptions (juce::Graphics& g, int width, int height,
                                                           const juce::PopupMenu::Options&)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
    g.setColour (lazer::panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (lazer::ink.withAlpha (0.22f));
    g.drawRoundedRectangle (r, 8.0f, 1.0f);
}

void LazerLookAndFeel::getIdealPopupMenuItemSizeWithOptions (const juce::String& text,
                                                             bool isSeparator,
                                                             int standardMenuItemHeight,
                                                             int& idealWidth, int& idealHeight,
                                                             const juce::PopupMenu::Options&)
{
    if (isSeparator)
    {
        idealWidth = 60;
        idealHeight = 9;
        return;
    }

    const auto font = lazer::uiFont (12.5f);
    idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight : 24;
    idealHeight = juce::jmax (24, idealHeight);
    idealWidth = juce::GlyphArrangement::getStringWidthInt (font, text) + idealHeight + 30;
}

void LazerLookAndFeel::drawPopupMenuItemWithOptions (juce::Graphics& g,
                                                     const juce::Rectangle<int>& area,
                                                     bool isHighlighted,
                                                     const juce::PopupMenu::Item& item,
                                                     const juce::PopupMenu::Options&)
{
    if (item.isSeparator)
    {
        auto line = area.reduced (10, 0).withHeight (1).withY (area.getCentreY());
        g.setColour (lazer::ink.withAlpha (0.15f));
        g.fillRect (line);
        return;
    }

    auto r = area.reduced (4, 1);

    if (isHighlighted && item.isEnabled)
    {
        g.setColour (lazer::ink);
        g.fillRoundedRectangle (r.toFloat(), 5.0f);
    }

    const auto textColour = ! item.isEnabled ? lazer::inkSoft.withAlpha (0.5f)
                          : isHighlighted    ? lazer::panel
                                             : lazer::ink;
    g.setColour (textColour);
    g.setFont (lazer::uiFont (12.5f));

    auto textArea = r.reduced (10, 0);

    // Tanda centang untuk item aktif — kolom kiri tetap, agar teks tidak bergeser
    auto tickArea = textArea.removeFromLeft (14);
    if (item.isTicked)
    {
        juce::Path tick;
        const auto t = tickArea.toFloat().withSizeKeepingCentre (8.0f, 8.0f);
        tick.startNewSubPath (t.getX(), t.getCentreY());
        tick.lineTo (t.getCentreX() - 1.0f, t.getBottom());
        tick.lineTo (t.getRight(), t.getY());
        g.setColour (isHighlighted ? lazer::panel : lazer::pink);
        g.strokePath (tick, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        g.setColour (textColour);
    }
    textArea.removeFromLeft (4);

    if (item.subMenu != nullptr)
    {
        auto arrowArea = textArea.removeFromRight (14).toFloat();
        juce::Path arrow;
        const float ax = arrowArea.getCentreX() - 2.0f;
        const float ay = arrowArea.getCentreY();
        arrow.startNewSubPath (ax, ay - 4.0f);
        arrow.lineTo (ax + 4.0f, ay);
        arrow.lineTo (ax, ay + 4.0f);
        g.setColour (isHighlighted ? lazer::panel : lazer::inkSoft);
        g.strokePath (arrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        g.setColour (textColour);
    }

    g.drawText (item.text, textArea, juce::Justification::centredLeft, true);
}
