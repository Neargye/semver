cmake_minimum_required(VERSION 3.15)

project(semver_consumer LANGUAGES CXX)

enable_testing()

find_package(semver 1 CONFIG REQUIRED)

add_executable(semver_consumer main.cpp)
target_link_libraries(semver_consumer PRIVATE semver::semver)
target_compile_features(semver_consumer PRIVATE cxx_std_17)
set_target_properties(semver_consumer PROPERTIES CXX_EXTENSIONS OFF)

add_test(NAME semver_consumer COMMAND semver_consumer)
