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
    juce::ToggleButton cabFlipAButton { "Flip A" };
    juce::ToggleButton cabFlipBButton { "Flip B" };
    juce::TextButton undoButton { "Undo" };
    juce::TextButton redoButton { "Redo" };
    juce::ComboBox ampTypeCombo;
    juce::Label ampTypeLabel { {}, "Amp Type" };
    juce::ComboBox oversamplingCombo;
    juce::Label oversamplingLabel { {}, "Oversampling" };
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
    juce::Slider boostSlider;
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
    juce::Label  boostLabel { {}, "Boost" };
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

    juce::AudioProcessorValueTreeState::SliderAttachment driveAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment volumeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gateAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment boostAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment delayTimeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment delayMixAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment reverbMixAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment cabBlendAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment cabPanAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment cabLevelAAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment cabLevelBAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment irLowCutAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment irHighCutAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment irLevelAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment ampTypeAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment irPhaseAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment cabFlipAAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment cabFlipBAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment oversamplingAttachment;
};
