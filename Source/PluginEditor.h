#pragma once

#include "PluginProcessor.h"
#include "Skins.h"

// Thin host window: shows whichever skin is selected and swaps it on request
// (right-click on the plugin background opens the skin menu).
class FreaxVolumeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit FreaxVolumeAudioProcessorEditor (FreaxVolumeAudioProcessor&);
    ~FreaxVolumeAudioProcessorEditor() override;

    void resized() override;

private:
    void showSkin (SkinId);

    FreaxVolumeAudioProcessor& freaxVolumeProcessor;
    std::unique_ptr<SkinView> skin;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreaxVolumeAudioProcessorEditor)
};
