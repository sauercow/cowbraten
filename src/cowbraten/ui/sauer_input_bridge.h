#ifndef COWBRATEN_UI_SAUER_INPUT_BRIDGE_H
#define COWBRATEN_UI_SAUER_INPUT_BRIDGE_H

union SDL_Event;

namespace cowui
{
    namespace sauerinput
    {
        void beginmenu();
        void endmenu();

        void keyrepeat(bool enabled);
        void textinput(bool enabled);

        bool consoleactive();
        bool openconsolefromkey(int key);

        // Borrowed canonical keymap name, or nullptr for unmapped input.
        const char *capturekeyname(const SDL_Event &event);
    }
}

#endif
