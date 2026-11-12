#include "engine.h"

#include "sauer_input_bridge.h"

extern void setinputgrabinhibited(bool inhibit);
extern void quit();

namespace cowui
{
    namespace sauerinput
    {
        void beginmenu()
        {
            releasepressedbinds();
            setinputgrabinhibited(true);
        }

        void endmenu()
        {
            setinputgrabinhibited(false);
        }

        bool canclosemenu()
        {
            return isconnected(true);
        }

        void keyrepeat(bool enabled)
        {
            ::keyrepeat(enabled, KR_COWUI);
        }

        void textinput(bool enabled)
        {
            ::textinput(enabled, TI_COWUI);
        }

        bool commandlineactive()
        {
            return ::commandlineactive();
        }

        bool fullconsoleactive()
        {
            return fullconsole != 0;
        }

        bool openconsolefromkey(int key)
        {
            return processconsolebind(key);
        }

        void quit()
        {
            ::quit();
        }

        const char *capturekeyname(const SDL_Event &event)
        {
            int code = 0;
            switch(event.type)
            {
                case SDL_KEYDOWN:
                    if(event.key.repeat || event.key.keysym.sym == SDLK_ESCAPE)
                        return nullptr;
                    // Match checkinput/processkey: key identity, never text.
                    code = event.key.keysym.sym;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    // Keep aligned with checkinput in engine/main.cpp.
                    switch(event.button.button)
                    {
                        case SDL_BUTTON_LEFT: code = -1; break;
                        case SDL_BUTTON_MIDDLE: code = -2; break;
                        case SDL_BUTTON_RIGHT: code = -3; break;
                        case SDL_BUTTON_X1: code = -6; break;
                        case SDL_BUTTON_X2: code = -7; break;
                        case SDL_BUTTON_X2 + 1: code = -10; break;
                        case SDL_BUTTON_X2 + 2: code = -11; break;
                        case SDL_BUTTON_X2 + 3: code = -12; break;
                        default: return nullptr;
                    }
                    break;
                case SDL_MOUSEWHEEL:
                    // Sauer prioritizes vertical motion and uses raw SDL signs.
                    if(event.wheel.y > 0) code = -4;
                    else if(event.wheel.y < 0) code = -5;
                    else if(event.wheel.x > 0) code = -8;
                    else if(event.wheel.x < 0) code = -9;
                    else return nullptr;
                    break;
                default: return nullptr;
            }
            return getkeyname(code);
        }
    }
}
