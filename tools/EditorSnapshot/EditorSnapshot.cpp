// Renders a plugin's editor to a PNG after running a synthetic guitar note
// through its processor, so any SpectrumAnalyzer in the editor has something to show.
// Built per plugin as <Plugin>Snapshot when SONIC_LAB_BUILD_SNAPSHOT_TOOL is on.
//
//   <Plugin>Snapshot out.png [paramId=normalisedValue ...]
//   e.g. TriadOverdriveSnapshot overdrive.png drive=0.9 tone=0.8

#include <juce_audio_processors/juce_audio_processors.h>
#include <triad_shared/triad_shared.h>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    SpectrumAnalyzer* findAnalyzer (juce::Component& parent)
    {
        for (auto* child : parent.getChildren())
        {
            if (auto* analyzer = dynamic_cast<SpectrumAnalyzer*> (child))
                return analyzer;

            if (auto* nested = findAnalyzer (*child))
                return nested;
        }

        return nullptr;
    }

    bool setParameter (juce::AudioProcessor& processor, const juce::String& assignment)
    {
        const auto id = assignment.upToFirstOccurrenceOf ("=", false, false);
        const auto value = assignment.fromFirstOccurrenceOf ("=", false, false).getFloatValue();

        for (auto* param : processor.getParameters())
        {
            if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (param); withId != nullptr && withId->paramID == id)
            {
                param->setValueNotifyingHost (value);
                return true;
            }
        }

        return false;
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto outFile = juce::File::getCurrentWorkingDirectory()
                             .getChildFile (argc > 1 ? argv[1] : "editor_snapshot.png");

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;

    std::unique_ptr<juce::AudioProcessor> processor (createPluginFilter());

    for (int i = 2; i < argc; ++i)
    {
        if (! setParameter (*processor, argv[i]))
        {
            std::cerr << "Unknown parameter in '" << argv[i] << "'" << std::endl;
            return 1;
        }
    }

    const auto numChannels = juce::jmax (processor->getTotalNumInputChannels(), processor->getTotalNumOutputChannels());
    processor->setPlayConfigDetails (processor->getTotalNumInputChannels(), processor->getTotalNumOutputChannels(),
                                     sampleRate, blockSize);
    processor->prepareToPlay (sampleRate, blockSize);

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor->createEditorIfNeeded());
    auto* analyzer = editor != nullptr ? findAnalyzer (*editor) : nullptr;

    // Plucked A2 (110 Hz): a stack of harmonics with a slow decay, peaking around -12 dBFS
    juce::AudioBuffer<float> buffer (numChannels, blockSize);
    juce::MidiBuffer midi;
    constexpr double fundamental = 110.0;
    constexpr int numHarmonics = 12;
    constexpr int totalBlocks = (int) (1.5 * sampleRate) / blockSize;
    constexpr int blocksPerFrame = (int) (sampleRate / 30.0) / blockSize;
    int64_t sampleIndex = 0;

    for (int b = 0; b < totalBlocks; ++b)
    {
        for (int i = 0; i < blockSize; ++i, ++sampleIndex)
        {
            const auto t = (double) sampleIndex / sampleRate;
            double value = 0.0;

            for (int h = 1; h <= numHarmonics; ++h)
                value += std::sin (juce::MathConstants<double>::twoPi * fundamental * h * t) / (h * h * 0.6 + 0.4);

            const auto sample = (float) (0.25 * std::exp (-0.4 * t) * value);

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample (ch, i, sample);
        }

        processor->processBlock (buffer, midi);

        if (analyzer != nullptr && b % juce::jmax (1, blocksPerFrame) == 0)
            analyzer->refresh();
    }

    if (editor == nullptr)
    {
        std::cerr << "Plugin has no editor" << std::endl;
        return 1;
    }

    if (analyzer != nullptr)
        analyzer->refresh();

    editor->setVisible (true);

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
    outFile.deleteFile();

    juce::FileOutputStream stream (outFile);
    const auto ok = stream.openedOk() && juce::PNGImageFormat().writeImageToStream (image, stream);

    editor.reset();
    processor.reset();

    std::cout << (ok ? "Wrote " : "Failed to write ") << outFile.getFullPathName() << std::endl;
    return ok ? 0 : 1;
}
