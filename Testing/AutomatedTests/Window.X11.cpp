// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
//##TODO## Clean all this up
#include "Window.h"
#include "MenuItem.h"
#include <Cobalt/Debug/Debug.pkg>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
//##TODO## Eliminate this and make everything class members
struct PopupMenu;
struct X11WindowImpl
{
	Display* display = nullptr;

	// Top-level X11 window that contains both the menu bar and the client area
	::Window window = 0;

	// Child window that represents the drawable client area below the menu bar
	::Window clientWindow = 0;

	Atom wmDeleteAtom = 0;
	Atom invokeAtom = 0;

	GC gc = 0;
	XFontStruct* font = nullptr;

	// Height of the menu bar at the top of the main window
	int menuBarHeight = 24;

	// Hover state for the top-level menu bar (visual index among non-separator items)
	int hotMenuBarItem = -1;

	// Colors used for Windows-style menu rendering
	unsigned long menuBg = 0;
	unsigned long menuText = 0;
	unsigned long menuBorder = 0;
	unsigned long menuHighlightBg = 0;

	// Active popup menus
	std::vector<PopupMenu> popups;
};

//----------------------------------------------------------------------------------------
struct PopupMenu
{
	::Window win = 0;
	MenuItem* node = nullptr; // children drawn vertically
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	int itemHeight = 0;

	// Index of the currently "hot" (hovered) item in this popup, -1 if none.
	int hotIndex = -1;
};

//----------------------------------------------------------------------------------------
// Menu drawing functions
//----------------------------------------------------------------------------------------
static unsigned long AllocColorOrDefault(Display* dpy, int screen, const char* name, unsigned long fallback)
{
	Colormap cmap = DefaultColormap(dpy, screen);
	XColor exact{};
	XColor screenDef{};
	if (XAllocNamedColor(dpy, cmap, name, &screenDef, &exact))
	{
		return screenDef.pixel;
	}
	return fallback;
}

//----------------------------------------------------------------------------------------
static int MeasureTextWidth(X11WindowImpl* impl, const std::string& text)
{
	if (!impl->font)
	{
		// Reasonable fixed-width fallback.
		return 8 * (int)text.size();
	}
	return XTextWidth(impl->font, text.c_str(), (int)text.size());
}

//----------------------------------------------------------------------------------------
static void DrawX11MenuBar(X11WindowImpl* impl, MenuItem* menuRoot, int windowWidth)
{
	if (!impl || !impl->display || !impl->window)
	{
		return;
	}

	Display* dpy = impl->display;
	::Window win = impl->window;
	GC gc = impl->gc;

	// Background (Windows-style grey).
	XSetForeground(dpy, gc, impl->menuBg);
	XFillRectangle(dpy, win, gc, 0, 0, (unsigned int)windowWidth, (unsigned int)impl->menuBarHeight);

	// Bottom border line.
	XSetForeground(dpy, gc, impl->menuBorder);
	XDrawLine(dpy, win, gc, 0, impl->menuBarHeight - 1, windowWidth, impl->menuBarHeight - 1);

	if (!menuRoot)
	{
		return;
	}

	int x = 4;
	int y = impl->menuBarHeight - 4;
	int visualIndex = 0; // counts only non-separator items

	for (const MenuItem& item : menuRoot->GetChildren())
	{
		if (item.isSeparator)
		{
			continue;
		}

		const std::string& label = item.caption;
		int textWidth = MeasureTextWidth(impl, label);
		int itemWidth = textWidth + 16;

		// Hover highlight for current menubar item.
		if (visualIndex == impl->hotMenuBarItem)
		{
			XSetForeground(dpy, gc, impl->menuHighlightBg);
			XFillRectangle(dpy, win, gc, x, 1, (unsigned int)itemWidth, (unsigned int)(impl->menuBarHeight - 2));
		}

		XSetForeground(dpy, gc, impl->menuText);
		XDrawString(dpy, win, gc, x + 8, y, label.c_str(), (int)label.size());

		x += itemWidth;
		++visualIndex;
	}
}

//----------------------------------------------------------------------------------------
static void DrawPopupMenu(X11WindowImpl* impl, const PopupMenu& popup)
{
	if (!impl || !impl->display || !impl->font || !popup.node)
	{
		return;
	}

	Display* dpy = impl->display;
	::Window win = popup.win;
	GC gc = impl->gc;

	// Fill background with menu grey.
	XSetForeground(dpy, gc, impl->menuBg);
	XFillRectangle(dpy, win, gc, 0, 0, (unsigned int)popup.width, (unsigned int)popup.height);

	// Border.
	XSetForeground(dpy, gc, impl->menuBorder);
	XDrawRectangle(dpy, win, gc, 0, 0, popup.width - 1, popup.height - 1);

	const auto& children = popup.node->GetChildren();
	int index = 0;

	const int paddingLeft = 4;
	const int checkAreaWidth = 16;
	const int gapAfterCheck = 4;
	const int paddingRight = 4;

	for (const MenuItem& item : children)
	{
		int yTop = index * popup.itemHeight;
		int yMid = yTop + popup.itemHeight / 2;
		int textY = yTop + popup.itemHeight - 4;

		if (item.isSeparator)
		{
			// Draw separator line.
			XSetForeground(dpy, gc, impl->menuBorder);
			XDrawLine(dpy, win, gc, paddingLeft, yMid, popup.width - paddingRight, yMid);
		}
		else
		{
			bool isHot = (index == popup.hotIndex);

			// Hover background rectangle.
			if (isHot)
			{
				XSetForeground(dpy, gc, impl->menuHighlightBg);
				XFillRectangle(dpy, win, gc, 1, yTop + 1, (unsigned int)(popup.width - 2), (unsigned int)(popup.itemHeight - 2));
			}

			// Text and glyphs.
			XSetForeground(dpy, gc, impl->menuText);

			// Checkmark area.
			int checkLeft = paddingLeft;
			int checkRight = checkLeft + checkAreaWidth - 4;

			if (item.isTickable && item.isTicked)
			{
				int cy = yMid;
				// Simple check mark.
				XDrawLine(dpy, win, gc, checkLeft + 2, cy, checkLeft + 5, cy + 3);
				XDrawLine(dpy, win, gc, checkLeft + 5, cy + 3, checkRight, cy - 3);
			}

			// Label text.
			int textX = paddingLeft + checkAreaWidth + gapAfterCheck;
			const std::string& label = item.caption;

			XDrawString(dpy, win, gc, textX, textY, label.c_str(), (int)label.size());

			// Submenu arrow for items with children.
			if (!item.GetChildren().empty())
			{
				int arrowX = popup.width - paddingRight - 6;
				int arrowY = yMid;

				XDrawLine(dpy, win, gc, arrowX - 3, arrowY - 4, arrowX + 1, arrowY);
				XDrawLine(dpy, win, gc, arrowX + 1, arrowY, arrowX - 3, arrowY + 4);
			}
		}

		++index;
	}
}

//----------------------------------------------------------------------------------------
static PopupMenu CreatePopupForNode(X11WindowImpl* impl, MenuItem* node, int x, int y)
{
	PopupMenu popup;
	popup.node = node;
	popup.itemHeight = impl->menuBarHeight; // reuse menu bar height for rows
	popup.x = x;
	popup.y = y;

	// Determine width/height from children.
	int maxTextWidth = 0;
	int count = 0;

	for (const MenuItem& child : node->GetChildren())
	{
		++count;
		if (child.isSeparator)
		{
			continue;
		}
		maxTextWidth = std::max(maxTextWidth, MeasureTextWidth(impl, child.caption));
	}

	if (count == 0)
	{
		return popup;
	}

	const int paddingLeft = 4;
	const int checkAreaWidth = 16;
	const int gapAfterCheck = 4;
	const int arrowAreaWidth = 12;
	const int paddingRight = 4;

	popup.width = paddingLeft + checkAreaWidth + gapAfterCheck + maxTextWidth + arrowAreaWidth + paddingRight;
	popup.height = count * popup.itemHeight;

	Display* dpy = impl->display;

	popup.win = XCreateSimpleWindow(dpy, impl->window, popup.x, popup.y, (unsigned int)popup.width, (unsigned int)popup.height, 1, impl->menuBorder, impl->menuBg);

	XSelectInput(dpy, popup.win, ExposureMask | ButtonPressMask | PointerMotionMask | LeaveWindowMask | StructureNotifyMask);

	XMapWindow(dpy, popup.win);

	return popup;
}

//----------------------------------------------------------------------------------------
static void ClosePopupsFromIndex(X11WindowImpl* impl, std::size_t index)
{
	if ((impl == nullptr) || (index >= impl->popups.size()))
	{
		return;
	}

	for (std::size_t i = index; i < impl->popups.size(); ++i)
	{
		if (impl->popups[i].win != 0)
		{
			XDestroyWindow(impl->display, impl->popups[i].win);
			impl->popups[i].win = 0;
		}
	}
	impl->popups.erase(impl->popups.begin() + static_cast<long>(index), impl->popups.end());
}

//----------------------------------------------------------------------------------------
static void CloseAllPopups(X11WindowImpl* impl)
{
	ClosePopupsFromIndex(impl, 0);
}

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Window::~Window()
{
	if (_nativeHandle != nullptr)
	{
		Close();
		WaitUntilClosed();
		auto impl = reinterpret_cast<X11WindowImpl*>(_nativeHandle);
		delete impl;
	}
}

//----------------------------------------------------------------------------------------
// Window creation methods
//----------------------------------------------------------------------------------------
bool Window::Create(::Display* display, const std::string& title, int clientWidth, int clientHeight, bool visible)
{
	// Create our native object
	//##TODO## Remove this entirely
	auto* impl = new X11WindowImpl();
	impl->display = display;

	// Load a simple fixed font for menu rendering
	impl->font = XLoadQueryFont(impl->display, "fixed");
	impl->menuBarHeight = (impl->font ? (impl->font->ascent + impl->font->descent + 8) : 24);

	// Calculate the dimensions of the window
	unsigned int outerWidth = (unsigned int)clientWidth;
	unsigned int outerHeight = (unsigned int)(clientHeight + impl->menuBarHeight);

	// Create the window
	int screen = DefaultScreen(impl->display);
	::Window root = RootWindow(impl->display, screen);
	unsigned long black = BlackPixel(impl->display, screen);
	unsigned long white = WhitePixel(impl->display, screen);
	impl->window = XCreateSimpleWindow(impl->display, root, 0, 0, outerWidth, outerHeight, 0, black, white);
	if (!impl->window)
	{
		if (impl->font)
		{
			XFreeFont(impl->display, impl->font);
		}
		delete impl;
		return false;
	}
	XStoreName(impl->display, impl->window, title.c_str());
	XSelectInput(impl->display, impl->window, ExposureMask | ButtonPressMask | PointerMotionMask | LeaveWindowMask | StructureNotifyMask);
	_nativeHandle = impl;

	// Create a child window representing the drawable client area below the menu bar
	impl->clientWindow = XCreateSimpleWindow(impl->display, impl->window, 0, impl->menuBarHeight, outerWidth, (unsigned int)clientHeight, 0, black, white);
	XSelectInput(impl->display, impl->clientWindow, ExposureMask | StructureNotifyMask);

	// Define our custom events
	impl->invokeAtom = XInternAtom(impl->display, "THREAD_INVOKE", False);
	impl->wmDeleteAtom = XInternAtom(impl->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(impl->display, impl->window, &impl->wmDeleteAtom, 1);

	// Create GC for drawing the menu into the top-level window
	impl->gc = XCreateGC(impl->display, impl->window, 0, nullptr);
	if (impl->font)
	{
		XSetFont(impl->display, impl->gc, impl->font->fid);
	}

	// Assign colors for the menu bar
	impl->menuBg = AllocColorOrDefault(impl->display, screen, "lightgray", white);
	impl->menuText = black;
	impl->menuBorder = black;
	impl->menuHighlightBg = AllocColorOrDefault(impl->display, screen, "gray", black);

	// Show the window if requested
	if (visible)
	{
		XMapWindow(impl->display, impl->window);
		XMapWindow(impl->display, impl->clientWindow);
		XFlush(impl->display);
	}

	// Mark the window as opened
	std::unique_lock<std::mutex> lock(_accessMutex);
	_windowClosed = false;
	return true;
}

//----------------------------------------------------------------------------------------
void Window::Close()
{
	auto* impl = static_cast<X11WindowImpl*>(_nativeHandle);
	if (!impl || !impl->display || !impl->window)
	{
		return;
	}

	XEvent ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = impl->window;
	ev.xclient.message_type = impl->wmDeleteAtom;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = impl->wmDeleteAtom;
	ev.xclient.data.l[1] = CurrentTime;

	XSendEvent(impl->display, impl->window, False, NoEventMask, &ev);
	XFlush(impl->display);
}

//----------------------------------------------------------------------------------------
void Window::WaitUntilClosed()
{
	auto* impl = static_cast<X11WindowImpl*>(_nativeHandle);
	if (!impl || !impl->display || !impl->window)
	{
		return;
	}

	std::unique_lock<std::mutex> lock(_accessMutex);
	while (!_windowClosed)
	{
		_windowClosedEvent.wait(lock);
	}
}

//----------------------------------------------------------------------------------------
void Window::UpdateMenu(MenuItem* root)
{
	_menu = root;
}

//----------------------------------------------------------------------------------------
// Native handle methods
//----------------------------------------------------------------------------------------
Display* Window::GetDisplay() const
{
	auto impl = reinterpret_cast<X11WindowImpl*>(_nativeHandle);
	return impl->display;
}

//----------------------------------------------------------------------------------------
::Window Window::GetWindow() const
{
	auto impl = reinterpret_cast<X11WindowImpl*>(_nativeHandle);
	return (impl->clientWindow != 0 ? impl->clientWindow : impl->window);
}

//----------------------------------------------------------------------------------------
::Window Window::GetInvokeWindow() const
{
	auto impl = reinterpret_cast<X11WindowImpl*>(_nativeHandle);
	return impl->window;
}

//----------------------------------------------------------------------------------------
xcb_connection_t* Window::GetXCBConnection() const
{
	auto* impl = static_cast<X11WindowImpl*>(_nativeHandle);
	return XGetXCBConnection(impl->display);
}

//----------------------------------------------------------------------------------------
xcb_window_t Window::GetXCBWindow() const
{
	auto* impl = static_cast<X11WindowImpl*>(_nativeHandle);
	return static_cast<xcb_window_t>(impl->clientWindow);
}

//----------------------------------------------------------------------------------------
// Message handling methods
//----------------------------------------------------------------------------------------
void Window::AttachCloseHandler(std::function<void()> onClose)
{
	_onClose = std::move(onClose);
}

//----------------------------------------------------------------------------------------
void Window::RemoveCloseHandler()
{
	_onClose = nullptr;
}

//----------------------------------------------------------------------------------------
unsigned long Window::GetInvokeAtom() const
{
	auto impl = reinterpret_cast<X11WindowImpl*>(_nativeHandle);
	return impl->invokeAtom;
}

//----------------------------------------------------------------------------------------
void Window::HandleX11Event(const XEvent& ev)
{
	auto* impl = static_cast<X11WindowImpl*>(_nativeHandle);
	if (ev.xany.window == impl->window)
	{
		// Main window events.
		switch (ev.type)
		{
		case Expose:
		{
			// Redraw menu bar on expose.
			if (_menu)
			{
				XWindowAttributes attrs{};
				XGetWindowAttributes(impl->display, impl->window, &attrs);
				DrawX11MenuBar(impl, _menu, attrs.width);
			}
			break;
		}
		case ConfigureNotify:
		{
			// Resize client window to remain directly below the menu bar.
			if (impl->clientWindow != 0)
			{
				int clientHeight = ev.xconfigure.height - impl->menuBarHeight;
				if (clientHeight < 0)
				{
					clientHeight = 0;
				}
				XMoveResizeWindow(impl->display, impl->clientWindow, 0, impl->menuBarHeight, (unsigned int)ev.xconfigure.width, (unsigned int)clientHeight);
			}
			break;
		}
		case MotionNotify:
		{
			// Update hover state for the menubar items.
			int x = ev.xmotion.x;
			int y = ev.xmotion.y;

			int newHot = -1;

			if (y >= 0 && y < impl->menuBarHeight && _menu)
			{
				int currentX = 4;
				int visualIndex = 0;

				for (MenuItem& item : _menu->GetChildren())
				{
					if (item.isSeparator)
					{
						continue;
					}

					int textWidth = MeasureTextWidth(impl, item.caption);
					int itemWidth = textWidth + 16;

					if (x >= currentX && x < currentX + itemWidth)
					{
						newHot = visualIndex;
						break;
					}

					currentX += itemWidth;
					++visualIndex;
				}
			}

			if (newHot != impl->hotMenuBarItem)
			{
				impl->hotMenuBarItem = newHot;

				if (_menu)
				{
					XWindowAttributes attrs{};
					XGetWindowAttributes(impl->display, impl->window, &attrs);
					DrawX11MenuBar(impl, _menu, attrs.width);
				}
			}
			break;
		}
		case LeaveNotify:
		{
			// Mouse left the main window: clear menubar hover.
			if (impl->hotMenuBarItem != -1)
			{
				impl->hotMenuBarItem = -1;

				if (_menu)
				{
					XWindowAttributes attrs{};
					XGetWindowAttributes(impl->display, impl->window, &attrs);
					DrawX11MenuBar(impl, _menu, attrs.width);
				}
			}
			break;
		}
		case ButtonPress:
		{
			// Clicks in the menubar open the first-level popup.
			int x = ev.xbutton.x;
			int y = ev.xbutton.y;

			if (!_menu)
			{
				// No menu: nothing to do.
				break;
			}

			// If click is inside the menu bar region.
			if (y >= 0 && y < impl->menuBarHeight)
			{
				int currentX = 4;
				int popupX = 0;

				MenuItem* clickedItem = nullptr;

				for (MenuItem& item : _menu->GetChildren())
				{
					if (item.isSeparator)
					{
						continue;
					}

					int textWidth = MeasureTextWidth(impl, item.caption);
					int itemWidth = textWidth + 16;

					if (x >= currentX && x < currentX + itemWidth)
					{
						clickedItem = &item;
						popupX = currentX;
						break;
					}

					currentX += itemWidth;
				}

				if (clickedItem)
				{
					// Close any existing popups and open a new one directly under the menubar item
					CloseAllPopups(impl);

					if (!clickedItem->GetChildren().empty())
					{
						int popupY = impl->menuBarHeight;
						PopupMenu popup = CreatePopupForNode(impl, clickedItem, popupX, popupY);
						if (popup.win != 0)
						{
							impl->popups.push_back(popup);
						}
					}
					else
					{
						// Direct menubar item with no children - treat as leaf.
						if (clickedItem->isTickable)
						{
							clickedItem->isTicked = !clickedItem->isTicked;
						}
						if (clickedItem->onClick)
						{
							clickedItem->onClick();
						}
					}
				}
				else
				{
					// Click in menubar but not on an item-  close popups.
					CloseAllPopups(impl);
				}
			}
			else
			{
				// Click outside the menubar in the main window - close popups.
				CloseAllPopups(impl);
			}
			break;
		}
		case ClientMessage:
		{
			if ((Atom)ev.xclient.data.l[0] == impl->wmDeleteAtom)
			{
				XDestroyWindow(impl->display, impl->window);
			}
			else if (ev.xclient.message_type == impl->invokeAtom)
			{
#if INTPTR_MAX == INT64_MAX
				uint32_t lowBytes = (uint32_t)ev.xclient.data.l[0];
				uint32_t highBytes = (uint32_t)ev.xclient.data.l[1];
				auto* function = reinterpret_cast<std::function<void()>*>((((uintptr_t)highBytes) << 32) | (uintptr_t)lowBytes);
#else
				auto* function = reinterpret_cast<std::function<void()>*>(ev.xclient.data.l[0]);
#endif
				(*function)();
				delete function;
			}
			break;
		}
		case DestroyNotify:
		{
			if (ev.xdestroywindow.window == impl->window)
			{
				// Cleanup core resources
				CloseAllPopups(impl);

				if (_onClose)
				{
					_onClose();
				}
				_onClose = {};

				if (impl->gc)
				{
					XFreeGC(impl->display, impl->gc);
					impl->gc = 0;
				}
				if (impl->font)
				{
					XFreeFont(impl->display, impl->font);
					impl->font = nullptr;
				}
				impl->window = 0;
				impl->clientWindow = 0;

				std::unique_lock<std::mutex> lock(_accessMutex);
				_windowClosed = true;
				_windowClosedEvent.notify_all();
			}
			break;
		}
		default:
			break;
		}
	}
	else
	{
		// Events for popup windows
		auto it = std::find_if(impl->popups.begin(), impl->popups.end(), [&](const PopupMenu& p) { return p.win == ev.xany.window; });
		if (it == impl->popups.end())
		{
			return;
		}

		std::size_t popupIndex = static_cast<std::size_t>(std::distance(impl->popups.begin(), it));
		PopupMenu& popup = *it;

		switch (ev.type)
		{
		case Expose:
		{
			DrawPopupMenu(impl, popup);
			break;
		}
		case MotionNotify:
		{
			// Hover feedback in popup rows
			int y = ev.xmotion.y;

			const auto& children = popup.node->GetChildren();
			if (children.empty())
			{
				break;
			}

			int rowIndex = y / popup.itemHeight;
			int newHotIndex = -1;

			if (rowIndex >= 0)
			{
				int i = 0;
				for (auto itChild = children.begin(); itChild != children.end(); ++itChild, ++i)
				{
					if (i == rowIndex)
					{
						if (!itChild->isSeparator)
						{
							newHotIndex = rowIndex;
						}
						break;
					}
				}
			}

			if (newHotIndex != popup.hotIndex)
			{
				popup.hotIndex = newHotIndex;
				DrawPopupMenu(impl, popup);
			}
			break;
		}
		case LeaveNotify:
		{
			// Clear hover for popup
			if (popup.hotIndex != -1)
			{
				popup.hotIndex = -1;
				DrawPopupMenu(impl, popup);
			}
			break;
		}
		case ButtonPress:
		{
			// Click inside a popup item.
			int y = ev.xbutton.y;

			auto& children = popup.node->GetChildren();
			if (children.empty())
			{
				break;
			}

			int index = y / popup.itemHeight;
			if (index < 0 || index >= (int)children.size())
			{
				break;
			}

			auto childIt = children.begin();
			std::advance(childIt, index);
			MenuItem& child = const_cast<MenuItem&>(*childIt);

			if (child.isSeparator)
			{
				// Clicking on a separator does nothing.
				break;
			}

			// Close any deeper popups (children of this popup).
			ClosePopupsFromIndex(impl, popupIndex + 1);

			if (!child.GetChildren().empty())
			{
				// Open nested submenu to the right.
				int subX = popup.x + popup.width;
				int subY = popup.y + index * popup.itemHeight;

				PopupMenu subPopup = CreatePopupForNode(impl, &child, subX, subY);
				if (subPopup.win != 0)
				{
					impl->popups.push_back(subPopup);
				}
			}
			else
			{
				// Leaf: toggle + callback, then close all popups.
				if (child.isTickable)
				{
					child.isTicked = !child.isTicked;
				}
				if (child.onClick)
				{
					child.onClick();
				}

				CloseAllPopups(impl);
			}
			break;
		}
		default:
			break;
		}
	}
}

} // namespace cobalt::graphics::testing
