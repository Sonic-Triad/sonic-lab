#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::String driveId { "drive" };
    const juce::String toneId  { "tone" };
    const juce::String levelId { "level" };

    constexpr float maxDriveDb = 40.0f;

    // Small DC offset before the waveshaper skews it, adding even harmonics
    constexpr float clipBias = 0.15f;

    // Tone knob (0..1) maps exponentially onto the post-clip low-pass cutoff
    constexpr float minToneHz = 600.0f;
    constexpr float maxToneHz = 9000.0f;

    float toneToCutoff (float tone)
    {
        return minToneHz * std::pow (maxToneHz / minToneHz, tone);
    }
}

OverdriveAudioProcessor::OverdriveAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    driveParam = apvts.getRawParameterValue (driveId);
    toneParam  = apvts.getRawParameterValue (toneId);
    levelParam = apvts.getRawParameterValue (levelId);
}

juce::AudioProcessorValueTreeState::ParameterLayout OverdriveAudioProcessor::createParameterLayout()
{
    using namespace juce;

    auto percent = AudioParameterFloatAttributes()
                       .withLabel ("%")
                       .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)); });

    auto decibels = AudioParameterFloatAttributes()
                        .withLabel ("dB")
                        .withStringFromValueFunction ([] (float v, int) { return String (v, 1); });

    return {
        std::make_unique<AudioParameterFloat> (ParameterID { driveId, 1 }, "Drive",
                                               NormalisableRange<float> (0.0f, 1.0f), 0.5f, percent),
        std::make_unique<AudioParameterFloat> (ParameterID { toneId, 1 }, "Tone",
                                               NormalisableRange<float> (0.0f, 1.0f), 0.5f, percent),
        std::make_unique<AudioParameterFloat> (ParameterID { levelId, 1 }, "Level",
                                               NormalisableRange<float> (-24.0f, 12.0f), 0.0f, decibels)
    };
}

void OverdriveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto numChannels = (juce::uint32) juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels());
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, numChannels };

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        numChannels, oversamplingFactorLog2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,   // max quality
        true);  // integer latency so the host can compensate exactly
    oversampling->initProcessing ((size_t) samplesPerBlock);
    setLatencySamples ((int) oversampling->getLatencyInSamples());

    const auto osFactor = oversampling->getOversamplingFactor();
    const juce::dsp::ProcessSpec osSpec { sampleRate * (double) osFactor,
                                          (juce::uint32) (samplesPerBlock * (int) osFactor),
                                          numChannels };

    inputHighPass.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    inputHighPass.setCutoffFrequency (70.0f);
    inputHighPass.prepare (spec);

    driveGain.setRampDurationSeconds (0.05);
    driveGain.prepare (osSpec);

    dcBlocker.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    dcBlocker.setCutoffFrequency (15.0f);
    dcBlocker.prepare (spec);

    toneFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    toneFilter.setResonance (0.6f);
    toneFilter.prepare (spec);

    outputGain.setRampDurationSeconds (0.05);
    outputGain.prepare (spec);

    toneCutoff.reset (sampleRate, 0.05);
    toneCutoff.setCurrentAndTargetValue (toneToCutoff (toneParam->load()));

    updateParameters();
    reset();
}

void OverdriveAudioProcessor::releaseResources()
{
    oversampling.reset();
}

bool OverdriveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void OverdriveAudioProcessor::updateParameters()
{
    const auto driveDb = driveParam->load() * maxDriveDb;
    driveGain.setGainDecibels (driveDb);

    // Partially compensate for the loudness the drive stage adds
    outputGain.setGainDecibels (levelParam->load() - 0.3f * driveDb);

    toneCutoff.setTargetValue (toneToCutoff (toneParam->load()));
}

void OverdriveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateParameters();

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    inputTap.push (block);

    inputHighPass.process (context);

    // Nonlinear stage runs at 4x so harmonics above the original Nyquist
    // are filtered out by the decimator instead of folding back as aliasing
    auto osBlock = oversampling->processSamplesUp (block);
    juce::dsp::ProcessContextReplacing<float> osContext (osBlock);

    driveGain.process (osContext);

    const auto biasOffset = std::tanh (clipBias);

    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        auto* samples = osBlock.getChannelPointer (ch);

        for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
            samples[i] = std::tanh (samples[i] + clipBias) - biasOffset;
    }

    oversampling->processSamplesDown (block);

    dcBlocker.process (context);

    // Tone cutoff is smoothed per sample to avoid zipper noise when the knob moves
    if (toneCutoff.isSmoothing())
    {
        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            toneFilter.setCutoffFrequency (toneCutoff.getNextValue());

            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
                block.setSample ((int) ch, (int) i, toneFilter.processSample ((int) ch, block.getSample ((int) ch, (int) i)));
        }
    }
    else
    {
        toneFilter.setCutoffFrequency (toneCutoff.getTargetValue());
        toneFilter.process (context);
    }

    outputGain.process (context);

    outputTap.push (block);
}

void OverdriveAudioProcessor::reset()
{
    if (oversampling != nullptr)
        oversampling->reset();

    inputHighPass.reset();
    driveGain.reset();
    dcBlocker.reset();
    toneFilter.reset();
    outputGain.reset();
}

juce::AudioProcessorEditor* OverdriveAudioProcessor::createEditor()
{
    return new OverdriveAudioProcessorEditor (*this);
}

void OverdriveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OverdriveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OverdriveAudioProcessor();
}
