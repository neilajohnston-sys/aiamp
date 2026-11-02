#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    constexpr float bias1 = 0.10f;   // first triode stage grid bias
    constexpr float bias2 = 0.14f;   // recovery stage (more asymmetric = more "chime")
    constexpr float biasPwr = 0.10f; // class A power stage
    inline float dB (float db) { return juce::Decibels::decibelsToGain (db); }
}

juce::AudioProcessorValueTreeState::ParameterLayout Chime30Processor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto knob = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<F> (juce::ParameterID { id, 1 }, name, R (0.0f, 10.0f, 0.01f), def));
    };

    knob ("volume", "Volume", 5.0f);
    knob ("treble", "Treble", 6.0f);
    knob ("bass",   "Bass",   4.0f);
    knob ("cut",    "Cut",    3.0f);
    knob ("master", "Master", 6.0f);
    knob ("reverb", "Reverb", 2.0f);
    knob ("depth",  "Depth",  0.0f);

    layout.add (std::make_unique<F> (juce::ParameterID { "speed", 1 }, "Speed",
                                     R (0.5f, 12.0f, 0.01f, 0.6f), 4.5f));
    layout.add (std::make_unique<F> (juce::ParameterID { "output", 1 }, "Output",
                                     R (-24.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "brilliant", 1 }, "Brilliant", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "cab", 1 }, "Cab Sim", true));
    return layout;
}

Chime30Processor::Chime30Processor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout()),
      oversampler (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true)
{
}

bool Chime30Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void Chime30Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr2 = sampleRate * 2.0; // 2x oversampled nonlinear chain

    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();
    setLatencySamples (juce::roundToInt (oversampler.getLatencyInSamples()));

    for (auto* s : { &volSm, &masterSm, &revSm, &depthSm, &outSm })
        s->reset (sr2, 0.03);

    volSm.setCurrentAndTargetValue (getParam ("volume"));
    masterSm.setCurrentAndTargetValue (getParam ("master"));
    revSm.setCurrentAndTargetValue (getParam ("reverb") / 10.0f * 0.9f);
    depthSm.setCurrentAndTargetValue (getParam ("depth") / 10.0f * 0.8f);
    outSm.setCurrentAndTargetValue (dB (getParam ("output")) * 1.4f);

    sagAtk = 1.0f - std::exp (-1.0f / (0.004f * (float) sr2));
    sagRel = 1.0f - std::exp (-1.0f / (0.120f * (float) sr2));
    tremPhase = 0.0f;

    // Fixed voicing: the Top Boost "chime" and the scoop that goes with it
    const auto scoopC = Coeffs::makePeakFilter (sr2, 750.0f, 0.8f, dB (-3.0f));
    const auto chimeC = Coeffs::makePeakFilter (sr2, 3200.0f, 0.9f, dB (2.5f));
    const auto dcBlock = Coeffs::makeHighPass (sr2, 25.0f, 0.707f);

    // Alnico-style 12" cab: tight low end, strong upper-mid bark, smooth top
    const auto cabHP   = Coeffs::makeHighPass (sr2, 90.0f, 0.707f);
    const auto cabLP   = Coeffs::makeLowPass  (sr2, 5600.0f, 0.707f);
    const auto cabPeak = Coeffs::makePeakFilter (sr2, 2200.0f, 1.0f, dB (3.5f));
    const auto cabLow  = Coeffs::makePeakFilter (sr2, 100.0f, 1.2f, dB (2.5f));

    chans[0].spring.prepare (sr2, 33.1f, 41.3f);
    chans[1].spring.prepare (sr2, 35.7f, 44.9f);
    for (auto& c : chans)
        c.spring.setFeedback (0.68f);

    for (auto& c : chans)
    {
        c.scoop.coefficients   = scoopC;
        c.chime.coefficients   = chimeC;
        c.dc1.coefficients     = dcBlock;
        c.dc2.coefficients     = dcBlock;
        c.dc3.coefficients     = dcBlock;
        c.cabHP.coefficients   = cabHP;
        c.cabLP1.coefficients  = cabLP;
        c.cabLP2.coefficients  = cabLP;
        c.cabPeak.coefficients = cabPeak;
        c.cabLow.coefficients  = cabLow;
    }

    updateTone (true);
    for (auto& c : chans)
        c.resetAll();
}

void Chime30Processor::updateTone (bool force)
{
    const std::array<float, 5> t { getParam ("bass"), getParam ("treble"), getParam ("cut"),
                                   getParam ("brilliant"), getParam ("volume") };
    if (! force && t == lastTone)
        return;
    lastTone = t;

    // Top Boost has only Treble and Bass; they interact, so keep the ranges moderate
    const auto bassC = Coeffs::makeLowShelf  (sr2, 140.0f, 0.6f, dB ((t[0] - 5.0f) * 2.2f));
    const auto trebC = Coeffs::makeHighShelf (sr2, 2600.0f, 0.6f, dB ((t[1] - 5.0f) * 2.6f));

    // Cut: a high-cut in the power section. Higher = darker (like the real control)
    const float cutHz = 9000.0f * std::pow (0.2f, t[2] / 10.0f);
    const auto cutC = Coeffs::makeLowPass (sr2, cutHz, 0.707f);

    // Brilliant channel: brighter and more gain; Normal channel: warmer
    const bool brilliant = t[3] > 0.5f;
    const auto brC = Coeffs::makeHighShelf (sr2, 3000.0f, 0.7f, dB (brilliant ? 5.0f : -4.0f));

    for (auto& c : chans)
    {
        c.bass.coefficients   = bassC;
        c.treble.coefficients = trebC;
        c.cut1.coefficients   = cutC;
        c.cut2.coefficients   = cutC;
        c.bright.coefficients = brC;
    }
}

void Chime30Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0)
        return;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    updateTone (false);

    volSm.setTargetValue (getParam ("volume"));
    masterSm.setTargetValue (getParam ("master"));
    revSm.setTargetValue (getParam ("reverb") / 10.0f * 0.9f);
    depthSm.setTargetValue (getParam ("depth") / 10.0f * 0.8f);
    outSm.setTargetValue (dB (getParam ("output")) * 1.4f);
    tremInc = twoPi * getParam ("speed") / (float) sr2;
    const bool cabOn = getParam ("cab") > 0.5f;
    const float chanGain = getParam ("brilliant") > 0.5f ? 1.3f : 0.8f;

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) numCh);
    auto up = oversampler.processSamplesUp (sub);
    const size_t m = up.getNumSamples();

    for (size_t i = 0; i < m; ++i)
    {
        const float vol  = volSm.getNextValue();
        const float mst  = masterSm.getNextValue();
        const float rev  = revSm.getNextValue();
        const float dep  = depthSm.getNextValue();
        const float outG = outSm.getNextValue();

        const float lin = chanGain * (0.5f + 0.15f * vol);
        const float a1  = 0.7f + 0.4f * vol;   // first stage saturation
        const float a2  = 0.8f + 0.2f * vol;   // recovery stage saturation
        const float t1  = std::tanh (a1 * bias1);
        const float t2  = std::tanh (a2 * bias2);
        const float g   = 0.6f + 0.25f * mst;  // class A power-stage drive
        const float tp  = std::tanh (g * biasPwr);

        const float trem = 1.0f - dep * 0.5f * (1.0f + std::sin (tremPhase));
        tremPhase += tremInc;
        if (tremPhase >= twoPi)
            tremPhase -= twoPi;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& c = chans[(size_t) ch];
            float x = up.getChannelPointer ((size_t) ch)[i];

            // Channel voicing + first triode stage
            x = c.bright.processSample (x) * lin;
            x = (std::tanh (a1 * (x + bias1)) - t1) / a1;
            x = c.dc1.processSample (x);

            // Top Boost tone circuit (passive, so it costs level; recovery gain follows)
            x = c.bass.processSample (x);
            x = c.scoop.processSample (x);
            x = c.treble.processSample (x);
            x = c.chime.processSample (x);
            x *= 1.8f;

            // Recovery stage: asymmetric, adds the chimey even harmonics
            x = (std::tanh (a2 * (x + bias2)) - t2) / a2;
            x = c.dc2.processSample (x);

            // Spring reverb, then tremolo
            x += c.spring.process (x) * rev * 1.2f;
            x *= trem;

            // Class A EL84-style power stage: asymmetric, compressive, with sag
            const float lvl = std::abs (x);
            c.sagEnv += (lvl > c.sagEnv ? sagAtk : sagRel) * (lvl - c.sagEnv);
            x *= 1.5f * (1.0f - 0.30f * juce::jmin (c.sagEnv, 1.0f));
            x = (std::tanh (g * (x + biasPwr)) - tp) / g;
            x = c.dc3.processSample (x);

            // Cut control (two cascaded poles for a smooth 24 dB/oct high-cut)
            x = c.cut1.processSample (x);
            x = c.cut2.processSample (x);

            if (cabOn)
            {
                x = c.cabHP.processSample (x);
                x = c.cabLP1.processSample (x);
                x = c.cabLP2.processSample (x);
                x = c.cabPeak.processSample (x);
                x = c.cabLow.processSample (x);
            }

            up.getChannelPointer ((size_t) ch)[i] = x * outG;
        }
    }

    oversampler.processSamplesDown (sub);
}

juce::AudioProcessorEditor* Chime30Processor::createEditor()
{
    return new Chime30Editor (*this);
}

void Chime30Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Chime30Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Chime30Processor();
}
