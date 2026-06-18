cmake_minimum_required(VERSION 3.10)
project(axonbrowser VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pass version to C code
add_definitions(-DAXONBROWSER_VERSION="${PROJECT_VERSION}")

# Find required packages
find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)
pkg_check_modules(JSONGLIB REQUIRED json-glib-1.0)
pkg_check_modules(CLIB REQUIRED libcurl)

# Source files
set(SOURCES
    # Core
    src/main.c
    src/browser.c
    src/command.c

    # Commands (command handlers)
    src/commands/core.c
    src/commands/navigation.c
    src/commands/input.c
    src/commands/elements.c
    src/commands/screenshot.c
    src/commands/tabs.c
    src/commands/storage.c
    src/commands/media.c
    src/commands/monitoring.c
    src/commands/extensions.c
    src/commands/window.c

    # Feature implementations
    src/page.c
    src/input.c
    src/humanize.c
    src/screenshot.c
    src/video.c
    src/profile.c
    src/extensions.c
    src/headless.c

    # Sub-module implementations
    src/navigation/navigation.c
    src/elements/elements.c
    src/monitor/monitoring.c
    src/media/media.c
)

# Build
add_executable(axonbrowser ${SOURCES})

target_include_directories(axonbrowser PRIVATE
    ${GTK3_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    ${JSONGLIB_INCLUDE_DIRS}
    ${CLIB_INCLUDE_DIRS}
    src
)

target_link_libraries(axonbrowser PRIVATE
    ${GTK3_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${JSONGLIB_LIBRARIES}
    ${CLIB_LIBRARIES}
    m
)

target_compile_options(axonbrowser PRIVATE
    ${GTK3_CFLAGS_OTHER}
    ${WEBKIT_CFLAGS_OTHER}
    ${JSONGLIB_CFLAGS_OTHER}
)

# Install
install(TARGETS axonbrowser DESTINATION bin)
