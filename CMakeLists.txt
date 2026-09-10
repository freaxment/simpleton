cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel). Must be set before project().
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)

project(FreaxVolume VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_subdirectory(libs/JUCE)

# Embedded assets: Freaxment logo + TikTok Sans (OFL), same set as the other Freaxment plugins
juce_add_binary_data(FreaxVolumeAssets
    HEADER_NAME Assets.h
    NAMESPACE Assets
    SOURCES
        Assets/freaxment_logo.svg
        Assets/fonts/TikTokSans-Regular.ttf
        Assets/fonts/TikTokSans-Medium.ttf
        Assets/fonts/TikTokSans-ExpandedSemiBold.ttf
        Assets/fonts/TikTokSans-ExpandedBold.ttf
        Assets/fonts/TikTokSans-ExpandedBlack.ttf)
set_target_properties(FreaxVolumeAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_plugin(FreaxVolume
    PRODUCT_NAME                "FreaxVolume"
    COMPANY_NAME                "Freaxment"
    COMPANY_WEBSITE             ""
    BUNDLE_ID                   "com.freaxment.freaxvolume"
    PLUGIN_MANUFACTURER_CODE    Frxm
    PLUGIN_CODE                 Smpl
    FORMATS                     VST3 AU
    VST3_CATEGORIES             Fx Tools
    AU_MAIN_TYPE                kAudioUnitType_Effect
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    HARDENED_RUNTIME_ENABLED    TRUE)

target_sources(FreaxVolume PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(FreaxVolume PRIVATE Source)

target_compile_definitions(FreaxVolume PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(FreaxVolume PRIVATE
    FreaxVolumeAssets
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Optional self-test host: numeric checks of the knob laws, VST3/AU hosting, GUI snapshots.
option(FREAXVOLUME_BUILD_TESTS "Build the FreaxVolume self-test host" OFF)

if(FREAXVOLUME_BUILD_TESTS)
    juce_add_console_app(FreaxVolumeTest PRODUCT_NAME "FreaxVolumeTest")

    target_sources(FreaxVolumeTest PRIVATE
        tests/FreaxVolumeTest.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

    target_include_directories(FreaxVolumeTest PRIVATE Source)

    target_compile_definitions(FreaxVolumeTest PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        $<$<PLATFORM_ID:Darwin>:JUCE_PLUGINHOST_AU=1>
        JucePlugin_Name="FreaxVolume")

    target_link_libraries(FreaxVolumeTest PRIVATE
        FreaxVolumeAssets
        juce::juce_audio_utils
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()
