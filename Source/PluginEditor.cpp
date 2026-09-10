#include "PluginEditor.h"
#include "FamilySkin.h"
#include "MinimalistSkin.h"

FreaxVolumeAudioProcessorEditor::FreaxVolumeAudioProcessorEditor (FreaxVolumeAudioProcessor& p)
    : AudioProcessorEditor (&p), freaxVolumeProcessor (p)
{
    setOpaque (true);
    showSkin (Skins::load (freaxVolumeProcessor.getState()));
}

FreaxVolumeAudioProcessorEditor::~FreaxVolumeAudioProcessorEditor()
{
    skin.reset();
}

void FreaxVolumeAudioProcessorEditor::showSkin (SkinId id)
{
    skin.reset();

    if (id == SkinId::minimalist)
        skin = std::make_unique<MinimalistSkin> (freaxVolumeProcessor);
    else
        skin = std::make_unique<FamilySkin> (freaxVolumeProcessor);

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

            Skins::save (safeThis->freaxVolumeProcessor.getState(), next);
            safeThis->showSkin (next);
        });
    };

    addAndMakeVisible (*skin);

    // Only the Minimalist skin scales; the dark skin keeps the fixed size of the other Freaxment plugins.
    setResizable (true, id == SkinId::minimalist);

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
