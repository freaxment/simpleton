#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamID
{
    inline constexpr const char* volume = "volume";
    inline constexpr const char* width  = "width";
    inline constexpr const char* mute   = "mute";
    inline constexpr const char* mono   = "mono";
}

/*  Knob laws
    ---------
    Volume (0..100 %):  0 %  -> silence
                        50 % -> 0 dB
                        100 % -> +10 dB
        Lower half is a fader-like taper (gain = (2x)^2, i.e. -12 dB at 25 %),
        upper half is linear in dB (0..+10 dB).

    Width (0..300 %):   0 %   -> mono (side removed)
                        100 % -> untouched stereo (side x1)
                        300 % -> side x3
        The displayed value IS the side gain in percent. The knob travel is
        non-linear so that 100 % sits exactly in the middle: with knob position
        x in 0..1, side gain = 2x^2 + x (0 -> 0, 0.5 -> 1, 1 -> 3).
*/
namespace Mapping
{
    inline float volumeToGain (float percent)
    {
        const float x = juce::jlimit (0.0f, 100.0f, percent) / 100.0f;

        if (x <= 0.5f)
        {
            const float a = 2.0f * x;
            return a * a;
        }

        return juce::Decibels::decibelsToGain (20.0f * (x - 0.5f));
    }

    inline float volumeToDecibels (float percent)
    {
        return juce::Decibels::gainToDecibels (volumeToGain (percent), -120.0f);
    }

    inline float decibelsToVolume (float dB)
    {
        if (dB <= 0.0f)
            return juce::jlimit (0.0f, 100.0f, 50.0f * std::pow (10.0f, dB / 40.0f));

        return juce::jlimit (0.0f, 100.0f, 50.0f + 50.0f * dB / 10.0f);
    }

    inline constexpr float maxWidthPercent = 300.0f;

    inline float widthToSideGain (float percent)
    {
        return juce::jlimit (0.0f, maxWidthPercent, percent) / 100.0f;
    }

    // Knob position (0..1) <-> width percent (0..300)
    inline float knobToWidth (float x)
    {
        x = juce::jlimit (0.0f, 1.0f, x);
        return 100.0f * (2.0f * x * x + x);
    }

    inline float widthToKnob (float percent)
    {
        const float g = juce::jlimit (0.0f, maxWidthPercent, percent) / 100.0f;
        return (std::sqrt (1.0f + 8.0f * g) - 1.0f) / 4.0f;
    }
}

class SimpletonAudioProcessor final : public juce::AudioProcessor
{
public:
    SimpletonAudioProcessor();
    ~SimpletonAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() { return state; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState state;

    std::atomic<float>* volumeParam = nullptr;
    std::atomic<float>* widthParam  = nullptr;
    std::atomic<float>* muteParam   = nullptr;
    std::atomic<float>* monoParam   = nullptr;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sideSmoother;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpletonAudioProcessor)
};
