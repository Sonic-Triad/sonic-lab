#include "PluginEditor.h"

namespace
{
    constexpr int editorWidth  = 720;
    constexpr int editorHeight = 500;
    constexpr int margin       = 20;
    constexpr int headerHeight = 72;
}

OverdriveAudioProcessorEditor::Knob::Knob (juce::AudioProcessorValueTreeState& state,
                                          const juce::String& paramId,
                                          const juce::String& name,
                                          const juce::String& suffix)
    : slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow),
      attachment (state, paramId, slider)
{
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 22);
    slider.setTextValueSuffix (suffix);
    slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);

    label.setText (name.toUpperCase(), juce::dontSendNotification);
    label.setFont (juce::FontOptions (13.0f, juce::Font::bold).withKerningFactor (0.18f));
    label.setColour (juce::Label::textColourId, TriadColours::textDim);
    label.setJustificationType (juce::Justification::centred);
}

OverdriveAudioProcessorEditor::OverdriveAudioProcessorEditor (OverdriveAudioProcessor& p)
    : AudioProcessorEditor (&p),
      header ("Overdrive", "Soft clip  /  4x oversampled", margin),
      analyzer (p.inputTap, p.outputTap, [&p] { return p.getSampleRate(); }),
      drive (p.apvts, "drive", "Drive", "%"),
      tone  (p.apvts, "tone",  "Tone",  "%"),
      level (p.apvts, "level", "Level", " dB")
{
    addAndMakeVisible (header);
    addAndMakeVisible (analyzer);

    for (auto* knob : { &drive, &tone, &level })
    {
        addAndMakeVisible (knob->slider);
        addAndMakeVisible (knob->label);
    }

    // After adding children, so the look-and-feel change reaches them and the
    // sliders rebuild their text boxes with our colours
    setLookAndFeel (&lookAndFeel);

    setSize (editorWidth, editorHeight);
}

OverdriveAudioProcessorEditor::~OverdriveAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void OverdriveAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;

    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (ColourGradient (Colour (0xff1a1917), 0.0f, 0.0f,
                                       Colour (0xff0e0e0d), 0.0f, bounds.getBottom(), false));
    g.fillAll();

    // Control panel behind the knobs
    const auto panel = controlsArea.toFloat().reduced ((float) margin, 0.0f).withTrimmedBottom ((float) margin);
    g.setColour (TriadColours::panel);
    g.fillRoundedRectangle (panel, 10.0f);
    g.setColour (TriadColours::panelEdge);
    g.drawRoundedRectangle (panel.reduced (0.5f), 10.0f, 1.0f);
}

void OverdriveAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    header.setBounds (area.removeFromTop (headerHeight));
    area.removeFromTop (margin - 4);

    analyzer.setBounds (area.removeFromTop (220).reduced (margin, 0));
    area.removeFromTop (margin - 4);

    controlsArea = area;

    auto knobs = controlsArea.reduced (margin, 0).withTrimmedBottom (margin).reduced (24, 12);
    const auto columnWidth = knobs.getWidth() / 3;

    for (auto* knob : { &drive, &tone, &level })
    {
        auto column = knobs.removeFromLeft (columnWidth).reduced (16, 0);
        knob->label.setBounds (column.removeFromTop (20));
        knob->slider.setBounds (column);
    }
}
