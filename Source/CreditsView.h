#pragma once
#include <JuceHeader.h>

// Jendela kredit — dibuka dengan mengklik nama "Lazerator" di kiri atas.
// Menutup dengan klik di luar kartu atau tombol Escape.
class CreditsView : public juce::Component
{
public:
    CreditsView();

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void visibilityChanged() override;

private:
    juce::Rectangle<float> cardBounds() const;

    juce::Image logo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CreditsView)
};
