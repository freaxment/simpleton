#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

// Horizontal slider with a caption above it, an optional one-word description at the right of the
// caption row, and the value (editable) at the right end. Same widget as in FreaxKlip / Freaxcalibur.
class Fader : public juce::Component
{
public:
    Fader (juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& caption,
           const juce::String& tooltip, std::function<juce::String (double)> describeFn = nullptr);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::String caption;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    std::function<juce::String (double)> describe;
};

// Round lamp toggle: dark body with a light ring when off, accent with a glow when on.
class Lamp : public juce::Button
{
public:
    Lamp (juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& caption,
          const juce::String& tooltip);
    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    int getPreferredWidth() const;

private:
    juce::String caption;
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment;
};

class FreaxVolumeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit FreaxVolumeAudioProcessorEditor (FreaxVolumeAudioProcessor&);
    ~FreaxVolumeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FreaxVolumeLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 500 };

    std::unique_ptr<juce::Drawable> logo;   // Freaxment wordmark, tinted accent
    Fader volumeFader, widthFader;
    Lamp muteLamp, monoLamp;

    static constexpr int headerHeight = 56, margin = 14;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreaxVolumeAudioProcessorEditor)
};
