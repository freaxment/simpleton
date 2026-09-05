cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel). Must be set before project().
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)

project(Simpleton VERSION 1.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_subdirectory(libs/JUCE)

# Embedded assets (Freaxment logo)
juce_add_binary_data(SimpletonAssets SOURCES Resources/freaxment_logo.svg)

juce_add_plugin(Simpleton
    PRODUCT_NAME                "Simpleton"
    COMPANY_NAME                "Freaxment"
    COMPANY_WEBSITE             ""
    BUNDLE_ID                   "com.freaxment.simpleton"
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
    COPY_PLUGIN_AFTER_BUILD     TRUE)

target_sources(Simpleton PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/MinimalistSkin.cpp
    Source/FlexSkin.cpp)

target_include_directories(Simpleton PRIVATE Source)

target_compile_definitions(Simpleton PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Simpleton PRIVATE
    SimpletonAssets
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Optional self-test host: numeric checks of the knob laws, VST3/AU hosting, GUI snapshots.
option(SIMPLETON_BUILD_TESTS "Build the Simpleton self-test host" OFF)

if(SIMPLETON_BUILD_TESTS)
    juce_add_console_app(SimpletonTest PRODUCT_NAME "SimpletonTest")

    target_sources(SimpletonTest PRIVATE
        tests/SimpletonTest.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/MinimalistSkin.cpp
        Source/FlexSkin.cpp)

    target_include_directories(SimpletonTest PRIVATE Source)

    target_compile_definitions(SimpletonTest PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=1
        $<$<PLATFORM_ID:Darwin>:JUCE_PLUGINHOST_AU=1>
        JucePlugin_Name="Simpleton")

    target_link_libraries(SimpletonTest PRIVATE
        SimpletonAssets
        juce::juce_audio_utils
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()
