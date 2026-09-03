#ifndef COWBRATEN_UI_SAUER_INPUT_BRIDGE_H
#define COWBRATEN_UI_SAUER_INPUT_BRIDGE_H

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
    }
}

#endif
