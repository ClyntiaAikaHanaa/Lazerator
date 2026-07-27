#include "PluginProcessor.h"
#include "PluginEditor.h"

// Penanda versi skema state untuk jalur migrasi maju (§3.1)
static constexpr int kSchemaVersion = 2;

LazeratorAudioProcessor::LazeratorAudioProcessor()
     : AudioProcessor (BusesProperties()
                        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    pBaseFreq    = apvts.getRawParameterValue ("BaseFreq");
    pPinch       = apvts.getRawParameterValue ("Pinch");
    pAmount      = apvts.getRawParameterValue ("Amount");
    pSweepDepth  = apvts.getRawParameterValue ("SweepDepth");
    pSweepTime   = apvts.getRawParameterValue ("SweepTime");
    pCurve       = apvts.getRawParameterValue ("Curve");
    pRetrigger   = apvts.getRawParameterValue ("Retrigger");
    pThreshold   = apvts.getRawParameterValue ("Threshold");
    pSensitivity = apvts.getRawParameterValue ("Sensitivity");
    pHold        = apvts.getRawParameterValue ("Hold");
    pScHpf       = apvts.getRawParameterValue ("SCHighPass");
    pScLpf       = apvts.getRawParameterValue ("SCLowPass");
    pScSource    = apvts.getRawParameterValue ("SCSource");
    pScListen    = apvts.getRawParameterValue ("SCListen");
    pStereoMode  = apvts.getRawParameterValue ("StereoMode");
    pSpread      = apvts.getRawParameterValue ("Spread");
    pTimeOffset  = apvts.getRawParameterValue ("TimeOffset");
    pSideDepth   = apvts.getRawParameterValue ("SideDepth");
    pDryWet      = apvts.getRawParameterValue ("DryWet");
    pPhaseInvert = apvts.getRawParameterValue ("PhaseInvert");
    pSoftClip    = apvts.getRawParameterValue ("SoftClip");
    pDrive       = apvts.getRawParameterValue ("Drive");
    pAutoGain    = apvts.getRawParameterValue ("AutoGain");
    pOutputGain  = apvts.getRawParameterValue ("OutputGain");
    pBypass      = apvts.getRawParameterValue ("Bypass");

    mBypassParam = apvts.getParameter ("Bypass");

    apvts.state.setProperty ("schemaVersion", kSchemaVersion, nullptr);
}

LazeratorAudioProcessor::~LazeratorAudioProcessor()
{
}

const juce::String LazeratorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LazeratorAudioProcessor::acceptsMidi() const  { return false; }
bool LazeratorAudioProcessor::producesMidi() const { return false; }
bool LazeratorAudioProcessor::isMidiEffect() const { return false; }

double LazeratorAudioProcessor::getTailLengthSeconds() const
{
    // Group delay maksimum yang dapat dicapai antarmuka berada di kisaran detik (§6.4)
    return 2.0;
}

int LazeratorAudioProcessor::getNumPrograms()                                   { return 1; }
int LazeratorAudioProcessor::getCurrentProgram()                                { return 0; }
void LazeratorAudioProcessor::setCurrentProgram (int)                           {}
const juce::String LazeratorAudioProcessor::getProgramName (int)                { return {}; }
void LazeratorAudioProcessor::changeProgramName (int, const juce::String&)      {}

void LazeratorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mLazerDSP.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (0);   // §7.3: nol sampel pada seluruh konfigurasi
}

void LazeratorAudioProcessor::releaseResources()
{
    mLazerDSP.reset();
}

bool LazeratorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn  = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
     && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != mainOut)
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled()
         && sc != juce::AudioChannelSet::mono()
         && sc != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void LazeratorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;   // §6.8: wajib
    juce::ignoreUnused (midiMessages);

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (pBaseFreq == nullptr)
        return;

    LazerDSP::Parameters p;
    p.baseFreq        = pBaseFreq->load();
    p.pinch           = pPinch->load();
    p.amount          = (int) std::lround (pAmount->load());
    p.sweepDepth      = pSweepDepth->load();
    p.sweepTimeMs     = pSweepTime->load();
    p.curve           = pCurve->load();
    p.retrigger       = static_cast<LazerDSP::RetriggerMode> ((int) pRetrigger->load());
    p.thresholdDb     = pThreshold->load();
    p.sensitivityDb   = pSensitivity->load();
    p.holdMs          = pHold->load();
    p.scHighPass      = pScHpf->load();
    p.scLowPass       = pScLpf->load();
    p.scExternal      = pScSource->load() > 0.5f;
    p.scListen        = pScListen->load() > 0.5f;
    p.stereoMode      = static_cast<LazerDSP::StereoMode> ((int) pStereoMode->load());
    p.spreadSemitones = pSpread->load();
    p.timeOffsetPct   = pTimeOffset->load();
    p.sideDepthPct    = pSideDepth->load();
    p.dryWetPct       = pDryWet->load();
    p.phaseInvert     = pPhaseInvert->load() > 0.5f;
    p.softClip        = pSoftClip->load() > 0.5f;
    p.drive           = pDrive->load();
    p.autoGain        = pAutoGain->load() > 0.5f;
    p.outputGainDb    = pOutputGain->load();
    p.bypass          = pBypass->load() > 0.5f;

    mLazerDSP.setParameters (p);

    const float* scPtrs[2] = { nullptr, nullptr };
    int scCh = 0;
    if (getBusCount (true) > 1)
    {
        auto scBus = getBusBuffer (buffer, true, 1);
        scCh = juce::jmin (2, scBus.getNumChannels());
        for (int i = 0; i < scCh; ++i)
            scPtrs[i] = scBus.getReadPointer (i);
    }

    auto mainBus = getBusBuffer (buffer, true, 0);
    mLazerDSP.process (mainBus, scPtrs, scCh);
}

bool LazeratorAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* LazeratorAudioProcessor::createEditor()
{
    return new LazeratorAudioProcessorEditor (*this);
}

juce::AudioProcessorParameter* LazeratorAudioProcessor::getBypassParameter() const
{
    return mBypassParam;
}

void LazeratorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LazeratorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        auto newState = juce::ValueTree::fromXml (*xmlState);

        // Jalur migrasi maju (§3.1): versi < 2 berasal dari prototipe pra-rilis,
        // parameternya tidak kompatibel (Depth dulu dalam Hz) — cukup distempel ulang.
        newState.setProperty ("schemaVersion", kSchemaVersion, nullptr);

        apvts.replaceState (newState);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout LazeratorAudioProcessor::createParameterLayout()
{
    using FloatParam  = juce::AudioParameterFloat;
    using IntParam    = juce::AudioParameterInt;
    using BoolParam   = juce::AudioParameterBool;
    using ChoiceParam = juce::AudioParameterChoice;

    // Interval menentukan jumlah desimal pada teks parameter (host & textbox GUI)
    auto logRange = [] (float lo, float hi, float centre, float step = 0.1f)
    {
        juce::NormalisableRange<float> r (lo, hi, step);
        r.setSkewForCentre (centre);
        return r;
    };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Lazer Engine (§8.1)
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("BaseFreq", 1), "Base Frequency",
                logRange (20.0f, 20000.0f, 632.0f), 800.0f));
    // ID tetap "Pinch" agar preset lama dan pemetaan otomasi DAW tidak putus;
    // hanya nama tampilannya yang menjadi Focus.
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("Pinch", 1), "Focus",
                logRange (0.5f, 20.0f, 3.16f), 4.0f));
    layout.add (std::make_unique<IntParam> (juce::ParameterID ("Amount", 1), "Amount",
                1, LazerDSP::maxStages, 12));

    // --- Auto-Sweep Modulation (§8.2)
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("SweepDepth", 1), "Sweep Depth",
                juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f), 3.0f));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("SweepTime", 1), "Lazer Time",
                logRange (5.0f, 2000.0f, 100.0f), 120.0f));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("Curve", 1), "Curve",
                logRange (0.25f, 4.0f, 1.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<ChoiceParam> (juce::ParameterID ("Retrigger", 1), "Retrigger",
                juce::StringArray { "Reset", "Legato", "Lock" }, 0));

    // --- Detektor & Sidechain (§8.3)
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("Threshold", 1), "Threshold",
                juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -24.0f));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("Sensitivity", 1), "Sensitivity",
                juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 6.0f));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("Hold", 1), "Hold",
                logRange (1.0f, 500.0f, 22.4f), 20.0f));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("SCHighPass", 1), "SC High-Pass",
                logRange (20.0f, 2000.0f, 200.0f), 20.0f));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("SCLowPass", 1), "SC Low-Pass",
                logRange (200.0f, 20000.0f, 2000.0f), 20000.0f));
    layout.add (std::make_unique<ChoiceParam> (juce::ParameterID ("SCSource", 1), "SC Source",
                juce::StringArray { "Internal", "External" }, 0));
    layout.add (std::make_unique<BoolParam> (juce::ParameterID ("SCListen", 1), "SC Listen", false));

    // --- Stereo (§8.4)
    layout.add (std::make_unique<ChoiceParam> (juce::ParameterID ("StereoMode", 1), "Stereo Mode",
                juce::StringArray { "Link", "Offset", "Mid-Side" }, 0));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("Spread", 1), "Spread",
                juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("TimeOffset", 1), "Time Offset",
                juce::NormalisableRange<float> (-20.0f, 20.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("SideDepth", 1), "Side Depth",
                juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f), 100.0f));

    // --- Master & Output (§8.5)
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("DryWet", 1), "Dry/Wet",
                juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    layout.add (std::make_unique<BoolParam> (juce::ParameterID ("PhaseInvert", 1), "Phase Invert", false));
    layout.add (std::make_unique<BoolParam> (juce::ParameterID ("SoftClip", 1), "Soft Clip", true));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("Drive", 1), "Drive",
                logRange (1.0f, 10.0f, 3.16f), 2.0f));
    layout.add (std::make_unique<BoolParam> (juce::ParameterID ("AutoGain", 1), "Auto Gain", true));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID ("OutputGain", 1), "Output Gain",
                juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<BoolParam> (juce::ParameterID ("Bypass", 1), "Bypass", false));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LazeratorAudioProcessor();
}
