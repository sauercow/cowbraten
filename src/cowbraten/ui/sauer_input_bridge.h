#ifndef COWBRATEN_UI_SAUER_INPUT_BRIDGE_H
#define COWBRATEN_UI_SAUER_INPUT_BRIDGE_H

union SDL_Event;

namespace cowui
{
    namespace sauerinput
    {
        void beginmenu();
        void endmenu();
        bool canclosemenu();

        void keyrepeat(bool enabled);
        void textinput(bool enabled);

        bool commandlineactive();
        bool fullconsoleactive();
        bool openconsolefromkey(int key);
        void quit();

        // Borrowed canonical keymap name, or nullptr for unmapped input.
        const char *capturekeyname(const SDL_Event &event);
    }
}

#endif
