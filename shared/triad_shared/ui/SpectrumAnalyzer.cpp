#include "SpectrumAnalyzer.h"
#include "TriadLookAndFeel.h"

namespace
{
    // How fast a peak falls back when the signal drops, per 30 Hz frame
    constexpr float releaseDbPerFrame = 1.5f;

    // A full-scale sine through a Hann window peaks at fftSize / 4 in its bin
    constexpr float windowNormalisation = 0.25f;

    constexpr float gridFrequencies[] { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
}

SpectrumAnalyzer::SpectrumAnalyzer (SpectrumTap& inputTap, SpectrumTap& outputTap, std::function<double()> sampleRateSource)
    : input (inputTap), output (outputTap), getSampleRate (std::move (sampleRateSource))
{
    input.levelsDb.fill (minDb);
    output.levelsDb.fill (minDb);

    setOpaque (true);
    startTimerHz (30);
}

void SpectrumAnalyzer::refresh()
{
    analyse (input);
    analyse (output);
    repaint();
}

void SpectrumAnalyzer::analyse (Channel& channel)
{
    int received = 0;

    channel.tap.pull ([&] (const float* samples, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            channel.history[(size_t) channel.writeIndex] = samples[i];
            channel.writeIndex = (channel.writeIndex + 1) % fftSize;
        }

        received += count;
    });

    // Nothing new (transport stopped, plugin bypassed): let the display fall away
    if (received == 0)
    {
        for (auto& level : channel.levelsDb)
            level = juce::jmax (minDb, level - releaseDbPerFrame);

        return;
    }

    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = channel.history[(size_t) ((channel.writeIndex + i) % fftSize)];

    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    const auto norm = windowNormalisation * (float) fftSize;

    for (int bin = 0; bin < numBins; ++bin)
    {
        const auto db = juce::Decibels::gainToDecibels (fftData[(size_t) bin] / norm, minDb);
        auto& level = channel.levelsDb[(size_t) bin];
        level = db > level ? db : juce::jmax (db, level - releaseDbPerFrame);
    }
}

float SpectrumAnalyzer::frequencyToX (float freq, float width) const
{
    return width * std::log (freq / minFreq) / std::log (maxFreq / minFreq);
}

float SpectrumAnalyzer::dbToY (float db, float height) const
{
    return juce::jmap (juce::jlimit (minDb, maxDb, db), maxDb, minDb, 0.0f, height);
}

juce::Path SpectrumAnalyzer::createSpectrumPath (const Channel& channel, juce::Rectangle<float> area) const
{
    auto sampleRate = (float) getSampleRate();

    if (sampleRate <= 0.0f)
        sampleRate = 48000.0f;

    const auto binsPerHz = (float) fftSize / sampleRate;
    const auto ratio = maxFreq / minFreq;
    const auto width = area.getWidth();

    auto levelAt = [&] (float x)
    {
        // Each pixel column covers a frequency range; at the top end it spans
        // many bins (take the loudest), at the bottom it falls between two (interpolate)
        const auto lowBin  = minFreq * std::pow (ratio, x / width) * binsPerHz;
        const auto highBin = minFreq * std::pow (ratio, (x + 1.0f) / width) * binsPerHz;

        const auto first = juce::jlimit (0, numBins - 1, (int) lowBin);
        const auto last  = juce::jlimit (0, numBins - 1, (int) highBin);

        if (last > first)
            return *std::max_element (channel.levelsDb.begin() + first, channel.levelsDb.begin() + last + 1);

        const auto next = juce::jmin (first + 1, numBins - 1);
        const auto frac = lowBin - (float) first;
        return juce::jmap (frac, channel.levelsDb[(size_t) first], channel.levelsDb[(size_t) next]);
    };

    juce::Path path;

    for (float x = 0.0f; x <= width; x += 1.0f)
    {
        const juce::Point<float> point { area.getX() + x, area.getY() + dbToY (levelAt (x), area.getHeight()) };

        if (x == 0.0f)
            path.startNewSubPath (point);
        else
            path.lineTo (point);
    }

    return path;
}

void SpectrumAnalyzer::paint (juce::Graphics& g)
{
    using namespace juce;

    g.fillAll (TriadColours::background);

    const auto bounds = getLocalBounds().toFloat();
    const auto plot = bounds.reduced (1.0f);

    g.setGradientFill (ColourGradient (Colour (0xff1d1c19), 0.0f, plot.getY(),
                                       Colour (0xff141413), 0.0f, plot.getBottom(), false));
    g.fillRoundedRectangle (plot, 8.0f);

    {
        Graphics::ScopedSaveState state (g);

        Path clip;
        clip.addRoundedRectangle (plot, 8.0f);
        g.reduceClipRegion (clip);

        // Grid lines sit behind the spectra; their labels are drawn on top later
        for (auto db = -20.0f; db > minDb; db -= 20.0f)
        {
            g.setColour (Colours::white.withAlpha (0.05f));
            g.drawHorizontalLine (roundToInt (plot.getY() + dbToY (db, plot.getHeight())), plot.getX(), plot.getRight());
        }

        for (auto freq : gridFrequencies)
        {
            g.setColour (Colours::white.withAlpha (0.05f));
            g.drawVerticalLine (roundToInt (plot.getX() + frequencyToX (freq, plot.getWidth())), plot.getY(), plot.getBottom());
        }

        // Input: neutral, sits behind
        const auto inputLine = createSpectrumPath (input, plot);
        auto inputFill = inputLine;
        inputFill.lineTo (plot.getBottomRight());
        inputFill.lineTo (plot.getBottomLeft());
        inputFill.closeSubPath();

        g.setColour (Colours::white.withAlpha (0.06f));
        g.fillPath (inputFill);
        g.setColour (Colours::white.withAlpha (0.35f));
        g.strokePath (inputLine, PathStrokeType (1.2f));

        // Output: gold, in front
        const auto outputLine = createSpectrumPath (output, plot);
        auto outputFill = outputLine;
        outputFill.lineTo (plot.getBottomRight());
        outputFill.lineTo (plot.getBottomLeft());
        outputFill.closeSubPath();

        g.setGradientFill (ColourGradient (TriadColours::gold.withAlpha (0.32f), 0.0f, plot.getY(),
                                           TriadColours::gold.withAlpha (0.02f), 0.0f, plot.getBottom(), false));
        g.fillPath (outputFill);
        g.setColour (TriadColours::goldLight);
        g.strokePath (outputLine, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));

        // Axis labels, with a dark halo so they stay readable over the curves
        g.setFont (FontOptions (11.0f));

        auto drawLabel = [&] (const String& text, Rectangle<float> area, Justification justification)
        {
            g.setColour (TriadColours::background.withAlpha (0.75f));
            g.fillRoundedRectangle (area.withSizeKeepingCentre (GlyphArrangement::getStringWidth (g.getCurrentFont(), text) + 8.0f,
                                                                area.getHeight())
                                        .withX (justification == Justification::centredLeft ? area.getX() - 4.0f : area.getX()),
                                    3.0f);
            g.setColour (TriadColours::textDim);
            g.drawText (text, area, justification);
        };

        for (auto db = -20.0f; db > minDb; db -= 20.0f)
        {
            const auto y = plot.getY() + dbToY (db, plot.getHeight());
            drawLabel (String (roundToInt (db)) + " dB", { plot.getX() + 10.0f, y - 16.0f, 50.0f, 14.0f }, Justification::centredLeft);
        }

        for (auto freq : gridFrequencies)
        {
            const auto x = plot.getX() + frequencyToX (freq, plot.getWidth());
            const auto text = freq >= 1000.0f ? String (roundToInt (freq / 1000.0f)) + "k" : String (roundToInt (freq));
            drawLabel (text, { x + 6.0f, plot.getBottom() - 20.0f, 30.0f, 14.0f }, Justification::centredLeft);
        }
    }

    // Legend
    auto legend = Rectangle<float> (plot.getRight() - 112.0f, plot.getY() + 10.0f, 100.0f, 14.0f);
    g.setFont (FontOptions (11.0f, Font::bold));

    auto drawKey = [&] (Rectangle<float> area, Colour colour, const String& name)
    {
        g.setColour (colour);
        g.fillRoundedRectangle (area.removeFromLeft (14.0f).withSizeKeepingCentre (14.0f, 3.0f), 1.5f);
        g.setColour (TriadColours::text.withAlpha (0.8f));
        g.drawText (name, area.withTrimmedLeft (6.0f), Justification::centredLeft);
    };

    drawKey (legend.removeFromLeft (44.0f), Colours::white.withAlpha (0.5f), "IN");
    drawKey (legend.withTrimmedLeft (6.0f), TriadColours::goldLight, "OUT");

    g.setColour (TriadColours::panelEdge);
    g.drawRoundedRectangle (plot, 8.0f, 1.0f);
}
