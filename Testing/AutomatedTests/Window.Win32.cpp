// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Window.h"
#include <Cobalt/Debug/Debug.pkg>
#include "UnicodeConversion.h"
#include <locale>
#include <string>
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Window::~Window()
{
	if (_hwnd != nullptr)
	{
		Close();
		WaitUntilClosed();
	}
}

//----------------------------------------------------------------------------------------
// Window creation methods
//----------------------------------------------------------------------------------------
bool Window::Create(const std::string& title, int clientWidth, int clientHeight, DWORD windowStyle, DWORD windowStyleEx, HINSTANCE hinstance, HWND parent, HICON icon, bool visible)
{
	// Calculate the dimensions of the window
	RECT clientRect;
	clientRect.left = 0;
	clientRect.top = 0;
	clientRect.right = LONG(clientWidth);
	clientRect.bottom = LONG(clientHeight);
	AdjustWindowRectEx(&clientRect, windowStyle, (_menu != nullptr ? TRUE : FALSE), windowStyleEx);
	int newWindowWidth = clientRect.right - clientRect.left;
	int newWindowHeight = clientRect.bottom - clientRect.top;

	// Register the main window class
	WNDCLASSEXW wc = {};
	std::wstring windowClassName = L"CobaltGraphicsTestWindow";
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinstance;
	wc.hCursor = nullptr;
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszClassName = windowClassName.c_str();
	wc.hIcon = icon;
	wc.hIconSm = icon;
	RegisterClassExW(&wc);

	// Create the window
	_hwnd = CreateWindowExW(windowStyleEx, windowClassName.c_str(), UTF8ToUTF16(title).c_str(), windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, newWindowWidth, newWindowHeight, parent, nullptr, wc.hInstance, reinterpret_cast<LPVOID>(this));
	if (_hwnd == nullptr)
	{
		return false;
	}
	SetWindowLongPtr(_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	// If a menu structure has been provided, populate it now.
	if (_menu != nullptr)
	{
		UpdateMenu(_menu);
	}

	// Show the window if requested
	if (visible)
	{
		ShowWindow(_hwnd, SW_SHOWNORMAL);
	}

	// Mark the window as opened
	std::unique_lock<std::mutex> lock(_accessMutex);
	_windowClosed = false;
	lock.unlock();
	return true;
}

//----------------------------------------------------------------------------------------
void Window::Close()
{
	if (_hwnd != nullptr)
	{
		PostMessage(_hwnd, WM_CLOSE, 0, 0);
	}
}

//----------------------------------------------------------------------------------------
void Window::WaitUntilClosed()
{
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
	if (_hwnd != nullptr)
	{
		uint32_t counter = 1000;
		root->UpdateMenuItem(root, &counter);
		::SetMenu(_hwnd, root->GetHandle());
		::DrawMenuBar(_hwnd);
	}
}

//----------------------------------------------------------------------------------------
// Native handle methods
//----------------------------------------------------------------------------------------
HWND Window::GetOsHandle()
{
	return _hwnd;
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
void Window::AttachHandler(UINT msg, std::function<LRESULT(WPARAM, LPARAM)> handler)
{
	_customHandlers[msg] = std::move(handler);
}

//----------------------------------------------------------------------------------------
void Window::RemoveHandler(UINT msg)
{
	_customHandlers.erase(msg);
}

//----------------------------------------------------------------------------------------
LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// If we're still in the initial phases of window creation, or in the final stages of window closing, pass the
	// message onto the default handler.
	auto* window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (window == nullptr)
	{
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	// Handle menu item selection
	if (msg == WM_COMMAND)
	{
		if (window->_menu != nullptr)
		{
			window->_menu->RunCallback(window->_menu->GetHandle(), window->_menu, LOWORD(wParam));
		}
		return 0;
	}

	// Handle window close requests
	if ((msg == WM_CLOSE) || (msg == WM_DESTROY))
	{
		// Run the close handler
		if (window->_onClose)
		{
			window->_onClose();
		}
		window->_onClose = {};

		// Clear the association between the native window and our handler
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		window->_hwnd = nullptr;

		// Notify any waiting threads that the window is now closed
		std::unique_lock<std::mutex> lock(window->_accessMutex);
		window->_windowClosed = true;
		lock.unlock();
		window->_windowClosedEvent.notify_all();

		// If this was a close handler, destroy the window now.
		if (msg == WM_CLOSE)
		{
			DestroyWindow(hwnd);
		}
		return 0;
	}

	// If there is a custom handler for this message, execute it.
	auto customHandlersIterator = window->_customHandlers.find(msg);
	if (customHandlersIterator != window->_customHandlers.end())
	{
		return customHandlersIterator->second(wParam, lParam);
	}

	// Since we didn't handle this message specifically, pass it to the default handler.
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace cobalt::graphics::testing
