set(BORATO_DESIRE_UI_BACKEND "WEBVIEW" CACHE STRING "UI backend: WEBVIEW, NATIVE, or AUTO")
set_property(CACHE BORATO_DESIRE_UI_BACKEND PROPERTY STRINGS WEBVIEW NATIVE AUTO)

set(DESIRE_WEBVIEW2_OPTIONS)
if(WIN32)
    list(APPEND DESIRE_WEBVIEW2_OPTIONS NEEDS_WEBVIEW2 TRUE)
endif()

set(DESIRE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND DESIRE_FORMATS AU)
endif()

juce_add_plugin(BoratoDesire
    COMPANY_NAME "Borato Company"
    BUNDLE_ID com.boratocompany.desire
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BoCo
    PLUGIN_CODE Dsre
    FORMATS ${DESIRE_FORMATS}
    PRODUCT_NAME "Borato Desire"
    VERSION ${PROJECT_VERSION}
    NEEDS_WEB_BROWSER TRUE
    ${DESIRE_WEBVIEW2_OPTIONS}
)

target_sources(BoratoDesire PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/ParameterIDs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginProcessor.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginEditor.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginEditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/DesireEngine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/DesireEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/SpectrumAnalyser.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/SpectrumAnalyser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/PresetManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/PresetManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/NativeEditorFallback.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Source/NativeEditorFallback.cpp"
)

target_include_directories(BoratoDesire PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../Source")

# Keep the browser tree shaped like the repository so the existing relative
# paths (Prototype -> Design/Assets) work both in python http.server and JUCE.
set(WEBVIEW_STAGE_DIR "${CMAKE_BINARY_DIR}/desire_webview")
set(WEBVIEW_ZIP "${CMAKE_BINARY_DIR}/desire_webview.zip")

add_custom_target(StageDesireWebView
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${WEBVIEW_STAGE_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${WEBVIEW_STAGE_DIR}/Design"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${WEBVIEW_STAGE_DIR}/Assets"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../Prototype" "${WEBVIEW_STAGE_DIR}/Prototype"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${WEBVIEW_STAGE_DIR}/Design/static-svg"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../Design/static-svg/borato-desire-shell.svg"
            "${CMAKE_CURRENT_SOURCE_DIR}/../Design/static-svg/borato-desire-toolbar.svg"
            "${CMAKE_CURRENT_SOURCE_DIR}/../Design/static-svg/borato-desire-display-frame.svg"
            "${CMAKE_CURRENT_SOURCE_DIR}/../Design/static-svg/borato-desire-knob-main-base.svg"
            "${CMAKE_CURRENT_SOURCE_DIR}/../Design/static-svg/borato-desire-knob-small-base.svg"
            "${WEBVIEW_STAGE_DIR}/Design/static-svg"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../Design/textures" "${WEBVIEW_STAGE_DIR}/Design/textures"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../Assets/borato-desire-dancer-art.svg"
            "${WEBVIEW_STAGE_DIR}/Assets/borato-desire-dancer-art.svg"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${JUCE_MODULES_DIR}/juce_gui_extra/native/javascript"
            "${WEBVIEW_STAGE_DIR}/Prototype/scripts/juce"
    COMMAND "${CMAKE_COMMAND}" -E tar cvf "${WEBVIEW_ZIP}" --format=zip "desire_webview"
    BYPRODUCTS "${WEBVIEW_ZIP}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Staging and zipping Desire WebView assets"
    VERBATIM
)

juce_add_binary_data(DesireWebViewFiles
    HEADER_NAME DesireWebViewFiles.h
    NAMESPACE desire_webview_files
    SOURCES "${WEBVIEW_ZIP}"
)
add_dependencies(DesireWebViewFiles StageDesireWebView)
add_dependencies(BoratoDesire StageDesireWebView)

target_link_libraries(BoratoDesire PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    DesireWebViewFiles
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(BoratoDesire PRIVATE
    DESIRE_WEBVIEW_ZIP_PREFIX="desire_webview/"
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

if(WIN32)
    target_compile_definitions(BoratoDesire PRIVATE JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()

if(BORATO_DESIRE_UI_BACKEND STREQUAL "NATIVE")
    target_compile_definitions(BoratoDesire PRIVATE DESIRE_UI_BACKEND_NATIVE=1)
elseif(BORATO_DESIRE_UI_BACKEND STREQUAL "AUTO")
    target_compile_definitions(BoratoDesire PRIVATE DESIRE_UI_BACKEND_AUTO=1)
else()
    target_compile_definitions(BoratoDesire PRIVATE DESIRE_UI_BACKEND_WEBVIEW=1)
endif()
