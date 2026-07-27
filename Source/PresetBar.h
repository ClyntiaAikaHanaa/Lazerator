#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Bilah preset (§3.1): mundur/maju, nama (klik = menu berkategori),
// simpan preset pengguna, dan A/B compare.
class PresetBar : public juce::Component,
                  private juce::Timer
{
public:
    explicit PresetBar (LazeratorAudioProcessor&);
    ~PresetBar() override = default;

    void resized() override;

private:
    void timerCallback() override;
    void showMenu();
    void savePreset();

    LazeratorAudioProcessor& processor;

    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::TextButton nameButton;
    juce::TextButton saveButton { "Save" };
    juce::TextButton abButton { "A" };

    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
