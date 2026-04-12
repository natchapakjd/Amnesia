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
    # Resolve source directory: vendor/ → _deps/ (FetchContent cache) → download fresh
    set(_NAM_VENDOR "${CMAKE_SOURCE_DIR}/vendor/NeuralAmpModelerCore")
    set(_NAM_DEPS   "${CMAKE_BINARY_DIR}/_deps/neuralampmodelercore-src")

    if(EXISTS "${_NAM_VENDOR}/NAM/get_dsp.h")
        set(NAM_SOURCE_DIR "${_NAM_VENDOR}")
    elseif(EXISTS "${_NAM_DEPS}/NAM/get_dsp.h")
        set(NAM_SOURCE_DIR "${_NAM_DEPS}")
    else()
        FetchContent_Declare(
            NeuralAmpModelerCore
            GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
            GIT_TAG        main
        )
        FetchContent_Populate(NeuralAmpModelerCore)
        set(NAM_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/neuralampmodelercore-src")
    endif()

    message(STATUS "NAM source dir: ${NAM_SOURCE_DIR}")

    file(GLOB NAM_SOURCES "${NAM_SOURCE_DIR}/NAM/*.cpp")
    set(VAYU_NAM_SOURCES ${NAM_SOURCES})
    set(VAYU_NAM_INCLUDE_DIRS
        "${NAM_SOURCE_DIR}"
        "${NAM_SOURCE_DIR}/Dependencies/eigen"
        "${NAM_SOURCE_DIR}/Dependencies/nlohmann"
        "${NAM_SOURCE_DIR}/Dependencies/AudioDSPTools"
    )
    if(WIN32)
        set(VAYU_NAM_COMPILE_DEFINITIONS NOMINMAX WIN32_LEAN_AND_MEAN NAM_SAMPLE_FLOAT)
    else()
        set(VAYU_NAM_COMPILE_DEFINITIONS NAM_SAMPLE_FLOAT)
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
    src/ModernKnobLookAndFeel.h
    src/ModernKnobLookAndFeel.cpp
        src/PluginEditor.h
        src/PluginEditor.cpp
        src/PluginProcessor.h
        src/PluginProcessor.cpp
        ${VAYU_NAM_SOURCES}
)

if(VAYU_WITH_NAM)
    target_include_directories(Vayu PRIVATE ${VAYU_NAM_INCLUDE_DIRS})
endif()

target_compile_definitions(Vayu
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        $<$<BOOL:${VAYU_WITH_NAM}>:NAM_SAMPLE_FLOAT>
    PRIVATE
        ${VAYU_NAM_COMPILE_DEFINITIONS}
)

target_link_libraries(Vayu
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
