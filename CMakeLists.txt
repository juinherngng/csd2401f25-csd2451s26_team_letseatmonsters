# ======================= Project Configuration =========================

cmake_minimum_required(VERSION 2.8...3.13)
project (Lets-Eat-Monsters)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(ImportDependencies)
importDependencies()

# List of external libraries
set(ALL_LIBS
   glfw
   glad
   imgui
   fmod
)

# Gather source files for the core game library
file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/include/*.h
    ${CMAKE_CURRENT_LIST_DIR}/include/*.hpp
)

# Create a static library target for game engine/core code
add_library(GameEngine STATIC ${SOURCE_FILES})

# Include directories for the project
target_include_directories(GameEngine PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${glm_SOURCE_DIR} # glm include from FetchContent
)

target_link_libraries(GameEngine PUBLIC ${ALL_LIBS})

# Set C++ standard for the library
set_property(TARGET GameEngine PROPERTY CXX_STANDARD 20)

# Create executable target, typically with only main.cpp or launcher
add_executable(LetsEatMonsters ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)

target_link_libraries(LetsEatMonsters PRIVATE GameEngine)

# Copy FMOD DLL to the executable's output directory
copy_fmod_dll_to_target(LetsEatMonsters)

# Add compiler warnings and flags based on compiler
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(GameEngine PRIVATE -Wall)
    target_compile_options(LetsEatMonsters PRIVATE -Wall)
elseif (MSVC)
    target_compile_options(GameEngine PRIVATE /W3 /WX-)
    target_compile_options(LetsEatMonsters PRIVATE /W3 /WX-)
endif()
