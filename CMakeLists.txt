cmake_minimum_required(VERSION 3.16)
project(ROOTRaylibCanvas CXX)

# 2. Locate CERN ROOT configuration files
find_package(ROOT REQUIRED COMPONENTS Core RIO Hist Gpad Graf)
include(${ROOT_USE_FILE})


# 1. Enforce modern C++ requirements
# set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 3. Locate raylib package configuration files
find_package(raylib QUIET)
if(NOT raylib_FOUND)
    find_path(RAYLIB_INCLUDE_DIR NAMES raylib.h)
    find_library(RAYLIB_LIBRARY NAMES raylib)
    if(NOT RAYLIB_INCLUDE_DIR OR NOT RAYLIB_LIBRARY)
        message(FATAL_ERROR "raylib library or development headers missing.")
    endif()
endif()

# 4. Declare your library source files
set(SOURCES
    src/TRaylibCanvas.cxx
    src/TRaylibGuiFactory.cxx
    src/TRaylibPadPainter.cxx
)

set(HEADERS
    inc/TRaylibCanvas.h
    inc/TRaylibGuiFactory.h
    inc/TRaylibPadPainter.h
)

# 5. Generate ROOT Dictionary
# This will output 'G__MyDict.cxx' which must be compiled into the library.
ROOT_GENERATE_DICTIONARY(G__MyDict ${HEADERS}
                         LINKDEF inc/LinkDef.h
                         MODULE ${PROJECT_NAME}
                         NOINSTALL)

# 6. Define the Library Build Target
# Options: STATIC (creates .a file) or SHARED (creates .so file on Linux)
add_library(${PROJECT_NAME} SHARED ${SOURCES} G__MyDict.cxx)

# 7. Configure Include Directories Interface
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${ROOT_INCLUDE_DIRS}
)
if(NOT raylib_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${RAYLIB_INCLUDE_DIR})
endif()

# 8. Link Dependencies to the Library Target
if(raylib_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIBRARY})
endif()

# ROOT libraries are linked as PUBLIC so that any application linking against
# your library will also inherit the necessary ROOT library mappings automatically.
target_link_libraries(${PROJECT_NAME} PUBLIC ${ROOT_LIBRARIES})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/rootlogon.C"
    "${CMAKE_CURRENT_BINARY_DIR}/rootlogon.C"
    COMMENT "Copying rootlogon.C to build directory..."
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/.rootrc"
    "${CMAKE_CURRENT_BINARY_DIR}/.rootrc"
    COMMENT "Copying .rootrc to build directory..."
)