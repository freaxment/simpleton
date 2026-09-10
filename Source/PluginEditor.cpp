#include "PluginEditor.h"
#include "Assets.h"

// ============================================================================ Fader
Fader::Fader (juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& text,
              const juce::String& tooltip, std::function<juce::String (double)> describeFn)
    : caption (text), attachment (state, paramID, slider), describe (std::move (describeFn))
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 74, 20);
    const auto defaultValue = state.getParameterRange (paramID).convertFrom0to1 (state.getParameter (paramID)->getDefaultValue());
    slider.setDoubleClickReturnValue (true, (double) defaultValue);
    slider.setTooltip (tooltip);
    slider.setWantsKeyboardFocus (false);
    if (describe != nullptr) slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
}

void Fader::paint (juce::Graphics& g)
{
    auto row = getLocalBounds().removeFromTop (16);
    g.setColour (Theme::text2);
    g.setFont (Theme::bold (11.0f));
    g.drawText (caption.toUpperCase(), row.withTrimmedLeft (8), juce::Justification::centredLeft);
    if (describe != nullptr)
    {
        g.setColour (Theme::text3);
        g.setFont (Theme::ui (11.0f, true));
        g.drawText (describe (slider.getValue()), row.withTrimmedRight (10), juce::Justification::centredRight);
    }
}

void Fader::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (14));
}

// ============================================================================ Lamp
Lamp::Lamp (juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& text,
            const juce::String& tooltip)
    : juce::Button (paramID), caption (text), attachment (state, paramID, *this)
{
    setClickingTogglesState (true);
    setTooltip (tooltip);
    setWantsKeyboardFocus (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

int Lamp::getPreferredWidth() const
{
    return 14 + 8 + juce::GlyphArrangement::getStringWidthInt (Theme::bold (11.0f), caption.toUpperCase()) + 4;
}

void Lamp::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    const bool on = getToggleState();
    const float r = 7.0f;
    const auto centre = juce::Point<float> (2.0f + r, (float) getHeight() * 0.5f);
    if (on)
    {
        g.setColour (Theme::accent.withAlpha (0.25f));
        g.fillEllipse (centre.x - r - 4.0f, centre.y - r - 4.0f, (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f);   // glow
        g.setColour (Theme::accent);
        g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
    }
    else
    {
        g.setColour (Theme::knobBody);
        g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
        g.setColour (highlighted ? Theme::accent : Theme::text);
        g.drawEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 1.5f);
    }
    g.setColour (on ? Theme::accent : Theme::text2);
    g.setFont (Theme::bold (11.0f));
    g.drawText (caption.toUpperCase(), (int) (centre.x + r) + 8, 0, getWidth() - (int) (centre.x + r) - 8, getHeight(),
                juce::Justification::centredLeft);
}

// ============================================================================ Editor
static void tintDrawable (juce::Drawable& d, juce::Colour c)
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

static juce::String describeWidth (double percent)
{
    if (percent <= 0.0)   return "Side removed";
    if (percent < 99.5)   return "Narrower";
    if (percent <= 100.5) return "Original stereo";
    return "Wider";
}

FreaxVolumeAudioProcessorEditor::FreaxVolumeAudioProcessorEditor (FreaxVolumeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      volumeFader (p.getState(), ParamID::volume, "Volume", "Output level: silence at 0 %, 0 dB in the middle, +10 dB at 100 % (double-click to reset)"),
      widthFader  (p.getState(), ParamID::width,  "Width",  "Stereo width: 0 % = mono, 100 % = untouched, 300 % = side signal x3 (double-click to reset)", describeWidth),
      muteLamp    (p.getState(), ParamID::mute,   "Mute",   "Silence the output (20 ms fade, no click)"),
      monoLamp    (p.getState(), ParamID::mono,   "Mono",   "Output the mid signal only: L = R (overrides Width)")
{
    setLookAndFeel (&lnf);
    setOpaque (true);

    logo = juce::Drawable::createFromImageData (Assets::freaxment_logo_svg, (size_t) Assets::freaxment_logo_svgSize);
    if (logo != nullptr) tintDrawable (*logo, Theme::accent);

    for (auto* c : std::initializer_list<juce::Component*> { &volumeFader, &widthFader, &muteLamp, &monoLamp })
        addAndMakeVisible (c);

    setSize (520, 190);
}

FreaxVolumeAudioProcessorEditor::~FreaxVolumeAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void FreaxVolumeAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (headerHeight);
    r = r.reduced (margin, 0).withTrimmedTop (2).withTrimmedBottom (margin);

    // lamps in the header, right after the plugin name
    const int nameRight = margin + juce::GlyphArrangement::getStringWidthInt (Theme::black (14.0f), "FreaxVolume");
    muteLamp.setBounds (nameRight + 22, 8, muteLamp.getPreferredWidth(), headerHeight - 16);
    monoLamp.setBounds (muteLamp.getRight() + 10, 8, monoLamp.getPreferredWidth(), headerHeight - 16);

    // faders stacked, top to bottom: volume -> width
    const int faderH = r.getHeight() / 2;
    volumeFader.setBounds (r.removeFromTop (faderH));
    widthFader.setBounds (r);
}

void FreaxVolumeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);

    auto bar = getLocalBounds().removeFromTop (headerHeight);
    g.setColour (Theme::border);
    g.fillRect (bar.getX(), bar.getBottom() - 1, bar.getWidth(), 1);

    // plugin name, top-left
    juce::GlyphArrangement ga;
    ga.addLineOfText (Theme::black (14.0f), "FreaxVolume", (float) margin, (float) bar.getCentreY() - 2.0f);
    g.setColour (Theme::accent);
    ga.draw (g);
    g.setColour (Theme::text3);
    g.setFont (Theme::ui (10.0f));
    g.drawText ("CHANNEL UTILITY", margin, bar.getCentreY() + 3, 140, 12, juce::Justification::centredLeft);

    // Freaxment logo, centred in the space between the lamps and the right edge
    if (logo != nullptr)
    {
        const int logoH = headerHeight - 18;
        const auto b = logo->getDrawableBounds();
        const int logoW = b.getHeight() > 0.0f ? (int) std::round (logoH * b.getWidth() / b.getHeight()) : 0;
        auto area = juce::Rectangle<int> (monoLamp.getRight(), 0, getWidth() - margin - monoLamp.getRight(), headerHeight)
                        .withSizeKeepingCentre (logoW, logoH);
        logo->drawWithin (g, area.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
}
