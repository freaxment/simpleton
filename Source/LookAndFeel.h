#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Palette
{
    const juce::Colour paper  { 0xfff4efe6 };   // warm off-white background
    const juce::Colour knob   { 0xffe9e2d5 };   // knob body
    const juce::Colour track  { 0xffd9d0c1 };   // idle part of the arc
    const juce::Colour ink    { 0xff1f1b18 };   // text, pointer
    const juce::Colour muted  { 0xff8c8378 };   // captions, secondary text
    const juce::Colour greenLight  { 0xff2bcf3c };   // volume arc above the middle
    const juce::Colour greenDark   { 0xff11b41e };   // volume arc below the middle, active Mute
    const juce::Colour purpleLight { 0xff6f64cc };   // width arc above the middle
    const juce::Colour purpleDark  { 0xff3f3590 };   // width arc below the middle, active Mono
}

inline juce::Font makeFont (float height, bool bold = false, float kerning = 0.0f)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain))
               .withExtraKerningFactor (kerning);
}

class SimpletonLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SimpletonLookAndFeel()
    {
        setColour (juce::Label::textColourId,                    Palette::ink);
        setColour (juce::Label::textWhenEditingColourId,         Palette::ink);
        setColour (juce::Label::backgroundWhenEditingColourId,   juce::Colours::transparentBlack);
        setColour (juce::Label::outlineWhenEditingColourId,      juce::Colours::transparentBlack);

        setColour (juce::TextEditor::textColourId,               Palette::ink);
        setColour (juce::TextEditor::highlightedTextColourId,    Palette::ink);
        setColour (juce::TextEditor::highlightColourId,          Palette::purpleDark.withAlpha (0.25f));
        setColour (juce::TextEditor::backgroundColourId,         juce::Colours::transparentBlack);
        setColour (juce::TextEditor::outlineColourId,            juce::Colours::transparentBlack);
        setColour (juce::TextEditor::focusedOutlineColourId,     juce::Colours::transparentBlack);
        setColour (juce::CaretComponent::caretColourId,          Palette::ink);

        setColour (juce::Slider::rotarySliderOutlineColourId,    Palette::track);
        setColour (juce::Slider::rotarySliderFillColourId,       Palette::greenLight);   // above the middle
        setColour (juce::Slider::trackColourId,                  Palette::greenDark);    // below the middle
        setColour (juce::Slider::thumbColourId,                  Palette::ink);

        setColour (juce::TextButton::buttonColourId,             juce::Colours::transparentBlack);
        setColour (juce::TextButton::buttonOnColourId,           Palette::ink);
        setColour (juce::TextButton::textColourOffId,            Palette::ink);
        setColour (juce::TextButton::textColourOnId,             Palette::paper);
    }

    //==============================================================================
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider& slider) override
    {
        const auto bounds  = juce::Rectangle<int> (x, y, width, height).toFloat();
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre  = bounds.getCentre();

        const float lineW     = juce::jmax (2.0f, radius * 0.11f);
        const float arcRadius = radius - lineW * 1.6f - 1.0f;      // leaves room for the neutral marker
        const float midAngle  = (startAngle + endAngle) * 0.5f;            // 12 o'clock = neutral
        const float angle     = startAngle + sliderPos * (endAngle - startAngle);

        // Idle track
        {
            juce::Path track;
            track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
            g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
            g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Value arc, grows from the neutral position in either direction:
        // dark shade below the middle, light shade above it.
        if (std::abs (angle - midAngle) > 0.001f)
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                 juce::jmin (midAngle, angle), juce::jmax (midAngle, angle), true);
            g.setColour (slider.findColour (angle < midAngle ? juce::Slider::trackColourId
                                                             : juce::Slider::rotarySliderFillColourId));
            g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Neutral marker just outside the arc
        {
            const auto p1 = centre.getPointOnCircumference (arcRadius + lineW * 0.9f, midAngle);
            const auto p2 = centre.getPointOnCircumference (arcRadius + lineW * 1.5f, midAngle);
            g.setColour (Palette::muted.withAlpha (0.8f));
            g.drawLine (juce::Line<float> (p1, p2), juce::jmax (1.0f, lineW * 0.25f));
        }

        // Knob body
        const float bodyRadius = arcRadius - lineW * 1.45f;
        const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

        g.setColour (Palette::ink.withAlpha (0.06f));
        g.fillEllipse (body.translated (0.0f, lineW * 0.35f));
        g.setColour (Palette::knob);
        g.fillEllipse (body);
        g.setColour (Palette::ink.withAlpha (0.10f));
        g.drawEllipse (body, 1.0f);

        // Pointer
        {
            juce::Path pointer;
            pointer.startNewSubPath (centre.getPointOnCircumference (bodyRadius * 0.42f, angle));
            pointer.lineTo          (centre.getPointOnCircumference (bodyRadius * 0.82f, angle));
            g.setColour (slider.findColour (juce::Slider::thumbColourId));
            g.strokePath (pointer, juce::PathStrokeType (juce::jmax (2.0f, lineW * 0.55f),
                                                         juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }
    }

    //==============================================================================
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return makeFont (juce::jmax (11.0f, (float) buttonHeight * 0.30f), true, 0.10f);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isHighlighted, bool isDown) override
    {
        const auto bounds  = button.getLocalBounds().toFloat().reduced (1.5f);
        const float corner = bounds.getHeight() * 0.5f;
        const bool on      = button.getToggleState();

        if (on)
        {
            auto fill = button.findColour (juce::TextButton::buttonOnColourId);

            if (isDown)
                fill = fill.darker (0.15f);
            else if (isHighlighted)
                fill = fill.brighter (0.06f);

            g.setColour (fill);
            g.fillRoundedRectangle (bounds, corner);
        }
        else
        {
            if (isDown)
            {
                g.setColour (Palette::ink.withAlpha (0.08f));
                g.fillRoundedRectangle (bounds, corner);
            }

            g.setColour (Palette::ink.withAlpha (isHighlighted ? 0.60f : 0.28f));
            g.drawRoundedRectangle (bounds, corner, 1.5f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        const bool on = button.getToggleState();

        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.setColour (button.findColour (on ? juce::TextButton::textColourOnId
                                           : juce::TextButton::textColourOffId));
        g.drawText (button.getButtonText().toUpperCase(), button.getLocalBounds(),
                    juce::Justification::centred, false);
    }

    //==============================================================================
    void fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor&) override
    {
        const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
        g.setColour (Palette::knob);
        g.fillRoundedRectangle (r, 6.0f);
    }

    void drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor&) override
    {
        const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
        g.setColour (Palette::ink.withAlpha (0.25f));
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }

    void drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override
    {
        g.setColour (Palette::muted.withAlpha ((isMouseOver || isMouseDragging) ? 0.9f : 0.35f));

        for (int i = 1; i <= 2; ++i)
        {
            const float o = (float) i * 4.0f;
            g.drawLine ((float) w - o, (float) h - 2.0f, (float) w - 2.0f, (float) h - o, 1.2f);
        }
    }
};
