#ifndef COWBRATEN_UI_SAUER_CONSOLE_BRIDGE_H
#define COWBRATEN_UI_SAUER_CONSOLE_BRIDGE_H

namespace cowui
{
    namespace sauerconsole
    {
        bool active();
        int skip();
        int filter();
        int maxlines();
        void scroll(int lines);
        int tagwindow();
        bool allowed(int type);
        bool tagged(int type);
        bool sametag(int first, int second);

        bool inputactive();
        const char *inputtext();
        int inputposition();
        const char *inputlabel();

        bool warning(int type);
        bool error(int type);
        bool chat(int type);
        bool teamchat(int type);
        bool echo(int type);
        bool gameinfo(int type);
    }
}

#endif
