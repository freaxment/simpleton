#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr double smoothingSeconds = 0.02;   // 20 ms ramps: no zipper noise, no clicks on mute/mono

    juce::String formatPercent (float v)
    {
        const float rounded = std::round (v * 10.0f) / 10.0f;

        if (juce::approximatelyEqual (rounded, std::round (rounded)))
            return juce::String (juce::roundToInt (rounded)) + " %";

        return juce::String (rounded, 1) + " %";
    }

    juce::String volumeToText (float value, int)
    {
        if (value <= 0.0f)
            return "-inf dB";

        float dB = std::round (Mapping::volumeToDecibels (value) * 10.0f) / 10.0f;

        if (std::abs (dB) < 0.05f)
            dB = 0.0f;

        juce::String text (dB, 1);

        if (dB > 0.0f)
            text = "+" + text;

        return text + " dB";
    }

    float textToVolume (const juce::String& rawText)
    {
        const auto text = rawText.trim().toLowerCase();

        if (text.contains ("inf"))
            return 0.0f;

        if (text.contains ("%"))
            return juce::jlimit (0.0f, 100.0f, text.getFloatValue());

        return Mapping::decibelsToVolume (text.getFloatValue());
    }

    juce::String widthToText (float value, int)
    {
        if (value <= 0.0f)
            return "Mono";

        return formatPercent (value);
    }

    float textToWidth (const juce::String& rawText)
    {
        const auto text = rawText.trim().toLowerCase();

        if (text.contains ("mono"))
            return 0.0f;

        return juce::jlimit (0.0f, Mapping::maxWidthPercent, text.getFloatValue());
    }
}

SimpletonAudioProcessor::SimpletonAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "Simpleton", createParameterLayout())
{
    volumeParam = state.getRawParameterValue (ParamID::volume);
    widthParam  = state.getRawParameterValue (ParamID::width);
    muteParam   = state.getRawParameterValue (ParamID::mute);
    monoParam   = state.getRawParameterValue (ParamID::mono);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpletonAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::volume, 1 },
        "Volume",
        juce::NormalisableRange<float> (0.0f, 100.0f),
        50.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction (volumeToText)
            .withValueFromStringFunction (textToVolume)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::width, 1 },
        "Width",
        juce::NormalisableRange<float> (0.0f, Mapping::maxWidthPercent,
                                        [] (float, float, float normalised) { return Mapping::knobToWidth (normalised); },
                                        [] (float, float, float percent)    { return Mapping::widthToKnob (percent); }),
        100.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction (widthToText)
            .withValueFromStringFunction (textToWidth)));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::mute, 1 }, "Mute", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::mono, 1 }, "Mono", false));

    return layout;
}

void SimpletonAudioProcessor::prepareToPlay (double sampleRate, int)
{
    gainSmoother.reset (sampleRate, smoothingSeconds);
    sideSmoother.reset (sampleRate, smoothingSeconds);

    const bool mute = *muteParam >= 0.5f;
    const bool mono = *monoParam >= 0.5f;

    gainSmoother.setCurrentAndTargetValue (mute ? 0.0f : Mapping::volumeToGain (*volumeParam));
    sideSmoother.setCurrentAndTargetValue (mono ? 0.0f : Mapping::widthToSideGain (*widthParam));
}

bool SimpletonAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void SimpletonAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInputs  = getTotalNumInputChannels();
    const int numOutputs = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = numInputs; ch < numOutputs; ++ch)
        buffer.clear (ch, 0, numSamples);

    const bool mute = *muteParam >= 0.5f;
    const bool mono = *monoParam >= 0.5f;

    gainSmoother.setTargetValue (mute ? 0.0f : Mapping::volumeToGain (*volumeParam));
    sideSmoother.setTargetValue (mono ? 0.0f : Mapping::widthToSideGain (*widthParam));

    if (numInputs >= 2 && buffer.getNumChannels() >= 2)
    {
        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);

        for (int i = 0; i < numSamples; ++i)
        {
            const float gain     = gainSmoother.getNextValue();
            const float sideGain = sideSmoother.getNextValue();

            const float mid  = 0.5f * (left[i] + right[i]);
            const float side = 0.5f * (left[i] - right[i]) * sideGain;

            left[i]  = (mid + side) * gain;
            right[i] = (mid - side) * gain;
        }
    }
    else if (numInputs == 1 && buffer.getNumChannels() >= 1)
    {
        auto* data = buffer.getWritePointer (0);

        for (int i = 0; i < numSamples; ++i)
            data[i] *= gainSmoother.getNextValue();

        sideSmoother.skip (numSamples);
    }
    else
    {
        gainSmoother.skip (numSamples);
        sideSmoother.skip (numSamples);
    }
}

juce::AudioProcessorEditor* SimpletonAudioProcessor::createEditor()
{
    return new SimpletonAudioProcessorEditor (*this);
}

void SimpletonAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SimpletonAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpletonAudioProcessor();
}
