#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MyAmpSimAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit MyAmpSimAudioProcessorEditor(MyAmpSimAudioProcessor&);
    ~MyAmpSimAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    class AmpLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics& g,
                              int x,
                              int y,
                              int width,
                              int height,
                              float sliderPosProportional,
                              float rotaryStartAngle,
                              float rotaryEndAngle,
                              juce::Slider& slider) override;
    };

    enum class UiTab { amp = 0, fx, tools };
    enum class UiScale { small = 0, medium, large };
    void updateTabVisibility();
    void switchToTab(UiTab tab);
    void applyEditorSizePreset(UiScale preset);

    // Inline spectrum analyser component
    class SpectrumAnalyzer : public juce::Component, public juce::Timer
    {
    public:
        explicit SpectrumAnalyzer(MyAmpSimAudioProcessor& p)
            : processor(p), fft(fftOrder)
        {
            startTimerHz(30);
        }
        ~SpectrumAnalyzer() override { stopTimer(); }

        void pushSamples(const float* data, int numSamples)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                fifo[fifoIndex++] = data[i];
                if (fifoIndex == fftSize)
                {
                    std::copy(fifo.begin(), fifo.end(), fftData.begin());
                    juce::dsp::WindowingFunction<float>::fillWindowingTables(
                        window.data(), static_cast<size_t>(fftSize),
                        juce::dsp::WindowingFunction<float>::hann);
                    for (int k = 0; k < fftSize; ++k) fftData[static_cast<size_t>(k)] *= window[static_cast<size_t>(k)];
                    fft.performFrequencyOnlyForwardTransform(fftData.data());
                    newDataReady.store(true);
                    fifoIndex = 0;
                }
            }
        }

        void paint(juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();
            g.fillAll(juce::Colour(0xff0d0d0d));
            g.setColour(juce::Colour(0xff333333));
            g.drawRect(bounds, 1.0f);

            const int numBins = fftSize / 2;
            const float binWidth = bounds.getWidth() / static_cast<float>(numBins);

            for (int i = 1; i < numBins; ++i)
            {
                const float level = juce::jmap(
                    juce::Decibels::gainToDecibels(smoothed[static_cast<size_t>(i)], -100.0f),
                    -100.0f, 0.0f, 0.0f, 1.0f);
                const float x = static_cast<float>(i) * binWidth;
                const float barH = level * bounds.getHeight();
                const float hue = 0.58f - level * 0.35f;
                g.setColour(juce::Colour::fromHSV(hue, 0.8f, 0.85f, 0.9f));
                g.fillRect(x, bounds.getBottom() - barH, binWidth * 0.9f, barH);
            }
        }

        void timerCallback() override
        {
            if (newDataReady.exchange(false))
            {
                for (int i = 0; i < fftSize / 2; ++i)
                {
                    const float v = fftData[static_cast<size_t>(i)] / static_cast<float>(fftSize);
                    smoothed[static_cast<size_t>(i)] = smoothed[static_cast<size_t>(i)] * 0.85f + v * 0.15f;
                }
                repaint();
            }
        }

    private:
        MyAmpSimAudioProcessor& processor;
        static constexpr int fftOrder = 9;
        static constexpr int fftSize  = 1 << fftOrder;   // 512
        juce::dsp::FFT fft;
        std::array<float, fftSize> fifo {};
        std::array<float, fftSize * 2> fftData {};
        std::array<float, fftSize> window {};
        std::array<float, fftSize / 2> smoothed {};
        int fifoIndex = 0;
        std::atomic<bool> newDataReady { false };
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
    };

    void openAudioSettings();
    void chooseCabinetIR();
    void chooseCabinetIRSlot(int slotIndex);
    void refreshIrStatus();
    void refreshTunerStatus();
    void refreshToolsStatus();
    void savePresetToFile();
    void loadPresetFromFile();
    void captureSnapshotA();
    void captureSnapshotB();
    void recallSnapshotAorB();
    void loadBackgroundImageFromFile(const juce::File& file);
    void chooseBackgroundImage();
    void clearBackgroundImage();
    void drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float linearLevel, const juce::String& label) const;

    MyAmpSimAudioProcessor& audioProcessor;
    juce::TextButton ampTabButton { "Amp" };
    juce::TextButton fxTabButton { "FX" };
    juce::TextButton toolsTabButton { "Tools" };
    juce::Label uiSizeLabel { {}, "Size" };
    juce::ComboBox uiSizeCombo;
    juce::TextButton audioSettingsButton { "Audio Settings" };
    juce::TextButton loadIrButton { "Load IR" };
    juce::TextButton clearIrButton { "Clear IR" };
    juce::TextButton loadIrAButton { "Load A" };
    juce::TextButton clearIrAButton { "Clear A" };
    juce::TextButton loadIrBButton { "Load B" };
    juce::TextButton clearIrBButton { "Clear B" };
    juce::TextButton savePresetButton { "Save Preset" };
    juce::TextButton loadPresetButton { "Load Preset" };
    juce::TextButton captureAButton { "Capture A" };
    juce::TextButton captureBButton { "Capture B" };
    juce::TextButton loadBackgroundButton { "Load Background" };
    juce::TextButton clearBackgroundButton { "Clear Background" };
    juce::ToggleButton compareABToggle { "Compare B" };
    juce::ToggleButton tunerToggle { "Show Tuner" };
    juce::ToggleButton irPhaseToggle { "IR Phase" };
    juce::ToggleButton delaySyncToggle { "Delay Sync" };
    juce::ToggleButton cabFlipAButton { "Flip A" };
    juce::ToggleButton cabFlipBButton { "Flip B" };
    juce::TextButton undoButton { "Undo" };
    juce::TextButton redoButton { "Redo" };
    juce::ComboBox ampTypeCombo;
    juce::Label ampTypeLabel { {}, "Amp Type" };
    juce::ComboBox oversamplingCombo;
    juce::Label oversamplingLabel { {}, "Oversampling" };
    juce::ComboBox delayDivisionCombo;
    juce::Label delayDivisionLabel { {}, "Division" };
    juce::ComboBox midiParamCombo;
    juce::TextButton midiLearnButton { "MIDI Learn" };
    juce::Label irStatusLabel;
    juce::Label irStatusALabel;
    juce::Label irStatusBLabel;
    juce::Label tunerNoteLabel;
    juce::Label tunerDetailLabel;
    juce::Label midiMapStatusLabel;
    juce::Label backgroundStatusLabel;
    juce::Rectangle<int> inputMeterBounds;
    juce::Rectangle<int> outputMeterBounds;

    juce::Slider driveSlider;
    juce::Slider volumeSlider;
    juce::Slider gateSlider;
    juce::Slider gateAttackSlider;
    juce::Slider gateReleaseSlider;
    juce::Slider gateHysteresisSlider;
    juce::Slider gateRangeSlider;
    juce::Slider boostSlider;
    juce::Slider bassSlider;
    juce::Slider midSlider;
    juce::Slider trebleSlider;
    juce::Slider presenceSlider;
    juce::Slider delayTimeSlider;
    juce::Slider delayMixSlider;
    juce::Slider reverbMixSlider;
    juce::Slider cabBlendSlider;
    juce::Slider cabPanSlider;
    juce::Slider cabLevelASlider;
    juce::Slider cabLevelBSlider;
    juce::Slider irLowCutSlider;
    juce::Slider irHighCutSlider;
    juce::Slider irLevelSlider;
    juce::Label  driveLabel  { {}, "Drive" };
    juce::Label  volumeLabel { {}, "Volume" };
    juce::Label  gateLabel { {}, "Gate" };
    juce::Label  gateAttackLabel { {}, "Gate Attack" };
    juce::Label  gateReleaseLabel { {}, "Gate Release" };
    juce::Label  gateHysteresisLabel { {}, "Gate Hyst" };
    juce::Label  gateRangeLabel { {}, "Gate Range" };
    juce::Label  boostLabel { {}, "Boost" };
    juce::Label  bassLabel { {}, "Bass" };
    juce::Label  midLabel { {}, "Mid" };
    juce::Label  trebleLabel { {}, "Treble" };
    juce::Label  presenceLabel { {}, "Presence" };
    juce::Label  delayTimeLabel { {}, "Delay Time" };
    juce::Label  delayMixLabel { {}, "Delay Mix" };
    juce::Label  reverbMixLabel { {}, "Reverb Mix" };
    juce::Label  cabBlendLabel { {}, "Cab Blend" };
    juce::Label  cabPanLabel { {}, "Cab Pan" };
    juce::Label  cabLevelALabel { {}, "Cab A Level" };
    juce::Label  cabLevelBLabel { {}, "Cab B Level" };
    juce::Label  irLowCutLabel { {}, "IR Low Cut" };
    juce::Label  irHighCutLabel { {}, "IR High Cut" };
    juce::Label  irLevelLabel { {}, "IR Level" };

    std::unique_ptr<juce::FileChooser> irChooser;
    std::unique_ptr<juce::FileChooser> presetChooser;
    std::unique_ptr<juce::FileChooser> backgroundChooser;
    juce::MemoryBlock snapshotA;
    juce::MemoryBlock snapshotB;
    juce::Image backgroundImage;
    AmpLookAndFeel ampLookAndFeel;
    UiTab currentTab = UiTab::amp;
    UiScale currentScale = UiScale::medium;

    // Reverb advanced controls
    juce::Slider reverbRoomSizeSlider;
    juce::Slider reverbDampingSlider;
    juce::Slider reverbWidthSlider;
    juce::Slider reverbPreDelaySlider;
    juce::Label  reverbRoomSizeLabel { {}, "Room" };
    juce::Label  reverbDampingLabel  { {}, "Damping" };
    juce::Label  reverbWidthLabel    { {}, "Width" };
    juce::Label  reverbPreDelayLabel { {}, "Pre-Delay" };

    // Input trim + output limiter controls
    juce::Slider inputTrimSlider;
    juce::Slider limiterThreshSlider;
    juce::ToggleButton limiterEnabledToggle { "Limiter" };
    juce::Label  inputTrimLabel    { {}, "Input Trim" };
    juce::Label  limiterThreshLabel { {}, "Lim Thresh" };
    juce::Label  limiterClipLabel;

    // Delay extras
    juce::Slider delayFeedbackSlider;
    juce::Slider delayModRateSlider;
    juce::Slider delayModDepthSlider;
    juce::Label  delayFeedbackLabel { {}, "Feedback" };
    juce::Label  delayModRateLabel  { {}, "Mod Rate" };
    juce::Label  delayModDepthLabel { {}, "Mod Depth" };

    juce::Rectangle<int> correlationMeterBounds;

    // Preset browser
    juce::ListBox   presetListBox;
    juce::TextButton loadFactoryPresetButton { "Load Preset" };
    juce::Label      presetBrowserLabel { {}, "Factory Presets" };

    struct PresetListModel : public juce::ListBoxModel
    {
        std::vector<MyAmpSimAudioProcessor::FactoryPreset> presets;
        int getNumRows() override { return static_cast<int>(presets.size()); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override
        {
            if (selected) g.fillAll(juce::Colour(0xff3a7bd5));
            g.setColour(selected ? juce::Colours::white : juce::Colour(0xffc8c8c8));
            g.setFont(13.0f);
            if (row >= 0 && row < static_cast<int>(presets.size()))
            {
                const auto& p = presets[static_cast<size_t>(row)];
                g.drawText("[" + p.category + "] " + p.name, 6, 0, w - 6, h, juce::Justification::centredLeft);
            }
        }
    } presetListModel;

    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer;

    juce::AudioProcessorValueTreeState::SliderAttachment driveAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment volumeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gateAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gateAttackAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gateReleaseAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gateHysteresisAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gateRangeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment boostAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment bassAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment midAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment trebleAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment presenceAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment delayTimeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment delayMixAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment reverbMixAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment reverbRoomSizeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment reverbDampingAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment reverbWidthAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment reverbPreDelayAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment inputTrimAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment limiterThreshAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment limiterEnabledAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment delayFeedbackAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment delayModRateAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment delayModDepthAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment cabBlendAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment cabPanAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment cabLevelAAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment cabLevelBAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment irLowCutAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment irHighCutAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment irLevelAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment ampTypeAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment irPhaseAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment delaySyncAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment delayDivisionAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment cabFlipAAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment cabFlipBAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment oversamplingAttachment;
};
