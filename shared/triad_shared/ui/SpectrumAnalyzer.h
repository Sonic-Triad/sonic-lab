#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../dsp/SpectrumTap.h"

// Live input-vs-output spectrum on a log-frequency axis
class SpectrumAnalyzer final : public juce::Component,
                               private juce::Timer
{
public:
    SpectrumAnalyzer (SpectrumTap& input, SpectrumTap& output, std::function<double()> getSampleRate);

    void paint (juce::Graphics&) override;

    // Drains the taps and recomputes both spectra; runs on a timer while shown
    void refresh();

private:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int numBins  = fftSize / 2;

    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDb   = -90.0f;
    static constexpr float maxDb   = 0.0f;

    struct Channel
    {
        explicit Channel (SpectrumTap& t) : tap (t) {}

        SpectrumTap& tap;
        std::array<float, fftSize> history {};      // most recent fftSize samples, circular
        int writeIndex = 0;
        std::array<float, numBins> levelsDb {};     // smoothed per-bin level for display
    };

    void timerCallback() override { refresh(); }
    void analyse (Channel&);

    float frequencyToX (float freq, float width) const;
    float dbToY (float db, float height) const;
    juce::Path createSpectrumPath (const Channel&, juce::Rectangle<float> area) const;

    Channel input, output;
    std::function<double()> getSampleRate;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, fftSize * 2> fftData {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzer)
};
