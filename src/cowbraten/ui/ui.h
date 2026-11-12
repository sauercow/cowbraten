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
        Demos,
        Settings
    };

    // Window dimensions define logical UI density and SDL input coordinates;
    // drawable dimensions define the RmlUi framebuffer and rendering space.
    bool init(
        int window_width,
        int window_height,
        int drawable_width,
        int drawable_height);
    void shutdown();

    void resize(
        int window_width,
        int window_height,
        int drawable_width,
        int drawable_height);

    void beforeglreset();
    void afterglreset(
        int window_width,
        int window_height,
        int drawable_width,
        int drawable_height);

    // Route key/button/text events even while hidden to drain captured releases.
    bool processevent(const SDL_Event &event);

    void update();
    void render();

    bool show(ScreenId screen);
    bool tryroutelegacygui(const char *name);
    void showmain();
    void showservers();
    void showdemos();
    void showdemosrecording();
    void showdemosplayback();
    void showdemostrash();
    void showdemosduels();
    void filterdemos(const char *value);
    void selectfirstdemo();
    bool playselecteddemo();
    void showsettings();
    void showtest();
    void close();
    // The modal session survives temporary console/chat ownership.
    bool modalactive();
    // RmlUi receives events only while the console is not taking its turn.
    bool receivesinput();
    // The fullscreen demo overlay draws the legacy cursor texture itself so
    // it remains visible above RmlUi without also drawing the engine copy.
    bool ownsplaybackcursor();
    bool visible();
}

#endif
