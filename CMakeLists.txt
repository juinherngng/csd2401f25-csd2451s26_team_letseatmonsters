# ======================= Project Configuration =========================

cmake_minimum_required(VERSION 3.5...3.27)
project (Lets-Eat-Monsters)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(ImportDependencies)
importDependencies()

function(copy_runtime_directory target source_dir destination_dir description)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${source_dir}"
        "${CMAKE_BINARY_DIR}/${destination_dir}"
        COMMENT "${description}"
    )
endfunction()

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
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/*.h
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/*.hpp
)

set(GAME_FILES_IN_THESTOVE
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/PlayerLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/PlayerLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/TableLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/TableLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/WorkTableLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/WorkTableLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/CustomerTableLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/CustomerTableLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/IngredientBoxLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/IngredientBoxLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/ExitGateLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/ExitGateLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/TrashCanLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/TrashCanLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/OrderUILogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/OrderUILogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/MenuButtonLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/MenuButtonLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/HowToPlayButtonLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/HowToPlayButtonLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/PauseButtonLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/PauseButtonLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/PlateLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/PlateLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/IngredientLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/IngredientLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/CustomerManagerLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/CustomerManagerLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/SimpleNpcLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/SimpleNpcLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/CustomerOrderUILogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/CustomerOrderUILogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/Quota.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/Quota.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameCore/FoodTypes.hpp
)

list(REMOVE_ITEM SOURCE_FILES ${GAME_FILES_IN_THESTOVE})
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/TheStove/Main.cpp)

# Gather source files for game-specific code
file(GLOB_RECURSE GAME_SOURCE_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/*.h
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/*.hpp
)

# Create a static library target for game engine/core code
add_library(TheStove STATIC ${SOURCE_FILES})

# Enable precompiled headers for faster builds (CMake 3.16+)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    if(MSVC)
        # MSVC PCH artifacts have been getting invalidated across toolset/configuration
        # changes and causing Release builds to fail with stale `cmake_pch.pch` errors.
        # Keep MSVC builds deterministic by disabling CMake PCH for this target.
    else()
        target_precompile_headers(TheStove PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/TheStove/EngineCore/Precompiled.hpp
        )
    endif()
endif()

# Include directories for the project
target_include_directories(TheStove PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/TheStove
    ${glm_SOURCE_DIR} # glm include from FetchContent
)

target_link_libraries(TheStove PUBLIC ${ALL_LIBS})

# Game-specific library layered on top of TheStove
add_library(MyoonchiDiner STATIC ${GAME_SOURCE_FILES})

target_sources(MyoonchiDiner PRIVATE ${GAME_FILES_IN_THESTOVE})
target_sources(MyoonchiDiner PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameBootstrap.cpp
)

target_include_directories(MyoonchiDiner PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/EngineCore
    ${CMAKE_CURRENT_LIST_DIR}/TheStove
    ${glm_SOURCE_DIR}
)

target_link_libraries(MyoonchiDiner PUBLIC TheStove)

# Create executable target, typically with only main.cpp or launcher
set(LETS_EAT_MONSTERS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/Main.cpp
)

add_executable(LetsEatMonsters ${LETS_EAT_MONSTERS_SOURCES})

target_link_libraries(LetsEatMonsters PRIVATE MyoonchiDiner)

# Keep third-party dependency warnings from drowning out project warnings on MSVC.
if(MSVC AND TARGET freetype)
    target_compile_options(freetype PRIVATE /W0)
endif()

# Disable Visual Studio's vcpkg app-local post-build step so the project does not
# depend on `pwsh.exe` being available on the machine.
if(MSVC)
    set(_local_targets TheStove MyoonchiDiner LetsEatMonsters)
    foreach(_target IN LISTS _local_targets)
        if(TARGET ${_target})
            set_target_properties(${_target} PROPERTIES
                VS_GLOBAL_VcpkgEnabled "false"
                VS_GLOBAL_VcpkgApplocalDeps "false"
            )
        endif()
    endforeach()
endif()

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

# Set the working directory for Visual Studio debugging to the build directory
# This ensures relative paths like ../assets work correctly
set_property(TARGET LetsEatMonsters PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

# Copy FMOD DLL to the executable's output directory
copy_fmod_dll_to_target(LetsEatMonsters)

copy_runtime_directory(LetsEatMonsters "${CMAKE_SOURCE_DIR}/assets" "assets" "Copying assets to build directory")
copy_runtime_directory(LetsEatMonsters "${CMAKE_SOURCE_DIR}/TheStove/EngineGraphics/shaders" "shaders" "Copying shader files to build directory")
copy_runtime_directory(LetsEatMonsters "${CMAKE_SOURCE_DIR}/levels" "levels" "Copying level files to build directory")
copy_runtime_directory(LetsEatMonsters "${CMAKE_SOURCE_DIR}/prefabs" "prefabs" "Copying prefab files to build directory")

# Add compiler warnings and flags based on compiler
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(TheStove PRIVATE -Wall)
    target_compile_options(MyoonchiDiner PRIVATE -Wall)
    target_compile_options(LetsEatMonsters PRIVATE -Wall)
elseif (MSVC)
    # /W4 = Warning level 4
    # /MP = Multi-processor compilation (parallel builds)
    # /Gm- = Disable minimal rebuild (incompatible with /MP but /MP is faster)
    target_compile_options(TheStove PRIVATE /W4 /WX- /MP)
    target_compile_options(MyoonchiDiner PRIVATE /W4 /WX- /MP)
    target_compile_options(LetsEatMonsters PRIVATE /W4 /WX- /MP)
endif()
