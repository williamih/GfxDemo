#include "OsWindow.h"
#include "Application.h"

#ifdef __APPLE__
#  include "Mac/MacApplication.h"
#endif

static void OnQuit(void*)
{
    OsWindow::QuitEventLoop();
}

int main()
{
#ifdef __APPLE__
    MacApplication::Initialize();
    MacApplication::RegisterOnQuit(OnQuit, NULL);
#endif

    Application app;
    OsWindow::RunEventLoop();

    return 0;
}
