cmake_minimum_required(VERSION 3.22)

project(BoratoDesire VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(BORATO_DESIRE_JUCE_SOURCE "" CACHE PATH
    "Path to an existing JUCE 8.0.12 checkout (avoids downloading JUCE)")

if(BORATO_DESIRE_JUCE_SOURCE)
    message(STATUS "Using JUCE from ${BORATO_DESIRE_JUCE_SOURCE}")
    add_subdirectory("${BORATO_DESIRE_JUCE_SOURCE}" "${CMAKE_BINARY_DIR}/juce")
else()
    include(cmake/cpm.cmake)
    CPMAddPackage(
        NAME JUCE
        GITHUB_REPOSITORY juce-framework/JUCE
        GIT_TAG 8.0.12
        VERSION 8.0.12
        SOURCE_DIR "${LIB_DIR}/juce"
    )
endif()

if(MSVC)
    set(WEBVIEW2_PACKAGE
        "$ENV{USERPROFILE}/AppData/Local/PackageManagement/NuGet/Packages/Microsoft.Web.WebView2.1.0.3485.44")
    if(NOT EXISTS "${WEBVIEW2_PACKAGE}")
        message(STATUS "Installing the WebView2 SDK required by JUCE 8.0.12")
        find_program(POWERSHELL_EXE NAMES pwsh powershell REQUIRED)
        execute_process(
            COMMAND "${POWERSHELL_EXE}" -NoProfile -ExecutionPolicy Bypass
                    -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/DownloadWebView2.ps1"
            RESULT_VARIABLE WEBVIEW2_INSTALL_RESULT
        )
        if(NOT WEBVIEW2_INSTALL_RESULT EQUAL 0)
            message(FATAL_ERROR "WebView2 SDK installation failed (${WEBVIEW2_INSTALL_RESULT})")
        endif()
    endif()
endif()

add_subdirectory(plugin)

enable_testing()
add_subdirectory(tests)
