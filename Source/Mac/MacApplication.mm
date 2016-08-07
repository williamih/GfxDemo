#include "Mac/MacApplication.h"

#import <Cocoa/Cocoa.h>

@interface MacCocoaApplication : NSApplication

- (void)loadNib;
- (IBAction)quit:(id)sender;

@property (retain) NSArray *nibTopLevelObjects;
@property (assign) void (*onQuitCallback)(void*);
@property (assign) void *onQuitUserdata;

@end

@implementation MacCocoaApplication

@synthesize nibTopLevelObjects, onQuitCallback, onQuitUserdata;

- (void)loadNib {
    NSNib *nib = [[NSNib alloc] initWithNibNamed:@"MainMenu" bundle:nil];

    NSArray *array;
    [nib instantiateWithOwner:self topLevelObjects:&array];
    self.nibTopLevelObjects = array;
}

- (IBAction)quit:(id)sender {
    if (self.onQuitCallback)
        self.onQuitCallback(self.onQuitUserdata);
}

@end

void MacApplication::Initialize()
{
    [MacCocoaApplication sharedApplication];
    [NSApp loadNib];

    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    NSString* appDir = [bundlePath stringByDeletingLastPathComponent];
    [[NSFileManager defaultManager] changeCurrentDirectoryPath:appDir];
}

void MacApplication::RegisterOnQuit(void (*callback)(void*), void* userdata)
{
    MacCocoaApplication* app = (MacCocoaApplication*)NSApp;
    app.onQuitCallback = callback;
    app.onQuitUserdata = userdata;
}
