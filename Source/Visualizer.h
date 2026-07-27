#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Visualizer utama (§9.3), gaya tinta terang:
//  - latar: spektrum keluaran sebagai sapuan tinta lembut
//  - depan: kurva group delay (tinta tebal) yang bergerak mengikuti sweep
//  - node pink draggable: X = Base Frequency, Y = Amount
//  - node teal draggable: posisi frekuensi awal sweep = Depth (arah + besar)
//  - garis f0 sesaat + jejak, tick trigger, readout GD/Start, meter korelasi
class Visualizer : public juce::Component,
                   private juce::Timer
{
public:
    explicit Visualizer (LazeratorAudioProcessor&);
    ~Visualizer() override = default;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    float freqToX (float freq, juce::Rectangle<float> area) const;
    float xToFreq (float x, juce::Rectangle<float> area) const;

    LazeratorAudioProcessor& processor;

    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;   // 2048

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, (size_t) fftSize> ring {};
    int ringPos = 0;

    std::array<float, (size_t) fftSize * 2> fftData {};
    std::array<float, (size_t) fftSize / 2 + 1> smoothDb {};

    std::vector<float> hitAges;          // tick trigger diterima (3 detik)
    int lastTrigCount = 0;
    float triggerFlash = 0.0f;

    struct TrailPoint { float freq = 0.0f; float age = 0.0f; };
    std::vector<TrailPoint> trail;

    // Interaksi node
    juce::ParameterAttachment baseAttachment, amountAttachment, depthAttachment, pinchAttachment;
    enum class Drag { none, baseNode, startNode };
    Drag dragTarget = Drag::none;
    bool hoverBase = false, hoverStart = false;

    juce::Rectangle<float> plotArea;
    juce::Point<float> baseNodePos, startNodePos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Visualizer)
};
