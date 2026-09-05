#include "MinimalistSkin.h"
#include "BinaryData.h"

namespace
{
    // Paints every shape of a drawable in one flat colour.
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

MinimalistSkin::MinimalistSkin (SimpletonAudioProcessor& p)
    : processor (p)
{
    logo = juce::Drawable::createFromImageData (BinaryData::freaxment_logo_svg, BinaryData::freaxment_logo_svgSize);
    jassert (logo != nullptr);

    if (logo != nullptr)
        tintDrawable (*logo, Palette::purpleDark);

    setupKnob (volumeKnob, volumeCaption, volumeValue, "Volume", Palette::greenDark,  Palette::greenLight,  50.0);
    setupKnob (widthKnob,  widthCaption,  widthValue,  "Width",  Palette::purpleDark, Palette::purpleLight, 100.0);

    muteButton.setClickingTogglesState (true);
    muteButton.setColour (juce::TextButton::buttonOnColourId, Palette::greenDark);
    muteButton.setColour (juce::TextButton::textColourOnId,   Palette::ink);     // dark text reads better on bright green
    monoButton.setClickingTogglesState (true);
    monoButton.setColour (juce::TextButton::buttonOnColourId, Palette::purpleDark);
    monoButton.setColour (juce::TextButton::textColourOnId,   Palette::paper);

    for (auto* b : { &muteButton, &monoButton })
    {
        b->onClick       = [this] { refreshDimming(); };
        b->onStateChange = [this] { refreshDimming(); };
        addAndMakeVisible (*b);
    }

    auto& state = processor.getState();
    volumeAttachment = std::make_unique<SliderAttachment> (state, ParamID::volume, volumeKnob);
    widthAttachment  = std::make_unique<SliderAttachment> (state, ParamID::width,  widthKnob);
    muteAttachment   = std::make_unique<ButtonAttachment> (state, ParamID::mute,   muteButton);
    monoAttachment   = std::make_unique<ButtonAttachment> (state, ParamID::mono,   monoButton);

    refreshValueLabel (volumeKnob, volumeValue);
    refreshValueLabel (widthKnob,  widthValue);
    refreshDimming();
}

void MinimalistSkin::configureConstrainer (juce::ComponentBoundsConstrainer& constrainer) const
{
    constrainer.setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    constrainer.setSizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 3, baseHeight * 3);
}

void MinimalistSkin::setupKnob (juce::Slider& knob, juce::Label& caption, juce::Label& value,
                                const juce::String& captionText, juce::Colour accentBelow,
                                juce::Colour accentAbove, double neutralValue)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                              juce::MathConstants<float>::pi * 2.75f, true);
    knob.setDoubleClickReturnValue (true, neutralValue);  // double-click = back to neutral
    knob.setColour (juce::Slider::trackColourId, accentBelow);              // arc below the middle
    knob.setColour (juce::Slider::rotarySliderFillColourId, accentAbove);   // arc above the middle
    knob.onValueChange = [this, &knob, &value] { refreshValueLabel (knob, value); };
    addAndMakeVisible (knob);

    caption.setText (captionText.toUpperCase(), juce::dontSendNotification);
    caption.setJustificationType (juce::Justification::centred);
    caption.setColour (juce::Label::textColourId, Palette::muted);
    caption.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (caption);

    value.setJustificationType (juce::Justification::centred);
    value.setEditable (false, true, false);               // double-click to type a value
    value.setTooltip ("Double-click to type a value");
    value.onTextChange = [this, &knob, &value]
    {
        knob.setValue (knob.getValueFromText (value.getText()), juce::sendNotificationSync);
        refreshValueLabel (knob, value);
    };
    addAndMakeVisible (value);
}

void MinimalistSkin::refreshValueLabel (juce::Slider& knob, juce::Label& value)
{
    value.setText (knob.getTextFromValue (knob.getValue()), juce::dontSendNotification);
}

void MinimalistSkin::refreshDimming()
{
    // A knob that currently has no effect fades back a little.
    const float volumeAlpha = muteButton.getToggleState() ? 0.45f : 1.0f;
    const float widthAlpha  = monoButton.getToggleState() ? 0.45f : 1.0f;

    volumeKnob.setAlpha (volumeAlpha);
    volumeValue.setAlpha (volumeAlpha);
    widthKnob.setAlpha (widthAlpha);
    widthValue.setAlpha (widthAlpha);
}

void MinimalistSkin::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (Palette::paper.brighter (0.03f), 0.0f, 0.0f,
                                             Palette::paper.darker (0.03f), 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Title: "simpleton" + Freaxment logo, centred as one group
    {
        const auto titleFont  = makeFont (scale * 24.0f, true);
        const juce::String titleText ("simpleton");
        const float textWidth = juce::GlyphArrangement::getStringWidth (titleFont, titleText);
        const float gap       = scale * 12.0f;

        const float logoHeight = scale * 26.0f;
        float logoWidth = 0.0f;

        if (logo != nullptr)
        {
            const auto logoBounds = logo->getDrawableBounds();
            logoWidth = logoBounds.getHeight() > 0.0f ? logoHeight * logoBounds.getWidth() / logoBounds.getHeight() : 0.0f;
        }

        const float groupWidth = textWidth + (logoWidth > 0.0f ? gap + logoWidth : 0.0f);
        auto row = titleArea.toFloat().withSizeKeepingCentre (groupWidth, (float) titleArea.getHeight());

        g.setFont (titleFont);
        g.setColour (Palette::ink);
        g.drawText (titleText, row.removeFromLeft (textWidth), juce::Justification::centredLeft, false);

        if (logo != nullptr && logoWidth > 0.0f)
        {
            row.removeFromLeft (gap);
            const auto logoArea = row.withSizeKeepingCentre (logoWidth, logoHeight).translated (0.0f, scale * 1.0f);
            logo->drawWithin (g, logoArea, juce::RectanglePlacement::centred, 1.0f);
        }
    }

    // Divider with a small two-colour accent in the middle
    const float y      = (float) titleArea.getBottom() - 0.5f;
    const float margin = scale * 28.0f;
    const float cx     = (float) getWidth() * 0.5f;
    const float accentHalf = scale * 12.0f;

    g.setColour (Palette::ink.withAlpha (0.12f));
    g.drawLine (margin, y, cx - accentHalf - scale * 4.0f, y, 1.0f);
    g.drawLine (cx + accentHalf + scale * 4.0f, y, (float) getWidth() - margin, y, 1.0f);

    g.setColour (Palette::greenDark);
    g.drawLine (cx - accentHalf, y, cx, y, scale * 2.0f);
    g.setColour (Palette::purpleDark);
    g.drawLine (cx, y, cx + accentHalf, y, scale * 2.0f);
}

void MinimalistSkin::resized()
{
    scale = (float) getWidth() / (float) baseWidth;

    auto area = getLocalBounds();
    titleArea = area.removeFromTop (scaled (72.0f)).withTrimmedTop (scaled (10.0f));

    auto buttonRow = area.removeFromBottom (scaled (96.0f)).reduced (scaled (28.0f), scaled (24.0f));
    muteButton.setBounds (buttonRow.removeFromLeft (buttonRow.getWidth() / 2).reduced (scaled (10.0f), 0));
    monoButton.setBounds (buttonRow.reduced (scaled (10.0f), 0));

    auto knobRow = area.reduced (scaled (18.0f), scaled (14.0f));
    auto left  = knobRow.removeFromLeft (knobRow.getWidth() / 2);
    auto right = knobRow;

    const auto captionFont = makeFont (scale * 12.0f, true, 0.14f);
    const auto valueFont   = makeFont (scale * 16.0f, false);

    auto layoutColumn = [&] (juce::Rectangle<int> column, juce::Slider& knob, juce::Label& caption, juce::Label& value)
    {
        caption.setFont (captionFont);
        caption.setBounds (column.removeFromTop (scaled (22.0f)));

        value.setFont (valueFont);
        value.setBounds (column.removeFromBottom (scaled (28.0f)).reduced (scaled (14.0f), 0));

        const int side = juce::jmin (column.getWidth(), column.getHeight()) - scaled (8.0f);
        knob.setBounds (juce::Rectangle<int> (side, side).withCentre (column.getCentre()));
    };

    layoutColumn (left,  volumeKnob, volumeCaption, volumeValue);
    layoutColumn (right, widthKnob,  widthCaption,  widthValue);
}
