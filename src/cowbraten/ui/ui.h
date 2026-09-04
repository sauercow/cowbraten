#ifndef COWBRATEN_UI_H
#define COWBRATEN_UI_H

union SDL_Event;

namespace cowui
{
    enum class ScreenId
    {
        Test,
        Main,
        Servers,
        Settings
    };

    bool init(int width, int height);
    void shutdown();

    void resize(int width, int height);

    void beforeglreset();
    void afterglreset(int width, int height);

    // Route key/button/text events even while hidden to drain captured releases.
    bool processevent(const SDL_Event &event);

    void update();
    void render();

    bool show(ScreenId screen);
    bool tryroutelegacygui(const char *name);
    void showmain();
    void showservers();
    void showsettings();
    void showtest();
    void close();
    bool visible();
}

#endif
