#pragma once
#include <JuceHeader.h>
#include "LazerDSP.h"
#include "Presets.h"

class LazeratorAudioProcessor : public juce::AudioProcessor
{
public:
    LazeratorAudioProcessor();
    ~LazeratorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    LazerDSP& getDsp() { return mLazerDSP; }

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets { apvts };   // dipakai dari thread pesan (GUI) saja

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    LazerDSP mLazerDSP;

    // Cache pointer nilai mentah parameter (§8)
    std::atomic<float>* pBaseFreq    = nullptr;
    std::atomic<float>* pPinch       = nullptr;
    std::atomic<float>* pAmount      = nullptr;
    std::atomic<float>* pSweepDepth  = nullptr;
    std::atomic<float>* pSweepTime   = nullptr;
    std::atomic<float>* pCurve       = nullptr;
    std::atomic<float>* pRetrigger   = nullptr;
    std::atomic<float>* pThreshold   = nullptr;
    std::atomic<float>* pSensitivity = nullptr;
    std::atomic<float>* pHold        = nullptr;
    std::atomic<float>* pScHpf       = nullptr;
    std::atomic<float>* pScLpf       = nullptr;
    std::atomic<float>* pScSource    = nullptr;
    std::atomic<float>* pScListen    = nullptr;
    std::atomic<float>* pStereoMode  = nullptr;
    std::atomic<float>* pSpread      = nullptr;
    std::atomic<float>* pTimeOffset  = nullptr;
    std::atomic<float>* pSideDepth   = nullptr;
    std::atomic<float>* pDryWet      = nullptr;
    std::atomic<float>* pPhaseInvert = nullptr;
    std::atomic<float>* pSoftClip    = nullptr;
    std::atomic<float>* pDrive       = nullptr;
    std::atomic<float>* pAutoGain    = nullptr;
    std::atomic<float>* pOutputGain  = nullptr;
    std::atomic<float>* pBypass      = nullptr;

    juce::RangedAudioParameter* mBypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LazeratorAudioProcessor)
};
