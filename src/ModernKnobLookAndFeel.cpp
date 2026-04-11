#include "ModernKnobLookAndFeel.h"

#include <cmath>

void ModernKnobLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                             int x,
                                             int y,
                                             int width,
                                             int height,
                                             float sliderPosProportional,
                                             float rotaryStartAngle,
                                             float rotaryEndAngle,
                                             juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x),
                                               static_cast<float>(y),
                                               static_cast<float>(width),
                                               static_cast<float>(height)).reduced(12.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    const bool isHovered = slider.isMouseOverOrDragging();

    g.setColour(juce::Colours::black.withAlpha(0.34f));
    g.fillEllipse(bounds.translated(0.0f, 1.7f));

    juce::ColourGradient capGradient(
        juce::Colour(0xff181b20),
        centre.x, bounds.getY(),
        juce::Colour(0xff0f1115),
        centre.x, bounds.getBottom(),
        false);
    capGradient.addColour(0.40, juce::Colour(0xff14171c));
    g.setGradientFill(capGradient);
    g.fillEllipse(bounds);

    g.setColour(juce::Colour(0xff333844));
    g.drawEllipse(bounds.reduced(radius * 0.04f), 1.0f);

    const float trackRadius = radius * 0.83f;
    juce::Path trackArc;
    trackArc.addCentredArc(centre.x,
                           centre.y,
                           trackRadius,
                           trackRadius,
                           0.0f,
                           rotaryStartAngle,
                           rotaryEndAngle,
                           true);

    g.setColour(juce::Colour(0xff2a2f38));
    g.strokePath(trackArc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x,
                           centre.y,
                           trackRadius,
                           trackRadius,
                           0.0f,
                           rotaryStartAngle,
                           angle,
                           true);

    juce::Colour arcColour = juce::Colour(0xff49cfff);
    if (isHovered)
        arcColour = arcColour.brighter(0.10f);

    if (isHovered)
    {
        g.setColour(arcColour.withAlpha(0.22f));
        g.strokePath(valueArc, juce::PathStrokeType(4.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour(arcColour.withAlpha(0.95f));
    g.strokePath(valueArc, juce::PathStrokeType(2.35f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float pointerStart = radius * 0.18f;
    const float pointerEnd = radius * 0.62f;
    const float x1 = centre.x + std::cos(angle) * pointerStart;
    const float y1 = centre.y + std::sin(angle) * pointerStart;
    const float x2 = centre.x + std::cos(angle) * pointerEnd;
    const float y2 = centre.y + std::sin(angle) * pointerEnd;

    if (isHovered)
    {
        g.setColour(juce::Colour(0xffffffff).withAlpha(0.16f));
        g.drawLine(x1, y1, x2, y2, slider.isMouseButtonDown() ? 3.3f : 2.7f);
    }

    g.setColour(juce::Colour(0xffeef4ff).withAlpha(0.94f));
    g.drawLine(x1, y1, x2, y2, slider.isMouseButtonDown() ? 2.0f : 1.7f);

    g.setColour(juce::Colour(0xffdbe2ef).withAlpha(0.70f));
    g.fillEllipse(juce::Rectangle<float>(centre.x - 1.2f, centre.y - 1.2f, 2.4f, 2.4f));
}
