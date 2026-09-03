#ifndef COWBRATEN_UI_SAUER_SETTINGS_BRIDGE_H
#define COWBRATEN_UI_SAUER_SETTINGS_BRIDGE_H

namespace cowui
{
    namespace sauersettings
    {
        struct Fov
        {
            int value;
            int minimum;
            int maximum;
        };

        bool readfov(Fov &result);
        bool writefov(int value);
    }
}

#endif
