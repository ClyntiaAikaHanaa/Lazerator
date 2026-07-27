#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Tampilan bentuk sweep: lintasan modulasi D_eff * g(u) dalam oktaf (§6.2).
//  - Curve diubah dengan menarik kurva naik-turun
//  - mode Offset menggambar kurva L dan R terpisah (beda durasi, §6.7)
//  - mode Mid-Side menggambar kurva M dan S terpisah (beda kedalaman)
//  - titik berjalan menyusuri kurva setiap kali trigger menyala
class SweepView : public juce::Component,
                  private juce::Timer
{
public:
    explicit SweepView (LazeratorAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit  (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    LazeratorAudioProcessor& processor;
    juce::ParameterAttachment curveAttachment;

    // Parameter lain yang ikut digambar; tanpa attachment ini tampilan hanya
    // menyegar saat Curve diputar atau saat envelope berjalan.
    std::vector<std::unique_ptr<juce::ParameterAttachment>> shapeAttachments;

    float curve = 1.0f;
    float env = 0.0f;

    bool dragging = false, hovering = false;
    float dragStartY = 0.0f, dragStartCurve = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SweepView)
};
