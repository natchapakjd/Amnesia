#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <limits>

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

MyAmpSimAudioProcessor::MyAmpSimAudioProcessor()
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
}

juce::AudioProcessorValueTreeState::ParameterLayout MyAmpSimAudioProcessor::createParameterLayout()
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

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbMix", 1 },
        "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 0.75f, 0.001f, 1.0f),
        0.12f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value * 100.0f, 0) + " %"; },
        [](const juce::String& text) { return text.trimCharactersAtEnd(" %").getFloatValue() / 100.0f; }));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "oversamplingMode", 1 },
        "Oversampling",
        juce::StringArray { "Off", "2x", "4x" },
        0));

    return layout;
}

MyAmpSimAudioProcessor::~MyAmpSimAudioProcessor() = default;

const juce::String MyAmpSimAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MyAmpSimAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MyAmpSimAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MyAmpSimAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MyAmpSimAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MyAmpSimAudioProcessor::getNumPrograms()
{
    return 1;
}

int MyAmpSimAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MyAmpSimAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String MyAmpSimAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void MyAmpSimAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

bool MyAmpSimAudioProcessor::loadCabinetIR(const juce::File& irFile)
{
    if (!irFile.existsAsFile())
        return false;

    {
        const juce::ScopedLock lock(stateLock);
        currentIRFile = irFile;
    }

    if (isPrepared)
    {
        cabinetConvolution.loadImpulseResponse(irFile,
                                               juce::dsp::Convolution::Stereo::yes,
                                               juce::dsp::Convolution::Trim::yes,
                                               0,
                                               juce::dsp::Convolution::Normalise::yes);
    }

    return true;
}

void MyAmpSimAudioProcessor::clearCabinetIR()
{
    {
        const juce::ScopedLock lock(stateLock);
        currentIRFile = juce::File();
    }

    if (isPrepared)
        loadDefaultCabinetIR();
}

juce::String MyAmpSimAudioProcessor::getCurrentIRName() const
{
    const juce::ScopedLock lock(stateLock);

    if (currentIRFile.existsAsFile())
        return currentIRFile.getFileName();

    return "Default (No Cabinet)";
}

void MyAmpSimAudioProcessor::setBackgroundImagePath(const juce::String& path)
{
    const juce::ScopedLock lock(stateLock);
    backgroundImagePath = path;
}

juce::String MyAmpSimAudioProcessor::getBackgroundImagePath() const
{
    const juce::ScopedLock lock(stateLock);
    return backgroundImagePath;
}

float MyAmpSimAudioProcessor::getTunerFrequencyHz() const
{
    return tunerFrequencyHz.load();
}

float MyAmpSimAudioProcessor::getTunerCents() const
{
    return tunerCents.load();
}

juce::String MyAmpSimAudioProcessor::getTunerNoteName() const
{
    const auto idx = tunerNoteIndex.load();

    if (idx < 0 || idx > 11)
        return "--";

    return kNoteNames[idx];
}

float MyAmpSimAudioProcessor::getInputMeterLevel() const
{
    return inputMeterLevel.load();
}

float MyAmpSimAudioProcessor::getOutputMeterLevel() const
{
    return outputMeterLevel.load();
}

bool MyAmpSimAudioProcessor::canUndo() const
{
    return undoManager.canUndo();
}

bool MyAmpSimAudioProcessor::canRedo() const
{
    return undoManager.canRedo();
}

void MyAmpSimAudioProcessor::undoLastChange()
{
    if (undoManager.canUndo())
        undoManager.undo();
}

void MyAmpSimAudioProcessor::redoLastChange()
{
    if (undoManager.canRedo())
        undoManager.redo();
}

void MyAmpSimAudioProcessor::beginMidiLearnForParam(int paramIndex)
{
    if (paramIndex >= 0 && paramIndex < static_cast<int>(midiCCMap.size()))
        learningParamIndex = paramIndex;
}

juce::String MyAmpSimAudioProcessor::getMidiMappingDescription() const
{
    static constexpr const char* names[] = { "Drive", "Output", "Gate", "Boost", "Delay Mix", "Reverb Mix" };

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

void MyAmpSimAudioProcessor::handleMidiLearnAndMapping(juce::MidiBuffer& midiMessages)
{
    static constexpr const char* parameterIds[] = { "drive", "outputVolume", "gateThreshold", "boostDb", "delayMix", "reverbMix" };

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

void MyAmpSimAudioProcessor::loadDefaultCabinetIR()
{
    juce::AudioBuffer<float> identityIR(1, 1);
    identityIR.clear();
    identityIR.setSample(0, 0, 1.0f);

    cabinetConvolution.loadImpulseResponse(std::move(identityIR),
                                           processSpec.sampleRate > 0.0 ? processSpec.sampleRate : 44100.0,
                                           juce::dsp::Convolution::Stereo::no,
                                           juce::dsp::Convolution::Trim::no,
                                           juce::dsp::Convolution::Normalise::no);
}

void MyAmpSimAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    processSpec.sampleRate = sampleRate;
    processSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    processSpec.numChannels = static_cast<juce::uint32>(juce::jmax(1, getMainBusNumOutputChannels()));
    processingChannels = static_cast<int>(processSpec.numChannels);

    cabinetConvolution.reset();
    cabinetConvolution.prepare(processSpec);

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

    gateEnvelope = 0.0f;
    gateGain = 1.0f;

    isPrepared = true;

    const juce::ScopedLock lock(stateLock);
    if (currentIRFile.existsAsFile())
    {
        cabinetConvolution.loadImpulseResponse(currentIRFile,
                                               juce::dsp::Convolution::Stereo::yes,
                                               juce::dsp::Convolution::Trim::yes,
                                               0,
                                               juce::dsp::Convolution::Normalise::yes);
    }
    else
    {
        loadDefaultCabinetIR();
    }
}

void MyAmpSimAudioProcessor::releaseResources()
{
    isPrepared = false;
    inputMeterLevel.store(0.0f);
    outputMeterLevel.store(0.0f);
    gateEnvelope = 0.0f;
    gateGain = 1.0f;
    oversampling2x.reset();
    oversampling4x.reset();
}

void MyAmpSimAudioProcessor::updateTuner(const juce::AudioBuffer<float>& buffer)
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
bool MyAmpSimAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void MyAmpSimAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    // Read parameters once per block — lock-free atomic loads
    const float drive    = apvts.getRawParameterValue("drive")->load();
    const float outputDb = apvts.getRawParameterValue("outputVolume")->load();
    const float gateThresholdDb = apvts.getRawParameterValue("gateThreshold")->load();
    const float boostDb = apvts.getRawParameterValue("boostDb")->load();
    const float delayTimeMs = apvts.getRawParameterValue("delayTimeMs")->load();
    const float delayMix = apvts.getRawParameterValue("delayMix")->load();
    const float reverbMix = apvts.getRawParameterValue("reverbMix")->load();
    const int oversamplingMode = static_cast<int>(apvts.getRawParameterValue("oversamplingMode")->load());

    const float outputGain = juce::Decibels::decibelsToGain(outputDb);
    const float gateThresholdLinear = juce::Decibels::decibelsToGain(gateThresholdDb);
    const float boostGain = juce::Decibels::decibelsToGain(boostDb);
    const float delayFeedback = 0.35f;

    const float delaySamples = static_cast<float>(getSampleRate() * delayTimeMs * 0.001);
    delayLine.setDelay(delaySamples);

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

                gateEnvelope = juce::jmax(std::abs(in), gateEnvelope * 0.995f);
                const float targetGate = gateEnvelope >= gateThresholdLinear ? 1.0f : 0.0f;
                gateGain += (targetGate - gateGain) * 0.02f;

                const float gated = in * gateGain;
                const float boosted = gated * boostGain;
                channelData[sample] = std::tanh(boosted * drive) * outputGain;
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

    // Copy processed input channels into any extra output channels.
    for (int channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.copyFrom(channel, 0, buffer, channel % totalNumInputChannels, 0, numSamples);

    // Cabinet simulation: convolve amp output with the loaded speaker IR.
    juce::dsp::ProcessContextReplacing<float> context(block);
    cabinetConvolution.process(context);

    // Post-FX delay (parallel blend).
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float dry = channelData[sample];
            const float delayed = delayLine.popSample(channel);
            delayLine.pushSample(channel, dry + delayed * delayFeedback);
            channelData[sample] = (dry * (1.0f - delayMix)) + (delayed * delayMix);
        }
    }

    // Post-FX reverb.
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.58f;
    reverbParams.damping = 0.4f;
    reverbParams.wetLevel = reverbMix;
    reverbParams.dryLevel = 1.0f - (reverbMix * 0.5f);
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    reverb.process(context);

    const float outputPeak = getBufferPeak(buffer, 0, totalNumOutputChannels);
    outputMeterLevel.store(juce::jmax(outputPeak, outputMeterLevel.load() * 0.92f));
}

bool MyAmpSimAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MyAmpSimAudioProcessor::createEditor()
{
    return new MyAmpSimAudioProcessorEditor(*this);
}

void MyAmpSimAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    {
        const juce::ScopedLock lock(stateLock);
        state.setProperty("cabIrPath", currentIRFile.getFullPathName(), nullptr);
        state.setProperty("bgImagePath", backgroundImagePath, nullptr);
    }

    state.setProperty("cc_drive", midiCCMap[0], nullptr);
    state.setProperty("cc_output", midiCCMap[1], nullptr);
    state.setProperty("cc_gate", midiCCMap[2], nullptr);
    state.setProperty("cc_boost", midiCCMap[3], nullptr);
    state.setProperty("cc_delayMix", midiCCMap[4], nullptr);
    state.setProperty("cc_reverbMix", midiCCMap[5], nullptr);

    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void MyAmpSimAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
        return;

    auto loadedState = juce::ValueTree::fromXml(*xml);
    apvts.replaceState(loadedState);

    const auto irPath = loadedState.getProperty("cabIrPath").toString();
    setBackgroundImagePath(loadedState.getProperty("bgImagePath").toString());

    midiCCMap[0] = static_cast<int>(loadedState.getProperty("cc_drive", -1));
    midiCCMap[1] = static_cast<int>(loadedState.getProperty("cc_output", -1));
    midiCCMap[2] = static_cast<int>(loadedState.getProperty("cc_gate", -1));
    midiCCMap[3] = static_cast<int>(loadedState.getProperty("cc_boost", -1));
    midiCCMap[4] = static_cast<int>(loadedState.getProperty("cc_delayMix", -1));
    midiCCMap[5] = static_cast<int>(loadedState.getProperty("cc_reverbMix", -1));

    if (irPath.isNotEmpty())
        loadCabinetIR(juce::File(irPath));
    else
        clearCabinetIR();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyAmpSimAudioProcessor();
}
