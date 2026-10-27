#include "TriadLookAndFeel.h"

TriadLookAndFeel::TriadLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, TriadColours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, TriadColours::gold.withAlpha (0.4f));
    setColour (juce::Label::textColourId, TriadColours::text);
    setColour (juce::TextEditor::textColourId, TriadColours::text);
    setColour (juce::TextEditor::highlightColourId, TriadColours::gold.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, TriadColours::gold);
}

void TriadLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider& slider)
{
    using namespace juce;

    const auto bounds = Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const auto radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Value arc: dim track behind a gold fill
    const auto arcRadius = radius - 3.0f;
    const auto arcWidth  = 4.0f;

    Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (TriadColours::panelEdge);
    g.strokePath (track, PathStrokeType (arcWidth, PathStrokeType::curved, PathStrokeType::rounded));

    Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
    g.setGradientFill (ColourGradient (TriadColours::goldLight, centre.x - radius, centre.y - radius,
                                       TriadColours::goldDark, centre.x + radius, centre.y + radius, false));
    g.strokePath (value, PathStrokeType (arcWidth, PathStrokeType::curved, PathStrokeType::rounded));

    // Knob body with a soft top-down sheen
    const auto knobRadius = arcRadius - 9.0f;
    const auto knob = Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre);

    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (knob.translated (0.0f, 3.0f).expanded (1.0f));

    g.setGradientFill (ColourGradient (Colour (0xff34322d), centre.x, knob.getY(),
                                       Colour (0xff1a1917), centre.x, knob.getBottom(), false));
    g.fillEllipse (knob);

    g.setColour (slider.isMouseOverOrDragging() ? TriadColours::gold.withAlpha (0.6f)
                                                : Colours::white.withAlpha (0.08f));
    g.drawEllipse (knob.reduced (0.5f), 1.0f);

    // Pointer
    Path pointer;
    const auto pointerLength = knobRadius * 0.55f;
    pointer.addRoundedRectangle (-1.5f, -knobRadius + 5.0f, 3.0f, pointerLength, 1.5f);
    g.setColour (TriadColours::goldLight);
    g.fillPath (pointer, AffineTransform::rotation (angle).translated (centre));
}

juce::Label* TriadLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (juce::FontOptions (14.0f));
    return label;
}
