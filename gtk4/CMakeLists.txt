cmake_minimum_required(VERSION 3.16)
project(ROOTGtk4Canvas CXX)

find_package(ROOT REQUIRED COMPONENTS Core Gpad)
include(${ROOT_USE_FILE})

set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

# 4. Declare your library source files
set(SOURCES
    src/Gtk4DrawArea.cxx
    src/Gtk4CanvasWindow.cxx
    src/Gtk4MethodDialog.cxx
    src/TGtk4ContextMenu.cxx
    src/TGtk4Canvas.cxx
    src/TGtk4GuiFactory.cxx
    src/TGtk4PadPainter.cxx
)

set(HEADERS
    inc/TGtk4Canvas.h
    inc/TGtk4GuiFactory.h
    inc/TGtk4PadPainter.h
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

target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::GTKMM)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/rootlogon.C"
    "${CMAKE_CURRENT_BINARY_DIR}/rootlogon.C"
    COMMENT "Copying rootlogon.C to build directory..."
)

