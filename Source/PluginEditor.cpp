#include "PluginEditor.h"
#include "MinimalistSkin.h"
#include "FlexSkin.h"

SimpletonAudioProcessorEditor::SimpletonAudioProcessorEditor (SimpletonAudioProcessor& p)
    : AudioProcessorEditor (&p), simpletonProcessor (p)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    showSkin (Skins::load (simpletonProcessor.getState()));
}

SimpletonAudioProcessorEditor::~SimpletonAudioProcessorEditor()
{
    skin.reset();
    setLookAndFeel (nullptr);
}

void SimpletonAudioProcessorEditor::showSkin (SkinId id)
{
    skin.reset();

    if (id == SkinId::flex)
        skin = std::make_unique<FlexSkin> (simpletonProcessor);
    else
        skin = std::make_unique<MinimalistSkin> (simpletonProcessor);

    skin->currentSkin = id;

    // The choice comes from a menu opened by the skin itself, so the swap is
    // deferred until that call has fully finished before the skin is destroyed.
    skin->onSelectSkin = [safeThis = juce::Component::SafePointer<SimpletonAudioProcessorEditor> (this), id] (SkinId next)
    {
        if (next == id)
            return;

        juce::MessageManager::callAsync ([safeThis, next]
        {
            if (safeThis == nullptr)
                return;

            Skins::save (safeThis->simpletonProcessor.getState(), next);
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

void SimpletonAudioProcessorEditor::resized()
{
    if (skin != nullptr)
        skin->setBounds (getLocalBounds());
}
