#ifdef __APPLE__

#include "OsWindow.h"
#include <vector>
#include <queue>

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <QuartzCore/QuartzCore.h>

#include "Core/Macros.h"

@class OsWindowMacView;
@class OsWindowMacEventTarget;

// -----------------------------------------------------------------------------
// OsWindowMacImpl class declaration
// -----------------------------------------------------------------------------

class OsWindowMacImpl {
public:
    OsWindowMacImpl(int width, int height, const OsWindowPixelFormat& pf);
    ~OsWindowMacImpl();

    void RegisterEvent(OsEventType type,
                       void (*callback)(const OsEvent&, void*),
                       void* userdata);
    void UnregisterEvent(OsEventType type,
                         void (*callback)(const OsEvent&, void*),
                         void* userdata);

    OsWindow::OsHandle* GetOsHandle() const;

    // Implementation-detail methods that don't correspond to OsWindow methods
    void DispatchEvents();
    void DispatchEvent(const OsEvent& event);
    void EnqueueEvent(const OsEvent& event);
private:
    struct ResizeHandler {
        void (*callback)(OsWindow*, void*);
        void* userdata;
    };

    struct EventHandler {
        OsEventType type;
        void (*callback)(const OsEvent&, void*);
        void* userdata;
    };

    void CreateTimers();

    NSWindow* m_window;
    OsWindowMacView* m_view;
    OsWindowMacEventTarget* m_eventTarget;
    NSTimer* m_eventTimer;
    std::queue<OsEvent> m_eventQueue;
    std::vector<EventHandler> m_eventHandlers;
};

static const OsWindowMacImpl* Cast(const OsWindow* w) { return (const OsWindowMacImpl*)w; }
static OsWindowMacImpl* Cast(OsWindow* w) { return (OsWindowMacImpl*)w; }
static OsWindow* Cast(OsWindowMacImpl* w) { return (OsWindow*)w; }

// -----------------------------------------------------------------------------
// OsWindowMacView class
// -----------------------------------------------------------------------------

@interface NSEvent (OsWindowMacAdditions)

- (NSString *)charactersIgnoringModifiersIncludingShift;

@end

@implementation NSEvent (OsWindowMacAdditions)

// Modified from code by John Stiles at
// http://lists.apple.com/archives/cocoa-dev/2008/Apr/msg01582.html.
- (NSString *)charactersIgnoringModifiersIncludingShift {
    // First, try -charactersIgnoringModifiers and look for keys which UCKeyTranslate translates
    // differently than AppKit.
    NSString *c = [self charactersIgnoringModifiers];
    if ([c length] == 1) {
        unichar codepoint = [c characterAtIndex:0];
        if ((codepoint >= 0xF700 && codepoint <= 0xF8FF) || codepoint == 0x7F) {
            return c;
        }
    }

    // OK, this is not a "special" key, so ask UCKeyTranslate to give us the character with no
    // modifiers attached. Actually, that's not quite accurate--we attach the Command modifier.
    // Command hints the OS to use Latin characters where possible, which is generally what we
    // are after here.
    NSString *result = @"";

    TISInputSourceRef inputSourceRef = TISCopyCurrentKeyboardInputSource();

    // Note: the docs say we shouldn't release this CFDataRef.
    CFDataRef data = (CFDataRef)TISGetInputSourceProperty(
        inputSourceRef,
        kTISPropertyUnicodeKeyLayoutData
    );

    const UCKeyboardLayout *uchrData = NULL;
    if (data)
        uchrData = (const UCKeyboardLayout *)CFDataGetBytePtr(data);

    if (uchrData != NULL) {
        // Use UCKeyTranslate.
        UniChar buf[256];
        UniCharCount actualStringLength;
        UInt32 deadKeyState = 0;
        OSStatus err = UCKeyTranslate(
            uchrData,
            [self keyCode],
            kUCKeyActionDown,
            // forcing the Command key to "on" hints the OS to show Latin characters where possible
            cmdKey >> 8,
            LMGetKbdType(),
            kUCKeyTranslateNoDeadKeysMask,
            &deadKeyState,
            sizeof buf / sizeof buf[0],
            &actualStringLength,
            buf
        );
        if (err != noErr)
            return @"";

        result = [NSString stringWithCharacters:buf length:actualStringLength];

    }

    CFRelease(inputSourceRef);

    return result;
}

@end

static OsKeyCode CharacterToKeyCode(unichar c)
{
    switch (c) {
        case NSLeftArrowFunctionKey: return OSKEY_LEFT_ARROW;
        case NSRightArrowFunctionKey: return OSKEY_RIGHT_ARROW;
        case NSUpArrowFunctionKey: return OSKEY_UP_ARROW;
        case NSDownArrowFunctionKey: return OSKEY_DOWN_ARROW;
        case NSDeleteFunctionKey: return OSKEY_DELETE;
        case 27: return OSKEY_ESCAPE;
        case NSEnterCharacter:
        case NSNewlineCharacter:
        case NSCarriageReturnCharacter: return OSKEY_ENTER;
        case ' ': return OSKEY_SPACE;
        case '[': return OSKEY_LBRACKET;
        case ']': return OSKEY_RBRACKET;
        case '/': return OSKEY_SLASH;
        case '.': return OSKEY_PERIOD;
        case ',': return OSKEY_COMMA;
        case '\'': return OSKEY_QUOTE;
        case '=': return OSKEY_EQUAL;
        case ';': return OSKEY_SEMICOLON;
        case '-': return OSKEY_MINUS;
        case '0': return OSKEY_NUM0;
        case '1': return OSKEY_NUM1;
        case '2': return OSKEY_NUM2;
        case '3': return OSKEY_NUM3;
        case '4': return OSKEY_NUM4;
        case '5': return OSKEY_NUM5;
        case '6': return OSKEY_NUM6;
        case '7': return OSKEY_NUM7;
        case '8': return OSKEY_NUM8;
        case '9': return OSKEY_NUM9;
        case 'a': return OSKEY_A;
        case 'b': return OSKEY_B;
        case 'c': return OSKEY_C;
        case 'd': return OSKEY_D;
        case 'e': return OSKEY_E;
        case 'f': return OSKEY_F;
        case 'g': return OSKEY_G;
        case 'h': return OSKEY_H;
        case 'i': return OSKEY_I;
        case 'j': return OSKEY_J;
        case 'k': return OSKEY_K;
        case 'l': return OSKEY_L;
        case 'm': return OSKEY_M;
        case 'n': return OSKEY_N;
        case 'o': return OSKEY_O;
        case 'p': return OSKEY_P;
        case 'q': return OSKEY_Q;
        case 'r': return OSKEY_R;
        case 's': return OSKEY_S;
        case 't': return OSKEY_T;
        case 'u': return OSKEY_U;
        case 'v': return OSKEY_V;
        case 'w': return OSKEY_W;
        case 'x': return OSKEY_X;
        case 'y': return OSKEY_Y;
        case 'z': return OSKEY_Z;

        default: return OSKEY_LAST;
    }
}

// -----------------------------------------------------------------------------
// OsWindowMacView class
// -----------------------------------------------------------------------------

@interface OsWindowMacView : NSView

@property (assign) OsWindowMacImpl *impl;

@end

@implementation OsWindowMacView

@synthesize impl;

- (CALayer *)makeBackingLayer {
    CAMetalLayer *layer = [[CAMetalLayer alloc] init];
    layer.framebufferOnly = YES;
    return layer;
}

- (void)viewDidEndLiveResize {
    [super viewDidEndLiveResize];
    if (self.impl) {
        OsEvent event;
        event.type = OSEVENT_WINDOW_RESIZE;
        self.impl->EnqueueEvent(event);
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)enqueueKeyEvent:(NSEvent *)theEvent type:(OsEventType)type {
    NSString *str = [theEvent charactersIgnoringModifiersIncludingShift];
    NSEventModifierFlags modifierFlags = [theEvent modifierFlags];

    OsEvent e;
    e.type = type;
    e.key.code = CharacterToKeyCode([str characterAtIndex:0]);
    if (e.key.code == OSKEY_LAST)
        return;
    e.key.modifierFlags = 0;
    if (modifierFlags & NSAlternateKeyMask)
        e.key.modifierFlags |= OsKeyEvent::FLAG_ALT;
    if (modifierFlags & NSControlKeyMask)
        e.key.modifierFlags |= OsKeyEvent::FLAG_CONTROL;
    if (modifierFlags & NSShiftKeyMask)
        e.key.modifierFlags |= OsKeyEvent::FLAG_SHIFT;

    impl->EnqueueEvent(e);
}

- (void)keyDown:(NSEvent *)theEvent {
    OsEventType type = [theEvent isARepeat] ? OSEVENT_KEY_DOWN_REPEAT : OSEVENT_KEY_DOWN;
    [self enqueueKeyEvent:theEvent type:type];
}

- (void)keyUp:(NSEvent *)theEvent {
    [self enqueueKeyEvent:theEvent type:OSEVENT_KEY_UP];
}

- (void)enqueueMouseButtonEvent:(NSEvent *)theEvent type:(OsEventType)type button:(OsMouseButton)button {
    NSPoint point = [self convertPoint:[theEvent locationInWindow] fromView:nil];

    OsEvent event;
    event.type = type;
    event.mouseButton.button = button;
    event.mouseButton.x = (int)point.x;
    event.mouseButton.y = (int)point.y;
    event.mouseButton.normalizedX = (float)(point.x / [self bounds].size.width);
    event.mouseButton.normalizedY = (float)(point.y / [self bounds].size.height);

    impl->EnqueueEvent(event);
}

- (void)mouseDown:(NSEvent *)theEvent {
    [self enqueueMouseButtonEvent:theEvent type:OSEVENT_MOUSE_BUTTON_DOWN button:OSMOUSE_BUTTON_LEFT];
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    [self enqueueMouseButtonEvent:theEvent type:OSEVENT_MOUSE_BUTTON_DOWN button:OSMOUSE_BUTTON_RIGHT];
}

- (void)mouseUp:(NSEvent *)theEvent {
    [self enqueueMouseButtonEvent:theEvent type:OSEVENT_MOUSE_BUTTON_UP button:OSMOUSE_BUTTON_LEFT];
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    [self enqueueMouseButtonEvent:theEvent type:OSEVENT_MOUSE_BUTTON_UP button:OSMOUSE_BUTTON_RIGHT];
}

- (void)enqueueMouseDraggedEvent:(NSEvent *)theEvent button:(OsMouseButton)button {
    NSPoint point = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    CGFloat deltaX = [theEvent deltaX];
    CGFloat deltaY = -[theEvent deltaY];

    OsEvent event;
    event.type = OSEVENT_MOUSE_DRAG;
    event.mouseDrag.button = button;
    event.mouseDrag.x = (int)point.x;
    event.mouseDrag.y = (int)point.y;
    event.mouseDrag.normalizedX = (float)(point.x / [self bounds].size.width);
    event.mouseDrag.normalizedY = (float)(point.y / [self bounds].size.height);
    event.mouseDrag.deltaX = (int)deltaX;
    event.mouseDrag.deltaY = (int)deltaY;
    event.mouseDrag.normalizedDeltaX = (float)(deltaX / [self bounds].size.width);
    event.mouseDrag.normalizedDeltaY = (float)(deltaY / [self bounds].size.height);

    impl->EnqueueEvent(event);
}

- (void)mouseDragged:(NSEvent *)theEvent {
    [self enqueueMouseDraggedEvent:theEvent button:OSMOUSE_BUTTON_LEFT];
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
    [self enqueueMouseDraggedEvent:theEvent button:OSMOUSE_BUTTON_RIGHT];
}

@end

// -----------------------------------------------------------------------------
// OsWindowMacEventTarget class
// -----------------------------------------------------------------------------

@interface OsWindowMacEventTarget : NSObject

- (void)timerFired:(NSTimer *)timer;

@property (assign) OsWindow *osWindow;

@end

@implementation OsWindowMacEventTarget

@synthesize osWindow;

- (void)timerFired:(NSTimer *)timer {
    Cast(osWindow)->DispatchEvents();
}

@end

// -----------------------------------------------------------------------------
// OsWindowMacImpl class implementation
// -----------------------------------------------------------------------------

OsWindowMacImpl::OsWindowMacImpl(int width, int height, const OsWindowPixelFormat& pf)
    : m_window(nil)
    , m_view(nil)
    , m_eventTarget(nil)
    , m_eventTimer(nil)
    , m_eventQueue()
    , m_eventHandlers()
{
    NSUInteger styleMask = NSTitledWindowMask | NSClosableWindowMask |
        NSMiniaturizableWindowMask | NSResizableWindowMask;
    NSRect contentRect = NSMakeRect(0, 0, width, height);
    m_window = [[NSWindow alloc] initWithContentRect:contentRect
                                           styleMask:styleMask
                                             backing:NSBackingStoreBuffered
                                               defer:YES];

    m_view = [[OsWindowMacView alloc] initWithFrame:[m_window frame]];
    m_view.impl = this;
    [m_view setWantsLayer:YES];

    [m_window setReleasedWhenClosed:NO];
    [m_window setContentView:m_view];
    [m_window makeFirstResponder:[m_window contentView]];
    [m_window setAcceptsMouseMovedEvents:YES];
    [m_window makeKeyAndOrderFront:NSApp];
    [m_window setExcludedFromWindowsMenu:YES];
    [m_window center];

    CreateTimers();
}

void OsWindowMacImpl::CreateTimers()
{
    m_eventTarget = [[OsWindowMacEventTarget alloc] init];
    m_eventTarget.osWindow = Cast(this);

    m_eventTimer = [NSTimer scheduledTimerWithTimeInterval:0
                                                    target:m_eventTarget
                                                  selector:@selector(timerFired:)
                                                  userInfo:nil
                                                   repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:m_eventTimer forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:m_eventTimer forMode:NSEventTrackingRunLoopMode];
}

OsWindowMacImpl::~OsWindowMacImpl()
{
    [m_eventTarget release];
    [m_eventTimer release];
    [m_window release];
    [m_view release];
}

void OsWindowMacImpl::RegisterEvent(OsEventType type,
                                    void (*callback)(const OsEvent&, void*),
                                    void* userdata)
{
    EventHandler handler = {type, callback, userdata};
    m_eventHandlers.push_back(handler);
}

void OsWindowMacImpl::UnregisterEvent(OsEventType type,
                                      void (*callback)(const OsEvent&, void*),
                                      void* userdata)
{
    for (size_t i = 0; i < m_eventHandlers.size(); ++i) {
        const EventHandler& h = m_eventHandlers[i];
        if (h.type == type && h.callback == callback && h.userdata == userdata) {
            m_eventHandlers.erase(m_eventHandlers.begin() + i);
            break;
        }
    }
}

OsWindow::OsHandle* OsWindowMacImpl::GetOsHandle() const
{
    NSView* view = m_view;
    return (OsWindow::OsHandle*)view;
}

void OsWindowMacImpl::DispatchEvents()
{
    OsEvent idleEvent;
    idleEvent.type = OSEVENT_IDLE;
    OsEvent paintEvent;
    paintEvent.type = OSEVENT_PAINT;

    while (!m_eventQueue.empty()) {
        DispatchEvent(m_eventQueue.front());
        m_eventQueue.pop();
    }

    DispatchEvent(idleEvent);
    DispatchEvent(paintEvent);
}

void OsWindowMacImpl::DispatchEvent(const OsEvent& event)
{
    for (size_t i = 0; i < m_eventHandlers.size(); ++i) {
        const EventHandler& h = m_eventHandlers[i];
        if (h.type == event.type)
            h.callback(event, h.userdata);
    }
}

void OsWindowMacImpl::EnqueueEvent(const OsEvent& event)
{
    m_eventQueue.push(event);
}

OsWindow::OsHandle* OsWindow::GetOsHandle() const { return Cast(this)->GetOsHandle(); }

void OsWindow::RunEventLoop()
{
    ASSERT(NSApp != nil);
    [NSApp run];
}

void OsWindow::QuitEventLoop()
{
    [NSApp terminate:nil];
}

OsWindow* OsWindow::Create(int width, int height, const OsWindowPixelFormat& pf)
{ return Cast(new OsWindowMacImpl(width, height, pf)); }

void OsWindow::Destroy(OsWindow* w) { delete Cast(w); }

void OsWindow::RegisterEvent(OsEventType type,
                             void (*callback)(const OsEvent&, void*),
                             void* userdata)
{ Cast(this)->RegisterEvent(type, callback, userdata); }

void OsWindow::UnregisterEvent(OsEventType type,
                               void (*callback)(const OsEvent&, void*),
                               void* userdata)
{ Cast(this)->RegisterEvent(type, callback, userdata); }

#endif // __APPLE__
