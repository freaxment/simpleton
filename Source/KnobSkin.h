#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "Skins.h"

// Alternative skin: same Freaxment palette and type, laid out with two big knobs
// (value pill underneath) and MUTE / MONO buttons along the bottom.
class KnobSkin final : public SkinView
{
public:
    explicit KnobSkin (FreaxVolumeAudioProcessor&);
    ~KnobSkin() override;

    juce::Point<int> defaultSize() const override { return { 500, 340 }; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void setupKnob (juce::Slider&, const juce::String& paramID, const juce::String& tooltip);

    FreaxVolumeAudioProcessor& processor;
    FreaxVolumeLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 500 };

    std::unique_ptr<juce::Drawable> logo;
    juce::Slider volumeKnob, widthKnob;
    juce::TextButton muteButton { "MUTE" }, monoButton { "MONO" };
    std::unique_ptr<SliderAttachment> volumeAttachment, widthAttachment;
    std::unique_ptr<ButtonAttachment> muteAttachment, monoAttachment;

    juce::Rectangle<int> volumeColumn, widthColumn;
    static constexpr int headerHeight = 56, margin = 14;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobSkin)
};
