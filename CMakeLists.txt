cmake_minimum_required(VERSION 3.27)
project(evolution_sim)

set(CMAKE_CXX_STANDARD 20)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
#add_subdirectory(vendored/SDL3 EXCLUDE_FROM_ALL)
#add_subdirectory(vendored/SDL3_ttf EXCLUDE_FROM_ALL)

include_directories(${CMAKE_SOURCE_DIR}/vendored/SDL3/x86_64-w64-mingw32/include ${CMAKE_SOURCE_DIR}/vendored/SDL3_ttf/x86_64-w64-mingw32/include)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    message("Building on Windows")
    link_directories(${CMAKE_SOURCE_DIR}/vendored/SDL3_ttf/x86_64-w64-mingw32/lib)
    set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vendored/SDL3/cmake") #"${CMAKE_SOURCE_DIR}/vendored/SDL3_ttf/cmake")
    find_package(SDL3 REQUIRED)
    add_executable(evolution_sim WIN32 main.cpp
            NeuralNet.cpp
            NeuralNet.hpp)
    target_sources(evolution_sim PRIVATE ${CMAKE_SOURCE_DIR}/renderer/SDL3/clay_renderer_SDL3.c)
    target_link_libraries(evolution_sim PRIVATE SDL3::SDL3 SDL3_ttf)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message("Building on macOS")
    set(CMAKE_PREFIX_PATH "~/Library/Frameworks") #"${CMAKE_SOURCE_DIR}/vendored/SDL3_ttf/cmake")
    find_package(SDL3 REQUIRED)
    find_package(SDL3_ttf REQUIRED)
    add_executable(evolution_sim MACOSX_BUNDLE main.cpp
            NeuralNet.cpp
            NeuralNet.hpp)
    target_sources(evolution_sim PRIVATE ${CMAKE_SOURCE_DIR}/renderer/SDL3/clay_renderer_SDL3.c)
    target_link_libraries(evolution_sim PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
endif()