#include "PluginEditor.h"
#include "MinimalistSkin.h"
#include "FlexSkin.h"

FreaxVolumeAudioProcessorEditor::FreaxVolumeAudioProcessorEditor (FreaxVolumeAudioProcessor& p)
    : AudioProcessorEditor (&p), freaxvolumeProcessor (p)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    showSkin (Skins::load (freaxvolumeProcessor.getState()));
}

FreaxVolumeAudioProcessorEditor::~FreaxVolumeAudioProcessorEditor()
{
    skin.reset();
    setLookAndFeel (nullptr);
}

void FreaxVolumeAudioProcessorEditor::showSkin (SkinId id)
{
    skin.reset();

    if (id == SkinId::flex)
        skin = std::make_unique<FlexSkin> (freaxvolumeProcessor);
    else
        skin = std::make_unique<MinimalistSkin> (freaxvolumeProcessor);

    skin->currentSkin = id;

    // The choice comes from a menu opened by the skin itself, so the swap is
    // deferred until that call has fully finished before the skin is destroyed.
    skin->onSelectSkin = [safeThis = juce::Component::SafePointer<FreaxVolumeAudioProcessorEditor> (this), id] (SkinId next)
    {
        if (next == id)
            return;

        juce::MessageManager::callAsync ([safeThis, next]
        {
            if (safeThis == nullptr)
                return;

            Skins::save (safeThis->freaxvolumeProcessor.getState(), next);
            safeThis->showSkin (next);
        });
    };

    addAndMakeVisible (*skin);

    if (auto* constrainer = getConstrainer())
        skin->configureConstrainer (*constrainer);

    const auto size = skin->defaultSize();

    if (getWidth() == size.x && getHeight() == size.y)
        resized();
    else
        setSize (size.x, size.y);
}

void FreaxVolumeAudioProcessorEditor::resized()
{
    if (skin != nullptr)
        skin->setBounds (getLocalBounds());
}
