#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>

namespace
{
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
    midiCCMap.fill(-1);
}

#if JUCE_MSVC
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
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
    static constexpr const char* names[] =
    {
        "Drive", "Output", "Gate", "Boost", "Delay Mix", "Reverb Mix",
        "Cab Blend", "Cab Pan", "Cab A Level", "Cab B Level",
        "Delay Feedback", "Delay Time", "Reverb Room", "Reverb Damping",
        "Bass", "Mid", "Treble", "Presence",
        "Pitch", "Tight Cut", "Wah Freq", "Wah Depth",
        "Kill Depth", "NAM Blend", "Input Trim"
    };

    juce::StringArray items;
    for (int i = 0; i < static_cast<int>(midiCCMap.size()); ++i)
    {
        const auto cc = midiCCMap[static_cast<size_t>(i)];
        if (cc >= 0)
            items.add(juce::String(names[i]) + "=CC" + juce::String(cc));
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
          {"boostDb",0.0f}, {"ampBassDb",3.0f}, {"ampMidDb",1.5f}, {"ampTrebleDb",-1.0f}, {"ampPresenceDb",2.0f},
          {"delayMix",0.12f}, {"delayTimeMs",380.0f}, {"reverbMix",0.18f},
          {"reverbRoomSize",0.4f}, {"reverbDamping",0.5f}, {"reverbWidth",0.9f}, {"reverbPreDelayMs",8.0f},
          {"oversamplingMode",1.0f}, {"ampType",0.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- British Crunch ---
    presets.push_back({ "British Crunch", "Crunch",
        { {"drive",0.55f}, {"outputVolume",-4.5f}, {"gateThreshold",-52.0f},
          {"boostDb",3.0f}, {"ampBassDb",2.0f}, {"ampMidDb",4.0f}, {"ampTrebleDb",2.0f}, {"ampPresenceDb",3.0f},
          {"delayMix",0.08f}, {"delayTimeMs",500.0f}, {"reverbMix",0.1f},
          {"reverbRoomSize",0.35f}, {"reverbDamping",0.4f}, {"reverbWidth",0.8f}, {"reverbPreDelayMs",5.0f},
          {"oversamplingMode",1.0f}, {"ampType",1.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- Modern Metal Lead ---
    presets.push_back({ "Modern Metal Lead", "Lead",
        { {"drive",0.85f}, {"outputVolume",-6.0f}, {"gateThreshold",-42.0f},
          {"gateAttackMs",2.0f}, {"gateReleaseMs",80.0f}, {"gateHysteresisDb",6.0f}, {"gateRangeDb",-70.0f},
          {"boostDb",6.0f}, {"ampBassDb",5.0f}, {"ampMidDb",-2.0f}, {"ampTrebleDb",4.0f}, {"ampPresenceDb",5.0f},
          {"delayMix",0.0f}, {"reverbMix",0.05f},
          {"reverbRoomSize",0.3f}, {"reverbDamping",0.6f}, {"reverbWidth",1.0f}, {"reverbPreDelayMs",0.0f},
          {"oversamplingMode",2.0f}, {"ampType",2.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.5f} } });

    // --- Ambient Pad ---
    presets.push_back({ "Ambient Pad", "Clean",
        { {"drive",0.15f}, {"outputVolume",-5.0f}, {"gateThreshold",-65.0f},
          {"boostDb",0.0f}, {"ampBassDb",1.0f}, {"ampMidDb",-1.0f}, {"ampTrebleDb",0.0f}, {"ampPresenceDb",1.0f},
          {"delayMix",0.35f}, {"delayTimeMs",600.0f}, {"delaySync",1.0f}, {"delayDivision",1.0f},
          {"reverbMix",0.55f}, {"reverbRoomSize",0.9f}, {"reverbDamping",0.2f}, {"reverbWidth",1.0f}, {"reverbPreDelayMs",25.0f},
          {"oversamplingMode",0.0f}, {"ampType",0.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- High Gain Riff ---
    presets.push_back({ "High Gain Riff", "Lead",
        { {"drive",0.78f}, {"outputVolume",-5.5f}, {"gateThreshold",-46.0f},
          {"gateAttackMs",1.5f}, {"gateReleaseMs",100.0f}, {"gateHysteresisDb",8.0f}, {"gateRangeDb",-80.0f},
          {"boostDb",4.5f}, {"ampBassDb",6.0f}, {"ampMidDb",-4.0f}, {"ampTrebleDb",5.0f}, {"ampPresenceDb",4.0f},
          {"delayMix",0.0f}, {"reverbMix",0.0f},
          {"oversamplingMode",2.0f}, {"ampType",2.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- Vintage Jazz ---
    presets.push_back({ "Vintage Jazz", "Clean",
        { {"drive",0.18f}, {"outputVolume",-2.0f}, {"gateThreshold",-58.0f},
          {"boostDb",0.0f}, {"ampBassDb",4.0f}, {"ampMidDb",3.0f}, {"ampTrebleDb",-3.0f}, {"ampPresenceDb",-1.0f},
          {"delayMix",0.0f}, {"reverbMix",0.22f},
          {"reverbRoomSize",0.5f}, {"reverbDamping",0.6f}, {"reverbWidth",0.7f}, {"reverbPreDelayMs",12.0f},
          {"oversamplingMode",0.0f}, {"ampType",0.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

    // --- Country Slap ---
    presets.push_back({ "Country Slap", "Crunch",
        { {"drive",0.42f}, {"outputVolume",-3.5f}, {"gateThreshold",-55.0f},
          {"boostDb",2.0f}, {"ampBassDb",1.0f}, {"ampMidDb",2.0f}, {"ampTrebleDb",3.0f}, {"ampPresenceDb",4.0f},
          {"delayMix",0.18f}, {"delayTimeMs",250.0f}, {"reverbMix",0.12f},
          {"reverbRoomSize",0.35f}, {"reverbDamping",0.5f}, {"reverbWidth",0.85f}, {"reverbPreDelayMs",0.0f},
          {"oversamplingMode",1.0f}, {"ampType",1.0f}, {"limiterEnabled",1.0f}, {"limiterThreshDb",-0.3f} } });

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

    try
    {
        auto loaded = nam::get_dsp(std::filesystem::path(path.toStdString()));
        if (loaded == nullptr)
            throw std::runtime_error("NAM model load returned null DSP");

        if (isPrepared && processSpec.sampleRate > 0.0 && processSpec.maximumBlockSize > 0)
            loaded->Reset(processSpec.sampleRate, static_cast<int>(processSpec.maximumBlockSize));

        std::shared_ptr<nam::DSP> sharedLoaded(std::move(loaded));
        std::atomic_store_explicit(&namEngine, sharedLoaded, std::memory_order_release);

        const juce::ScopedLock lock(stateLock);
        namModelPath = path;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("NAM load failed: " + juce::String(e.what()));
    }
    catch (...)
    {
        juce::Logger::writeToLog("NAM load failed: unknown exception");
    }
#else
    juce::ignoreUnused(path);
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

void VayuAudioProcessor::handleMidiLearnAndMapping(juce::MidiBuffer& midiMessages)
{
    static constexpr const char* parameterIds[] =
    {
        "drive", "outputVolume", "gateThreshold", "boostDb", "delayMix", "reverbMix",
        "cabBlend", "cabPan", "cabLevelA", "cabLevelB",
        "delayFeedback", "delayTimeMs", "reverbRoomSize", "reverbDamping",
        "ampBassDb", "ampMidDb", "ampTrebleDb", "ampPresenceDb",
        "pitchShiftSemi", "tightLowCutHz", "wahCenterHz", "wahDepth",
        "killDepth", "namBlend", "inputTrimDb"
    };

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

            if (auto* param = apvts.getParameter(parameterIds[i]))
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
#endif

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
    const float cabAlignDelaySamples = cabAlignDelayMs * 0.001f * static_cast<float>(getSampleRate());
    const float delayFeedback  = apvts.getRawParameterValue("delayFeedback")->load();
    const float delayModRate   = apvts.getRawParameterValue("delayModRate")->load();
    const float delayModDepth  = apvts.getRawParameterValue("delayModDepth")->load();
    const float delayModDepthSamples = delayModDepth * 0.001f * static_cast<float>(getSampleRate());
    const float attackSamples = juce::jmax(1.0f, gateAttackMs * 0.001f * static_cast<float>(getSampleRate()));
    const float releaseSamples = juce::jmax(1.0f, gateReleaseMs * 0.001f * static_cast<float>(getSampleRate()));
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
    const auto wahCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(getSampleRate(), wahCenter, 0.42f);
    wahFilterL.coefficients = wahCoeffs;
    wahFilterR.coefficients = wahCoeffs;

    irLowCutL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(getSampleRate(), irLowCutHz);
    irLowCutR.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(getSampleRate(), irLowCutHz);
    irHighCutL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), irHighCutHz);
    irHighCutR.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), irHighCutHz);

    float effectiveDelayMs = delayTimeMs;
    if (delaySync)
    {
        double bpm = 120.0;
        if (auto* hostPlayHead = getPlayHead())
        {
            if (auto position = hostPlayHead->getPosition())
                if (auto hostBpm = position->getBpm(); hostBpm.hasValue() && *hostBpm > 0.0)
                    bpm = *hostBpm;
        }

        static constexpr float divisions[] = { 1.0f, 0.5f, 0.75f, 1.0f / 3.0f, 0.25f };
        const int divIndex = juce::jlimit(0, 4, delayDivision);
        effectiveDelayMs = static_cast<float>(60000.0 / bpm) * divisions[divIndex];
    }
    // delaySamples used as base; actual setDelay is per-sample inside the LFO loop

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
                float amped = std::tanh(tightened * drive * ampSaturation) * ampPostEQ;
                if (metalMode)
                {
                    const float hard = juce::jlimit(-0.95f, 0.95f, tightened * drive * 0.88f);
                    amped = amped * 0.62f + hard * 0.38f;
                }
                channelData[sample] = amped * outputGain;
            }
        }
    };

    juce::dsp::AudioBlock<float> block(buffer);
    auto inOutBlock = block.getSubsetChannelBlock(0, static_cast<size_t>(totalNumInputChannels));

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

    // NAM stage (real-time safe): no allocation, no locks, stable shared_ptr snapshot per block.
#if VAYU_HAS_NAM
    if (!namBypass)
    {
        if (auto activeNam = std::atomic_load_explicit(&namEngine, std::memory_order_acquire))
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

                if (namIn >= 2 && namOut >= 2 && totalNumOutputChannels > 1)
                {
                    auto* inL = buffer.getWritePointer(0);
                    auto* inR = buffer.getWritePointer(1);
                    auto* outL = namScratchA.data();
                    auto* outR = namScratchB.data();

                    if (namBlend < 0.999f)
                    {
                        std::memcpy(namDryA.data(), inL, static_cast<size_t>(numSamples) * sizeof(float));
                        std::memcpy(namDryB.data(), inR, static_cast<size_t>(numSamples) * sizeof(float));
                    }

                    float* inPtrs[2] = { inL, inR };
                    float* outPtrs[2] = { outL, outR };
                    activeNam->process(inPtrs, outPtrs, numSamples);

                    if (namBlend < 0.999f)
                    {
                        const float dryMix = 1.0f - namBlend;
                        for (int i = 0; i < numSamples; ++i)
                        {
                            buffer.setSample(0, i, namDryA[static_cast<size_t>(i)] * dryMix + namScratchA[static_cast<size_t>(i)] * namBlend);
                            buffer.setSample(1, i, namDryB[static_cast<size_t>(i)] * dryMix + namScratchB[static_cast<size_t>(i)] * namBlend);
                        }
                    }
                    else
                    {
                        std::memcpy(buffer.getWritePointer(0), namScratchA.data(), static_cast<size_t>(numSamples) * sizeof(float));
                        std::memcpy(buffer.getWritePointer(1), namScratchB.data(), static_cast<size_t>(numSamples) * sizeof(float));
                    }
                }
                else
                {
                    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
                    {
                        auto* in = buffer.getWritePointer(ch);
                        auto* out = namScratchA.data();
                        if (namBlend < 0.999f)
                            std::memcpy(namDryA.data(), in, static_cast<size_t>(numSamples) * sizeof(float));

                        float* inPtrs[1] = { in };
                        float* outPtrs[1] = { out };
                        activeNam->process(inPtrs, outPtrs, numSamples);

                        if (namBlend < 0.999f)
                        {
                            const float dryMix = 1.0f - namBlend;
                            for (int i = 0; i < numSamples; ++i)
                                buffer.setSample(ch, i, namDryA[static_cast<size_t>(i)] * dryMix + namScratchA[static_cast<size_t>(i)] * namBlend);
                        }
                        else
                        {
                            std::memcpy(buffer.getWritePointer(ch), namScratchA.data(), static_cast<size_t>(numSamples) * sizeof(float));
                        }
                    }
                }

                if (applyNamMatch)
                {
                    const float postNamRms = measureStereoRms();
                    constexpr float smooth = 0.96f;
                    namMatchInRms = (smooth * namMatchInRms) + ((1.0f - smooth) * preNamRms);
                    namMatchOutRms = (smooth * namMatchOutRms) + ((1.0f - smooth) * postNamRms);

                    if (namMatchOutRms > 1.0e-6f && namMatchInRms > 1.0e-6f)
                    {
                        const float matchGain = juce::jlimit(0.5f, 2.0f, namMatchInRms / namMatchOutRms);
                        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
                            buffer.applyGain(ch, 0, numSamples, matchGain);
                        namMatchGainDb.store(juce::Decibels::gainToDecibels(matchGain));
                    }
                }
            }
        }
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

    if (killEnable)
    {
        constexpr float killDivisions[] = { 1.0f, 2.0f, 4.0f, 8.0f };
        const int rateIndex = juce::jlimit(0, 3, killRate);
        double bpm = 120.0;
        if (auto* hostPlayHead = getPlayHead())
        {
            if (auto position = hostPlayHead->getPosition())
            {
                if (auto hostBpm = position->getBpm(); hostBpm.hasValue() && *hostBpm > 0.0)
                    bpm = *hostBpm;
            }
        }

        const float hz = static_cast<float>(bpm / 60.0) * killDivisions[rateIndex];
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

    // Post-FX delay with LFO modulation.
    {
        const float lfoPhaseInc = juce::MathConstants<float>::twoPi * delayModRate
                                  / static_cast<float>(getSampleRate());
        const float baseDelaySamples = static_cast<float>(getSampleRate() * effectiveDelayMs * 0.001);

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
        const float preDelaySamples = reverbPreDelayMs * 0.001f * static_cast<float>(getSampleRate());
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
        const float limRelCoeff = std::exp(-1.0f / (0.08f * static_cast<float>(getSampleRate())));
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

    state.setProperty("cc_drive", midiCCMap[0], nullptr);
    state.setProperty("cc_output", midiCCMap[1], nullptr);
    state.setProperty("cc_gate", midiCCMap[2], nullptr);
    state.setProperty("cc_boost", midiCCMap[3], nullptr);
    state.setProperty("cc_delayMix", midiCCMap[4], nullptr);
    state.setProperty("cc_reverbMix", midiCCMap[5], nullptr);
    state.setProperty("cc_cabBlend", midiCCMap[6], nullptr);
    state.setProperty("cc_cabPan", midiCCMap[7], nullptr);
    state.setProperty("cc_cabLevelA", midiCCMap[8], nullptr);
    state.setProperty("cc_cabLevelB", midiCCMap[9], nullptr);
    state.setProperty("cc_delayFeedback", midiCCMap[10], nullptr);
    state.setProperty("cc_delayTimeMs", midiCCMap[11], nullptr);
    state.setProperty("cc_reverbRoomSize", midiCCMap[12], nullptr);
    state.setProperty("cc_reverbDamping", midiCCMap[13], nullptr);
    state.setProperty("cc_bass", midiCCMap[14], nullptr);
    state.setProperty("cc_mid", midiCCMap[15], nullptr);
    state.setProperty("cc_treble", midiCCMap[16], nullptr);
    state.setProperty("cc_presence", midiCCMap[17], nullptr);
    state.setProperty("cc_pitch", midiCCMap[18], nullptr);
    state.setProperty("cc_tight", midiCCMap[19], nullptr);
    state.setProperty("cc_wahFreq", midiCCMap[20], nullptr);
    state.setProperty("cc_wahDepth", midiCCMap[21], nullptr);
    state.setProperty("cc_killDepth", midiCCMap[22], nullptr);
    state.setProperty("cc_namBlend", midiCCMap[23], nullptr);
    state.setProperty("cc_inputTrim", midiCCMap[24], nullptr);

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

    midiCCMap[0] = static_cast<int>(loadedState.getProperty("cc_drive", -1));
    midiCCMap[1] = static_cast<int>(loadedState.getProperty("cc_output", -1));
    midiCCMap[2] = static_cast<int>(loadedState.getProperty("cc_gate", -1));
    midiCCMap[3] = static_cast<int>(loadedState.getProperty("cc_boost", -1));
    midiCCMap[4] = static_cast<int>(loadedState.getProperty("cc_delayMix", -1));
    midiCCMap[5] = static_cast<int>(loadedState.getProperty("cc_reverbMix", -1));
    midiCCMap[6] = static_cast<int>(loadedState.getProperty("cc_cabBlend", -1));
    midiCCMap[7]  = static_cast<int>(loadedState.getProperty("cc_cabPan", -1));
    midiCCMap[8]  = static_cast<int>(loadedState.getProperty("cc_cabLevelA", -1));
    midiCCMap[9]  = static_cast<int>(loadedState.getProperty("cc_cabLevelB", -1));
    midiCCMap[10] = static_cast<int>(loadedState.getProperty("cc_delayFeedback", -1));
    midiCCMap[11] = static_cast<int>(loadedState.getProperty("cc_delayTimeMs", -1));
    midiCCMap[12] = static_cast<int>(loadedState.getProperty("cc_reverbRoomSize", -1));
    midiCCMap[13] = static_cast<int>(loadedState.getProperty("cc_reverbDamping", -1));
    midiCCMap[14] = static_cast<int>(loadedState.getProperty("cc_bass", -1));
    midiCCMap[15] = static_cast<int>(loadedState.getProperty("cc_mid", -1));
    midiCCMap[16] = static_cast<int>(loadedState.getProperty("cc_treble", -1));
    midiCCMap[17] = static_cast<int>(loadedState.getProperty("cc_presence", -1));
    midiCCMap[18] = static_cast<int>(loadedState.getProperty("cc_pitch", -1));
    midiCCMap[19] = static_cast<int>(loadedState.getProperty("cc_tight", -1));
    midiCCMap[20] = static_cast<int>(loadedState.getProperty("cc_wahFreq", -1));
    midiCCMap[21] = static_cast<int>(loadedState.getProperty("cc_wahDepth", -1));
    midiCCMap[22] = static_cast<int>(loadedState.getProperty("cc_killDepth", -1));
    midiCCMap[23] = static_cast<int>(loadedState.getProperty("cc_namBlend", -1));
    midiCCMap[24] = static_cast<int>(loadedState.getProperty("cc_inputTrim", -1));

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
