cmake_minimum_required(VERSION 3.16)
project(VulkanCanvas CXX)

find_package(ROOT REQUIRED COMPONENTS Core Gpad)
include(${ROOT_USE_FILE})

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)


set(SOURCES
    src/TVulkanCanvasImp.cxx
    src/TVulkanGuiFactory.cxx
    src/TVulkanPadPainter.cxx
    src/VulkanRenderer.cxx
)

set(HEADERS
inc/TVulkanGuiFactory.h
)

# ─── Generate ROOT Dictionary ────────────────────────────────────────────
ROOT_GENERATE_DICTIONARY(G__VulkanDict ${HEADERS}
                         LINKDEF inc/LinkDef.h
                         MODULE ${PROJECT_NAME}
                         NOINSTALL)

# ─── Shared Library Target ──────────────────────────────────────────────
add_library(${PROJECT_NAME} SHARED ${SOURCES} G__VulkanDict.cxx)


target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${ROOT_LIBRARIES}
    Vulkan::Vulkan
    glfw
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${ROOT_INCLUDE_DIRS}
)

# Compile GLSL -> SPIR-V at build time (requires glslc from the Vulkan SDK on PATH)
find_program(GLSLC glslc REQUIRED)
set(SHADER_SRC ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUT ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT})

foreach(shader canvas.vert canvas.frag)
    add_custom_command(
        OUTPUT ${SHADER_OUT}/${shader}.spv
        COMMAND ${GLSLC} ${SHADER_SRC}/${shader} -o ${SHADER_OUT}/${shader}.spv
        DEPENDS ${SHADER_SRC}/${shader}
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUT}/${shader}.spv)
endforeach()

add_custom_target(Shaders DEPENDS ${SHADER_OUTPUTS})
add_dependencies(${PROJECT_NAME} Shaders)


# ─── Copy rootlogon.C to build dir ───────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/rootlogon.C"
        "${CMAKE_CURRENT_BINARY_DIR}/rootlogon.C"
    COMMENT "Copying rootlogon.C to build directory...")

