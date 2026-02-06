# ======================= Project Configuration =========================

cmake_minimum_required(VERSION 3.5...3.27)
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
   stb_image
   freetype
)

# Gather source files for the core game library
file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/*.h
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/*.hpp
)

# Create a static library target for game engine/core code
add_library(TheStove STATIC ${SOURCE_FILES})

# Enable precompiled headers for faster builds (CMake 3.16+)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(TheStove PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/TheStove/Core/Precompiled.hpp
    )
endif()

# Include directories for the project
target_include_directories(TheStove PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/TheStove
    ${glm_SOURCE_DIR} # glm include from FetchContent
)

target_link_libraries(TheStove PUBLIC ${ALL_LIBS})

# Set C++ standard for the library
set_property(TARGET TheStove PROPERTY CXX_STANDARD 20)

# Create executable target, typically with only main.cpp or launcher
add_executable(LetsEatMonsters ${CMAKE_CURRENT_LIST_DIR}/TheStove/main.cpp)

target_link_libraries(LetsEatMonsters PRIVATE TheStove)

# Hide console window in Release builds (Windows subsystem instead of Console)
if(WIN32)
    set_target_properties(LetsEatMonsters PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    )
    # Use mainCRTStartup entry point so we can keep using int main() instead of WinMain
    target_link_options(LetsEatMonsters PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>
    )
endif()

# Set C++ standard for the build
set_property(TARGET LetsEatMonsters PROPERTY CXX_STANDARD 20)

# Set the working directory for Visual Studio debugging to the build directory
# This ensures relative paths like ../assets work correctly
set_property(TARGET LetsEatMonsters PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

# Copy FMOD DLL to the executable's output directory
copy_fmod_dll_to_target(LetsEatMonsters)

# Copy assets folder to the executable's output directory for run.bat
add_custom_command(TARGET LetsEatMonsters POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "${CMAKE_BINARY_DIR}/assets"
    COMMENT "Copying assets to build directory"
)

# Copy shader files to the build directory for run.bat
add_custom_command(TARGET LetsEatMonsters POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/TheStove/Graphics/shaders"
    "${CMAKE_BINARY_DIR}/shaders"
    COMMENT "Copying shader files to build directory"
)

# Copy level files to the build directory for run.bat
add_custom_command(TARGET LetsEatMonsters POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/levels"
    "${CMAKE_BINARY_DIR}/levels"
    COMMENT "Copying shader files to build directory"
)

# Copy prefab files to the build directory for run.bat
add_custom_command(TARGET LetsEatMonsters POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/prefabs"
    "${CMAKE_BINARY_DIR}/prefabs"
    COMMENT "Copying prefab files to build directory"
)

# Copy extern files to the build directory for run.bat
add_custom_command(TARGET LetsEatMonsters POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/extern"
    "${CMAKE_BINARY_DIR}/extern"
    COMMENT "Copying extern files to build directory"
)

# Add compiler warnings and flags based on compiler
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(TheStove PRIVATE -Wall)
    target_compile_options(LetsEatMonsters PRIVATE -Wall)
elseif (MSVC)
    # /W4 = Warning level 4
    # /MP = Multi-processor compilation (parallel builds)
    # /Gm- = Disable minimal rebuild (incompatible with /MP but /MP is faster)
    target_compile_options(TheStove PRIVATE /W4 /WX- /MP)
    target_compile_options(LetsEatMonsters PRIVATE /W4 /WX- /MP)
endif()
