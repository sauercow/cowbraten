#include "engine.h"

#include "sauer_settings_bridge.h"

#include <cstdint>
#include <cstring>

extern int scr_w, scr_h, fullscreendesktop;
extern bool nosound;
extern void resetgl();
extern void screenres(int width, int height);

namespace cowui
{
    namespace sauersettings
    {
        static bool finitefloat(float value)
        {
            static_assert(
                sizeof(float) == sizeof(std::uint32_t),
                "Cowbraten settings require 32-bit IEEE-754 floats"
            );
            std::uint32_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            return (bits & 0x7F800000u) != 0x7F800000u;
        }

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

        static ident *desktopfullscreenident()
        {
            ident *id = getident("fullscreendesktop");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *vsyncident()
        {
            ident *id = getident("vsync");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *maxfpsident()
        {
            ident *id = getident("maxfps");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *fsaaident()
        {
            ident *id = getident("fsaa");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *soundvolumeident()
        {
            ident *id = getident("soundvol");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *musicvolumeident()
        {
            ident *id = getident("musicvol");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *sensitivityident()
        {
            ident *id = getident("sensitivity");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *invertedmouseident()
        {
            ident *id = getident("invmouse");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static bool validantialiasing(int samples)
        {
            switch(samples)
            {
                case 0:
                case 2:
                case 4:
                case 8:
                case 16:
                    return true;
                default:
                    return false;
            }
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
            if(screen)
            {
                const Uint32 flags = SDL_GetWindowFlags(screen);
                if((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ==
                   SDL_WINDOW_FULLSCREEN_DESKTOP)
                {
                    result.presentation =
                        PresentationMode::DesktopFullscreen;
                }
                else if(flags & SDL_WINDOW_FULLSCREEN)
                {
                    result.presentation =
                        PresentationMode::ExclusiveFullscreen;
                }
                else result.presentation = PresentationMode::Windowed;
            }
            else if(*id->storage.i)
            {
                result.presentation = ::fullscreendesktop
                    ? PresentationMode::DesktopFullscreen
                    : PresentationMode::ExclusiveFullscreen;
            }
            else result.presentation = PresentationMode::Windowed;
            return true;
        }

        bool writepresentation(PresentationMode mode)
        {
            ident *fullscreen_id = fullscreenident();
            ident *desktop_id = desktopfullscreenident();
            if(!fullscreen_id || !fullscreen_id->storage.i ||
               !desktop_id || !desktop_id->storage.i)
            {
                return false;
            }

            Video previous;
            if(!readvideo(previous))
                return false;

            const bool was_fullscreen = previous.presentation !=
                PresentationMode::Windowed;
            switch(mode)
            {
                case PresentationMode::Windowed:
                    if(was_fullscreen)
                        setvarchecked(fullscreen_id, 0);
                    break;

                case PresentationMode::ExclusiveFullscreen:
                    if(*desktop_id->storage.i)
                        setvarchecked(desktop_id, 0);
                    if(!was_fullscreen)
                        setvarchecked(fullscreen_id, 1);
                    break;

                case PresentationMode::DesktopFullscreen:
                    if(!*desktop_id->storage.i)
                        setvarchecked(desktop_id, 1);
                    if(!was_fullscreen)
                        setvarchecked(fullscreen_id, 1);
                    break;
            }

            Video current;
            return readvideo(current) && current.presentation == mode;
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

        bool readvsync(Vsync &result)
        {
            ident *id = vsyncident();
            if(!id || !id->storage.i || !screen)
                return false;

            const int interval = SDL_GL_GetSwapInterval();
            result.enabled = interval != 0;
            result.adaptive = interval < 0;
            return true;
        }

        bool writevsync(bool enabled)
        {
            ident *id = vsyncident();
            if(!id || !id->storage.i)
                return false;

            setvarchecked(id, enabled ? 1 : 0);
            Vsync current;
            return readvsync(current) && current.enabled == enabled;
        }

        bool readfpslimit(FpsLimit &result)
        {
            ident *id = maxfpsident();
            if(!id || !id->storage.i)
                return false;

            result.value = *id->storage.i;
            result.minimum = id->minval;
            result.maximum = id->maxval;
            return true;
        }

        bool writefpslimit(int value)
        {
            ident *id = maxfpsident();
            if(!id || !id->storage.i ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            setvarchecked(id, value);
            return *id->storage.i == value;
        }

        bool readantialiasing(Antialiasing &result)
        {
            ident *id = fsaaident();
            if(!id || !id->storage.i || !screen)
                return false;

            int buffers = 0, samples = 0;
            if(SDL_GL_GetAttribute(
                   SDL_GL_MULTISAMPLEBUFFERS,
                   &buffers) < 0 ||
               SDL_GL_GetAttribute(
                   SDL_GL_MULTISAMPLESAMPLES,
                   &samples) < 0)
            {
                return false;
            }

            result.requested_samples = *id->storage.i;
            result.actual_samples = buffers > 0 ? max(samples, 0) : 0;
            return true;
        }

        bool writeantialiasing(int samples)
        {
            ident *id = fsaaident();
            if(!id || !id->storage.i ||
               !validantialiasing(samples))
            {
                return false;
            }

            if(*id->storage.i != samples)
                setvarchecked(id, samples);
            return *id->storage.i == samples;
        }

        void resetgraphics()
        {
            ::resetgl();
        }

        bool readaudio(Audio &result)
        {
            ident *sound_id = soundvolumeident();
            ident *music_id = musicvolumeident();
            if(!sound_id || !sound_id->storage.i ||
               !music_id || !music_id->storage.i)
            {
                return false;
            }

            result.sound_volume = *sound_id->storage.i;
            result.sound_volume_minimum = sound_id->minval;
            result.sound_volume_maximum = sound_id->maxval;
            result.music_volume = *music_id->storage.i;
            result.music_volume_minimum = music_id->minval;
            result.music_volume_maximum = music_id->maxval;
            result.backend_active = !::nosound;
            return true;
        }

        bool writesoundvolume(int value)
        {
            ident *id = soundvolumeident();
            if(!id || !id->storage.i ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            setvarchecked(id, value);
            return *id->storage.i == value;
        }

        bool writemusicvolume(int value)
        {
            ident *id = musicvolumeident();
            if(!id || !id->storage.i ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            setvarchecked(id, value);
            return *id->storage.i == value;
        }

        bool readmouseinput(MouseInput &result)
        {
            ident *sensitivity_id = sensitivityident();
            ident *inverted_id = invertedmouseident();
            if(!sensitivity_id || !sensitivity_id->storage.f ||
               !inverted_id || !inverted_id->storage.i)
            {
                return false;
            }

            result.sensitivity = *sensitivity_id->storage.f;
            result.sensitivity_minimum = sensitivity_id->minvalf;
            result.sensitivity_maximum = sensitivity_id->maxvalf;
            result.inverted = *inverted_id->storage.i != 0;
            return true;
        }

        bool writesensitivity(float value)
        {
            ident *id = sensitivityident();
            if(!id || !id->storage.f || !finitefloat(value) ||
               value < id->minvalf || value > id->maxvalf)
            {
                return false;
            }

            setfvarchecked(id, value);
            return *id->storage.f == value;
        }

        bool writeinvertedmouse(bool enabled)
        {
            ident *id = invertedmouseident();
            if(!id || !id->storage.i)
                return false;

            setvarchecked(id, enabled ? 1 : 0);
            return (*id->storage.i != 0) == enabled;
        }
    }
}
