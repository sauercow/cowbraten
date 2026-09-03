#include "engine.h"

#include "sauer_settings_bridge.h"

extern int scr_w, scr_h, fullscreendesktop;
extern void screenres(int width, int height);

namespace cowui
{
    namespace sauersettings
    {
        static ident *fovident()
        {
            ident *id = getident("fov");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *fullscreenident()
        {
            ident *id = getident("fullscreen");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        bool readfov(Fov &result)
        {
            ident *id = fovident();
            if(!id || !id->storage.i)
                return false;

            result.value = *id->storage.i;
            result.minimum = id->minval;
            result.maximum = id->maxval;
            return true;
        }

        bool writefov(int value)
        {
            ident *id = fovident();
            if(!id || !id->storage.i)
                return false;

            setvarchecked(id, value);
            return true;
        }

        bool readvideo(Video &result)
        {
            ident *id = fullscreenident();
            if(!id || !id->storage.i)
                return false;

            if(screen)
                SDL_GetWindowSize(screen, &result.width, &result.height);
            else
            {
                result.width = scr_w;
                result.height = scr_h;
            }
            result.fullscreen = screen
                ? (SDL_GetWindowFlags(screen) & SDL_WINDOW_FULLSCREEN) != 0
                : *id->storage.i != 0;
            result.desktop_fullscreen = ::fullscreendesktop != 0;
            return true;
        }

        bool writefullscreen(bool enabled)
        {
            ident *id = fullscreenident();
            if(!id || !id->storage.i)
                return false;

            setvarchecked(id, enabled ? 1 : 0);

            Video current;
            return readvideo(current) && current.fullscreen == enabled;
        }

        bool writeresolution(int width, int height)
        {
            if(width <= 0 || height <= 0)
                return false;

            screenres(width, height);
            Video current;
            return readvideo(current) &&
                current.width == width &&
                current.height == height;
        }

        bool visitresolutions(ResolutionVisitor visitor, void *data)
        {
            if(!visitor || !screen)
                return false;

            int display = SDL_GetWindowDisplayIndex(screen);
            if(display < 0)
                return false;

            const int count = SDL_GetNumDisplayModes(display);
            if(count < 0)
                return false;

            loopi(count)
            {
                SDL_DisplayMode mode;
                if(SDL_GetDisplayMode(display, i, &mode) < 0)
                    continue;

                Resolution resolution;
                resolution.width = mode.w;
                resolution.height = mode.h;
                visitor(resolution, data);
            }
            return true;
        }
    }
}
