cmake_minimum_required(VERSION 2.8)
project(errorcalculator)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${PROJECT_SOURCE_DIR})

list(APPEND CMAKE_CXX_FLAGS "-std=c++11")

add_executable(error error.cpp)
target_link_libraries(error ${CMAKE_DL_LIBS})

add_executable(ols ols.cpp)

add_executable(stddev stddev.cpp)

add_executable(approximate_tangent approximate_tangent.cpp)

add_subdirectory(differentiator)

install(TARGETS error ols stddev approximate_tangent
        RUNTIME DESTINATION bin)