#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LazerLook.h"
#include "Visualizer.h"
#include "DetectView.h"
#include "PresetBar.h"
#include "CreditsView.h"
#include "OutputSlider.h"
#include "SweepView.h"
#include "Controls.h"

// Seluruh UI ditata pada kanvas berukuran tetap (canvasWidth x canvasHeight).
// Editor hanya menskalakannya lewat AffineTransform, sehingga setiap elemen —
// kenop, garis, dan teks tombol — ikut membesar secara proporsional (§9.4).
class MainPanel : public juce::Component
{
public:
    static constexpr int canvasWidth  = 980;
    static constexpr int canvasHeight = 700;

    explicit MainPanel (LazeratorAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using Formatter        = std::function<juce::String (double)>;

    struct Knob
    {
        ValueSlider slider;              // klik ganda = ketik nilainya
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<ButtonAttachment> attachment;
    };

    void addKnob   (Knob&, const juce::String& name, const juce::String& paramID, Formatter fmt);
    void addToggle (Toggle&, const juce::String& name, const juce::String& paramID);
    void addSeg    (std::unique_ptr<SegmentedControl>&, juce::Label&,
                    const juce::String& name, const juce::String& paramID);

    // Kenop dipusatkan sebagai lingkaran berdiameter tetap di dalam selnya,
    // sehingga jarak antar-kenop tetap merata berapa pun lebar sel.
    void layoutKnob (Knob&, juce::Rectangle<int> cell, int diameter = 0);

    LazeratorAudioProcessor& audioProcessor;

    PresetBar   mPresetBar;
    Visualizer  mVisualizer;
    DetectView  mDetectView;
    CreditsView mCredits;
    ClickableArea mTitleArea;

    // Baris utama (§9.2) — Threshold ada sebagai garis drag di DetectView
    Knob mPinch, mAmount, mBase, mDepth, mTime;

    // DETECT (§8.3)
    Knob mSens, mHold, mScHpf, mScLpf;
    std::unique_ptr<SegmentedControl> mSourceSeg;
    juce::Label mSourceLabel;
    HeadphoneToggle mScListen;
    std::unique_ptr<ButtonAttachment> mScListenAttachment;

    // MOD / STEREO (§8.2, §8.4)
    std::unique_ptr<SegmentedControl> mRetrigSeg, mStereoSeg;
    juce::Label mRetrigLabel, mStereoLabel;
    Knob mCurve, mSpread, mTimeOff, mSideDepth;
    SweepView mSweepView;

    // OUTPUT (§8.5) — Drive & Output Gain jadi satu slider dua-garis
    Knob mDryWet;
    OutputSlider mOutputSlider;
    Toggle mSoftClip, mAutoGain, mPhaseInvert;

    // Phase Invert tidak dapat berbuat apa pun pada Dry/Wet 100%: membalik
    // polaritas seluruh keluaran tidak mengubah bunyi. Tombolnya dinonaktifkan
    // di sana agar tidak terlihat rusak.
    std::unique_ptr<juce::ParameterAttachment> mMixWatcher;

    // Menampilkan nama nada di bawah kenop Base saat frekuensinya tepat di nada
    std::unique_ptr<juce::ParameterAttachment> mBaseNoteWatcher;

    juce::Rectangle<int> mKnobCard, mDetectCard, mModCard, mOutCard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};

class LazeratorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit LazeratorAudioProcessorEditor (LazeratorAudioProcessor&);
    ~LazeratorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    LazerLookAndFeel lookAndFeel;
    MainPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LazeratorAudioProcessorEditor)
};
