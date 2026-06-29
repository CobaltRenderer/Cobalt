// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_SDL3_SUPPORT
#include <SDL3/SDL.h>
namespace cobalt { namespace graphics {

inline SDL_Window* CobaltSDLCreateWindow(const char* title, int windowWidthInPixels, int windowHeightInPixels, SDL_WindowFlags flags = 0);

}} // namespace cobalt::graphics
#include "PlatformHelpersSDL3.inl"
#endif
