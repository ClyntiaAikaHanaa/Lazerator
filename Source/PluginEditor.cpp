#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // Formatter nilai kompak — tampil di dalam kenop, satuan menyatu
    juce::String fmtFreq (double v)
    {
        return v < 1000.0 ? juce::String ((int) std::lround (v)) + "Hz"
                          : juce::String (v / 1000.0, 1) + "kHz";
    }
    juce::String fmtMs (double v)
    {
        return v < 1000.0 ? juce::String ((int) std::lround (v)) + "ms"
                          : juce::String (v / 1000.0, 2) + "s";
    }
    juce::String fmtDb      (double v) { return juce::String (v, 1) + "dB"; }
    juce::String fmtOct     (double v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + "oct"; }
    juce::String fmtSt      (double v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + "st"; }
    juce::String fmtPct     (double v) { return juce::String ((int) std::lround (v)) + "%"; }
    juce::String fmtPctSign (double v) { return (v > 0 ? "+" : "") + juce::String ((int) std::lround (v)) + "%"; }
    juce::String fmtPlain   (double v) { return juce::String (v, 1); }
    juce::String fmtPlain2  (double v) { return juce::String (v, 2); }
    juce::String fmtInt     (double v) { return juce::String ((int) std::lround (v)); }
}

//==============================================================================
// MainPanel — kanvas ukuran tetap

MainPanel::MainPanel (LazeratorAudioProcessor& p)
    : audioProcessor (p), mPresetBar (p), mVisualizer (p), mDetectView (p),
      mOutputSlider (p), mSweepView (p)
{
    addAndMakeVisible (mOutputSlider);
    addAndMakeVisible (mSweepView);

    addAndMakeVisible (mPresetBar);
    addAndMakeVisible (mVisualizer);
    addAndMakeVisible (mDetectView);

    addAndMakeVisible (mTitleArea);
    mTitleArea.onClick = [this]
    {
        mCredits.setVisible (true);
        mCredits.toFront (true);
    };
    addChildComponent (mCredits);

    // --- baris utama
    addKnob (mPinch,  "Focus",  "Pinch",      fmtPlain);
    addKnob (mAmount, "Amount", "Amount",     fmtInt);
    addKnob (mBase,   "Base",   "BaseFreq",   fmtFreq);
    addKnob (mDepth,  "Depth",  "SweepDepth", fmtOct);
    addKnob (mTime,   "Time",   "SweepTime",  fmtMs);

    // --- DETECT
    addKnob (mSens,  "Sens",   "Sensitivity", fmtDb);
    addKnob (mHold,  "Hold",   "Hold",        fmtMs);
    addKnob (mScHpf, "SC HPF", "SCHighPass",  fmtFreq);
    addKnob (mScLpf, "SC LPF", "SCLowPass",   fmtFreq);
    addSeg (mSourceSeg, mSourceLabel, "Source", "SCSource");
    addAndMakeVisible (mScListen);
    mScListenAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "SCListen", mScListen);

    // --- MOD / STEREO
    addSeg (mRetrigSeg, mRetrigLabel, "Retrigger", "Retrigger");
    addSeg (mStereoSeg, mStereoLabel, "Stereo", "StereoMode");
    addKnob (mCurve,     "Curve",  "Curve",      fmtPlain2);
    addKnob (mSpread,    "Spread", "Spread",     fmtSt);
    addKnob (mTimeOff,   "T.Off",  "TimeOffset", fmtPctSign);
    addKnob (mSideDepth, "Side",   "SideDepth",  fmtPct);

    // --- OUTPUT
    addKnob (mDryWet, "Dry/Wet", "DryWet", fmtPct);
    addToggle (mSoftClip,    "Clip",   "SoftClip");
    addToggle (mAutoGain,    "A.Gain", "AutoGain");
    addToggle (mPhaseInvert, "Ph.Inv", "PhaseInvert");

    mMixWatcher = std::make_unique<juce::ParameterAttachment> (
        *p.apvts.getParameter ("DryWet"),
        [this] (float mix) { mPhaseInvert.button.setEnabled (mix < 99.95f); });
    mMixWatcher->sendInitialUpdate();

    // Base menempel ke nada saat ditarik, dan labelnya menyebutkan nada itu
    mBase.slider.setSnapToNotes (true);
    mBaseNoteWatcher = std::make_unique<juce::ParameterAttachment> (
        *p.apvts.getParameter ("BaseFreq"),
        [this] (float hz)
        {
            const int note = lazer::frequencyToMidiNote (hz);
            const double exact = lazer::midiNoteToFrequency (note);
            const double cents = 12.0 * std::log2 (juce::jmax (1.0, (double) hz) / exact);
            const bool onNote = std::abs (cents) < 0.1;      // dalam 10 sen
            mBase.label.setText (onNote ? "Base · " + lazer::noteName (note)
                                        : juce::String ("Base"),
                                 juce::dontSendNotification);
        });
    mBaseNoteWatcher->sendInitialUpdate();
}

void MainPanel::addKnob (Knob& k, const juce::String& name,
                         const juce::String& paramID, Formatter fmt)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (lazer::uiFont (11.0f));
    k.label.setColour (juce::Label::textColourId, lazer::inkSoft);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, paramID, k.slider);

    // Setelah attachment: ganti fungsi teksnya dengan format kompak
    k.slider.textFromValueFunction = [fmt] (double v) { return fmt (v); };
    k.slider.updateText();
}

void MainPanel::addToggle (Toggle& t, const juce::String& name, const juce::String& paramID)
{
    t.button.setButtonText (name);
    addAndMakeVisible (t.button);
    t.attachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, paramID, t.button);
}

void MainPanel::addSeg (std::unique_ptr<SegmentedControl>& seg, juce::Label& label,
                        const juce::String& name, const juce::String& paramID)
{
    seg = std::make_unique<SegmentedControl> (*audioProcessor.apvts.getParameter (paramID));
    addAndMakeVisible (*seg);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (lazer::uiFont (10.0f));
    label.setColour (juce::Label::textColourId, lazer::inkSoft);
    addAndMakeVisible (label);
}

void MainPanel::layoutKnob (Knob& k, juce::Rectangle<int> cell, int diameter)
{
    k.label.setBounds (cell.removeFromBottom (14));
    if (diameter <= 0)
        diameter = juce::jmin (cell.getWidth(), cell.getHeight());
    k.slider.setBounds (juce::Rectangle<int> (diameter, diameter).withCentre (cell.getCentre()));
}

//==============================================================================

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (lazer::bg);

    // --- header
    constexpr int headerH = 50;
    g.setColour (lazer::pink);
    g.setFont (lazer::uiFont (22.0f, true));
    g.drawText ("//", 16, 0, 34, headerH, juce::Justification::centredLeft);
    g.setColour (lazer::ink);
    g.drawText ("Lazerator", 44, 0, 220, headerH, juce::Justification::centredLeft);

    // --- kartu
    auto title = [&g] (juce::Rectangle<int> card, const juce::String& t)
    {
        lazer::drawCard (g, card.toFloat(), 10.0f, lazer::card);
        g.setColour (lazer::inkSoft);
        g.setFont (lazer::uiFont (11.0f, true));
        g.drawText (t, card.getX() + 12, card.getY() + 4, card.getWidth() - 24, 14,
                    juce::Justification::left);
    };

    lazer::drawCard (g, mKnobCard.toFloat(), 10.0f, lazer::card);
    title (mDetectCard, "Detect");
    title (mModCard, "Mod / Stereo");
    title (mOutCard, "Output");
}

void MainPanel::resized()
{
    auto bounds = getLocalBounds();

    mCredits.setBounds (getLocalBounds());

    // --- header: judul kiri (klik = kredit), bilah preset kanan
    auto header = bounds.removeFromTop (50);
    mTitleArea.setBounds (16, 0, 200, 50);
    mPresetBar.setBounds (header.removeFromRight (400).withSizeKeepingCentre (400, 26)
                                .translated (-16, 0));

    // --- visualizer
    bounds.removeFromTop (2);
    mVisualizer.setBounds (bounds.removeFromTop (234).reduced (12, 0));

    // --- kartu kenop utama
    bounds.removeFromTop (8);
    mKnobCard = bounds.removeFromTop (126).reduced (12, 0);
    {
        // Lima kolom identik: pusat kenop berjarak sama persis, Base sedikit
        // lebih besar sebagai penanda kontrol utama tanpa merusak ritme.
        auto row = mKnobCard.reduced (16, 8);
        const int cellW = row.getWidth() / 5;
        Knob* const order[] = { &mPinch, &mAmount, &mBase, &mDepth, &mTime };
        for (int i = 0; i < 5; ++i)
            layoutKnob (*order[i],
                        juce::Rectangle<int> (row.getX() + i * cellW, row.getY(), cellW, row.getHeight()),
                        i == 2 ? 88 : 76);
    }

    // --- tiga kartu bawah
    bounds.removeFromTop (8);
    bounds.removeFromBottom (18);
    bounds.reduce (12, 0);

    auto detectArea = bounds.removeFromLeft (juce::roundToInt ((float) bounds.getWidth() * 0.42f));
    bounds.removeFromLeft (8);
    auto modArea = bounds.removeFromLeft (juce::roundToInt ((float) bounds.getWidth() * 0.55f));
    bounds.removeFromLeft (8);
    auto outArea = bounds;

    mDetectCard = detectArea;
    mModCard    = modArea;
    mOutCard    = outArea;

    // DETECT: view ala kompresor di atas, kenop + pil di bawah
    {
        auto content = detectArea.reduced (10);
        content.removeFromTop (16);
        mDetectView.setBounds (content.removeFromTop (106));
        content.removeFromTop (6);

        auto knobRow = content.removeFromTop (76);
        const int w = knobRow.getWidth() / 4;
        layoutKnob (mSens,  knobRow.removeFromLeft (w));
        layoutKnob (mHold,  knobRow.removeFromLeft (w));
        layoutKnob (mScHpf, knobRow.removeFromLeft (w));
        layoutKnob (mScLpf, knobRow);

        content.removeFromTop (4);
        auto pillRow = content.removeFromTop (26);
        mSourceLabel.setBounds (pillRow.removeFromLeft (48));
        mSourceSeg->setBounds (pillRow.removeFromLeft (150).withSizeKeepingCentre (150, 24));
        mScListen.setBounds (pillRow.removeFromRight (26).withSizeKeepingCentre (26, 26));
    }

    // MOD / STEREO: dua pil tersegmen, empat kenop, lalu tampilan bentuk sweep
    {
        auto content = modArea.reduced (10);
        content.removeFromTop (16);

        auto r1 = content.removeFromTop (24);
        mRetrigLabel.setBounds (r1.removeFromLeft (62));
        mRetrigSeg->setBounds (r1);
        content.removeFromTop (6);
        auto r2 = content.removeFromTop (24);
        mStereoLabel.setBounds (r2.removeFromLeft (62));
        mStereoSeg->setBounds (r2);

        content.removeFromTop (10);
        auto knobRow = content.removeFromTop (84);
        const int w = knobRow.getWidth() / 4;
        layoutKnob (mCurve,     knobRow.removeFromLeft (w));
        layoutKnob (mSpread,    knobRow.removeFromLeft (w));
        layoutKnob (mTimeOff,   knobRow.removeFromLeft (w));
        layoutKnob (mSideDepth, knobRow);

        content.removeFromTop (8);
        mSweepView.setBounds (content.withTrimmedBottom (2));
    }

    // OUTPUT: kolom kiri = kenop Dry/Wet dengan tiga pil bertumpuk di bawahnya;
    // sisa lebar dan seluruh tinggi kartu diberikan ke slider Drive/Out.
    {
        auto content = outArea.reduced (10);
        content.removeFromTop (16);

        auto leftCol = content.removeFromLeft (86);
        content.removeFromLeft (10);
        mOutputSlider.setBounds (content.reduced (0, 2));

        layoutKnob (mDryWet, leftCol.removeFromTop (84), 70);
        leftCol.removeFromTop (10);
        for (Toggle* t : { &mSoftClip, &mAutoGain, &mPhaseInvert })
        {
            t->button.setBounds (leftCol.removeFromTop (26));
            leftCol.removeFromTop (8);
        }
    }
}

//==============================================================================
// Editor — hanya menampung kanvas dan menskalakannya

LazeratorAudioProcessorEditor::LazeratorAudioProcessorEditor (LazeratorAudioProcessor& p)
    : AudioProcessorEditor (&p), panel (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (panel);

    // §9.4: 70% hingga 200% dari ukuran kanvas
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (MainPanel::canvasWidth  * 0.7),
                     juce::roundToInt (MainPanel::canvasHeight * 0.7),
                     MainPanel::canvasWidth  * 2,
                     MainPanel::canvasHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) MainPanel::canvasWidth / (double) MainPanel::canvasHeight);

    setSize (MainPanel::canvasWidth, MainPanel::canvasHeight);
}

LazeratorAudioProcessorEditor::~LazeratorAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void LazeratorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (lazer::bg);   // menutup sisa pembulatan di tepi kanvas
}

void LazeratorAudioProcessorEditor::resized()
{
    // Kanvas selalu berukuran nominal; skala diterapkan sebagai transform tunggal
    // sehingga setiap elemen di dalamnya, termasuk teks, ikut proporsional.
    const float scale = (float) getWidth() / (float) MainPanel::canvasWidth;
    panel.setBounds (0, 0, MainPanel::canvasWidth, MainPanel::canvasHeight);
    panel.setTransform (juce::AffineTransform::scale (scale));
}
