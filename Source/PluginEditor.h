#pragma once
#include "PluginProcessor.h"
#include <array>

class ChimeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChimeLookAndFeel()
    {
        setColour (juce::ToggleButton::textColourId, juce::Colour (0xff15110a));
        setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff15110a));
        setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff6b6048));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (10.0f);
        const float r = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        // 0-10 tick marks
        g.setColour (juce::Colour (0xff2a2418));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            const float s = std::sin (a), co = std::cos (a);
            g.drawLine (c.x + s * r * 1.05f, c.y - co * r * 1.05f,
                        c.x + s * r * 1.22f, c.y - co * r * 1.22f, i % 5 == 0 ? 2.0f : 1.2f);
        }

        // Gold skirt
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff3e2a6), c.x - r * 0.5f, c.y - r * 0.6f,
                                                 juce::Colour (0xff6e5a22), c.x + r * 0.6f, c.y + r * 0.7f, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

        // Black cap
        const float cr = r * 0.68f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2a28), c.x - cr * 0.4f, c.y - cr * 0.5f,
                                                 juce::Colour (0xff0b0b0a), c.x + cr * 0.5f, c.y + cr * 0.6f, false));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);

        // Cream pointer
        juce::Path p;
        p.addRoundedRectangle (-1.8f, -r * 0.66f, 3.6f, r * 0.42f, 1.4f);
        g.setColour (juce::Colour (0xfffff3d0));
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }
};

class Chime30Editor : public juce::AudioProcessorEditor
{
public:
    explicit Chime30Editor (Chime30Processor&);
    ~Chime30Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAtt> att;
    };

    Chime30Processor& proc;
    ChimeLookAndFeel laf; // declared before the controls so it outlives them

    static constexpr int numKnobs = 9;
    std::array<Knob, numKnobs> knobs;
    juce::ToggleButton brilliantButton { "Brilliant" };
    juce::ToggleButton cabButton { "Cab Sim" };
    std::unique_ptr<ButtonAtt> brilliantAtt, cabAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chime30Editor)
};
