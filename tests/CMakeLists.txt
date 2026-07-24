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

add_test(NAME PresetManagerTests COMMAND PresetManagerTests)
