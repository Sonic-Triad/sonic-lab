#include "TriadHeader.h"
#include "TriadLookAndFeel.h"

#include <TriadAssets.h>

TriadHeader::TriadHeader (juce::String titleText, juce::String subtitleText, int horizontalPadding)
    : title (std::move (titleText)),
      subtitle (std::move (subtitleText)),
      padding (horizontalPadding),
      logo (juce::ImageCache::getFromMemory (TriadAssets::triad_logo_png, TriadAssets::triad_logo_pngSize))
{
    setInterceptsMouseClicks (false, false);
}

void TriadHeader::paint (juce::Graphics& g)
{
    using namespace juce;

    const auto bounds = getLocalBounds().toFloat();
    auto area = bounds.reduced ((float) padding, 0.0f);

    if (logo.isValid())
    {
        const auto logoHeight = 30.0f;
        const auto logoWidth  = logoHeight * (float) logo.getWidth() / (float) logo.getHeight();
        g.drawImage (logo, area.removeFromLeft (logoWidth).withSizeKeepingCentre (logoWidth, logoHeight),
                     RectanglePlacement::centred);
    }

    auto titleArea = area.removeFromRight (260.0f);
    g.setColour (TriadColours::gold);
    g.setFont (FontOptions (20.0f, Font::bold).withKerningFactor (0.25f));
    g.drawText (title.toUpperCase(), titleArea.removeFromTop (titleArea.getHeight() * 0.58f), Justification::bottomRight);

    g.setColour (TriadColours::textDim);
    g.setFont (FontOptions (11.0f).withKerningFactor (0.12f));
    g.drawText (subtitle.toUpperCase(), titleArea.withTrimmedTop (3.0f), Justification::topRight);

    // Hairline that fades out at both ends
    const auto left  = (float) padding;
    const auto right = bounds.getRight() - (float) padding;

    ColourGradient line (TriadColours::gold.withAlpha (0.0f), left, 0.0f,
                         TriadColours::gold.withAlpha (0.0f), right, 0.0f, false);
    line.addColour (0.5, TriadColours::gold.withAlpha (0.45f));
    g.setGradientFill (line);
    g.fillRect (Rectangle<float> (left, bounds.getBottom() - 1.0f, right - left, 1.0f));
}
