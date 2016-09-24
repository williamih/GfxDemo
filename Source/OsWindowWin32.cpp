#ifdef _WIN32

#include "OsWindow.h"

#include <vector>
#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

const LPCWSTR WIN32_WINDOW_CLASS_NAME = L"OsWindow_Win32_WindowClass";

class OsWindowWin32Impl {
public:
    OsWindowWin32Impl(int width, int height, const OsWindowPixelFormat& pf);
    ~OsWindowWin32Impl();

    void RegisterEvent(OsEventType type,
                       void (*callback)(const OsEvent&, void*),
                       void* userdata);
    void UnregisterEvent(OsEventType type,
                         void (*callback)(const OsEvent&, void*),
                         void* userdata);

    OsWindow::OsHandle* GetOsHandle() const;

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

private:
    OsWindowWin32Impl(const OsWindowWin32Impl&);
    OsWindowWin32Impl& operator=(const OsWindowWin32Impl&);

    struct EventHandler {
        OsEventType type;
        void (*callback)(const OsEvent&, void*);
        void* userdata;
    };

    HWND m_hWnd;
    std::vector<EventHandler> m_eventHandlers;
};

static ATOM OsWindowRegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = &OsWindowWin32Impl::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = WIN32_WINDOW_CLASS_NAME;
    wcex.hIconSm = nullptr;

    return RegisterClassExW(&wcex);
}

OsWindowWin32Impl::OsWindowWin32Impl(int width, int height, const OsWindowPixelFormat& pf)
    : m_hWnd(nullptr)
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    ATOM windowClass = OsWindowRegisterWindowClass(hInstance);

    HWND hWnd = CreateWindowW(WIN32_WINDOW_CLASS_NAME, L"", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    assert(hWnd != nullptr && "CreateWindowW failed");
    m_hWnd = hWnd;

    SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)this);

    ShowWindow(hWnd, SW_SHOW); // TODO: Use the proper value of nCmdShow
    UpdateWindow(hWnd);
}

OsWindowWin32Impl::~OsWindowWin32Impl()
{
    DestroyWindow(m_hWnd);
}

void OsWindowWin32Impl::RegisterEvent(OsEventType type,
                                      void (*callback)(const OsEvent&, void*),
                                      void* userdata)
{
    EventHandler handler = {type, callback, userdata};
    m_eventHandlers.push_back(handler);
}

void OsWindowWin32Impl::UnregisterEvent(OsEventType type,
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

OsWindow::OsHandle* OsWindowWin32Impl::GetOsHandle() const
{
    return (OsWindow::OsHandle*)m_hWnd;
}

LRESULT CALLBACK OsWindowWin32Impl::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LONG_PTR windowUserLong = GetWindowLongPtr(hWnd, DWLP_USER);
    OsWindowWin32Impl* self = nullptr;
    if (windowUserLong != 0)
        OsWindowWin32Impl* self = (OsWindowWin32Impl*)windowUserLong;

    HINSTANCE hInst = GetModuleHandle(nullptr);

    switch (message) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

static const OsWindowWin32Impl* Cast(const OsWindow* w) { return (const OsWindowWin32Impl*)w; }
static OsWindowWin32Impl* Cast(OsWindow* w) { return (OsWindowWin32Impl*)w; }
static OsWindow* Cast(OsWindowWin32Impl* w) { return (OsWindow*)w; }

OsWindow* OsWindow::Create(int width, int height, const OsWindowPixelFormat& pf)
{
    return Cast(new OsWindowWin32Impl(width, height, pf));
}

void OsWindow::Destroy(OsWindow* w) { delete Cast(w); }

void OsWindow::RegisterEvent(OsEventType type,
                             void (*callback)(const OsEvent&, void*),
                             void* userdata)
{ Cast(this)->RegisterEvent(type, callback, userdata); }

void OsWindow::UnregisterEvent(OsEventType type,
                               void (*callback)(const OsEvent&, void*),
                               void* userdata)
{ Cast(this)->RegisterEvent(type, callback, userdata); }

OsWindow::OsHandle* OsWindow::GetOsHandle() const
{ return Cast(this)->GetOsHandle(); }

void OsWindow::RunEventLoop()
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // TODO: Actually return this
    // return (int)msg.wParam;
}

void OsWindow::QuitEventLoop()
{
    PostQuitMessage(0);
}

#endif // _WIN32
