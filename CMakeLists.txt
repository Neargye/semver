cmake_minimum_required(VERSION 3.15)

project(semver
    VERSION "1.0.1"
    HOMEPAGE_URL "https://github.com/Neargye/semver"
    DESCRIPTION "Header-only C++17 library for Semantic Versioning 2.0.0"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(SEMVER_IS_TOP_LEVEL ON)
else()
    set(SEMVER_IS_TOP_LEVEL OFF)
endif()

option(SEMVER_OPT_BUILD_EXAMPLES "Build semver examples" ${SEMVER_IS_TOP_LEVEL})
option(SEMVER_OPT_BUILD_TESTS "Build and perform semver tests" ${SEMVER_IS_TOP_LEVEL})
option(SEMVER_OPT_INSTALL "Generate and install semver target" ${SEMVER_IS_TOP_LEVEL})

set(EXPORT_NAMESPACE "${PROJECT_NAME}::")

add_library("${PROJECT_NAME}" INTERFACE)
add_library("${EXPORT_NAMESPACE}${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")
set(INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include")

function(semver_check_version_component component expected)
    file(STRINGS "${INCLUDES}/semver.hpp" header_version_definition
        REGEX "^#define SEMVER_VERSION_${component} [0-9]+$"
    )
    if(NOT "${header_version_definition}" MATCHES "^#define SEMVER_VERSION_${component} ([0-9]+)$")
        message(FATAL_ERROR
            "Could not read SEMVER_VERSION_${component} from include/semver.hpp"
        )
    endif()
    if(NOT "${CMAKE_MATCH_1}" STREQUAL "${expected}")
        message(FATAL_ERROR
            "Project version ${PROJECT_VERSION} does not match include/semver.hpp: "
            "SEMVER_VERSION_${component} is ${CMAKE_MATCH_1}, expected ${expected}"
        )
    endif()
endfunction()

semver_check_version_component(MAJOR "${PROJECT_VERSION_MAJOR}")
semver_check_version_component(MINOR "${PROJECT_VERSION_MINOR}")
semver_check_version_component(PATCH "${PROJECT_VERSION_PATCH}")

target_include_directories("${PROJECT_NAME}"
    INTERFACE
        $<BUILD_INTERFACE:${INCLUDES}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features("${PROJECT_NAME}" INTERFACE cxx_std_17)

if(SEMVER_OPT_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(SEMVER_OPT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(SEMVER_OPT_INSTALL)
    set(CMAKE_CONFIG_FILE_BASENAME "${PROJECT_NAME}Config.cmake")
    set(CMAKE_CONFIG_VERSION_FILE_BASENAME "${PROJECT_NAME}ConfigVersion.cmake")
    set(CMAKE_CONFIG_VERSION_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CONFIG_VERSION_FILE_BASENAME}")

    install(TARGETS "${PROJECT_NAME}"
        EXPORT "${PROJECT_NAME}"
    )
    install(FILES "${INCLUDES}/semver.hpp"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    )

    install(EXPORT "${PROJECT_NAME}"
        FILE "${CMAKE_CONFIG_FILE_BASENAME}"
        NAMESPACE "${EXPORT_NAMESPACE}"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )
    export(EXPORT "${PROJECT_NAME}"
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CONFIG_FILE_BASENAME}"
        NAMESPACE "${EXPORT_NAMESPACE}"
    )

    write_basic_package_version_file(
        "${CMAKE_CONFIG_VERSION_FILE_NAME}"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )
    install(FILES "${CMAKE_CONFIG_VERSION_FILE_NAME}"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )
endif()
