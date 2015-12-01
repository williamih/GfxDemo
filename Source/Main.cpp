#include "OsWindow.h"
#include "Application.h"

#ifdef __APPLE__
#  include "Mac/MacApplication.h"
#endif

static void OnQuit(void* userdata)
{
    Application* app = *(Application**)userdata;
    delete app;
    OsWindow::QuitEventLoop();
}

int main()
{
    Application* app;

#ifdef __APPLE__
    MacApplication::Initialize();
    MacApplication::RegisterOnQuit(OnQuit, (void*)&app);
#endif

    app = new Application;

    OsWindow::RunEventLoop();

    return 0;
}
