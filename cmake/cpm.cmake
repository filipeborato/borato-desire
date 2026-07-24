set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${LIB_DIR}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
get_filename_component(CPM_DOWNLOAD_LOCATION "${CPM_DOWNLOAD_LOCATION}" ABSOLUTE)

if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    message(STATUS "Downloading CPM.cmake ${CPM_DOWNLOAD_VERSION}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}"
        TLS_VERIFY ON
    )
endif()

include("${CPM_DOWNLOAD_LOCATION}")
