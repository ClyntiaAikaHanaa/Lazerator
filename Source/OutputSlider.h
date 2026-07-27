#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Satu slider vertikal dengan dua garis yang masing-masing dapat ditarik
// naik-turun (§8.5):
//   - garis pink  = ambang clip (Drive). Melintang menutupi meter dan memakai
//     skala dB yang sama, jadi makin ke bawah makin banyak sinyal yang ter-clip.
//   - garis tinta = Output Gain, diterapkan sebelum clipper.
// Meter puncak L dan R berada di tepi kanan; bagian yang menembus ambang
// berubah merah.
class OutputSlider : public juce::Component,
                     private juce::Timer
{
public:
    explicit OutputSlider (LazeratorAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    enum class Handle { none, drive, gain };

    void timerCallback() override;
    juce::Rectangle<float> trackArea() const;
    juce::Rectangle<float> meterArea() const;

    // Skala dB bersama antara meter dan garis ambang Drive.
    float dbToY (float db) const;
    float yToDb (float y) const;

    float driveY() const;                    // posisi garis ambang Drive
    float gainY()  const;                    // posisi garis Out Gain
    Handle hitTest (juce::Point<float>) const;

    LazeratorAudioProcessor& processor;
    juce::RangedAudioParameter* driveParam = nullptr;
    juce::RangedAudioParameter* gainParam  = nullptr;
    juce::ParameterAttachment driveAttachment, gainAttachment;

    float driveValue = 2.0f, gainNorm = 0.5f;
    float meterLdb = -99.0f, meterRdb = -99.0f;

    Handle dragging = Handle::none, hovering = Handle::none;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputSlider)
};
