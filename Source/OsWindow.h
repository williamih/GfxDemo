#ifndef OSWINDOW_H
#define OSWINDOW_H

#include "OsEvent.h"

struct OsWindowPixelFormat {
    unsigned colorBits;
    unsigned depthBits;
};

class OsWindow {
public:
    struct OsHandle;

    static OsWindow* Create(int width, int height, const OsWindowPixelFormat& pf);
    static void Destroy(OsWindow* window);

    void RegisterEvent(OsEventType type,
                       void (*callback)(const OsEvent&, void*),
                       void* userdata);
    void UnregisterEvent(OsEventType type,
                         void (*callback)(const OsEvent&, void*),
                         void* userdata);

    // Returns an NSView* for Mac, or an HWND for Windows.
    OsHandle* GetOsHandle() const;

    static void RunEventLoop();
    static void QuitEventLoop();

private:
    OsWindow();
    ~OsWindow();
    OsWindow(const OsWindow&);
    OsWindow& operator=(const OsWindow&);
};

#endif // OSWINDOW_H
