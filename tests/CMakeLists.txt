# DSP tests only depend on juce_dsp (SmoothedValue, IIR, FFT, WindowingFunction) --
# no need to pull in juce_audio_processors/juce_gui_extra just to exercise the engine.
add_executable(DesireEngineTests
    DesireEngineTests.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/DesireEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/SpectrumAnalyser.cpp"
)

target_include_directories(DesireEngineTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../Source")

target_link_libraries(DesireEngineTests PRIVATE
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(DesireEngineTests PRIVATE JUCE_STANDALONE_APPLICATION=1 JUCE_USE_CURL=0)

add_test(NAME DesireEngineTests COMMAND DesireEngineTests)

# Exercises PresetManager through the same APVTS -> DesireEngine path
# PluginProcessor uses in production, without pulling in the WebView editor.
add_executable(PresetManagerTests
    PresetManagerTests.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/DesireEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/PresetManager.cpp"
)

target_include_directories(PresetManagerTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../Source")

target_link_libraries(PresetManagerTests PRIVATE
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(PresetManagerTests PRIVATE JUCE_STANDALONE_APPLICATION=1 JUCE_USE_CURL=0)

# juce_audio_processors pulls in juce_gui_extra transitively (AudioProcessorEditor
# lives there), and juce_gui_extra.cpp unconditionally #includes <gtk/gtk.h> on
# Linux -- regardless of whether this target actually uses any GUI. JUCE only
# wires up that include path automatically for targets built via
# juce_add_plugin(NEEDS_WEB_BROWSER ...); juce::pkgconfig_JUCE_BROWSER_LINUX_DEPS
# is a headers-only (NOLINK) interface target JUCE always creates on Linux, so
# it's safe to attach directly here too.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(PresetManagerTests PRIVATE juce::pkgconfig_JUCE_BROWSER_LINUX_DEPS)
endif()

add_test(NAME PresetManagerTests COMMAND PresetManagerTests)
