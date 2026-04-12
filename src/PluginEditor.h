#pragma once

#if __has_include(<JuceHeader.h>)
#include <JuceHeader.h>
#elif __has_include("../build-local/Vayu_artefacts/JuceLibraryCode/JuceHeader.h")
#include "../build-local/Vayu_artefacts/JuceLibraryCode/JuceHeader.h"
#else
#error Could not locate JuceHeader.h
#endif
#include "PluginProcessor.h"
#if __has_include("./ModernKnobLookAndFeel.h")
#include "./ModernKnobLookAndFeel.h"
#elif __has_include("ModernKnobLookAndFeel.h")
#include "ModernKnobLookAndFeel.h"
#elif __has_include("src/ModernKnobLookAndFeel.h")
#include "src/ModernKnobLookAndFeel.h"
#else
#error Could not locate ModernKnobLookAndFeel.h
#endif

class VayuAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit VayuAudioProcessorEditor(VayuAudioProcessor&);
    ~VayuAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    enum class UiTab { amp = 0, fx, tools };
    enum class UiScale { small = 0, medium, large, xlarge };
    void updateTabVisibility();
    void switchToTab(UiTab tab);
    void applyEditorSizePreset(UiScale preset);

    // Inline spectrum analyser component
    class SpectrumAnalyzer : public juce::Component, public juce::Timer
    {
    public:
        explicit SpectrumAnalyzer(VayuAudioProcessor& p)
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
        VayuAudioProcessor& processor;
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
    void chooseNamModel();
    void clearBackgroundImage();
    void drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float linearLevel, const juce::String& label) const;

    VayuAudioProcessor& audioProcessor;
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
    juce::TextButton autoAlignCabButton { "Auto Align B" };
    juce::TextButton savePresetButton { "Save Preset" };
    juce::TextButton loadPresetButton { "Load Preset" };
    juce::TextButton captureAButton { "Capture A" };
    juce::TextButton captureBButton { "Capture B" };
    juce::TextButton loadBackgroundButton { "Load Background" };
    juce::TextButton clearBackgroundButton { "Clear Background" };
    juce::TextButton loadNamButton  { "Load NAM"  };
    juce::TextButton clearNamButton  { "Clear NAM" };
    juce::ToggleButton namBypassToggle { "NAM Bypass" };
    juce::ToggleButton namAutoMatchToggle { "NAM Auto Match" };
    juce::ToggleButton compareABToggle { "Compare B" };
    juce::ToggleButton tunerToggle { "Show Tuner" };
    juce::ToggleButton irPhaseToggle { "IR Phase" };
    juce::ToggleButton delaySyncToggle { "Delay Sync" };
    juce::ToggleButton cabFlipAButton { "Flip A" };
    juce::ToggleButton cabFlipBButton { "Flip B" };
    juce::ToggleButton metalModeToggle { "Metal Mode" };
    juce::ToggleButton wahEnableToggle { "Wah" };
    juce::ToggleButton wahAutoToggle { "Auto Wah" };
    juce::ToggleButton killSwitchToggle { "Kill Switch" };
    juce::TextButton undoButton { "Undo" };
    juce::TextButton redoButton { "Redo" };
    juce::ComboBox ampTypeCombo;
    juce::Label ampTypeLabel { {}, "Amp Type" };
    juce::ComboBox oversamplingCombo;
    juce::Label oversamplingLabel { {}, "Oversampling" };
    juce::ComboBox delayDivisionCombo;
    juce::Label delayDivisionLabel { {}, "Division" };
    juce::ComboBox killRateCombo;
    juce::Label killRateLabel { {}, "Kill Rate" };
    juce::ComboBox midiParamCombo;
    juce::TextButton midiLearnButton { "MIDI Learn" };
    juce::Label irStatusLabel;
    juce::Label irStatusALabel;
    juce::Label irStatusBLabel;
    juce::Label cabAlignStatusLabel;
    juce::Label tunerNoteLabel;
    juce::Label tunerDetailLabel;
    juce::Label midiMapStatusLabel;
    juce::Label backgroundStatusLabel;
    juce::Label namStatusLabel;
    juce::Label namMatchLabel;
    juce::Label namBlendLabel { {}, "NAM Blend" };
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
    juce::Slider pitchShiftSlider;
    juce::Slider tightLowCutSlider;
    juce::Slider wahCenterSlider;
    juce::Slider wahDepthSlider;
    juce::Slider killDepthSlider;
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
    juce::Label  pitchShiftLabel { {}, "Pitch" };
    juce::Label  tightLowCutLabel { {}, "Tight Cut" };
    juce::Label  wahCenterLabel { {}, "Wah Freq" };
    juce::Label  wahDepthLabel { {}, "Wah Depth" };
    juce::Label  killDepthLabel { {}, "Kill Depth" };

    std::unique_ptr<juce::FileChooser> irChooser;
    std::unique_ptr<juce::FileChooser> presetChooser;
    std::unique_ptr<juce::FileChooser> backgroundChooser;
    std::unique_ptr<juce::FileChooser> namChooser;
    juce::MemoryBlock snapshotA;
    juce::MemoryBlock snapshotB;
    juce::Image backgroundImage;
    ModernKnobLookAndFeel ampLookAndFeel;
    UiTab currentTab = UiTab::amp;
    UiScale currentScale = UiScale::medium;
    float uiAnimationPhase = 0.0f;

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
    juce::Slider namBlendSlider;
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
        std::vector<VayuAudioProcessor::FactoryPreset> presets;
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
    juce::AudioProcessorValueTreeState::ButtonAttachment metalModeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment tightLowCutAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment pitchShiftAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment wahEnableAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment wahAutoAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment wahCenterAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment wahDepthAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment killSwitchAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment killRateAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment killDepthAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment namBypassAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment namAutoMatchAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment namBlendAttachment;

    // ═══════════════════════════════════════════════════════════════════
    //  NEW EFFECT UI CONTROLS
    // ═══════════════════════════════════════════════════════════════════

    // Distortion
    juce::ToggleButton distEnableToggle { "Dist" };
    juce::ComboBox distModeCombo;
    juce::Slider distGainSlider, distDriveSlider, distToneSlider, distMixSlider, distOutputSlider;
    juce::Slider distTightSlider, distHiCutSlider, distPresenceSlider, distGateSlider;
    juce::Label  distModeLabel { {}, "Mode" }, distGainLabel { {}, "Gain" }, distDriveLabel { {}, "Drive" }, distToneLabel { {}, "Tone" },
                 distMixLabel { {}, "Mix" }, distOutputLabel { {}, "Output" };
    juce::Label  distTightLabel { {}, "LowCut" }, distHiCutLabel { {}, "HiCut" },
                 distPresenceLabel { {}, "Presence" }, distGateLabel { {}, "Gate" };
    juce::AudioProcessorValueTreeState::ButtonAttachment distEnableAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment distModeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment distGainAttachment, distDriveAttachment,
        distToneAttachment, distMixAttachment, distOutputAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment distTightAttachment, distHiCutAttachment,
        distPresenceAttachment, distGateAttachment;

    // Compressor
    juce::ToggleButton compEnableToggle { "Comp" };
    juce::Slider compThreshSlider, compRatioSlider, compAttackSlider, compReleaseSlider, compMakeupSlider, compMixSlider;
    juce::Label  compThreshLabel { {}, "Thresh" }, compRatioLabel { {}, "Ratio" },
                 compAttackLabel { {}, "Attack" }, compReleaseLabel { {}, "Release" },
                 compMakeupLabel { {}, "Makeup" }, compMixLabel { {}, "Mix" };
    juce::AudioProcessorValueTreeState::ButtonAttachment compEnableAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compThreshAttachment, compRatioAttachment,
        compAttackAttachment, compReleaseAttachment, compMakeupAttachment, compMixAttachment;

    // Chorus
    juce::ToggleButton chorusEnableToggle { "Chorus" };
    juce::Slider chorusRateSlider, chorusDepthSlider, chorusMixSlider;
    juce::Label  chorusRateLabel { {}, "Rate" }, chorusDepthLabel { {}, "Depth" }, chorusMixLabel { {}, "Mix" };
    juce::AudioProcessorValueTreeState::ButtonAttachment chorusEnableAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment chorusRateAttachment, chorusDepthAttachment, chorusMixAttachment;

    // Phaser
    juce::ToggleButton phaserEnableToggle { "Phaser" };
    juce::Slider phaserRateSlider, phaserDepthSlider, phaserFeedbackSlider, phaserMixSlider;
    juce::Label  phaserRateLabel { {}, "Rate" }, phaserDepthLabel { {}, "Depth" },
                 phaserFeedbackLabel { {}, "Feedback" }, phaserMixLabel { {}, "Mix" };
    juce::AudioProcessorValueTreeState::ButtonAttachment phaserEnableAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment phaserRateAttachment, phaserDepthAttachment,
        phaserFeedbackAttachment, phaserMixAttachment;

    // Flanger
    juce::ToggleButton flangerEnableToggle { "Flanger" };
    juce::Slider flangerRateSlider, flangerDepthSlider, flangerFeedbackSlider, flangerMixSlider;
    juce::Label  flangerRateLabel { {}, "Rate" }, flangerDepthLabel { {}, "Depth" },
                 flangerFeedbackLabel { {}, "Feedback" }, flangerMixLabel { {}, "Mix" };
    juce::AudioProcessorValueTreeState::ButtonAttachment flangerEnableAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment flangerRateAttachment, flangerDepthAttachment,
        flangerFeedbackAttachment, flangerMixAttachment;

    // Tremolo
    juce::ToggleButton tremoloEnableToggle { "Tremolo" };
    juce::ToggleButton tremoloSyncToggle { "Sync" };
    juce::Slider tremoloRateSlider, tremoloDepthSlider;
    juce::ComboBox tremoloShapeCombo;
    juce::ComboBox tremoloDivisionCombo;
    juce::Label  tremoloRateLabel { {}, "Rate" }, tremoloDepthLabel { {}, "Depth" }, tremoloShapeLabel { {}, "Shape" },
                 tremoloDivisionLabel { {}, "Div" };
    juce::AudioProcessorValueTreeState::ButtonAttachment tremoloEnableAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment tremoloSyncAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment tremoloRateAttachment, tremoloDepthAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment tremoloShapeAttachment, tremoloDivisionAttachment;

    // Parametric EQ
    juce::ToggleButton peqEnableToggle { "PEQ" };
    juce::Slider peqFreq1Slider, peqGain1Slider, peqQ1Slider;
    juce::Slider peqFreq2Slider, peqGain2Slider, peqQ2Slider;
    juce::Slider peqFreq3Slider, peqGain3Slider, peqQ3Slider;
    juce::Label  peqFreq1Label { {}, "Freq1" }, peqGain1Label { {}, "Gain1" }, peqQ1Label { {}, "Q1" };
    juce::Label  peqFreq2Label { {}, "Freq2" }, peqGain2Label { {}, "Gain2" }, peqQ2Label { {}, "Q2" };
    juce::Label  peqFreq3Label { {}, "Freq3" }, peqGain3Label { {}, "Gain3" }, peqQ3Label { {}, "Q3" };
    juce::AudioProcessorValueTreeState::ButtonAttachment peqEnableAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment peqFreq1Attachment, peqGain1Attachment, peqQ1Attachment;
    juce::AudioProcessorValueTreeState::SliderAttachment peqFreq2Attachment, peqGain2Attachment, peqQ2Attachment;
    juce::AudioProcessorValueTreeState::SliderAttachment peqFreq3Attachment, peqGain3Attachment, peqQ3Attachment;

    // Metronome
    juce::ToggleButton metroEnableToggle { "Metro" };
    juce::ToggleButton metroSyncToggle { "Host Sync" };
    juce::TextButton tapTempoButton { "Tap Tempo" };
    juce::Slider metroBpmSlider, metroLevelSlider;
    juce::ComboBox metroTimeSigCombo;
    juce::Label  metroBpmLabel { {}, "BPM" }, metroLevelLabel { {}, "Level" }, metroTimeSigLabel { {}, "Time Sig" };
    juce::AudioProcessorValueTreeState::ButtonAttachment metroEnableAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment metroSyncAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment metroBpmAttachment, metroLevelAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment metroTimeSigAttachment;

    // Looper
    juce::TextButton looperRecordButton { "Rec" };
    juce::TextButton looperPlayButton { "Play" };
    juce::TextButton looperStopButton { "Stop" };
    juce::TextButton looperClearButton { "Clear" };
    juce::Slider looperLevelSlider;
    juce::Label  looperLevelLabel { {}, "Loop Vol" };
    juce::Label  looperStatusLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment looperLevelAttachment;

    double lastTapTempoMs = 0.0;
    std::array<double, 4> tapTempoIntervals {};
    int tapTempoCount = 0;
};
