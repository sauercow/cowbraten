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

        struct Video
        {
            int width;
            int height;
            bool fullscreen;
            bool desktop_fullscreen;
        };

        struct Resolution
        {
            int width;
            int height;
        };

        typedef void (*ResolutionVisitor)(
            const Resolution &resolution,
            void *data
        );

        bool readfov(Fov &result);
        bool writefov(int value);

        bool readvideo(Video &result);
        bool writefullscreen(bool enabled);
        bool writeresolution(int width, int height);
        bool visitresolutions(ResolutionVisitor visitor, void *data);
    }
}

#endif
