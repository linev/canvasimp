cmake_minimum_required(VERSION 3.16)
project(ROOTVulkanCanvas CXX)

# Enforce modern C++ requirements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Find CERN ROOT ──────────────────────────────────────────────────────
find_package(ROOT REQUIRED COMPONENTS Core RIO Hist Gpad Graf)
include(${ROOT_USE_FILE})

# ─── Find Vulkan SDK ─────────────────────────────────────────────────────
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Install vulkan-sdk.")
endif()

# ─── Find SDL2 for window management (WSI) ───────────────────────────────
find_package(SDL2 REQUIRED)
if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found. Install libsdl2-dev.")
endif()

# ─── Find Freetype + HarfBuzz for text rendering ─────────────────────────
find_package(Freetype QUIET)
find_package(Harfbuzz QUIET)

# ─── Source & Header files ───────────────────────────────────────────────
set(SOURCES
    src/TVulkanCanvas.cxx
    src/TVulkanGuiFactory.cxx
    src/TVulkanPadPainter.cxx
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

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${ROOT_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES}
)
if(Freetype_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Freetype::Freetype)
endif()
if(Harfbuzz_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE harfbuzz)
endif()

# ROOT libraries — PUBLIC so downstream apps inherit them
target_link_libraries(${PROJECT_NAME} PUBLIC ${ROOT_LIBRARIES})

# ─── Copy rootlogon.C to build dir ───────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/rootlogon.C"
        "${CMAKE_CURRENT_BINARY_DIR}/rootlogon.C"
    COMMENT "Copying rootlogon.C to build directory...")
