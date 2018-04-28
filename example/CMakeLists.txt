include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME}_example
        ${CMAKE_SOURCE_DIR}/include/semver.hpp
        example.cpp)
