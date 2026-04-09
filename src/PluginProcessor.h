#pragma once

#include <JuceHeader.h>

class MyAmpSimAudioProcessor : public juce::AudioProcessor
{
public:
    MyAmpSimAudioProcessor();
    ~MyAmpSimAudioProcessor() override;

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

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void loadDefaultCabinetIR();
    void updateTuner(const juce::AudioBuffer<float>& buffer);
    void handleMidiLearnAndMapping(juce::MidiBuffer& midiMessages);

    juce::dsp::Convolution cabinetConvolution;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 192000 };
    juce::dsp::Reverb reverb;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling2x;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling4x;
    juce::dsp::ProcessSpec processSpec;

    mutable juce::CriticalSection stateLock;
    juce::File currentIRFile;
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
    int processingChannels = 2;

    juce::UndoManager undoManager;
    int learningParamIndex = -1;
    std::array<int, 6> midiCCMap { -1, -1, -1, -1, -1, -1 };
};
