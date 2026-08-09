include(CheckCXXCompilerFlag)

set(SOURCES
    test.cpp
    test_operators.cpp
    test_parse.cpp
    test_ranges.cpp
    test_to_string.cpp
    test_validation.cpp
    test_api.cpp
    test_advanced_ranges.cpp
    test_edge_cases.cpp
)

if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(SEMVER_MSVC_FRONTEND ON)
else()
    set(SEMVER_MSVC_FRONTEND OFF)
endif()

if(SEMVER_MSVC_FRONTEND)
    list(APPEND SOURCES test_windows_minmax.cpp)
endif()

if(SEMVER_MSVC_FRONTEND)
    set(OPTIONS /W4 /WX /FS)
    check_cxx_compiler_flag(/permissive- HAS_PERMISSIVE_FLAG)
    if(HAS_PERMISSIVE_FLAG)
        set(OPTIONS ${OPTIONS} /permissive-)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(OPTIONS -Wall -Wextra -Wundef -pedantic-errors -Werror)
endif()

function(make_test target std)
    add_executable(${target} ${SOURCES})
    target_compile_definitions(${target} PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)
    target_compile_options(${target} PRIVATE ${OPTIONS})
    target_include_directories(${target} SYSTEM PRIVATE 3rdparty/doctest)
    target_link_libraries(${target} PRIVATE ${PROJECT_NAME})
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_features(${target} PRIVATE cxx_std_${std})
    add_test(NAME ${target} COMMAND ${target})
endfunction()

function(make_config_test target)
    add_executable(${target} test_config.cpp)
    target_compile_definitions(${target} PRIVATE SEMVER_CONFIG_FILE=\"semver_test_config.hpp\")
    target_compile_options(${target} PRIVATE ${OPTIONS})
    target_include_directories(${target} PRIVATE config)
    target_link_libraries(${target} PRIVATE ${PROJECT_NAME})
    target_compile_features(${target} PRIVATE cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

make_test(${PROJECT_NAME}-cpp17.t 17)
make_config_test(${PROJECT_NAME}-config-cpp17.t)

if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    make_test(${PROJECT_NAME}-cpp20.t 20)
endif()

set(SEMVER_TEST_HIGH_STANDARDS_SUPPORTED ON)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 18
   AND NOT SEMVER_MSVC_FRONTEND)
    set(SEMVER_TEST_HIGH_STANDARDS_SUPPORTED OFF)
    message(STATUS
        "Skipping C++23/C++26 tests with Clang ${CMAKE_CXX_COMPILER_VERSION}: "
        "Clang before 18 has a known C++23 conditional-explicit bug (LLVM #59827).")
endif()

if(SEMVER_TEST_HIGH_STANDARDS_SUPPORTED AND "cxx_std_23" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    make_test(${PROJECT_NAME}-cpp23.t 23)
endif()

if(SEMVER_TEST_HIGH_STANDARDS_SUPPORTED AND "cxx_std_26" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    make_test(${PROJECT_NAME}-cpp26.t 26)
endif()
