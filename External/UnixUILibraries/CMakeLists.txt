# X11
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)

# XCB
pkg_check_modules(X11_XCB REQUIRED x11-xcb)
add_library(X11Xcb INTERFACE)
target_include_directories(X11Xcb SYSTEM INTERFACE ${X11_XCB_INCLUDE_DIRS})
target_link_libraries(X11Xcb INTERFACE ${X11_XCB_LIBRARIES})
target_compile_options(X11Xcb INTERFACE ${X11_XCB_CFLAGS_OTHER})

# Wayland
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
add_library(WaylandClient INTERFACE)
target_include_directories(WaylandClient SYSTEM INTERFACE ${WAYLAND_CLIENT_INCLUDE_DIRS})
target_link_libraries(WaylandClient INTERFACE ${WAYLAND_CLIENT_LIBRARIES})
target_compile_options(WaylandClient INTERFACE ${WAYLAND_CLIENT_CFLAGS_OTHER})
find_program(WAYLAND_SCANNER NAMES wayland-scanner REQUIRED)
pkg_get_variable(WAYLAND_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)
set(XDG_SHELL_XML           "${WAYLAND_PROTOCOLS_DATADIR}/stable/xdg-shell/xdg-shell.xml")
set(XDG_SHELL_CLIENT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
set(XDG_SHELL_CLIENT_CODE   "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.c")
add_custom_command(
    OUTPUT
        "${XDG_SHELL_CLIENT_HEADER}"
        "${XDG_SHELL_CLIENT_CODE}"
    COMMAND "${WAYLAND_SCANNER}" client-header
            "${XDG_SHELL_XML}"
            "${XDG_SHELL_CLIENT_HEADER}"
    COMMAND "${WAYLAND_SCANNER}" private-code
            "${XDG_SHELL_XML}"
            "${XDG_SHELL_CLIENT_CODE}"
    DEPENDS "${XDG_SHELL_XML}"
    VERBATIM
)
add_library(xdg-shell-protocol STATIC "${XDG_SHELL_CLIENT_CODE}")
target_include_directories(xdg-shell-protocol SYSTEM PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(xdg-shell-protocol PUBLIC WaylandClient)

# Cairo
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)
add_library(CAIRO::cairo INTERFACE IMPORTED)
set_target_properties(CAIRO::cairo PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CAIRO_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES      "${CAIRO_LINK_LIBRARIES}"
    INTERFACE_COMPILE_OPTIONS     "${CAIRO_CFLAGS_OTHER}"
)
