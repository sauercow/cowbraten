#ifndef COWBRATEN_UI_SETTINGS_API_H
#define COWBRATEN_UI_SETTINGS_API_H

namespace cowui
{
    namespace settings
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
