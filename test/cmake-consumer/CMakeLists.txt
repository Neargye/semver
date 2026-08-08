cmake_minimum_required(VERSION 3.15)

project(semver_consumer LANGUAGES CXX)

enable_testing()

if(NOT DEFINED SEMVER_EXPECTED_VERSION OR SEMVER_EXPECTED_VERSION STREQUAL "")
    message(FATAL_ERROR "SEMVER_EXPECTED_VERSION is required")
endif()

find_package(semver "${SEMVER_EXPECTED_VERSION}" EXACT CONFIG REQUIRED)

add_executable(semver_consumer main.cpp)
target_link_libraries(semver_consumer PRIVATE semver::semver)
target_compile_features(semver_consumer PRIVATE cxx_std_17)
set_target_properties(semver_consumer PROPERTIES CXX_EXTENSIONS OFF)

add_test(NAME semver_consumer COMMAND semver_consumer)
