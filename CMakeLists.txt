cmake_minimum_required(VERSION 3.20)
project(Lazerator VERSION 1.1.0)

# We bundle JUCE
add_subdirectory(JUCE)

juce_add_plugin(Lazerator
    COMPANY_NAME "AnakBaek DSP"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE AnBk
    PLUGIN_CODE Lzrt
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Lazerator"
    VST3_CATEGORIES Fx)

juce_generate_juce_header(Lazerator)

# Aset tertanam (logo pada jendela kredit)
juce_add_binary_data(LazeratorAssets SOURCES "AnakBaek DSP.png")

target_sources(Lazerator PRIVATE
    Source/CreditsView.cpp
    Source/CreditsView.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/LazerDSP.cpp
    Source/LazerDSP.h
    Source/LazerLook.cpp
    Source/LazerLook.h
    Source/Controls.h
    Source/DetectView.cpp
    Source/DetectView.h
    Source/OutputSlider.cpp
    Source/OutputSlider.h
    Source/Presets.cpp
    Source/Presets.h
    Source/PresetBar.cpp
    Source/PresetBar.h
    Source/SweepView.cpp
    Source/SweepView.h
    Source/Visualizer.cpp
    Source/Visualizer.h)

target_compile_definitions(Lazerator PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Lazerator PRIVATE
    LazeratorAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Target C++20 as requested
target_compile_features(Lazerator PUBLIC cxx_std_20)

# Berkas sumber ditulis dalam UTF-8. Tanpa ini MSVC membaca literal memakai
# codepage sistem sehingga karakter non-ASCII muncul rusak di antarmuka.
if(MSVC)
    target_compile_options(Lazerator PRIVATE /utf-8)
endif()

# Harness pengukuran DSP offline (§10.1)
juce_add_console_app(LazeratorTests PRODUCT_NAME "LazeratorTests")
juce_generate_juce_header(LazeratorTests)

target_sources(LazeratorTests PRIVATE
    Tests/DspTests.cpp
    Source/LazerDSP.cpp)

target_compile_definitions(LazeratorTests PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(LazeratorTests PRIVATE
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_recommended_config_flags)

target_compile_features(LazeratorTests PUBLIC cxx_std_20)
