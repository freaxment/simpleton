#pragma once

#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "Skins.h"

// Thin host window: shows whichever skin is selected and swaps it on request.
class SimpletonAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SimpletonAudioProcessorEditor (SimpletonAudioProcessor&);
    ~SimpletonAudioProcessorEditor() override;

    void resized() override;

private:
    void showSkin (SkinId);

    SimpletonAudioProcessor& simpletonProcessor;
    SimpletonLookAndFeel lookAndFeel;
    std::unique_ptr<SkinView> skin;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpletonAudioProcessorEditor)
};
