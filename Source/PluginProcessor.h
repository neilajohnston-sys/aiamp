#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "SpringReverb.h"

class Chime30Processor : public juce::AudioProcessor
{
public:
    Chime30Processor();
    ~Chime30Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Chime 30"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    struct Channel
    {
        Filter bright, bass, scoop, treble, chime, dc1, dc2, dc3, cut1, cut2;
        Filter cabHP, cabLP1, cabLP2, cabPeak, cabLow;
        SpringReverb spring;
        float sagEnv = 0.0f;

        void resetAll()
        {
            for (auto* f : { &bright, &bass, &scoop, &treble, &chime, &dc1, &dc2, &dc3, &cut1, &cut2,
                             &cabHP, &cabLP1, &cabLP2, &cabPeak, &cabLow })
                f->reset();
            sagEnv = 0.0f;
        }
    };

    void updateTone (bool force);
    float getParam (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    std::array<Channel, 2> chans;
    juce::dsp::Oversampling<float> oversampler;

    juce::SmoothedValue<float> volSm, masterSm, revSm, depthSm, outSm;
    std::array<float, 5> lastTone {};

    double sr2 = 88200.0;
    float tremPhase = 0.0f, tremInc = 0.0f;
    float sagAtk = 0.01f, sagRel = 0.0001f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chime30Processor)
};
