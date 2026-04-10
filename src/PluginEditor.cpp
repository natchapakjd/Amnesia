#include "PluginEditor.h"

#include <cmath>

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

void MyAmpSimAudioProcessorEditor::AmpLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                                                     int x,
                                                                     int y,
                                                                     int width,
                                                                     int height,
                                                                     float sliderPosProportional,
                                                                     float rotaryStartAngle,
                                                                     float rotaryEndAngle,
                                                                     juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)).reduced(8.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::ColourGradient ring(juce::Colour::fromRGB(90, 90, 95), centre.x, bounds.getY(), juce::Colour::fromRGB(30, 30, 32), centre.x, bounds.getBottom(), false);
    g.setGradientFill(ring);
    g.fillEllipse(bounds);

    g.setColour(juce::Colour::fromRGB(18, 18, 20));
    g.fillEllipse(bounds.reduced(radius * 0.16f));

    g.setColour(juce::Colour::fromRGB(165, 168, 175));
    g.drawEllipse(bounds.reduced(radius * 0.08f), 1.8f);

    juce::Path notch;
    notch.addRoundedRectangle(-1.8f, -radius * 0.72f, 3.6f, radius * 0.34f, 1.5f);
    g.setColour(juce::Colours::orange.withAlpha(0.95f));
    g.fillPath(notch, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));

    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawEllipse(bounds.translated(1.5f, 2.0f), 1.0f);
}

MyAmpSimAudioProcessorEditor::MyAmpSimAudioProcessorEditor(MyAmpSimAudioProcessor& processor)
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
    delayModDepthAttachment(audioProcessor.apvts, "delayModDepth", delayModDepthSlider)
{
    setLookAndFeel(&ampLookAndFeel);

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
    uiSizeCombo.setSelectedId(2);
    uiSizeCombo.onChange = [this]
    {
        const int selected = uiSizeCombo.getSelectedId();
        if (selected == 1)
            applyEditorSizePreset(UiScale::small);
        else if (selected == 3)
            applyEditorSizePreset(UiScale::large);
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

    irStatusLabel.setJustificationType(juce::Justification::centredLeft);
    irStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(irStatusLabel);

    irStatusALabel.setJustificationType(juce::Justification::centredLeft);
    irStatusALabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(irStatusALabel);

    irStatusBLabel.setJustificationType(juce::Justification::centredLeft);
    irStatusBLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(irStatusBLabel);

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
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
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

    limiterClipLabel.setText("", juce::dontSendNotification);
    limiterClipLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    limiterClipLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    limiterClipLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    limiterClipLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(limiterClipLabel);

    // Preset browser
    presetListModel.presets = MyAmpSimAudioProcessor::getFactoryPresets();
    presetListBox.setModel(&presetListModel);
    presetListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    presetListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff404040));
    presetListBox.setOutlineThickness(1);
    addAndMakeVisible(presetListBox);
    addAndMakeVisible(presetBrowserLabel);
    presetBrowserLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0a0a0));
    presetBrowserLabel.setFont(juce::Font(13.0f, juce::Font::bold));

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

MyAmpSimAudioProcessorEditor::~MyAmpSimAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void MyAmpSimAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto full = getLocalBounds();

    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage, full.toFloat(), juce::RectanglePlacement::fillDestination);
        g.setColour(juce::Colours::black.withAlpha(0.42f));
        g.fillRect(full);
    }
    else
    {
        juce::ColourGradient bg(juce::Colour::fromRGB(26, 28, 30), 0.0f, 0.0f, juce::Colour::fromRGB(12, 12, 14), 0.0f, static_cast<float>(full.getBottom()), false);
        g.setGradientFill(bg);
        g.fillRect(full);
    }

    g.setColour(juce::Colour::fromRGB(230, 230, 230));
    g.setFont(g.getCurrentFont().withHeight(24.0f).boldened());

    auto titleArea = full.removeFromTop(50);
    g.drawFittedText("MyAmpSim", titleArea, juce::Justification::centred, 1);

    drawMeter(g, inputMeterBounds, audioProcessor.getInputMeterLevel(), "IN");
    drawMeter(g, outputMeterBounds, audioProcessor.getOutputMeterLevel(), "OUT");
    
    if (!correlationMeterBounds.isEmpty())
    {
        auto r = correlationMeterBounds;
        g.setColour(juce::Colours::dimgrey);
        g.fillRoundedRectangle(r.toFloat(), 4.0f);

        auto inner = r.reduced(3);
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRect(inner);

        const float corr = juce::jlimit(-1.0f, 1.0f, audioProcessor.correlationValue.load());
        const int centerY = inner.getCentreY();
        const int halfH = inner.getHeight() / 2;
        const int barLen = static_cast<int>(std::round(halfH * std::abs(corr)));

        juce::Colour corrColour = juce::Colours::yellow;
        if (corr > 0.3f)
            corrColour = juce::Colours::limegreen;
        else if (corr < -0.3f)
            corrColour = juce::Colours::orangered;

        g.setColour(corrColour);
        if (corr >= 0.0f)
            g.fillRect(inner.withY(centerY - barLen).withHeight(barLen));
        else
            g.fillRect(inner.withY(centerY).withHeight(barLen));

        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.drawHorizontalLine(centerY, static_cast<float>(inner.getX()), static_cast<float>(inner.getRight()));
        g.setFont(10.0f);
        g.drawFittedText("CORR", r.withY(r.getBottom() - 16).withHeight(14), juce::Justification::centred, 1);
    }
}

void MyAmpSimAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(12);

    auto tabBar = bounds.removeFromTop(34);
    ampTabButton.setBounds(tabBar.removeFromLeft(90));
    tabBar.removeFromLeft(8);
    fxTabButton.setBounds(tabBar.removeFromLeft(90));
    tabBar.removeFromLeft(8);
    toolsTabButton.setBounds(tabBar.removeFromLeft(90));

    tabBar.removeFromRight(8);
    uiSizeCombo.setBounds(tabBar.removeFromRight(120));
    tabBar.removeFromRight(6);
    uiSizeLabel.setBounds(tabBar.removeFromRight(56));

    bounds.removeFromTop(6);

    auto topBar = bounds.removeFromTop(36);
    audioSettingsButton.setBounds(topBar.removeFromLeft(150));
    topBar.removeFromLeft(10);
    loadIrAButton.setBounds(topBar.removeFromLeft(90));
    topBar.removeFromLeft(6);
    clearIrAButton.setBounds(topBar.removeFromLeft(90));
    topBar.removeFromLeft(8);
    loadIrBButton.setBounds(topBar.removeFromLeft(90));
    topBar.removeFromLeft(6);
    clearIrBButton.setBounds(topBar.removeFromLeft(90));
    savePresetButton.setBounds(topBar.removeFromRight(130));
    topBar.removeFromRight(8);
    loadPresetButton.setBounds(topBar.removeFromRight(130));

    auto irBar = bounds.removeFromTop(28);
    irStatusLabel.setBounds(irBar.removeFromLeft(210));
    irBar.removeFromLeft(8);
    oversamplingLabel.setBounds(irBar.removeFromLeft(90));
    oversamplingCombo.setBounds(irBar.removeFromLeft(120));
    irBar.removeFromLeft(8);
    delaySyncToggle.setBounds(irBar.removeFromLeft(110));
    irBar.removeFromLeft(6);
    delayDivisionLabel.setBounds(irBar.removeFromLeft(70));
    delayDivisionCombo.setBounds(irBar.removeFromLeft(90));
    irBar.removeFromLeft(8);
    ampTypeLabel.setBounds(irBar.removeFromLeft(80));
    ampTypeCombo.setBounds(irBar.removeFromLeft(120));

    auto slotBar = bounds.removeFromTop(24);
    auto fxSlotBar = slotBar;
    irStatusALabel.setBounds(fxSlotBar.removeFromLeft(250));
    cabLevelASlider.setBounds(fxSlotBar.removeFromLeft(220));
    fxSlotBar.removeFromLeft(8);
    irStatusBLabel.setBounds(fxSlotBar.removeFromLeft(250));
    cabLevelBSlider.setBounds(fxSlotBar.removeFromLeft(220));

    auto ampGateBar = slotBar;
    gateAttackSlider.setBounds(ampGateBar.removeFromLeft(170));
    ampGateBar.removeFromLeft(8);
    gateReleaseSlider.setBounds(ampGateBar.removeFromLeft(170));
    ampGateBar.removeFromLeft(8);
    gateHysteresisSlider.setBounds(ampGateBar.removeFromLeft(170));
    ampGateBar.removeFromLeft(8);
    gateRangeSlider.setBounds(ampGateBar.removeFromLeft(170));

    bounds.removeFromTop(6);

    bounds.removeFromTop(8);

    auto metersArea = bounds.removeFromRight(110);
    metersArea.removeFromTop(6);

    auto meterTop = metersArea.removeFromTop(180);
    inputMeterBounds = meterTop.removeFromLeft(30);
    meterTop.removeFromLeft(10);
    outputMeterBounds = meterTop.removeFromLeft(30);
    meterTop.removeFromLeft(8);
    correlationMeterBounds = meterTop.removeFromLeft(24);

    const int columns = 4;
    const int rows = 4;
    const int knobGapX = 18;
    const int knobGapY = 14;

    auto knobGrid = bounds.removeFromTop(460);
    const int knobW = (knobGrid.getWidth() - knobGapX * (columns - 1)) / columns;
    const int knobH = (knobGrid.getHeight() - knobGapY * (rows - 1)) / rows;

    auto setKnob = [&](juce::Slider& slider, int col, int row)
    {
        const int x = knobGrid.getX() + col * (knobW + knobGapX);
        const int y = knobGrid.getY() + row * (knobH + knobGapY);
        slider.setBounds(x, y, knobW, knobH);
    };

    setKnob(gateSlider, 0, 0);
    setKnob(boostSlider, 1, 0);
    setKnob(driveSlider, 2, 0);
    setKnob(volumeSlider, 3, 0);
    setKnob(bassSlider, 0, 1);
    setKnob(midSlider, 1, 1);
    setKnob(trebleSlider, 2, 1);
    setKnob(presenceSlider, 3, 1);

    setKnob(delayTimeSlider, 1, 1);
    setKnob(delayMixSlider, 2, 1);
    setKnob(reverbMixSlider, 3, 1);
    setKnob(reverbRoomSizeSlider, 0, 2);
    setKnob(reverbDampingSlider, 1, 2);
    setKnob(reverbWidthSlider, 2, 2);
    setKnob(reverbPreDelaySlider, 3, 2);
    setKnob(delayFeedbackSlider, 1, 3);
    setKnob(delayModRateSlider, 2, 3);
    setKnob(delayModDepthSlider, 3, 3);
    setKnob(irLowCutSlider, 0, 0);
    setKnob(irHighCutSlider, 1, 0);
    setKnob(irLevelSlider, 2, 0);
    setKnob(cabPanSlider, 3, 0);
    setKnob(cabBlendSlider, 0, 1);

    irPhaseToggle.setBounds(knobGrid.getX() + 2 * (knobW + knobGapX), knobGrid.getY() + 8, knobW, 24);
    cabFlipAButton.setBounds(knobGrid.getX() + 2 * (knobW + knobGapX), knobGrid.getY() + 34, knobW, 24);
    cabFlipBButton.setBounds(knobGrid.getX() + 3 * (knobW + knobGapX), knobGrid.getY() + 34, knobW, 24);

    auto bottomArea = bounds;

    auto tunerArea = bottomArea.removeFromLeft(220);
    tunerToggle.setBounds(tunerArea.removeFromTop(24));
    tunerArea.removeFromTop(8);
    tunerNoteLabel.setBounds(tunerArea.removeFromTop(44));
    tunerDetailLabel.setBounds(tunerArea.removeFromTop(22));

    bottomArea.removeFromLeft(12);

    auto toolsArea = bottomArea;

    auto row1 = toolsArea.removeFromTop(30);
    captureAButton.setBounds(row1.removeFromLeft(88));
    row1.removeFromLeft(6);
    captureBButton.setBounds(row1.removeFromLeft(88));
    row1.removeFromLeft(6);
    compareABToggle.setBounds(row1.removeFromLeft(92));
    row1.removeFromLeft(6);
    undoButton.setBounds(row1.removeFromLeft(72));
    row1.removeFromLeft(6);
    redoButton.setBounds(row1.removeFromLeft(72));

    toolsArea.removeFromTop(6);
    auto row2 = toolsArea.removeFromTop(30);
    midiParamCombo.setBounds(row2.removeFromLeft(160));
    row2.removeFromLeft(8);
    midiLearnButton.setBounds(row2.removeFromLeft(105));

    toolsArea.removeFromTop(6);
    auto row3 = toolsArea.removeFromTop(30);
    loadBackgroundButton.setBounds(row3.removeFromLeft(148));
    row3.removeFromLeft(8);
    clearBackgroundButton.setBounds(row3.removeFromLeft(138));

    toolsArea.removeFromTop(8);
    auto ioRow = toolsArea.removeFromTop(26);
    inputTrimLabel.setBounds(ioRow.removeFromLeft(70));
    inputTrimSlider.setBounds(ioRow.removeFromLeft(150));
    ioRow.removeFromLeft(10);
    limiterEnabledToggle.setBounds(ioRow.removeFromLeft(80));
    ioRow.removeFromLeft(8);
    limiterClipLabel.setBounds(ioRow.removeFromLeft(44));
    ioRow.removeFromLeft(4);
    limiterThreshLabel.setBounds(ioRow.removeFromLeft(80));
    limiterThreshSlider.setBounds(ioRow.removeFromLeft(130));

    toolsArea.removeFromTop(8);
    spectrumAnalyzer->setBounds(toolsArea.removeFromTop(80));
    toolsArea.removeFromTop(6);
    presetBrowserLabel.setBounds(toolsArea.removeFromTop(18));
    toolsArea.removeFromTop(4);
    auto presetListArea = toolsArea.removeFromTop(120);
    auto presetButtonArea = presetListArea.removeFromBottom(28);
    presetListBox.setBounds(presetListArea);
    presetListArea = presetButtonArea;
    loadFactoryPresetButton.setBounds(presetListArea.removeFromLeft(130));

    toolsArea.removeFromTop(6);
    midiMapStatusLabel.setBounds(toolsArea.removeFromTop(22));
    toolsArea.removeFromTop(4);
    backgroundStatusLabel.setBounds(toolsArea.removeFromTop(22));
}

void MyAmpSimAudioProcessorEditor::chooseCabinetIR()
{
    chooseCabinetIRSlot(0);
}

void MyAmpSimAudioProcessorEditor::chooseCabinetIRSlot(int slotIndex)
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

void MyAmpSimAudioProcessorEditor::refreshIrStatus()
{
    irStatusLabel.setText("Cab Section", juce::dontSendNotification);
    irStatusALabel.setText("A: " + audioProcessor.getCurrentIRNameForSlot(0), juce::dontSendNotification);
    irStatusBLabel.setText("B: " + audioProcessor.getCurrentIRNameForSlot(1), juce::dontSendNotification);
}

void MyAmpSimAudioProcessorEditor::refreshTunerStatus()
{
    const auto note = audioProcessor.getTunerNoteName();
    const auto frequency = audioProcessor.getTunerFrequencyHz();
    const auto cents = audioProcessor.getTunerCents();

    if (frequency <= 0.0f || note == "--")
    {
        tunerNoteLabel.setText("TUNER --", juce::dontSendNotification);
        tunerDetailLabel.setText("Play a note", juce::dontSendNotification);
        return;
    }

    tunerNoteLabel.setText("TUNER " + note, juce::dontSendNotification);
    tunerDetailLabel.setText(juce::String(frequency, 2) + " Hz | " + juce::String(cents, 1) + " cents", juce::dontSendNotification);
}

void MyAmpSimAudioProcessorEditor::refreshToolsStatus()
{
    midiMapStatusLabel.setText(audioProcessor.getMidiMappingDescription(), juce::dontSendNotification);
    undoButton.setEnabled(audioProcessor.canUndo());
    redoButton.setEnabled(audioProcessor.canRedo());

    if (audioProcessor.getBackgroundImagePath().isNotEmpty())
        backgroundStatusLabel.setText("Background: " + juce::File(audioProcessor.getBackgroundImagePath()).getFileName(), juce::dontSendNotification);
    else
        backgroundStatusLabel.setText("Background: Default", juce::dontSendNotification);
}

void MyAmpSimAudioProcessorEditor::loadBackgroundImageFromFile(const juce::File& file)
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

void MyAmpSimAudioProcessorEditor::chooseBackgroundImage()
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

void MyAmpSimAudioProcessorEditor::clearBackgroundImage()
{
    backgroundImage = {};
    audioProcessor.setBackgroundImagePath({});
    refreshToolsStatus();
    repaint();
}

void MyAmpSimAudioProcessorEditor::savePresetToFile()
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

void MyAmpSimAudioProcessorEditor::loadPresetFromFile()
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

void MyAmpSimAudioProcessorEditor::captureSnapshotA()
{
    audioProcessor.getStateInformation(snapshotA);
}

void MyAmpSimAudioProcessorEditor::captureSnapshotB()
{
    audioProcessor.getStateInformation(snapshotB);
}

void MyAmpSimAudioProcessorEditor::recallSnapshotAorB()
{
    const auto& chosen = compareABToggle.getToggleState() ? snapshotB : snapshotA;
    if (chosen.getSize() == 0)
        return;

    audioProcessor.setStateInformation(chosen.getData(), static_cast<int>(chosen.getSize()));
    refreshIrStatus();
    refreshToolsStatus();
}

void MyAmpSimAudioProcessorEditor::switchToTab(UiTab tab)
{
    currentTab = tab;
    updateTabVisibility();
}

void MyAmpSimAudioProcessorEditor::applyEditorSizePreset(UiScale preset)
{
    currentScale = preset;

    if (preset == UiScale::small)
        setSize(680, 460);
    else if (preset == UiScale::large)
        setSize(920, 620);
    else
        setSize(760, 520);
}

void MyAmpSimAudioProcessorEditor::updateTabVisibility()
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
    irPhaseToggle.setVisible(showFx);
    cabFlipAButton.setVisible(showFx);
    cabFlipBButton.setVisible(showFx);
    irStatusLabel.setVisible(showFx);
    irStatusALabel.setVisible(showFx);
    irStatusBLabel.setVisible(showFx);
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
    compareABToggle.setVisible(showTools);
    undoButton.setVisible(showTools);
    redoButton.setVisible(showTools);
    midiParamCombo.setVisible(showTools);
    midiLearnButton.setVisible(showTools);
    midiMapStatusLabel.setVisible(showTools);
    backgroundStatusLabel.setVisible(showTools);
    inputTrimSlider.setVisible(showTools);
    inputTrimLabel.setVisible(showTools);
    limiterThreshSlider.setVisible(showTools);
    limiterThreshLabel.setVisible(showTools);
    limiterEnabledToggle.setVisible(showTools);
    presetListBox.setVisible(showTools);
    presetBrowserLabel.setVisible(showTools);
    loadFactoryPresetButton.setVisible(showTools);
    spectrumAnalyzer->setVisible(showTools);

    ampTabButton.setColour(juce::TextButton::buttonColourId, showAmp ? juce::Colours::darkorange : juce::Colours::darkgrey);
    fxTabButton.setColour(juce::TextButton::buttonColourId, showFx ? juce::Colours::darkorange : juce::Colours::darkgrey);
    toolsTabButton.setColour(juce::TextButton::buttonColourId, showTools ? juce::Colours::darkorange : juce::Colours::darkgrey);
}

void MyAmpSimAudioProcessorEditor::timerCallback()
{
    refreshTunerStatus();
    refreshToolsStatus();
    const bool limiterOn = audioProcessor.limiterActive.load();
    limiterClipLabel.setText(limiterOn ? "CLIP" : "", juce::dontSendNotification);
    limiterClipLabel.setColour(juce::Label::textColourId, limiterOn ? juce::Colours::red : juce::Colours::transparentBlack);
    
    repaint(inputMeterBounds.getUnion(outputMeterBounds).getUnion(correlationMeterBounds));

    // Feed spectrum analyzer from processor FIFO
    if (spectrumAnalyzer->isVisible())
    {
        static std::array<float, MyAmpSimAudioProcessor::kSpecFifoSize> tmpBuf;
        if (audioProcessor.consumeSpectrumBlock(tmpBuf.data()))
            spectrumAnalyzer->pushSamples(tmpBuf.data(), MyAmpSimAudioProcessor::kSpecFifoSize);
    }
}

void MyAmpSimAudioProcessorEditor::drawMeter(juce::Graphics& g,
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

void MyAmpSimAudioProcessorEditor::openAudioSettings()
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