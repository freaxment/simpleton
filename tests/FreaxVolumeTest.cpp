// Self-test host for FreaxVolume.
//   1. Instantiates the processor directly and checks the knob laws, smoothing,
//      parameter text, state round-trip and the mono bus layout.
//   2. Loads the installed VST3 and AU bundles through JUCE's plugin hosting
//      and checks the metadata + audio against the direct instance.
//   3. Renders the editor to PNG files (path given as argv[1]).

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Skins.h"

#include <iostream>

namespace
{
    int failures = 0;

    void check (bool ok, const juce::String& what)
    {
        std::cout << (ok ? "  ok    " : "  FAIL  ") << what << std::endl;
        if (! ok) ++failures;
    }

    bool near (float a, float b, float tol = 2.0e-3f) { return std::abs (a - b) <= tol; }

    // Direct instance: our own RangedAudioParameters, looked up by ID.
    juce::RangedAudioParameter* findParam (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* param : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (param))
                if (r->getParameterID() == id)
                    return r;

        return nullptr;
    }

    void setPlain (juce::AudioProcessor& p, const juce::String& id, float plain)
    {
        auto* param = findParam (p, id);
        jassert (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (plain));
    }

    // Hosted instance: generic host-side parameters, looked up by display name.
    juce::AudioProcessorParameter* findHostedParam (juce::AudioProcessor& p, const juce::String& name)
    {
        for (auto* param : p.getParameters())
            if (param->getName (64) == name)
                return param;

        return nullptr;
    }

    bool setHostedNormalised (juce::AudioProcessor& p, const juce::String& name, float normalised)
    {
        if (auto* param = findHostedParam (p, name))
        {
            param->setValueNotifyingHost (normalised);
            return true;
        }

        return false;
    }

    struct Stereo { float l, r; };

    Stereo settle (juce::AudioProcessor& p, float inL, float inR, int blocks = 20, int blockSize = 512)
    {
        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;

        for (int b = 0; b < blocks; ++b)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                buffer.setSample (0, i, inL);
                buffer.setSample (1, i, inR);
            }

            p.processBlock (buffer, midi);
        }

        return { buffer.getSample (0, blockSize - 1), buffer.getSample (1, blockSize - 1) };
    }

    juce::String fmt (Stereo s) { return "(" + juce::String (s.l, 4) + ", " + juce::String (s.r, 4) + ")"; }
    juce::String fmt (float l, float r) { return fmt ({ l, r }); }

    //==============================================================================
    void testDirectInstance()
    {
        std::cout << "\n[direct instance]" << std::endl;

        FreaxVolumeAudioProcessor p;
        p.setRateAndBufferSizeDetails (48000.0, 512);
        p.prepareToPlay (48000.0, 512);

        auto expect = [&] (const juce::String& name, float inL, float inR, float outL, float outR)
        {
            const auto out = settle (p, inL, inR);
            check (near (out.l, outL) && near (out.r, outR),
                   name + ": in " + fmt (inL, inR) + " -> " + fmt (out) + ", expected " + fmt (outL, outR));
        };

        const float plus10 = juce::Decibels::decibelsToGain (10.0f);
        const float plus5  = juce::Decibels::decibelsToGain (5.0f);

        setPlain (p, ParamID::volume, 50.0f);  setPlain (p, ParamID::width, 100.0f);
        setPlain (p, ParamID::mute, 0.0f);     setPlain (p, ParamID::mono, 0.0f);
        expect ("volume 50 % = 0 dB, width 100 % = untouched", 0.5f, -0.25f, 0.5f, -0.25f);

        setPlain (p, ParamID::volume, 100.0f);
        expect ("volume 100 % = +10 dB", 0.5f, -0.25f, 0.5f * plus10, -0.25f * plus10);

        setPlain (p, ParamID::volume, 75.0f);
        expect ("volume 75 % = +5 dB", 0.5f, -0.25f, 0.5f * plus5, -0.25f * plus5);

        setPlain (p, ParamID::volume, 25.0f);
        expect ("volume 25 % = -12 dB (gain 0.25)", 0.5f, -0.25f, 0.125f, -0.0625f);

        setPlain (p, ParamID::volume, 0.0f);
        expect ("volume 0 % = silence", 0.5f, -0.25f, 0.0f, 0.0f);

        setPlain (p, ParamID::volume, 50.0f);
        setPlain (p, ParamID::width, 0.0f);
        expect ("width 0 % = mono (L = R = mid)", 0.5f, -0.25f, 0.125f, 0.125f);

        setPlain (p, ParamID::width, 300.0f);
        expect ("width 300 % = side x3", 1.0f, 0.0f, 2.0f, -1.0f);

        setPlain (p, ParamID::width, 50.0f);
        expect ("width 50 % = side x0.5", 1.0f, 0.0f, 0.75f, 0.25f);

        setPlain (p, ParamID::width, 200.0f);
        expect ("width 200 % = side x2", 1.0f, 0.0f, 1.5f, -0.5f);

        setPlain (p, ParamID::width, 300.0f);
        setPlain (p, ParamID::mono, 1.0f);
        expect ("mono button overrides width: L = R = mid", 1.0f, 0.0f, 0.5f, 0.5f);

        setPlain (p, ParamID::mono, 0.0f);
        setPlain (p, ParamID::volume, 100.0f);
        setPlain (p, ParamID::mute, 1.0f);
        expect ("mute = silence even at +10 dB", 1.0f, 0.0f, 0.0f, 0.0f);

        // Smoothing: 20 ms linear ramp = 960 samples at 48 kHz.
        setPlain (p, ParamID::mute, 0.0f);
        setPlain (p, ParamID::volume, 50.0f);
        setPlain (p, ParamID::width, 100.0f);
        settle (p, 1.0f, 1.0f);
        setPlain (p, ParamID::volume, 0.0f);
        {
            juce::AudioBuffer<float> buffer (2, 512);
            juce::MidiBuffer midi;
            for (int i = 0; i < 512; ++i) { buffer.setSample (0, i, 1.0f); buffer.setSample (1, i, 1.0f); }
            p.processBlock (buffer, midi);

            const float first = buffer.getSample (0, 0);
            const float last  = buffer.getSample (0, 511);
            const float expectedLast = 1.0f - 512.0f / 960.0f;
            check (first > 0.99f && near (last, expectedLast, 0.01f),
                   "volume jump is ramped over 20 ms: first sample " + juce::String (first, 3)
                   + ", sample 511 = " + juce::String (last, 3) + " (expected " + juce::String (expectedLast, 3) + ")");
        }

        // Parameter text.
        auto* volume = findParam (p, ParamID::volume);
        auto* width  = findParam (p, ParamID::width);
        auto text = [] (juce::RangedAudioParameter* param, float plain) { return param->getText (param->convertTo0to1 (plain), 0); };
        auto value = [] (juce::RangedAudioParameter* param, const juce::String& s) { return param->convertFrom0to1 (param->getValueForText (s)); };

        check (text (volume, 0.0f)   == "-inf dB",   "volume text at 0 %: "   + text (volume, 0.0f));
        check (text (volume, 50.0f)  == "0.00 dB",   "volume text at 50 %: "  + text (volume, 50.0f));
        check (text (volume, 100.0f) == "+10.00 dB", "volume text at 100 %: " + text (volume, 100.0f));
        check (text (volume, 25.0f)  == "-12.04 dB", "volume text at 25 %: "  + text (volume, 25.0f));
        check (text (width, 0.0f)    == "Mono",     "width text at 0 %: "    + text (width, 0.0f));
        check (text (width, 100.0f)  == "100 %",    "width text at 100 %: "  + text (width, 100.0f));
        check (text (width, 300.0f)  == "300 %",    "width text at 300 %: "  + text (width, 300.0f));
        check (text (width, 33.3f)   == "33.3 %",   "width text at 33.3 %: " + text (width, 33.3f));
        check (near (width->convertTo0to1 (100.0f), 0.5f) && near (width->convertFrom0to1 (0.5f), 100.0f)
                && near (width->convertTo0to1 (300.0f), 1.0f) && near (width->convertTo0to1 (0.0f), 0.0f),
               "width knob middle = 100 %, ends = 0 % / 300 %");
        check (near (width->getDefaultValue(), 0.5f), "width default sits in the middle of the knob");

        check (near (Mapping::volumeToDecibels (value (volume, "+3 dB")), 3.0f, 0.01f), "typing '+3 dB' into volume gives +3 dB");
        check (near (Mapping::volumeToDecibels (value (volume, "-6")), -6.0f, 0.01f),   "typing '-6' into volume gives -6 dB");
        check (near (value (volume, "-inf"), 0.0f), "typing '-inf' into volume gives 0 %");
        check (near (value (volume, "80 %"), 80.0f), "typing '80 %' into volume gives 80 %");
        check (near (value (width, "mono"), 0.0f),  "typing 'mono' into width gives 0 %");
        check (near (value (width, "42"), 42.0f),   "typing '42' into width gives 42 %");
        check (near (value (width, "250 %"), 250.0f), "typing '250 %' into width gives 250 %");
        check (near (value (width, "999"), 300.0f), "typing '999' into width clamps to 300 %");

        check (findHostedParam (p, "Volume") != nullptr && findHostedParam (p, "Width") != nullptr
                && findHostedParam (p, "Mute") != nullptr && findHostedParam (p, "Mono") != nullptr,
               "parameters are named Volume / Width / Mute / Mono");

        // State round-trip.
        setPlain (p, ParamID::volume, 80.0f);
        setPlain (p, ParamID::width, 20.0f);
        setPlain (p, ParamID::mute, 1.0f);
        setPlain (p, ParamID::mono, 0.0f);

        juce::MemoryBlock blob;
        p.getStateInformation (blob);

        FreaxVolumeAudioProcessor q;
        q.setStateInformation (blob.getData(), (int) blob.getSize());
        check (Skins::load (q.getState()) == SkinId::family, "default skin is the dark Freaxment design");

        Skins::save (p.getState(), SkinId::knobs);
        p.getStateInformation (blob);
        q.setStateInformation (blob.getData(), (int) blob.getSize());
        check (Skins::load (q.getState()) == SkinId::knobs, "skin choice survives save/restore");

        q.getState().state.setProperty (Skins::property, "minimalist", nullptr);
        check (Skins::load (q.getState()) == SkinId::family, "old skin values fall back to the default skin");

        // A state saved by the plugin while it was still called "Simpleton" must load too.
        {
            juce::XmlElement legacy ("Simpleton");
            auto* param = legacy.createNewChildElement ("PARAM");
            param->setAttribute ("id", "width");
            param->setAttribute ("value", 250.0);
            juce::MemoryBlock legacyBlob;
            FreaxVolumeAudioProcessor::copyXmlToBinary (legacy, legacyBlob);

            FreaxVolumeAudioProcessor r;
            r.setStateInformation (legacyBlob.getData(), (int) legacyBlob.getSize());
            auto* widthParam = findParam (r, ParamID::width);
            const float restoredWidth = widthParam->convertFrom0to1 (widthParam->getValue());
            check (near (restoredWidth, 250.0f), "legacy 'Simpleton' state loads (width 250 %)");
        }

        auto plain = [] (juce::AudioProcessor& proc, const char* id)
        {
            auto* param = findParam (proc, id);
            return param->convertFrom0to1 (param->getValue());
        };

        check (near (plain (q, ParamID::volume), 80.0f) && near (plain (q, ParamID::width), 20.0f)
                && near (plain (q, ParamID::mute), 1.0f) && near (plain (q, ParamID::mono), 0.0f),
               "state save/restore round-trip");

        // Mono bus layout.
        FreaxVolumeAudioProcessor m;
        juce::AudioProcessor::BusesLayout monoLayout;
        monoLayout.inputBuses.add (juce::AudioChannelSet::mono());
        monoLayout.outputBuses.add (juce::AudioChannelSet::mono());
        check (m.setBusesLayout (monoLayout), "mono in / mono out layout accepted");

        juce::AudioProcessor::BusesLayout badLayout;
        badLayout.inputBuses.add (juce::AudioChannelSet::stereo());
        badLayout.outputBuses.add (juce::AudioChannelSet::mono());
        check (! m.checkBusesLayoutSupported (badLayout), "stereo in / mono out layout rejected");

        m.setRateAndBufferSizeDetails (44100.0, 256);
        m.prepareToPlay (44100.0, 256);
        setPlain (m, ParamID::volume, 100.0f);
        {
            juce::AudioBuffer<float> buffer (1, 256);
            juce::MidiBuffer midi;
            for (int b = 0; b < 20; ++b)
            {
                for (int i = 0; i < 256; ++i) buffer.setSample (0, i, 0.5f);
                m.processBlock (buffer, midi);
            }
            const float out = buffer.getSample (0, 255);
            check (near (out, 0.5f * plus10), "mono layout applies +10 dB: " + juce::String (out, 4));
        }
    }

    //==============================================================================
    void testHosted (juce::AudioPluginFormat& format, const juce::File& bundle)
    {
        std::cout << "\n[hosted " << format.getName() << "] " << bundle.getFullPathName() << std::endl;

        juce::OwnedArray<juce::PluginDescription> descriptions;

        // A freshly installed AU can take a moment to show up in the system's
        // component registry, so give the scan a few tries.
        for (int attempt = 0; attempt < 10 && descriptions.isEmpty(); ++attempt)
        {
            if (attempt > 0)
                juce::Thread::sleep (1000);

            format.findAllTypesForFile (descriptions, bundle.getFullPathName());
        }

        check (descriptions.size() == 1, "bundle describes exactly one plugin (" + juce::String (descriptions.size()) + ")");

        if (descriptions.isEmpty())
            return;

        const auto& desc = *descriptions[0];
        check (desc.name == "FreaxVolume",              "name: " + desc.name);
        check (desc.manufacturerName == "Freaxment",  "manufacturer: " + desc.manufacturerName);
        check (! desc.isInstrument,                   "category: " + desc.category + " (effect)");
        std::cout << "        version " << desc.version << ", id " << desc.fileOrIdentifier << std::endl;

        juce::String error;
        auto instance = format.createInstanceFromDescription (desc, 48000.0, 512, error);
        check (instance != nullptr, "instance created" + (error.isEmpty() ? juce::String() : " (" + error + ")"));

        if (instance == nullptr)
            return;

        instance->enableAllBuses();
        instance->prepareToPlay (48000.0, 512);

        juce::StringArray names;
        for (auto* param : instance->getParameters())
            names.add (param->getName (64));

        check (names.contains ("Volume") && names.contains ("Width") && names.contains ("Mute") && names.contains ("Mono"),
               "hosted parameters visible: " + names.joinIntoString (", "));

        if (auto* volume = findHostedParam (*instance, "Volume"))
            check (volume->getText (0.5f, 64) == "0.00 dB" && volume->getText (1.0f, 64) == "+10.00 dB" && volume->getText (0.0f, 64) == "-inf dB",
                   "hosted volume text: " + volume->getText (0.0f, 64) + " / " + volume->getText (0.5f, 64) + " / " + volume->getText (1.0f, 64));

        if (auto* width = findHostedParam (*instance, "Width"))
            check (width->getText (0.0f, 64) == "Mono" && width->getText (0.5f, 64) == "100 %" && width->getText (1.0f, 64) == "300 %",
                   "hosted width text: " + width->getText (0.0f, 64) + " / " + width->getText (0.5f, 64) + " / " + width->getText (1.0f, 64));

        if (! (setHostedNormalised (*instance, "Volume", 1.0f) && setHostedNormalised (*instance, "Width", 1.0f)))
            return;
        const auto out = settle (*instance, 1.0f, 0.0f, 40);
        const float plus10 = juce::Decibels::decibelsToGain (10.0f);
        check (near (out.l, 2.0f * plus10, 0.01f) && near (out.r, -1.0f * plus10, 0.01f),
               "hosted audio: volume 100 %, width 300 % on (1, 0) -> " + fmt (out)
               + ", expected " + fmt (2.0f * plus10, -plus10));

        setHostedNormalised (*instance, "Mute", 1.0f);
        const auto muted = settle (*instance, 1.0f, 0.0f, 40);
        check (near (muted.l, 0.0f) && near (muted.r, 0.0f), "hosted audio: mute -> " + fmt (muted));

        instance->releaseResources();
    }

    //==============================================================================
    // Editor lifetime stress: open/close many times, also with two instances alive at once.
    void stressEditorLifetime()
    {
        std::cout << "\n[editor lifetime]" << std::endl;

        auto pump = [] (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); };
        auto skinOf = [] (juce::AudioProcessorEditor& editor) { return dynamic_cast<SkinView*> (editor.getChildComponent (0)); };

        bool allGood = true;
        FreaxVolumeAudioProcessor p;
        p.prepareToPlay (48000.0, 512);

        for (int cycle = 0; cycle < 30; ++cycle)
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
            auto* skin = skinOf (*editor);
            allGood = allGood && skin != nullptr;
            if (skin == nullptr) break;

            setPlain (p, ParamID::volume, (float) (cycle * 3 % 100));
            setPlain (p, ParamID::mute, (float) (cycle % 2));

            const auto before = editor->getBounds();
            skin->onSelectSkin (Skins::other (skin->currentSkin));   // same call the right-click menu makes
            pump (30);                                                // lets the deferred swap run

            auto* swapped = skinOf (*editor);
            allGood = allGood && swapped != nullptr && swapped != skin
                      && editor->getBounds() != before
                      && Skins::load (p.getState()) == swapped->currentSkin;

            // Request another swap and destroy the editor before it runs.
            swapped->onSelectSkin (Skins::other (swapped->currentSkin));
            editor.reset();
            pump (30);
        }

        check (allGood, "30 open/switch/close cycles, including destroy-while-switch-pending");
        Skins::save (p.getState(), SkinId::family);

        {
            FreaxVolumeAudioProcessor a, b;
            std::unique_ptr<juce::AudioProcessorEditor> ea (a.createEditor()), eb (b.createEditor());
            pump (30);
            ea.reset();
            eb.reset();
            pump (30);
            check (true, "two editors alive at once, torn down cleanly");
        }
    }

    //==============================================================================
    void renderEditor (const juce::File& outDir)
    {
        std::cout << "\n[editor snapshots] -> " << outDir.getFullPathName() << std::endl;
        outDir.createDirectory();

        auto snapshot = [&] (FreaxVolumeAudioProcessor& p, const juce::String& fileName, float scaleFactor)
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
            check (editor != nullptr, "editor created");
            if (editor == nullptr) return;

            editor->setSize (juce::roundToInt ((float) editor->getWidth() * scaleFactor),
                             juce::roundToInt ((float) editor->getHeight() * scaleFactor));
            auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);

            juce::File file = outDir.getChildFile (fileName);
            file.deleteFile();
            juce::FileOutputStream stream (file);
            juce::PNGImageFormat png;
            check (stream.openedOk() && png.writeImageToStream (image, stream), "wrote " + fileName
                   + " (" + juce::String (image.getWidth()) + "x" + juce::String (image.getHeight()) + ")");
        };

        FreaxVolumeAudioProcessor a;
        snapshot (a, "freaxvolume_default.png", 1.0f);

        FreaxVolumeAudioProcessor b;
        setPlain (b, ParamID::volume, 82.0f);
        setPlain (b, ParamID::width, 240.0f);
        setPlain (b, ParamID::mono, 1.0f);
        snapshot (b, "freaxvolume_up_mono.png", 1.0f);

        FreaxVolumeAudioProcessor c;
        setPlain (c, ParamID::volume, 18.0f);
        setPlain (c, ParamID::width, 0.0f);
        setPlain (c, ParamID::mute, 1.0f);
        snapshot (c, "freaxvolume_down_mute.png", 1.0f);

        // Knobs skin
        FreaxVolumeAudioProcessor e;
        Skins::save (e.getState(), SkinId::knobs);
        snapshot (e, "knobs_default.png", 1.0f);

        FreaxVolumeAudioProcessor f;
        Skins::save (f.getState(), SkinId::knobs);
        setPlain (f, ParamID::volume, 82.0f);
        setPlain (f, ParamID::width, 240.0f);
        setPlain (f, ParamID::mono, 1.0f);
        snapshot (f, "knobs_up_mono.png", 1.0f);

        FreaxVolumeAudioProcessor h;
        Skins::save (h.getState(), SkinId::knobs);
        setPlain (h, ParamID::volume, 18.0f);
        setPlain (h, ParamID::width, 0.0f);
        setPlain (h, ParamID::mute, 1.0f);
        snapshot (h, "knobs_down_mute.png", 1.0f);
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File outDir = argc > 1 ? juce::File (juce::String (argv[1]))
                                       : juce::File::getCurrentWorkingDirectory().getChildFile ("snapshots");

    testDirectInstance();

    const auto plugins = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins");

    {
        juce::VST3PluginFormat vst3;
        testHosted (vst3, plugins.getChildFile ("VST3/FreaxVolume.vst3"));
    }
   #if JUCE_MAC
    {
        juce::AudioUnitPluginFormat au;
        testHosted (au, plugins.getChildFile ("Components/FreaxVolume.component"));
    }
   #endif

    stressEditorLifetime();
    renderEditor (outDir);

    std::cout << "\n" << (failures == 0 ? "ALL CHECKS PASSED" : juce::String (failures) + " CHECK(S) FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
