#include "PluginEditor.h"

#include <cmath>

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

VayuAudioProcessorEditor::VayuAudioProcessorEditor(VayuAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor),
      driveAttachment(audioProcessor.apvts, "drive", driveSlider),
    volumeAttachment(audioProcessor.apvts, "outputVolume", volumeSlider),
    gateAttachment(audioProcessor.apvts, "gateThreshold", gateSlider),
        gateAttackAttachment(audioProcessor.apvts, "gateAttackMs", gateAttackSlider),
        gateReleaseAttachment(audioProcessor.apvts, "gateReleaseMs", gateReleaseSlider),
        gateHysteresisAttachment(audioProcessor.apvts, "gateHysteresisDb", gateHysteresisSlider),
        gateRangeAttachment(audioProcessor.apvts, "gateRangeDb", gateRangeSlider),
    boostAttachment(audioProcessor.apvts, "boostDb", boostSlider),
        bassAttachment(audioProcessor.apvts, "ampBassDb", bassSlider),
        midAttachment(audioProcessor.apvts, "ampMidDb", midSlider),
        trebleAttachment(audioProcessor.apvts, "ampTrebleDb", trebleSlider),
        presenceAttachment(audioProcessor.apvts, "ampPresenceDb", presenceSlider),
    delayTimeAttachment(audioProcessor.apvts, "delayTimeMs", delayTimeSlider),
    delayMixAttachment(audioProcessor.apvts, "delayMix", delayMixSlider),
    reverbMixAttachment(audioProcessor.apvts, "reverbMix", reverbMixSlider),
    irLowCutAttachment(audioProcessor.apvts, "irLowCutHz", irLowCutSlider),
    irHighCutAttachment(audioProcessor.apvts, "irHighCutHz", irHighCutSlider),
    irLevelAttachment(audioProcessor.apvts, "irLevelDb", irLevelSlider),
    cabBlendAttachment(audioProcessor.apvts, "cabBlend", cabBlendSlider),
    cabPanAttachment(audioProcessor.apvts, "cabPan", cabPanSlider),
    cabLevelAAttachment(audioProcessor.apvts, "cabLevelA", cabLevelASlider),
    cabLevelBAttachment(audioProcessor.apvts, "cabLevelB", cabLevelBSlider),
    ampTypeAttachment(audioProcessor.apvts, "ampType", ampTypeCombo),
    irPhaseAttachment(audioProcessor.apvts, "irPhaseInvert", irPhaseToggle),
    delaySyncAttachment(audioProcessor.apvts, "delaySync", delaySyncToggle),
    delayDivisionAttachment(audioProcessor.apvts, "delayDivision", delayDivisionCombo),
    cabFlipAAttachment(audioProcessor.apvts, "cabFlipA", cabFlipAButton),
    cabFlipBAttachment(audioProcessor.apvts, "cabFlipB", cabFlipBButton),
    oversamplingAttachment(audioProcessor.apvts, "oversamplingMode", oversamplingCombo),
    metalModeAttachment(audioProcessor.apvts, "metalMode", metalModeToggle),
    tightLowCutAttachment(audioProcessor.apvts, "tightLowCutHz", tightLowCutSlider),
    pitchShiftAttachment(audioProcessor.apvts, "pitchShiftSemi", pitchShiftSlider),
    wahEnableAttachment(audioProcessor.apvts, "wahEnable", wahEnableToggle),
    wahAutoAttachment(audioProcessor.apvts, "wahAuto", wahAutoToggle),
    wahCenterAttachment(audioProcessor.apvts, "wahCenterHz", wahCenterSlider),
    wahDepthAttachment(audioProcessor.apvts, "wahDepth", wahDepthSlider),
    killSwitchAttachment(audioProcessor.apvts, "killEnable", killSwitchToggle),
    killRateAttachment(audioProcessor.apvts, "killRate", killRateCombo),
    killDepthAttachment(audioProcessor.apvts, "killDepth", killDepthSlider),
    reverbRoomSizeAttachment(audioProcessor.apvts, "reverbRoomSize", reverbRoomSizeSlider),
    reverbDampingAttachment(audioProcessor.apvts, "reverbDamping", reverbDampingSlider),
    reverbWidthAttachment(audioProcessor.apvts, "reverbWidth", reverbWidthSlider),
    reverbPreDelayAttachment(audioProcessor.apvts, "reverbPreDelayMs", reverbPreDelaySlider),
    inputTrimAttachment(audioProcessor.apvts, "inputTrimDb", inputTrimSlider),
    limiterThreshAttachment(audioProcessor.apvts, "limiterThreshDb", limiterThreshSlider),
    limiterEnabledAttachment(audioProcessor.apvts, "limiterEnabled", limiterEnabledToggle)
    ,
    delayFeedbackAttachment(audioProcessor.apvts, "delayFeedback", delayFeedbackSlider),
    delayModRateAttachment(audioProcessor.apvts, "delayModRate", delayModRateSlider),
    delayModDepthAttachment(audioProcessor.apvts, "delayModDepth", delayModDepthSlider),
    namBypassAttachment(audioProcessor.apvts, "namBypass", namBypassToggle),
    namAutoMatchAttachment(audioProcessor.apvts, "namAutoMatch", namAutoMatchToggle),
    namBlendAttachment(audioProcessor.apvts, "namBlend", namBlendSlider)
{
    setLookAndFeel(&ampLookAndFeel);

    // Dark premium baseline palette.
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff141518));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff2a2c31));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffeceff4));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1f2228));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd8dce3));
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101114));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffeef1f7));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2a2d33));

    addAndMakeVisible(ampTabButton);
    addAndMakeVisible(fxTabButton);
    addAndMakeVisible(toolsTabButton);
    addAndMakeVisible(uiSizeLabel);
    addAndMakeVisible(uiSizeCombo);

    ampTabButton.onClick = [this] { switchToTab(UiTab::amp); };
    fxTabButton.onClick = [this] { switchToTab(UiTab::fx); };
    toolsTabButton.onClick = [this] { switchToTab(UiTab::tools); };

    uiSizeLabel.setJustificationType(juce::Justification::centredRight);
    uiSizeCombo.addItem("Small", 1);
    uiSizeCombo.addItem("Medium", 2);
    uiSizeCombo.addItem("Large", 3);
    uiSizeCombo.addItem("XLarge", 4);
    uiSizeCombo.setSelectedId(2);
    uiSizeCombo.onChange = [this]
    {
        const int selected = uiSizeCombo.getSelectedId();
        if (selected == 1)
            applyEditorSizePreset(UiScale::small);
        else if (selected == 3)
            applyEditorSizePreset(UiScale::large);
        else if (selected == 4)
            applyEditorSizePreset(UiScale::xlarge);
        else
            applyEditorSizePreset(UiScale::medium);
    };

    ampTypeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(ampTypeLabel);
    addAndMakeVisible(ampTypeCombo);
    ampTypeCombo.addItem("Clean", 1);
    ampTypeCombo.addItem("Crunch", 2);
    ampTypeCombo.addItem("Lead", 3);

    addAndMakeVisible(audioSettingsButton);
    audioSettingsButton.onClick = [this] { openAudioSettings(); };
    audioSettingsButton.setEnabled(audioProcessor.wrapperType == juce::AudioProcessor::wrapperType_Standalone);

    addAndMakeVisible(loadIrButton);
    loadIrButton.onClick = [this] { chooseCabinetIR(); };

    addAndMakeVisible(clearIrButton);
    clearIrButton.onClick = [this]
    {
        audioProcessor.clearCabinetIR();
        refreshIrStatus();
    };

    addAndMakeVisible(loadIrAButton);
    loadIrAButton.onClick = [this] { chooseCabinetIRSlot(0); };

    addAndMakeVisible(clearIrAButton);
    clearIrAButton.onClick = [this]
    {
        audioProcessor.clearCabinetIRSlot(0);
        refreshIrStatus();
    };

    addAndMakeVisible(loadIrBButton);
    loadIrBButton.onClick = [this] { chooseCabinetIRSlot(1); };

    addAndMakeVisible(clearIrBButton);
    clearIrBButton.onClick = [this]
    {
        audioProcessor.clearCabinetIRSlot(1);
        refreshIrStatus();
    };

    addAndMakeVisible(autoAlignCabButton);
    autoAlignCabButton.onClick = [this]
    {
        if (!audioProcessor.autoAlignCabPolarity())
        {
            cabAlignStatusLabel.setText("Align: Load IR A and B", juce::dontSendNotification);
            return;
        }
        refreshIrStatus();
    };

    addAndMakeVisible(savePresetButton);
    savePresetButton.onClick = [this] { savePresetToFile(); };

    addAndMakeVisible(loadPresetButton);
    loadPresetButton.onClick = [this] { loadPresetFromFile(); };

    addAndMakeVisible(captureAButton);
    captureAButton.onClick = [this] { captureSnapshotA(); };

    addAndMakeVisible(captureBButton);
    captureBButton.onClick = [this] { captureSnapshotB(); };

    addAndMakeVisible(compareABToggle);
    compareABToggle.onClick = [this] { recallSnapshotAorB(); };

    addAndMakeVisible(loadBackgroundButton);
    loadBackgroundButton.onClick = [this] { chooseBackgroundImage(); };

    addAndMakeVisible(clearBackgroundButton);
    clearBackgroundButton.onClick = [this] { clearBackgroundImage(); };

    addAndMakeVisible(loadNamButton);
    loadNamButton.onClick = [this] { chooseNamModel(); };
    addAndMakeVisible(clearNamButton);
    clearNamButton.onClick = [this] {
        audioProcessor.clearNamModel();
        refreshToolsStatus();
    };
    addAndMakeVisible(namBypassToggle);
    addAndMakeVisible(namAutoMatchToggle);
    namBlendLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(namBlendLabel);
    namBlendSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    namBlendSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 20);
    addAndMakeVisible(namBlendSlider);

    addAndMakeVisible(tunerToggle);
    tunerToggle.setToggleState(false, juce::dontSendNotification);
    tunerToggle.onClick = [this] { updateTabVisibility(); };

    addAndMakeVisible(irPhaseToggle);
    addAndMakeVisible(cabFlipAButton);
    addAndMakeVisible(cabFlipBButton);

    oversamplingLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(oversamplingLabel);
    addAndMakeVisible(oversamplingCombo);
    oversamplingCombo.addItem("Off", 1);
    oversamplingCombo.addItem("2x", 2);
    oversamplingCombo.addItem("4x", 3);

    addAndMakeVisible(delaySyncToggle);
    delaySyncToggle.setButtonText("Delay Sync");

    delayDivisionLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(delayDivisionLabel);
    addAndMakeVisible(delayDivisionCombo);
    delayDivisionCombo.addItem("1/4", 1);
    delayDivisionCombo.addItem("1/8", 2);
    delayDivisionCombo.addItem("1/8D", 3);
    delayDivisionCombo.addItem("1/8T", 4);
    delayDivisionCombo.addItem("1/16", 5);

    killRateLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(killRateLabel);
    addAndMakeVisible(killRateCombo);
    killRateCombo.addItem("1/4", 1);
    killRateCombo.addItem("1/8", 2);
    killRateCombo.addItem("1/16", 3);
    killRateCombo.addItem("1/32", 4);

    addAndMakeVisible(undoButton);
    undoButton.onClick = [this] { audioProcessor.undoLastChange(); };

    addAndMakeVisible(redoButton);
    redoButton.onClick = [this] { audioProcessor.redoLastChange(); };

    addAndMakeVisible(midiParamCombo);
    midiParamCombo.addItem("Drive", 1);
    midiParamCombo.addItem("Output", 2);
    midiParamCombo.addItem("Gate", 3);
    midiParamCombo.addItem("Boost", 4);
    midiParamCombo.addItem("Delay Mix", 5);
    midiParamCombo.addItem("Reverb Mix", 6);
    midiParamCombo.addItem("Cab Blend", 7);
    midiParamCombo.addItem("Cab Pan", 8);
    midiParamCombo.addItem("Cab A Level", 9);
    midiParamCombo.addItem("Cab B Level", 10);
    midiParamCombo.addItem("Delay Feedback", 11);
    midiParamCombo.addItem("Delay Time", 12);
    midiParamCombo.addItem("Reverb Room", 13);
    midiParamCombo.addItem("Reverb Damping", 14);
    midiParamCombo.addItem("Bass", 15);
    midiParamCombo.addItem("Mid", 16);
    midiParamCombo.addItem("Treble", 17);
    midiParamCombo.addItem("Presence", 18);
    midiParamCombo.addItem("Pitch", 19);
    midiParamCombo.addItem("Tight Cut", 20);
    midiParamCombo.addItem("Wah Freq", 21);
    midiParamCombo.addItem("Wah Depth", 22);
    midiParamCombo.addItem("Kill Depth", 23);
    midiParamCombo.addItem("NAM Blend", 24);
    midiParamCombo.addItem("Input Trim", 25);
    midiParamCombo.setSelectedId(1);

    addAndMakeVisible(midiLearnButton);
    midiLearnButton.onClick = [this]
    {
        const int idx = juce::jmax(0, midiParamCombo.getSelectedId() - 1);
        audioProcessor.beginMidiLearnForParam(idx);
        midiMapStatusLabel.setText("Move a MIDI CC now...", juce::dontSendNotification);
    };

    midiMapStatusLabel.setJustificationType(juce::Justification::centredLeft);
    midiMapStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
    addAndMakeVisible(midiMapStatusLabel);

    backgroundStatusLabel.setJustificationType(juce::Justification::centredLeft);
    backgroundStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(backgroundStatusLabel);

    namStatusLabel.setJustificationType(juce::Justification::centredLeft);
    namStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(namStatusLabel);

    namMatchLabel.setJustificationType(juce::Justification::centredLeft);
    namMatchLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen.withAlpha(0.75f));
    addAndMakeVisible(namMatchLabel);

    irStatusLabel.setJustificationType(juce::Justification::centredLeft);
    irStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(irStatusLabel);

    irStatusALabel.setJustificationType(juce::Justification::centredLeft);
    irStatusALabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(irStatusALabel);

    irStatusBLabel.setJustificationType(juce::Justification::centredLeft);
    irStatusBLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(irStatusBLabel);

    cabAlignStatusLabel.setJustificationType(juce::Justification::centredLeft);
    cabAlignStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue.withAlpha(0.85f));
    cabAlignStatusLabel.setText("Align: --", juce::dontSendNotification);
    addAndMakeVisible(cabAlignStatusLabel);

    tunerNoteLabel.setJustificationType(juce::Justification::centred);
    tunerNoteLabel.setFont(tunerNoteLabel.getFont().withHeight(36.0f).boldened());
    tunerNoteLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    addAndMakeVisible(tunerNoteLabel);

    tunerDetailLabel.setJustificationType(juce::Justification::centred);
    tunerDetailLabel.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
    addAndMakeVisible(tunerDetailLabel);

    auto setupSlider = [this](juce::Slider& slider, juce::Label& label)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 16);
        addAndMakeVisible(slider);

        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false);
        addAndMakeVisible(label);
    };

    setupSlider(driveSlider, driveLabel);
    setupSlider(volumeSlider, volumeLabel);
    setupSlider(gateSlider, gateLabel);
    setupSlider(boostSlider, boostLabel);
    setupSlider(bassSlider, bassLabel);
    setupSlider(midSlider, midLabel);
    setupSlider(trebleSlider, trebleLabel);
    setupSlider(presenceSlider, presenceLabel);
    setupSlider(delayTimeSlider, delayTimeLabel);
    setupSlider(delayMixSlider, delayMixLabel);
    setupSlider(reverbMixSlider, reverbMixLabel);
    setupSlider(irLowCutSlider, irLowCutLabel);
    setupSlider(irHighCutSlider, irHighCutLabel);
    setupSlider(irLevelSlider, irLevelLabel);
    setupSlider(cabBlendSlider, cabBlendLabel);
    setupSlider(cabPanSlider, cabPanLabel);
    setupSlider(cabLevelASlider, cabLevelALabel);
    setupSlider(cabLevelBSlider, cabLevelBLabel);

    // Reverb advanced
    setupSlider(reverbRoomSizeSlider, reverbRoomSizeLabel);
    setupSlider(reverbDampingSlider, reverbDampingLabel);
    setupSlider(reverbWidthSlider, reverbWidthLabel);
    setupSlider(reverbPreDelaySlider, reverbPreDelayLabel);

    // Input trim + limiter
    // Delay extras
    setupSlider(delayFeedbackSlider, delayFeedbackLabel);
    setupSlider(delayModRateSlider, delayModRateLabel);
    setupSlider(delayModDepthSlider, delayModDepthLabel);

    setupSlider(inputTrimSlider, inputTrimLabel);
    setupSlider(limiterThreshSlider, limiterThreshLabel);
    addAndMakeVisible(limiterEnabledToggle);
    limiterEnabledToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::orangered);
    addAndMakeVisible(metalModeToggle);
    addAndMakeVisible(wahEnableToggle);
    addAndMakeVisible(wahAutoToggle);
    addAndMakeVisible(killSwitchToggle);

    limiterClipLabel.setText("", juce::dontSendNotification);
    limiterClipLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    limiterClipLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    limiterClipLabel.setFont(limiterClipLabel.getFont().withHeight(11.0f).boldened());
    limiterClipLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(limiterClipLabel);

    // Preset browser
    presetListModel.presets = VayuAudioProcessor::getFactoryPresets();
    presetListBox.setModel(&presetListModel);
    presetListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    presetListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff404040));
    presetListBox.setOutlineThickness(1);
    addAndMakeVisible(presetListBox);
    addAndMakeVisible(presetBrowserLabel);
    presetBrowserLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0a0a0));
    presetBrowserLabel.setFont(presetBrowserLabel.getFont().withHeight(13.0f).boldened());

    addAndMakeVisible(loadFactoryPresetButton);
    loadFactoryPresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a5fa5));
    loadFactoryPresetButton.onClick = [this]
    {
        const int row = presetListBox.getSelectedRow();
        if (row >= 0)
            audioProcessor.loadFactoryPreset(row);
    };

    spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>(audioProcessor);
    addAndMakeVisible(spectrumAnalyzer.get());

    auto makeCompactLevelSlider = [](juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
    };

    auto makeCompactGateSlider = [](juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
    };

    setupSlider(gateAttackSlider, gateAttackLabel);
    setupSlider(gateReleaseSlider, gateReleaseLabel);
    setupSlider(gateHysteresisSlider, gateHysteresisLabel);
    setupSlider(gateRangeSlider, gateRangeLabel);
    makeCompactGateSlider(gateAttackSlider);
    makeCompactGateSlider(gateReleaseSlider);
    makeCompactGateSlider(gateHysteresisSlider);
    makeCompactGateSlider(gateRangeSlider);

    makeCompactLevelSlider(cabLevelASlider);
    makeCompactLevelSlider(cabLevelBSlider);

    auto makeCompactControlSlider = [](juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 18);
    };

    makeCompactControlSlider(pitchShiftSlider);
    makeCompactControlSlider(tightLowCutSlider);
    makeCompactControlSlider(wahCenterSlider);
    makeCompactControlSlider(wahDepthSlider);
    makeCompactControlSlider(killDepthSlider);

    addAndMakeVisible(pitchShiftSlider);
    addAndMakeVisible(tightLowCutSlider);
    addAndMakeVisible(wahCenterSlider);
    addAndMakeVisible(wahDepthSlider);
    addAndMakeVisible(killDepthSlider);

    auto setupCompactLabel = [this](juce::Label& label)
    {
        label.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(label);
    };
    setupCompactLabel(pitchShiftLabel);
    setupCompactLabel(tightLowCutLabel);
    setupCompactLabel(wahCenterLabel);
    setupCompactLabel(wahDepthLabel);
    setupCompactLabel(killDepthLabel);

    // Unified control styling for a cleaner, professional dark UI.
    auto styleSecondaryButton = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1b1f26));
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff2b3340));
        b.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd7dce6));
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    };

    auto styleAccentButton = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff243a52));
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff2f4f73));
        b.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffedf2fb));
    };

    styleAccentButton(loadFactoryPresetButton);
    styleAccentButton(audioSettingsButton);
    styleSecondaryButton(loadIrAButton);
    styleSecondaryButton(clearIrAButton);
    styleSecondaryButton(loadIrBButton);
    styleSecondaryButton(clearIrBButton);
    styleSecondaryButton(autoAlignCabButton);
    styleSecondaryButton(savePresetButton);
    styleSecondaryButton(loadPresetButton);
    styleSecondaryButton(captureAButton);
    styleSecondaryButton(captureBButton);
    styleSecondaryButton(loadBackgroundButton);
    styleSecondaryButton(clearBackgroundButton);
    styleSecondaryButton(loadNamButton);
    styleSecondaryButton(clearNamButton);
    styleSecondaryButton(undoButton);
    styleSecondaryButton(redoButton);
    styleAccentButton(midiLearnButton);

    auto styleToggle = [](juce::ToggleButton& t, juce::Colour tick)
    {
        t.setColour(juce::ToggleButton::tickColourId, tick);
        t.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffd7dce6));
    };

    styleToggle(tunerToggle, juce::Colour(0xff5db7ff));
    styleToggle(delaySyncToggle, juce::Colour(0xff5db7ff));
    styleToggle(irPhaseToggle, juce::Colour(0xff86c7ff));
    styleToggle(cabFlipAButton, juce::Colour(0xff86c7ff));
    styleToggle(cabFlipBButton, juce::Colour(0xff86c7ff));
    styleToggle(metalModeToggle, juce::Colour(0xffff9254));
    styleToggle(wahEnableToggle, juce::Colour(0xff58d6ff));
    styleToggle(wahAutoToggle, juce::Colour(0xff58d6ff));
    styleToggle(killSwitchToggle, juce::Colour(0xffff7070));
    styleToggle(namBypassToggle, juce::Colour(0xff5affb4));
    styleToggle(namAutoMatchToggle, juce::Colour(0xff5affb4));

    for (auto* combo : { &uiSizeCombo, &ampTypeCombo, &oversamplingCombo, &delayDivisionCombo, &killRateCombo, &midiParamCombo })
    {
        combo->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff141821));
        combo->setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff2f3642));
        combo->setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6ebf4));
        combo->setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff90a0b8));
    }

    for (auto* status : { &midiMapStatusLabel, &backgroundStatusLabel, &namStatusLabel, &namMatchLabel, &irStatusLabel, &irStatusALabel, &irStatusBLabel, &cabAlignStatusLabel })
        status->setFont(status->getFont().withHeight(12.5f));

    const auto bgPath = audioProcessor.getBackgroundImagePath();
    if (bgPath.isNotEmpty())
        loadBackgroundImageFromFile(juce::File(bgPath));

    refreshIrStatus();
    refreshTunerStatus();
    refreshToolsStatus();

    audioProcessor.getStateInformation(snapshotA);
    snapshotB = snapshotA;

    switchToTab(UiTab::amp);

    startTimerHz(15);
    applyEditorSizePreset(UiScale::medium);
}

VayuAudioProcessorEditor::~VayuAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VayuAudioProcessorEditor::paint(juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();
    const float sf  = static_cast<float>(W) / 760.0f;
    const auto fpx  = [&](float v) { return juce::jmax(2.0f, v * sf); };
    const float pulse = 0.5f + 0.5f * std::sin(uiAnimationPhase);

    // ── Background ───────────────────────────────────────────────────────────
    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
        g.setColour(juce::Colours::black.withAlpha(0.52f));
        g.fillRect(getLocalBounds());
    }
    else
    {
        g.fillAll(juce::Colour(0xff0b0d11));
        // Subtle bottom vignette
        juce::ColourGradient vig(juce::Colours::transparentBlack, W * 0.5f, H * 0.45f,
                                  juce::Colours::black.withAlpha(0.28f), W * 0.5f, static_cast<float>(H), true);
        g.setGradientFill(vig);
        g.fillRect(getLocalBounds());
    }

    // ── Header panel ─────────────────────────────────────────────────────────
    const float headerH = juce::jmax(36.0f, 44.0f * (static_cast<float>(H) / 520.0f));
    const float hInset  = 4.0f;
    juce::Rectangle<float> hPanel(hInset, hInset, W - hInset * 2.0f, headerH - hInset);
    g.setColour(juce::Colour(0xff131620).withAlpha(0.92f));
    g.fillRoundedRectangle(hPanel, fpx(7.0f));
    g.setColour(juce::Colour(0xff263040).withAlpha(0.5f));
    g.drawRoundedRectangle(hPanel.reduced(0.5f), fpx(7.0f), 1.0f);

    // VAYU title – centred in the header, right of tabs
    const float tabW   = fpx(88.0f);
    const float tabGap = fpx(5.0f);
    const float tabsEndX = fpx(10.0f) + 3.0f * tabW + 2.0f * tabGap + fpx(16.0f);
    const float titleFontH = juce::jmax(14.0f, headerH * 0.48f);
    g.setFont(juce::Font("Arial", titleFontH, juce::Font::bold));
    g.setColour(juce::Colours::white.withAlpha(0.88f + 0.10f * pulse));
    g.drawFittedText("VAYU",
        juce::Rectangle<float>(tabsEndX, hInset, fpx(110.0f), headerH - hInset).toNearestInt(),
        juce::Justification::centredLeft, 1);
    // Tiny tagline
    g.setFont(juce::Font("Arial", juce::jmax(8.5f, headerH * 0.22f), juce::Font::plain));
    g.setColour(juce::Colour(0xff3e4d66).withAlpha(0.85f));
    g.drawFittedText("AMP SIMULATOR",
        juce::Rectangle<float>(tabsEndX, hInset + titleFontH * 0.96f, fpx(120.0f), headerH - hInset).toNearestInt(),
        juce::Justification::centredLeft, 1);

    // Active-tab underline indicator (cyan)
    const int activeIdx = (currentTab == UiTab::amp) ? 0 : (currentTab == UiTab::fx) ? 1 : 2;
    const float tabLineX = fpx(10.0f) + activeIdx * (tabW + tabGap);
    const float tabLineY = hPanel.getBottom() - 2.5f;
    g.setColour(juce::Colour(0xff42c8ff).withAlpha(0.88f + 0.10f * pulse));
    g.fillRoundedRectangle(tabLineX + tabW * 0.12f, tabLineY, tabW * 0.76f, 2.5f, 1.5f);

    // Outer frame
    g.setColour(juce::Colour(0xff1e2530).withAlpha(0.55f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), fpx(8.0f), 1.0f);

    // ── Section labels (drawn behind knobs) ──────────────────────────────────
    // These approximate the same geometry as resized() so they sit just
    // above each knob row without having to store extra member Rectangles.
    if (!inputMeterBounds.isEmpty() && currentTab == UiTab::amp)
    {
        const float ctxH    = juce::jmax(24.0f, 28.0f * sf);
        const float statusH = juce::jmax(18.0f, 22.0f * sf);
        const float dualH   = juce::jmax(20.0f, 24.0f * sf);
        const float topUsed = headerH + 4.0f + ctxH + 3.0f + statusH + 6.0f + dualH + 6.0f;
        const float availH  = H - topUsed - 8.0f;
        const float kAreaX  = fpx(10.0f);
        const float kAreaY  = topUsed + fpx(8.0f);
        const float mColW   = fpx(26.0f) * 3.0f + fpx(14.0f) + fpx(5.0f);
        const float kAreaW  = W - fpx(10.0f) * 2.0f - fpx(8.0f) - mColW;
        const float gapY    = fpx(12.0f);
        const float kAreaH  = juce::jmax(180.0f, availH * 0.68f);
        const float cellH   = (kAreaH - gapY * 3.0f) / 4.0f;
        const float panelW  = kAreaW;

        // SIGNAL panel (row 0)
        {
            juce::Rectangle<float> p(kAreaX, kAreaY, panelW, cellH + gapY * 0.55f);
            g.setColour(juce::Colour(0xff131620).withAlpha(0.42f));
            g.fillRoundedRectangle(p, fpx(7.0f));
            g.setColour(juce::Colour(0xff253040).withAlpha(0.28f));
            g.drawRoundedRectangle(p, fpx(7.0f), 0.8f);
            g.setFont(juce::Font("Arial", juce::jmax(9.0f, 10.0f * sf), juce::Font::plain));
            g.setColour(juce::Colour(0xff3d5070).withAlpha(0.80f));
            g.drawText("SIGNAL", juce::Rectangle<float>(kAreaX + fpx(8.0f), kAreaY - fpx(13.0f),
                                                         fpx(80.0f), fpx(14.0f)).toNearestInt(),
                       juce::Justification::centredLeft);
        }
        // EQ panel (row 1)
        {
            const float rowY = kAreaY + cellH + gapY;
            juce::Rectangle<float> p(kAreaX, rowY, panelW, cellH + gapY * 0.55f);
            g.setColour(juce::Colour(0xff131620).withAlpha(0.42f));
            g.fillRoundedRectangle(p, fpx(7.0f));
            g.setColour(juce::Colour(0xff253040).withAlpha(0.28f));
            g.drawRoundedRectangle(p, fpx(7.0f), 0.8f);
            g.setFont(juce::Font("Arial", juce::jmax(9.0f, 10.0f * sf), juce::Font::plain));
            g.setColour(juce::Colour(0xff3d5070).withAlpha(0.80f));
            g.drawText("EQ", juce::Rectangle<float>(kAreaX + fpx(8.0f), rowY - fpx(13.0f),
                                                     fpx(48.0f), fpx(14.0f)).toNearestInt(),
                       juce::Justification::centredLeft);
        }
    }

    // ── Tuner overlay ─────────────────────────────────────────────────────────
    if (tunerToggle.getToggleState() && tunerNoteLabel.isVisible())
    {
        auto tBounds = tunerNoteLabel.getBounds().getUnion(tunerDetailLabel.getBounds());
        auto card    = tBounds.expanded(static_cast<int>(fpx(14.0f)), static_cast<int>(fpx(14.0f))).toFloat();

        // Drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.38f));
        g.fillRoundedRectangle(card.translated(0.0f, 2.0f), fpx(12.0f));

        // Card body
        juce::ColourGradient bg2(juce::Colour(0xff1a2230), card.getX(), card.getY(),
                                  juce::Colour(0xff0d1420), card.getX(), card.getBottom(), false);
        g.setGradientFill(bg2);
        g.fillRoundedRectangle(card, fpx(12.0f));
        g.setColour(juce::Colour(0xff3ac6ff).withAlpha(0.45f + 0.12f * pulse));
        g.drawRoundedRectangle(card.reduced(0.8f), fpx(12.0f), 1.2f);

        // Cents meter bar
        auto meter = card.withTrimmedTop(card.getHeight() - fpx(16.0f)).reduced(fpx(12.0f), fpx(2.0f));
        g.setColour(juce::Colour(0xff182030));
        g.fillRoundedRectangle(meter, fpx(4.0f));

        const float cents = juce::jlimit(-50.0f, 50.0f, audioProcessor.getTunerCents());
        // Tick marks
        for (int ti = 0; ti <= 10; ++ti)
        {
            const float tx = meter.getX() + (ti / 10.0f) * meter.getWidth();
            const bool isCtr = (ti == 5);
            g.setColour(juce::Colours::white.withAlpha(isCtr ? 0.45f : 0.16f));
            g.fillRect(tx - 0.5f, meter.getY() + (isCtr ? 0.0f : fpx(2.0f)),
                       1.0f, meter.getHeight() - (isCtr ? 0.0f : fpx(4.0f)));
        }

        const float norm    = (cents + 50.0f) / 100.0f;
        const float markerX = meter.getX() + norm * meter.getWidth();
        juce::Colour mk = juce::Colour(0xffff5555);
        if (std::abs(cents) < 10.0f) mk = juce::Colour(0xffffc840);
        if (std::abs(cents) <  4.0f) mk = juce::Colour(0xff55f090);

        g.setColour(mk.withAlpha(0.22f));
        g.fillRoundedRectangle(markerX - fpx(4.0f), meter.getY() - 1.0f,
                               fpx(8.0f), meter.getHeight() + 2.0f, fpx(4.0f));
        g.setColour(mk);
        g.fillRoundedRectangle(markerX - fpx(2.0f), meter.getY(),
                               fpx(4.0f), meter.getHeight(), fpx(2.0f));
    }

    // ── Meters ───────────────────────────────────────────────────────────────
    drawMeter(g, inputMeterBounds,  audioProcessor.getInputMeterLevel(),  "IN");
    drawMeter(g, outputMeterBounds, audioProcessor.getOutputMeterLevel(), "OUT");

    if (!correlationMeterBounds.isEmpty())
    {
        auto r = correlationMeterBounds;
        g.setColour(juce::Colour(0xff1c2030));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        auto inner = r.reduced(3);
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRect(inner);

        const float corr    = juce::jlimit(-1.0f, 1.0f, audioProcessor.correlationValue.load());
        const int   centerY = inner.getCentreY();
        const int   barLen  = static_cast<int>(std::round((inner.getHeight() / 2) * std::abs(corr)));

        juce::Colour cc = juce::Colours::yellow;
        if (corr >  0.3f) cc = juce::Colours::limegreen;
        else if (corr < -0.3f) cc = juce::Colours::orangered;

        g.setColour(cc);
        if (corr >= 0.0f)
            g.fillRect(inner.withY(centerY - barLen).withHeight(barLen));
        else
            g.fillRect(inner.withY(centerY).withHeight(barLen));

        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.drawHorizontalLine(centerY, static_cast<float>(inner.getX()), static_cast<float>(inner.getRight()));
        g.setFont(9.5f);
        g.setColour(juce::Colour(0xff607090));
        g.drawFittedText("CORR", r.withY(r.getBottom() - 15).withHeight(13),
                         juce::Justification::centred, 1);
    }
}

void VayuAudioProcessorEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();
    // Scale factor: 1.0 at medium (760 wide), scales smoothly with window width.
    const float sf = static_cast<float>(W) / 760.0f;
    // px() – scale a nominal pixel value, never below minimum.
    auto px = [&](int v, int mn = 4) { return juce::jmax(mn, static_cast<int>(v * sf)); };

    auto bounds = getLocalBounds().reduced(px(10, 2), px(8, 2));

    // ═══════════════════════════════════════════════════════════════════
    //  HEADER ROW
    // ═══════════════════════════════════════════════════════════════════
    const int headerH = juce::jmax(36, static_cast<int>(44.0f * (static_cast<float>(H) / 520.0f)));
    auto header = bounds.removeFromTop(headerH);

    const int tabW   = px(88, 50);
    const int tabGap = px(5, 3);
    ampTabButton.setBounds(header.removeFromLeft(tabW));
    header.removeFromLeft(tabGap);
    fxTabButton.setBounds(header.removeFromLeft(tabW));
    header.removeFromLeft(tabGap);
    toolsTabButton.setBounds(header.removeFromLeft(tabW));

    // Right side of header: size selector
    uiSizeCombo.setBounds(header.removeFromRight(px(106, 70)));
    header.removeFromRight(px(4, 2));
    uiSizeLabel.setBounds(header.removeFromRight(px(36, 28)));
    header.removeFromRight(px(8, 4));

    // Audio settings button (always visible, standalone only)
    audioSettingsButton.setBounds(header.removeFromRight(px(108, 70)));
    header.removeFromRight(px(8, 4));

    bounds.removeFromTop(4);

    // ═══════════════════════════════════════════════════════════════════
    //  CONTEXT ROW  – each tab's controls placed via copies of the same row.
    //  Controls from different tabs occupy the same screen area; visibility
    //  is handled by updateTabVisibility(), so only one set shows at a time.
    // ═══════════════════════════════════════════════════════════════════
    const int ctxH = juce::jmax(24, static_cast<int>(28.0f * sf));
    auto ctxBase = bounds.removeFromTop(ctxH);

    // --- FX tab context: IR load/clear + oversampling + delay ---
    {
        auto r = ctxBase;
        const int ibW = px(80, 44);
        loadIrAButton.setBounds (r.removeFromLeft(ibW)); r.removeFromLeft(px(4,2));
        clearIrAButton.setBounds(r.removeFromLeft(ibW)); r.removeFromLeft(px(5,3));
        loadIrBButton.setBounds (r.removeFromLeft(ibW)); r.removeFromLeft(px(4,2));
        clearIrBButton.setBounds(r.removeFromLeft(ibW)); r.removeFromLeft(px(5,3));
        if (W >= 580)
        {
            autoAlignCabButton.setBounds(r.removeFromLeft(px(96, 60)));
            r.removeFromLeft(px(7,3));
        }
        oversamplingLabel.setBounds(r.removeFromLeft(px(78, 44)));
        oversamplingCombo.setBounds(r.removeFromLeft(px(96, 60)));
        r.removeFromLeft(px(6,3));
        delaySyncToggle.setBounds  (r.removeFromLeft(px(92, 60)));
        r.removeFromLeft(px(5,3));
        delayDivisionLabel.setBounds(r.removeFromLeft(px(56, 32)));
        delayDivisionCombo.setBounds(r.removeFromLeft(px(78, 50)));
    }

    // --- AMP tab context: amp type combo ---
    {
        auto r = ctxBase;
        ampTypeLabel.setBounds(r.removeFromLeft(px(66, 40)));
        ampTypeCombo.setBounds(r.removeFromLeft(px(102, 64)));
    }

    // --- TOOLS tab context: save/load preset ---
    {
        auto r = ctxBase;
        savePresetButton.setBounds(r.removeFromLeft(px(112, 70)));
        r.removeFromLeft(px(5,3));
        loadPresetButton.setBounds(r.removeFromLeft(px(112, 70)));
    }

    bounds.removeFromTop(3);

    // ═══════════════════════════════════════════════════════════════════
    //  STATUS ROW  (FX tab: IR labels; shared space for AMP/Tools blanks)
    // ═══════════════════════════════════════════════════════════════════
    const int statusH = juce::jmax(18, static_cast<int>(22.0f * sf));
    {
        auto r = bounds.removeFromTop(statusH);
        irStatusLabel.setBounds       (r.removeFromLeft(px(178, 100))); r.removeFromLeft(px(5,3));
        irStatusALabel.setBounds      (r.removeFromLeft(px(205, 110))); r.removeFromLeft(px(5,3));
        irStatusBLabel.setBounds      (r.removeFromLeft(px(205, 110))); r.removeFromLeft(px(5,3));
        cabAlignStatusLabel.setBounds (r.removeFromLeft(px(128, 80)));
    }
    bounds.removeFromTop(5);

    // ═══════════════════════════════════════════════════════════════════
    //  SIDE METERS  (right column, spans below header to below knob grid)
    // ═══════════════════════════════════════════════════════════════════
    const int mW  = juce::jmax(18, px(25));
    const int mH  = juce::jmin(static_cast<int>(H * 0.43f), 220);
    {
        auto mc = bounds.removeFromRight(mW * 3 + px(14, 8));
        mc.removeFromTop(4);
        inputMeterBounds    = mc.removeFromLeft(mW).removeFromTop(mH);
        mc.removeFromLeft(px(4,2));
        outputMeterBounds   = mc.removeFromLeft(mW).removeFromTop(mH);
        mc.removeFromLeft(px(4,2));
        correlationMeterBounds = mc.removeFromLeft(juce::jmax(14, mW - 4)).removeFromTop(mH);
    }
    bounds.removeFromRight(px(4,2));

    // ═══════════════════════════════════════════════════════════════════
    //  DUAL-PURPOSE ROW  (AMP: gate advanced | FX: cab level sliders)
    // ═══════════════════════════════════════════════════════════════════
    const int dualH = juce::jmax(20, static_cast<int>(24.0f * sf));
    auto dualBase = bounds.removeFromTop(dualH);

    // AMP: gate advanced sliders
    {
        auto r = dualBase;
        const int gsW = juce::jmin(px(158, 100), (r.getWidth() - px(15,6)) / 4);
        gateAttackSlider.setBounds   (r.removeFromLeft(gsW)); r.removeFromLeft(px(4,2));
        gateReleaseSlider.setBounds  (r.removeFromLeft(gsW)); r.removeFromLeft(px(4,2));
        gateHysteresisSlider.setBounds(r.removeFromLeft(gsW));r.removeFromLeft(px(4,2));
        gateRangeSlider.setBounds    (r.removeFromLeft(gsW));
    }

    // FX: IR status + cab level sliders (same row via copy)
    {
        auto r = dualBase;
        irStatusALabel.setBounds (r.removeFromLeft(px(190, 110)));
        cabLevelASlider.setBounds(r.removeFromLeft(px(178, 100)));
        r.removeFromLeft(px(6,3));
        irStatusBLabel.setBounds (r.removeFromLeft(px(190, 110)));
        cabLevelBSlider.setBounds(r.removeFromLeft(px(178, 100)));
    }

    bounds.removeFromTop(6);

    // ═══════════════════════════════════════════════════════════════════
    //  CONTENT AREA  – each tab gets the full remaining space
    // ═══════════════════════════════════════════════════════════════════
    auto contentArea = bounds;

    // Common helpers
    const int gapX  = px(15, 8);
    const int gapY  = px(14, 8);
    const int bH    = juce::jmax(26, static_cast<int>(30.0f * sf));
    const int bH2   = juce::jmax(22, static_cast<int>(26.0f * sf));
    auto P = [&](int v, int mn = 10) { return juce::jmax(mn, px(v)); };

    // ─── AMP TAB ──────────────────────────────────────────────────────
    if (currentTab == UiTab::amp)
    {
        const int cols  = 4;
        const int rows  = 2;
        const int totalGapX = gapX * (cols - 1);
        const int totalGapY = gapY * (rows - 1);
        const int gridH = juce::jmax(200, static_cast<int>(contentArea.getHeight() * 0.54f));
        auto kArea = contentArea.removeFromTop(gridH);
        const int cellW = (kArea.getWidth()  - totalGapX) / cols;
        const int cellH = (kArea.getHeight() - totalGapY) / rows;
        const int inX = juce::jmax(8, cellW / 8);
        const int inY = juce::jmax(8, cellH / 8);

        auto setK = [&](juce::Slider& s, int c, int r)
        {
            s.setBounds(kArea.getX() + c*(cellW+gapX) + inX,
                        kArea.getY() + r*(cellH+gapY) + inY,
                        cellW - inX*2, cellH - inY*2);
        };
        setK(gateSlider,    0, 0); setK(boostSlider,    1, 0);
        setK(driveSlider,   2, 0); setK(volumeSlider,   3, 0);
        setK(bassSlider,    0, 1); setK(midSlider,       1, 1);
        setK(trebleSlider,  2, 1); setK(presenceSlider, 3, 1);

        // Gate advanced – slim row below knob grid
        contentArea.removeFromTop(6);
        auto gRow = contentArea.removeFromTop(bH2);
        const int gsW = juce::jmin(P(155, 100), (gRow.getWidth() - P(18, 6)) / 4);
        gateAttackSlider.setBounds    (gRow.removeFromLeft(gsW)); gRow.removeFromLeft(P(4,2));
        gateReleaseSlider.setBounds   (gRow.removeFromLeft(gsW)); gRow.removeFromLeft(P(4,2));
        gateHysteresisSlider.setBounds(gRow.removeFromLeft(gsW)); gRow.removeFromLeft(P(4,2));
        gateRangeSlider.setBounds     (gRow.removeFromLeft(gsW));

        // Tuner area – bottom of contentArea
        contentArea.removeFromTop(8);
        tunerToggle.setBounds    (contentArea.removeFromTop(26));
        contentArea.removeFromTop(5);
        tunerNoteLabel.setBounds (contentArea.removeFromTop(juce::jmax(34, px(44))));
        tunerDetailLabel.setBounds(contentArea.removeFromTop(juce::jmax(18, px(24))));
    }

    // ─── FX TAB ───────────────────────────────────────────────────────
    else if (currentTab == UiTab::fx)
    {
        const int cols  = 4;
        const int rows  = 4;
        const int totalGapX = gapX * (cols - 1);
        const int totalGapY = gapY * (rows - 1);
        auto kArea = contentArea; // use full content area
        const int cellW = (kArea.getWidth()  - totalGapX) / cols;
        const int cellH = (kArea.getHeight() - totalGapY) / rows;
        const int inX = juce::jmax(8, cellW / 8);
        const int inY = juce::jmax(8, cellH / 8);

        auto setK = [&](juce::Slider& s, int c, int r)
        {
            s.setBounds(kArea.getX() + c*(cellW+gapX) + inX,
                        kArea.getY() + r*(cellH+gapY) + inY,
                        cellW - inX*2, cellH - inY*2);
        };
        // Row 0: IR chain
        setK(irLowCutSlider,       0, 0); setK(irHighCutSlider,     1, 0);
        setK(irLevelSlider,        2, 0); setK(cabPanSlider,         3, 0);
        // Row 1: Cab + sends
        setK(cabBlendSlider,       0, 1); setK(delayTimeSlider,     1, 1);
        setK(delayMixSlider,       2, 1); setK(reverbMixSlider,     3, 1);
        // Row 2: Reverb
        setK(reverbRoomSizeSlider, 0, 2); setK(reverbDampingSlider, 1, 2);
        setK(reverbWidthSlider,    2, 2); setK(reverbPreDelaySlider, 3, 2);
        // Row 3: Delay detail
        setK(delayFeedbackSlider,  0, 3); setK(delayModRateSlider,  1, 3);
        setK(delayModDepthSlider,  2, 3);

        // Cab level sliders (in the dual-purpose row above — already set above)
        // IR Phase / Flip toggles inside inY gap of row 0 col 2-3 area
        const int tX2  = kArea.getX() + 2*(cellW+gapX);
        const int tX3  = kArea.getX() + 3*(cellW+gapX);
        const int tTogH = juce::jmax(18, inY - 2);
        irPhaseToggle.setBounds (tX2, kArea.getY() + 2,        cellW, tTogH);
        cabFlipAButton.setBounds(tX2, kArea.getY() + 2 + inY,  cellW, tTogH);
        cabFlipBButton.setBounds(tX3, kArea.getY() + 2 + inY,  cellW, tTogH);
    }

    // ─── TOOLS TAB ────────────────────────────────────────────────────
    else // currentTab == UiTab::tools
    {
        auto T = contentArea;

        // Row 1: Snapshots + undo/redo
        {
            auto r = T.removeFromTop(bH);
            captureAButton.setBounds  (r.removeFromLeft(P(88)));  r.removeFromLeft(P(6,3));
            captureBButton.setBounds  (r.removeFromLeft(P(88)));  r.removeFromLeft(P(6,3));
            compareABToggle.setBounds (r.removeFromLeft(P(96)));  r.removeFromLeft(P(6,3));
            undoButton.setBounds      (r.removeFromLeft(P(76)));  r.removeFromLeft(P(6,3));
            redoButton.setBounds      (r.removeFromLeft(P(76)));
        }
        T.removeFromTop(7);

        // Row 2: MIDI
        {
            auto r = T.removeFromTop(bH);
            midiParamCombo.setBounds (r.removeFromLeft(P(168))); r.removeFromLeft(P(8,4));
            midiLearnButton.setBounds(r.removeFromLeft(P(110)));
        }
        T.removeFromTop(7);

        // Row 3: Background + NAM file controls
        {
            auto r = T.removeFromTop(bH);
            loadBackgroundButton.setBounds (r.removeFromLeft(P(130))); r.removeFromLeft(P(6,3));
            clearBackgroundButton.setBounds(r.removeFromLeft(P(130))); r.removeFromLeft(P(6,3));
            loadNamButton.setBounds  (r.removeFromLeft(P(108))); r.removeFromLeft(P(6,3));
            clearNamButton.setBounds (r.removeFromLeft(P(108)));
        }
        T.removeFromTop(7);

        // Row 4: Metal + Pitch + Tight
        {
            auto r = T.removeFromTop(bH2);
            metalModeToggle.setBounds  (r.removeFromLeft(P(100))); r.removeFromLeft(P(8,4));
            pitchShiftLabel.setBounds  (r.removeFromLeft(P(45)));
            pitchShiftSlider.setBounds (r.removeFromLeft(P(118))); r.removeFromLeft(P(10,5));
            tightLowCutLabel.setBounds (r.removeFromLeft(P(58)));
            tightLowCutSlider.setBounds(r.removeFromLeft(P(118)));
        }
        T.removeFromTop(5);

        // Row 5: Wah
        {
            auto r = T.removeFromTop(bH2);
            wahEnableToggle.setBounds (r.removeFromLeft(P(68)));  r.removeFromLeft(P(6,3));
            wahAutoToggle.setBounds   (r.removeFromLeft(P(84)));  r.removeFromLeft(P(8,4));
            wahCenterLabel.setBounds  (r.removeFromLeft(P(52)));
            wahCenterSlider.setBounds (r.removeFromLeft(P(114))); r.removeFromLeft(P(10,5));
            wahDepthLabel.setBounds   (r.removeFromLeft(P(56)));
            wahDepthSlider.setBounds  (r.removeFromLeft(P(114)));
        }
        T.removeFromTop(5);

        // Row 6: Kill switch
        {
            auto r = T.removeFromTop(bH2);
            killSwitchToggle.setBounds(r.removeFromLeft(P(90)));  r.removeFromLeft(P(8,4));
            killRateLabel.setBounds   (r.removeFromLeft(P(56)));
            killRateCombo.setBounds   (r.removeFromLeft(P(88)));  r.removeFromLeft(P(8,4));
            killDepthLabel.setBounds  (r.removeFromLeft(P(58)));
            killDepthSlider.setBounds (r.removeFromLeft(P(118)));
        }
        T.removeFromTop(7);

        // NAM
        namBypassToggle.setBounds   (T.removeFromTop(bH2)); T.removeFromTop(4);
        namAutoMatchToggle.setBounds(T.removeFromTop(bH2)); T.removeFromTop(4);
        {
            auto r = T.removeFromTop(bH2);
            namBlendLabel.setBounds (r.removeFromLeft(P(80)));
            namBlendSlider.setBounds(r.removeFromLeft(P(200)));
        }
        T.removeFromTop(8);

        // I/O
        {
            auto r = T.removeFromTop(bH2);
            inputTrimLabel.setBounds    (r.removeFromLeft(P(68)));
            inputTrimSlider.setBounds   (r.removeFromLeft(P(140))); r.removeFromLeft(P(10,5));
            limiterEnabledToggle.setBounds(r.removeFromLeft(P(80))); r.removeFromLeft(P(6,3));
            limiterClipLabel.setBounds  (r.removeFromLeft(38));      r.removeFromLeft(P(4,2));
            limiterThreshLabel.setBounds(r.removeFromLeft(P(76)));
            limiterThreshSlider.setBounds(r.removeFromLeft(P(120)));
        }
        T.removeFromTop(8);

        // Spectrum + Preset browser
        spectrumAnalyzer->setBounds(T.removeFromTop(juce::jmax(52, static_cast<int>(H / 9.0f))));
        T.removeFromTop(6);
        presetBrowserLabel.setBounds(T.removeFromTop(20));
        T.removeFromTop(4);
        {
            const int listH = juce::jmax(72, static_cast<int>(H / 7.0f));
            auto la = T.removeFromTop(listH);
            loadFactoryPresetButton.setBounds(la.removeFromBottom(bH));
            presetListBox.setBounds(la);
        }
        T.removeFromTop(6);

        // Status labels
        midiMapStatusLabel.setBounds    (T.removeFromTop(bH2)); T.removeFromTop(4);
        backgroundStatusLabel.setBounds (T.removeFromTop(bH2)); T.removeFromTop(4);
        namStatusLabel.setBounds        (T.removeFromTop(bH2)); T.removeFromTop(4);
        namMatchLabel.setBounds         (T.removeFromTop(bH2));
    }
}

void VayuAudioProcessorEditor::chooseCabinetIR()
{
    chooseCabinetIRSlot(0);
}

void VayuAudioProcessorEditor::chooseCabinetIRSlot(int slotIndex)
{
    irChooser = std::make_unique<juce::FileChooser>(
        slotIndex == 0 ? "Choose cabinet IR for Slot A" : "Choose cabinet IR for Slot B",
        juce::File(),
        "*.wav");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    irChooser->launchAsync(chooserFlags, [this, slotIndex](const juce::FileChooser& chooser)
    {
        const auto selected = chooser.getResult();

        if (!selected.existsAsFile())
            return;

        if (audioProcessor.loadCabinetIRSlot(slotIndex, selected))
            refreshIrStatus();
    });
}

void VayuAudioProcessorEditor::refreshIrStatus()
{
    irStatusLabel.setText("Cab Section", juce::dontSendNotification);
    irStatusALabel.setText("A: " + audioProcessor.getCurrentIRNameForSlot(0), juce::dontSendNotification);
    irStatusBLabel.setText("B: " + audioProcessor.getCurrentIRNameForSlot(1), juce::dontSendNotification);

    if (audioProcessor.canAutoAlignCab())
    {
        const auto corr = audioProcessor.getLastCabAlignCorrelation();
        const auto delayMs = audioProcessor.getCabAlignDelayMs();
        const juce::String corrSign = corr >= 0.0f ? "+" : "";
        const juce::String delaySign = delayMs >= 0.0f ? "+" : "";
        cabAlignStatusLabel.setText("Align Corr: " + corrSign + juce::String(corr, 2)
                                        + " | B Delay: " + delaySign + juce::String(delayMs, 2) + " ms",
                                    juce::dontSendNotification);
        autoAlignCabButton.setEnabled(true);
    }
    else
    {
        cabAlignStatusLabel.setText("Align: Load IR A and B", juce::dontSendNotification);
        autoAlignCabButton.setEnabled(false);
    }
}

void VayuAudioProcessorEditor::refreshTunerStatus()
{
    const auto note = audioProcessor.getTunerNoteName();
    const auto frequency = audioProcessor.getTunerFrequencyHz();
    const auto cents = audioProcessor.getTunerCents();

    if (frequency <= 0.0f || note == "--")
    {
        tunerNoteLabel.setText("TUNER --", juce::dontSendNotification);
        tunerDetailLabel.setText("Play a note", juce::dontSendNotification);
        tunerNoteLabel.setColour(juce::Label::textColourId, juce::Colour(0xffc9d1de));
        tunerDetailLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaab5c8));
        return;
    }

    juce::Colour noteColour = juce::Colour(0xffff7a7a);
    if (std::abs(cents) < 10.0f)
        noteColour = juce::Colour(0xffffd56b);
    if (std::abs(cents) < 4.0f)
        noteColour = juce::Colour(0xff82f5ab);

    tunerNoteLabel.setText("TUNER " + note, juce::dontSendNotification);
    tunerDetailLabel.setText(juce::String(frequency, 2) + " Hz | " + juce::String(cents, 1) + " cents", juce::dontSendNotification);
    tunerNoteLabel.setColour(juce::Label::textColourId, noteColour);
    tunerDetailLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd7e0ef));
}

void VayuAudioProcessorEditor::refreshToolsStatus()
{
    midiMapStatusLabel.setText(audioProcessor.getMidiMappingDescription(), juce::dontSendNotification);
    undoButton.setEnabled(audioProcessor.canUndo());
    redoButton.setEnabled(audioProcessor.canRedo());

    if (audioProcessor.getBackgroundImagePath().isNotEmpty())
        backgroundStatusLabel.setText("Background: " + juce::File(audioProcessor.getBackgroundImagePath()).getFileName(), juce::dontSendNotification);
    else
        backgroundStatusLabel.setText("Background: Default", juce::dontSendNotification);

    const auto namPath = audioProcessor.getNamModelPath();
    const bool namBypassed = namBypassToggle.getToggleState();
    const bool namAutoMatch = namAutoMatchToggle.getToggleState();
    if (namPath.isNotEmpty())
        namStatusLabel.setText(namBypassed
                                   ? "NAM: Bypassed (" + juce::File(namPath).getFileName() + ")"
                                   : "NAM: " + juce::File(namPath).getFileName() + (namAutoMatch ? " [Match]" : "")
                                       + " [Blend " + juce::String(juce::roundToInt(namBlendSlider.getValue() * 100.0)) + "%]",
                               juce::dontSendNotification);
    else
        namStatusLabel.setText("NAM: Off", juce::dontSendNotification);

    if (namPath.isNotEmpty() && namAutoMatch && !namBypassed)
    {
        const auto matchDb = audioProcessor.getNamMatchGainDb();
        const juce::String sign = matchDb >= 0.0f ? "+" : "";
        namMatchLabel.setText("NAM Match: " + sign + juce::String(matchDb, 2) + " dB", juce::dontSendNotification);
    }
    else
    {
        namMatchLabel.setText("NAM Match: --", juce::dontSendNotification);
    }
}

void VayuAudioProcessorEditor::loadBackgroundImageFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    const auto loaded = juce::ImageFileFormat::loadFrom(file);
    if (!loaded.isValid())
        return;

    backgroundImage = loaded;
    audioProcessor.setBackgroundImagePath(file.getFullPathName());
    refreshToolsStatus();
    repaint();
}

void VayuAudioProcessorEditor::chooseBackgroundImage()
{
    backgroundChooser = std::make_unique<juce::FileChooser>(
        "Choose background image",
        juce::File(),
        "*.png;*.jpg;*.jpeg;*.webp");

    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    backgroundChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file.existsAsFile())
            loadBackgroundImageFromFile(file);
    });
}

void VayuAudioProcessorEditor::chooseNamModel()
{
    namChooser = std::make_unique<juce::FileChooser>("Choose NAM model", juce::File(), "*.nam");

    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    namChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (!file.existsAsFile())
            return;

        audioProcessor.loadNamModel(file.getFullPathName());
        refreshToolsStatus();
    });
}

void VayuAudioProcessorEditor::clearBackgroundImage()
{
    backgroundImage = {};
    audioProcessor.setBackgroundImagePath({});
    refreshToolsStatus();
    repaint();
}

void VayuAudioProcessorEditor::savePresetToFile()
{
    auto nameDialog = std::make_unique<juce::AlertWindow>("Save Preset",
                                                           "Enter preset name",
                                                           juce::MessageBoxIconType::NoIcon);
    nameDialog->addTextEditor("presetName", "MyPreset", "Name:");
    nameDialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    nameDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    auto* dialogPtr = nameDialog.release();
    dialogPtr->enterModalState(true, juce::ModalCallbackFunction::create([this, dialogPtr](int result)
    {
        std::unique_ptr<juce::AlertWindow> dialogOwner(dialogPtr);
        if (result != 1)
            return;

        juce::String presetName = dialogOwner->getTextEditorContents("presetName").trim();
        if (presetName.isEmpty())
            presetName = "MyPreset";

        juce::File defaultFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                     .getChildFile(presetName)
                                     .withFileExtension(".amnesia");

        presetChooser = std::make_unique<juce::FileChooser>("Save preset", defaultFile, "*.amnesia");
        const auto chooserFlags = juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles
                                | juce::FileBrowserComponent::warnAboutOverwriting;

        presetChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file == juce::File())
                return;

            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension(".amnesia");

            juce::MemoryBlock data;
            audioProcessor.getStateInformation(data);
            file.replaceWithData(data.getData(), data.getSize());
        });
    }), true);
}

void VayuAudioProcessorEditor::loadPresetFromFile()
{
    presetChooser = std::make_unique<juce::FileChooser>("Load preset", juce::File(), "*.amnesia");
    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    presetChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (!file.existsAsFile())
            return;

        juce::MemoryBlock data;
        if (file.loadFileAsData(data))
        {
            audioProcessor.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
            refreshIrStatus();
            refreshToolsStatus();
        }
    });
}

void VayuAudioProcessorEditor::captureSnapshotA()
{
    audioProcessor.getStateInformation(snapshotA);
}

void VayuAudioProcessorEditor::captureSnapshotB()
{
    audioProcessor.getStateInformation(snapshotB);
}

void VayuAudioProcessorEditor::recallSnapshotAorB()
{
    const auto& chosen = compareABToggle.getToggleState() ? snapshotB : snapshotA;
    if (chosen.getSize() == 0)
        return;

    audioProcessor.setStateInformation(chosen.getData(), static_cast<int>(chosen.getSize()));
    refreshIrStatus();
    refreshToolsStatus();
}

void VayuAudioProcessorEditor::switchToTab(UiTab tab)
{
    currentTab = tab;
    updateTabVisibility();
    resized(); // re-layout for active tab content area
}

void VayuAudioProcessorEditor::applyEditorSizePreset(UiScale preset)
{
    currentScale = preset;

    if (preset == UiScale::small)
        setSize(800, 580);
    else if (preset == UiScale::large)
        setSize(1100, 760);
    else if (preset == UiScale::xlarge)
        setSize(1320, 900);
    else
        setSize(960, 660);
}

void VayuAudioProcessorEditor::updateTabVisibility()
{
    const bool showAmp = currentTab == UiTab::amp;
    const bool showFx = currentTab == UiTab::fx;
    const bool showTools = currentTab == UiTab::tools;
    const bool showTuner = showAmp && tunerToggle.getToggleState();

    driveSlider.setVisible(showAmp);
    volumeSlider.setVisible(showAmp);
    gateSlider.setVisible(showAmp);
    gateAttackSlider.setVisible(showAmp);
    gateReleaseSlider.setVisible(showAmp);
    gateHysteresisSlider.setVisible(showAmp);
    gateRangeSlider.setVisible(showAmp);
    boostSlider.setVisible(showAmp);
    bassSlider.setVisible(showAmp);
    midSlider.setVisible(showAmp);
    trebleSlider.setVisible(showAmp);
    presenceSlider.setVisible(showAmp);
    driveLabel.setVisible(showAmp);
    volumeLabel.setVisible(showAmp);
    gateLabel.setVisible(showAmp);
    gateAttackLabel.setVisible(showAmp);
    gateReleaseLabel.setVisible(showAmp);
    gateHysteresisLabel.setVisible(showAmp);
    gateRangeLabel.setVisible(showAmp);
    boostLabel.setVisible(showAmp);
    bassLabel.setVisible(showAmp);
    midLabel.setVisible(showAmp);
    trebleLabel.setVisible(showAmp);
    presenceLabel.setVisible(showAmp);
    ampTypeLabel.setVisible(showAmp);
    ampTypeCombo.setVisible(showAmp);
    tunerToggle.setVisible(showAmp);
    tunerNoteLabel.setVisible(showTuner);
    tunerDetailLabel.setVisible(showTuner);

    delayTimeSlider.setVisible(showFx);
    delayFeedbackSlider.setVisible(showFx);
    delayModRateSlider.setVisible(showFx);
    delayModDepthSlider.setVisible(showFx);
    delayMixSlider.setVisible(showFx);
    reverbMixSlider.setVisible(showFx);
    reverbRoomSizeSlider.setVisible(showFx);
    reverbDampingSlider.setVisible(showFx);
    reverbWidthSlider.setVisible(showFx);
    reverbPreDelaySlider.setVisible(showFx);
    irLowCutSlider.setVisible(showFx);
    irHighCutSlider.setVisible(showFx);
    irLevelSlider.setVisible(showFx);
    cabBlendSlider.setVisible(showFx);
    cabPanSlider.setVisible(showFx);
    cabLevelASlider.setVisible(showFx);
    cabLevelBSlider.setVisible(showFx);
    delayTimeLabel.setVisible(showFx);
    delayFeedbackLabel.setVisible(showFx);
    delayModRateLabel.setVisible(showFx);
    delayModDepthLabel.setVisible(showFx);
    delayMixLabel.setVisible(showFx);
    reverbMixLabel.setVisible(showFx);
    reverbRoomSizeLabel.setVisible(showFx);
    reverbDampingLabel.setVisible(showFx);
    reverbWidthLabel.setVisible(showFx);
    reverbPreDelayLabel.setVisible(showFx);
    irLowCutLabel.setVisible(showFx);
    irHighCutLabel.setVisible(showFx);
    irLevelLabel.setVisible(showFx);
    cabBlendLabel.setVisible(showFx);
    cabPanLabel.setVisible(showFx);
    cabLevelALabel.setVisible(showFx);
    cabLevelBLabel.setVisible(showFx);
    loadIrButton.setVisible(false);
    clearIrButton.setVisible(false);
    loadIrAButton.setVisible(showFx);
    clearIrAButton.setVisible(showFx);
    loadIrBButton.setVisible(showFx);
    clearIrBButton.setVisible(showFx);
    autoAlignCabButton.setVisible(showFx);
    irPhaseToggle.setVisible(showFx);
    cabFlipAButton.setVisible(showFx);
    cabFlipBButton.setVisible(showFx);
    irStatusLabel.setVisible(showFx);
    irStatusALabel.setVisible(showFx);
    irStatusBLabel.setVisible(showFx);
    cabAlignStatusLabel.setVisible(showFx);
    oversamplingLabel.setVisible(showFx);
    oversamplingCombo.setVisible(showFx);
    delaySyncToggle.setVisible(showFx);
    delayDivisionLabel.setVisible(showFx);
    delayDivisionCombo.setVisible(showFx);
    limiterClipLabel.setVisible(showTools);

    savePresetButton.setVisible(showTools);
    loadPresetButton.setVisible(showTools);
    captureAButton.setVisible(showTools);
    captureBButton.setVisible(showTools);
    loadBackgroundButton.setVisible(showTools);
    clearBackgroundButton.setVisible(showTools);
    loadNamButton.setVisible(showTools);
    clearNamButton.setVisible(showTools);
    namBypassToggle.setVisible(showTools);
    namAutoMatchToggle.setVisible(showTools);
    compareABToggle.setVisible(showTools);
    undoButton.setVisible(showTools);
    redoButton.setVisible(showTools);
    midiParamCombo.setVisible(showTools);
    midiLearnButton.setVisible(showTools);
    midiMapStatusLabel.setVisible(showTools);
    backgroundStatusLabel.setVisible(showTools);
    namStatusLabel.setVisible(showTools);
    namMatchLabel.setVisible(showTools);
    namBlendLabel.setVisible(showTools);
    namBlendSlider.setVisible(showTools);
    metalModeToggle.setVisible(showTools);
    pitchShiftSlider.setVisible(showTools);
    pitchShiftLabel.setVisible(showTools);
    tightLowCutSlider.setVisible(showTools);
    tightLowCutLabel.setVisible(showTools);
    wahEnableToggle.setVisible(showTools);
    wahAutoToggle.setVisible(showTools);
    wahCenterSlider.setVisible(showTools);
    wahCenterLabel.setVisible(showTools);
    wahDepthSlider.setVisible(showTools);
    wahDepthLabel.setVisible(showTools);
    killSwitchToggle.setVisible(showTools);
    killRateLabel.setVisible(showTools);
    killRateCombo.setVisible(showTools);
    killDepthSlider.setVisible(showTools);
    killDepthLabel.setVisible(showTools);
    inputTrimSlider.setVisible(showTools);
    inputTrimLabel.setVisible(showTools);
    limiterThreshSlider.setVisible(showTools);
    limiterThreshLabel.setVisible(showTools);
    limiterEnabledToggle.setVisible(showTools);
    presetListBox.setVisible(showTools);
    presetBrowserLabel.setVisible(showTools);
    loadFactoryPresetButton.setVisible(showTools);
    spectrumAnalyzer->setVisible(showTools);

    // Tabs: active tab has a slightly brighter tint; the cyan underline
    // is drawn by paint() so we only need the background color here.
    const auto tabOnBg   = juce::Colour(0xff1e2530);
    const auto tabOffBg  = juce::Colour(0xff111418);
    const auto tabOnTxt  = juce::Colours::white;
    const auto tabOffTxt = juce::Colour(0xff7a8898);

    auto styleTab = [&](juce::TextButton& btn, bool active)
    {
        btn.setColour(juce::TextButton::buttonColourId,  active ? tabOnBg  : tabOffBg);
        btn.setColour(juce::TextButton::textColourOffId, active ? tabOnTxt : tabOffTxt);
    };
    styleTab(ampTabButton,   showAmp);
    styleTab(fxTabButton,    showFx);
    styleTab(toolsTabButton, showTools);
}

void VayuAudioProcessorEditor::timerCallback()
{
    uiAnimationPhase += 0.10f;
    if (uiAnimationPhase > juce::MathConstants<float>::twoPi)
        uiAnimationPhase -= juce::MathConstants<float>::twoPi;

    refreshTunerStatus();
    refreshToolsStatus();
    const bool limiterOn = audioProcessor.limiterActive.load();
    limiterClipLabel.setText(limiterOn ? "CLIP" : "", juce::dontSendNotification);
    limiterClipLabel.setColour(juce::Label::textColourId, limiterOn ? juce::Colours::red : juce::Colours::transparentBlack);
    
    repaint(inputMeterBounds.getUnion(outputMeterBounds).getUnion(correlationMeterBounds));
    repaint(getLocalBounds().removeFromTop(56));

    // Feed spectrum analyzer from processor FIFO
    if (spectrumAnalyzer->isVisible())
    {
        static std::array<float, VayuAudioProcessor::kSpecFifoSize> tmpBuf;
        if (audioProcessor.consumeSpectrumBlock(tmpBuf.data()))
            spectrumAnalyzer->pushSamples(tmpBuf.data(), VayuAudioProcessor::kSpecFifoSize);
    }
}

void VayuAudioProcessorEditor::drawMeter(juce::Graphics& g,
                                             juce::Rectangle<int> bounds,
                                             float linearLevel,
                                             const juce::String& label) const
{
    if (bounds.isEmpty())
        return;

    const auto frame = bounds;
    auto fillArea = bounds.reduced(4);

    g.setColour(juce::Colours::dimgrey);
    g.fillRoundedRectangle(frame.toFloat(), 4.0f);

    const float clamped = juce::jlimit(0.0f, 1.0f, linearLevel);
    const int fillHeight = static_cast<int>(std::round(fillArea.getHeight() * clamped));
    auto active = fillArea.removeFromBottom(fillHeight);

    const int greenStart = fillArea.getY() + static_cast<int>(fillArea.getHeight() * 0.55f);
    const int yellowStart = fillArea.getY() + static_cast<int>(fillArea.getHeight() * 0.25f);

    if (active.isEmpty())
    {
        g.setColour(juce::Colours::grey);
    }
    else if (active.getY() <= yellowStart)
    {
        g.setColour(juce::Colours::red);
    }
    else if (active.getY() <= greenStart)
    {
        g.setColour(juce::Colours::yellow);
    }
    else
    {
        g.setColour(juce::Colours::limegreen);
    }

    g.fillRoundedRectangle(active.toFloat(), 2.0f);

    g.setColour(juce::Colours::white);
    g.setFont(11.0f);
    g.drawFittedText(label, frame.withY(frame.getBottom() - 18).withHeight(16), juce::Justification::centred, 1);
}

void VayuAudioProcessorEditor::openAudioSettings()
{
#if JucePlugin_Build_Standalone
    if (audioProcessor.wrapperType != juce::AudioProcessor::wrapperType_Standalone)
        return;

    auto* standaloneWindow = dynamic_cast<juce::StandaloneFilterWindow*>(getTopLevelComponent());

    if (standaloneWindow == nullptr)
        return;

    auto settingsComponent = std::make_unique<juce::AudioDeviceSelectorComponent>(
        standaloneWindow->getDeviceManager(),
        0,
        juce::jmax(0, audioProcessor.getMainBusNumInputChannels()),
        0,
        juce::jmax(0, audioProcessor.getMainBusNumOutputChannels()),
        true,
        audioProcessor.producesMidi(),
        true,
        false);

    settingsComponent->setSize(520, 420);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(settingsComponent.release());
    options.dialogTitle = "Audio Settings";
    options.componentToCentreAround = this;
    options.dialogBackgroundColour = findColour(juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
#endif
}
