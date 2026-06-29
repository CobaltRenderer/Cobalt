#import <Cocoa/Cocoa.h>
#include "MenuItem.h"

//----------------------------------------------------------------------------------------
@interface MenuItemActionTarget : NSObject
{
@private
    cobalt::graphics::testing::MenuItem* _item;
}
- (instancetype)initWithItem:(cobalt::graphics::testing::MenuItem*)item;
- (void)onMenuItem:(id)sender;
@end

//----------------------------------------------------------------------------------------
@implementation MenuItemActionTarget
- (instancetype)initWithItem:(cobalt::graphics::testing::MenuItem*)item
{
    self = [super init];
    _item = item;
    return self;
}

//----------------------------------------------------------------------------------------
- (void)onMenuItem:(id)sender
{
    // Toggle tick state if applicable
    if (_item->isTickable)
    {
        NSMenuItem* menuItem = (NSMenuItem*)sender;
        _item->isTicked = !_item->isTicked;
        [menuItem setState:(_item->isTicked ? NSControlStateValueOn : NSControlStateValueOff)];
    }

    // Perform the click action if one is attached
    if (_item->onClick)
    {
        _item->onClick();
    }
}
@end

namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
static void BuildCocoaSubmenu(NSMenu* nsMenu, MenuItem& node)
{
    for (MenuItem& child : node.GetChildren())
    {
        if (child.isSeparator)
        {
            // Add the separator menu item
            [nsMenu addItem:[NSMenuItem separatorItem]];
        }
        else if (!child.GetChildren().empty())
        {
            // Create the submenu item
            NSString* title = [NSString stringWithUTF8String:child.caption.c_str()];
            NSMenuItem* topItem = [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];

            // Populate the submenu
            NSMenu* subMenu = [[NSMenu alloc] initWithTitle:title];
            BuildCocoaSubmenu(subMenu, child);
            [topItem setSubmenu:subMenu];

            // Add the submenu to the menu structure
            [nsMenu addItem:topItem];
            [subMenu release];
            [topItem release];
        }
        else
        {
            // Create the leaf menu item
            NSString* title = [NSString stringWithUTF8String:child.caption.c_str()];
            NSMenuItem* leafItem = [[NSMenuItem alloc] initWithTitle:title action:@selector(onMenuItem:) keyEquivalent:@""];

            // Attach the check state and/or click handler
            MenuItemActionTarget* target = [[MenuItemActionTarget alloc] initWithItem:&child];
            leafItem.representedObject = target;
            leafItem.target = target; // Weak reference. We use the "representedObject" reference above to keep it alive.
            [target release];
            if (child.isTickable)
            {
                [leafItem setState:(child.isTicked ? NSControlStateValueOn : NSControlStateValueOff)];
            }

            // Add the menu item to the menu structure
            [nsMenu addItem:leafItem];
            [leafItem release];
        }
    }
}

//----------------------------------------------------------------------------------------
void AttachCocoaMenuBar(MenuItem& root)
{
    // Build the menu structure
    NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
    BuildCocoaSubmenu(mainMenu, root);

    // Attach the menu structure to the application
    [NSApp setMainMenu:mainMenu];
    [mainMenu release];
}

} // namespace cobalt::graphics::testing
