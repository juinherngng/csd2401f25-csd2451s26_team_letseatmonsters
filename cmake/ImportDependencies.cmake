include(FetchContent)

# Macro to import GLFW
macro(import_glfw)
    if(NOT TARGET glfw)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        if(NOT glfw_POPULATED)
            set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(glfw)
        endif()

        include_directories(${GLFW_SOURCE_DIR}/include)
    endif()
endmacro()

# Macro to import glm
macro(import_glm)
    if(NOT TARGET glm)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(glm)

        include_directories(${glm_SOURCE_DIR})
    endif()
endmacro()

# Macro to import glad
macro(import_glad)
    if(NOT TARGET glad)
        FetchContent_Declare(
            glad
            GIT_REPOSITORY https://github.com/Dav1dde/glad.git
            GIT_TAG v2.0.8
        )
        FetchContent_MakeAvailable(glad)

         # Manually add glad as a static library as glad does not provide a CMakeLists.txt
        add_library(glad STATIC "${CMAKE_CURRENT_LIST_DIR}/extern/glad/src/glad.c")
        target_include_directories(glad PUBLIC "${CMAKE_CURRENT_LIST_DIR}/extern/glad/include")
    endif()
endmacro()

# Macro to import ImGui
macro(import_imgui)
    if(NOT TARGET imgui)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.90.4
        )
        if(NOT imgui_POPULATED)
            FetchContent_MakeAvailable(imgui)
        endif()

        # Create ImGui library manually since it doesn't have CMakeLists.txt
        set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )

        add_library(imgui STATIC ${IMGUI_SOURCES})
        target_include_directories(imgui PUBLIC 
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC glfw glad)
        
        # Set C++ standard for ImGui (required for constexpr support)
        set_property(TARGET imgui PROPERTY CXX_STANDARD 11)
        set_property(TARGET imgui PROPERTY CXX_STANDARD_REQUIRED ON)
    endif()
endmacro()

# Helper function to copy FMOD DLL to target directory
function(copy_fmod_dll_to_target target_name)
    if(WIN32 AND TARGET fmod)
        get_target_property(FMOD_DLL_PATH fmod IMPORTED_LOCATION)
        if(FMOD_DLL_PATH)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FMOD_DLL_PATH}"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying FMOD DLL to ${target_name} output directory"
            )
            message(STATUS "FMOD DLL will be copied to ${target_name} output directory")
        endif()
    endif()
endfunction()

# Macro to import fmod
macro(import_fmod)
    if(NOT TARGET fmod)  # Guard to prevent multiple inclusion
        # Set CMake policy to avoid warnings
        cmake_policy(SET CMP0111 NEW)
        
        # Set FMOD path - adjust this to your FMOD installation directory
        set(FMOD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/fmod" CACHE PATH "FMOD installation directory")
        
        # Find FMOD headers
        find_path(FMOD_INCLUDE_DIR
            NAMES fmod.h fmod.hpp
            PATHS ${FMOD_DIR}/api/core/inc
                  ${FMOD_DIR}/inc
                  ${FMOD_DIR}/include
        )
        
        # Find FMOD libraries
        if(WIN32)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(FMOD_LIB_ARCH "x64")
            else()
                set(FMOD_LIB_ARCH "x86")
            endif()
            
            # Find import library (.lib)
            find_library(FMOD_IMPORT_LIBRARY
                NAMES fmod_vc fmod
                PATHS ${FMOD_DIR}/api/core/lib/${FMOD_LIB_ARCH}
                      ${FMOD_DIR}/lib/${FMOD_LIB_ARCH}
                      ${FMOD_DIR}/lib
            )
            
            # Find DLL (.dll)
            find_file(FMOD_DLL
                NAMES fmod.dll
                PATHS ${FMOD_DIR}/api/core/lib/${FMOD_LIB_ARCH}
                      ${FMOD_DIR}/lib/${FMOD_LIB_ARCH}
                      ${FMOD_DIR}/lib
                      ${FMOD_DIR}/bin/${FMOD_LIB_ARCH}
                      ${FMOD_DIR}/bin
            )
            
            set(FMOD_LIBRARY ${FMOD_IMPORT_LIBRARY})
        elseif(UNIX AND NOT APPLE)
            find_library(FMOD_LIBRARY
                NAMES fmod
                PATHS ${FMOD_DIR}/api/core/lib/x86_64
                      ${FMOD_DIR}/lib/x86_64
                      ${FMOD_DIR}/lib
            )
        elseif(APPLE)
            find_library(FMOD_LIBRARY
                NAMES fmod
                PATHS ${FMOD_DIR}/api/core/lib
                      ${FMOD_DIR}/lib
            )
        endif()
        
        if(FMOD_INCLUDE_DIR AND FMOD_LIBRARY)
            # Create imported target
            add_library(fmod SHARED IMPORTED)
            
            if(WIN32 AND FMOD_DLL)
                # On Windows, set both the import library and DLL location
                set_target_properties(fmod PROPERTIES
                    IMPORTED_LOCATION ${FMOD_DLL}
                    IMPORTED_IMPLIB ${FMOD_IMPORT_LIBRARY}
                    INTERFACE_INCLUDE_DIRECTORIES ${FMOD_INCLUDE_DIR}
                )
                message(STATUS "FMOD found - Import lib: ${FMOD_IMPORT_LIBRARY}")
                message(STATUS "FMOD found - DLL: ${FMOD_DLL}")
            else()
                # On other platforms, just set the library location
                set_target_properties(fmod PROPERTIES
                    IMPORTED_LOCATION ${FMOD_LIBRARY}
                    INTERFACE_INCLUDE_DIRECTORIES ${FMOD_INCLUDE_DIR}
                )
                message(STATUS "FMOD found: ${FMOD_LIBRARY}")
            endif()
        else()
            message(WARNING "FMOD not found. Please install FMOD and set FMOD_DIR to the installation path.")
        endif()
    endif()
endmacro()

# Macro to import stb_image
macro(import_stb_image)
    if(NOT TARGET stb_image)  # Guard to prevent multiple inclusion
        # stb_image is header-only, so we create an interface library
        add_library(stb_image INTERFACE)
        target_include_directories(stb_image INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/extern/stb_image")
        message(STATUS "STB_IMAGE found: ${CMAKE_CURRENT_SOURCE_DIR}/extern/stb_image")
    endif()
endmacro()

# Macro to import all dependencies
macro(importDependencies)
    message(STATUS "Starting to import dependencies...")

    message(STATUS "Importing GLFW...")
    import_glfw()
    message(STATUS "GLFW imported successfully.")

    message(STATUS "Importing GLM...")
    import_glm()
    message(STATUS "GLM imported successfully.")

    message(STATUS "Importing GLAD...")
    import_glad()
    message(STATUS "GLAD imported successfully.")

    message(STATUS "Importing ImGui...")
    import_imgui()
    message(STATUS "ImGui imported successfully.")

    message(STATUS "Importing FMOD...")
    import_fmod()
    message(STATUS "FMOD imported successfully.")

    message(STATUS "Importing STB_IMAGE...")
    import_stb_image()
    message(STATUS "STB_IMAGE imported successfully.")

    message(STATUS "All dependencies have been imported successfully.")
endmacro()