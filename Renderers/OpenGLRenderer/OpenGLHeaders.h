// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/Debug/Debug.pkg>
// We include this here because we need IFrameBuffer.h included before the X11 headers due to "None" macro
#include <Cobalt/RendererInterface/RendererInterface.pkg>
// OpenGL is marked deprecated on macOS and triggers compilation warnings, which we suppress here.
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
// We include glad before other OpenGL headers, otherwise it complains.
WARNINGS_PUSH_OFF
#include <glad/glad.h>
WARNINGS_POP
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <GL/wglext.h>
#include <Windows.h>
#define OPENGL_USE_PLATFORM_WIN32
#elif defined(__linux__)
#ifdef OPENGL_USE_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif
// Once we do this, X11 headers with their macros like "None" are active.
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#define OPENGL_USE_PLATFORM_XLIB
#define OPENGL_USE_PLATFORM_XCB
#define OPENGL_USE_PLATFORM_WAYLAND
#else
// Once we do this, X11 headers with their macros like "None" are active.
#include <GL/glx.h>
#include <GL/glxext.h>
#define OPENGL_USE_PLATFORM_XLIB
#endif
#elif defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#define OPENGL_USE_PLATFORM_APPKIT
#endif
// This will strip out X11 macros and replace them with our constants and typedefs.
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
