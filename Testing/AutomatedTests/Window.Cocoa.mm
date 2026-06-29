#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#include "Window.h"
#include "MenuItem.h"
#include <Cobalt/Debug/Debug.pkg>

//----------------------------------------------------------------------------------------
@interface WindowCloseDelegate : NSObject<NSWindowDelegate>
{
@public
    std::function<void()> onClose;
}
@end

@implementation WindowCloseDelegate
- (void)windowWillClose:(NSNotification *)notification
{
    if (onClose)
    {
        onClose();
    }
}
@end

namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
void AttachCocoaMenuBar(MenuItem& root);

//----------------------------------------------------------------------------------------
struct CocoaWindowImpl
{
    NSWindow* window = nil;
    WindowCloseDelegate* closeDelegate = nil;
};

//----------------------------------------------------------------------------------------
NSView* Window::GetAppKitView() const
{
    auto* impl = static_cast<CocoaWindowImpl*>(_nativeHandle);
    return impl->window.contentView;
}

//----------------------------------------------------------------------------------------
Window::~Window()
{
    auto* impl = static_cast<CocoaWindowImpl*>(_nativeHandle);
    if (impl)
    {
        delete impl;
        _nativeHandle = nullptr;
    }
}

//----------------------------------------------------------------------------------------
bool Window::Create(const std::string& title, int clientWidth, int clientHeight, bool visible)
{
    auto* impl = new CocoaWindowImpl();

    CGFloat desiredPixelW = clientWidth;
    CGFloat desiredPixelH = clientHeight;

    NSScreen *screen = [NSScreen mainScreen];
    CGFloat scale = screen.backingScaleFactor;

    NSRect contentRectPts = NSMakeRect(0, 0, desiredPixelW / scale, desiredPixelH / scale);
    NSWindowStyleMask style = (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable);
    impl->window = [[NSWindow alloc] initWithContentRect:contentRectPts styleMask:style backing:NSBackingStoreBuffered defer:NO];
    if (!impl->window)
    {
        delete impl;
        return false;
    }

    [impl->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    [impl->window setReleasedWhenClosed:NO];

    if (visible)
    {
        [impl->window makeKeyAndOrderFront:nil];
        dispatch_async(dispatch_get_main_queue(), ^{ [NSApp activateIgnoringOtherApps:YES]; });
    }

    _nativeHandle = impl;

    return true;
}

//----------------------------------------------------------------------------------------
void Window::Close()
{
    auto* impl = static_cast<CocoaWindowImpl*>(_nativeHandle);
    if (!impl || !impl->window)
    {
        return;
    }

    [impl->window performClose:nil];
}

//----------------------------------------------------------------------------------------
void Window::WaitUntilClosed()
{
    //##TODO##
}

//----------------------------------------------------------------------------------------
void Window::UpdateMenu(MenuItem* root)
{
    auto* impl = static_cast<CocoaWindowImpl*>(_nativeHandle);

    _menu = root;
    if (_menu)
    {
        AttachCocoaMenuBar(*_menu);
        [[NSApp mainMenu] update];
    }
}

//----------------------------------------------------------------------------------------
void Window::AttachCloseHandler(std::function<void()> onClose)
{
    auto* impl = static_cast<CocoaWindowImpl*>(_nativeHandle);
    if (!impl || !impl->window)
    {
        return;
    }

    if (!impl->closeDelegate)
    {
        impl->closeDelegate = [[WindowCloseDelegate alloc] init];
        impl->window.delegate = impl->closeDelegate;
    }

    impl->closeDelegate->onClose = std::move(onClose);
}

//----------------------------------------------------------------------------------------
void Window::RemoveCloseHandler()
{
    auto* impl = static_cast<CocoaWindowImpl*>(_nativeHandle);
    if (!impl || !impl->window || !impl->closeDelegate)
    {
        return;
    }
    impl->closeDelegate->onClose = nullptr;
}

} // namespace cobalt::graphics::testing
