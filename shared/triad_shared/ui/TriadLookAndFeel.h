#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace TriadColours
{
    const juce::Colour background { 0xff121212 };
    const juce::Colour panel      { 0xff1b1b1b };
    const juce::Colour panelEdge  { 0xff2c2a26 };
    const juce::Colour gold       { 0xffc7a954 };
    const juce::Colour goldLight  { 0xffe2cb7d };
    const juce::Colour goldDark   { 0xff9c7a2c };
    const juce::Colour text       { 0xffece6d6 };
    const juce::Colour textDim    { 0xff8c867a };
}

// Gold-on-charcoal styling to match the Triad brand
class TriadLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    TriadLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
};
