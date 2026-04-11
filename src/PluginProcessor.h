#pragma once

#if __has_include(<JuceHeader.h>)
#include <JuceHeader.h>
#elif __has_include("../build-local/Vayu_artefacts/JuceLibraryCode/JuceHeader.h")
#include "../build-local/Vayu_artefacts/JuceLibraryCode/JuceHeader.h"
#else
#error Could not locate JuceHeader.h
#endif

#if __has_include("NAM/get_dsp.h")
#include "NAM/get_dsp.h"
#define VAYU_HAS_NAM 1
#elif __has_include("nam/nam.hpp")
#include "nam/nam.hpp"
#define VAYU_HAS_NAM 0
#else
#define VAYU_HAS_NAM 0
namespace nam { class DSP; }
#endif

class VayuAudioProcessor : public juce::AudioProcessor
{
public:
    VayuAudioProcessor();
    ~VayuAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_IsMidiEffect
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool loadCabinetIR(const juce::File& irFile);
    void clearCabinetIR();
    juce::String getCurrentIRName() const;
    bool loadCabinetIRSlot(int slotIndex, const juce::File& irFile);
    void clearCabinetIRSlot(int slotIndex);
    juce::String getCurrentIRNameForSlot(int slotIndex) const;
    bool autoAlignCabPolarity();
    bool canAutoAlignCab() const;
    float getLastCabAlignCorrelation() const;
    float getCabAlignDelayMs() const;
    void setBackgroundImagePath(const juce::String& path);
    juce::String getBackgroundImagePath() const;

    float getTunerFrequencyHz() const;
    float getTunerCents() const;
    juce::String getTunerNoteName() const;
    float getInputMeterLevel() const;
    float getOutputMeterLevel() const;

    bool canUndo() const;
    bool canRedo() const;
    void undoLastChange();
    void redoLastChange();

    void beginMidiLearnForParam(int paramIndex);
    juce::String getMidiMappingDescription() const;

    struct FactoryPreset
    {
        juce::String name;
        juce::String category;
        std::map<juce::String, float> params;
    };

    static std::vector<FactoryPreset> getFactoryPresets();
    void loadFactoryPreset(int index);
    void loadNamModel(const juce::String& path);
    void clearNamModel();
    juce::String getNamModelPath() const;
    float getNamMatchGainDb() const;

    // Spectrum FIFO — accessed from editor timer thread
    static constexpr int kSpecFifoSize = 512;
    bool consumeSpectrumBlock(float* dest) noexcept
    {
        if (specFifoWriteCount.load() == 0) return false;
        std::copy(spectrumFifo.begin(), spectrumFifo.end(), dest);
        specFifoWriteCount.store(0);
        return true;
    }

    std::atomic<bool> limiterActive { false };
    std::atomic<float> correlationValue { 1.0f };

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void loadDefaultCabinetIR(int slotIndex);
    void applyCabinetSlotLoadIfPrepared(int slotIndex);
    void updateTuner(const juce::AudioBuffer<float>& buffer);
    void handleMidiLearnAndMapping(juce::MidiBuffer& midiMessages);

    juce::dsp::Convolution cabinetConvolutionA;
    juce::dsp::Convolution cabinetConvolutionB;
    juce::dsp::IIR::Filter<float> irLowCutL;
    juce::dsp::IIR::Filter<float> irLowCutR;
    juce::dsp::IIR::Filter<float> irHighCutL;
    juce::dsp::IIR::Filter<float> irHighCutR;
    juce::dsp::IIR::Filter<float> ampBassL;
    juce::dsp::IIR::Filter<float> ampBassR;
    juce::dsp::IIR::Filter<float> ampMidL;
    juce::dsp::IIR::Filter<float> ampMidR;
    juce::dsp::IIR::Filter<float> ampTrebleL;
    juce::dsp::IIR::Filter<float> ampTrebleR;
    juce::dsp::IIR::Filter<float> ampPresenceL;
    juce::dsp::IIR::Filter<float> ampPresenceR;
    juce::dsp::IIR::Filter<float> tightFilterL;
    juce::dsp::IIR::Filter<float> tightFilterR;
    juce::dsp::IIR::Filter<float> wahFilterL;
    juce::dsp::IIR::Filter<float> wahFilterR;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 192000 };
    juce::dsp::Reverb reverb;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling2x;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling4x;
    juce::dsp::ProcessSpec processSpec;

    mutable juce::CriticalSection stateLock;
    juce::File currentIRFileA;
    juce::File currentIRFileB;
    juce::String backgroundImagePath;
    bool isPrepared = false;

    std::atomic<float> tunerFrequencyHz { 0.0f };
    std::atomic<float> tunerCents { 0.0f };
    std::atomic<int> tunerNoteIndex { -1 };
    std::atomic<float> inputMeterLevel { 0.0f };
    std::atomic<float> outputMeterLevel { 0.0f };

    static constexpr int kTunerBufSize = 4096;
    std::array<float, kTunerBufSize> tunerBuffer {};
    int tunerFillCount = 0;

    float gateEnvelope = 0.0f;
    float gateGain = 1.0f;
    bool gateIsOpen = true;
    float delayLfoPhase = 0.0f;
    float killLfoPhase = 0.0f;
    float wahLfoPhase = 0.0f;
    int processingChannels = 2;

    juce::UndoManager undoManager;
    int learningParamIndex = -1;
    std::array<int, 25> midiCCMap;  // init in constructor

    juce::AudioBuffer<float> cabBufferA;
    juce::AudioBuffer<float> cabBufferB;
    juce::AudioBuffer<float> pitchBuffer;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> reverbPreDelay { 22050 };
    float limiterEnvelope = 0.0f;

    std::array<float, kSpecFifoSize> spectrumFifo {};
    std::atomic<int> specFifoWriteCount { 0 };
    int specFifoFill = 0;

    // NAM engine shared with audio thread via atomic shared_ptr ops.
    std::shared_ptr<nam::DSP> namEngine;
    std::vector<float> namScratchA;
    std::vector<float> namScratchB;
    std::vector<float> namDryA;
    std::vector<float> namDryB;
    juce::String namModelPath;
    float namMatchInRms = 0.0f;
    float namMatchOutRms = 0.0f;
    std::atomic<float> namMatchGainDb { 0.0f };
    std::atomic<float> cabAlignCorrelation { 0.0f };
    std::atomic<float> cabAlignDelayMsTelemetry { 0.0f };
};
