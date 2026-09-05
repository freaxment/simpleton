#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// Simpleton ships with two skins that drive the same four parameters.
enum class SkinId { minimalist, flex };

namespace Skins
{
    // Plain literal on purpose: nothing with a destructor lives at static scope,
    // so unloading the plugin or quitting the host never runs JUCE code late.
    inline constexpr const char* property = "skin";

    inline juce::String toString (SkinId skin)            { return skin == SkinId::flex ? "flex" : "minimalist"; }
    inline SkinId fromString (const juce::String& text)   { return text == "flex" ? SkinId::flex : SkinId::minimalist; }

    // The chosen skin lives in the plugin state, so it is saved with the project.
    inline SkinId load (juce::AudioProcessorValueTreeState& state)
    {
        return fromString (state.state.getProperty (property, "minimalist").toString());
    }

    inline void save (juce::AudioProcessorValueTreeState& state, SkinId skin)
    {
        state.state.setProperty (property, toString (skin), nullptr);
    }

    inline SkinId other (SkinId skin) { return skin == SkinId::flex ? SkinId::minimalist : SkinId::flex; }
}

//==============================================================================
// A skin is a full-window component that owns its widgets and attachments.
// Right-clicking the background opens a small menu to pick the other skin.
class SkinView : public juce::Component
{
public:
    ~SkinView() override = default;

    virtual juce::Point<int> defaultSize() const = 0;
    virtual void configureConstrainer (juce::ComponentBoundsConstrainer&) const = 0;

    SkinId currentSkin = SkinId::minimalist;
    std::function<void (SkinId)> onSelectSkin;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu())
            return;

        juce::PopupMenu menu;
        menu.addSectionHeader ("Skin");
        menu.addItem (1, "Minimalist", true, currentSkin == SkinId::minimalist);
        menu.addItem (2, "Flex",       true, currentSkin == SkinId::flex);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                            [safeThis = juce::Component::SafePointer<SkinView> (this)] (int result)
                            {
                                if (safeThis == nullptr || result == 0 || safeThis->onSelectSkin == nullptr)
                                    return;

                                safeThis->onSelectSkin (result == 2 ? SkinId::flex : SkinId::minimalist);
                            });
    }
};
