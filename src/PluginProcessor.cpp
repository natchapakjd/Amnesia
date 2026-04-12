#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>

#if VAYU_HAS_NAM
#include "NAM/model_config.h"
#include "NAM/container.h"
#include "NAM/convnet.h"
#include "NAM/dsp.h"
#include "NAM/lstm.h"
#include "NAM/wavenet.h"
#endif

namespace
{
#if VAYU_HAS_NAM
void ensureNamConfigParsersRegistered()
{
    auto& registry = nam::ConfigParserRegistry::instance();

    if (!registry.has("Linear"))
        registry.registerParser("Linear", nam::linear::create_config);
    if (!registry.has("LSTM"))
        registry.registerParser("LSTM", nam::lstm::create_config);
    if (!registry.has("ConvNet"))
        registry.registerParser("ConvNet", nam::convnet::create_config);
    if (!registry.has("WaveNet"))
        registry.registerParser("WaveNet", nam::wavenet::create_config);
    if (!registry.has("Container"))
        registry.registerParser("Container", nam::container::create_config);
}
#endif

const char* kNoteNames[12] =
{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

float getBufferPeak (const juce::AudioBuffer<float>& buffer, int startChannel, int endChannel)
{
    float peak = 0.0f;

    for (int channel = startChannel; channel < endChannel; ++channel)
        peak = juce::jmax(peak, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));

    return peak;
}
const std::vector<VayuAudioProcessor::MidiLearnTarget>& getStaticMidiLearnTargets()
{
    static const std::vector<VayuAudioProcessor::MidiLearnTarget> targets
    {
        { "Drive", "drive", "cc_drive" },
        { "Output", "outputVolume", "cc_output" },
        { "Gate", "gateThreshold", "cc_gate" },
        { "Boost", "boostDb", "cc_boost" },
        { "Delay Mix", "delayMix", "cc_delayMix" },
        { "Reverb Mix", "reverbMix", "cc_reverbMix" },
        { "Cab Blend", "cabBlend", "cc_cabBlend" },
        { "Cab Pan", "cabPan", "cc_cabPan" },
        { "Cab A Level", "cabLevelA", "cc_cabLevelA" },
        { "Cab B Level", "cabLevelB", "cc_cabLevelB" },
        { "Delay Feedback", "delayFeedback", "cc_delayFeedback" },
        { "Delay Time", "delayTimeMs", "cc_delayTimeMs" },
        { "Reverb Room", "reverbRoomSize", "cc_reverbRoomSize" },
        { "Reverb Damping", "reverbDamping", "cc_reverbDamping" },
        { "Bass", "ampBassDb", "cc_bass" },
        { "Mid", "ampMidDb", "cc_mid" },
        { "Treble", "ampTrebleDb", "cc_treble" },
        { "Presence", "ampPresenceDb", "cc_presence" },
        { "Pitch", "pitchShiftSemi", "cc_pitch" },
        { "Tight Cut", "tightLowCutHz", "cc_tight" },
        { "Wah Freq", "wahCenterHz", "cc_wahFreq" },
        { "Wah Depth", "wahDepth", "cc_wahDepth" },
        { "Kill Depth", "killDepth", "cc_killDepth" },
        { "NAM Blend", "namBlend", "cc_namBlend" },
        { "Input Trim", "inputTrimDb", "cc_inputTrim" },
        { "Dist On", "distEnable", "cc_distEnable" },
        { "Dist Gain", "distGainDb", "cc_distGain" },
        { "Dist Drive", "distDrive", "cc_distDrive" },
        { "Dist Tone", "distTone", "cc_distTone" },
        { "Dist Mix", "distMix", "cc_distMix" },
        { "Dist Out", "distOutputDb", "cc_distOut" },
        { "Dist Mode", "distMode", "cc_distMode" },
        { "Dist LowCut", "distTightHz", "cc_distTight" },
        { "Dist HiCut", "distHiCutHz", "cc_distHiCut" },
        { "Dist Presence", "distPresenceDb", "cc_distPresence" },
        { "Dist Gate", "distGateDb", "cc_distGate" },
        { "Comp On", "compEnable", "cc_compEnable" },
        { "Comp Thresh", "compThreshDb", "cc_compThresh" },
        { "Comp Ratio", "compRatio", "cc_compRatio" },
        { "Comp Mix", "compMix", "cc_compMix" },
        { "Chorus On", "chorusEnable", "cc_chorusEnable" },
        { "Chorus Rate", "chorusRate", "cc_chorusRate" },
        { "Chorus Depth", "chorusDepth", "cc_chorusDepth" },
        { "Chorus Mix", "chorusMix", "cc_chorusMix" },
        { "Phaser On", "phaserEnable", "cc_phaserEnable" },
        { "Phaser Rate", "phaserRate", "cc_phaserRate" },
        { "Phaser Depth", "phaserDepth", "cc_phaserDepth" },
        { "Phaser Mix", "phaserMix", "cc_phaserMix" },
        { "Flanger On", "flangerEnable", "cc_flangerEnable" },
        { "Flanger Rate", "flangerRate", "cc_flangerRate" },
        { "Flanger Depth", "flangerDepth", "cc_flangerDepth" },
        { "Flanger Mix", "flangerMix", "cc_flangerMix" },
        { "Tremolo On", "tremoloEnable", "cc_tremoloEnable" },
        { "Tremolo Rate", "tremoloRate", "cc_tremoloRate" },
        { "Tremolo Depth", "tremoloDepth", "cc_tremoloDepth" },
        { "PEQ On", "peqEnable", "cc_peqEnable" },
        { "PEQ Low Gain", "peqBand1GainDb", "cc_peqGain1" },
        { "PEQ Mid Gain", "peqBand2GainDb", "cc_peqGain2" },
        { "PEQ High Gain", "peqBand3GainDb", "cc_peqGain3" },
        { "Metro On", "metroEnable", "cc_metroEnable" },
        { "Metro BPM", "metroBpm", "cc_metroBpm" },
        { "Metro Level", "metroLevel", "cc_metroLevel" },
        { "Looper Level", "looperLevel", "cc_looperLevel" }
    };

    return targets;
}
}

VayuAudioProcessor::VayuAudioProcessor()
    : AudioProcessor(BusesProperties()
    #if ! JucePlugin_IsMidiEffect
    #if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
    #endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    #endif
      ),
    apvts(*this, &undoManager, "Parameters", createParameterLayout())
{
#if VAYU_HAS_NAM
    ensureNamConfigParsersRegistered();
    namStatusText = "NAM: Ready to load";
#else
    namStatusText = "NAM: Unavailable in this build";
#endif
    midiCCMap.assign(getMidiLearnTargets().size(), -1);
}

#if JUCE_MSVC
#pragma warning(disable: 4996)
#endif

const std::vector<VayuAudioProcessor::MidiLearnTarget>& VayuAudioProcessor::getMidiLearnTargets()
{
    return getStaticMidiLearnTargets();
}

juce::AudioProcessorValueTreeState::ParameterLayout VayuAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "drive", 1 },
        "Drive",
        juce::NormalisableRange<float>(1.0f, 10.0f, 0.01f, 1.0f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputVolume", 1 },
        "Output Volume",
        juce::NormalisableRange<float>(-36.0f, 12.0f, 0.01f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gateThreshold", 1 },
        "Gate Threshold",
        juce::NormalisableRange<float>(-80.0f, -20.0f, 0.1f, 1.0f),
        -55.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "boostDb", 1 },
        "Boost",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gateAttackMs", 1 },
        "Gate Attack",
        juce::NormalisableRange<float>(0.1f, 60.0f, 0.1f, 0.5f),
        4.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" ms").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gateReleaseMs", 1 },
        "Gate Release",
        juce::NormalisableRange<float>(5.0f, 350.0f, 0.1f, 0.45f),
        90.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" ms").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gateHysteresisDb", 1 },
        "Gate Hysteresis",
        juce::NormalisableRange<float>(0.0f, 18.0f, 0.1f, 1.0f),
        6.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gateRangeDb", 1 },
        "Gate Range",
        juce::NormalisableRange<float>(-80.0f, -6.0f, 0.1f, 1.0f),
        -80.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayTimeMs", 1 },
        "Delay Time",
        juce::NormalisableRange<float>(40.0f, 800.0f, 1.0f, 0.4f),
        280.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" ms").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayMix", 1 },
        "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 0.65f, 0.001f, 1.0f),
        0.15f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value * 100.0f, 0) + " %"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" %").getFloatValue() / 100.0f; }));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "delaySync", 1 },
        "Delay Sync",
        false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "delayDivision", 1 },
        "Delay Division",
        juce::StringArray { "1/4", "1/8", "1/8D", "1/8T", "1/16" },
        1));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbMix", 1 },
        "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 0.75f, 0.001f, 1.0f),
        0.12f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value * 100.0f, 0) + " %"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" %").getFloatValue() / 100.0f; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbRoomSize", 1 },
        "Reverb Room",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.55f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbDamping", 1 },
        "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.4f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbWidth", 1 },
        "Reverb Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbPreDelayMs", 1 },
        "Reverb Pre-Delay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" ms").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "inputTrimDb", 1 },
        "Input Trim",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "limiterThreshDb", 1 },
        "Limiter Threshold",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f, 1.0f),
        -0.3f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "limiterEnabled", 1 },
        "Limiter",
        true));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "ampType", 1 },
        "Amp Type",
        juce::StringArray { "Clean", "Crunch", "Lead" },
        1));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "metalMode", 1 },
        "Metal Mode",
        false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tightLowCutHz", 1 },
        "Tight Low Cut",
        juce::NormalisableRange<float>(40.0f, 240.0f, 1.0f, 0.5f),
        120.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " Hz"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" Hz").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchShiftSemi", 1 },
        "Pitch Shift",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 1.0f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return (value >= 0.0f ? "+" : "") + juce::String(value, 0) + " st"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" st").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "wahEnable", 1 },
        "Wah Enable",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "wahAuto", 1 },
        "Wah Auto",
        true));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wahCenterHz", 1 },
        "Wah Center",
        juce::NormalisableRange<float>(250.0f, 2200.0f, 1.0f, 0.45f),
        900.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " Hz"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" Hz").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wahDepth", 1 },
        "Wah Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.6f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "killEnable", 1 },
        "Kill Switch",
        false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "killRate", 1 },
        "Kill Rate",
        juce::StringArray { "1/4", "1/8", "1/16", "1/32" },
        2));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "killDepth", 1 },
        "Kill Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ampBassDb", 1 },
        "Amp Bass",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ampMidDb", 1 },
        "Amp Mid",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ampTrebleDb", 1 },
        "Amp Treble",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ampPresenceDb", 1 },
        "Amp Presence",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "irLowCutHz", 1 },
        "IR Low Cut",
        juce::NormalisableRange<float>(20.0f, 1200.0f, 1.0f, 0.35f),
        80.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " Hz"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" Hz").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "irHighCutHz", 1 },
        "IR High Cut",
        juce::NormalisableRange<float>(1200.0f, 20000.0f, 1.0f, 0.35f),
        9000.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " Hz"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" Hz").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "irLevelDb", 1 },
        "IR Level",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "irPhaseInvert", 1 },
        "IR Phase Invert",
        false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "cabBlend", 1 },
        "Cab Blend",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value * 100.0f, 0) + " %"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" %").getFloatValue() / 100.0f; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "cabLevelA", 1 },
        "Cab A Level",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "cabLevelB", 1 },
        "Cab B Level",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" dB").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "cabFlipA", 1 },
        "Cab Flip A",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "cabFlipB", 1 },
        "Cab Flip B",
        false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "cabPan", 1 },
        "Cab Pan",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f, 1.0f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "cabAlignDelayMs", 1 },
        "Cab Align Delay",
        juce::NormalisableRange<float>(-2.0f, 2.0f, 0.01f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 2) + " ms"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" ms").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "oversamplingMode", 1 },
        "Oversampling",
        juce::StringArray { "Off", "2x", "4x" },
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayFeedback", 1 },
        "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f, 1.0f),
        0.35f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(juce::roundToInt(value * 100)) + " %"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" %").getFloatValue() / 100.0f; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayModRate", 1 },
        "Delay Mod Rate",
        juce::NormalisableRange<float>(0.01f, 8.0f, 0.01f, 0.4f),
        0.5f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 2) + " Hz"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" Hz").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayModDepth", 1 },
        "Delay Mod Depth",
        juce::NormalisableRange<float>(0.0f, 20.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" ms").getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "namBypass", 1 },
        "NAM Bypass",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "namAutoMatch", 1 },
        "NAM Auto Match",
        true));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "namBlend", 1 },
        "NAM Blend",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        1.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(juce::roundToInt(value * 100.0f)) + " %"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" %").getFloatValue() / 100.0f; }));

    // ═══════════════════════════════════════════════════════════════════
    //  DISTORTION (pre-NAM stomp)
    // ═══════════════════════════════════════════════════════════════════
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "distEnable", 1 }, "Distortion", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distGainDb", 1 }, "Dist Gain",
        juce::NormalisableRange<float>(0.0f, 42.0f, 0.1f, 1.0f), 18.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " dB"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" dB").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distDrive", 1 }, "Dist Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.82f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distTone", 1 }, "Dist Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.55f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distMix", 1 }, "Dist Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distOutputDb", 1 }, "Dist Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f, 1.0f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " dB"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" dB").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "distMode", 1 }, "Dist Mode",
        juce::StringArray { "Tight", "Brutal", "Fuzz", "Insane" },
        1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distTightHz", 1 }, "Dist Low Cut",
        juce::NormalisableRange<float>(60.0f, 320.0f, 1.0f, 0.45f), 140.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 0) + " Hz"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" Hz").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distHiCutHz", 1 }, "Dist Hi Cut",
        juce::NormalisableRange<float>(2500.0f, 12000.0f, 10.0f, 0.4f), 6500.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 0) + " Hz"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" Hz").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distPresenceDb", 1 }, "Dist Presence",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f, 1.0f), 2.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " dB"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" dB").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distGateDb", 1 }, "Dist Gate",
        juce::NormalisableRange<float>(-80.0f, -30.0f, 0.1f, 1.0f), -58.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " dB"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" dB").getFloatValue(); }));

    // ═══════════════════════════════════════════════════════════════════
    //  COMPRESSOR (pre-amp)
    // ═══════════════════════════════════════════════════════════════════
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "compEnable", 1 }, "Compressor", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "compThreshDb", 1 }, "Comp Threshold",
        juce::NormalisableRange<float>(-40.0f, 0.0f, 0.1f, 1.0f), -18.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " dB"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" dB").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "compRatio", 1 }, "Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 4.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + ":1"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(":1").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "compAttackMs", 1 }, "Comp Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.4f), 10.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " ms"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" ms").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "compReleaseMs", 1 }, "Comp Release",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.45f), 120.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 0) + " ms"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" ms").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "compMakeupDb", 1 }, "Comp Makeup",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f, 1.0f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " dB"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" dB").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "compMix", 1 }, "Comp Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(juce::roundToInt(v * 100)) + " %"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" %").getFloatValue() / 100.0f; }));

    // ═══════════════════════════════════════════════════════════════════
    //  CHORUS
    // ═══════════════════════════════════════════════════════════════════
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "chorusEnable", 1 }, "Chorus", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusRate", 1 }, "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 6.0f, 0.01f, 0.5f), 1.2f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 2) + " Hz"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" Hz").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusDepth", 1 }, "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusMix", 1 }, "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.5f));

    // ═══════════════════════════════════════════════════════════════════
    //  PHASER
    // ═══════════════════════════════════════════════════════════════════
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "phaserEnable", 1 }, "Phaser", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "phaserRate", 1 }, "Phaser Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.5f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 2) + " Hz"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" Hz").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "phaserDepth", 1 }, "Phaser Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.6f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "phaserFeedback", 1 }, "Phaser Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f, 1.0f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "phaserMix", 1 }, "Phaser Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.5f));

    // ═══════════════════════════════════════════════════════════════════
    //  FLANGER
    // ═══════════════════════════════════════════════════════════════════
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "flangerEnable", 1 }, "Flanger", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "flangerRate", 1 }, "Flanger Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.5f), 0.3f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 2) + " Hz"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" Hz").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "flangerDepth", 1 }, "Flanger Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "flangerFeedback", 1 }, "Flanger Feedback",
        juce::NormalisableRange<float>(-0.95f, 0.95f, 0.001f, 1.0f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "flangerMix", 1 }, "Flanger Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.5f));

    // ═══════════════════════════════════════════════════════════════════
    //  TREMOLO
    // ═══════════════════════════════════════════════════════════════════
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "tremoloEnable", 1 }, "Tremolo", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tremoloRate", 1 }, "Tremolo Rate",
        juce::NormalisableRange<float>(0.5f, 12.0f, 0.01f, 0.5f), 4.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " Hz"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" Hz").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tremoloDepth", 1 }, "Tremolo Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.6f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "tremoloSync", 1 }, "Tremolo Sync", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tremoloShape", 1 }, "Tremolo Shape",
        juce::StringArray { "Sine", "Square", "Triangle" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tremoloDivision", 1 }, "Tremolo Division",
        juce::StringArray { "1/4", "1/8", "1/8D", "1/8T", "1/16" }, 1));

    // ═══════════════════════════════════════════════════════════════════
    //  PARAMETRIC EQ (3-band)
    // ═══════════════════════════════════════════════════════════════════
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "peqEnable", 1 }, "Parametric EQ", false));
    for (int b = 1; b <= 3; ++b)
    {
        auto id = [b](const char* suffix) { return juce::String("peqBand") + juce::String(b) + suffix; };
        const float defFreq = (b == 1) ? 200.0f : (b == 2) ? 1000.0f : 4000.0f;
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id("Freq"), 1 }, "PEQ " + juce::String(b) + " Freq",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), defFreq,
            juce::String(), juce::AudioProcessorParameter::genericParameter,
            [](float v, int) { return (v >= 1000.0f) ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String(v, 0) + " Hz"; },
            [](const juce::String& t) { return t.contains("kHz") ? t.trimCharactersAtEnd(" kHz").getFloatValue() * 1000.0f : t.trimCharactersAtEnd(" Hz").getFloatValue(); }));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id("GainDb"), 1 }, "PEQ " + juce::String(b) + " Gain",
            juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f, 1.0f), 0.0f,
            juce::String(), juce::AudioProcessorParameter::genericParameter,
            [](float v, int) { return juce::String(v, 1) + " dB"; },
            [](const juce::String& t) { return t.trimCharactersAtEnd(" dB").getFloatValue(); }));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id("Q"), 1 }, "PEQ " + juce::String(b) + " Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    }

    // ═══════════════════════════════════════════════════════════════════
    //  METRONOME
    // ═══════════════════════════════════════════════════════════════════
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "metroEnable", 1 }, "Metronome", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "metroSync", 1 }, "Metronome Sync", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "metroBpm", 1 }, "Metronome BPM",
        juce::NormalisableRange<float>(30.0f, 300.0f, 0.1f, 1.0f), 120.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " BPM"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" BPM").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "metroLevel", 1 }, "Metronome Level",
        juce::NormalisableRange<float>(-30.0f, 0.0f, 0.1f, 1.0f), -12.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " dB"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" dB").getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "metroTimeSig", 1 }, "Metronome Time Sig",
        juce::StringArray { "4/4", "3/4", "6/8", "5/4", "7/8" }, 0));

    // ═══════════════════════════════════════════════════════════════════
    //  LOOPER
    // ═══════════════════════════════════════════════════════════════════
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "looperLevel", 1 }, "Looper Level",
        juce::NormalisableRange<float>(-30.0f, 6.0f, 0.1f, 1.0f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " dB"; },
        [](const juce::String& t) { return t.trimCharactersAtEnd(" dB").getFloatValue(); }));

    return layout;
}
#if JUCE_MSVC
#pragma warning(pop)
#endif

VayuAudioProcessor::~VayuAudioProcessor() = default;

const juce::String VayuAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VayuAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VayuAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VayuAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VayuAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VayuAudioProcessor::getNumPrograms()
{
    return 1;
}

int VayuAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VayuAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String VayuAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void VayuAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

bool VayuAudioProcessor::loadCabinetIR(const juce::File& irFile)
{
    return loadCabinetIRSlot(0, irFile);
}

void VayuAudioProcessor::clearCabinetIR()
{
    clearCabinetIRSlot(0);
}

juce::String VayuAudioProcessor::getCurrentIRName() const
{
    return getCurrentIRNameForSlot(0);
}

bool VayuAudioProcessor::loadCabinetIRSlot(int slotIndex, const juce::File& irFile)
{
    if (slotIndex < 0 || slotIndex > 1 || !irFile.existsAsFile())
        return false;

    {
        const juce::ScopedLock lock(stateLock);

        if (slotIndex == 0)
            currentIRFileA = irFile;
        else
            currentIRFileB = irFile;
    }

    applyCabinetSlotLoadIfPrepared(slotIndex);
    return true;
}

void VayuAudioProcessor::clearCabinetIRSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex > 1)
        return;

    {
        const juce::ScopedLock lock(stateLock);

        if (slotIndex == 0)
            currentIRFileA = juce::File();
        else
            currentIRFileB = juce::File();
    }

    if (isPrepared)
        loadDefaultCabinetIR(slotIndex);
}

juce::String VayuAudioProcessor::getCurrentIRNameForSlot(int slotIndex) const
{
    const juce::ScopedLock lock(stateLock);

    if (slotIndex == 1)
    {
        if (currentIRFileB.existsAsFile())
            return currentIRFileB.getFileName();

        return "Slot B (Empty)";
    }

    if (currentIRFileA.existsAsFile())
        return currentIRFileA.getFileName();

    return "Slot A (Default)";
}

bool VayuAudioProcessor::canAutoAlignCab() const
{
    const juce::ScopedLock lock(stateLock);
    return currentIRFileA.existsAsFile() && currentIRFileB.existsAsFile();
}

float VayuAudioProcessor::getLastCabAlignCorrelation() const
{
    return cabAlignCorrelation.load();
}

float VayuAudioProcessor::getCabAlignDelayMs() const
{
    if (auto* value = apvts.getRawParameterValue("cabAlignDelayMs"))
        return value->load();

    return 0.0f;
}

bool VayuAudioProcessor::autoAlignCabPolarity()
{
    juce::File irA;
    juce::File irB;
    {
        const juce::ScopedLock lock(stateLock);
        irA = currentIRFileA;
        irB = currentIRFileB;
    }

    if (!irA.existsAsFile() || !irB.existsAsFile())
        return false;

    auto readMonoIR = [](const juce::File& file, std::vector<float>& out, double& sampleRateOut) -> bool
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        auto input = file.createInputStream();
        if (input == nullptr)
            return false;

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(std::move(input)));
        if (reader == nullptr || reader->lengthInSamples <= 0)
            return false;

        sampleRateOut = reader->sampleRate;

        const int maxSamples = 8192;
        const int numSamples = static_cast<int>(juce::jlimit<juce::int64>(0, maxSamples, reader->lengthInSamples));
        if (numSamples <= 0)
            return false;

        const int channelsToRead = static_cast<int>(juce::jlimit<juce::uint32>(1u, 2u, reader->numChannels));
        juce::AudioBuffer<float> temp(channelsToRead, numSamples);
        if (!reader->read(&temp, 0, numSamples, 0, true, true))
            return false;

        out.assign(static_cast<size_t>(numSamples), 0.0f);
        for (int i = 0; i < numSamples; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < channelsToRead; ++ch)
                sum += temp.getSample(ch, i);

            out[static_cast<size_t>(i)] = sum / static_cast<float>(channelsToRead);
        }

        return true;
    };

    std::vector<float> monoA;
    std::vector<float> monoB;
    double sampleRateA = 44100.0;
    double sampleRateB = 44100.0;
    if (!readMonoIR(irA, monoA, sampleRateA) || !readMonoIR(irB, monoB, sampleRateB))
        return false;

    const int n = juce::jmin(static_cast<int>(monoA.size()), static_cast<int>(monoB.size()));
    if (n < 64)
        return false;

    double sumA = 0.0;
    double sumB = 0.0;
    for (int i = 0; i < n; ++i)
    {
        sumA += monoA[static_cast<size_t>(i)];
        sumB += monoB[static_cast<size_t>(i)];
    }
    const double meanA = sumA / static_cast<double>(n);
    const double meanB = sumB / static_cast<double>(n);

    const int maxLag = juce::jmin(256, n / 3);
    double bestCorr = 0.0;
    int bestLag = 0;

    for (int lag = -maxLag; lag <= maxLag; ++lag)
    {
        double dot = 0.0;
        double energyA = 0.0;
        double energyB = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const int j = i + lag;
            if (j < 0 || j >= n)
                continue;

            const double a = static_cast<double>(monoA[static_cast<size_t>(i)]) - meanA;
            const double b = static_cast<double>(monoB[static_cast<size_t>(j)]) - meanB;
            dot += a * b;
            energyA += a * a;
            energyB += b * b;
        }

        const double denom = std::sqrt(juce::jmax(1.0e-18, energyA * energyB));
        const double corr = dot / denom;
        if (std::abs(corr) > std::abs(bestCorr))
        {
            bestCorr = corr;
            bestLag = lag;
        }
    }

    const float correlation = static_cast<float>(bestCorr);
    cabAlignCorrelation.store(correlation);

    const bool shouldFlipB = correlation < 0.0f;
    if (auto* param = apvts.getParameter("cabFlipB"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(shouldFlipB ? 1.0f : 0.0f);
        param->endChangeGesture();
    }

    const double sr = sampleRateB > 1000.0 ? sampleRateB : 44100.0;
    const float delayMs = juce::jlimit(-2.0f, 2.0f, static_cast<float>((-bestLag * 1000.0) / sr));
    cabAlignDelayMsTelemetry.store(delayMs);
    if (auto* delayParam = apvts.getParameter("cabAlignDelayMs"))
    {
        const auto* ranged = dynamic_cast<juce::AudioParameterFloat*>(delayParam);
        delayParam->beginChangeGesture();
        if (ranged != nullptr)
            delayParam->setValueNotifyingHost(ranged->convertTo0to1(delayMs));
        delayParam->endChangeGesture();
    }

    return true;
}

void VayuAudioProcessor::setBackgroundImagePath(const juce::String& path)
{
    const juce::ScopedLock lock(stateLock);
    backgroundImagePath = path;
}

juce::String VayuAudioProcessor::getBackgroundImagePath() const
{
    const juce::ScopedLock lock(stateLock);
    return backgroundImagePath;
}

float VayuAudioProcessor::getTunerFrequencyHz() const
{
    return tunerFrequencyHz.load();
}

float VayuAudioProcessor::getTunerCents() const
{
    return tunerCents.load();
}

juce::String VayuAudioProcessor::getTunerNoteName() const
{
    const auto idx = tunerNoteIndex.load();

    if (idx < 0 || idx > 11)
        return "--";

    return kNoteNames[idx];
}

float VayuAudioProcessor::getInputMeterLevel() const
{
    return inputMeterLevel.load();
}

float VayuAudioProcessor::getOutputMeterLevel() const
{
    return outputMeterLevel.load();
}

bool VayuAudioProcessor::canUndo() const
{
    return undoManager.canUndo();
}

bool VayuAudioProcessor::canRedo() const
{
    return undoManager.canRedo();
}

void VayuAudioProcessor::undoLastChange()
{
    if (undoManager.canUndo())
        undoManager.undo();
}

void VayuAudioProcessor::redoLastChange()
{
    if (undoManager.canRedo())
        undoManager.redo();
}

void VayuAudioProcessor::beginMidiLearnForParam(int paramIndex)
{
    if (paramIndex >= 0 && paramIndex < static_cast<int>(midiCCMap.size()))
        learningParamIndex = paramIndex;
}

juce::String VayuAudioProcessor::getMidiMappingDescription() const
{
    const auto& targets = getMidiLearnTargets();
    juce::StringArray items;
    for (int i = 0; i < static_cast<int>(midiCCMap.size()); ++i)
    {
        const auto cc = midiCCMap[static_cast<size_t>(i)];
        if (cc >= 0)
            items.add(juce::String(targets[static_cast<size_t>(i)].label) + "=CC" + juce::String(cc));
    }

    if (items.isEmpty())
        return "No MIDI mappings";

    return items.joinIntoString(" | ");
}

std::vector<VayuAudioProcessor::FactoryPreset> VayuAudioProcessor::getFactoryPresets()
{
    std::vector<FactoryPreset> presets;

    // --- Blues Clean ---
    presets.push_back({ "Blues Clean", "Clean",
        { {"drive",0.22f}, {"outputVolume",-3.0f}, {"gateThreshold",-60.0f},
                    {"compEnable",1.0f}, {"compThreshDb",-22.0f}, {"compRatio",3.0f}, {"compMix",0.65f},
          {"boostDb",0.0f}, {"ampBassDb",3.0f}, {"ampMidDb",1.5f}, {"ampTrebleDb",-1.0f}, {"ampPresenceDb",2.0f},
                    {"peqEnable",1.0f}, {"peqBand1GainDb",1.0f}, {"peqBand2GainDb",0.8f}, {"peqBand3GainDb",-0.6f},
          {"delayMix",0.12f}, {"delayTimeMs",380.0f}, {"reverbMix",0.18f},
          {"reverbRoomSize",0.4f}, {"reverbDamping",0.5f}, {"reverbWidth",0.9f}, {"reverbPreDelayMs",8.0f},
          {"oversamplingMode",1.0f}, {"ampType",0.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- British Crunch ---
    presets.push_back({ "British Crunch", "Crunch",
        { {"drive",0.55f}, {"outputVolume",-4.5f}, {"gateThreshold",-52.0f},
          {"boostDb",3.0f}, {"ampBassDb",2.0f}, {"ampMidDb",4.0f}, {"ampTrebleDb",2.0f}, {"ampPresenceDb",3.0f},
                    {"compEnable",1.0f}, {"compThreshDb",-18.0f}, {"compRatio",2.2f}, {"compMix",0.45f},
          {"delayMix",0.08f}, {"delayTimeMs",500.0f}, {"reverbMix",0.1f},
          {"reverbRoomSize",0.35f}, {"reverbDamping",0.4f}, {"reverbWidth",0.8f}, {"reverbPreDelayMs",5.0f},
          {"oversamplingMode",1.0f}, {"ampType",1.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- Modern Metal Lead ---
    presets.push_back({ "Modern Metal Lead", "Lead",
        { {"drive",0.85f}, {"outputVolume",-6.0f}, {"gateThreshold",-42.0f},
          {"gateAttackMs",2.0f}, {"gateReleaseMs",80.0f}, {"gateHysteresisDb",6.0f}, {"gateRangeDb",-70.0f},
          {"boostDb",6.0f}, {"ampBassDb",5.0f}, {"ampMidDb",-2.0f}, {"ampTrebleDb",4.0f}, {"ampPresenceDb",5.0f},
                    {"peqEnable",1.0f}, {"peqBand1Freq",110.0f}, {"peqBand1GainDb",1.5f}, {"peqBand2Freq",950.0f}, {"peqBand2GainDb",-2.0f}, {"peqBand3Freq",5600.0f}, {"peqBand3GainDb",2.2f},
          {"delayMix",0.0f}, {"reverbMix",0.05f},
          {"reverbRoomSize",0.3f}, {"reverbDamping",0.6f}, {"reverbWidth",1.0f}, {"reverbPreDelayMs",0.0f},
          {"oversamplingMode",2.0f}, {"ampType",2.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.5f} } });

    // --- Ambient Pad ---
    presets.push_back({ "Ambient Pad", "Clean",
        { {"drive",0.15f}, {"outputVolume",-5.0f}, {"gateThreshold",-65.0f},
          {"boostDb",0.0f}, {"ampBassDb",1.0f}, {"ampMidDb",-1.0f}, {"ampTrebleDb",0.0f}, {"ampPresenceDb",1.0f},
                    {"chorusEnable",1.0f}, {"chorusRate",0.42f}, {"chorusDepth",0.72f}, {"chorusMix",0.52f},
                    {"peqEnable",1.0f}, {"peqBand1GainDb",-0.8f}, {"peqBand2GainDb",1.5f}, {"peqBand3GainDb",2.5f},
          {"delayMix",0.35f}, {"delayTimeMs",600.0f}, {"delaySync",1.0f}, {"delayDivision",1.0f},
          {"reverbMix",0.55f}, {"reverbRoomSize",0.9f}, {"reverbDamping",0.2f}, {"reverbWidth",1.0f}, {"reverbPreDelayMs",25.0f},
          {"oversamplingMode",0.0f}, {"ampType",0.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- High Gain Riff ---
    presets.push_back({ "High Gain Riff", "Lead",
        { {"drive",0.78f}, {"outputVolume",-5.5f}, {"gateThreshold",-46.0f},
          {"gateAttackMs",1.5f}, {"gateReleaseMs",100.0f}, {"gateHysteresisDb",8.0f}, {"gateRangeDb",-80.0f},
          {"boostDb",4.5f}, {"ampBassDb",6.0f}, {"ampMidDb",-4.0f}, {"ampTrebleDb",5.0f}, {"ampPresenceDb",4.0f},
                    {"compEnable",1.0f}, {"compThreshDb",-16.0f}, {"compRatio",2.5f}, {"compMix",0.25f},
                    {"peqEnable",1.0f}, {"peqBand1GainDb",1.4f}, {"peqBand2GainDb",-3.0f}, {"peqBand3GainDb",1.6f},
          {"delayMix",0.0f}, {"reverbMix",0.0f},
          {"oversamplingMode",2.0f}, {"ampType",2.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- Vintage Jazz ---
    presets.push_back({ "Vintage Jazz", "Clean",
        { {"drive",0.18f}, {"outputVolume",-2.0f}, {"gateThreshold",-58.0f},
                    {"compEnable",1.0f}, {"compThreshDb",-24.0f}, {"compRatio",2.0f}, {"compMix",0.55f},
          {"boostDb",0.0f}, {"ampBassDb",4.0f}, {"ampMidDb",3.0f}, {"ampTrebleDb",-3.0f}, {"ampPresenceDb",-1.0f},
                    {"peqEnable",1.0f}, {"peqBand1GainDb",1.0f}, {"peqBand2GainDb",1.2f}, {"peqBand3GainDb",-1.5f},
          {"delayMix",0.0f}, {"reverbMix",0.22f},
          {"reverbRoomSize",0.5f}, {"reverbDamping",0.6f}, {"reverbWidth",0.7f}, {"reverbPreDelayMs",12.0f},
          {"oversamplingMode",0.0f}, {"ampType",0.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- Country Slap ---
    presets.push_back({ "Country Slap", "Crunch",
        { {"drive",0.42f}, {"outputVolume",-3.5f}, {"gateThreshold",-55.0f},
          {"boostDb",2.0f}, {"ampBassDb",1.0f}, {"ampMidDb",2.0f}, {"ampTrebleDb",3.0f}, {"ampPresenceDb",4.0f},
                    {"compEnable",1.0f}, {"compThreshDb",-20.0f}, {"compRatio",3.5f}, {"compMix",0.72f},
          {"delayMix",0.18f}, {"delayTimeMs",250.0f}, {"reverbMix",0.12f},
          {"reverbRoomSize",0.35f}, {"reverbDamping",0.5f}, {"reverbWidth",0.85f}, {"reverbPreDelayMs",0.0f},
          {"oversamplingMode",1.0f}, {"ampType",1.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

        presets.push_back({ "Liquid Chorus Lead", "Lead",
                { {"drive",0.64f}, {"outputVolume",-4.0f}, {"gateThreshold",-50.0f},
                    {"boostDb",3.0f}, {"ampBassDb",2.5f}, {"ampMidDb",3.8f}, {"ampTrebleDb",2.4f}, {"ampPresenceDb",3.2f},
                    {"chorusEnable",1.0f}, {"chorusRate",0.75f}, {"chorusDepth",0.58f}, {"chorusMix",0.44f},
                    {"delaySync",1.0f}, {"delayDivision",1.0f}, {"delayMix",0.20f}, {"delayFeedback",0.34f},
                    {"reverbMix",0.18f}, {"ampType",1.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

        presets.push_back({ "Phase Pulse Clean", "Clean",
                { {"drive",0.20f}, {"outputVolume",-3.0f}, {"gateThreshold",-62.0f},
                    {"ampBassDb",1.5f}, {"ampMidDb",0.5f}, {"ampTrebleDb",1.5f}, {"ampPresenceDb",1.0f},
                    {"phaserEnable",1.0f}, {"phaserRate",0.32f}, {"phaserDepth",0.82f}, {"phaserMix",0.48f},
                    {"tremoloEnable",1.0f}, {"tremoloSync",1.0f}, {"tremoloDivision",4.0f}, {"tremoloDepth",0.42f},
                    {"reverbMix",0.22f}, {"ampType",0.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

        presets.push_back({ "Jet Sweep Rhythm", "Crunch",
                { {"drive",0.58f}, {"outputVolume",-4.4f}, {"gateThreshold",-53.0f},
                    {"boostDb",2.0f}, {"ampBassDb",2.0f}, {"ampMidDb",1.0f}, {"ampTrebleDb",3.0f}, {"ampPresenceDb",2.0f},
                    {"flangerEnable",1.0f}, {"flangerRate",0.22f}, {"flangerDepth",0.78f}, {"flangerFeedback",0.52f}, {"flangerMix",0.40f},
                    {"delayMix",0.12f}, {"reverbMix",0.08f}, {"ampType",1.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

        presets.push_back({ "Studio Sustain", "Utility",
                { {"drive",0.28f}, {"outputVolume",-2.5f}, {"gateThreshold",-65.0f},
                    {"compEnable",1.0f}, {"compThreshDb",-26.0f}, {"compRatio",4.5f}, {"compAttackMs",8.0f}, {"compReleaseMs",140.0f}, {"compMakeupDb",3.5f}, {"compMix",0.78f},
                    {"peqEnable",1.0f}, {"peqBand1Freq",140.0f}, {"peqBand1GainDb",1.8f}, {"peqBand2Freq",1800.0f}, {"peqBand2GainDb",1.0f}, {"peqBand3Freq",6200.0f}, {"peqBand3GainDb",1.4f},
                    {"ampType",0.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    return presets;
}

void VayuAudioProcessor::loadFactoryPreset(int index)
{
    const auto presets = getFactoryPresets();
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return;

    const auto& preset = presets[static_cast<size_t>(index)];
    undoManager.beginNewTransaction(preset.name);

    for (const auto& [paramId, value] : preset.params)
    {
        if (auto* param = apvts.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }
}

void VayuAudioProcessor::loadNamModel(const juce::String& path)
{
#if VAYU_HAS_NAM
    if (path.isEmpty())
        return;

    // ── Defensive: register parsers right before loading ──────────────
    ensureNamConfigParsersRegistered();

    // Log parser state for diagnostics
    {
        auto& reg = nam::ConfigParserRegistry::instance();
        juce::String info;
        for (const char* arch : {"Linear", "LSTM", "ConvNet", "WaveNet", "Container"})
            info += juce::String(arch) + (reg.has(arch) ? ":OK " : ":MISS ");
        juce::Logger::writeToLog("NAM registry check: " + info);
    }

    try
    {
        juce::Logger::writeToLog("NAM loading: " + path);
        auto loaded = nam::get_dsp(std::filesystem::path(path.toStdString()));
        if (loaded == nullptr)
            throw std::runtime_error("NAM model load returned null DSP");

        const int namIn = loaded->NumInputChannels();
        const int namOut = loaded->NumOutputChannels();
        juce::Logger::writeToLog("NAM model channels: in=" + juce::String(namIn)
                                 + " out=" + juce::String(namOut));

        if (isPrepared && processSpec.sampleRate > 0.0 && processSpec.maximumBlockSize > 0)
            loaded->Reset(processSpec.sampleRate, static_cast<int>(processSpec.maximumBlockSize));

        std::shared_ptr<nam::DSP> sharedLoaded(std::move(loaded));
        std::atomic_store_explicit(&namEngine, sharedLoaded, std::memory_order_release);

        const juce::ScopedLock lock(stateLock);
        namModelPath = path;
        namStatusText = "NAM: Loaded " + juce::File(path).getFileName()
                        + " (" + juce::String(namIn) + "->" + juce::String(namOut) + ")";
        juce::Logger::writeToLog(namStatusText);
    }
    catch (const std::exception& e)
    {
        std::atomic_store_explicit(&namEngine, std::shared_ptr<nam::DSP>{}, std::memory_order_release);
        const juce::ScopedLock lock(stateLock);
        namModelPath.clear();
        namStatusText = "NAM load failed: " + juce::String(e.what());
        juce::Logger::writeToLog(namStatusText);
    }
    catch (...)
    {
        std::atomic_store_explicit(&namEngine, std::shared_ptr<nam::DSP>{}, std::memory_order_release);
        const juce::ScopedLock lock(stateLock);
        namModelPath.clear();
        namStatusText = "NAM load failed: unknown error";
        juce::Logger::writeToLog(namStatusText);
    }
#else
    juce::ignoreUnused(path);
    const juce::ScopedLock lock(stateLock);
    namStatusText = "NAM: Unavailable in this build";
    juce::Logger::writeToLog("NAM load requested, but NeuralAmpModelerCore headers are not available in this build.");
#endif
}

void VayuAudioProcessor::clearNamModel()
{
    std::atomic_store_explicit(&namEngine, std::shared_ptr<nam::DSP>{}, std::memory_order_release);
    namMatchInRms = 0.0f;
    namMatchOutRms = 0.0f;
    namMatchGainDb.store(0.0f);
    const juce::ScopedLock lock(stateLock);
    namModelPath.clear();
#if VAYU_HAS_NAM
    namStatusText = "NAM: Off";
#else
    namStatusText = "NAM: Unavailable in this build";
#endif
}

juce::String VayuAudioProcessor::getNamModelPath() const
{
    const juce::ScopedLock lock(stateLock);
    return namModelPath;
}

float VayuAudioProcessor::getNamMatchGainDb() const
{
    return namMatchGainDb.load();
}

juce::String VayuAudioProcessor::getNamStatusText() const
{
    const juce::ScopedLock lock(stateLock);
    return namStatusText;
}

bool VayuAudioProcessor::isNamAvailableInBuild() const
{
#if VAYU_HAS_NAM
    return true;
#else
    return false;
#endif
}

bool VayuAudioProcessor::isNamLoaded() const
{
#if VAYU_HAS_NAM
    return std::atomic_load_explicit(&namEngine, std::memory_order_acquire) != nullptr;
#else
    return false;
#endif
}

double VayuAudioProcessor::resolveTempoBpm(double fallbackBpm) const
{
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
        {
            if (auto hostBpm = position->getBpm(); hostBpm.hasValue() && *hostBpm > 0.0)
                return *hostBpm;
        }
    }

    return fallbackBpm > 0.0 ? fallbackBpm : 120.0;
}

void VayuAudioProcessor::looperTrigger(LooperAction action)
{
    switch (action)
    {
        case LooperAction::Record:
            if (looperState == LooperState::Playing)
            {
                looperState = LooperState::Overdubbing;
            }
            else
            {
                looperLength = 0;
                looperWritePos = 0;
                looperPlayPos = 0;
                looperBuffer.clear();
                looperState = LooperState::Recording;
            }
            break;
        case LooperAction::Play:
            if (looperLength > 0)
            {
                looperPlayPos = 0;
                looperState = LooperState::Playing;
            }
            break;
        case LooperAction::Stop:
            if (looperState == LooperState::Recording)
                looperLength = looperWritePos;
            looperState = LooperState::Stopped;
            break;
        case LooperAction::Clear:
            looperState = LooperState::Stopped;
            looperLength = 0;
            looperWritePos = 0;
            looperPlayPos = 0;
            looperBuffer.clear();
            break;
    }
}

bool VayuAudioProcessor::isLooperRecording() const
{
    return looperState == LooperState::Recording || looperState == LooperState::Overdubbing;
}

bool VayuAudioProcessor::isLooperPlaying() const
{
    return looperState == LooperState::Playing || looperState == LooperState::Overdubbing;
}

int VayuAudioProcessor::getLooperLengthSamples() const
{
    return looperLength;
}

void VayuAudioProcessor::handleMidiLearnAndMapping(juce::MidiBuffer& midiMessages)
{
    const auto& targets = getMidiLearnTargets();

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (!msg.isController())
            continue;

        const auto cc = msg.getControllerNumber();
        const auto value01 = static_cast<float>(msg.getControllerValue()) / 127.0f;

        if (learningParamIndex >= 0)
        {
            midiCCMap[static_cast<size_t>(learningParamIndex)] = cc;
            learningParamIndex = -1;
            continue;
        }

        for (int i = 0; i < static_cast<int>(midiCCMap.size()); ++i)
        {
            if (midiCCMap[static_cast<size_t>(i)] != cc)
                continue;

            if (auto* param = apvts.getParameter(targets[static_cast<size_t>(i)].paramId))
                param->setValueNotifyingHost(value01);
        }
    }
}

void VayuAudioProcessor::applyCabinetSlotLoadIfPrepared(int slotIndex)
{
    if (!isPrepared)
        return;

    const juce::ScopedLock lock(stateLock);
    const auto& fileToLoad = (slotIndex == 0) ? currentIRFileA : currentIRFileB;

    auto& convolution = (slotIndex == 0) ? cabinetConvolutionA : cabinetConvolutionB;
    if (fileToLoad.existsAsFile())
    {
        convolution.loadImpulseResponse(fileToLoad,
                                        juce::dsp::Convolution::Stereo::yes,
                                        juce::dsp::Convolution::Trim::yes,
                                        0,
                                        juce::dsp::Convolution::Normalise::yes);
    }
    else
    {
        loadDefaultCabinetIR(slotIndex);
    }
}

void VayuAudioProcessor::loadDefaultCabinetIR(int slotIndex)
{
    juce::AudioBuffer<float> identityIR(1, 1);
    identityIR.clear();
    identityIR.setSample(0, 0, 1.0f);

    auto& convolution = (slotIndex == 0) ? cabinetConvolutionA : cabinetConvolutionB;
    convolution.loadImpulseResponse(std::move(identityIR),
                                    processSpec.sampleRate > 0.0 ? processSpec.sampleRate : 44100.0,
                                    juce::dsp::Convolution::Stereo::no,
                                    juce::dsp::Convolution::Trim::no,
                                    juce::dsp::Convolution::Normalise::no);
}

void VayuAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    processSpec.sampleRate = sampleRate;
    processSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    processSpec.numChannels = static_cast<juce::uint32>(juce::jmax(1, getMainBusNumOutputChannels()));
    processingChannels = static_cast<int>(processSpec.numChannels);

    cabinetConvolutionA.reset();
    cabinetConvolutionA.prepare(processSpec);
    cabinetConvolutionB.reset();
    cabinetConvolutionB.prepare(processSpec);

    cabBufferA.setSize(processingChannels, samplesPerBlock, false, false, true);
    cabBufferB.setSize(processingChannels, samplesPerBlock, false, false, true);
    pitchBuffer.setSize(processingChannels, samplesPerBlock, false, false, true);

    delayLine.reset();
    delayLine.prepare(processSpec);
    delayLine.setMaximumDelayInSamples(static_cast<int>(sampleRate * 1.0));

    oversampling2x = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(processingChannels),
        1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        true);
    oversampling2x->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling2x->reset();

    oversampling4x = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(processingChannels),
        2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        true);
    oversampling4x->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling4x->reset();

    reverb.reset();
    reverb.prepare(processSpec);

    reverbPreDelay.reset();
    reverbPreDelay.prepare(processSpec);
    reverbPreDelay.setMaximumDelayInSamples(static_cast<int>(sampleRate * 0.12));  // 120 ms max

#if VAYU_HAS_NAM
    namScratchA.assign(static_cast<size_t>(juce::jmax(1, samplesPerBlock)), 0.0f);
    namScratchB.assign(static_cast<size_t>(juce::jmax(1, samplesPerBlock)), 0.0f);
    namDryA.assign(static_cast<size_t>(juce::jmax(1, samplesPerBlock)), 0.0f);
    namDryB.assign(static_cast<size_t>(juce::jmax(1, samplesPerBlock)), 0.0f);
    namMatchInRms = 0.0f;
    namMatchOutRms = 0.0f;
    namMatchGainDb.store(0.0f);

    if (auto activeNam = std::atomic_load_explicit(&namEngine, std::memory_order_acquire))
        activeNam->Reset(sampleRate, samplesPerBlock);
#else
    namScratchA.clear();
    namScratchB.clear();
#endif

    limiterEnvelope = 0.0f;
    gateEnvelope = 0.0f;
    gateGain = 1.0f;
    killLfoPhase = 0.0f;
    wahLfoPhase = 0.0f;

    // New effects reset
    compEnvelope = 0.0f;
    distToneStateL = 0.0f;
    distToneStateR = 0.0f;
    distGateEnv = 0.0f;
    distGateGain = 1.0f;
    distGateOpen = true;

    chorusDelayL.reset(); chorusDelayL.prepare(processSpec);
    chorusDelayR.reset(); chorusDelayR.prepare(processSpec);
    chorusDelayL.setMaximumDelayInSamples(4096);
    chorusDelayR.setMaximumDelayInSamples(4096);
    chorusLfoPhase = 0.0f;

    flangerDelayL.reset(); flangerDelayL.prepare(processSpec);
    flangerDelayR.reset(); flangerDelayR.prepare(processSpec);
    flangerDelayL.setMaximumDelayInSamples(2048);
    flangerDelayR.setMaximumDelayInSamples(2048);
    flangerLfoPhase = 0.0f;
    flangerFeedbackSampleL = 0.0f;
    flangerFeedbackSampleR = 0.0f;

    phaserLfoPhase = 0.0f;
    phaserAPStateL.fill(0.0f);
    phaserAPStateR.fill(0.0f);

    tremoloLfoPhase = 0.0f;

    peqL1.reset(); peqL2.reset(); peqL3.reset();
    peqR1.reset(); peqR2.reset(); peqR3.reset();

    metroPhase = 0.0f;
    metroBeatCount = 0;
    metroCurrentClickBeat = 0;
    metroClickPhase = 0.0f;
    metroClickOscPhase = 0.0f;
    metroClickActive = false;
    hostWasPlaying = false;

    // Looper buffer: allocate up to 60 seconds stereo
    looperBuffer.setSize(processingChannels, static_cast<int>(sampleRate * 60.0), false, true, false);
    looperWritePos = 0;
    looperPlayPos = 0;
    looperLength = 0;
    looperState = LooperState::Stopped;

    isPrepared = true;

    const juce::ScopedLock lock(stateLock);
    if (currentIRFileA.existsAsFile())
        cabinetConvolutionA.loadImpulseResponse(currentIRFileA,
                                                juce::dsp::Convolution::Stereo::yes,
                                                juce::dsp::Convolution::Trim::yes,
                                                0,
                                                juce::dsp::Convolution::Normalise::yes);
    else
        loadDefaultCabinetIR(0);

    if (currentIRFileB.existsAsFile())
        cabinetConvolutionB.loadImpulseResponse(currentIRFileB,
                                                juce::dsp::Convolution::Stereo::yes,
                                                juce::dsp::Convolution::Trim::yes,
                                                0,
                                                juce::dsp::Convolution::Normalise::yes);
    else
        loadDefaultCabinetIR(1);
}

void VayuAudioProcessor::releaseResources()
{
    isPrepared = false;
    inputMeterLevel.store(0.0f);
    outputMeterLevel.store(0.0f);
    gateEnvelope = 0.0f;
    gateGain = 1.0f;
    killLfoPhase = 0.0f;
    wahLfoPhase = 0.0f;
    oversampling2x.reset();
    oversampling4x.reset();
    pitchBuffer.setSize(0, 0);
    namScratchA.clear();
    namScratchB.clear();
    namDryA.clear();
    namDryB.clear();
    namMatchInRms = 0.0f;
    namMatchOutRms = 0.0f;
    namMatchGainDb.store(0.0f);
}

void VayuAudioProcessor::updateTuner(const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() == 0 || getSampleRate() <= 0.0)
        return;

    const auto* input = buffer.getReadPointer(0);
    const int incoming = buffer.getNumSamples();

    // Accumulate samples into a flat buffer.
    const int toCopy = juce::jmin(incoming, kTunerBufSize - tunerFillCount);
    std::memcpy(tunerBuffer.data() + tunerFillCount, input, sizeof(float) * toCopy);
    tunerFillCount += toCopy;

    if (tunerFillCount < kTunerBufSize)
        return;

    // Buffer is full — run analysis and reset.
    tunerFillCount = 0;

    const float sampleRate = static_cast<float>(getSampleRate());
    constexpr int analysisLen = kTunerBufSize / 2;  // first half as template, second as shifted

    float rms = 0.0f;
    for (int i = 0; i < kTunerBufSize; ++i)
        rms += tunerBuffer[i] * tunerBuffer[i];
    rms = std::sqrt(rms / kTunerBufSize);

    if (rms < 0.001f)
    {
        tunerFrequencyHz.store(0.0f);
        tunerCents.store(0.0f);
        tunerNoteIndex.store(-1);
        return;
    }

    // AMDF: lag range covers guitar B1 (61 Hz) to C7 (2093 Hz).
    const int minLag = static_cast<int>(sampleRate / 2100.0f);
    const int maxLag = juce::jmin(static_cast<int>(sampleRate / 55.0f), analysisLen - 1);

    float bestScore = std::numeric_limits<float>::max();
    int bestLag = -1;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float sum = 0.0f;
        for (int i = 0; i < analysisLen; i += 3)  // stride 3 for speed
            sum += std::abs(tunerBuffer[i] - tunerBuffer[i + lag]);

        if (sum < bestScore)
        {
            bestScore = sum;
            bestLag = lag;
        }
    }

    if (bestLag <= 0)
        return;

    const float frequency = sampleRate / static_cast<float>(bestLag);

    if (frequency < 55.0f || frequency > 2100.0f)
        return;

    const float midi = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    const int nearest = static_cast<int>(std::round(midi));
    const float cents = (midi - static_cast<float>(nearest)) * 100.0f;
    const int noteIndex = (nearest % 12 + 12) % 12;

    tunerFrequencyHz.store(frequency);
    tunerCents.store(cents);
    tunerNoteIndex.store(noteIndex);
}

#if ! JucePlugin_IsMidiEffect
bool VayuAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
   #if JucePlugin_IsSynth
    juce::ignoreUnused(layouts);
    return true;
   #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    {
        return false;
    }

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
   #endif
}
#endif

void VayuAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const float sr = static_cast<float>(getSampleRate());

    handleMidiLearnAndMapping(midiMessages);

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples             = buffer.getNumSamples();

    if (totalNumInputChannels == 0)
    {
        buffer.clear();
        return;
    }

    updateTuner(buffer);

    const float inputPeak = getBufferPeak(buffer, 0, totalNumInputChannels);
    inputMeterLevel.store(juce::jmax(inputPeak, inputMeterLevel.load() * 0.92f));

    // Apply input trim before processing
    {
        const float trimDb = apvts.getRawParameterValue("inputTrimDb")->load();
        if (std::abs(trimDb) > 0.05f)
        {
            const float trimGain = juce::Decibels::decibelsToGain(trimDb);
            for (int ch = 0; ch < totalNumInputChannels; ++ch)
                buffer.applyGain(ch, 0, buffer.getNumSamples(), trimGain);
        }
    }

    // Read parameters once per block — lock-free atomic loads
    const float drive    = apvts.getRawParameterValue("drive")->load();
    const float outputDb = apvts.getRawParameterValue("outputVolume")->load();
    const float gateThresholdDb = apvts.getRawParameterValue("gateThreshold")->load();
    const float gateAttackMs = apvts.getRawParameterValue("gateAttackMs")->load();
    const float gateReleaseMs = apvts.getRawParameterValue("gateReleaseMs")->load();
    const float gateHysteresisDb = apvts.getRawParameterValue("gateHysteresisDb")->load();
    const float gateRangeDb = apvts.getRawParameterValue("gateRangeDb")->load();
    const float boostDb = apvts.getRawParameterValue("boostDb")->load();
    const float delayTimeMs = apvts.getRawParameterValue("delayTimeMs")->load();
    const float delayMix = apvts.getRawParameterValue("delayMix")->load();
    const bool delaySync = apvts.getRawParameterValue("delaySync")->load() > 0.5f;
    const int delayDivision = static_cast<int>(apvts.getRawParameterValue("delayDivision")->load());
    const float reverbMix = apvts.getRawParameterValue("reverbMix")->load();
    const float reverbRoomSize = apvts.getRawParameterValue("reverbRoomSize")->load();
    const float reverbDamping = apvts.getRawParameterValue("reverbDamping")->load();
    const float reverbWidth = apvts.getRawParameterValue("reverbWidth")->load();
    const float reverbPreDelayMs = apvts.getRawParameterValue("reverbPreDelayMs")->load();
    const float limiterThreshDb = apvts.getRawParameterValue("limiterThreshDb")->load();
    const bool limiterEnabled = apvts.getRawParameterValue("limiterEnabled")->load() > 0.5f;
    const int ampType = static_cast<int>(apvts.getRawParameterValue("ampType")->load());
    const bool metalMode = apvts.getRawParameterValue("metalMode")->load() > 0.5f;
    const float tightLowCutHz = apvts.getRawParameterValue("tightLowCutHz")->load();
    const float pitchShiftSemi = apvts.getRawParameterValue("pitchShiftSemi")->load();
    const bool wahEnable = apvts.getRawParameterValue("wahEnable")->load() > 0.5f;
    const bool wahAuto = apvts.getRawParameterValue("wahAuto")->load() > 0.5f;
    const float wahCenterHz = apvts.getRawParameterValue("wahCenterHz")->load();
    const float wahDepth = apvts.getRawParameterValue("wahDepth")->load();
    const bool killEnable = apvts.getRawParameterValue("killEnable")->load() > 0.5f;
    const int killRate = static_cast<int>(apvts.getRawParameterValue("killRate")->load());
    const float killDepth = apvts.getRawParameterValue("killDepth")->load();
    const float ampBassDb = apvts.getRawParameterValue("ampBassDb")->load();
    const float ampMidDb = apvts.getRawParameterValue("ampMidDb")->load();
    const float ampTrebleDb = apvts.getRawParameterValue("ampTrebleDb")->load();
    const float ampPresenceDb = apvts.getRawParameterValue("ampPresenceDb")->load();
    const float irLowCutHz = apvts.getRawParameterValue("irLowCutHz")->load();
    const float irHighCutHz = apvts.getRawParameterValue("irHighCutHz")->load();
    const float irLevelDb = apvts.getRawParameterValue("irLevelDb")->load();
    const bool irPhaseInvert = apvts.getRawParameterValue("irPhaseInvert")->load() > 0.5f;
    const float cabBlend = apvts.getRawParameterValue("cabBlend")->load();
    const float cabLevelA = apvts.getRawParameterValue("cabLevelA")->load();
    const float cabLevelB = apvts.getRawParameterValue("cabLevelB")->load();
    const bool cabFlipA = apvts.getRawParameterValue("cabFlipA")->load() > 0.5f;
    const bool cabFlipB = apvts.getRawParameterValue("cabFlipB")->load() > 0.5f;
    const float cabPan = apvts.getRawParameterValue("cabPan")->load();
    const float cabAlignDelayMs = apvts.getRawParameterValue("cabAlignDelayMs")->load();
    const int oversamplingMode = static_cast<int>(apvts.getRawParameterValue("oversamplingMode")->load());
#if VAYU_HAS_NAM
    const bool namBypass = apvts.getRawParameterValue("namBypass")->load() > 0.5f;
    const bool namAutoMatch = apvts.getRawParameterValue("namAutoMatch")->load() > 0.5f;
    const float namBlend = apvts.getRawParameterValue("namBlend")->load();
    auto activeNamBlock = !namBypass ? std::atomic_load_explicit(&namEngine, std::memory_order_acquire) : std::shared_ptr<nam::DSP>{};
    const bool namStageActive = activeNamBlock != nullptr;
#endif

    // New effect parameters
    const bool compEnable = apvts.getRawParameterValue("compEnable")->load() > 0.5f;
    const float compThreshDb = apvts.getRawParameterValue("compThreshDb")->load();
    const float compRatio = apvts.getRawParameterValue("compRatio")->load();
    const float compAttackMs = apvts.getRawParameterValue("compAttackMs")->load();
    const float compReleaseMs = apvts.getRawParameterValue("compReleaseMs")->load();
    const float compMakeupDb = apvts.getRawParameterValue("compMakeupDb")->load();
    const float compMix = apvts.getRawParameterValue("compMix")->load();

    const bool distEnable = apvts.getRawParameterValue("distEnable")->load() > 0.5f;
    const float distGainDb = apvts.getRawParameterValue("distGainDb")->load();
    const float distDrive = apvts.getRawParameterValue("distDrive")->load();
    const float distTone = apvts.getRawParameterValue("distTone")->load();
    const float distMix = apvts.getRawParameterValue("distMix")->load();
    const float distOutputDb = apvts.getRawParameterValue("distOutputDb")->load();
    const int distMode = static_cast<int>(apvts.getRawParameterValue("distMode")->load());
    const float distTightHz = apvts.getRawParameterValue("distTightHz")->load();
    const float distHiCutHz = apvts.getRawParameterValue("distHiCutHz")->load();
    const float distPresenceDb = apvts.getRawParameterValue("distPresenceDb")->load();
    const float distGateDb = apvts.getRawParameterValue("distGateDb")->load();

    const bool chorusEnable = apvts.getRawParameterValue("chorusEnable")->load() > 0.5f;
    const float chorusRate = apvts.getRawParameterValue("chorusRate")->load();
    const float chorusDepthParam = apvts.getRawParameterValue("chorusDepth")->load();
    const float chorusMix = apvts.getRawParameterValue("chorusMix")->load();

    const bool phaserEnable = apvts.getRawParameterValue("phaserEnable")->load() > 0.5f;
    const float phaserRate = apvts.getRawParameterValue("phaserRate")->load();
    const float phaserDepthParam = apvts.getRawParameterValue("phaserDepth")->load();
    const float phaserFeedbackParam = apvts.getRawParameterValue("phaserFeedback")->load();
    const float phaserMix = apvts.getRawParameterValue("phaserMix")->load();

    const bool flangerEnable = apvts.getRawParameterValue("flangerEnable")->load() > 0.5f;
    const float flangerRate = apvts.getRawParameterValue("flangerRate")->load();
    const float flangerDepthParam = apvts.getRawParameterValue("flangerDepth")->load();
    const float flangerFeedbackParam = apvts.getRawParameterValue("flangerFeedback")->load();
    const float flangerMix = apvts.getRawParameterValue("flangerMix")->load();

    const bool tremoloEnable = apvts.getRawParameterValue("tremoloEnable")->load() > 0.5f;
    const float tremoloRate = apvts.getRawParameterValue("tremoloRate")->load();
    const float tremoloDepthParam = apvts.getRawParameterValue("tremoloDepth")->load();
    const bool tremoloSync = apvts.getRawParameterValue("tremoloSync")->load() > 0.5f;
    const int tremoloShape = static_cast<int>(apvts.getRawParameterValue("tremoloShape")->load());
    const int tremoloDivision = static_cast<int>(apvts.getRawParameterValue("tremoloDivision")->load());

    const bool peqEnable = apvts.getRawParameterValue("peqEnable")->load() > 0.5f;
    const float peqFreq1 = apvts.getRawParameterValue("peqBand1Freq")->load();
    const float peqGain1 = apvts.getRawParameterValue("peqBand1GainDb")->load();
    const float peqQ1 = apvts.getRawParameterValue("peqBand1Q")->load();
    const float peqFreq2 = apvts.getRawParameterValue("peqBand2Freq")->load();
    const float peqGain2 = apvts.getRawParameterValue("peqBand2GainDb")->load();
    const float peqQ2 = apvts.getRawParameterValue("peqBand2Q")->load();
    const float peqFreq3 = apvts.getRawParameterValue("peqBand3Freq")->load();
    const float peqGain3 = apvts.getRawParameterValue("peqBand3GainDb")->load();
    const float peqQ3 = apvts.getRawParameterValue("peqBand3Q")->load();

    const bool metroEnable = apvts.getRawParameterValue("metroEnable")->load() > 0.5f;
    const bool metroSync = apvts.getRawParameterValue("metroSync")->load() > 0.5f;
    const float metroBpm = apvts.getRawParameterValue("metroBpm")->load();
    const float metroLevel = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("metroLevel")->load());
    const int metroTimeSig = static_cast<int>(apvts.getRawParameterValue("metroTimeSig")->load());
    const float looperLevelGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("looperLevel")->load());

    const bool needsTempo = delaySync || killEnable || (tremoloEnable && tremoloSync) || (metroEnable && metroSync);
    const bool needsTransportState = killEnable || metroSync;
    const double resolvedTempoBpm = needsTempo ? resolveTempoBpm(static_cast<double>(metroBpm)) : static_cast<double>(metroBpm);

    bool hostIsPlaying = true;
    if (needsTransportState)
    {
        if (auto* hostPlayHead = getPlayHead())
        {
            if (auto position = hostPlayHead->getPosition())
                hostIsPlaying = position->getIsPlaying();
        }
    }

    if (!hostWasPlaying && hostIsPlaying)
    {
        // Realign synced modulators/click on transport start.
        delayLfoPhase = 0.0f;
        killLfoPhase = 0.0f;
        tremoloLfoPhase = 0.0f;
        metroPhase = 0.0f;
        metroBeatCount = 0;
        metroCurrentClickBeat = 0;
        metroClickPhase = 0.0f;
        metroClickOscPhase = 0.0f;
        metroClickActive = false;
    }

    if (metroSync && !hostIsPlaying)
    {
        metroClickActive = false;
        metroClickPhase = 0.0f;
        metroClickOscPhase = 0.0f;
    }

    const float outputGain = juce::Decibels::decibelsToGain(outputDb);
    const float gateThresholdLinear = juce::Decibels::decibelsToGain(gateThresholdDb);
    const float gateCloseLinear = juce::Decibels::decibelsToGain(gateThresholdDb - gateHysteresisDb);
    const float gateRangeLinear = juce::Decibels::decibelsToGain(gateRangeDb);
    const float boostGain = juce::Decibels::decibelsToGain(boostDb);
    const float irLevelGain = juce::Decibels::decibelsToGain(irLevelDb);
    const float cabAGain = juce::Decibels::decibelsToGain(cabLevelA);
    const float cabBGain = juce::Decibels::decibelsToGain(cabLevelB);
    const float cabAPolarity = cabFlipA ? -1.0f : 1.0f;
    const float cabBPolarity = cabFlipB ? -1.0f : 1.0f;
    const float cabAlignDelaySamples = cabAlignDelayMs * 0.001f * sr;
    const float delayFeedback  = apvts.getRawParameterValue("delayFeedback")->load();
    const float delayModRate   = apvts.getRawParameterValue("delayModRate")->load();
    const float delayModDepth  = apvts.getRawParameterValue("delayModDepth")->load();
    const float delayModDepthSamples = delayModDepth * 0.001f * sr;
    const float attackSamples = juce::jmax(1.0f, gateAttackMs * 0.001f * sr);
    const float releaseSamples = juce::jmax(1.0f, gateReleaseMs * 0.001f * sr);
    const float envAttackCoeff = std::exp(-1.0f / attackSamples);
    const float envReleaseCoeff = std::exp(-1.0f / releaseSamples);
    const float gateGainCoeff = envReleaseCoeff;

    float ampSaturation = 1.0f;
    float ampPostEQ = 1.0f;

    if (ampType == 0)      { ampSaturation = 0.72f; ampPostEQ = 1.05f; }
    else if (ampType == 2) { ampSaturation = 1.38f; ampPostEQ = 0.92f; }
    if (metalMode)
    {
        ampSaturation *= 1.16f;
        ampPostEQ *= 0.96f;
    }

    const auto bassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(getSampleRate(), 120.0, 0.707f, juce::Decibels::decibelsToGain(ampBassDb));
    const auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), 750.0, 0.8f, juce::Decibels::decibelsToGain(ampMidDb));
    const auto trebleCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(getSampleRate(), 3200.0, 0.707f, juce::Decibels::decibelsToGain(ampTrebleDb));
    const auto presenceCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(getSampleRate(), 6000.0, 0.707f, juce::Decibels::decibelsToGain(ampPresenceDb));

    ampBassL.coefficients = bassCoeffs;
    ampBassR.coefficients = bassCoeffs;
    ampMidL.coefficients = midCoeffs;
    ampMidR.coefficients = midCoeffs;
    ampTrebleL.coefficients = trebleCoeffs;
    ampTrebleR.coefficients = trebleCoeffs;
    ampPresenceL.coefficients = presenceCoeffs;
    ampPresenceR.coefficients = presenceCoeffs;

    const auto tightCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        getSampleRate(),
        metalMode ? tightLowCutHz : 30.0f);
    tightFilterL.coefficients = tightCoeffs;
    tightFilterR.coefficients = tightCoeffs;

    const float autoSweep = 0.5f + 0.5f * std::sin(wahLfoPhase);
    const float wahCenter = wahAuto
        ? juce::jlimit(250.0f, 2200.0f, 350.0f + autoSweep * 1700.0f * juce::jlimit(0.0f, 1.0f, wahDepth))
        : wahCenterHz;
    const float wahQ = juce::jlimit(0.4f, 6.0f, 0.8f + wahDepth * 3.6f);
    const auto wahCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(getSampleRate(), wahCenter, wahQ);
    wahFilterL.coefficients = wahCoeffs;
    wahFilterR.coefficients = wahCoeffs;

    irLowCutL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(getSampleRate(), irLowCutHz);
    irLowCutR.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(getSampleRate(), irLowCutHz);
    irHighCutL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), irHighCutHz);
    irHighCutR.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), irHighCutHz);

    float effectiveDelayMs = delayTimeMs;
    if (delaySync)
    {
        static constexpr float divisions[] = { 1.0f, 0.5f, 0.75f, 1.0f / 3.0f, 0.25f };
        const int divIndex = juce::jlimit(0, 4, delayDivision);
        effectiveDelayMs = static_cast<float>(60000.0 / resolvedTempoBpm) * divisions[divIndex];
    }
    // delaySamples used as base; actual setDelay is per-sample inside the LFO loop

    // ═══════════════════════════════════════════════════════════════════
    //  COMPRESSOR (pre-amp, parallel mix)
    // ═══════════════════════════════════════════════════════════════════
    if (compEnable)
    {
        const float compThreshLin = juce::Decibels::decibelsToGain(compThreshDb);
        const float compAttCoeff = std::exp(-1.0f / juce::jmax(1.0f, compAttackMs * 0.001f * static_cast<float>(getSampleRate())));
        const float compRelCoeff = std::exp(-1.0f / juce::jmax(1.0f, compReleaseMs * 0.001f * static_cast<float>(getSampleRate())));
        const float makeupGain = juce::Decibels::decibelsToGain(compMakeupDb);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Peak detect across channels
            float peak = 0.0f;
            for (int ch = 0; ch < totalNumInputChannels; ++ch)
                peak = juce::jmax(peak, std::abs(buffer.getSample(ch, sample)));

            // Envelope follower
            if (peak > compEnvelope)
                compEnvelope = compAttCoeff * compEnvelope + (1.0f - compAttCoeff) * peak;
            else
                compEnvelope = compRelCoeff * compEnvelope + (1.0f - compRelCoeff) * peak;

            // Gain computer
            float gainReduction = 1.0f;
            if (compEnvelope > compThreshLin && compEnvelope > 1e-10f)
            {
                const float envDb = juce::Decibels::gainToDecibels(compEnvelope);
                const float overDb = envDb - compThreshDb;
                const float compressedDb = overDb * (1.0f - 1.0f / compRatio);
                gainReduction = juce::Decibels::decibelsToGain(-compressedDb);
            }

            for (int ch = 0; ch < totalNumInputChannels; ++ch)
            {
                const float dry = buffer.getSample(ch, sample);
                const float wet = dry * gainReduction * makeupGain;
                buffer.setSample(ch, sample, dry * (1.0f - compMix) + wet * compMix);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    //  PARAMETRIC EQ setup
    // ═══════════════════════════════════════════════════════════════════
    if (peqEnable)
    {
        auto peqCoeffs1 = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peqFreq1, peqQ1, juce::Decibels::decibelsToGain(peqGain1));
        auto peqCoeffs2 = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peqFreq2, peqQ2, juce::Decibels::decibelsToGain(peqGain2));
        auto peqCoeffs3 = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peqFreq3, peqQ3, juce::Decibels::decibelsToGain(peqGain3));
        peqL1.coefficients = peqCoeffs1; peqR1.coefficients = peqCoeffs1;
        peqL2.coefficients = peqCoeffs2; peqR2.coefficients = peqCoeffs2;
        peqL3.coefficients = peqCoeffs3; peqR3.coefficients = peqCoeffs3;
    }

    auto processAmpStage = [&](auto block)
    {
        const int blockChannels = static_cast<int>(block.getNumChannels());
        const int blockSamples = static_cast<int>(block.getNumSamples());

        for (int channel = 0; channel < blockChannels; ++channel)
        {
            auto* channelData = block.getChannelPointer(static_cast<size_t>(channel));

            for (int sample = 0; sample < blockSamples; ++sample)
            {
                const float in = channelData[sample];
                const float inputAbs = std::abs(in);

                if (inputAbs > gateEnvelope)
                    gateEnvelope = (envAttackCoeff * gateEnvelope) + ((1.0f - envAttackCoeff) * inputAbs);
                else
                    gateEnvelope = (envReleaseCoeff * gateEnvelope) + ((1.0f - envReleaseCoeff) * inputAbs);

                if (gateIsOpen)
                {
                    if (gateEnvelope < gateCloseLinear)
                        gateIsOpen = false;
                }
                else if (gateEnvelope > gateThresholdLinear)
                {
                    gateIsOpen = true;
                }

                const float targetGate = gateIsOpen ? 1.0f : gateRangeLinear;
                gateGain = (gateGainCoeff * gateGain) + ((1.0f - gateGainCoeff) * targetGate);

                const float gated = in * gateGain;
                const float boosted = gated * boostGain;
                const float tightened = (channel == 0 ? tightFilterL.processSample(boosted)
                                                      : tightFilterR.processSample(boosted));
                const float wahed = wahEnable
                    ? (channel == 0 ? wahFilterL.processSample(tightened)
                                    : wahFilterR.processSample(tightened))
                    : tightened;
                float amped = std::tanh(wahed * drive * ampSaturation) * ampPostEQ;
                if (metalMode)
                {
                    const float hard = juce::jlimit(-0.95f, 0.95f, wahed * drive * 0.88f);
                    amped = amped * 0.62f + hard * 0.38f;
                }
                channelData[sample] = amped * outputGain;
            }
        }
    };

    juce::dsp::AudioBlock<float> block(buffer);
    auto inOutBlock = block.getSubsetChannelBlock(0, static_cast<size_t>(totalNumInputChannels));

    // ── When NAM is active, skip amp sim / gate / oversampling / distortion entirely.
    //    NAM models include their own amp modeling — doubling up destroys the signal.
#if VAYU_HAS_NAM
    if (!namStageActive)
#endif
    {
        // Pre-FX and amp stage with optional oversampling.
        if (oversamplingMode == 1 && oversampling2x != nullptr)
        {
            auto up = oversampling2x->processSamplesUp(juce::dsp::AudioBlock<const float>(inOutBlock));
            processAmpStage(up);
            oversampling2x->processSamplesDown(inOutBlock);
        }
        else if (oversamplingMode == 2 && oversampling4x != nullptr)
        {
            auto up = oversampling4x->processSamplesUp(juce::dsp::AudioBlock<const float>(inOutBlock));
            processAmpStage(up);
            oversampling4x->processSamplesDown(inOutBlock);
        }
        else
        {
            processAmpStage(inOutBlock);
        }
    }

    // Distortion stomp stage (between amp and NAM).
    if (distEnable
#if VAYU_HAS_NAM
        && !namStageActive
#endif
        )
    {
        const float distPreGain = juce::Decibels::decibelsToGain(distGainDb);
        const float distOutGain = juce::Decibels::decibelsToGain(distOutputDb);
        const float driveAmt = juce::jlimit(0.0f, 1.0f, distDrive);
        const float toneAmt = juce::jlimit(0.0f, 1.0f, distTone);
        const float wetAmt = juce::jlimit(0.0f, 1.0f, distMix);
        const float dryAmt = 1.0f - wetAmt;
        const int mode = juce::jlimit(0, 3, distMode);

        float modePreMul = 1.0f;
        float modeHardBoost = 1.0f;
        float modeFoldBoost = 1.0f;
        float lpMin = 900.0f;
        float lpMax = 7200.0f;
        float hpMin = 120.0f;
        float hpMax = 260.0f;

        if (mode == 1) // Brutal
        {
            modePreMul = 1.45f;
            modeHardBoost = 1.28f;
            modeFoldBoost = 1.45f;
            hpMin = 150.0f;
            hpMax = 330.0f;
        }
        else if (mode == 2) // Fuzz
        {
            modePreMul = 1.65f;
            modeHardBoost = 0.72f;
            modeFoldBoost = 2.1f;
            lpMin = 700.0f;
            lpMax = 4200.0f;
            hpMin = 80.0f;
            hpMax = 190.0f;
        }
        else if (mode == 3) // Insane
        {
            modePreMul = 2.25f;
            modeHardBoost = 1.45f;
            modeFoldBoost = 2.6f;
            lpMin = 850.0f;
            lpMax = 5200.0f;
            hpMin = 170.0f;
            hpMax = 360.0f;
        }

        const float driveCurve = 0.35f + driveAmt * driveAmt * 1.85f;
        const float hardAmt = juce::jlimit(0.10f, 0.95f, (0.25f + driveAmt * 0.7f) * modeHardBoost);
        const float foldAmt = juce::jlimit(0.0f, 1.0f, juce::jmax(0.0f, (driveAmt - 0.5f) * 2.1f) * modeFoldBoost);
        const float lpCutHz = juce::jlimit(2000.0f, 14000.0f,
            juce::jmap(toneAmt, lpMin, lpMax) * (distHiCutHz / 6500.0f));
        const float hpCutHz = juce::jlimit(40.0f, 500.0f,
            juce::jmap(toneAmt, hpMin, hpMax) * (distTightHz / 140.0f));
        const float presenceGain = juce::Decibels::decibelsToGain(distPresenceDb);
        const float lpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * lpCutHz / sr);
        const float hpCoeff = std::exp(-juce::MathConstants<float>::twoPi * hpCutHz / sr);

        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            float& toneState = (ch % 2 == 0) ? distToneStateL : distToneStateR;
            float hpState = 0.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                const float dry = data[i];
                const float boosted = dry * distPreGain * modePreMul * (1.0f + 4.6f * driveCurve);

                // Tighten lows before clipping to keep palm-mutes punchy.
                hpState = hpCoeff * hpState + (1.0f - hpCoeff) * boosted;
                const float x = boosted - hpState * 0.92f;

                const float soft1 = std::tanh(x * (2.0f + 8.0f * driveCurve));
                const float soft2 = std::tanh(soft1 * (1.5f + 8.5f * driveCurve));
                const float hard = juce::jlimit(-1.0f, 1.0f, x * (1.6f + 6.0f * driveCurve));

                // Fuzz/Insane modes: asymmetry and gated tail texture.
                const float asym = (mode >= 2) ? std::tanh((x + 0.12f) * (2.0f + 7.0f * driveCurve)) : soft2;

                float folded = asym;
                if (foldAmt > 0.0f)
                {
                    const float f = asym * (1.0f + foldAmt * 2.8f);
                    folded = std::sin(f * juce::MathConstants<float>::pi);
                }

                float clipped = (asym * (1.0f - hardAmt) + hard * hardAmt) * (1.0f - foldAmt * 0.35f)
                              + folded * (foldAmt * 0.35f);
                if (mode == 1)
                    clipped = std::tanh(clipped * (1.25f + driveAmt * 1.7f));
                else if (mode == 2)
                    clipped = juce::jlimit(-1.0f, 1.0f, clipped * (1.35f + driveAmt * 2.0f));
                else if (mode == 3)
                {
                    const float oct = std::sin(clipped * juce::MathConstants<float>::twoPi) * 0.22f;
                    clipped = std::tanh((clipped + oct) * (1.8f + driveAmt * 2.7f));
                }

                toneState += lpCoeff * (clipped - toneState);
                const float presence = clipped - toneState;
                const float toned = toneState + presence * (0.20f + toneAmt * 1.15f) * presenceGain;
                const float wet = std::tanh(toned * (1.0f + driveAmt * 0.5f)) * distOutGain;

                data[i] = dry * dryAmt + wet * wetAmt;
            }
        }

        // Cleanup gate reduces ghost tails / overlapping fizz while idle.
        const float gateOpenLin = juce::Decibels::decibelsToGain(distGateDb);
        const float gateCloseLin = gateOpenLin * 0.63f;
        const float envAtk = std::exp(-1.0f / juce::jmax(1.0f, 0.0012f * sr));
        const float envRel = std::exp(-1.0f / juce::jmax(1.0f, 0.020f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < totalNumOutputChannels; ++ch)
            {
                const float s = buffer.getSample(ch, i);
                peak = juce::jmax(peak, std::abs(s));
            }

            if (peak > distGateEnv)
                distGateEnv = envAtk * distGateEnv + (1.0f - envAtk) * peak;
            else
                distGateEnv = envRel * distGateEnv + (1.0f - envRel) * peak;

            if (distGateOpen)
            {
                if (distGateEnv < gateCloseLin)
                    distGateOpen = false;
            }
            else if (distGateEnv > gateOpenLin)
            {
                distGateOpen = true;
            }

            const float target = distGateOpen ? 1.0f : 0.10f;
            distGateGain += (target - distGateGain) * 0.12f;

            for (int ch = 0; ch < totalNumOutputChannels; ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * distGateGain);
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    //  NAM STAGE — clean input → NAM model → output volume
    //  Mono models: process L channel once, copy result to R.
    //  Stereo models: process L+R through the model's 2-in/2-out path.
    //  Signal chain before this point is SKIPPED when NAM is active
    //  (gate, amp sim, oversampling, distortion all bypassed above).
    // ═══════════════════════════════════════════════════════════════════
#if VAYU_HAS_NAM
    if (!namBypass)
    {
        if (auto activeNam = activeNamBlock)
        {
            if (numSamples <= static_cast<int>(namScratchA.size()) && numSamples <= static_cast<int>(namDryA.size()))
            {
                auto measureStereoRms = [&buffer, numSamples, totalNumOutputChannels]()
                {
                    const int chCount = juce::jmin(2, totalNumOutputChannels);
                    if (chCount <= 0 || numSamples <= 0)
                        return 0.0f;

                    double sumSquares = 0.0;
                    int count = 0;
                    for (int ch = 0; ch < chCount; ++ch)
                    {
                        const float* data = buffer.getReadPointer(ch);
                        for (int i = 0; i < numSamples; ++i)
                        {
                            const float s = data[i];
                            sumSquares += static_cast<double>(s * s);
                        }
                        count += numSamples;
                    }

                    if (count <= 0)
                        return 0.0f;

                    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(count)));
                };

                const bool applyNamMatch = namAutoMatch;
                const float preNamRms = applyNamMatch ? measureStereoRms() : 0.0f;
                const int namIn = activeNam->NumInputChannels();
                const int namOut = activeNam->NumOutputChannels();

                // Save dry signal for blend
                if (namBlend < 0.999f)
                {
                    std::memcpy(namDryA.data(), buffer.getReadPointer(0),
                                static_cast<size_t>(numSamples) * sizeof(float));
                    if (totalNumOutputChannels > 1)
                        std::memcpy(namDryB.data(), buffer.getReadPointer(1),
                                    static_cast<size_t>(numSamples) * sizeof(float));
                }

                if (namIn >= 2 && namOut >= 2 && totalNumOutputChannels > 1)
                {
                    // ── Stereo NAM model (2-in → 2-out) ──
                    float* inPtrs[2] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
                    float* outPtrs[2] = { namScratchA.data(), namScratchB.data() };
                    activeNam->process(inPtrs, outPtrs, numSamples);

                    std::memcpy(buffer.getWritePointer(0), namScratchA.data(),
                                static_cast<size_t>(numSamples) * sizeof(float));
                    std::memcpy(buffer.getWritePointer(1), namScratchB.data(),
                                static_cast<size_t>(numSamples) * sizeof(float));
                }
                else
                {
                    // ── Mono NAM model (1-in → 1-out) ──
                    // Process L channel only through single engine, copy to R.
                    // Guitar input is typically mono — this avoids internal
                    // state corruption from processing the same engine twice.
                    float* inPtrs[1] = { buffer.getWritePointer(0) };
                    float* outPtrs[1] = { namScratchA.data() };
                    activeNam->process(inPtrs, outPtrs, numSamples);

                    std::memcpy(buffer.getWritePointer(0), namScratchA.data(),
                                static_cast<size_t>(numSamples) * sizeof(float));
                    if (totalNumOutputChannels > 1)
                        std::memcpy(buffer.getWritePointer(1), namScratchA.data(),
                                    static_cast<size_t>(numSamples) * sizeof(float));
                }

                // Blend dry/wet
                if (namBlend < 0.999f)
                {
                    const float dryMix = 1.0f - namBlend;
                    auto* outL = buffer.getWritePointer(0);
                    for (int i = 0; i < numSamples; ++i)
                        outL[i] = namDryA[static_cast<size_t>(i)] * dryMix + outL[i] * namBlend;

                    if (totalNumOutputChannels > 1)
                    {
                        auto* outR = buffer.getWritePointer(1);
                        for (int i = 0; i < numSamples; ++i)
                            outR[i] = namDryB[static_cast<size_t>(i)] * dryMix + outR[i] * namBlend;
                    }
                }

                // Auto-match gain compensation
                if (applyNamMatch)
                {
                    const float postNamRms = measureStereoRms();
                    constexpr float smooth = 0.96f;
                    namMatchInRms = (smooth * namMatchInRms) + ((1.0f - smooth) * preNamRms);
                    namMatchOutRms = (smooth * namMatchOutRms) + ((1.0f - smooth) * postNamRms);

                    if (namMatchOutRms > 1.0e-6f && namMatchInRms > 1.0e-6f)
                    {
                        const float matchGain = juce::jlimit(0.125f, 16.0f, namMatchInRms / namMatchOutRms);
                        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
                            buffer.applyGain(ch, 0, numSamples, matchGain);
                        namMatchGainDb.store(juce::Decibels::gainToDecibels(matchGain));
                    }
                }
            }
        }
    }

    // Apply output volume after NAM (since amp stage was skipped)
    if (namStageActive && std::abs(outputDb) > 0.05f)
    {
        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
            buffer.applyGain(ch, 0, numSamples, outputGain);
    }
#endif

    // Tone stack at base sample rate for predictable voicing across oversampling modes.
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto& bass = (channel == 0) ? ampBassL : ampBassR;
        auto& mid = (channel == 0) ? ampMidL : ampMidR;
        auto& treble = (channel == 0) ? ampTrebleL : ampTrebleR;
        auto& presence = (channel == 0) ? ampPresenceL : ampPresenceR;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float x = channelData[sample];
            x = bass.processSample(x);
            x = mid.processSample(x);
            x = treble.processSample(x);
            x = presence.processSample(x);
            channelData[sample] = x;
        }
    }

    // Key shift (simple real-time resample transposer for quick down/up tune workflow).
    if (std::abs(pitchShiftSemi) > 0.01f && numSamples > 4)
    {
        const float ratio = std::pow(2.0f, pitchShiftSemi / 12.0f);
        pitchBuffer.makeCopyOf(buffer, true);
        const float sourceLen = static_cast<float>(numSamples - 1);

        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            const float* src = pitchBuffer.getReadPointer(channel);
            float* dst = buffer.getWritePointer(channel);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float srcPos = static_cast<float>(sample) * ratio;
                while (srcPos > sourceLen)
                    srcPos -= sourceLen;
                const int i0 = static_cast<int>(srcPos);
                const int i1 = juce::jmin(i0 + 1, numSamples - 1);
                const float frac = srcPos - static_cast<float>(i0);
                dst[sample] = src[i0] + (src[i1] - src[i0]) * frac;
            }
        }
    }

    if (wahEnable)
    {
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* data = buffer.getWritePointer(channel);
            auto& wah = (channel == 0) ? wahFilterL : wahFilterR;
            for (int sample = 0; sample < numSamples; ++sample)
                data[sample] = wah.processSample(data[sample]);
        }
    }

    if (killEnable && hostIsPlaying)
    {
        constexpr float killDivisions[] = { 1.0f, 2.0f, 4.0f, 8.0f };
        const int rateIndex = juce::jlimit(0, 3, killRate);
        const float hz = static_cast<float>(resolvedTempoBpm / 60.0) * killDivisions[rateIndex];
        const float phaseInc = juce::MathConstants<float>::twoPi * hz / static_cast<float>(getSampleRate());
        const float depth = juce::jlimit(0.0f, 1.0f, killDepth);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            killLfoPhase += phaseInc;
            if (killLfoPhase > juce::MathConstants<float>::twoPi)
                killLfoPhase -= juce::MathConstants<float>::twoPi;

            const float square = std::sin(killLfoPhase) > 0.0f ? 1.0f : 0.0f;
            const float gate = (1.0f - depth) + square * depth;

            for (int channel = 0; channel < totalNumOutputChannels; ++channel)
                buffer.setSample(channel, sample, buffer.getSample(channel, sample) * gate);
        }
    }

    wahLfoPhase += juce::MathConstants<float>::twoPi * (0.3f + wahDepth * 2.5f)
                   * static_cast<float>(numSamples) / static_cast<float>(getSampleRate());
    if (wahLfoPhase > juce::MathConstants<float>::twoPi)
        wahLfoPhase = std::fmod(wahLfoPhase, juce::MathConstants<float>::twoPi);

    // Copy processed input channels into any extra output channels.
    for (int channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.copyFrom(channel, 0, buffer, channel % totalNumInputChannels, 0, numSamples);

    // Dual cabinet simulation: process slot A and B, then blend.
    cabBufferA.makeCopyOf(buffer, true);
    cabBufferB.makeCopyOf(buffer, true);

    juce::dsp::AudioBlock<float> cabBlockA(cabBufferA);
    juce::dsp::AudioBlock<float> cabBlockB(cabBufferB);
    juce::dsp::ProcessContextReplacing<float> cabContextA(cabBlockA);
    juce::dsp::ProcessContextReplacing<float> cabContextB(cabBlockB);
    cabinetConvolutionA.process(cabContextA);
    cabinetConvolutionB.process(cabContextB);

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* out = buffer.getWritePointer(channel);
        const auto* a = cabBufferA.getReadPointer(channel);
        const auto* b = cabBufferB.getReadPointer(channel);

        auto readShiftedB = [b, numSamples, cabAlignDelaySamples](int sampleIndex)
        {
            const float sourceIndex = static_cast<float>(sampleIndex) - cabAlignDelaySamples;
            const int i0 = static_cast<int>(std::floor(sourceIndex));
            const int i1 = i0 + 1;
            const float frac = sourceIndex - static_cast<float>(i0);

            const float s0 = (i0 >= 0 && i0 < numSamples) ? b[i0] : 0.0f;
            const float s1 = (i1 >= 0 && i1 < numSamples) ? b[i1] : 0.0f;
            return s0 + (s1 - s0) * frac;
        };

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float slotA = a[sample] * cabAGain * cabAPolarity;
            const float slotB = readShiftedB(sample) * cabBGain * cabBPolarity;
            out[sample] = slotA * (1.0f - cabBlend) + slotB * cabBlend;
        }
    }

    // IR section finishing: cab low/high cut + level + phase + stereo pan.
    const float panL = std::cos((cabPan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
    const float panR = std::sin((cabPan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        auto l = buffer.getSample(0, sample);
        l = irLowCutL.processSample(l);
        l = irHighCutL.processSample(l);

        float r = (totalNumOutputChannels > 1) ? buffer.getSample(1, sample) : l;
        r = irLowCutR.processSample(r);
        r = irHighCutR.processSample(r);

        if (irPhaseInvert)
        {
            l = -l;
            r = -r;
        }

        l *= irLevelGain;
        r *= irLevelGain;

        l *= panL;
        r *= panR;

        buffer.setSample(0, sample, l);
        if (totalNumOutputChannels > 1)
            buffer.setSample(1, sample, r);
    }

    // ═══════════════════════════════════════════════════════════════════
    //  PARAMETRIC EQ (post-cab, pre-modulation)
    // ═══════════════════════════════════════════════════════════════════
    if (peqEnable)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float l = buffer.getSample(0, sample);
            l = peqL1.processSample(l);
            l = peqL2.processSample(l);
            l = peqL3.processSample(l);
            buffer.setSample(0, sample, l);

            if (totalNumOutputChannels > 1)
            {
                float r = buffer.getSample(1, sample);
                r = peqR1.processSample(r);
                r = peqR2.processSample(r);
                r = peqR3.processSample(r);
                buffer.setSample(1, sample, r);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    //  CHORUS (stereo, LFO-modulated delay)
    // ═══════════════════════════════════════════════════════════════════
    if (chorusEnable)
    {
        const float chorusMaxDelayMs = 7.0f; // ms
        const float chorusBaseDelay = 5.0f;   // ms base delay
        const float chorusDepthMs = chorusDepthParam * chorusMaxDelayMs;
        const float chorusPhaseInc = juce::MathConstants<float>::twoPi * chorusRate / static_cast<float>(getSampleRate());

        for (int sample = 0; sample < numSamples; ++sample)
        {
            chorusLfoPhase += chorusPhaseInc;
            if (chorusLfoPhase > juce::MathConstants<float>::twoPi)
                chorusLfoPhase -= juce::MathConstants<float>::twoPi;

            const float lfoL = std::sin(chorusLfoPhase);
            const float lfoR = std::sin(chorusLfoPhase + juce::MathConstants<float>::pi * 0.5f); // 90° offset for stereo

            const float delayL = (chorusBaseDelay + lfoL * chorusDepthMs) * 0.001f * static_cast<float>(getSampleRate());
            const float delayR = (chorusBaseDelay + lfoR * chorusDepthMs) * 0.001f * static_cast<float>(getSampleRate());

            const float dryL = buffer.getSample(0, sample);
            chorusDelayL.pushSample(0, dryL);
            const float wetL = chorusDelayL.popSample(0, juce::jmax(1.0f, delayL));
            buffer.setSample(0, sample, dryL * (1.0f - chorusMix) + wetL * chorusMix);

            if (totalNumOutputChannels > 1)
            {
                const float dryR = buffer.getSample(1, sample);
                chorusDelayR.pushSample(0, dryR);
                const float wetR = chorusDelayR.popSample(0, juce::jmax(1.0f, delayR));
                buffer.setSample(1, sample, dryR * (1.0f - chorusMix) + wetR * chorusMix);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    //  PHASER (4-stage allpass, stereo)
    // ═══════════════════════════════════════════════════════════════════
    if (phaserEnable)
    {
        const float phaserMinFreq = 200.0f;
        const float phaserMaxFreq = 4000.0f;
        const float phaserPhaseInc = juce::MathConstants<float>::twoPi * phaserRate / static_cast<float>(getSampleRate());

        for (int sample = 0; sample < numSamples; ++sample)
        {
            phaserLfoPhase += phaserPhaseInc;
            if (phaserLfoPhase > juce::MathConstants<float>::twoPi)
                phaserLfoPhase -= juce::MathConstants<float>::twoPi;

            const float lfo = 0.5f + 0.5f * std::sin(phaserLfoPhase);
            const float notchFreq = phaserMinFreq + (phaserMaxFreq - phaserMinFreq) * lfo * phaserDepthParam;
            const float apCoeff = (std::tan(juce::MathConstants<float>::pi * notchFreq / static_cast<float>(getSampleRate())) - 1.0f)
                                / (std::tan(juce::MathConstants<float>::pi * notchFreq / static_cast<float>(getSampleRate())) + 1.0f);

            auto processAllpass = [&](float in, std::array<float, kPhaserStages>& state) -> float
            {
                float x = in + phaserFeedbackParam * state[kPhaserStages - 1];
                for (int s = 0; s < kPhaserStages; ++s)
                {
                    const float y = apCoeff * x + state[s];
                    state[s] = x - apCoeff * y;
                    x = y;
                }
                return x;
            };

            const float dryL = buffer.getSample(0, sample);
            const float wetL = processAllpass(dryL, phaserAPStateL);
            buffer.setSample(0, sample, dryL * (1.0f - phaserMix) + wetL * phaserMix);

            if (totalNumOutputChannels > 1)
            {
                const float dryR = buffer.getSample(1, sample);
                const float wetR = processAllpass(dryR, phaserAPStateR);
                buffer.setSample(1, sample, dryR * (1.0f - phaserMix) + wetR * phaserMix);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    //  FLANGER (short modulated delay with feedback)
    // ═══════════════════════════════════════════════════════════════════
    if (flangerEnable)
    {
        const float flangerMaxMs = 5.0f;
        const float flangerBaseMs = 1.0f;
        const float flangerPhaseInc = juce::MathConstants<float>::twoPi * flangerRate / sr;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            flangerLfoPhase += flangerPhaseInc;
            if (flangerLfoPhase > juce::MathConstants<float>::twoPi)
                flangerLfoPhase -= juce::MathConstants<float>::twoPi;

            const float lfo = 0.5f + 0.5f * std::sin(flangerLfoPhase);
            const float delaySamp = (flangerBaseMs + lfo * flangerDepthParam * flangerMaxMs) * 0.001f * sr;

            // Left
            const float dryL = buffer.getSample(0, sample);
            flangerDelayL.pushSample(0, dryL + flangerFeedbackSampleL * flangerFeedbackParam);
            const float wetL = flangerDelayL.popSample(0, juce::jmax(1.0f, delaySamp));
            flangerFeedbackSampleL = wetL;
            buffer.setSample(0, sample, dryL * (1.0f - flangerMix) + wetL * flangerMix);

            if (totalNumOutputChannels > 1)
            {
                const float dryR = buffer.getSample(1, sample);
                flangerDelayR.pushSample(0, dryR + flangerFeedbackSampleR * flangerFeedbackParam);
                const float wetR = flangerDelayR.popSample(0, juce::jmax(1.0f, delaySamp));
                flangerFeedbackSampleR = wetR;
                buffer.setSample(1, sample, dryR * (1.0f - flangerMix) + wetR * flangerMix);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    //  TREMOLO
    // ═══════════════════════════════════════════════════════════════════
    if (tremoloEnable)
    {
        static constexpr float divisions[] = { 1.0f, 2.0f, 1.5f, 3.0f, 4.0f };
        const int divIndex = juce::jlimit(0, 4, tremoloDivision);
        const float tremoloHz = tremoloSync
            ? static_cast<float>(resolvedTempoBpm / 60.0) * divisions[divIndex]
            : tremoloRate;
        const float tremoloPhaseInc = juce::MathConstants<float>::twoPi * tremoloHz / sr;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            tremoloLfoPhase += tremoloPhaseInc;
            if (tremoloLfoPhase > juce::MathConstants<float>::twoPi)
                tremoloLfoPhase -= juce::MathConstants<float>::twoPi;

            float lfo;
            if (tremoloShape == 1)       // Square
                lfo = std::sin(tremoloLfoPhase) > 0.0f ? 1.0f : 0.0f;
            else if (tremoloShape == 2)  // Triangle
                lfo = std::asin(std::sin(tremoloLfoPhase)) / juce::MathConstants<float>::halfPi * 0.5f + 0.5f;
            else                          // Sine
                lfo = 0.5f + 0.5f * std::sin(tremoloLfoPhase);

            const float gain = 1.0f - tremoloDepthParam * (1.0f - lfo);

            for (int ch = 0; ch < totalNumOutputChannels; ++ch)
                buffer.setSample(ch, sample, buffer.getSample(ch, sample) * gain);
        }
    }

    // Post-FX delay with LFO modulation.
    {
        const float lfoPhaseInc = juce::MathConstants<float>::twoPi * delayModRate
                                  / sr;
        const float baseDelaySamples = sr * effectiveDelayMs * 0.001f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Advance LFO phase (shared across channels)
            delayLfoPhase += lfoPhaseInc;
            if (delayLfoPhase > juce::MathConstants<float>::twoPi)
                delayLfoPhase -= juce::MathConstants<float>::twoPi;

            const float lfoOffset = delayModDepthSamples * std::sin(delayLfoPhase);
            const float modulatedDelay = juce::jmax(1.0f, baseDelaySamples + lfoOffset);
            delayLine.setDelay(modulatedDelay);

            for (int channel = 0; channel < totalNumOutputChannels; ++channel)
            {
                auto* channelData = buffer.getWritePointer(channel);
                const float dry = channelData[sample];
                const float delayed = delayLine.popSample(channel);
                delayLine.pushSample(channel, dry + delayed * delayFeedback);
                channelData[sample] = (dry * (1.0f - delayMix)) + (delayed * delayMix);
            }
        }
    }

    // Reverb pre-delay
    if (reverbPreDelayMs > 0.01f)
    {
        const float preDelaySamples = reverbPreDelayMs * 0.001f * sr;
        reverbPreDelay.setDelay(preDelaySamples);
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                reverbPreDelay.pushSample(channel, channelData[sample]);
                channelData[sample] = reverbPreDelay.popSample(channel);
            }
        }
    }

    // Post-FX reverb.
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = reverbRoomSize;
    reverbParams.damping = reverbDamping;
    reverbParams.wetLevel = reverbMix;
    reverbParams.dryLevel = 1.0f - (reverbMix * 0.5f);
    reverbParams.width = reverbWidth;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    juce::dsp::ProcessContextReplacing<float> reverbContext(block);
    reverb.process(reverbContext);

    // Brickwall output limiter (look-ahead-free peak limiter)
    if (limiterEnabled)
    {
        const float limThresh = juce::Decibels::decibelsToGain(limiterThreshDb);
        const float limRelCoeff = std::exp(-1.0f / (0.08f * sr));
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < totalNumOutputChannels; ++ch)
                peak = juce::jmax(peak, std::abs(buffer.getSample(ch, sample)));

            const float target = peak > limThresh ? (limThresh / peak) : 1.0f;
            limiterEnvelope = (target < limiterEnvelope) ? target : (limiterEnvelope + (1.0f - limiterEnvelope) * (1.0f - limRelCoeff));
            limiterEnvelope = juce::jmin(limiterEnvelope, 1.0f);

            for (int ch = 0; ch < totalNumOutputChannels; ++ch)
                buffer.setSample(ch, sample, buffer.getSample(ch, sample) * limiterEnvelope);
        }
        // Flag clip when limiting is active (envelope < 0.998)
        limiterActive.store(limiterEnvelope < 0.998f);
    }
    else
    {
        limiterActive.store(false);
    }

    const float outputPeak = getBufferPeak(buffer, 0, totalNumOutputChannels);
    outputMeterLevel.store(juce::jmax(outputPeak, outputMeterLevel.load() * 0.92f));

    // ═══════════════════════════════════════════════════════════════════
    //  LOOPER (record / playback / overdub)
    // ═══════════════════════════════════════════════════════════════════
    if (looperState == LooperState::Recording)
    {
        const int maxLen = looperBuffer.getNumSamples();
        for (int s = 0; s < numSamples && looperWritePos < maxLen; ++s)
        {
            for (int ch = 0; ch < totalNumOutputChannels; ++ch)
                looperBuffer.setSample(ch, looperWritePos, buffer.getReadPointer(ch)[s]);
            ++looperWritePos;
        }
        looperLength = looperWritePos;
    }
    else if (looperState == LooperState::Playing && looperLength > 0)
    {
        if (totalNumOutputChannels > 1 && looperBuffer.getNumChannels() > 1)
        {
            auto* outL = buffer.getWritePointer(0);
            auto* outR = buffer.getWritePointer(1);
            const auto* loopL = looperBuffer.getReadPointer(0);
            const auto* loopR = looperBuffer.getReadPointer(1);

            for (int s = 0; s < numSamples; ++s)
            {
                outL[s] += loopL[looperPlayPos] * looperLevelGain;
                outR[s] += loopR[looperPlayPos] * looperLevelGain;
                looperPlayPos = (looperPlayPos + 1) % looperLength;
            }
        }
        else
        {
            auto* out = buffer.getWritePointer(0);
            const auto* loop = looperBuffer.getReadPointer(0);
            for (int s = 0; s < numSamples; ++s)
            {
                out[s] += loop[looperPlayPos] * looperLevelGain;
                looperPlayPos = (looperPlayPos + 1) % looperLength;
            }
        }
    }
    else if (looperState == LooperState::Overdubbing && looperLength > 0)
    {
        constexpr float overdubInputGain = 0.5f;
        constexpr float overdubKeep = 0.995f;

        if (totalNumOutputChannels > 1 && looperBuffer.getNumChannels() > 1)
        {
            auto* outL = buffer.getWritePointer(0);
            auto* outR = buffer.getWritePointer(1);
            auto* loopL = looperBuffer.getWritePointer(0);
            auto* loopR = looperBuffer.getWritePointer(1);

            for (int s = 0; s < numSamples; ++s)
            {
                const float existingL = loopL[looperPlayPos];
                const float incomingL = outL[s];
                loopL[looperPlayPos] = std::tanh(existingL * overdubKeep + incomingL * overdubInputGain);
                outL[s] = std::tanh(incomingL + existingL * looperLevelGain);

                const float existingR = loopR[looperPlayPos];
                const float incomingR = outR[s];
                loopR[looperPlayPos] = std::tanh(existingR * overdubKeep + incomingR * overdubInputGain);
                outR[s] = std::tanh(incomingR + existingR * looperLevelGain);

                looperPlayPos = (looperPlayPos + 1) % looperLength;
            }
        }
        else
        {
            auto* out = buffer.getWritePointer(0);
            auto* loop = looperBuffer.getWritePointer(0);
            for (int s = 0; s < numSamples; ++s)
            {
                const float existing = loop[looperPlayPos];
                const float incoming = out[s];
                loop[looperPlayPos] = std::tanh(existing * overdubKeep + incoming * overdubInputGain);
                out[s] = std::tanh(incoming + existing * looperLevelGain);
                looperPlayPos = (looperPlayPos + 1) % looperLength;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    //  METRONOME (additive click generation)
    // ═══════════════════════════════════════════════════════════════════
    if (metroEnable && (!metroSync || hostIsPlaying))
    {
        float* metroOutL = buffer.getWritePointer(0);
        float* metroOutR = (totalNumOutputChannels > 1) ? buffer.getWritePointer(1) : nullptr;
        static constexpr int beatsPerSig[] = { 4, 3, 6, 5, 7 };
        const int beatsInBar = beatsPerSig[juce::jlimit(0, 4, metroTimeSig)];
        const float metroHz = static_cast<float>((metroSync ? resolvedTempoBpm : metroBpm) / 60.0);
        const float metroPhaseInc = metroHz / sr;
        const float clickDuration = 0.015f; // 15 ms click
        const float clickSamples = clickDuration * sr;

        for (int s = 0; s < numSamples; ++s)
        {
            metroPhase += metroPhaseInc;
            if (metroPhase >= 1.0f)
            {
                metroPhase -= 1.0f;
                metroClickActive = true;
                metroClickPhase = 0.0f;
                metroCurrentClickBeat = metroBeatCount;
                metroBeatCount = (metroBeatCount + 1) % beatsInBar;
                metroClickOscPhase = 0.0f;
            }

            if (metroClickActive)
            {
                const float clickFreq = (metroCurrentClickBeat == 0) ? 1500.0f : 1000.0f;
                const float clickEnv = 1.0f - (metroClickPhase / clickSamples);
                if (clickEnv <= 0.0f)
                {
                    metroClickActive = false;
                }
                else
                {
                    const float clickSample = std::sin(metroClickOscPhase) * clickEnv * metroLevel;
                    metroClickOscPhase += juce::MathConstants<float>::twoPi * clickFreq / sr;
                    metroOutL[s] += clickSample;
                    if (metroOutR != nullptr)
                        metroOutR[s] += clickSample;
                }
                metroClickPhase += 1.0f;
            }
        }
    }

    hostWasPlaying = hostIsPlaying;

    // Stereo correlation coefficient
    if (totalNumOutputChannels >= 2 && buffer.getNumSamples() > 0)
    {
        const float* L = buffer.getReadPointer(0);
        const float* R = buffer.getReadPointer(1);
        float sumLR = 0.0f, sumL2 = 0.0f, sumR2 = 0.0f;
        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            sumLR += L[s] * R[s];
            sumL2 += L[s] * L[s];
            sumR2 += R[s] * R[s];
        }
        const float denom = std::sqrt(sumL2 * sumR2);
        const float corr = (denom > 1e-10f) ? juce::jlimit(-1.0f, 1.0f, sumLR / denom) : 1.0f;
        // Smooth correlation (block-rate lowpass)
        correlationValue.store(correlationValue.load() * 0.85f + corr * 0.15f);
    }
    else
    {
        correlationValue.store(1.0f);
    }

    // Fill spectrum FIFO (channel 0, mix down if stereo)
    if (buffer.getNumChannels() > 0)
    {
        const float* ch0 = buffer.getReadPointer(0);
        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            spectrumFifo[static_cast<size_t>(specFifoFill++)] = ch0[s];
            if (specFifoFill >= kSpecFifoSize)
            {
                specFifoWriteCount.store(1);
                specFifoFill = 0;
            }
        }
    }
}

bool VayuAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VayuAudioProcessor::createEditor()
{
    return new VayuAudioProcessorEditor(*this);
}

void VayuAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    {
        const juce::ScopedLock lock(stateLock);
        state.setProperty("cabIrPath", currentIRFileA.getFullPathName(), nullptr);
        state.setProperty("cabIrPathA", currentIRFileA.getFullPathName(), nullptr);
        state.setProperty("cabIrPathB", currentIRFileB.getFullPathName(), nullptr);
        state.setProperty("bgImagePath", backgroundImagePath, nullptr);
        state.setProperty("namModelPath", namModelPath, nullptr);
    }

    const auto& midiTargets = getMidiLearnTargets();
    for (size_t i = 0; i < midiTargets.size(); ++i)
        state.setProperty(midiTargets[i].stateKey, i < midiCCMap.size() ? midiCCMap[i] : -1, nullptr);

    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void VayuAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
        return;

    auto loadedState = juce::ValueTree::fromXml(*xml);
    apvts.replaceState(loadedState);

    const auto irPathLegacy = loadedState.getProperty("cabIrPath").toString();
    const auto irPathA = loadedState.getProperty("cabIrPathA", irPathLegacy).toString();
    const auto irPathB = loadedState.getProperty("cabIrPathB").toString();
    const auto namPath = loadedState.getProperty("namModelPath").toString();
    setBackgroundImagePath(loadedState.getProperty("bgImagePath").toString());

    const auto& midiTargets = getMidiLearnTargets();
    midiCCMap.assign(midiTargets.size(), -1);
    for (size_t i = 0; i < midiTargets.size(); ++i)
        midiCCMap[i] = static_cast<int>(loadedState.getProperty(midiTargets[i].stateKey, -1));

    if (irPathA.isNotEmpty())
        loadCabinetIRSlot(0, juce::File(irPathA));
    else
        clearCabinetIRSlot(0);

    if (irPathB.isNotEmpty())
        loadCabinetIRSlot(1, juce::File(irPathB));
    else
        clearCabinetIRSlot(1);

    if (namPath.isNotEmpty())
        loadNamModel(namPath);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VayuAudioProcessor();
}
