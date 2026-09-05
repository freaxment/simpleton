#pragma once

#include "PluginProcessor.h"
#include "Skins.h"

// "Simpleton Flex": dark panel, lime accent, big black knobs (the SignalFlex design).
class FlexSkin final : public SkinView
{
public:
    explicit FlexSkin (SimpletonAudioProcessor&);

    juce::Point<int> defaultSize() const override { return { 500, 360 }; }
    void configureConstrainer (juce::ComponentBoundsConstrainer&) const override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    class Knob final : public juce::Slider
    {
    public:
        Knob();
        void paint (juce::Graphics&) override;
    };

    class Button final : public juce::TextButton
    {
    public:
        explicit Button (const juce::String&);
        void paintButton (juce::Graphics&, bool, bool) override;
    };

    void refreshLabels();

    SimpletonAudioProcessor& processor;

    Knob gain, width;
    Button mute { "MUTE" }, mono { "MONO" };
    juce::Label gainValue, widthValue;

    std::unique_ptr<SliderAttachment> gainAttachment, widthAttachment;
    std::unique_ptr<ButtonAttachment> muteAttachment, monoAttachment;

    std::unique_ptr<juce::Drawable> logo;   // Freaxment wordmark, tinted lime

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlexSkin)
};
