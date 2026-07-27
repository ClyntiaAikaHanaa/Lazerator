#pragma once
#include <JuceHeader.h>

// Palet & bentuk dasar UI Lazerator — tema terang kalem:
// lavender pudar, goresan tinta gelap, aksen pink/teal yang dipakai hemat (§9.4).
namespace lazer
{
    const juce::Colour bg      { 0xffb9b6ca };   // latar jendela
    const juce::Colour card    { 0xffd7d5e3 };   // kartu kelompok kontrol
    const juce::Colour panel   { 0xffefeef5 };   // panel display (paling terang)
    const juce::Colour ink     { 0xff2b2836 };   // goresan & teks utama
    const juce::Colour inkSoft { 0xff6d6a7a };   // label sekunder
    const juce::Colour pink    { 0xffef4d7e };   // aksen aktif / laser
    const juce::Colour teal    { 0xff2aaf9f };   // aksen sekunder
    const juce::Colour amber   { 0xffdd9420 };   // penanda trigger ditolak
    const juce::Colour warnCol { 0xffd93a3a };   // peringatan (GD > 500 ms)

    // Kartu membulat dengan bayangan lembut.
    void drawCard (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill);

    juce::Font hudFont (float height);                  // mono, untuk angka readout
    juce::Font uiFont  (float height, bool bold = false);
}

class LazerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LazerLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    // --- menu preset (§9.1: kontrol lanjutan disembunyikan, bukan dihilangkan)
    void drawPopupMenuBackgroundWithOptions (juce::Graphics&, int width, int height,
                                             const juce::PopupMenu::Options&) override;

    void drawPopupMenuItemWithOptions (juce::Graphics&, const juce::Rectangle<int>& area,
                                       bool isHighlighted, const juce::PopupMenu::Item&,
                                       const juce::PopupMenu::Options&) override;

    void getIdealPopupMenuItemSizeWithOptions (const juce::String& text, bool isSeparator,
                                               int standardMenuItemHeight,
                                               int& idealWidth, int& idealHeight,
                                               const juce::PopupMenu::Options&) override;

    int getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&) override;
};
