cmake_minimum_required(VERSION 3.16)
project(ROOTQt6Canvas CXX)

find_package(ROOT REQUIRED COMPONENTS Core Gpad)
include(${ROOT_USE_FILE})

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

find_package(Freetype REQUIRED)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


# 4. Declare your library source files
set(SOURCES
    src/QCanvasWidget.cpp
    src/QPaintWidget.cpp
    src/QRootMethodDialog.cpp
    src/QRootContextMenu.cpp
    src/TQt6Application.cxx
    src/TQt6Canvas.cxx
    src/TQt6PadPainter.cxx
    src/TQt6GuiFactory.cxx
)

set(HEADERS
    inc/TQt6Application.h
    inc/TQt6Canvas.h
    inc/TQt6PadPainter.h
    inc/TQt6GuiFactory.h
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${ROOT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Freetype::Freetype)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/rootlogon.C"
    "${CMAKE_CURRENT_BINARY_DIR}/rootlogon.C"
    COMMENT "Copying rootlogon.C to build directory..."
)

