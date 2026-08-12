cmake_minimum_required(VERSION 3.15)
project(GtkmmMainMenuExample LANGUAGES CXX)

# gtkmm-4.0 requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PkgConfig utility
find_package(PkgConfig REQUIRED)

# Detect and configure gtkmm 4.0 package using modern IMPORTED targets
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

# Declare all the source files belonging to the main_menu example
set(SOURCE_FILES
    main.cc
    examplewindow.h
    examplewindow.cc
)

# Define our executable
add_executable(gtkmm_menu_demo ${SOURCE_FILES})

# Link to the imported target (this handles compile flags, includes, and libs cleanly)
target_link_libraries(gtkmm_menu_demo PRIVATE PkgConfig::GTKMM)
