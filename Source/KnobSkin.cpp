#include "KnobSkin.h"
#include "Assets.h"

namespace
{
    void tintDrawable (juce::Drawable& d, juce::Colour c)
    {
        if (auto* shape = dynamic_cast<juce::DrawableShape*> (&d))
        {
            shape->setFill (juce::FillType (c));
            shape->setStrokeFill (juce::FillType (c));
        }
        for (auto* child : d.getChildren())
            if (auto* cd = dynamic_cast<juce::Drawable*> (child))
                tintDrawable (*cd, c);
    }
}

KnobSkin::KnobSkin (FreaxVolumeAudioProcessor& p)
    : processor (p)
{
    setLookAndFeel (&lnf);
    setOpaque (true);

    logo = juce::Drawable::createFromImageData (Assets::freaxment_logo_svg, (size_t) Assets::freaxment_logo_svgSize);
    if (logo != nullptr) tintDrawable (*logo, Theme::accent);

    setupKnob (volumeKnob, ParamID::volume, "Output level: silence at 0 %, 0 dB in the middle, +10 dB at 100 % (double-click to reset)");
    setupKnob (widthKnob,  ParamID::width,  "Stereo width: 0 % = mono, 100 % = untouched, 300 % = side signal x3 (double-click to reset)");

    for (auto* b : { &muteButton, &monoButton })
    {
        b->setClickingTogglesState (true);
        b->setWantsKeyboardFocus (false);
        b->setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible (*b);
    }
    muteButton.setTooltip ("Silence the output (20 ms fade, no click)");
    monoButton.setTooltip ("Output the mid signal only: L = R (overrides Width)");

    auto& state = processor.getState();
    volumeAttachment = std::make_unique<SliderAttachment> (state, ParamID::volume, volumeKnob);
    widthAttachment  = std::make_unique<SliderAttachment> (state, ParamID::width,  widthKnob);
    muteAttachment   = std::make_unique<ButtonAttachment> (state, ParamID::mute,   muteButton);
    monoAttachment   = std::make_unique<ButtonAttachment> (state, ParamID::mono,   monoButton);
}

KnobSkin::~KnobSkin()
{
    setLookAndFeel (nullptr);
}

void KnobSkin::setupKnob (juce::Slider& knob, const juce::String& paramID, const juce::String& tooltip)
{
    auto& state = processor.getState();
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 22);
    knob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    const auto defaultValue = state.getParameterRange (paramID).convertFrom0to1 (state.getParameter (paramID)->getDefaultValue());
    knob.setDoubleClickReturnValue (true, (double) defaultValue);
    knob.setTooltip (tooltip);
    knob.setWantsKeyboardFocus (false);
    addAndMakeVisible (knob);
}

void KnobSkin::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (headerHeight);
    r = r.reduced (margin, 0).withTrimmedTop (2).withTrimmedBottom (margin);

    auto buttonRow = r.removeFromBottom (34);
    r.removeFromBottom (12);

    volumeColumn = r.removeFromLeft (r.getWidth() / 2);
    widthColumn  = r;

    auto layoutKnob = [] (juce::Rectangle<int> column, juce::Slider& knob)
    {
        column.removeFromTop (18);                                     // caption row (painted)
        const int side = juce::jmin (column.getWidth(), column.getHeight() - 24);
        knob.setBounds (juce::Rectangle<int> (side, side + 24).withCentre (column.getCentre()));
    };
    layoutKnob (volumeColumn, volumeKnob);
    layoutKnob (widthColumn,  widthKnob);

    const int buttonW = 150;
    muteButton.setBounds (buttonRow.withWidth (buttonW).withCentre ({ volumeColumn.getCentreX(), buttonRow.getCentreY() }));
    monoButton.setBounds (buttonRow.withWidth (buttonW).withCentre ({ widthColumn.getCentreX(),  buttonRow.getCentreY() }));
}

void KnobSkin::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);

    auto bar = getLocalBounds().removeFromTop (headerHeight);
    g.setColour (Theme::border);
    g.fillRect (bar.getX(), bar.getBottom() - 1, bar.getWidth(), 1);

    juce::GlyphArrangement ga;
    ga.addLineOfText (Theme::black (14.0f), "FreaxVolume", (float) margin, (float) bar.getCentreY() - 2.0f);
    g.setColour (Theme::accent);
    ga.draw (g);
    g.setColour (Theme::text3);
    g.setFont (Theme::ui (10.0f));
    g.drawText ("CHANNEL UTILITY", margin, bar.getCentreY() + 3, 140, 12, juce::Justification::centredLeft);

    if (logo != nullptr)
    {
        const int logoH = headerHeight - 18;
        const auto b = logo->getDrawableBounds();
        const int logoW = b.getHeight() > 0.0f ? (int) std::round (logoH * b.getWidth() / b.getHeight()) : 0;
        auto area = juce::Rectangle<int> (getWidth() - margin - logoW, 0, logoW, headerHeight).withSizeKeepingCentre (logoW, logoH);
        logo->drawWithin (g, area.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }

    // captions above the knobs
    g.setColour (Theme::text2);
    g.setFont (Theme::bold (11.0f));
    g.drawText ("VOLUME", volumeColumn.removeFromTop (18), juce::Justification::centred);
    g.drawText ("WIDTH",  widthColumn.removeFromTop (18),  juce::Justification::centred);

    // column divider
    g.setColour (Theme::border);
    g.fillRect (getWidth() / 2, headerHeight + 10, 1, getHeight() - headerHeight - 24);
}
