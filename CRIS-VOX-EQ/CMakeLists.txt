cmake_minimum_required(VERSION 3.24)
project(CrisVoxEQ VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    add_compile_options(/utf-8)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
option(CRIS_BUILD_PLUGIN "Build JUCE plugin and standalone" ON)
option(CRIS_BUILD_TESTS "Build tests" ON)
add_library(cris_eq_dsp STATIC Source/dsp/Equalizer.cpp Source/dsp/Spectrum.cpp)
target_include_directories(cris_eq_dsp PUBLIC Source)
if(MSVC)
    target_compile_options(cris_eq_dsp PRIVATE /W4 /permissive-)
else()
    target_compile_options(cris_eq_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(CRIS_BUILD_TESTS)
    enable_testing()
    add_executable(cris_eq_dsp_tests tests/DspTests.cpp)
    target_link_libraries(cris_eq_dsp_tests PRIVATE cris_eq_dsp)
    add_test(NAME cris_eq_dsp_tests COMMAND cris_eq_dsp_tests)
endif()
if(CRIS_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7c9d3783b127263d72bb65fe0a7e2dc8a02a7ac2 GIT_SUBMODULES "")
    FetchContent_MakeAvailable(JUCE)
    juce_add_plugin(CrisVoxEQ COMPANY_NAME "CRIS Audio" BUNDLE_ID "audio.cris.voxeq"
        PLUGIN_MANUFACTURER_CODE CrAu PLUGIN_CODE CvEq
        PRODUCT_NAME "CRIS VOX EQ" DESCRIPTION "Eight-band vocal equalizer with analysis assistant"
        FORMATS VST3 Standalone IS_SYNTH FALSE IS_MIDI_EFFECT FALSE
        NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Fx EQ)
    target_sources(CrisVoxEQ PRIVATE Source/PluginProcessor.cpp Source/PluginProcessor.h
        Source/PluginEditor.cpp Source/PluginEditor.h Source/AnalysisWorker.cpp Source/AnalysisWorker.h
        Source/ui/EQGraph.cpp Source/ui/EQGraph.h Source/Presets.h)
    target_compile_definitions(CrisVoxEQ PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(CrisVoxEQ PRIVATE cris_eq_dsp juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags)
    if(CRIS_BUILD_TESTS)
        add_executable(cris_eq_integration_tests tests/IntegrationTests.cpp)
        target_include_directories(cris_eq_integration_tests PRIVATE Source "${juce_SOURCE_DIR}/modules")
        target_compile_definitions(cris_eq_integration_tests PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
            CRIS_TEST_VOCAL="${juce_SOURCE_DIR}/examples/Assets/singing.ogg")
        target_link_libraries(cris_eq_integration_tests PRIVATE CrisVoxEQ cris_eq_dsp)
        add_test(NAME cris_eq_integration_tests COMMAND cris_eq_integration_tests)
    endif()
endif()
