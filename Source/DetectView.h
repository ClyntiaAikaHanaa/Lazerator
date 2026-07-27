#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Tampilan detektor ala kompresor (§9.3):
//  - riwayat envelope cepat (garis tinta) & lambat (isian teal) dalam dB, bergulir
//  - garis putus-putus teal = ambang dinamis (slow + Sensitivity)
//  - garis pink = Threshold absolut, DAPAT DI-DRAG vertikal seperti kompresor
//  - tick atas: pink = trigger diterima, amber = ditolak Hold/mode
class DetectView : public juce::Component,
                   private juce::Timer
{
public:
    explicit DetectView (LazeratorAudioProcessor&);
    ~DetectView() override = default;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    float dbToY (float db, juce::Rectangle<float> area) const;
    float yToDb (float y, juce::Rectangle<float> area) const;

    static constexpr float kFloorDb   = -70.0f;
    static constexpr float kTimeWindow = 3.0f;   // detik
    static constexpr int   kHistLen    = 150;

    LazeratorAudioProcessor& processor;
    juce::ParameterAttachment threshAttachment;
    float threshDb = -24.0f;

    std::array<float, (size_t) kHistLen> fastDb {}, slowDb {};
    int head = 0;

    struct Marker { float age = 0.0f; bool rejected = false; };
    std::vector<Marker> markers;
    int lastTrigCount = 0, lastRejectCount = 0;

    bool draggingThresh = false, hoverThresh = false;
    juce::Rectangle<float> plotArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DetectView)
};
