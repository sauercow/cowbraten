#ifndef COWBRATEN_UI_SAUER_CHAT_BRIDGE_H
#define COWBRATEN_UI_SAUER_CHAT_BRIDGE_H

namespace cowui
{
    namespace sauerchat
    {
        enum class InputMode
        {
            None,
            Public,
            Team
        };

        InputMode inputmode();
        const char *inputtext();
        int inputposition();
        int now();
        bool classifyline(int type, bool &team);
        bool topenabled(int type);
        bool minienabled(int type);
        int topfade();
        int minifade();
        int topcapacity();
        int minicapacity();
        bool topfeedminimapclipped();
    }
}

#endif
