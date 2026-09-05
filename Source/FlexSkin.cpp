#include "FlexSkin.h"
#include "BinaryData.h"

namespace
{
    const juce::Colour lime  { 0xffc8ff00 };
    const juce::Colour panel { 0xff111315 };
    const juce::Colour ink   { 0xff060707 };
    const juce::Colour soft  { 0xffa6afa8 };

    constexpr float knobStart = juce::MathConstants<float>::pi * 1.24f;
    constexpr float knobEnd   = juce::MathConstants<float>::pi * 2.76f;

    void tintDrawable (juce::Drawable& drawable, juce::Colour colour)
    {
        if (auto* shape = dynamic_cast<juce::DrawableShape*> (&drawable))
        {
            shape->setFill (juce::FillType (colour));
            shape->setStrokeFill (juce::FillType (colour));
        }

        for (auto* child : drawable.getChildren())
            if (auto* childDrawable = dynamic_cast<juce::Drawable*> (child))
                tintDrawable (*childDrawable, colour);
    }
}

//==============================================================================
FlexSkin::Knob::Knob()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (knobStart, knobEnd, true);   // same sweep as the arc drawn below
}

void FlexSkin::Knob::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced (8.0f);
    const auto centre = area.getCentre();
    const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;

    // Knob position 0..1 regardless of the parameter's own range (0..100 % / 0..300 %)
    const float position = (float) valueToProportionOfLength (getValue());
    const float angle = knobStart + position * (knobEnd - knobStart);

    g.setColour (juce::Colour (0xff22272a));
    g.fillEllipse (area);
    g.setColour (juce::Colour (0xff050606));
    g.fillEllipse (area.reduced (7.0f));

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius - 3.5f, radius - 3.5f, 0.0f, knobStart, angle, true);
    g.setColour (lime);
    g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // JUCE rotary angles run clockwise from 12 o'clock: same convention as the arc.
    const auto tip = centre.getPointOnCircumference (radius - 18.0f, angle);
    g.setColour (juce::Colours::white);
    g.drawLine (centre.x, centre.y, tip.x, tip.y, 3.0f);

    g.setColour (juce::Colour (0x226eff00));
    g.drawEllipse (area, 1.0f);
}

//==============================================================================
FlexSkin::Button::Button (const juce::String& text) : TextButton (text)
{
    setClickingTogglesState (true);
}

void FlexSkin::Button::paintButton (juce::Graphics& g, bool over, bool)
{
    auto box = getLocalBounds().toFloat().reduced (1.0f);
    const auto active = getToggleState();

    g.setColour (active ? lime : juce::Colour (0xff171b1d));
    g.fillRoundedRectangle (box, 5.0f);
    g.setColour (active ? lime.brighter (0.2f) : juce::Colour (0xff41494b));
    g.drawRoundedRectangle (box, 5.0f, over ? 2.0f : 1.0f);
    g.setColour (active ? ink : juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (getButtonText(), getLocalBounds(), juce::Justification::centred, 1);
}

//==============================================================================
FlexSkin::FlexSkin (SimpletonAudioProcessor& p)
    : processor (p)
{
    logo = juce::Drawable::createFromImageData (BinaryData::freaxment_logo_svg, BinaryData::freaxment_logo_svgSize);
    jassert (logo != nullptr);

    if (logo != nullptr)
        tintDrawable (*logo, lime);

    for (auto* knob : { &gain, &width })
    {
        knob->onValueChange = [this] { refreshLabels(); };
        addAndMakeVisible (*knob);
    }

    for (auto* button : { &mute, &mono })
        addAndMakeVisible (*button);

    for (auto* label : { &gainValue, &widthValue })
    {
        label->setJustificationType (juce::Justification::centred);
        label->setColour (juce::Label::textColourId, juce::Colours::white);
        label->setFont (juce::FontOptions (14.0f, juce::Font::bold));
        label->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*label);
    }

    auto& state = processor.getState();
    gainAttachment  = std::make_unique<SliderAttachment> (state, ParamID::volume, gain);
    widthAttachment = std::make_unique<SliderAttachment> (state, ParamID::width,  width);
    muteAttachment  = std::make_unique<ButtonAttachment> (state, ParamID::mute,   mute);
    monoAttachment  = std::make_unique<ButtonAttachment> (state, ParamID::mono,   mono);

    gain.setDoubleClickReturnValue (true, 50.0);     // 0 dB
    width.setDoubleClickReturnValue (true, 100.0);   // untouched stereo

    refreshLabels();
}

void FlexSkin::configureConstrainer (juce::ComponentBoundsConstrainer& constrainer) const
{
    constrainer.setFixedAspectRatio (0.0);
    constrainer.setSizeLimits (400, 290, 900, 650);
}

void FlexSkin::refreshLabels()
{
    auto gainText = gain.getTextFromValue (gain.getValue());

    if (gainText.startsWith ("-inf"))
        gainText = juce::String::fromUTF8 ("\xe2\x88\x92\xe2\x88\x9e dB");   // "−∞ dB"

    gainValue.setText (gainText, juce::dontSendNotification);
    widthValue.setText (juce::String (juce::roundToInt (width.getValue())) + "% SIDE", juce::dontSendNotification);
}

void FlexSkin::paint (juce::Graphics& g)
{
    g.fillAll (ink);

    auto bounds = getLocalBounds().toFloat().reduced (10.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (lime);
    g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 3.0f);

    // Title
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText ("SIMPLETON", 29, 23, 260, 36, juce::Justification::left);

    g.setColour (soft);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("ESSENTIAL SIGNAL CONTROL", 30, 57, 220, 16, juce::Justification::left);

    // Freaxment logo, top right
    if (logo != nullptr)
    {
        const auto logoBounds = logo->getDrawableBounds();
        const float logoHeight = 22.0f;
        const float logoWidth = logoBounds.getHeight() > 0.0f ? logoHeight * logoBounds.getWidth() / logoBounds.getHeight() : 0.0f;
        const auto logoArea = juce::Rectangle<float> ((float) getWidth() - 29.0f - logoWidth, 35.0f, logoWidth, logoHeight);
        logo->drawWithin (g, logoArea, juce::RectanglePlacement::centred, 1.0f);
    }

    const int labelY = (int) ((float) getHeight() * 0.24f);
    g.setColour (soft);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("GAIN", getWidth() / 2 - 190, labelY, 120, 20, juce::Justification::centred);
    g.drawText ("WIDTH", getWidth() / 2 + 70, labelY, 120, 20, juce::Justification::centred);

    g.setColour (juce::Colour (0xff2b3233));
    g.drawLine ((float) getWidth() / 2.0f, 100.0f, (float) getWidth() / 2.0f, (float) getHeight() - 28.0f, 1.0f);
}

void FlexSkin::resized()
{
    const int w = getWidth(), h = getHeight();

    gain.setBounds  (w / 2 - 205, 98, 180, 180);
    width.setBounds (w / 2 + 25,  98, 180, 180);
    gainValue.setBounds  (w / 2 - 190, 265, 150, 24);
    widthValue.setBounds (w / 2 + 40,  265, 150, 24);
    mute.setBounds (w / 2 - 190, h - 57, 150, 34);
    mono.setBounds (w / 2 + 40,  h - 57, 150, 34);
}
