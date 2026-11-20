#ifndef COWBRATEN_UI_H
#define COWBRATEN_UI_H

union SDL_Event;

namespace Rml
{
    class ElementDocument;
}

namespace cowui
{
    struct FrameStats
    {
        double model_update_us = 0.0;
        double context_update_us = 0.0;
        double context_render_us = 0.0;
        unsigned int draw_calls = 0;
        unsigned int geometry_compiles = 0;
        unsigned int geometry_releases = 0;
        unsigned int texture_binds = 0;
        unsigned int uniform_updates = 0;
        unsigned int vertex_array_binds = 0;
        unsigned int scissor_changes = 0;
        unsigned int live_geometries = 0;
        unsigned int live_textures = 0;
        unsigned long long geometry_cpu_bytes = 0;
        unsigned long long texture_cpu_bytes = 0;
        int documents = 0;
        int visible_documents = 0;
    };

    enum class ScreenId
    {
        Test,
        GameMenu,
        Games,
        System,
        Utilities,
        Editor,
        Servers,
        Demos,
        Settings,
        Profile
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

    // Blocking engine loads cannot wait for the normal frame loop. These
    // entry points render the cached loading document synchronously and return
    // false whenever the original OpenGL loading screen must be used.
    bool renderloadingbackground(
        const char *caption,
        const char *mapshot,
        const char *mapname,
        const char *mapinfo);
    bool renderloadingprogress(
        float value,
        const char *text,
        bool full_background);

    // Route key/button/text events even while hidden to drain captured releases.
    bool processevent(const SDL_Event &event);

    void setprofilingenabled(bool enabled);
    void beginframestats();
    void update();
    void render();
    FrameStats framestats();

    bool show(ScreenId screen);
    bool tryroutelegacygui(const char *name);
    void showmain();
    void showfullmenu();
    void showgames();
    void showadmin();
    void showabout();
    void showutilities();
    void showscratchpad();
    void showpastebuffer();
    void showeditor();
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
    void showprofile();
    void showtest();
    void settheme(const char *name);
    bool applytheme(Rml::ElementDocument *document);
    void applyinterfacepreferences(Rml::ElementDocument *document);
    void interfacepreferenceschanged();
    void close();
    // The modal session survives temporary console/chat ownership.
    bool modalactive();
    // RmlUi receives events only while the console is not taking its turn.
    bool receivesinput();
    // The fullscreen demo overlay draws the legacy cursor texture itself so
    // it remains visible above RmlUi without also drawing the engine copy.
    bool ownsplaybackcursor();
    // Returns false when RmlUi is unavailable so gameplay can retain the
    // legacy g3d scoreboard as a safe fallback.
    bool setscoreboardvisible(bool value);
    void consoleline(int type, const char *text);
    void clearconsole();
    int filterconsole(int filter);
    bool chatinputvisible();
    bool fullconsolevisible();
    bool visible();
}

#endif
