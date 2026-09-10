#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Assets.h"

// Same dark palette and type system as FreaxKlip / Freaxcalibur / FreaxEasyRec, acid-lime accent.
namespace Theme
{
    const juce::Colour bg            { 0xff1b1b1d };
    const juce::Colour panel         { 0xff222225 };
    const juce::Colour panel2        { 0xff1e1e21 };
    const juce::Colour text          { 0xfff2f2f4 };
    const juce::Colour text2         { 0xffb6b6bc };
    const juce::Colour text3         { 0xff7c7c84 };
    const juce::Colour border        = juce::Colours::white.withAlpha (0.08f);
    const juce::Colour borderStrong  = juce::Colours::white.withAlpha (0.18f);
    const juce::Colour accent        { 0xffe5f55a };
    const juce::Colour accentText    { 0xff1b1b1d };
    const juce::Colour accentSoft    = accent.withAlpha (0.16f);
    const juce::Colour knobTrack     = juce::Colours::white.withAlpha (0.10f);
    const juce::Colour knobBody      { 0xff2a2a2e };

    // TikTok Sans (OFL, embedded). Hierarchy: Expanded Black for the brand, Expanded Bold for the
    // main controls, Expanded SemiBold for secondary controls, Regular / Medium for information text.
    enum class Face { regular, medium, expandedSemiBold, expandedBold, expandedBlack };

    inline juce::Typeface::Ptr typeface (Face face)
    {
        // Deliberately leaked: a static array with a destructor would release the CoreText typefaces while the
        // host process is already exiting, which aborts Ableton Live on quit (std::terminate in ~CoreTextTypeface).
        static juce::Typeface::Ptr* cache = new juce::Typeface::Ptr[5];
        const int i = (int) face;
        if (cache[i] == nullptr)
        {
            const char* data = nullptr; int size = 0;
            switch (face)
            {
                case Face::regular:          data = Assets::TikTokSansRegular_ttf;          size = Assets::TikTokSansRegular_ttfSize; break;
                case Face::medium:           data = Assets::TikTokSansMedium_ttf;           size = Assets::TikTokSansMedium_ttfSize; break;
                case Face::expandedSemiBold: data = Assets::TikTokSansExpandedSemiBold_ttf; size = Assets::TikTokSansExpandedSemiBold_ttfSize; break;
                case Face::expandedBold:     data = Assets::TikTokSansExpandedBold_ttf;     size = Assets::TikTokSansExpandedBold_ttfSize; break;
                case Face::expandedBlack:    data = Assets::TikTokSansExpandedBlack_ttf;    size = Assets::TikTokSansExpandedBlack_ttfSize; break;
            }
            cache[i] = juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        }
        return cache[i];
    }

    inline juce::Font font (Face face, float size)
    {
        return juce::Font (juce::FontOptions (typeface (face)).withHeight (size));
    }
    inline juce::Font ui (float size, bool medium = false) { return font (medium ? Face::medium : Face::regular, size); }
    inline juce::Font expanded (float size) { return font (Face::expandedSemiBold, size); }
    inline juce::Font bold (float size) { return font (Face::expandedBold, size); }
    inline juce::Font black (float size) { return font (Face::expandedBlack, size); }
}

class FreaxVolumeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FreaxVolumeLookAndFeel()
    {
        setDefaultSansSerifTypeface (Theme::typeface (Theme::Face::regular));
        setColour (juce::ResizableWindow::backgroundColourId, Theme::bg);
        setColour (juce::Slider::textBoxTextColourId, Theme::text);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, Theme::accentSoft);
        setColour (juce::Label::textWhenEditingColourId, Theme::accent);
        setColour (juce::TextEditor::highlightColourId, Theme::accentSoft);
        setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::CaretComponent::caretColourId, Theme::accent);
        setColour (juce::TooltipWindow::backgroundColourId, Theme::text);
        setColour (juce::TooltipWindow::textColourId, Theme::bg);
        setColour (juce::TooltipWindow::outlineColourId, juce::Colours::transparentBlack);
    }

    // Horizontal slider: thin track, accent fill from the default position, round thumb.
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const float cy = (float) y + (float) h * 0.5f;
        const float x0 = (float) x + 8.0f, x1 = (float) x + (float) w - 8.0f;
        const float trackH = 4.0f;
        g.setColour (Theme::knobTrack);
        g.fillRoundedRectangle (x0, cy - trackH * 0.5f, x1 - x0, trackH, trackH * 0.5f);

        const float anchor = x0 + (float) s.valueToProportionOfLength (s.getDoubleClickReturnValue()) * (x1 - x0);
        const float fx = juce::jlimit (x0, x1, pos);
        if (std::abs (fx - anchor) > 0.5f)
        {
            g.setColour (s.isEnabled() ? Theme::accent : Theme::accent.withAlpha (0.4f));
            g.fillRoundedRectangle (juce::jmin (anchor, fx), cy - trackH * 0.5f, std::abs (fx - anchor), trackH, trackH * 0.5f);
        }

        const float r = 7.0f;
        g.setColour (Theme::knobBody);
        g.fillEllipse (fx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour (s.isMouseOverOrDragging() ? Theme::accent : Theme::text);
        g.drawEllipse (fx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);
    }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (Theme::ui (12.0f, true));
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, Theme::text);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
        return l;
    }

    juce::Font getLabelFont (juce::Label&) override { return Theme::ui (12.0f, true); }

    // Slider value box: rounded pill, no rectangular outline
    void drawLabel (juce::Graphics& g, juce::Label& l) override
    {
        auto r = l.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (r, 6.0f);
        if (! l.isBeingEdited())
        {
            g.setColour (l.findColour (juce::Label::textColourId).withMultipliedAlpha (l.isEnabled() ? 1.0f : 0.5f));
            g.setFont (getLabelFont (l));
            g.drawFittedText (l.getText(), l.getLocalBounds().reduced (4, 0), l.getJustificationType(), 1, 1.0f);
        }
    }

    void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height) override
    {
        g.setColour (Theme::text);
        g.fillRoundedRectangle (juce::Rectangle<float> (0, 0, (float) width, (float) height), 8.0f);
        g.setColour (Theme::bg);
        g.setFont (Theme::ui (12.0f));
        g.drawText (text, 0, 0, width, height, juce::Justification::centred);
    }

    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override
    {
        auto f = Theme::ui (12.0f);
        const int w = juce::GlyphArrangement::getStringWidthInt (f, tipText) + 20;
        const int h = 24;
        return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                     screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6, w, h)
                .constrainedWithin (parentArea);
    }
};
