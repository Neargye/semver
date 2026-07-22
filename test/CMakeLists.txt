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
    set(OPTIONS /W4 /WX /FS)
    check_cxx_compiler_flag(/permissive- HAS_PERMISSIVE_FLAG)
    if(HAS_PERMISSIVE_FLAG)
        set(OPTIONS ${OPTIONS} /permissive-)
    endif()

    check_cxx_compiler_flag(/std:c++20 HAS_CPP20_FLAG)
    check_cxx_compiler_flag(/std:c++latest HAS_CPPLATEST_FLAG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(OPTIONS -Wall -Wextra -Wundef -pedantic-errors -Werror)

    check_cxx_compiler_flag(-std=c++20 HAS_CPP20_FLAG)
endif()

function(make_test target std)
    add_executable(${target} ${SOURCES})
    target_compile_definitions(${target} PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)
    target_compile_options(${target} PRIVATE ${OPTIONS})
    target_include_directories(${target} SYSTEM PRIVATE 3rdparty/doctest)
    target_link_libraries(${target} PRIVATE ${PROJECT_NAME})
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(std)
        if(SEMVER_MSVC_FRONTEND)
            target_compile_options(${target} PRIVATE /std:${std})
        else()
            target_compile_options(${target} PRIVATE -std=${std})
        endif()
    endif()
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

make_test(${PROJECT_NAME}-cpp17.t c++17)
make_config_test(${PROJECT_NAME}-config-cpp17.t)

if(HAS_CPP20_FLAG)
    make_test(${PROJECT_NAME}-cpp20.t c++20)
endif()

if(HAS_CPPLATEST_FLAG)
    make_test(${PROJECT_NAME}-cpplatest.t c++latest)
endif()
