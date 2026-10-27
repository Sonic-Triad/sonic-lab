#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <triad_shared/triad_shared.h>

class OverdriveAudioProcessor final : public juce::AudioProcessor
{
public:
    OverdriveAudioProcessor();
    ~OverdriveAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Dry input and final output, read by the editor's spectrum analyzer
    SpectrumTap inputTap, outputTap;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateParameters();

    // 2 stages of 2x = 4x oversampling around the nonlinearity
    static constexpr size_t oversamplingFactorLog2 = 2;

    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* toneParam  = nullptr;
    std::atomic<float>* levelParam = nullptr;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    juce::dsp::StateVariableTPTFilter<float> inputHighPass;  // tightens low end before clipping
    juce::dsp::Gain<float> driveGain;                        // runs at the oversampled rate
    juce::dsp::StateVariableTPTFilter<float> dcBlocker;      // removes DC from asymmetric clipping
    juce::dsp::StateVariableTPTFilter<float> toneFilter;
    juce::dsp::Gain<float> outputGain;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> toneCutoff { 4000.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverdriveAudioProcessor)
};
