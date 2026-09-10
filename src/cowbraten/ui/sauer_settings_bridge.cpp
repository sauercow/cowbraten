#include "engine.h"
#include "game.h"

#include "sauer_settings_bridge.h"

#include <cstdint>
#include <cstring>

extern int scr_w, scr_h, fullscreendesktop;
extern int grass, motionblur;
extern bool nosound;
extern void resetgl();
extern void screenres(int width, int height);
extern void loadcrosshair(const char *name, int index);
extern const char *getcrosshairname(int index);

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

        static ident *soundpackident()
        {
            ident *id = getident("soundpack");
            return id && id->type == ID_SVAR
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

        static ident *crosshairsizeident()
        {
            ident *id = getident("crosshairsize");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *crosshaireffectsident()
        {
            ident *id = getident("crosshairfx");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *crosshairhealthcolorsident()
        {
            ident *id = getident("crosshaircolors");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *teamcrosshairident()
        {
            ident *id = getident("teamcrosshair");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hitcrosshairident()
        {
            ident *id = getident("hitcrosshair");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *crosshairreloadfadeident()
        {
            ident *id = getident("crosshairreloadfade");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *crosshairreloadrotationident()
        {
            ident *id = getident("crosshairreloadrotate");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *damagescreenident()
        {
            ident *id = getident("damagescreen");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *damagecompassident()
        {
            ident *id = getident("damagecompass");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *fragmessagesident()
        {
            ident *id = getident("hudfragmessages");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *streakmilestonescaleident()
        {
            ident *id = getident("streakmilestonescale");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *consolefilterident()
        {
            ident *id = getident("confilter");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *miniconsolefilterident()
        {
            ident *id = getident("miniconfilter");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *teamcolorchatident()
        {
            ident *id = getident("teamcolorchat");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        struct CrosshairCatalogEntry
        {
            const char *label;
            const char *normal;
            const char *hit;
        };

        static const CrosshairCatalogEntry crosshair_catalog[] =
        {
            { "Classic", "data/crosshair.png", "data/hit.png" },
            { "X", "packages/crosshairs/x.png", "packages/crosshairs/x-hit.png" },
            { "X Dot", "packages/crosshairs/x_dot.png", "packages/crosshairs/x_dot-hit.png" },
            { "Circle", "packages/crosshairs/o.png", "packages/crosshairs/o-hit.png" },
            { "Circle X", "packages/crosshairs/o_x.png", "packages/crosshairs/o_x-hit.png" },
            { "Circle Dot", "packages/crosshairs/o_dot.png", "packages/crosshairs/o_dot-hit.png" },
            { "Dot", "packages/crosshairs/dot.png", "packages/crosshairs/dot-hit.png" },
            { "Wide Dot", "packages/crosshairs/dot_wide.png", "packages/crosshairs/dot_wide-hit.png" },
            { "Star", "packages/crosshairs/star.png", "packages/crosshairs/star-hit.png" },
            { "Wide", "packages/crosshairs/wide.png", "packages/crosshairs/wide-hit.png" },
            { "Circle Dot II", "packages/crosshairs/circle_dot.png", "packages/crosshairs/circle_dot_hit.png" },
            { "Cross Normal", "packages/crosshairs/cross_normal.png", "packages/crosshairs/cross_normal_hit.png" },
            { "Cross Whole", "packages/crosshairs/cross_whole.png", "packages/crosshairs/cross_whole_hit.png" },
            { "Dot Normal", "packages/crosshairs/dot_normal.png", "packages/crosshairs/dot_normal_hit.png" },
            { "Dot Whole", "packages/crosshairs/dot_whole.png", "packages/crosshairs/dot_whole_hit.png" },
            { "Dot II", "packages/crosshairs/dot2.png", "packages/crosshairs/dot2_hit.png" }
        };

        static bool resourceavailable(const char *path)
        {
            stream *file = openfile(path, "rb");
            if(!file)
                return false;

            delete file;
            return true;
        }

        static bool crosshairavailable(const CrosshairCatalogEntry &choice)
        {
            return resourceavailable(choice.normal) &&
                resourceavailable(choice.hit);
        }

        static int selectedcrosshair()
        {
            const char *current = getcrosshairname(0);
            loopi(int(sizeof(crosshair_catalog) / sizeof(crosshair_catalog[0])))
            {
                if(!std::strcmp(current, crosshair_catalog[i].normal))
                    return i;
            }
            return -1;
        }

        static bool writeboolean(ident *id, bool enabled)
        {
            if(!id || !id->storage.i)
                return false;

            setvarchecked(id, enabled ? 1 : 0);
            return (*id->storage.i != 0) == enabled;
        }

        static bool writefilterbit(ident *id, int bit, bool enabled)
        {
            if(!id || !id->storage.i || bit <= 0)
                return false;

            const int current = *id->storage.i;
            const int value = enabled
                ? current | bit
                : current & ~bit;
            if(value < id->minval || value > id->maxval)
                return false;

            if(value != current)
                setvarchecked(id, value);
            return ((*id->storage.i & bit) != 0) == enabled;
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
            {
                getwindowdimensions(result.width, result.height);
                getdrawabledimensions(
                    result.drawable_width,
                    result.drawable_height
                );
            }
            else
            {
                result.width = scr_w;
                result.height = scr_h;
                result.drawable_width = result.width;
                result.drawable_height = result.height;
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

        bool readgraphicsquality(GraphicsQuality &result)
        {
            result.grass = grass != 0;
            // Match the addmotionblur resolution gate.
            // Map content, pause state and effect strength remain native behavior.
            int drawablewidth, drawableheight;
            getdrawabledimensions(drawablewidth, drawableheight);
            result.motion_blur_available = drawablewidth > 0 && drawableheight > 0 &&
                max(drawablewidth, drawableheight) <= hwtexsize;
            result.motion_blur = motionblur != 0 && result.motion_blur_available;
            ident *glare_id = getident("glare");
            if(!glare_id || glare_id->type != ID_VAR || !glare_id->storage.i)
                return false;
            result.glare = *glare_id->storage.i != 0;
            return true;
        }

        bool writegrass(bool enabled)
        {
            ident *id = getident("grass");
            if(!id || id->type != ID_VAR || !writeboolean(id, enabled))
                return false;
            return *id->storage.i == (enabled ? 1 : 0);
        }

        bool writemotionblur(bool enabled)
        {
            GraphicsQuality current;
            if(!readgraphicsquality(current) || (enabled && !current.motion_blur_available))
                return false;

            ident *id = getident("motionblur");
            // setvarchecked runs the native cleanup callback when disabling.
            if(!id || id->type != ID_VAR || !writeboolean(id, enabled))
                return false;
            return readgraphicsquality(current) && current.motion_blur == enabled;
        }

        bool writeglare(bool enabled)
        {
            return writeboolean(getident("glare"), enabled);
        }

        bool readshadows(ShadowState &result)
        {
            result.shadow_maps = false;
            result.blob_shadows = false;
            result.shadow_maps_available = false;

            ident *shadowmap_id = getident("shadowmap");
            ident *blobs_id = getident("blobs");
            if(!shadowmap_id || shadowmap_id->type != ID_VAR ||
               !shadowmap_id->storage.i ||
               !blobs_id || blobs_id->type != ID_VAR ||
               !blobs_id->storage.i)
            {
                return false;
            }

            result.shadow_maps = *shadowmap_id->storage.i != 0;
            result.blob_shadows = *blobs_id->storage.i != 0;
            result.shadow_maps_available = hasFBO;
            return true;
        }

        bool writeshadowmaps(bool enabled, ShadowState &result)
        {
            if(!readshadows(result) ||
               (enabled && !result.shadow_maps_available))
            {
                return false;
            }

            ident *id = getident("shadowmap");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readshadows(result))
                return false;
            return result.shadow_maps == enabled;
        }

        bool writeblobshadows(bool enabled, ShadowState &result)
        {
            if(!readshadows(result))
                return false;

            ident *id = getident("blobs");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readshadows(result))
                return false;
            return result.blob_shadows == enabled;
        }

        static ShadowQuality classifyshadowquality(int map_size, int blur)
        {
            if(map_size == 9 && blur == 1)
                return ShadowQuality::Medium;
            if(map_size == 10 && blur == 2)
                return ShadowQuality::High;
            return ShadowQuality::Custom;
        }

        static bool shadowqualityvalues(
            ShadowQuality quality,
            int &map_size,
            int &blur)
        {
            switch(quality)
            {
                case ShadowQuality::Medium:
                    map_size = 9;
                    blur = 1;
                    return true;
                case ShadowQuality::High:
                    map_size = 10;
                    blur = 2;
                    return true;
                case ShadowQuality::Custom:
                    return false;
            }
            return false;
        }

        bool readshadowquality(ShadowQualityState &result)
        {
            result.quality = ShadowQuality::Custom;
            result.map_size = 0;
            result.blur = 0;

            ident *map_size_id = getident("shadowmapsize");
            ident *blur_id = getident("blurshadowmap");
            if(!map_size_id || map_size_id->type != ID_VAR ||
               !map_size_id->storage.i ||
               !blur_id || blur_id->type != ID_VAR || !blur_id->storage.i)
            {
                return false;
            }

            result.map_size = *map_size_id->storage.i;
            result.blur = *blur_id->storage.i;
            result.quality = classifyshadowquality(
                result.map_size, result.blur);
            return true;
        }

        bool writeshadowquality(
            ShadowQuality quality,
            ShadowQualityState &result)
        {
            if(!readshadowquality(result))
                return false;

            int map_size = 0, blur = 0;
            if(!shadowqualityvalues(quality, map_size, blur))
                return false;

            ident *map_size_id = getident("shadowmapsize");
            ident *blur_id = getident("blurshadowmap");
            if(!map_size_id || map_size_id->type != ID_VAR ||
               !map_size_id->storage.i || map_size_id->flags&IDF_READONLY ||
               map_size < map_size_id->minval ||
               map_size > map_size_id->maxval ||
               !blur_id || blur_id->type != ID_VAR || !blur_id->storage.i ||
               blur_id->flags&IDF_READONLY ||
               blur < blur_id->minval || blur > blur_id->maxval)
            {
                return false;
            }

            const bool change_map_size = *map_size_id->storage.i != map_size;
            const bool change_blur = *blur_id->storage.i != blur;
            if(change_map_size && !SDL_GL_GetCurrentContext())
                return false;

            if(change_blur)
                setvarchecked(blur_id, blur);
            if(change_map_size)
                setvarchecked(map_size_id, map_size);

            if(!readshadowquality(result))
                return false;
            return result.quality == quality &&
                result.map_size == map_size && result.blur == blur;
        }

        bool readwatersurfaceeffects(WaterSurfaceEffectsState &result)
        {
            result.refraction = false;
            result.reflections = false;
            result.caustics = false;
            result.liquid_animation = false;
            result.render_targets_available = false;

            ident *refraction_id = getident("waterrefract");
            ident *reflections_id = getident("waterreflect");
            ident *caustics_id = getident("caustics");
            ident *animation_id = getident("vertwater");
            if(!refraction_id || refraction_id->type != ID_VAR ||
               !refraction_id->storage.i ||
               !reflections_id || reflections_id->type != ID_VAR ||
               !reflections_id->storage.i ||
               !caustics_id || caustics_id->type != ID_VAR ||
               !caustics_id->storage.i ||
               !animation_id || animation_id->type != ID_VAR ||
               !animation_id->storage.i)
            {
                return false;
            }

            result.refraction = *refraction_id->storage.i != 0;
            result.reflections = *reflections_id->storage.i != 0;
            result.caustics = *caustics_id->storage.i != 0;
            result.liquid_animation = *animation_id->storage.i != 0;
            result.render_targets_available = hasFBO;
            return true;
        }

        bool writewaterrefraction(
            bool enabled,
            WaterSurfaceEffectsState &result)
        {
            if(!readwatersurfaceeffects(result))
                return false;

            ident *id = getident("waterrefract");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }
            if(enabled && !result.render_targets_available)
                return false;

            if(*id->storage.i != value)
            {
                if(!SDL_GL_GetCurrentContext())
                    return false;
                setvarchecked(id, value);
            }

            if(!readwatersurfaceeffects(result))
                return false;
            return result.refraction == enabled;
        }

        bool writewaterreflections(
            bool enabled,
            WaterSurfaceEffectsState &result)
        {
            if(!readwatersurfaceeffects(result))
                return false;

            ident *id = getident("waterreflect");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }
            if(enabled && !result.render_targets_available)
                return false;

            if(*id->storage.i != value)
            {
                if(!SDL_GL_GetCurrentContext())
                    return false;
                setvarchecked(id, value);
            }

            if(!readwatersurfaceeffects(result))
                return false;
            return result.reflections == enabled;
        }

        bool writecaustics(bool enabled, WaterSurfaceEffectsState &result)
        {
            if(!readwatersurfaceeffects(result))
                return false;

            ident *id = getident("caustics");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
            {
                if(enabled && !SDL_GL_GetCurrentContext())
                    return false;
                setvarchecked(id, value);
            }

            if(!readwatersurfaceeffects(result))
                return false;
            return result.caustics == enabled;
        }

        bool writeliquidanimation(
            bool enabled,
            WaterSurfaceEffectsState &result)
        {
            if(!readwatersurfaceeffects(result))
                return false;

            ident *id = getident("vertwater");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
            {
                if(!SDL_GL_GetCurrentContext())
                    return false;
                setvarchecked(id, value);
            }

            if(!readwatersurfaceeffects(result))
                return false;
            return result.liquid_animation == enabled;
        }

        bool readwaterfalleffects(WaterfallEffectsState &result)
        {
            result.refraction = false;
            result.reflections = false;
            result.render_targets_available = false;

            ident *refraction_id = getident("waterfallrefract");
            ident *reflections_id = getident("waterfallenv");
            if(!refraction_id || refraction_id->type != ID_VAR ||
               !refraction_id->storage.i ||
               !reflections_id || reflections_id->type != ID_VAR ||
               !reflections_id->storage.i)
            {
                return false;
            }

            result.refraction = *refraction_id->storage.i != 0;
            result.reflections = *reflections_id->storage.i != 0;
            result.render_targets_available = hasFBO;
            return true;
        }

        bool writewaterfallrefraction(
            bool enabled,
            WaterfallEffectsState &result)
        {
            if(!readwaterfalleffects(result))
                return false;

            ident *id = getident("waterfallrefract");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }
            if(enabled && !result.render_targets_available)
                return false;

            if(*id->storage.i != value)
            {
                if(!SDL_GL_GetCurrentContext())
                    return false;
                setvarchecked(id, value);
            }

            if(!readwaterfalleffects(result))
                return false;
            return result.refraction == enabled;
        }

        bool writewaterfallreflections(
            bool enabled,
            WaterfallEffectsState &result)
        {
            if(!readwaterfalleffects(result))
                return false;

            ident *id = getident("waterfallenv");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
            {
                if(!SDL_GL_GetCurrentContext())
                    return false;
                setvarchecked(id, value);
            }

            if(!readwaterfalleffects(result))
                return false;
            return result.reflections == enabled;
        }

        static DynamicLights classifydynamiclights(int max_lights)
        {
            switch(max_lights)
            {
                case 0: return DynamicLights::Off;
                case 3: return DynamicLights::Medium;
                case 5: return DynamicLights::High;
                default: return DynamicLights::Custom;
            }
        }

        static bool dynamiclightsvalue(DynamicLights setting, int &value)
        {
            switch(setting)
            {
                case DynamicLights::Off:
                    value = 0;
                    return true;
                case DynamicLights::Medium:
                    value = 3;
                    return true;
                case DynamicLights::High:
                    value = 5;
                    return true;
                case DynamicLights::Custom:
                    return false;
            }
            return false;
        }

        bool readdynamiclights(DynamicLightsState &result)
        {
            result.setting = DynamicLights::Custom;
            result.max_lights = 0;

            ident *id = getident("maxdynlights");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result.max_lights = *id->storage.i;
            result.setting = classifydynamiclights(result.max_lights);
            return true;
        }

        bool writedynamiclights(
            DynamicLights setting,
            DynamicLightsState &result)
        {
            if(!readdynamiclights(result))
                return false;

            int value = 0;
            if(!dynamiclightsvalue(setting, value))
                return false;

            ident *id = getident("maxdynlights");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readdynamiclights(result))
                return false;
            return result.setting == setting && result.max_lights == value;
        }

        bool readsoftparticles(SoftParticlesState &result)
        {
            result.enabled = false;

            ident *id = getident("depthfx");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result.enabled = *id->storage.i != 0;
            return true;
        }

        bool writesoftparticles(
            bool enabled,
            SoftParticlesState &result)
        {
            if(!readsoftparticles(result))
                return false;

            ident *id = getident("depthfx");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readsoftparticles(result))
                return false;
            return result.enabled == enabled;
        }

        static SoftParticleQuality classifysoftparticlequality(
            int depth_size,
            int blur)
        {
            if(depth_size == 7 && blur == 1)
                return SoftParticleQuality::Low;
            if(depth_size == 10 && blur == 0)
                return SoftParticleQuality::Medium;
            if(depth_size == 12 && blur == 0)
                return SoftParticleQuality::High;
            return SoftParticleQuality::Custom;
        }

        static bool softparticlequalityvalues(
            SoftParticleQuality quality,
            int &depth_size,
            int &blur)
        {
            switch(quality)
            {
                case SoftParticleQuality::Low:
                    depth_size = 7;
                    blur = 1;
                    return true;
                case SoftParticleQuality::Medium:
                    depth_size = 10;
                    blur = 0;
                    return true;
                case SoftParticleQuality::High:
                    depth_size = 12;
                    blur = 0;
                    return true;
                case SoftParticleQuality::Custom:
                    return false;
            }
            return false;
        }

        bool readsoftparticlequality(SoftParticleQualityState &result)
        {
            result.quality = SoftParticleQuality::Custom;
            result.depth_size = 0;
            result.blur = 0;

            ident *size_id = getident("depthfxsize");
            ident *blur_id = getident("blurdepthfx");
            if(!size_id || size_id->type != ID_VAR || !size_id->storage.i ||
               !blur_id || blur_id->type != ID_VAR || !blur_id->storage.i)
            {
                return false;
            }

            result.depth_size = *size_id->storage.i;
            result.blur = *blur_id->storage.i;
            result.quality = classifysoftparticlequality(
                result.depth_size, result.blur);
            return true;
        }

        bool writesoftparticlequality(
            SoftParticleQuality quality,
            SoftParticleQualityState &result)
        {
            if(!readsoftparticlequality(result))
                return false;

            int depth_size = 0, blur = 0;
            if(!softparticlequalityvalues(quality, depth_size, blur))
                return false;

            ident *size_id = getident("depthfxsize");
            ident *blur_id = getident("blurdepthfx");
            if(!size_id || size_id->type != ID_VAR || !size_id->storage.i ||
               size_id->flags&IDF_READONLY ||
               depth_size < size_id->minval ||
               depth_size > size_id->maxval ||
               !blur_id || blur_id->type != ID_VAR || !blur_id->storage.i ||
               blur_id->flags&IDF_READONLY ||
               blur < blur_id->minval || blur > blur_id->maxval)
            {
                return false;
            }

            const bool size_changed = *size_id->storage.i != depth_size;
            const bool blur_changed = *blur_id->storage.i != blur;
            if(size_changed && !SDL_GL_GetCurrentContext())
                return false;

            if(blur_changed)
                setvarchecked(blur_id, blur);
            if(size_changed)
                setvarchecked(size_id, depth_size);

            if(!readsoftparticlequality(result))
                return false;
            return result.quality == quality &&
                result.depth_size == depth_size && result.blur == blur;
        }

        bool readdecals(DecalsState &result)
        {
            result.enabled = false;

            ident *id = getident("decals");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result.enabled = *id->storage.i != 0;
            return true;
        }

        bool writedecals(bool enabled, DecalsState &result)
        {
            if(!readdecals(result))
                return false;

            ident *id = getident("decals");
            const int target = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               target < id->minval || target > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != target)
                setvarchecked(id, target);

            if(!readdecals(result))
                return false;
            return result.enabled == enabled;
        }

        static GrassDistance classifygrassdistance(int value)
        {
            switch(value)
            {
                case 128: return GrassDistance::Quick;
                case 256: return GrassDistance::Moderate;
                case 512: return GrassDistance::Slow;
                default: return GrassDistance::Custom;
            }
        }

        static bool grassdistancevalue(GrassDistance distance, int &value)
        {
            switch(distance)
            {
                case GrassDistance::Quick:
                    value = 128;
                    return true;
                case GrassDistance::Moderate:
                    value = 256;
                    return true;
                case GrassDistance::Slow:
                    value = 512;
                    return true;
                case GrassDistance::Custom:
                    return false;
            }
            return false;
        }

        bool readgrassdistance(GrassDistanceState &result)
        {
            result.distance = GrassDistance::Custom;
            result.value = 0;

            ident *id = getident("grassdist");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result.value = *id->storage.i;
            result.distance = classifygrassdistance(result.value);
            return true;
        }

        bool writegrassdistance(
            GrassDistance distance,
            GrassDistanceState &result)
        {
            if(!readgrassdistance(result))
                return false;

            int value = 0;
            if(!grassdistancevalue(distance, value))
                return false;

            ident *id = getident("grassdist");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            setvarchecked(id, value);

            if(!readgrassdistance(result))
                return false;
            return result.distance == distance && result.value == value;
        }

        static MotionBlurIntensity classifymotionblurintensity(float scale)
        {
            if(scale == 0.5f)
                return MotionBlurIntensity::Subtle;
            if(scale == 0.65f)
                return MotionBlurIntensity::Moderate;
            if(scale == 0.8f)
                return MotionBlurIntensity::Intense;
            return MotionBlurIntensity::Custom;
        }

        static bool motionblurintensityvalue(
            MotionBlurIntensity intensity,
            float &scale)
        {
            switch(intensity)
            {
                case MotionBlurIntensity::Subtle:
                    scale = 0.5f;
                    return true;
                case MotionBlurIntensity::Moderate:
                    scale = 0.65f;
                    return true;
                case MotionBlurIntensity::Intense:
                    scale = 0.8f;
                    return true;
                case MotionBlurIntensity::Custom:
                    return false;
            }
            return false;
        }

        bool readmotionblurintensity(MotionBlurIntensityState &result)
        {
            result.intensity = MotionBlurIntensity::Custom;
            result.scale = 0.0f;

            ident *id = getident("motionblurscale");
            if(!id || id->type != ID_FVAR || !id->storage.f)
                return false;

            result.scale = *id->storage.f;
            result.intensity = classifymotionblurintensity(result.scale);
            return true;
        }

        bool writemotionblurintensity(
            MotionBlurIntensity intensity,
            MotionBlurIntensityState &result)
        {
            if(!readmotionblurintensity(result))
                return false;

            ident *id = getident("motionblurscale");
            float scale = 0.0f;
            if(!id || id->type != ID_FVAR || !id->storage.f ||
               !motionblurintensityvalue(intensity, scale) ||
               id->flags&IDF_READONLY ||
               scale < id->minvalf || scale > id->maxvalf)
            {
                return false;
            }

            setfvarchecked(id, scale);

            if(!readmotionblurintensity(result))
                return false;
            return result.intensity == intensity && result.scale == scale;
        }

        static GlarePreset classifyglarepreset(float scale, int blur_radius)
        {
            if(scale == 1.0f && blur_radius == 4)
                return GlarePreset::Subtle;
            if(scale == 2.0f && blur_radius == 3)
                return GlarePreset::Glowy;
            if(scale == 1.0f && blur_radius == 7)
                return GlarePreset::Soft;
            if(scale == 2.0f && blur_radius == 7)
                return GlarePreset::Intense;
            return GlarePreset::Custom;
        }

        static bool glarepresetvalues(
            GlarePreset preset,
            float &scale,
            int &blur_radius)
        {
            switch(preset)
            {
                case GlarePreset::Subtle:
                    scale = 1.0f;
                    blur_radius = 4;
                    return true;
                case GlarePreset::Glowy:
                    scale = 2.0f;
                    blur_radius = 3;
                    return true;
                case GlarePreset::Soft:
                    scale = 1.0f;
                    blur_radius = 7;
                    return true;
                case GlarePreset::Intense:
                    scale = 2.0f;
                    blur_radius = 7;
                    return true;
                case GlarePreset::Custom:
                    return false;
            }
            return false;
        }

        bool readglarepreset(GlarePresetState &result)
        {
            result.preset = GlarePreset::Custom;
            result.scale = 0.0f;
            result.blur_radius = 0;

            ident *scale_id = getident("glarescale");
            ident *blur_id = getident("blurglare");
            if(!scale_id || scale_id->type != ID_FVAR || !scale_id->storage.f ||
               !blur_id || blur_id->type != ID_VAR || !blur_id->storage.i)
            {
                return false;
            }

            result.scale = *scale_id->storage.f;
            result.blur_radius = *blur_id->storage.i;
            result.preset = classifyglarepreset(result.scale, result.blur_radius);
            return true;
        }

        bool writeglarepreset(GlarePreset preset, GlarePresetState &result)
        {
            result.preset = GlarePreset::Custom;
            result.scale = 0.0f;
            result.blur_radius = 0;

            ident *scale_id = getident("glarescale");
            ident *blur_id = getident("blurglare");
            if(!scale_id || scale_id->type != ID_FVAR || !scale_id->storage.f ||
               !blur_id || blur_id->type != ID_VAR || !blur_id->storage.i)
            {
                return false;
            }

            result.scale = *scale_id->storage.f;
            result.blur_radius = *blur_id->storage.i;
            result.preset = classifyglarepreset(result.scale, result.blur_radius);

            float scale = 0.0f;
            int blur_radius = 0;
            if(!glarepresetvalues(preset, scale, blur_radius) ||
               scale_id->flags&IDF_READONLY || blur_id->flags&IDF_READONLY ||
               scale < scale_id->minvalf || scale > scale_id->maxvalf ||
               blur_radius < blur_id->minval || blur_radius > blur_id->maxval)
            {
                return false;
            }

            // Preserve the legacy menu's order. Neither ident has a callback.
            setvarchecked(blur_id, blur_radius);
            setfvarchecked(scale_id, scale);

            if(!readglarepreset(result))
                return false;
            return result.preset == preset && result.scale == scale &&
                result.blur_radius == blur_radius;
        }

        static void copyshaderdetail(const ::ShaderDetailState &source, ShaderDetailState &result)
        {
            result.requested = source.requested;
            result.ready = source.ready;
            result.inspected = source.inspected;
            result.alternatives = source.alternatives;
            result.unresolved = source.unresolved;
        }

        bool readshaderdetail(ShaderDetailState &result)
        {
            copyshaderdetail(::inspectshaderdetail(), result);
            return result.ready;
        }

        bool writeshaderdetail(ShaderDetailLevel level, ShaderDetailState &result)
        {
            const int value = static_cast<int>(level);
            ident *id = getident("shaderdetail");
            readshaderdetail(result);
            if(value < 0 || value > 3 || !result.ready || !id ||
               id->type != ID_VAR || !id->storage.i || id->flags&IDF_READONLY)
                return false;
            setvarchecked(id, value); // Existing synchronous fixshaderdetail callback.
            readshaderdetail(result);
            return *id->storage.i == value;
        }

        static void copymipmapblending(const ::MipmapBlendingState &source, MipmapBlendingState &result)
        {
            switch(source.status)
            {
                case ::MipmapBlendingStatus::Unavailable: result.status = MipmapBlendingStatus::Unavailable; break;
                case ::MipmapBlendingStatus::NoTextures: result.status = MipmapBlendingStatus::NoTextures; break;
                case ::MipmapBlendingStatus::Verified: result.status = MipmapBlendingStatus::Verified; break;
                case ::MipmapBlendingStatus::Mismatch: result.status = MipmapBlendingStatus::Mismatch; break;
                case ::MipmapBlendingStatus::Mixed: result.status = MipmapBlendingStatus::Mixed; break;
            }
            result.requested = source.requested;
            result.textures = source.textures;
        }

        bool readmipmapblending(MipmapBlendingState &result)
        {
            copymipmapblending(::inspectmipmapblending(), result);
            return result.status != MipmapBlendingStatus::Unavailable;
        }

        bool writemipmapblending(bool enabled, MipmapBlendingState &result)
        {
            ident *id = getident("trilinear");
            if(!SDL_GL_GetCurrentContext() || !id || id->type != ID_VAR ||
               !id->storage.i || id->flags&IDF_READONLY)
            {
                readmipmapblending(result);
                return false;
            }
            setvarchecked(id, enabled ? 1 : 0);
            copymipmapblending(::lastmipmapblendingapplication(), result);
            return *id->storage.i == (enabled ? 1 : 0);
        }

        static void copyanisotropy(const ::AnisotropyState &source, AnisotropyState &result)
        {
            switch(source.status)
            {
                case ::AnisotropyStatus::Unavailable: result.status = AnisotropyStatus::Unavailable; break;
                case ::AnisotropyStatus::Unsupported: result.status = AnisotropyStatus::Unsupported; break;
                case ::AnisotropyStatus::NoTextures: result.status = AnisotropyStatus::NoTextures; break;
                case ::AnisotropyStatus::Verified: result.status = AnisotropyStatus::Verified; break;
                case ::AnisotropyStatus::Mismatch: result.status = AnisotropyStatus::Mismatch; break;
                case ::AnisotropyStatus::Mixed: result.status = AnisotropyStatus::Mixed; break;
            }
            result.requested = source.requested;
            result.maximum = source.maximum;
            result.capped = source.capped;
            result.textures = source.textures;
            result.actual = source.actual;
        }

        bool readanisotropy(AnisotropyState &result)
        {
            copyanisotropy(::inspectanisotropy(), result);
            return result.status != AnisotropyStatus::Unavailable;
        }

        bool writeanisotropy(AnisotropyLevel level, AnisotropyState &result)
        {
            const int value = static_cast<int>(level);
            ident *id = getident("aniso");
            if((value != 0 && value != 2 && value != 4 && value != 8 && value != 16) ||
               !SDL_GL_GetCurrentContext() || !hasAF || hwmaxaniso < 2 ||
               value > hwmaxaniso || !id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY)
            {
                readanisotropy(result);
                return false;
            }
            setvarchecked(id, value);
            copyanisotropy(::lastanisotropyapplication(), result);
            // Write acceptance is independent of loaded-texture verification.
            return *id->storage.i == value;
        }

        bool readaudio(Audio &result)
        {
            ident *sound_id = soundvolumeident();
            ident *music_id = musicvolumeident();
            ident *soundpack_id = soundpackident();
            if(!sound_id || !sound_id->storage.i ||
               !music_id || !music_id->storage.i ||
               !soundpack_id || !soundpack_id->storage.s ||
               !*soundpack_id->storage.s)
            {
                return false;
            }

            result.sound_volume = *sound_id->storage.i;
            result.sound_volume_minimum = sound_id->minval;
            result.sound_volume_maximum = sound_id->maxval;
            result.music_volume = *music_id->storage.i;
            result.music_volume_minimum = music_id->minval;
            result.music_volume_maximum = music_id->maxval;
            result.active_soundpack = *soundpack_id->storage.s;
            result.backend_active = !::nosound;
            return true;
        }

        struct SoundpackVisitorAdapter
        {
            SoundpackChoiceVisitor visitor;
            void *data;
        };

        static void visitsoundpackchoice(const char *name, void *data)
        {
            SoundpackVisitorAdapter &adapter =
                *static_cast<SoundpackVisitorAdapter *>(data);
            SoundpackChoice choice;
            choice.id = name;
            choice.display_name = !std::strcmp(name, "original")
                ? "Original"
                : name;
            adapter.visitor(choice, adapter.data);
        }

        bool visitsoundpacks(SoundpackChoiceVisitor visitor, void *data)
        {
            if(!visitor)
                return false;

            SoundpackVisitorAdapter adapter;
            adapter.visitor = visitor;
            adapter.data = data;
            ::visitsoundpacks(visitsoundpackchoice, &adapter);
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

        struct SoundpackMatch
        {
            const char *requested;
            bool found;
        };

        static void findsoundpack(const char *name, void *data)
        {
            SoundpackMatch &match = *static_cast<SoundpackMatch *>(data);
            if(!std::strcmp(name, match.requested))
                match.found = true;
        }

        bool writesoundpack(const char *requested)
        {
            ident *id = soundpackident();
            if(!id || !id->storage.s || !requested || !requested[0])
                return false;

            SoundpackMatch match = { requested, false };
            ::visitsoundpacks(findsoundpack, &match);
            if(!match.found)
                return false;

            setsvarchecked(id, requested);
            return *id->storage.s &&
                !std::strcmp(*id->storage.s, requested);
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
            return writeboolean(id, enabled);
        }

        bool readcrosshair(Crosshair &result)
        {
            ident *size_id = crosshairsizeident();
            ident *effects_id = crosshaireffectsident();
            ident *colors_id = crosshairhealthcolorsident();
            ident *team_id = teamcrosshairident();
            ident *hit_id = hitcrosshairident();
            ident *fade_id = crosshairreloadfadeident();
            ident *rotation_id = crosshairreloadrotationident();
            if(!size_id || !size_id->storage.i ||
               !effects_id || !effects_id->storage.i ||
               !colors_id || !colors_id->storage.i ||
               !team_id || !team_id->storage.i ||
               !hit_id || !hit_id->storage.i ||
               !fade_id || !fade_id->storage.i ||
               !rotation_id || !rotation_id->storage.i)
            {
                return false;
            }

            result.selection = selectedcrosshair();
            result.resource = getcrosshairname(0);
            result.size = *size_id->storage.i;
            result.size_minimum = size_id->minval;
            result.size_maximum = size_id->maxval;
            result.effects = *effects_id->storage.i != 0;
            result.health_colors = *colors_id->storage.i != 0;
            result.teammates = *team_id->storage.i != 0;
            result.hit_marker = *hit_id->storage.i > 0;
            result.reload_fade = *fade_id->storage.i != 0;
            result.reload_rotation = *rotation_id->storage.i != 0;
            return true;
        }

        bool visitcrosshairs(CrosshairChoiceVisitor visitor, void *data)
        {
            if(!visitor)
                return false;

            loopi(int(sizeof(crosshair_catalog) / sizeof(crosshair_catalog[0])))
            {
                if(!crosshairavailable(crosshair_catalog[i]))
                    continue;

                CrosshairChoice choice;
                choice.id = i;
                choice.label = crosshair_catalog[i].label;
                choice.resource = crosshair_catalog[i].normal;
                visitor(choice, data);
            }
            return true;
        }

        bool writecrosshairselection(int selection)
        {
            if(selection < 0 ||
               selection >= int(sizeof(crosshair_catalog) /
                   sizeof(crosshair_catalog[0])))
            {
                return false;
            }

            const CrosshairCatalogEntry &choice =
                crosshair_catalog[selection];
            if(!crosshairavailable(choice))
                return false;

            loadcrosshair(choice.normal, 0);
            loadcrosshair(choice.hit, 2);
            return selectedcrosshair() == selection &&
                !std::strcmp(getcrosshairname(2), choice.hit);
        }

        bool writecrosshairsize(int size)
        {
            ident *id = crosshairsizeident();
            if(!id || !id->storage.i ||
               size < id->minval || size > id->maxval)
            {
                return false;
            }

            setvarchecked(id, size);
            return *id->storage.i == size;
        }

        bool writecrosshaireffects(bool enabled)
        {
            return writeboolean(crosshaireffectsident(), enabled);
        }

        bool writecrosshairhealthcolors(bool enabled)
        {
            return writeboolean(crosshairhealthcolorsident(), enabled);
        }

        bool writeteamcrosshair(bool enabled)
        {
            return writeboolean(teamcrosshairident(), enabled);
        }

        bool writehitcrosshair(bool enabled)
        {
            ident *id = hitcrosshairident();
            if(!id || !id->storage.i)
                return false;

            const int value = enabled ? 425 : 0;
            if(value < id->minval || value > id->maxval)
                return false;
            setvarchecked(id, value);
            return (*id->storage.i > 0) == enabled;
        }

        bool writecrosshairreloadfade(bool enabled)
        {
            return writeboolean(crosshairreloadfadeident(), enabled);
        }

        bool writecrosshairreloadrotation(bool enabled)
        {
            return writeboolean(crosshairreloadrotationident(), enabled);
        }

        bool readhud(Hud &result)
        {
            ident *screen_id = damagescreenident();
            ident *compass_id = damagecompassident();
            ident *messages_id = fragmessagesident();
            ident *streak_id = streakmilestonescaleident();
            if(!screen_id || !screen_id->storage.i ||
               !compass_id || !compass_id->storage.i ||
               !messages_id || !messages_id->storage.i ||
               !streak_id || !streak_id->storage.f)
            {
                return false;
            }

            result.damage_screen = *screen_id->storage.i != 0;
            result.damage_compass = *compass_id->storage.i != 0;
            switch(*messages_id->storage.i)
            {
                case 0:
                    result.frag_messages = FragMessagesMode::Off;
                    break;
                case 2:
                    result.frag_messages = FragMessagesMode::OutsideInsta;
                    break;
                default:
                    result.frag_messages = FragMessagesMode::AllModes;
                    break;
            }
            result.streak_milestone_scale = *streak_id->storage.f;
            result.streak_milestone_scale_minimum = streak_id->minvalf;
            result.streak_milestone_scale_maximum = streak_id->maxvalf;
            return true;
        }

        bool writedamagescreen(bool enabled)
        {
            return writeboolean(damagescreenident(), enabled);
        }

        bool writedamagecompass(bool enabled)
        {
            return writeboolean(damagecompassident(), enabled);
        }

        bool writefragmessages(FragMessagesMode mode)
        {
            ident *id = fragmessagesident();
            if(!id || !id->storage.i)
                return false;

            int value = 0;
            switch(mode)
            {
                case FragMessagesMode::Off:
                    value = 0;
                    break;
                case FragMessagesMode::AllModes:
                    value = 1;
                    break;
                case FragMessagesMode::OutsideInsta:
                    value = 2;
                    break;
            }
            if(value < id->minval || value > id->maxval)
                return false;
            setvarchecked(id, value);
            return *id->storage.i == value;
        }

        bool writestreakmilestonescale(float value)
        {
            ident *id = streakmilestonescaleident();
            if(!id || !id->storage.f || !finitefloat(value) ||
               value < id->minvalf || value > id->maxvalf)
            {
                return false;
            }

            setfvarchecked(id, value);
            return *id->storage.f == value;
        }

        bool readchat(Chat &result)
        {
            ident *top_id = consolefilterident();
            ident *bottom_id = miniconsolefilterident();
            ident *colors_id = teamcolorchatident();
            if(!top_id || !top_id->storage.i ||
               !bottom_id || !bottom_id->storage.i ||
               !colors_id || !colors_id->storage.i)
            {
                return false;
            }

            result.public_messages_in_top_feed =
                (*top_id->storage.i & CON_CHAT) != 0;
            result.team_messages_in_top_feed =
                (*top_id->storage.i & CON_TEAMCHAT) != 0;
            result.public_messages_in_bottom_box =
                (*bottom_id->storage.i & CON_CHAT) != 0;
            result.team_messages_in_bottom_box =
                (*bottom_id->storage.i & CON_TEAMCHAT) != 0;
            result.team_colored_names = *colors_id->storage.i != 0;
            return true;
        }

        bool writepublicchatintopfeed(bool enabled)
        {
            return writefilterbit(consolefilterident(), CON_CHAT, enabled);
        }

        bool writeteamchatintopfeed(bool enabled)
        {
            return writefilterbit(
                consolefilterident(),
                CON_TEAMCHAT,
                enabled
            );
        }

        bool writepublicchatinbottombox(bool enabled)
        {
            return writefilterbit(
                miniconsolefilterident(),
                CON_CHAT,
                enabled
            );
        }

        bool writeteamchatinbottombox(bool enabled)
        {
            return writefilterbit(
                miniconsolefilterident(),
                CON_TEAMCHAT,
                enabled
            );
        }

        bool writeteamcoloredchatnames(bool enabled)
        {
            return writeboolean(teamcolorchatident(), enabled);
        }
    }
}
