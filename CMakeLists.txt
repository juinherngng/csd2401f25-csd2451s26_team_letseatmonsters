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

set(GAME_FILES_IN_THESTOVE
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/PlayerLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/PlayerLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/TableLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/TableLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/WorkTableLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/WorkTableLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/CustomerTableLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/CustomerTableLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/IngredientBoxLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/IngredientBoxLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/ExitGateLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/ExitGateLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/TrashCanLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/TrashCanLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/OrderUILogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/OrderUILogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/MenuButtonLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/MenuButtonLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/HowToPlayButtonLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/HowToPlayButtonLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/PauseButtonLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/PauseButtonLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/PlateLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/PlateLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/IngredientLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/IngredientLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/CustomerManagerLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/CustomerManagerLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/SimpleNpcLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/SimpleNpcLogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/CustomerOrderUILogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/CustomerOrderUILogic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/Quota.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/Quota.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/Core/FoodTypes.hpp
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/Main.cpp
)

list(REMOVE_ITEM SOURCE_FILES ${GAME_FILES_IN_THESTOVE})

# Gather source files for game-specific code
file(GLOB_RECURSE GAME_SOURCE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/*.h
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/*.hpp
)

# Create a static library target for game engine/core code
add_library(TheStove STATIC ${SOURCE_FILES})

# Enable precompiled headers for faster builds (CMake 3.16+)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    if(MSVC)
        # Debug builds have been hitting stale/incompatible CMake PCH artifacts
        # during incremental builds. Keep PCH for non-Debug configs, but turn it
        # off in Debug so local iteration remains reliable.
        target_precompile_headers(TheStove PRIVATE
            "$<$<NOT:$<CONFIG:Debug>>:${CMAKE_CURRENT_LIST_DIR}/TheStove/Core/Precompiled.hpp>"
        )
    else()
        target_precompile_headers(TheStove PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/TheStove/Core/Precompiled.hpp
        )
    endif()
endif()

# Include directories for the project
target_include_directories(TheStove PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/TheStove
    ${glm_SOURCE_DIR} # glm include from FetchContent
)

target_link_libraries(TheStove PUBLIC ${ALL_LIBS})

# Set C++ standard for the library
set_property(TARGET TheStove PROPERTY CXX_STANDARD 20)

# Game-specific library layered on top of TheStove
add_library(MyoonchiDiner STATIC ${GAME_SOURCE_FILES})

target_sources(MyoonchiDiner PRIVATE ${GAME_FILES_IN_THESTOVE})
target_sources(MyoonchiDiner PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner/GameBootstrap.cpp
)

target_include_directories(MyoonchiDiner PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/MyoonchiDiner
    ${CMAKE_CURRENT_LIST_DIR}/TheStove/Core
    ${CMAKE_CURRENT_LIST_DIR}/TheStove
    ${glm_SOURCE_DIR}
)

target_link_libraries(MyoonchiDiner PUBLIC TheStove)
set_property(TARGET MyoonchiDiner PROPERTY CXX_STANDARD 20)

# Create executable target, typically with only main.cpp or launcher
add_executable(LetsEatMonsters ${CMAKE_CURRENT_LIST_DIR}/TheStove/main.cpp)

target_link_libraries(LetsEatMonsters PRIVATE MyoonchiDiner)

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
