#ifndef MAC_MACAPPLICATION_H
#define MAC_MACAPPLICATION_H

class MacApplication {
public:
    static void Initialize();

    static void RegisterOnQuit(void (*callback)(void*), void* userdata);

private:
    MacApplication();
    ~MacApplication();
};

#endif // MAC_MACAPPLICATION_H
