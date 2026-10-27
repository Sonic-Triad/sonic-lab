#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Branded title bar shared by every Sonic Lab plugin: Triad logo on the left,
// product name and a short subtitle on the right, gold hairline underneath
class TriadHeader final : public juce::Component
{
public:
    TriadHeader (juce::String title, juce::String subtitle, int horizontalPadding = 20);

    void paint (juce::Graphics&) override;

private:
    juce::String title, subtitle;
    int padding;
    juce::Image logo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriadHeader)
};
