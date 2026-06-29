// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <cmath>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
SDL_Window* CobaltSDLCreateWindow(const char* title, int windowWidthInPixels, int windowHeightInPixels, SDL_WindowFlags flags)
{
	// Adjust the supplied flags to add mandatory options, and strip forbidden ones. Note that we force high DPI support
	// always, and strip out the 3D API support flags. The 3D API support flags initialize internal SDL loading/helpers
	// for the specified graphics API. We don't want this, as the Cobalt renderer initializes everything itself
	// internally, with better understanding of what's required. SDL just sits "above" the Cobalt renderer here,
	// assisting with window creation and message handling.
	flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
	flags &= ~(SDL_WINDOW_OPENGL | SDL_WINDOW_VULKAN | SDL_WINDOW_METAL);

	// Build the set of window properties to pass to SDL for window creation. Note that we need to go with this lower
	// level API in order to pass SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN into the window creation
	// process. Without this, SDL will still initialize a 3D graphics API internally, it will just choose a platform
	// default automatically. We need to use the SDL_CreateWindowWithProperties API in order to prevent this.
	SDL_PropertiesID windowProperties = SDL_CreateProperties();
	if (windowProperties == 0)
	{
		return nullptr;
	}
	SDL_SetStringProperty(windowProperties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
	SDL_SetNumberProperty(windowProperties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, windowWidthInPixels);
	SDL_SetNumberProperty(windowProperties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, windowHeightInPixels);
	SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);
	SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
	if ((flags & SDL_WINDOW_FULLSCREEN) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_BORDERLESS) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_RESIZABLE) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_UTILITY) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_UTILITY_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_TOOLTIP) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_TOOLTIP_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_MINIMIZED) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_MINIMIZED_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_MAXIMIZED) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_MOUSE_GRABBED) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_MOUSE_GRABBED_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_HIGH_PIXEL_DENSITY) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_ALWAYS_ON_TOP) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_ALWAYS_ON_TOP_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_TRANSPARENT) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_TRANSPARENT_BOOLEAN, true);
	}
	if ((flags & SDL_WINDOW_MODAL) != 0)
	{
		SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_MODAL_BOOLEAN, true);
	}

	// Create the window object
	SDL_Window* window = SDL_CreateWindowWithProperties(windowProperties);
	SDL_DestroyProperties(windowProperties);
	if (window == nullptr)
	{
		return nullptr;
	}

	// If the window has DPI scaling, resize it now to match the requested size in pixels, as it will have been created
	// at the scaled size. Note that we can't use SDL_GetWindowDisplayScale() here, as while that will correctly report
	// the DPI scaling factor for the display the window is attached to, but under SDL pixel density and DPI can be
	// deatched from each other, and are in the case of X11 currently on a high DPI display, which returns 1, while
	// Wayland returns 2.
	float pixelDensity = SDL_GetWindowPixelDensity(window);
	if (pixelDensity != 1)
	{
		int mainWindowWidthInUnits = (int)std::round((float)windowWidthInPixels / pixelDensity);
		int mainWindowHeightInUnits = (int)std::round((float)windowHeightInPixels / pixelDensity);
		SDL_SetWindowSize(window, mainWindowWidthInUnits, mainWindowHeightInUnits);
	}

	// If the window was requested to be shown by default, now that we've adjusted the window size to account for DPI
	// scaling, make it visible.
	if ((flags & SDL_WINDOW_HIDDEN) == 0)
	{
		SDL_ShowWindow(window);
	}

	// Return the created window to the caller
	return window;
}

}} // namespace cobalt::graphics
