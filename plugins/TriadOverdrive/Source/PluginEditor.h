#pragma once

#include "PluginProcessor.h"

class OverdriveAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit OverdriveAudioProcessorEditor (OverdriveAudioProcessor&);
    ~OverdriveAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    SpectrumAnalyzer& getAnalyzer() { return analyzer; }

private:
    struct Knob
    {
        Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
              const juce::String& name, const juce::String& suffix);

        juce::Slider slider;
        juce::Label label;
        juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    };

    TriadLookAndFeel lookAndFeel;
    TriadHeader header;

    SpectrumAnalyzer analyzer;
    Knob drive, tone, level;

    juce::Rectangle<int> controlsArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverdriveAudioProcessorEditor)
};
