cmake_minimum_required(VERSION 3.10)
project(gtkbrowser VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)
pkg_check_modules(JSONGLIB REQUIRED json-glib-1.0)
pkg_check_modules(CLIB REQUIRED libcurl)

# Source files
set(SOURCES
    src/main.c
    src/browser.c
    src/input.c
    src/command.c
    src/screenshot.c
    src/page.c
    src/humanize.c
    src/profile.c
    src/headless.c
    src/video.c
)

# Build
add_executable(gtkbrowser ${SOURCES})

target_include_directories(gtkbrowser PRIVATE
    ${GTK3_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    ${JSONGLIB_INCLUDE_DIRS}
    ${CLIB_INCLUDE_DIRS}
    src
)

target_link_libraries(gtkbrowser PRIVATE
    ${GTK3_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${JSONGLIB_LIBRARIES}
    ${CLIB_LIBRARIES}
    m
)

target_compile_options(gtkbrowser PRIVATE
    ${GTK3_CFLAGS_OTHER}
    ${WEBKIT_CFLAGS_OTHER}
    ${JSONGLIB_CFLAGS_OTHER}
)

# Install
install(TARGETS gtkbrowser DESTINATION bin)
