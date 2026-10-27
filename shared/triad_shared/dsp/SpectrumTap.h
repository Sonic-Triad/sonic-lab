#pragma once

#include <juce_dsp/juce_dsp.h>

// Single-producer/single-consumer handoff of mono samples from the audio thread
// to the editor's analyzer. The audio side never blocks or allocates; when the
// editor isn't reading, the FIFO fills up and new samples are simply dropped.
class SpectrumTap
{
public:
    static constexpr int capacity = 1 << 15;

    // Audio thread: mixes all channels of the block down to mono and queues it
    void push (const juce::dsp::AudioBlock<const float>& block)
    {
        const auto numChannels = block.getNumChannels();

        if (numChannels == 0)
            return;

        const auto scale = 1.0f / (float) numChannels;
        const auto scope = fifo.write ((int) block.getNumSamples());

        auto writeRange = [&] (int dest, int count, size_t sourceOffset)
        {
            for (int i = 0; i < count; ++i)
            {
                float sum = 0.0f;

                for (size_t ch = 0; ch < numChannels; ++ch)
                    sum += block.getSample ((int) ch, (int) sourceOffset + i);

                buffer[(size_t) (dest + i)] = sum * scale;
            }
        };

        writeRange (scope.startIndex1, scope.blockSize1, 0);
        writeRange (scope.startIndex2, scope.blockSize2, (size_t) scope.blockSize1);
    }

    // Message thread: drains everything queued so far, oldest first
    template <typename Consumer>
    void pull (Consumer&& consume)
    {
        const auto scope = fifo.read (fifo.getNumReady());

        if (scope.blockSize1 > 0) consume (buffer.data() + scope.startIndex1, scope.blockSize1);
        if (scope.blockSize2 > 0) consume (buffer.data() + scope.startIndex2, scope.blockSize2);
    }

private:
    juce::AbstractFifo fifo { capacity };
    std::array<float, (size_t) capacity> buffer {};
};
