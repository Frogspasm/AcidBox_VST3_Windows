cmake_minimum_required(VERSION 3.22)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()

project(AcidBox VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ACIDBOX_COPY_AFTER_BUILD "Install the plugin into your system plugin folder after building" OFF)

# JUCE is downloaded automatically the first time you configure
include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(ACIDBOX_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND ACIDBOX_FORMATS AU)
endif()

juce_add_plugin(AcidBox
  COMPANY_NAME "AcidBox"
  PRODUCT_NAME "AcidBox"
  BUNDLE_ID "com.acidbox.acidbox"
  PLUGIN_MANUFACTURER_CODE Acbx
  PLUGIN_CODE Acd1
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  VST3_CATEGORIES Instrument Synth
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  COPY_PLUGIN_AFTER_BUILD ${ACIDBOX_COPY_AFTER_BUILD}
  FORMATS ${ACIDBOX_FORMATS})

target_sources(AcidBox PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(AcidBox PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AcidBox
  PRIVATE
    juce::juce_audio_utils
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
