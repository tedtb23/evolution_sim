cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 20)

project(evolution_sim)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

#set(SDLTTF_VENDORED ON)

add_subdirectory(vendored/SDL3 EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL3_image EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL3_ttf EXCLUDE_FROM_ALL )

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    message("Building on Windows")
    add_executable(evolution_sim WIN32)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message("Building on macOS")
    add_executable(evolution_sim MACOSX_BUNDLE)
    file(COPY resources/arial.ttf DESTINATION "${CMAKE_BINARY_DIR}/evolution_sim.app/Contents/resources")
    file(COPY resources/images DESTINATION "${CMAKE_BINARY_DIR}/evolution_sim.app/Contents/resources")
endif()
target_sources(evolution_sim PRIVATE
        main.cpp
        NeuralNet.cpp
        Organism.cpp
        Simulation.cpp
        QuadTree.cpp
        SimObject.cpp
        SimUtils.cpp)
target_link_libraries(evolution_sim PRIVATE SDL3_ttf::SDL3_ttf SDL3_image::SDL3_image SDL3::SDL3)