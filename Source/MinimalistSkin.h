#pragma once

#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "Skins.h"

// "Simpleton Minimalist": warm paper, two-tone arcs, pill buttons, Freaxment logo.
class MinimalistSkin final : public SkinView
{
public:
    explicit MinimalistSkin (SimpletonAudioProcessor&);

    juce::Point<int> defaultSize() const override { return { baseWidth, baseHeight }; }
    void configureConstrainer (juce::ComponentBoundsConstrainer&) const override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    static constexpr int baseWidth  = 400;
    static constexpr int baseHeight = 410;

    void setupKnob (juce::Slider&, juce::Label& caption, juce::Label& value,
                    const juce::String& captionText, juce::Colour accentBelow, juce::Colour accentAbove,
                    double neutralValue);
    void refreshValueLabel (juce::Slider&, juce::Label&);
    void refreshDimming();
    int scaled (float v) const { return juce::roundToInt (v * scale); }

    SimpletonAudioProcessor& processor;

    juce::Slider volumeKnob, widthKnob;
    juce::Label  volumeCaption, widthCaption, volumeValue, widthValue;
    juce::TextButton muteButton { "Mute" }, monoButton { "Mono" };

    std::unique_ptr<SliderAttachment> volumeAttachment, widthAttachment;
    std::unique_ptr<ButtonAttachment> muteAttachment, monoAttachment;

    std::unique_ptr<juce::Drawable> logo;   // Freaxment wordmark, tinted purpleDark
    juce::Rectangle<int> titleArea;
    float scale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MinimalistSkin)
};
