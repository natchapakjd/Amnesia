cmake_minimum_required(VERSION 3.22)

project(Vayu VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JUCE_LOCAL_PATH "${CMAKE_SOURCE_DIR}/vendor/JUCE")

if(EXISTS "${JUCE_LOCAL_PATH}/CMakeLists.txt")
    add_subdirectory("${JUCE_LOCAL_PATH}" JUCE)
else()
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
    )

    FetchContent_MakeAvailable(juce)
endif()

# ── NeuralAmpModelerCore (NAM) ─────────────────────────────────────────────
option(VAYU_WITH_NAM "Download and link NeuralAmpModelerCore" ON)
if(VAYU_WITH_NAM)
    set(NAM_LOCAL_PATH "${CMAKE_SOURCE_DIR}/vendor/NeuralAmpModelerCore")
    if(EXISTS "${NAM_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${NAM_LOCAL_PATH}" NeuralAmpModelerCore)
    else()
        FetchContent_Declare(
            NeuralAmpModelerCore
            GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
            GIT_TAG        main
        )
        FetchContent_MakeAvailable(NeuralAmpModelerCore)
    endif()
endif()
# ───────────────────────────────────────────────────────────────────────────

juce_add_plugin(Vayu
    COMPANY_NAME "MyCompany"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Vayu
    PLUGIN_CODE Vayu
    FORMATS VST3 Standalone
    PRODUCT_NAME "Vayu"
)

juce_generate_juce_header(Vayu)

target_sources(Vayu
    PRIVATE
        src/PluginEditor.h
        src/PluginEditor.cpp
        src/PluginProcessor.h
        src/PluginProcessor.cpp
)

target_compile_definitions(Vayu
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(Vayu
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        $<$<BOOL:${VAYU_WITH_NAM}>:NeuralAmpModelerCore>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
