cmake_minimum_required(VERSION 3.29)

project(VulkanRenderEngine VERSION 1.0.0 LANGUAGES CXX C)

# Option to enable/disable Vulkan C++20 module support for this standalone project
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan in SimpleEngine" OFF)

# Enable C++ module dependency scanning only when modules are enabled
if(ENABLE_CPP20_MODULE)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Dependencies
#   Vulkan (headers, loader, vulkan.hpp)     -> Vulkan SDK (VULKAN_SDK environment variable)
#   glfw3, glm, ktx, tinygltf, nlohmann-json -> vcpkg manifest (vcpkg.json)
# Every package is found through the CMake config files that vcpkg installs;
# tinygltf is a single header without a config, so it is located by header name.
# ─────────────────────────────────────────────────────────────────────────────
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIR tiny_gltf.h REQUIRED)

if(ENABLE_CPP20_MODULE)
    # Set up Vulkan C++ module for this standalone project
    add_library(VulkanCppModule)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule
            PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
    target_link_libraries(VulkanCppModule
            PUBLIC
            Vulkan::Vulkan
    )

    set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

    target_sources(VulkanCppModule
            PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES
            BASE_DIRS
            "${Vulkan_INCLUDE_DIR}"
            FILES
            "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )

    # MSVC-specific options to improve module support
    if(MSVC)
        target_compile_options(VulkanCppModule PRIVATE
            /std:c++latest
            /permissive-
            /Zc:__cplusplus
            /EHsc
            /Zc:preprocessor
        )
    endif()
else()
    add_library(VulkanCppModule INTERFACE)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)
    target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
    target_compile_definitions(VulkanCppModule
            INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
endif()

# Desktop is the only platform; the define selects the GLFW instance extensions in renderer_core.cpp
add_definitions(-DPLATFORM_DESKTOP)

# Shader compilation
# Find Slang shaders (exclude utility modules that are imported, not compiled standalone)
file(GLOB SLANG_SHADER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.slang)
list(FILTER SLANG_SHADER_SOURCES EXCLUDE REGEX ".*/(common_types|pbr_utils|lighting_utils|tonemapping_utils)\\.slang$")

# Find slangc executable (optional)
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin)
find_program(SPIRV_OPT_EXECUTABLE spirv-opt HINTS $ENV{VULKAN_SDK}/bin)

if(SLANGC_EXECUTABLE)
    # Ensure the output directory for compiled shaders exists
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

    # Compile Slang shaders using slangc
    foreach(SHADER ${SLANG_SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        get_filename_component(SHADER_NAME_WE ${SHADER_NAME} NAME_WE)
        string(REGEX REPLACE "\.slang$" "" OUTPUT_NAME ${SHADER_NAME})
        set(OUTPUT_SPV ${CMAKE_CURRENT_BINARY_DIR}/shaders/${OUTPUT_NAME}.spv)

        set(SLANGC_FLAGS -target spirv -profile spirv_1_3 -g0 -O3)

        if (SPIRV_OPT_EXECUTABLE)
            add_custom_command(
                    OUTPUT ${OUTPUT_SPV}
                    COMMAND ${SLANGC_EXECUTABLE} ${SHADER} ${SLANGC_FLAGS} -o ${OUTPUT_SPV}.tmp
                    COMMAND ${SPIRV_OPT_EXECUTABLE} --strip-debug --strip-nonsemantic -Os ${OUTPUT_SPV}.tmp -o ${OUTPUT_SPV}
                    COMMAND ${CMAKE_COMMAND} -E remove ${OUTPUT_SPV}.tmp
                    DEPENDS ${SHADER}
                    COMMENT "Compiling and optimizing Slang shader ${SHADER_NAME}"
            )
        else ()
            add_custom_command(
                    OUTPUT ${OUTPUT_SPV}
                    COMMAND ${SLANGC_EXECUTABLE} ${SHADER} ${SLANGC_FLAGS} -o ${OUTPUT_SPV}
                    DEPENDS ${SHADER}
                    COMMENT "Compiling Slang shader ${SHADER_NAME} with slangc"
            )
        endif ()
        list(APPEND SHADER_SPVS ${OUTPUT_SPV})
    endforeach()

    add_custom_target(shaders DEPENDS ${SHADER_SPVS})

    # Copy shaders to the source shaders folder so the app can find them when run from src/
    add_custom_command(TARGET shaders POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_SPVS} ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/
            COMMENT "Copying compiled shaders to source shaders directory"
    )
else()
    message(STATUS "slangc not found. Skipping shader compilation step.")
    add_custom_target(shaders)
endif()

# Source files
set(SOURCES_COMMON
    src/engine.cpp
    src/scene_loading.cpp
    src/sandbox_impl.cpp
    src/terminal_commands.cpp
    src/platform.cpp
    src/renderer_core.cpp
    src/renderer_rendering.cpp
    src/renderer_pipelines.cpp
    src/renderer_utils.cpp
    src/renderer_resources.cpp
    src/memory_pool.cpp
    src/resource_manager.cpp
    src/entity.cpp
    src/component.cpp
    src/transform_component.cpp
    src/mesh_component.cpp
    src/camera_component.cpp
    src/light_component.cpp
    src/model_loader.cpp
    src/imgui_system.cpp
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/vulkan_device.cpp
    src/descriptor_manager.cpp
    src/renderdoc_debug_system.cpp
    src/mikktspace.c
)

set(SOURCES_DESKTOP
        src/sandbox.cpp
)

# Create target
# Desktop: build the runnable executable.
add_executable(VulkanRenderEngine ${SOURCES_COMMON} ${SOURCES_DESKTOP})

add_dependencies(VulkanRenderEngine shaders)
set_target_properties (VulkanRenderEngine PROPERTIES CXX_STANDARD 20)

# Enable required defines for GLM experimental extensions and MSVC math constants
target_compile_definitions(VulkanRenderEngine PRIVATE
    GLM_ENABLE_EXPERIMENTAL
    _USE_MATH_DEFINES
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
    # tinygltf: images are decoded by the renderer with libktx, not by tinygltf/stb_image
    TINYGLTF_NO_EXTERNAL_IMAGE
    TINYGLTF_NO_STB_IMAGE
    TINYGLTF_NO_STB_IMAGE_WRITE
)

# The tinygltf implementation is compiled into exactly one translation unit
set_source_files_properties(src/model_loader.cpp PROPERTIES COMPILE_DEFINITIONS TINYGLTF_IMPLEMENTATION)

target_include_directories(VulkanRenderEngine PRIVATE ${TINYGLTF_INCLUDE_DIR})

# Link libraries
# Prefer the Vulkan C++ module target when available (configured above),
# but fall back to the standard Vulkan library otherwise.
if(TARGET Vulkan::cppm)
    target_link_libraries(VulkanRenderEngine PUBLIC Vulkan::cppm)
else()
    target_link_libraries(VulkanRenderEngine PUBLIC Vulkan::Vulkan)
endif()

target_link_libraries(VulkanRenderEngine PUBLIC
    glm::glm
    nlohmann_json::nlohmann_json
    KTX::ktx
)

target_link_libraries(VulkanRenderEngine PRIVATE glfw)

# Windows/MSVC portability and build settings
if(MSVC)
    # Avoid Windows.h macro pollution and CRT warnings; improve conformance and build perf
    target_compile_definitions(VulkanRenderEngine PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
    target_compile_options(VulkanRenderEngine PRIVATE
        /permissive-
        /Zc:__cplusplus
        /EHsc
        /W3
        /MP
        /bigobj
    )
    # Crash reporter uses Dbghelp; pragma should suffice, but make it explicit for clarity
    target_link_libraries(VulkanRenderEngine PRIVATE Dbghelp)
    # Visual Studio: start the engine from the debugger with src/ as the working directory.
    # The compiled shaders are copied there (shaders/<name>.spv) and the scene path in
    # main.cpp (../Assets/bistro/bistro.gltf) resolves to the assets folder of the repository.
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT VulkanRenderEngine)
    set_target_properties(VulkanRenderEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src")
elseif(WIN32)
    # Non-MSVC Windows toolchains (e.g., MinGW)
    target_compile_definitions(VulkanRenderEngine PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Copy model and texture files if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/models)
    add_custom_command(TARGET VulkanRenderEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/models ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/models
        COMMENT "Copying models to output directory"
    )
endif ()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/textures)
    add_custom_command(TARGET VulkanRenderEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/textures ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/textures
        COMMENT "Copying textures to output directory"
    )
endif ()

# Add packaging configuration
include(CPack)

# Set package properties
set(CPACK_PACKAGE_NAME "VulkanRenderEngine")
set(CPACK_PACKAGE_VENDOR "SimpleEngine Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple game engine built with Vulkan")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "VulkanRenderEngine")

# Set platform-specific package generators
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "VulkanRenderEngine")
    set(CPACK_NSIS_DISPLAY_NAME "VulkanRenderEngine")
    set(CPACK_NSIS_HELP_LINK "https://github.com/yourusername/SimpleEngine")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/yourusername/SimpleEngine")
    set(CPACK_NSIS_CONTACT "your.email@example.com")
    set(CPACK_NSIS_MODIFY_PATH ON)
endif()

# Include binary and resource directories in the package
install(TARGETS VulkanRenderEngine DESTINATION bin)
if(SLANGC_EXECUTABLE)
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders DESTINATION share/SimpleEngine)
endif()

# Install models and textures if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/models)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/models DESTINATION share/SimpleEngine)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/textures)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/textures DESTINATION share/SimpleEngine)
endif()

# Install README if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md DESTINATION share/SimpleEngine)
endif()
