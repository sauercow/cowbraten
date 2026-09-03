#ifndef COWBRATEN_UI_H
#define COWBRATEN_UI_H

union SDL_Event;

namespace cowui
{
    bool init(int width, int height);
    void shutdown();

    void resize(int width, int height);

    void beforeglreset();
    void afterglreset(int width, int height);

    bool processevent(const SDL_Event &event);

    void update();
    void render();

    void showtest();
    void close();
    bool visible();
}

#endif
