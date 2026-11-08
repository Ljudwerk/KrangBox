cmake_minimum_required(VERSION 3.22)

project(KrangBox VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Lägsta macOS-version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")

option(KRANG_BUILD_TOOLS "Bygg KrangRender (offline-rendering för lyssningstest)" ON)

# --- Beroenden -------------------------------------------------------------
# Hämtas automatiskt. För att använda en lokal kopia:
#   cmake -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=/sökväg/till/JUCE
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)

FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES_RECURSE TRUE)

FetchContent_MakeAvailable(JUCE clap-juce-extensions)

# Det delade biblioteket (header-only), bredvid den här mappen. Effekt-DSP:n (Ångledningen) och
# Krang-stilen ligger där och delas med effektlådan Ångledningen.
set(KRANG_LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../krang-libs" CACHE PATH "Sökväg till krang-libs")
add_subdirectory(${KRANG_LIBS_DIR} krang-libs)

# --- Pluginet --------------------------------------------------------------
set(KRANG_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND KRANG_FORMATS AU)
endif()

juce_add_plugin(KrangBox
    COMPANY_NAME                "Doktor Krang"
    BUNDLE_ID                   "org.krangbox.krangbox"
    PLUGIN_MANUFACTURER_CODE    Krng
    PLUGIN_CODE                 Kbx1
    PRODUCT_NAME                "KrangBox"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    FORMATS                     ${KRANG_FORMATS})

clap_juce_extensions_plugin(TARGET KrangBox
    CLAP_ID "org.krangbox.krangbox"
    CLAP_FEATURES instrument synthesizer distortion)

# clap-juce-extensions lägger inte CLAP-målet i KrangBox_All; gör det här.
add_dependencies(KrangBox_All KrangBox_CLAP)

target_sources(KrangBox PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(KrangBox PRIVATE Source)

target_compile_definitions(KrangBox PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)   # Tillåtet under AGPLv3 / JUCE:s fria licens

target_link_libraries(KrangBox
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        krang::libs
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Verktyg ---------------------------------------------------------------
if(KRANG_BUILD_TOOLS)
    juce_add_console_app(KrangRender PRODUCT_NAME "KrangRender")
    target_sources(KrangRender PRIVATE tools/KrangRender.cpp)
    target_include_directories(KrangRender PRIVATE Source)
    target_compile_definitions(KrangRender PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(KrangRender
        PRIVATE juce::juce_audio_formats juce::juce_dsp juce::juce_audio_processors krang::libs
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

    # Ritar stilprovet till PNG, så att grafiken kan granskas utan DAW.
    juce_add_console_app(KrangSnapshot PRODUCT_NAME "KrangSnapshot")
    # Klicktestet öppnar den riktiga panelen med den riktiga processorn, därför byggs de in här.
    target_sources(KrangSnapshot PRIVATE tools/KrangSnapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(KrangSnapshot PRIVATE Source)
    target_compile_definitions(KrangSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1 JucePlugin_Name="KrangBox")
    target_link_libraries(KrangSnapshot
        PRIVATE juce::juce_gui_basics juce::juce_gui_extra juce::juce_dsp juce::juce_audio_processors juce::juce_audio_utils krang::libs
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
