#include "PluginEditor.h"

namespace
{
    struct KnobDef { const char* id; const char* text; const char* suffix; };

    // Row 1 (6 knobs): volume, treble, bass, cut, master, output
    // Row 2: reverb, speed, depth, then the two toggles
    constexpr KnobDef defs[9] = {
        { "volume", "VOLUME", ""    },
        { "treble", "TREBLE", ""    },
        { "bass",   "BASS",   ""    },
        { "cut",    "CUT",    ""    },
        { "master", "MASTER", ""    },
        { "output", "OUTPUT", " dB" },
        { "reverb", "REVERB", ""    },
        { "speed",  "SPEED",  " Hz" },
        { "depth",  "DEPTH",  ""    },
    };
}

Chime30Editor::Chime30Editor (Chime30Processor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < numKnobs; ++i)
    {
        auto& k = knobs[(size_t) i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        k.slider.setNumDecimalPlacesToDisplay (1);
        k.slider.setTextValueSuffix (defs[i].suffix);
        k.slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xff15110a));
        k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k.slider.setLookAndFeel (&laf);
        addAndMakeVisible (k.slider);

        k.label.setText (defs[i].text, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        k.label.setColour (juce::Label::textColourId, juce::Colour (0xff15110a));
        addAndMakeVisible (k.label);

        k.att = std::make_unique<SliderAtt> (proc.apvts, defs[i].id, k.slider);
    }

    for (auto* b : { &brilliantButton, &cabButton })
    {
        b->setLookAndFeel (&laf);
        addAndMakeVisible (*b);
    }
    brilliantAtt = std::make_unique<ButtonAtt> (proc.apvts, "brilliant", brilliantButton);
    cabAtt       = std::make_unique<ButtonAtt> (proc.apvts, "cab", cabButton);

    setSize (780, 430);
}

Chime30Editor::~Chime30Editor()
{
    for (auto& k : knobs)
        k.slider.setLookAndFeel (nullptr);
    brilliantButton.setLookAndFeel (nullptr);
    cabButton.setLookAndFeel (nullptr);
}

void Chime30Editor::paint (juce::Graphics& g)
{
    // Black vinyl
    g.fillAll (juce::Colour (0xff0f0f10));

    // Diamond lattice across the header, like a grille cloth
    g.saveState();
    g.reduceClipRegion (0, 0, getWidth(), 76);
    g.setColour (juce::Colour (0xff2b2a2d));
    for (int x = -80; x < getWidth() + 80; x += 18)
    {
        g.drawLine ((float) x, 0.0f, (float) x + 76.0f, 76.0f, 1.0f);
        g.drawLine ((float) x, 76.0f, (float) x + 76.0f, 0.0f, 1.0f);
    }
    g.restoreState();

    // Title
    g.setColour (juce::Colour (0xffe9d28a));
    g.setFont (juce::FontOptions (40.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("Chime 30", 28, 8, 400, 50, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffb9a46a));
    g.setFont (juce::FontOptions (12.0f, juce::Font::plain));
    g.drawText ("TOP BOOST-STYLE CHIME  \xc2\xb7  CLASS A  \xc2\xb7  SPRING REVERB  \xc2\xb7  TREMOLO",
                30, 52, 560, 16, juce::Justification::centredLeft);

    // Gold trim + cream control plate
    auto plate = getLocalBounds().toFloat().withTrimmedTop (74.0f).reduced (10.0f, 0.0f).withTrimmedBottom (10.0f);
    g.setColour (juce::Colour (0xffc9a94a));
    g.fillRect (getLocalBounds().toFloat().removeFromTop (76.0f).removeFromBottom (3.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeadcae), 0.0f, plate.getY(),
                                             juce::Colour (0xffb39d60), 0.0f, plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 6.0f);
    g.setColour (juce::Colour (0xff6e5d2e));
    g.drawRoundedRectangle (plate.reduced (0.5f), 6.0f, 1.2f);
}

void Chime30Editor::resized()
{
    auto area = getLocalBounds().withTrimmedTop (84).reduced (20, 4).withTrimmedBottom (14);
    const int cols = 6;
    const int rowH = area.getHeight() / 2;
    const int colW = area.getWidth() / cols;

    for (int i = 0; i < numKnobs; ++i)
    {
        const int row = i / cols, col = i % cols;
        auto cell = juce::Rectangle<int> (area.getX() + col * colW, area.getY() + row * rowH, colW, rowH);
        auto& k = knobs[(size_t) i];
        k.label.setBounds (cell.removeFromTop (20));
        k.slider.setBounds (cell);
    }

    // Toggles in row 2, columns 4 and 5
    auto toggleCell = [&] (int col)
    {
        return juce::Rectangle<int> (area.getX() + col * colW, area.getY() + rowH, colW, rowH)
                   .withSizeKeepingCentre (colW - 16, 28);
    };
    brilliantButton.setBounds (toggleCell (3));
    cabButton.setBounds (toggleCell (4));
}
