cmake_minimum_required(VERSION 3.15)

# Project name and language - enable both C and CXX
project(application_template_c LANGUAGES C CXX)

# Use C11 by default
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Use C++11 for ImGui
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set shared library prefix for MinGW on Windows
if(WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# Set installation prefix to build directory
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})

# Allow switching between host builds and cross-compilation
if(DEFINED CROSS_COMPILE AND CROSS_COMPILE)
    message(STATUS "Cross-compiling using toolchain file")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi-toolchain.cmake")
endif()

# Detect platform and architecture
string(TOLOWER "${CMAKE_SYSTEM_NAME}" PLATFORM_NAME)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" ARCH_NAME)

if(ARCH_NAME STREQUAL "" OR ARCH_NAME STREQUAL "unknown")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_NAME "x86_64")
    else()
        set(ARCH_NAME "x86")
    endif()
endif()

# Create a config+platform+arch string
set(CONFIG_PLATFORM_DIR "${CMAKE_BUILD_TYPE}-${PLATFORM_NAME}-${ARCH_NAME}")

# Set custom output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CONFIG_PLATFORM_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${CONFIG_PLATFORM_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${CONFIG_PLATFORM_DIR}")

# ------------------------------------------------------------------------------
# utils
# ------------------------------------------------------------------------------

# Find OpenGL package
find_package(OpenGL REQUIRED)

# ------------------------------------------------------------------------------
# Vendor project: GLFW (as submodule)
# ------------------------------------------------------------------------------
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(vendor/glfw)

# ------------------------------------------------------------------------------
# Vendor project: cimgui
# ------------------------------------------------------------------------------

# Check backend folder location
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cimgui/imgui/backends)
    set(BACKENDS_DIR "vendor/cimgui/imgui/backends")
else()
    set(BACKENDS_DIR "vendor/cimgui/imgui/examples")
endif()

# Check if newer ImGui version with tables exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cimgui/imgui/imgui_tables.cpp)
    set(TABLES_SOURCE "vendor/cimgui/imgui/imgui_tables.cpp")
else()
    set(TABLES_SOURCE "")
endif()

# Set IMGUI source files
set(IMGUI_SOURCES 
    vendor/cimgui/cimgui.cpp 
    vendor/cimgui/imgui/imgui.cpp 
    vendor/cimgui/imgui/imgui_draw.cpp 
    vendor/cimgui/imgui/imgui_demo.cpp 
    vendor/cimgui/imgui/imgui_widgets.cpp
    ${TABLES_SOURCE}
)

# Add backend implementations
list(APPEND IMGUI_SOURCES 
    ${BACKENDS_DIR}/imgui_impl_opengl3.cpp
    ${BACKENDS_DIR}/imgui_impl_glfw.cpp
)

# Create cimgui library
add_library(cimgui STATIC ${IMGUI_SOURCES})

# Set IMGUI definitions
target_compile_definitions(cimgui PUBLIC
    -DIMGUI_USER_CONFIG=\"${CMAKE_CURRENT_SOURCE_DIR}/vendor/cimgui/cimconfig.h\"
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
    -DIMGUI_IMPL_OPENGL_LOADER_GL3W
    -DIMGUI_DISABLE_OBSOLETE_KEYIO=1
    -DIMGUI_DISABLE_OBSOLETE_DRAWALL=1
)

# Check if we're using a newer version that requires different handling
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cimgui/imgui/imgui.h)
    # Read the version from imgui.h to detect API level
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cimgui/imgui/imgui.h IMGUI_H_CONTENT)
    if(IMGUI_H_CONTENT MATCHES "#define IMGUI_VERSION \"([0-9]+\\.[0-9]+)\"")
        set(IMGUI_VERSION ${CMAKE_MATCH_1})
        message(STATUS "Detected ImGui version: ${IMGUI_VERSION}")
        
        # For newer versions, you might need additional defines
        if(IMGUI_VERSION VERSION_GREATER_EQUAL "1.89")
            target_compile_definitions(cimgui PUBLIC -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)
        endif()
    endif()
endif()

# For Windows: Remove the outer quotes and ensure proper escaping
if(WIN32)
    target_compile_definitions(cimgui PUBLIC -DIMGUI_IMPL_API=extern\ \"C\"\ __declspec\(dllexport\))
else()
    target_compile_definitions(cimgui PUBLIC -DIMGUI_IMPL_API=extern\ \"C\")
endif()

# Set include directories for cimgui
target_include_directories(cimgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cimgui
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cimgui/imgui
    ${BACKENDS_DIR}
)

# Link dependencies
target_link_libraries(cimgui 
    OpenGL::GL
    glfw
)

# ------------------------------------------------------------------------------
# Main project sources
# ------------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES "src/*.c")

add_executable(${PROJECT_NAME} ${SOURCES})

# Enable warnings only for our main project
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Set important definitions for the executable
target_compile_definitions(${PROJECT_NAME} PRIVATE
    -DCIMGUI_DEFINE_ENUMS_AND_STRUCTS
    -DCIMGUI_USE_OPENGL3
    -DCIMGUI_USE_GLFW
    $<$<CONFIG:Debug>:-DDEBUG>
)

# Set include directories for executable
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cimgui
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cimgui/imgui
    ${BACKENDS_DIR}
)

# ------------------------------------------------------------------------------
# Copy files to build directory
# ------------------------------------------------------------------------------
# Create a custom target to copy config files
add_custom_target(copy_config ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config
    COMMENT "Copying config files to build directory"
)
add_dependencies(${PROJECT_NAME} copy_config)           # Make the main target depend on the copy_config target

# Create a custom target to copy config files
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
    COMMENT "Copying assets files to build directory"
)
add_dependencies(${PROJECT_NAME} copy_assets)           # Make the main target depend on the copy_assets target

# ------------------------------------------------------------------------------
# Link with GLFW, cimgui and system libs
# ------------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE cimgui glfw OpenGL::GL)

# On Linux we need extra system libs (X11, pthread, etc.)
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 pthread dl)
endif()

# ------------------------------------------------------------------------------
# Print helpful info
# ------------------------------------------------------------------------------
message(STATUS "Configuration: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform:      ${PLATFORM_NAME}")
message(STATUS "Architecture:  ${ARCH_NAME}")
message(STATUS "Output dir:    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
