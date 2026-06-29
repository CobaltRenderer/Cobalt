// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IThreadInvocation.h"
#include "MenuItem.h"
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#elif defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/CGLTypes.h>
#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
struct NSWindow;
struct NSView;
#endif
#else
#include <X11/Xatom.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#endif

namespace cobalt { namespace graphics { namespace testing {

class Window
{
public:
	// Constructors
	~Window();

	// Window creation methods
#ifdef _WIN32
	bool Create(const std::string& title, int clientWidth, int clientHeight, DWORD windowStyle, DWORD windowStyleEx, HINSTANCE hinstance, HWND parent, HICON icon, bool visible);
#elif defined(__APPLE__)
	bool Create(const std::string& title, int clientWidth, int clientHeight, bool visible);
#else
	bool Create(::Display* display, const std::string& title, int clientWidth, int clientHeight, bool visible);
#endif
	void Close();
	void WaitUntilClosed();
	void UpdateMenu(MenuItem* root);

	// Native handle methods
#ifdef _WIN32
	HWND GetOsHandle();
#elif defined(__APPLE__)
	NSView* GetAppKitView() const;
#else
	Display* GetDisplay() const;
	::Window GetWindow() const;
	::Window GetInvokeWindow() const;
	xcb_connection_t* GetXCBConnection() const;
	xcb_window_t GetXCBWindow() const;
#endif

	// Message handling methods
	void AttachCloseHandler(std::function<void()> onClose);
	void RemoveCloseHandler();
#ifdef _WIN32
	void AttachHandler(UINT msg, std::function<LRESULT(WPARAM, LPARAM)> handler);
	void RemoveHandler(UINT msg);
#elif !defined(__APPLE__)
	Atom GetInvokeAtom() const;
	void HandleX11Event(const XEvent& event);
#endif

private:
	// Message handling methods
#ifdef _WIN32
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

private:
#ifdef _WIN32
	HWND _hwnd = nullptr;
#else
	void* _nativeHandle = nullptr;
#endif
	MenuItem* _menu = nullptr;
	std::mutex _accessMutex;
	bool _windowClosed = true;
	std::condition_variable _windowClosedEvent;
#ifndef __APPLE__
	std::function<void()> _onClose;
#endif
#ifdef _WIN32
	std::map<UINT, std::function<LRESULT(WPARAM, LPARAM)>> _customHandlers;
#endif
};

}}} // namespace cobalt::graphics::testing
