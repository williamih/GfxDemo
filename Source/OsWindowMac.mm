#ifdef __APPLE__

#include "OsWindow.h"
#include <vector>
#include <queue>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#include "Macros.h"

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

    void* GetNSView() const;

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

void* OsWindowMacImpl::GetNSView() const
{
    NSView* view = m_view;
    return (void*)view;
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

void* OsWindow::GetNSView() const { return Cast(this)->GetNSView(); }

#endif // __APPLE__
