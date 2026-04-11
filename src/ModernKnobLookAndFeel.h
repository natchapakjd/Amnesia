#pragma once

#if __has_include(<JuceHeader.h>)
#include <JuceHeader.h>
#elif __has_include("../build-local/Vayu_artefacts/JuceLibraryCode/JuceHeader.h")
#include "../build-local/Vayu_artefacts/JuceLibraryCode/JuceHeader.h"
#else
#error Could not locate JuceHeader.h
#endif

class ModernKnobLookAndFeel : public juce::LookAndFeel_V4
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
