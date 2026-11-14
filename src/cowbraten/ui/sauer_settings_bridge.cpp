#include "engine.h"
#include "game.h"

#include "sauer_settings_bridge.h"
#include "ui_persistence.h"

#ifdef __APPLE__
#include <SDL2_mixer/SDL_mixer.h>
#else
#include "SDL_mixer.h"
#endif

#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstring>

extern int scr_w, scr_h, fullscreendesktop;
extern int grass, motionblur;
extern bool nosound;
extern int maxchannels;
extern void resetgl();
extern void resetsound();
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

        static ident *bloodident()
        {
            ident *id = getident("blood");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hitsoundident()
        {
            ident *id = getident("hitsound");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *outlinemetersident()
        {
            ident *id = getident("outlinemeters");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *deathscoreident()
        {
            ident *id = getident("deathscore");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hexvarident(const char *name)
        {
            ident *id = getident(name);
            return id && id->type == ID_VAR && (id->flags & IDF_HEX)
                ? id
                : nullptr;
        }

        static ident *trailcolorshotgunident()
        {
            return hexvarident("trailcolorshotgun");
        }

        static ident *trailcolorchaingunident()
        {
            return hexvarident("trailcolorchaingun");
        }

        static ident *trailcolorrocketlauncherident()
        {
            return hexvarident("trailcolorrocketlauncher");
        }

        static ident *trailcolorrifleident()
        {
            return hexvarident("trailcolorrifle");
        }

        static ident *trailcolorgrenadelauncherident()
        {
            return hexvarident("trailcolorgrenadelauncher");
        }

        static ident *trailcolorpistolident()
        {
            return hexvarident("trailcolorpistol");
        }

        static ident *basenumbersident()
        {
            ident *id = getident("basenumbers");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *radarteammatesident()
        {
            ident *id = getident("radarteammates");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ragdollident()
        {
            ident *id = getident("ragdoll");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ragdollmillisident()
        {
            ident *id = getident("ragdollmillis");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *deadpushident()
        {
            ident *id = getident("deadpush");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hidedeadident()
        {
            ident *id = getident("hidedead");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *fullbrightmodelsident()
        {
            ident *id = getident("fullbrightmodels");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *forceplayermodelsident()
        {
            ident *id = getident("forceplayermodels");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *teamskinsident()
        {
            ident *id = getident("teamskins");
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

        static ident *gammaident()
        {
            ident *id = getident("gamma");
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

        static ident *depthbitsident()
        {
            ident *id = getident("depthbits");
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

        static ident *soundchansident()
        {
            ident *id = getident("soundchans");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *soundfreqident()
        {
            ident *id = getident("soundfreq");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *soundbufferlenident()
        {
            ident *id = getident("soundbufferlen");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *mumbleident()
        {
            ident *id = getident("mumble");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static bool nativesoundfrequency(int value)
        {
            return value == 11025 || value == 22050 || value == 44100;
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

        static ident *showfpsident()
        {
            ident *id = getident("showfps");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *wallclockident()
        {
            ident *id = getident("wallclock");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *wallclock24ident()
        {
            ident *id = getident("wallclock24");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *wallclocksecsident()
        {
            ident *id = getident("wallclocksecs");
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

        static ident *hudgunident()
        {
            ident *id = getident("hudgun");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hudgunswayident()
        {
            ident *id = getident("hudgunsway");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *muzzleflashident()
        {
            ident *id = getident("muzzleflash");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *muzzlelightident()
        {
            ident *id = getident("muzzlelight");
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

        static ident *maxhudfragmessagesident()
        {
            ident *id = getident("maxhudfragmessages");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hudfragmessagedurationident()
        {
            ident *id = getident("hudfragmessageduration");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *hudfragmessagefilterident()
        {
            // HVARP is ID_VAR with IDF_HEX; there is no ID_HVAR type.
            ident *id = getident("hudfragmessagefilter");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hudfragmessagescaleident()
        {
            ident *id = getident("hudfragmessagescale");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *hudfragmessagefadeident()
        {
            ident *id = getident("hudfragmessagefade");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hudfragmessagexident()
        {
            ident *id = getident("hudfragmessagex");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *hudfragmessageyident()
        {
            ident *id = getident("hudfragmessagey");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *hudfragmessagestackdirident()
        {
            ident *id = getident("hudfragmessagestackdir");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ammobarident()
        {
            ident *id = getident("ammobar");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ammobarhorizontalident()
        {
            ident *id = getident("ammobarhorizontal");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ammobarflipident()
        {
            ident *id = getident("ammobarflip");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ammobarhideemptyident()
        {
            ident *id = getident("ammobarhideempty");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ammobarsepident()
        {
            ident *id = getident("ammobarsep");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ammobarcountsepident()
        {
            ident *id = getident("ammobarcountsep");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ammobaralignident()
        {
            ident *id = getident("ammobaralign");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *ammobarscaleident()
        {
            ident *id = getident("ammobarscale");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *ammobarcountscaleident()
        {
            ident *id = getident("ammobarcountscale");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *ammobarxident()
        {
            ident *id = getident("ammobarx");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *ammobaryident()
        {
            ident *id = getident("ammobary");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *gameclockident()
        {
            ident *id = getident("gameclock");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *gameclockscaleident()
        {
            ident *id = getident("gameclockscale");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *gameclockalphaident()
        {
            ident *id = getident("gameclockalpha");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *gameclockalignident()
        {
            ident *id = getident("gameclockalign");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *gameclockxident()
        {
            ident *id = getident("gameclockx");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *gameclockyident()
        {
            ident *id = getident("gameclocky");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static bool isgameclockalign(int value)
        {
            return value == -1 || value == 0 || value == 1;
        }

        static ident *hudscoreident()
        {
            ident *id = getident("hudscore");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hudscorescaleident()
        {
            ident *id = getident("hudscorescale");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *hudscorealphaident()
        {
            ident *id = getident("hudscorealpha");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hudscoresepident()
        {
            ident *id = getident("hudscoresep");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hudscorealignident()
        {
            ident *id = getident("hudscorealign");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *hudscorexident()
        {
            ident *id = getident("hudscorex");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *hudscoreyident()
        {
            ident *id = getident("hudscorey");
            return id && id->type == ID_FVAR
                ? id
                : nullptr;
        }

        static ident *hidefragsident()
        {
            ident *id = getident("hidefrags");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showdeathsident()
        {
            ident *id = getident("showdeaths");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showpingident()
        {
            ident *id = getident("showping");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showpjident()
        {
            ident *id = getident("showpj");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showclientnumident()
        {
            ident *id = getident("showclientnum");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showflagsident()
        {
            ident *id = getident("showflags");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showsuicidesident()
        {
            ident *id = getident("showsuicides");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showkpdident()
        {
            ident *id = getident("showkpd");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showaccuracyident()
        {
            ident *id = getident("showaccuracy");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showdamageident()
        {
            ident *id = getident("showdamage");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *showdamagereceivedident()
        {
            ident *id = getident("showdamagereceived");
            return id && id->type == ID_VAR
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

        static ident *autoignoreenabledident()
        {
            ident *id = getident("autoignoreenabled");
            return id && id->type == ID_VAR ? id : nullptr;
        }

        static ident *autoignoreconfiguredident()
        {
            ident *id = getident("autoignoreconfigured");
            return id && id->type == ID_VAR ? id : nullptr;
        }

        static ident *autoignoreslotcountident()
        {
            ident *id = getident("autoignoreslotcount");
            return id && id->type == ID_VAR ? id : nullptr;
        }

        static ident *autoexecprofileident()
        {
            ident *id = getident("autoexecprofile");
            return id && id->type == ID_SVAR ? id : nullptr;
        }

        static ident *uithemeident()
        {
            ident *id = getident("uitheme");
            return id && id->type == ID_SVAR ? id : nullptr;
        }

        static ident *interfaceappearanceident(const char *name)
        {
            ident *id = getident(name);
            return id && id->type == ID_VAR ? id : nullptr;
        }

        static ident *autoignorenameident(int slot)
        {
            if(slot < 0 || slot >= Chat::AutoIgnoreSlotCount)
                return nullptr;
            string name;
            formatstring(name, "autoignorename%d", slot + 1);
            ident *id = getident(name);
            return id && id->type == ID_SVAR ? id : nullptr;
        }

        static ident *fullconsolefilterident()
        {
            ident *id = getident("fullconfilter");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        static ident *consolesizeident()
        {
            ident *id = getident("consize");
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

            const int value = enabled ? 1 : 0;
            if(*id->storage.i != value)
                setvarchecked(id, value);
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

        static bool validdepthbits(int bits)
        {
            switch(bits)
            {
                case 0:
                case 16:
                case 24:
                case 32:
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

        bool readgamefeedback(GameFeedback &result)
        {
            result.blood = false;
            result.hit_sound = false;
            result.outline_capture_meters = false;
            result.show_scoreboard_at_death = false;

            ident *blood_id = bloodident();
            ident *hitsound_id = hitsoundident();
            ident *meters_id = outlinemetersident();
            ident *death_id = deathscoreident();
            if(!blood_id || !blood_id->storage.i ||
               !hitsound_id || !hitsound_id->storage.i ||
               !meters_id || !meters_id->storage.i ||
               !death_id || !death_id->storage.i)
            {
                return false;
            }

            result.blood = *blood_id->storage.i != 0;
            result.hit_sound = *hitsound_id->storage.i != 0;
            result.outline_capture_meters = *meters_id->storage.i != 0;
            result.show_scoreboard_at_death = *death_id->storage.i != 0;
            return true;
        }

        static bool writegamefeedbackboolean(
            ident *id,
            bool enabled,
            GameFeedback &result)
        {
            if(!readgamefeedback(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            const int value = enabled ? 1 : 0;
            if(value < id->minval || value > id->maxval)
                return false;

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readgamefeedback(result))
                return false;
            return (*id->storage.i != 0) == enabled;
        }

        bool writeblood(bool enabled, GameFeedback &result)
        {
            return writegamefeedbackboolean(bloodident(), enabled, result);
        }

        bool writehitsound(bool enabled, GameFeedback &result)
        {
            return writegamefeedbackboolean(hitsoundident(), enabled, result);
        }

        bool writeoutlinemeters(bool enabled, GameFeedback &result)
        {
            return writegamefeedbackboolean(
                outlinemetersident(),
                enabled,
                result);
        }

        bool writedeathscore(bool enabled, GameFeedback &result)
        {
            return writegamefeedbackboolean(
                deathscoreident(),
                enabled,
                result);
        }

        static void defaulttrailcolors(TrailColors &result)
        {
            result.shotgun = 0xFFC864;
            result.chaingun = 0xFFC864;
            result.rocketlauncher = 0x404040;
            result.rifle = 0x404040;
            result.grenadelauncher = 0x404040;
            result.pistol = 0xFFC864;
            result.channel_minimum = 0;
            result.channel_maximum = 255;
        }

        bool readtrailcolors(TrailColors &result)
        {
            defaulttrailcolors(result);

            ident *shotgun_id = trailcolorshotgunident();
            ident *chaingun_id = trailcolorchaingunident();
            ident *rocket_id = trailcolorrocketlauncherident();
            ident *rifle_id = trailcolorrifleident();
            ident *grenade_id = trailcolorgrenadelauncherident();
            ident *pistol_id = trailcolorpistolident();
            if(!shotgun_id || !shotgun_id->storage.i ||
               !chaingun_id || !chaingun_id->storage.i ||
               !rocket_id || !rocket_id->storage.i ||
               !rifle_id || !rifle_id->storage.i ||
               !grenade_id || !grenade_id->storage.i ||
               !pistol_id || !pistol_id->storage.i)
            {
                return false;
            }

            result.shotgun = *shotgun_id->storage.i;
            result.chaingun = *chaingun_id->storage.i;
            result.rocketlauncher = *rocket_id->storage.i;
            result.rifle = *rifle_id->storage.i;
            result.grenadelauncher = *grenade_id->storage.i;
            result.pistol = *pistol_id->storage.i;
            result.channel_minimum = 0;
            result.channel_maximum = 255;
            return true;
        }

        static bool writetrailcolorpacked(
            ident *id,
            int packed,
            TrailColors &result)
        {
            if(!readtrailcolors(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               packed < id->minval || packed > id->maxval)
            {
                return false;
            }

            if(*id->storage.i == packed)
                return true;

            setvarchecked(id, packed);
            if(!readtrailcolors(result))
                return false;
            return *id->storage.i == packed;
        }

        bool writetrailcolorshotgun(int packed, TrailColors &result)
        {
            return writetrailcolorpacked(
                trailcolorshotgunident(),
                packed,
                result);
        }

        bool writetrailcolorchaingun(int packed, TrailColors &result)
        {
            return writetrailcolorpacked(
                trailcolorchaingunident(),
                packed,
                result);
        }

        bool writetrailcolorrocketlauncher(int packed, TrailColors &result)
        {
            return writetrailcolorpacked(
                trailcolorrocketlauncherident(),
                packed,
                result);
        }

        bool writetrailcolorrifle(int packed, TrailColors &result)
        {
            return writetrailcolorpacked(
                trailcolorrifleident(),
                packed,
                result);
        }

        bool writetrailcolorgrenadelauncher(int packed, TrailColors &result)
        {
            return writetrailcolorpacked(
                trailcolorgrenadelauncherident(),
                packed,
                result);
        }

        bool writetrailcolorpistol(int packed, TrailColors &result)
        {
            return writetrailcolorpacked(
                trailcolorpistolident(),
                packed,
                result);
        }

        bool readgameradar(GameRadar &result)
        {
            result.numbered_capture_bases = false;
            result.show_teammates_on_radar = false;

            ident *bases_id = basenumbersident();
            ident *teammates_id = radarteammatesident();
            if(!bases_id || !bases_id->storage.i ||
               !teammates_id || !teammates_id->storage.i)
            {
                return false;
            }

            result.numbered_capture_bases = *bases_id->storage.i != 0;
            result.show_teammates_on_radar = *teammates_id->storage.i != 0;
            return true;
        }

        static bool writegameradarboolean(
            ident *id,
            bool enabled,
            GameRadar &result)
        {
            if(!readgameradar(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            const int value = enabled ? 1 : 0;
            if(value < id->minval || value > id->maxval)
                return false;

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readgameradar(result))
                return false;
            return (*id->storage.i != 0) == enabled;
        }

        bool writebasenumbers(bool enabled, GameRadar &result)
        {
            return writegameradarboolean(
                basenumbersident(),
                enabled,
                result);
        }

        bool writeradarteammates(bool enabled, GameRadar &result)
        {
            return writegameradarboolean(
                radarteammatesident(),
                enabled,
                result);
        }

        enum
        {
            kRagdollKeepAfterRespawnMillis = 10000,
            kFullbrightSubtle = 60,
            kFullbrightBright = 100,
            kFullbrightOverbright = 150,
            kFullbrightMax = 200
        };

        static bool isfullbrightintensity(int value)
        {
            return value == kFullbrightSubtle ||
                value == kFullbrightBright ||
                value == kFullbrightOverbright ||
                value == kFullbrightMax;
        }

        bool readgamemodels(GameModels &result)
        {
            result.ragdoll_deaths = false;
            result.keep_ragdoll_after_respawn = false;
            result.ragdoll_millis = 0;
            result.deadpush = 2;
            result.deadpush_minimum = 1;
            result.deadpush_maximum = 20;
            result.hide_dead_players = false;
            result.hidedead = 0;
            result.fullbright_player_models = false;
            result.fullbrightmodels = 0;
            result.force_matching_player_models = false;
            result.always_use_team_skins = false;

            ident *ragdoll_id = ragdollident();
            ident *ragdollmillis_id = ragdollmillisident();
            ident *deadpush_id = deadpushident();
            ident *hidedead_id = hidedeadident();
            ident *fullbright_id = fullbrightmodelsident();
            ident *force_id = forceplayermodelsident();
            ident *teamskins_id = teamskinsident();
            if(!ragdoll_id || !ragdoll_id->storage.i ||
               !ragdollmillis_id || !ragdollmillis_id->storage.i ||
               !deadpush_id || !deadpush_id->storage.i ||
               !hidedead_id || !hidedead_id->storage.i ||
               !fullbright_id || !fullbright_id->storage.i ||
               !force_id || !force_id->storage.i ||
               !teamskins_id || !teamskins_id->storage.i)
            {
                return false;
            }

            result.ragdoll_deaths = *ragdoll_id->storage.i != 0;
            result.ragdoll_millis = *ragdollmillis_id->storage.i;
            result.keep_ragdoll_after_respawn = result.ragdoll_millis != 0;
            result.deadpush = *deadpush_id->storage.i;
            result.deadpush_minimum = deadpush_id->minval;
            result.deadpush_maximum = deadpush_id->maxval;
            result.hidedead = *hidedead_id->storage.i;
            result.hide_dead_players = result.hidedead != 0;
            result.fullbrightmodels = *fullbright_id->storage.i;
            result.fullbright_player_models = result.fullbrightmodels != 0;
            result.force_matching_player_models = *force_id->storage.i != 0;
            result.always_use_team_skins = *teamskins_id->storage.i != 0;
            return true;
        }

        static bool writegamemodelsboolean(
            ident *id,
            bool enabled,
            GameModels &result)
        {
            if(!readgamemodels(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            const int value = enabled ? 1 : 0;
            if(value < id->minval || value > id->maxval)
                return false;

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readgamemodels(result))
                return false;
            return (*id->storage.i != 0) == enabled;
        }

        // Native Settings checkboxes with a dedicated on-value only write
        // when the user toggles. A non-off engine value stays put on open
        // and on a no-op "checked" event.
        static bool writegamemodelsonoff(
            ident *id,
            bool enabled,
            int on_value,
            int off_value,
            GameModels &result)
        {
            if(!readgamemodels(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            if(enabled)
            {
                if(*id->storage.i == off_value)
                {
                    if(on_value < id->minval || on_value > id->maxval)
                        return false;
                    setvarchecked(id, on_value);
                }
            }
            else if(*id->storage.i != off_value)
            {
                if(off_value < id->minval || off_value > id->maxval)
                    return false;
                setvarchecked(id, off_value);
            }

            if(!readgamemodels(result))
                return false;
            return enabled
                ? *id->storage.i != off_value
                : *id->storage.i == off_value;
        }

        bool writeragdoll(bool enabled, GameModels &result)
        {
            return writegamemodelsboolean(
                ragdollident(),
                enabled,
                result);
        }

        bool writeragdollmillis(bool enabled, GameModels &result)
        {
            return writegamemodelsonoff(
                ragdollmillisident(),
                enabled,
                kRagdollKeepAfterRespawnMillis,
                0,
                result);
        }

        bool writedeadpush(int value, GameModels &result)
        {
            if(!readgamemodels(result))
                return false;

            ident *id = deadpushident();
            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i == value)
                return true;

            setvarchecked(id, value);
            if(!readgamemodels(result))
                return false;
            return result.deadpush == value;
        }

        bool writehidedead(bool enabled, GameModels &result)
        {
            return writegamemodelsonoff(
                hidedeadident(),
                enabled,
                1,
                0,
                result);
        }

        bool writefullbrightmodels(bool enabled, GameModels &result)
        {
            return writegamemodelsonoff(
                fullbrightmodelsident(),
                enabled,
                kFullbrightSubtle,
                0,
                result);
        }

        bool writefullbrightintensity(int value, GameModels &result)
        {
            if(!readgamemodels(result))
                return false;

            ident *id = fullbrightmodelsident();
            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               !isfullbrightintensity(value) ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i == value)
                return true;

            setvarchecked(id, value);
            if(!readgamemodels(result))
                return false;
            return result.fullbrightmodels == value;
        }

        bool writeforceplayermodels(bool enabled, GameModels &result)
        {
            return writegamemodelsboolean(
                forceplayermodelsident(),
                enabled,
                result);
        }

        bool writeteamskins(bool enabled, GameModels &result)
        {
            return writegamemodelsboolean(
                teamskinsident(),
                enabled,
                result);
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

            if(*id->storage.i != value)
                setvarchecked(id, value);
            return *id->storage.i == value;
        }

        static int windowgamma()
        {
            if(!screen)
                return 100;
            return int(std::lround(
                double(SDL_GetWindowBrightness(screen)) * 100.0));
        }

        bool readgamma(Gamma &result)
        {
            result.requested = 100;
            result.effective = 100;
            result.minimum = 30;
            result.maximum = 300;

            ident *id = gammaident();
            if(!id || !id->storage.i || !screen)
                return false;

            result.requested = *id->storage.i;
            result.minimum = id->minval;
            result.maximum = id->maxval;
            result.effective = windowgamma();
            return true;
        }

        bool writegamma(int value, Gamma &result)
        {
            if(!readgamma(result))
                return false;

            ident *id = gammaident();
            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            // Same requested value: keep the ident untouched so opening
            // Settings cannot rewrite SDL brightness.
            if(*id->storage.i == value)
                return true;

            setvarchecked(id, value);
            if(!readgamma(result))
                return false;
            return result.requested == value;
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

        bool readdepthbits(DepthBuffer &result)
        {
            ident *id = depthbitsident();
            if(!id || !id->storage.i || !screen)
                return false;

            int bits = 0;
            if(SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &bits) < 0)
                return false;

            result.requested_bits = *id->storage.i;
            result.actual_bits = max(bits, 0);
            return true;
        }

        bool writedepthbits(int bits)
        {
            ident *id = depthbitsident();
            if(!id || !id->storage.i ||
               !validdepthbits(bits) ||
               bits < id->minval || bits > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != bits)
                setvarchecked(id, bits);
            return *id->storage.i == bits;
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

        bool readwaterenvironmentreflections(
            WaterEnvironmentReflectionsState &result)
        {
            result.enabled = false;

            ident *id = getident("waterenvmap");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 0 ||
               id->maxval != 1 ||
               (*id->storage.i != 0 && *id->storage.i != 1))
            {
                return false;
            }

            result.enabled = *id->storage.i != 0;
            return true;
        }

        bool writewaterenvironmentreflections(
            bool enabled,
            WaterEnvironmentReflectionsState &result)
        {
            if(!readwaterenvironmentreflections(result))
                return false;

            const int value = enabled ? 1 : 0;
            if(result.enabled == enabled)
                return true;

            if(!SDL_GL_GetCurrentContext())
                return false;

            ident *id = getident("waterenvmap");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 0 ||
               id->maxval != 1 ||
               (*id->storage.i != 0 && *id->storage.i != 1))
            {
                return false;
            }
            if(*id->storage.i == value)
            {
                if(!readwaterenvironmentreflections(result))
                    return false;
                return result.enabled == enabled;
            }

            setvarchecked(id, value);

            if(!readwaterenvironmentreflections(result))
                return false;
            return result.enabled == enabled;
        }

        static bool waterrenderresolution(
            int exponent,
            WaterRenderResolution &level)
        {
            switch(exponent)
            {
                case 6: level = WaterRenderResolution::Minimum; return true;
                case 7: level = WaterRenderResolution::Low; return true;
                case 8: level = WaterRenderResolution::Medium; return true;
                case 9: level = WaterRenderResolution::High; return true;
                case 10: level = WaterRenderResolution::VeryHigh; return true;
                case 11: level = WaterRenderResolution::Ultra; return true;
            }
            return false;
        }

        static bool waterrenderresolution(
            WaterRenderResolution level,
            int &exponent)
        {
            switch(level)
            {
                case WaterRenderResolution::Minimum: exponent = 6; return true;
                case WaterRenderResolution::Low: exponent = 7; return true;
                case WaterRenderResolution::Medium: exponent = 8; return true;
                case WaterRenderResolution::High: exponent = 9; return true;
                case WaterRenderResolution::VeryHigh: exponent = 10; return true;
                case WaterRenderResolution::Ultra: exponent = 11; return true;
            }
            return false;
        }

        bool readwaterrenderresolution(
            WaterRenderResolutionState &result)
        {
            result.level = WaterRenderResolution::Minimum;
            result.configured_exponent = 0;
            result.requested_dimension = 0;

            ident *id = getident("reflectsize");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 6 ||
               id->maxval != 11 || *id->storage.i < id->minval ||
               *id->storage.i > id->maxval ||
               !waterrenderresolution(*id->storage.i, result.level))
            {
                return false;
            }

            result.configured_exponent = *id->storage.i;
            result.requested_dimension = 1 << result.configured_exponent;
            return true;
        }

        bool writewaterrenderresolution(
            WaterRenderResolution level,
            WaterRenderResolutionState &result)
        {
            if(!readwaterrenderresolution(result))
                return false;

            int exponent = 0;
            if(!waterrenderresolution(level, exponent))
                return false;
            if(result.configured_exponent == exponent)
                return true;

            ident *id = getident("reflectsize");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 6 ||
               id->maxval != 11 || *id->storage.i < id->minval ||
               *id->storage.i > id->maxval ||
               !SDL_GL_GetCurrentContext())
            {
                return false;
            }

            setvarchecked(id, exponent);

            if(!readwaterrenderresolution(result))
                return false;
            return result.level == level &&
                result.configured_exponent == exponent;
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

        bool readglassreflections(GlassReflectionsState &result)
        {
            result.enabled = false;

            ident *id = getident("glassenv");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result.enabled = *id->storage.i != 0;
            return true;
        }

        bool writeglassreflections(
            bool enabled,
            GlassReflectionsState &result)
        {
            if(!readglassreflections(result))
                return false;

            ident *id = getident("glassenv");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readglassreflections(result))
                return false;
            return result.enabled == enabled;
        }

        bool readmodelreflections(ModelReflectionsState &result)
        {
            result.enabled = false;

            ident *id = getident("envmapmodels");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result.enabled = *id->storage.i != 0;
            return true;
        }

        bool writemodelreflections(
            bool enabled,
            ModelReflectionsState &result)
        {
            if(!readmodelreflections(result))
                return false;

            ident *id = getident("envmapmodels");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            const bool changed = *id->storage.i != value;
            if(!changed)
            {
                if(!readmodelreflections(result))
                    return false;
                return result.enabled == enabled;
            }

            if(!SDL_GL_GetCurrentContext())
                return false;

            setvarchecked(id, value);

            if(!readmodelreflections(result))
                return false;
            return result.enabled == enabled;
        }

        bool readbumpmodels(BumpModelsState &result)
        {
            result.enabled = false;

            ident *id = getident("bumpmodels");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result.enabled = *id->storage.i != 0;
            return true;
        }

        bool writebumpmodels(bool enabled, BumpModelsState &result)
        {
            if(!readbumpmodels(result))
                return false;

            ident *id = getident("bumpmodels");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            const bool changed = *id->storage.i != value;
            if(!changed)
            {
                if(!readbumpmodels(result))
                    return false;
                return result.enabled == enabled;
            }

            if(!SDL_GL_GetCurrentContext())
                return false;

            setvarchecked(id, value);

            if(!readbumpmodels(result))
                return false;
            return result.enabled == enabled;
        }

        static bool environmentmapresolution(
            int exponent,
            EnvironmentMapResolution &level)
        {
            switch(exponent)
            {
                case 4: level = EnvironmentMapResolution::Minimum; return true;
                case 5: level = EnvironmentMapResolution::VeryLow; return true;
                case 6: level = EnvironmentMapResolution::Low; return true;
                case 7: level = EnvironmentMapResolution::Medium; return true;
                case 8: level = EnvironmentMapResolution::High; return true;
                case 9: level = EnvironmentMapResolution::VeryHigh; return true;
                case 10: level = EnvironmentMapResolution::Ultra; return true;
            }
            return false;
        }

        static bool environmentmapresolution(
            EnvironmentMapResolution level,
            int &exponent)
        {
            switch(level)
            {
                case EnvironmentMapResolution::Minimum: exponent = 4; return true;
                case EnvironmentMapResolution::VeryLow: exponent = 5; return true;
                case EnvironmentMapResolution::Low: exponent = 6; return true;
                case EnvironmentMapResolution::Medium: exponent = 7; return true;
                case EnvironmentMapResolution::High: exponent = 8; return true;
                case EnvironmentMapResolution::VeryHigh: exponent = 9; return true;
                case EnvironmentMapResolution::Ultra: exponent = 10; return true;
            }
            return false;
        }

        bool readenvironmentmapresolution(
            EnvironmentMapResolutionState &result)
        {
            result.level = EnvironmentMapResolution::Minimum;
            result.configured_exponent = 0;
            result.requested_dimension = 0;

            ident *id = getident("envmapsize");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 4 ||
               id->maxval != 10 || *id->storage.i < 4 ||
               *id->storage.i > 10 ||
               !environmentmapresolution(*id->storage.i, result.level))
            {
                return false;
            }

            result.configured_exponent = *id->storage.i;
            result.requested_dimension = 1 << result.configured_exponent;
            return true;
        }

        bool writeenvironmentmapresolution(
            EnvironmentMapResolution level,
            EnvironmentMapResolutionState &result)
        {
            if(!readenvironmentmapresolution(result))
                return false;

            int exponent = 0;
            if(!environmentmapresolution(level, exponent))
                return false;
            if(result.configured_exponent == exponent)
                return true;

            ident *id = getident("envmapsize");
            EnvironmentMapResolution current_level;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 4 ||
               id->maxval != 10 || *id->storage.i < 4 ||
               *id->storage.i > 10 ||
               !environmentmapresolution(*id->storage.i, current_level))
            {
                return false;
            }
            if(*id->storage.i == exponent)
            {
                if(!readenvironmentmapresolution(result))
                    return false;
                return result.configured_exponent == exponent &&
                    result.level == level;
            }

            setvarchecked(id, exponent);

            if(!readenvironmentmapresolution(result))
                return false;
            return result.configured_exponent == exponent &&
                result.level == level;
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

        bool readfixtjoints(FixTJointsState &result)
        {
            result.enabled = false;

            ident *id = getident("filltjoints");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result.enabled = *id->storage.i != 0;
            return true;
        }

        bool writefixtjoints(bool enabled, FixTJointsState &result)
        {
            if(!readfixtjoints(result))
                return false;

            ident *id = getident("filltjoints");
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

            if(!readfixtjoints(result))
                return false;
            return result.enabled == enabled;
        }

        static DecalPersistence classifydecalpersistence(
            int fade_start_milliseconds,
            int triangle_budget_per_type)
        {
            if(fade_start_milliseconds == 10000 &&
               triangle_budget_per_type == 1024)
            {
                return DecalPersistence::Short;
            }
            if(fade_start_milliseconds == 60000 &&
               triangle_budget_per_type == 4096)
            {
                return DecalPersistence::Long;
            }
            return DecalPersistence::Custom;
        }

        static bool decalpersistencevalues(
            DecalPersistence persistence,
            int &fade_start_milliseconds,
            int &triangle_budget_per_type)
        {
            switch(persistence)
            {
                case DecalPersistence::Short:
                    fade_start_milliseconds = 10000;
                    triangle_budget_per_type = 1024;
                    return true;
                case DecalPersistence::Long:
                    fade_start_milliseconds = 60000;
                    triangle_budget_per_type = 4096;
                    return true;
                case DecalPersistence::Custom:
                    return false;
            }
            return false;
        }

        bool readdecalpersistence(DecalPersistenceState &result)
        {
            result.persistence = DecalPersistence::Custom;
            result.fade_start_milliseconds = 0;
            result.triangle_budget_per_type = 0;

            ident *fade_id = getident("decalfade");
            ident *limit_id = getident("maxdecaltris");
            if(!fade_id || fade_id->type != ID_VAR || !fade_id->storage.i ||
               !limit_id || limit_id->type != ID_VAR || !limit_id->storage.i)
            {
                return false;
            }

            result.fade_start_milliseconds = *fade_id->storage.i;
            result.triangle_budget_per_type = *limit_id->storage.i;
            result.persistence = classifydecalpersistence(
                result.fade_start_milliseconds,
                result.triangle_budget_per_type);
            return true;
        }

        bool writedecalpersistence(
            DecalPersistence persistence,
            DecalPersistenceState &result)
        {
            if(!readdecalpersistence(result))
                return false;

            int fade_start_milliseconds = 0;
            int triangle_budget_per_type = 0;
            if(!decalpersistencevalues(persistence,
                fade_start_milliseconds, triangle_budget_per_type))
            {
                return false;
            }

            ident *fade_id = getident("decalfade");
            ident *limit_id = getident("maxdecaltris");
            if(!fade_id || fade_id->type != ID_VAR || !fade_id->storage.i ||
               fade_id->flags&IDF_READONLY ||
               fade_start_milliseconds < fade_id->minval ||
               fade_start_milliseconds > fade_id->maxval ||
               !limit_id || limit_id->type != ID_VAR ||
               !limit_id->storage.i || limit_id->flags&IDF_READONLY ||
               triangle_budget_per_type < limit_id->minval ||
               triangle_budget_per_type > limit_id->maxval)
            {
                return false;
            }

            const bool fade_changed =
                *fade_id->storage.i != fade_start_milliseconds;
            const bool limit_changed =
                *limit_id->storage.i != triangle_budget_per_type;
            if(limit_changed && !SDL_GL_GetCurrentContext())
                return false;

            if(fade_changed)
                setvarchecked(fade_id, fade_start_milliseconds);
            if(limit_changed)
                setvarchecked(limit_id, triangle_budget_per_type);

            if(!readdecalpersistence(result))
                return false;
            return result.persistence == persistence &&
                result.fade_start_milliseconds == fade_start_milliseconds &&
                result.triangle_budget_per_type == triangle_budget_per_type;
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

        bool readglareintensity(GlareIntensityState &result)
        {
            result.configured_scale = 0.0f;

            ident *id = getident("glarescale");
            if(!id || id->type != ID_FVAR || !id->storage.f ||
               id->flags&IDF_READONLY || id->minvalf != 0.0f ||
               id->maxvalf != 8.0f || !finitefloat(*id->storage.f) ||
               *id->storage.f < 0.0f || *id->storage.f > 8.0f)
            {
                return false;
            }

            result.configured_scale = *id->storage.f;
            return true;
        }

        bool writeglareintensity(
            float configured_scale,
            GlareIntensityState &result)
        {
            if(!readglareintensity(result))
                return false;
            if(!finitefloat(configured_scale) ||
               configured_scale < 0.0f || configured_scale > 8.0f)
            {
                return false;
            }
            if(result.configured_scale == configured_scale)
                return true;

            ident *id = getident("glarescale");
            if(!id || id->type != ID_FVAR || !id->storage.f ||
               id->flags&IDF_READONLY || id->minvalf != 0.0f ||
               id->maxvalf != 8.0f || !finitefloat(*id->storage.f) ||
               *id->storage.f < 0.0f || *id->storage.f > 8.0f)
            {
                return false;
            }
            if(*id->storage.f == configured_scale)
            {
                result.configured_scale = *id->storage.f;
                return true;
            }

            setfvarchecked(id, configured_scale);

            if(!readglareintensity(result))
                return false;
            return result.configured_scale == configured_scale;
        }

        static bool glareresolution(
            int exponent,
            GlareResolution &level)
        {
            switch(exponent)
            {
                case 6: level = GlareResolution::Minimum; return true;
                case 7: level = GlareResolution::Low; return true;
                case 8: level = GlareResolution::Medium; return true;
                case 9: level = GlareResolution::High; return true;
                case 10: level = GlareResolution::Ultra; return true;
            }
            return false;
        }

        static bool glareresolution(
            GlareResolution level,
            int &exponent)
        {
            switch(level)
            {
                case GlareResolution::Minimum: exponent = 6; return true;
                case GlareResolution::Low: exponent = 7; return true;
                case GlareResolution::Medium: exponent = 8; return true;
                case GlareResolution::High: exponent = 9; return true;
                case GlareResolution::Ultra: exponent = 10; return true;
            }
            return false;
        }

        bool readglareresolution(GlareResolutionState &result)
        {
            result.level = GlareResolution::Minimum;
            result.configured_exponent = 0;
            result.requested_width = 0;

            ident *id = getident("glaresize");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 6 ||
               id->maxval != 10 || *id->storage.i < 6 ||
               *id->storage.i > 10 ||
               !glareresolution(*id->storage.i, result.level))
            {
                return false;
            }

            result.configured_exponent = *id->storage.i;
            result.requested_width = 1 << result.configured_exponent;
            return true;
        }

        bool writeglareresolution(
            GlareResolution level,
            GlareResolutionState &result)
        {
            if(!readglareresolution(result))
                return false;

            int exponent = 0;
            if(!glareresolution(level, exponent))
                return false;
            if(result.configured_exponent == exponent)
                return true;

            if(!SDL_GL_GetCurrentContext())
                return false;

            ident *id = getident("glaresize");
            GlareResolution current_level;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 6 ||
               id->maxval != 10 || *id->storage.i < 6 ||
               *id->storage.i > 10 ||
               !glareresolution(*id->storage.i, current_level))
            {
                return false;
            }
            if(*id->storage.i == exponent)
            {
                if(!readglareresolution(result))
                    return false;
                return result.level == level &&
                    result.configured_exponent == exponent &&
                    result.requested_width == (1 << exponent);
            }

            setvarchecked(id, exponent);

            if(!readglareresolution(result))
                return false;
            return result.level == level &&
                result.configured_exponent == exponent &&
                result.requested_width == (1 << exponent);
        }

        static TextureQuality classifytexturequality(int texture_size_limit)
        {
            switch(texture_size_limit)
            {
                case 256: return TextureQuality::Low;
                case 512: return TextureQuality::Medium;
                case 0: return TextureQuality::Full;
                default: return TextureQuality::Custom;
            }
        }

        bool readtexturequality(TextureQualityState &result)
        {
            result.quality = TextureQuality::Custom;
            result.texture_size_limit = 0;

            ident *id = getident("maxtexsize");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result.texture_size_limit = *id->storage.i;
            result.quality = classifytexturequality(
                result.texture_size_limit);
            return true;
        }

        bool writetexturequality(
            TextureQuality quality,
            TextureQualityState &result)
        {
            if(!readtexturequality(result))
                return false;

            int target = 0;
            switch(quality)
            {
                case TextureQuality::Low: target = 256; break;
                case TextureQuality::Medium: target = 512; break;
                case TextureQuality::Full: target = 0; break;
                case TextureQuality::Custom: return false;
                default: return false;
            }

            ident *id = getident("maxtexsize");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY ||
               target < id->minval || target > id->maxval)
            {
                return false;
            }

            const bool changed = *id->storage.i != target;
            if(!changed)
            {
                if(!readtexturequality(result))
                    return false;
                return result.texture_size_limit == target &&
                    result.quality == quality;
            }

            setvarchecked(id, target);

            if(!readtexturequality(result))
                return false;
            return result.texture_size_limit == target &&
                result.quality == quality;
        }

        bool readtexturereduction(TextureReductionState &result)
        {
            result.levels = 0;
            result.minimum = 0;
            result.maximum = 0;

            ident *id = getident("texreduce");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 0 ||
               id->maxval != 12 || *id->storage.i < id->minval ||
               *id->storage.i > id->maxval)
            {
                return false;
            }

            result.levels = *id->storage.i;
            result.minimum = id->minval;
            result.maximum = id->maxval;
            return true;
        }

        bool writetexturereduction(
            int levels,
            TextureReductionState &result)
        {
            if(!readtexturereduction(result) || levels < result.minimum ||
               levels > result.maximum)
            {
                return false;
            }

            if(levels == result.levels)
                return true;

            ident *id = getident("texreduce");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != result.minimum ||
               id->maxval != result.maximum)
            {
                return false;
            }

            setvarchecked(id, levels);

            if(!readtexturereduction(result))
                return false;
            return result.levels == levels;
        }

        static TextureCompressionPreset classifytexturecompression(
            int threshold)
        {
            switch(threshold)
            {
                case 0: return TextureCompressionPreset::Off;
                case 1: return TextureCompressionPreset::AllEligible;
                case 256: return TextureCompressionPreset::AtLeast256;
                case 512: return TextureCompressionPreset::AtLeast512;
                case 1024: return TextureCompressionPreset::AtLeast1024;
                case 1025: return TextureCompressionPreset::Above1024;
                case 2048: return TextureCompressionPreset::AtLeast2048;
                case 4096: return TextureCompressionPreset::AtLeast4096;
                default: return TextureCompressionPreset::Custom;
            }
        }

        bool readtexturecompression(TextureCompressionState &result)
        {
            result.preset = TextureCompressionPreset::Custom;
            result.threshold = 0;
            result.minimum = 0;
            result.maximum = 0;
            result.supported = false;

            ident *id = getident("texcompress");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 0 ||
               id->maxval != 4096 || *id->storage.i < id->minval ||
               *id->storage.i > id->maxval)
            {
                return false;
            }

            ident *capability_id = getident("usetexcompress");
            if(!capability_id || capability_id->type != ID_VAR ||
               !capability_id->storage.i ||
               !(capability_id->flags&IDF_READONLY) ||
               *capability_id->storage.i < 0 ||
               *capability_id->storage.i > 2)
            {
                return false;
            }

            result.threshold = *id->storage.i;
            result.minimum = id->minval;
            result.maximum = id->maxval;
            result.supported = *capability_id->storage.i != 0;
            result.preset = classifytexturecompression(result.threshold);
            return true;
        }

        bool writetexturecompression(
            int threshold,
            TextureCompressionState &result)
        {
            if(!readtexturecompression(result) ||
               threshold < result.minimum || threshold > result.maximum ||
               !result.supported)
            {
                return false;
            }

            if(threshold == result.threshold)
                return true;

            ident *id = getident("texcompress");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != result.minimum ||
               id->maxval != result.maximum)
            {
                return false;
            }

            setvarchecked(id, threshold);

            if(!readtexturecompression(result))
                return false;
            return result.threshold == threshold;
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

        bool readbilinearfiltering(BilinearFilteringState &result)
        {
            result.enabled = false;

            ident *id = getident("bilinear");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 0 || id->maxval != 1 ||
               (*id->storage.i != 0 && *id->storage.i != 1))
            {
                return false;
            }

            result.enabled = *id->storage.i != 0;
            return true;
        }

        bool writebilinearfiltering(
            bool enabled,
            BilinearFilteringState &result)
        {
            if(!readbilinearfiltering(result))
                return false;

            ident *id = getident("bilinear");
            const int value = enabled ? 1 : 0;
            if(!id || id->type != ID_VAR || !id->storage.i ||
               id->flags&IDF_READONLY || id->minval != 0 || id->maxval != 1)
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readbilinearfiltering(result))
                return false;
            return result.enabled == enabled;
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
            ident *channels_id = soundchansident();
            ident *frequency_id = soundfreqident();
            ident *buffer_id = soundbufferlenident();
            ident *mumble_id = mumbleident();
            if(!sound_id || !sound_id->storage.i ||
               !music_id || !music_id->storage.i ||
               !soundpack_id || !soundpack_id->storage.s ||
               !*soundpack_id->storage.s ||
               !channels_id || !channels_id->storage.i ||
               !frequency_id || !frequency_id->storage.i ||
               !buffer_id || !buffer_id->storage.i ||
               !mumble_id || !mumble_id->storage.i)
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
            result.sound_channels = *channels_id->storage.i;
            result.sound_channels_minimum = channels_id->minval;
            result.sound_channels_maximum = channels_id->maxval;
            result.effective_sound_channels = ::nosound ? 0 : maxchannels;
            result.sound_frequency = *frequency_id->storage.i;
            result.effective_sound_frequency = 0;
            if(!::nosound)
            {
                int frequency = 0;
                Uint16 format = 0;
                int mixer_channels = 0;
                if(Mix_QuerySpec(&frequency, &format, &mixer_channels))
                    result.effective_sound_frequency = frequency;
            }
            result.sound_buffer_length = *buffer_id->storage.i;
            result.sound_buffer_length_minimum = buffer_id->minval;
            result.sound_buffer_length_maximum = buffer_id->maxval;
            result.mumble = *mumble_id->storage.i != 0;
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

            if(*id->storage.i != value)
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

            if(*id->storage.i != value)
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

        static bool writeaudiointeger(ident *id, int value, Audio &result)
        {
            if(!id || !id->storage.i ||
               value < id->minval || value > id->maxval)
            {
                readaudio(result);
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);
            if(!readaudio(result))
                return false;
            return *id->storage.i == value;
        }

        bool writesoundchannels(int value, Audio &result)
        {
            return writeaudiointeger(soundchansident(), value, result);
        }

        bool writesoundfrequency(int value, Audio &result)
        {
            if(!nativesoundfrequency(value))
            {
                readaudio(result);
                return false;
            }

            return writeaudiointeger(soundfreqident(), value, result);
        }

        bool writesoundbufferlength(int value, Audio &result)
        {
            return writeaudiointeger(soundbufferlenident(), value, result);
        }

        bool writemumble(bool enabled, Audio &result)
        {
            ident *id = mumbleident();
            if(!id || !id->storage.i)
            {
                readaudio(result);
                return false;
            }

            const int value = enabled ? 1 : 0;
            if(value < id->minval || value > id->maxval)
            {
                readaudio(result);
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);
            if(!readaudio(result))
                return false;
            return (*id->storage.i != 0) == enabled;
        }

        bool hassoundchanges()
        {
            return haschanges(CHANGE_SOUND);
        }

        void resetsound()
        {
            ::resetsound();
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
            ident *fps_id = showfpsident();
            ident *clock_id = wallclockident();
            ident *clock24_id = wallclock24ident();
            ident *clocksecs_id = wallclocksecsident();
            ident *screen_id = damagescreenident();
            ident *compass_id = damagecompassident();
            ident *guns_id = hudgunident();
            ident *sway_id = hudgunswayident();
            ident *flash_id = muzzleflashident();
            ident *light_id = muzzlelightident();
            ident *messages_id = fragmessagesident();
            ident *max_id = maxhudfragmessagesident();
            ident *duration_id = hudfragmessagedurationident();
            ident *filter_id = hudfragmessagefilterident();
            ident *scale_id = hudfragmessagescaleident();
            ident *fade_id = hudfragmessagefadeident();
            ident *x_id = hudfragmessagexident();
            ident *y_id = hudfragmessageyident();
            ident *stack_id = hudfragmessagestackdirident();
            ident *streak_id = streakmilestonescaleident();
            if(!fps_id || !fps_id->storage.i ||
               !clock_id || !clock_id->storage.i ||
               !clock24_id || !clock24_id->storage.i ||
               !clocksecs_id || !clocksecs_id->storage.i ||
               !screen_id || !screen_id->storage.i ||
               !compass_id || !compass_id->storage.i ||
               !guns_id || !guns_id->storage.i ||
               !sway_id || !sway_id->storage.i ||
               !flash_id || !flash_id->storage.i ||
               !light_id || !light_id->storage.i ||
               !messages_id || !messages_id->storage.i ||
               !max_id || !max_id->storage.i ||
               !duration_id || !duration_id->storage.f ||
               !filter_id || !filter_id->storage.i ||
               !scale_id || !scale_id->storage.f ||
               !fade_id || !fade_id->storage.i ||
               !x_id || !x_id->storage.f ||
               !y_id || !y_id->storage.f ||
               !stack_id || !stack_id->storage.i ||
               !streak_id || !streak_id->storage.f)
            {
                return false;
            }

            result.show_fps = *fps_id->storage.i != 0;
            result.wall_clock = *clock_id->storage.i != 0;
            result.wall_clock_24_hour = *clock24_id->storage.i != 0;
            result.wall_clock_seconds = *clocksecs_id->storage.i != 0;
            result.damage_screen = *screen_id->storage.i != 0;
            result.damage_compass = *compass_id->storage.i != 0;
            result.hud_guns = *guns_id->storage.i != 0;
            result.hud_gun_sway = *sway_id->storage.i != 0;
            result.muzzle_flash = *flash_id->storage.i != 0;
            result.muzzle_light = *light_id->storage.i != 0;
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
            result.max_frag_messages = *max_id->storage.i;
            result.max_frag_messages_minimum = max_id->minval;
            result.max_frag_messages_maximum = max_id->maxval;
            result.frag_message_duration = *duration_id->storage.f;
            result.frag_message_duration_minimum = duration_id->minvalf;
            result.frag_message_duration_maximum = duration_id->maxvalf;
            result.frag_message_self_frags =
                (*filter_id->storage.i & CON_FRAG_SELF) != 0;
            result.frag_message_other_frags =
                (*filter_id->storage.i & CON_FRAG_OTHER) != 0;
            result.frag_message_team_kills =
                (*filter_id->storage.i & CON_TEAMKILL) != 0;
            result.frag_message_scale = *scale_id->storage.f;
            result.frag_message_scale_minimum = scale_id->minvalf;
            result.frag_message_scale_maximum = scale_id->maxvalf;
            result.frag_message_fade = *fade_id->storage.i != 0;
            result.frag_message_x = *x_id->storage.f;
            result.frag_message_x_minimum = x_id->minvalf;
            result.frag_message_x_maximum = x_id->maxvalf;
            result.frag_message_y = *y_id->storage.f;
            result.frag_message_y_minimum = y_id->minvalf;
            result.frag_message_y_maximum = y_id->maxvalf;
            result.frag_message_stack_dir = *stack_id->storage.i;
            result.streak_milestone_scale = *streak_id->storage.f;
            result.streak_milestone_scale_minimum = streak_id->minvalf;
            result.streak_milestone_scale_maximum = streak_id->maxvalf;
            return true;
        }

        bool writeshowfps(bool enabled)
        {
            return writeboolean(showfpsident(), enabled);
        }

        bool writewallclock(bool enabled)
        {
            return writeboolean(wallclockident(), enabled);
        }

        bool writewallclock24hour(bool enabled)
        {
            return writeboolean(wallclock24ident(), enabled);
        }

        bool writewallclockseconds(bool enabled)
        {
            return writeboolean(wallclocksecsident(), enabled);
        }

        bool writedamagescreen(bool enabled)
        {
            return writeboolean(damagescreenident(), enabled);
        }

        bool writedamagecompass(bool enabled)
        {
            return writeboolean(damagecompassident(), enabled);
        }

        bool writehudguns(bool enabled)
        {
            return writeboolean(hudgunident(), enabled);
        }

        bool writehudgunsway(bool enabled)
        {
            return writeboolean(hudgunswayident(), enabled);
        }

        bool writemuzzleflash(bool enabled)
        {
            return writeboolean(muzzleflashident(), enabled);
        }

        bool writemuzzlelight(bool enabled)
        {
            return writeboolean(muzzlelightident(), enabled);
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

        static bool writehudboolean(ident *id, bool enabled, Hud &result)
        {
            if(!readhud(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            const int value = enabled ? 1 : 0;
            if(value < id->minval || value > id->maxval)
                return false;

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readhud(result))
                return false;
            return (*id->storage.i != 0) == enabled;
        }

        static bool writehudinteger(ident *id, int value, Hud &result)
        {
            if(!readhud(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readhud(result))
                return false;
            return *id->storage.i == value;
        }

        static bool writehudfloat(ident *id, float value, Hud &result)
        {
            if(!readhud(result))
                return false;

            if(!id || !id->storage.f || id->flags&IDF_READONLY ||
               !finitefloat(value))
            {
                return false;
            }

            float clamped = value;
            if(clamped < id->minvalf)
                clamped = id->minvalf;
            else if(clamped > id->maxvalf)
                clamped = id->maxvalf;

            if(*id->storage.f == clamped)
                return true;

            setfvarchecked(id, value);
            if(!readhud(result))
                return false;
            return true;
        }

        static bool writehudfilterbit(int bit, bool enabled, Hud &result)
        {
            if(!readhud(result))
                return false;

            if(!writefilterbit(hudfragmessagefilterident(), bit, enabled))
                return false;

            return readhud(result);
        }

        bool writemaxfragmessages(int value, Hud &result)
        {
            return writehudinteger(maxhudfragmessagesident(), value, result);
        }

        bool writefragmessageduration(float value, Hud &result)
        {
            return writehudfloat(
                hudfragmessagedurationident(),
                value,
                result);
        }

        bool writefragmessageselffrags(bool enabled, Hud &result)
        {
            return writehudfilterbit(CON_FRAG_SELF, enabled, result);
        }

        bool writefragmessageotherfrags(bool enabled, Hud &result)
        {
            return writehudfilterbit(CON_FRAG_OTHER, enabled, result);
        }

        bool writefragmessageteamkills(bool enabled, Hud &result)
        {
            return writehudfilterbit(CON_TEAMKILL, enabled, result);
        }

        bool writefragmessagescale(float value, Hud &result)
        {
            return writehudfloat(hudfragmessagescaleident(), value, result);
        }

        bool writefragmessagefade(bool enabled, Hud &result)
        {
            return writehudboolean(hudfragmessagefadeident(), enabled, result);
        }

        bool writefragmessagex(float value, Hud &result)
        {
            return writehudfloat(hudfragmessagexident(), value, result);
        }

        bool writefragmessagey(float value, Hud &result)
        {
            return writehudfloat(hudfragmessageyident(), value, result);
        }

        bool writefragmessagestackdir(int value, Hud &result)
        {
            if(value != -1 && value != 0 && value != 1)
            {
                readhud(result);
                return false;
            }

            return writehudinteger(
                hudfragmessagestackdirident(),
                value,
                result);
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

        bool readammobar(Ammobar &result)
        {
            ident *visible_id = ammobarident();
            ident *horizontal_id = ammobarhorizontalident();
            ident *flip_id = ammobarflipident();
            ident *scale_id = ammobarscaleident();
            ident *sep_id = ammobarsepident();
            ident *count_scale_id = ammobarcountscaleident();
            ident *count_sep_id = ammobarcountsepident();
            ident *align_id = ammobaralignident();
            ident *x_id = ammobarxident();
            ident *y_id = ammobaryident();
            ident *hide_empty_id = ammobarhideemptyident();
            if(!visible_id || !visible_id->storage.i ||
               !horizontal_id || !horizontal_id->storage.i ||
               !flip_id || !flip_id->storage.i ||
               !scale_id || !scale_id->storage.f ||
               !sep_id || !sep_id->storage.i ||
               !count_scale_id || !count_scale_id->storage.f ||
               !count_sep_id || !count_sep_id->storage.i ||
               !align_id || !align_id->storage.i ||
               !x_id || !x_id->storage.f ||
               !y_id || !y_id->storage.f ||
               !hide_empty_id || !hide_empty_id->storage.i)
            {
                return false;
            }

            result.visible = *visible_id->storage.i != 0;
            result.horizontal = *horizontal_id->storage.i != 0;
            result.flip = *flip_id->storage.i != 0;
            result.scale = *scale_id->storage.f;
            result.scale_minimum = scale_id->minvalf;
            result.scale_maximum = scale_id->maxvalf;
            result.icon_spacing = *sep_id->storage.i;
            result.icon_spacing_minimum = sep_id->minval;
            result.icon_spacing_maximum = sep_id->maxval;
            result.label_size = *count_scale_id->storage.f;
            result.label_size_minimum = count_scale_id->minvalf;
            result.label_size_maximum = count_scale_id->maxvalf;
            result.label_offset = *count_sep_id->storage.i;
            result.label_offset_minimum = count_sep_id->minval;
            result.label_offset_maximum = count_sep_id->maxval;
            result.align = *align_id->storage.i;
            result.x = *x_id->storage.f;
            result.x_minimum = x_id->minvalf;
            result.x_maximum = x_id->maxvalf;
            result.y = *y_id->storage.f;
            result.y_minimum = y_id->minvalf;
            result.y_maximum = y_id->maxvalf;
            result.hide_empty = *hide_empty_id->storage.i != 0;
            return true;
        }

        static bool writeammobarboolean(
            ident *id,
            bool enabled,
            Ammobar &result)
        {
            if(!readammobar(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            const int value = enabled ? 1 : 0;
            if(value < id->minval || value > id->maxval)
                return false;

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readammobar(result))
                return false;
            return (*id->storage.i != 0) == enabled;
        }

        static bool writeammobarinteger(
            ident *id,
            int value,
            Ammobar &result)
        {
            if(!readammobar(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readammobar(result))
                return false;
            return *id->storage.i == value;
        }

        static bool writeammobarfloat(
            ident *id,
            float value,
            Ammobar &result)
        {
            if(!readammobar(result))
                return false;

            if(!id || !id->storage.f || id->flags&IDF_READONLY ||
               !finitefloat(value))
            {
                return false;
            }

            float clamped = value;
            if(clamped < id->minvalf)
                clamped = id->minvalf;
            else if(clamped > id->maxvalf)
                clamped = id->maxvalf;

            if(*id->storage.f == clamped)
                return true;

            setfvarchecked(id, value);
            if(!readammobar(result))
                return false;
            return true;
        }

        bool writeammobar(bool enabled, Ammobar &result)
        {
            return writeammobarboolean(ammobarident(), enabled, result);
        }

        bool writeammobarhorizontal(bool enabled, Ammobar &result)
        {
            return writeammobarboolean(
                ammobarhorizontalident(),
                enabled,
                result);
        }

        bool writeammobarflip(bool enabled, Ammobar &result)
        {
            return writeammobarboolean(ammobarflipident(), enabled, result);
        }

        bool writeammobarscale(float value, Ammobar &result)
        {
            return writeammobarfloat(ammobarscaleident(), value, result);
        }

        bool writeammobarsep(int value, Ammobar &result)
        {
            return writeammobarinteger(ammobarsepident(), value, result);
        }

        bool writeammobarcountscale(float value, Ammobar &result)
        {
            return writeammobarfloat(
                ammobarcountscaleident(),
                value,
                result);
        }

        bool writeammobarcountsep(int value, Ammobar &result)
        {
            return writeammobarinteger(ammobarcountsepident(), value, result);
        }

        bool writeammobaralign(int value, Ammobar &result)
        {
            if(value != -1 && value != 0 && value != 1)
            {
                readammobar(result);
                return false;
            }

            return writeammobarinteger(ammobaralignident(), value, result);
        }

        bool writeammobarx(float value, Ammobar &result)
        {
            return writeammobarfloat(ammobarxident(), value, result);
        }

        bool writeammobary(float value, Ammobar &result)
        {
            return writeammobarfloat(ammobaryident(), value, result);
        }

        bool writeammobarhideempty(bool enabled, Ammobar &result)
        {
            return writeammobarboolean(
                ammobarhideemptyident(),
                enabled,
                result);
        }

        static void cleargameclock(GameClock &result)
        {
            result.enabled = false;
            result.scale = 1.0f;
            result.scale_minimum = 0.5f;
            result.scale_maximum = 2.0f;
            result.alpha = 255;
            result.alpha_minimum = 0;
            result.alpha_maximum = 255;
            result.align = 0;
            result.x = 0.50f;
            result.x_minimum = 0.0f;
            result.x_maximum = 1.0f;
            result.y = 0.03f;
            result.y_minimum = 0.0f;
            result.y_maximum = 1.0f;
        }

        bool readgameclock(GameClock &result)
        {
            cleargameclock(result);

            ident *enabled_id = gameclockident();
            ident *scale_id = gameclockscaleident();
            ident *alpha_id = gameclockalphaident();
            ident *align_id = gameclockalignident();
            ident *x_id = gameclockxident();
            ident *y_id = gameclockyident();
            if(!enabled_id || !enabled_id->storage.i ||
               !scale_id || !scale_id->storage.f ||
               !alpha_id || !alpha_id->storage.i ||
               !align_id || !align_id->storage.i ||
               !x_id || !x_id->storage.f ||
               !y_id || !y_id->storage.f)
            {
                return false;
            }

            result.enabled = *enabled_id->storage.i != 0;
            result.scale = *scale_id->storage.f;
            result.scale_minimum = scale_id->minvalf;
            result.scale_maximum = scale_id->maxvalf;
            result.alpha = *alpha_id->storage.i;
            result.alpha_minimum = alpha_id->minval;
            result.alpha_maximum = alpha_id->maxval;
            result.align = *align_id->storage.i;
            result.x = *x_id->storage.f;
            result.x_minimum = x_id->minvalf;
            result.x_maximum = x_id->maxvalf;
            result.y = *y_id->storage.f;
            result.y_minimum = y_id->minvalf;
            result.y_maximum = y_id->maxvalf;
            return true;
        }

        static bool writegameclockboolean(
            ident *id,
            bool enabled,
            GameClock &result)
        {
            if(!readgameclock(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            const int value = enabled ? 1 : 0;
            if(value < id->minval || value > id->maxval)
                return false;

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readgameclock(result))
                return false;
            return (*id->storage.i != 0) == enabled;
        }

        static bool writegameclockint(
            ident *id,
            int value,
            GameClock &result)
        {
            if(!readgameclock(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i == value)
                return true;

            setvarchecked(id, value);
            if(!readgameclock(result))
                return false;
            return *id->storage.i == value;
        }

        static bool writegameclockfloat(
            ident *id,
            float value,
            GameClock &result)
        {
            if(!readgameclock(result))
                return false;

            if(!id || !id->storage.f || id->flags&IDF_READONLY ||
               !finitefloat(value))
            {
                return false;
            }

            if(*id->storage.f == value)
                return true;

            setfvarchecked(id, value);
            if(!readgameclock(result))
                return false;
            return finitefloat(*id->storage.f);
        }

        bool writegameclock(bool enabled, GameClock &result)
        {
            return writegameclockboolean(
                gameclockident(),
                enabled,
                result);
        }

        bool writegameclockscale(float value, GameClock &result)
        {
            return writegameclockfloat(
                gameclockscaleident(),
                value,
                result);
        }

        bool writegameclockalpha(int value, GameClock &result)
        {
            return writegameclockint(
                gameclockalphaident(),
                value,
                result);
        }

        bool writegameclockalign(int value, GameClock &result)
        {
            if(!isgameclockalign(value))
            {
                readgameclock(result);
                return false;
            }

            return writegameclockint(
                gameclockalignident(),
                value,
                result);
        }

        bool writegameclockx(float value, GameClock &result)
        {
            return writegameclockfloat(
                gameclockxident(),
                value,
                result);
        }

        bool writegameclocky(float value, GameClock &result)
        {
            return writegameclockfloat(
                gameclockyident(),
                value,
                result);
        }

        static void defaulthudscore(HudScore &result)
        {
            result.enabled = false;
            result.scale = 1.0f;
            result.scale_minimum = 0.5f;
            result.scale_maximum = 2.0f;
            result.alpha = 255;
            result.alpha_minimum = 0;
            result.alpha_maximum = 255;
            result.spacing = 200;
            result.spacing_minimum = 0;
            result.spacing_maximum = 1000;
            result.align = 0;
            result.x = 0.50f;
            result.x_minimum = 0.0f;
            result.x_maximum = 1.0f;
            result.y = 0.03f;
            result.y_minimum = 0.0f;
            result.y_maximum = 1.0f;
        }

        bool readhudscore(HudScore &result)
        {
            defaulthudscore(result);

            ident *enabled_id = hudscoreident();
            ident *scale_id = hudscorescaleident();
            ident *alpha_id = hudscorealphaident();
            ident *sep_id = hudscoresepident();
            ident *align_id = hudscorealignident();
            ident *x_id = hudscorexident();
            ident *y_id = hudscoreyident();
            if(!enabled_id || !enabled_id->storage.i ||
               !scale_id || !scale_id->storage.f ||
               !alpha_id || !alpha_id->storage.i ||
               !sep_id || !sep_id->storage.i ||
               !align_id || !align_id->storage.i ||
               !x_id || !x_id->storage.f ||
               !y_id || !y_id->storage.f)
            {
                return false;
            }

            result.enabled = *enabled_id->storage.i != 0;
            result.scale = *scale_id->storage.f;
            result.scale_minimum = scale_id->minvalf;
            result.scale_maximum = scale_id->maxvalf;
            result.alpha = *alpha_id->storage.i;
            result.alpha_minimum = alpha_id->minval;
            result.alpha_maximum = alpha_id->maxval;
            result.spacing = *sep_id->storage.i;
            result.spacing_minimum = sep_id->minval;
            result.spacing_maximum = sep_id->maxval;
            result.align = *align_id->storage.i;
            result.x = *x_id->storage.f;
            result.x_minimum = x_id->minvalf;
            result.x_maximum = x_id->maxvalf;
            result.y = *y_id->storage.f;
            result.y_minimum = y_id->minvalf;
            result.y_maximum = y_id->maxvalf;
            return true;
        }

        static bool writehudscoreboolean(
            ident *id,
            bool enabled,
            HudScore &result)
        {
            if(!readhudscore(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            const int value = enabled ? 1 : 0;
            if(value < id->minval || value > id->maxval)
                return false;

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readhudscore(result))
                return false;
            return (*id->storage.i != 0) == enabled;
        }

        static bool writehudscoreint(
            ident *id,
            int value,
            HudScore &result)
        {
            if(!readhudscore(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i == value)
                return true;

            setvarchecked(id, value);
            if(!readhudscore(result))
                return false;
            return *id->storage.i == value;
        }

        static bool writehudscorefloat(
            ident *id,
            float value,
            HudScore &result)
        {
            if(!readhudscore(result))
                return false;

            if(!id || !id->storage.f || id->flags&IDF_READONLY ||
               !finitefloat(value) ||
               value < id->minvalf || value > id->maxvalf)
            {
                return false;
            }

            if(*id->storage.f == value)
                return true;

            setfvarchecked(id, value);
            if(!readhudscore(result))
                return false;
            return *id->storage.f == value;
        }

        bool writehudscore(bool enabled, HudScore &result)
        {
            return writehudscoreboolean(hudscoreident(), enabled, result);
        }

        bool writehudscorescale(float value, HudScore &result)
        {
            return writehudscorefloat(hudscorescaleident(), value, result);
        }

        bool writehudscorealpha(int value, HudScore &result)
        {
            return writehudscoreint(hudscorealphaident(), value, result);
        }

        bool writehudscoresep(int value, HudScore &result)
        {
            return writehudscoreint(hudscoresepident(), value, result);
        }

        bool writehudscorealign(int value, HudScore &result)
        {
            if(value != -1 && value != 0 && value != 1)
            {
                if(!readhudscore(result))
                    return false;
                return false;
            }

            return writehudscoreint(hudscorealignident(), value, result);
        }

        bool writehudscorex(float value, HudScore &result)
        {
            return writehudscorefloat(hudscorexident(), value, result);
        }

        bool writehudscorey(float value, HudScore &result)
        {
            return writehudscorefloat(hudscoreyident(), value, result);
        }

        bool readscoreboardcolumns(ScoreboardColumns &result)
        {
            result.show_flags = false;
            result.show_frags = false;
            result.show_deaths = false;
            result.show_suicides = false;
            result.show_kpd = false;
            result.show_accuracy = false;
            result.show_damage = false;
            result.showdamage = 0;
            result.show_damage_dealt = false;
            result.show_damage_net = false;
            result.show_damage_received = false;
            result.show_ping = false;
            result.showping = 0;
            result.show_pj = false;
            result.show_clientnum = false;

            ident *flags_id = showflagsident();
            ident *frags_id = hidefragsident();
            ident *deaths_id = showdeathsident();
            ident *suicides_id = showsuicidesident();
            ident *kpd_id = showkpdident();
            ident *accuracy_id = showaccuracyident();
            ident *damage_id = showdamageident();
            ident *received_id = showdamagereceivedident();
            ident *ping_id = showpingident();
            ident *pj_id = showpjident();
            ident *clientnum_id = showclientnumident();
            if(!flags_id || !flags_id->storage.i ||
               !frags_id || !frags_id->storage.i ||
               !deaths_id || !deaths_id->storage.i ||
               !suicides_id || !suicides_id->storage.i ||
               !kpd_id || !kpd_id->storage.i ||
               !accuracy_id || !accuracy_id->storage.i ||
               !damage_id || !damage_id->storage.i ||
               !received_id || !received_id->storage.i ||
               !ping_id || !ping_id->storage.i ||
               !pj_id || !pj_id->storage.i ||
               !clientnum_id || !clientnum_id->storage.i)
            {
                return false;
            }

            result.show_flags = *flags_id->storage.i != 0;
            // Native guicheckbox "frags" [hidefrags] 0 1: checked when != off(1).
            result.show_frags = *frags_id->storage.i != 1;
            result.show_deaths = *deaths_id->storage.i != 0;
            result.show_suicides = *suicides_id->storage.i != 0;
            result.show_kpd = *kpd_id->storage.i != 0;
            result.show_accuracy = *accuracy_id->storage.i != 0;
            result.showdamage = *damage_id->storage.i;
            result.show_damage = result.showdamage != 0;
            result.show_damage_dealt = result.showdamage == 1;
            result.show_damage_net = result.showdamage == 2;
            result.show_damage_received = *received_id->storage.i != 0;
            result.showping = *ping_id->storage.i;
            result.show_ping = result.showping != 0;
            result.show_pj = *pj_id->storage.i != 0;
            result.show_clientnum = *clientnum_id->storage.i != 0;
            return true;
        }

        static bool writescoreboardboolean(
            ident *id,
            bool enabled,
            ScoreboardColumns &result)
        {
            if(!readscoreboardcolumns(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            const int value = enabled ? 1 : 0;
            if(value < id->minval || value > id->maxval)
                return false;

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readscoreboardcolumns(result))
                return false;
            return (*id->storage.i != 0) == enabled;
        }

        // Native Settings checkboxes with a dedicated on-value only write
        // when the user toggles. A non-off engine value stays put on open
        // and on a no-op "checked" event.
        static bool writescoreboardonoff(
            ident *id,
            bool enabled,
            int on_value,
            int off_value,
            ScoreboardColumns &result)
        {
            if(!readscoreboardcolumns(result))
                return false;

            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            if(enabled)
            {
                if(*id->storage.i == off_value)
                {
                    if(on_value < id->minval || on_value > id->maxval)
                        return false;
                    setvarchecked(id, on_value);
                }
            }
            else if(*id->storage.i != off_value)
            {
                if(off_value < id->minval || off_value > id->maxval)
                    return false;
                setvarchecked(id, off_value);
            }

            if(!readscoreboardcolumns(result))
                return false;
            return enabled
                ? *id->storage.i != off_value
                : *id->storage.i == off_value;
        }

        bool writeshowflags(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardboolean(showflagsident(), enabled, result);
        }

        bool writehidefrags(bool show_column, ScoreboardColumns &result)
        {
            if(!readscoreboardcolumns(result))
                return false;

            ident *id = hidefragsident();
            if(!id || !id->storage.i || id->flags&IDF_READONLY)
                return false;

            const int value = show_column ? 0 : 1;
            if(value < id->minval || value > id->maxval)
                return false;

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readscoreboardcolumns(result))
                return false;
            return result.show_frags == show_column;
        }

        bool writeshowdeaths(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardboolean(
                showdeathsident(),
                enabled,
                result);
        }

        bool writeshowsuicides(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardboolean(
                showsuicidesident(),
                enabled,
                result);
        }

        bool writeshowkpd(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardboolean(showkpdident(), enabled, result);
        }

        bool writeshowaccuracy(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardboolean(
                showaccuracyident(),
                enabled,
                result);
        }

        bool writeshowdamage(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardonoff(
                showdamageident(),
                enabled,
                1,
                0,
                result);
        }

        bool writeshowdamagemode(int value, ScoreboardColumns &result)
        {
            if(!readscoreboardcolumns(result))
                return false;

            ident *id = showdamageident();
            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               (value != 1 && value != 2) ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i == value)
                return true;

            setvarchecked(id, value);
            if(!readscoreboardcolumns(result))
                return false;
            return result.showdamage == value;
        }

        bool writeshowdamagereceived(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardboolean(
                showdamagereceivedident(),
                enabled,
                result);
        }

        bool writeshowping(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardonoff(
                showpingident(),
                enabled,
                1,
                0,
                result);
        }

        bool writeshowpj(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardboolean(showpjident(), enabled, result);
        }

        bool writeshowclientnum(bool enabled, ScoreboardColumns &result)
        {
            return writescoreboardboolean(
                showclientnumident(),
                enabled,
                result);
        }

        static bool migratelegacyautoignore()
        {
            ident *configured_id = autoignoreconfiguredident();
            ident *enabled_id = autoignoreenabledident();
            if(!configured_id || !configured_id->storage.i ||
               !enabled_id || !enabled_id->storage.i)
            {
                return false;
            }
            if(*configured_id->storage.i)
                return true;

            char imported[Chat::AutoIgnoreSlotCount][Chat::AutoIgnoreNameSize] = {};
            bool found_legacy_name = false;
            loopi(5)
            {
                string alias;
                formatstring(alias, "ai_name_%d", i);
                const char *legacy = getalias(alias);
                if(!legacy[0])
                    continue;
                filtertext(imported[i], legacy, false, false, MAXNAMELEN);
                found_legacy_name = found_legacy_name || imported[i][0];
            }

            // Do not claim migration until an old AutoIgnore list is present;
            // autoexec.cfg may not have executed yet during early startup reads.
            if(!found_legacy_name)
                return true;

            loopi(Chat::AutoIgnoreSlotCount)
            {
                ident *name_id = autoignorenameident(i);
                if(!name_id || !name_id->storage.s)
                    return false;
                setsvarchecked(name_id, imported[i]);
            }

            const char *legacy_enabled = getalias("ai_enabled");
            setvarchecked(enabled_id,
                !legacy_enabled[0] || parseint(legacy_enabled) != 0 ? 1 : 0);
            setvarchecked(configured_id, 1);
            writecfg();
            return true;
        }

        bool readchat(Chat &result)
        {
            std::memset(&result, 0, sizeof(result));
            if(!migratelegacyautoignore())
                return false;

            ident *top_id = consolefilterident();
            ident *bottom_id = miniconsolefilterident();
            ident *colors_id = teamcolorchatident();
            ident *auto_ignore_id = autoignoreenabledident();
            ident *slot_count_id = autoignoreslotcountident();
            if(!top_id || !top_id->storage.i ||
               !bottom_id || !bottom_id->storage.i ||
               !colors_id || !colors_id->storage.i ||
               !auto_ignore_id || !auto_ignore_id->storage.i ||
               !slot_count_id || !slot_count_id->storage.i)
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
            result.auto_ignore_enabled = *auto_ignore_id->storage.i != 0;
            result.auto_ignore_slot_count = *slot_count_id->storage.i;
            loopi(Chat::AutoIgnoreSlotCount)
            {
                ident *name_id = autoignorenameident(i);
                if(!name_id || !name_id->storage.s || !*name_id->storage.s)
                    return false;
                copystring(result.auto_ignore_names[i], *name_id->storage.s,
                    Chat::AutoIgnoreNameSize);
            }
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

        bool writeautoignoreenabled(bool enabled, Chat &result)
        {
            ident *configured_id = autoignoreconfiguredident();
            if(!configured_id || !configured_id->storage.i ||
               !writeboolean(autoignoreenabledident(), enabled))
            {
                readchat(result);
                return false;
            }

            setvarchecked(configured_id, 1);
            writecfg();
            return readchat(result) &&
                result.auto_ignore_enabled == enabled;
        }

        bool writeautoignorename(int slot, const char *name, Chat &result)
        {
            ident *name_id = autoignorenameident(slot);
            ident *configured_id = autoignoreconfiguredident();
            if(!name || !name_id || !name_id->storage.s ||
               !configured_id || !configured_id->storage.i)
            {
                readchat(result);
                return false;
            }

            char filtered[Chat::AutoIgnoreNameSize] = "";
            filtertext(filtered, name, false, false, MAXNAMELEN);
            setsvarchecked(name_id, filtered);
            setvarchecked(configured_id, 1);
            writecfg();
            if(!readchat(result))
                return false;
            return !std::strcmp(result.auto_ignore_names[slot], filtered);
        }

        bool writeautoignoreslotcount(int count, Chat &result)
        {
            ident *count_id = autoignoreslotcountident();
            ident *configured_id = autoignoreconfiguredident();
            if(!count_id || !count_id->storage.i ||
               !configured_id || !configured_id->storage.i ||
               count < count_id->minval || count > count_id->maxval)
            {
                readchat(result);
                return false;
            }

            // A removed row should not remain active invisibly.
            if(count < *count_id->storage.i)
            {
                loopi(Chat::AutoIgnoreSlotCount)
                {
                    if(i < count)
                        continue;
                    ident *name_id = autoignorenameident(i);
                    if(name_id && name_id->storage.s)
                        setsvarchecked(name_id, "");
                }
            }
            setvarchecked(count_id, count);
            setvarchecked(configured_id, 1);
            writecfg();
            return readchat(result) &&
                result.auto_ignore_slot_count == count;
        }

        static bool validautoexecprofileid(const char *id)
        {
            if(!id || !id[0])
                return false;
            for(const char *c = id; *c; ++c)
            {
                const unsigned char value = static_cast<unsigned char>(*c);
                if(!std::isalnum(value) && *c != '_' && *c != '-')
                    return false;
            }
            return true;
        }

        static bool autoexecprofileexists(const char *id)
        {
            if(!validautoexecprofileid(id))
                return false;
            defformatstring(path, "autoexecs/%s.cfg", id);
            stream *file = openfile(path, "rb");
            if(!file)
                return false;
            delete file;
            return true;
        }

        bool visitautoexecprofiles(
            AutoexecChoiceVisitor visitor,
            void *data)
        {
            ident *selected_id = autoexecprofileident();
            if(!visitor || !selected_id || !selected_id->storage.s ||
               !*selected_id->storage.s)
            {
                return false;
            }
            const char *selected = *selected_id->storage.s;

            AutoexecChoice choice =
            {
                "original",
                "Original / autoexec.cfg",
                !std::strcmp(selected, "original")
            };
            visitor(choice, data);
            choice.id = "none";
            choice.display_name = "Disabled / load no autoexec";
            choice.selected = !std::strcmp(selected, "none");
            visitor(choice, data);

            vector<char *> files;
            listfiles("autoexecs", "cfg", files);
            files.sort();
            loopv(files)
            {
                const char *id = files[i];
                if(!validautoexecprofileid(id) ||
                   !std::strcmp(id, "original") ||
                   !std::strcmp(id, "none") ||
                   (i && !std::strcmp(id, files[i - 1])))
                {
                    continue;
                }
                choice.id = id;
                choice.display_name = id;
                choice.selected = !std::strcmp(selected, id);
                visitor(choice, data);
            }
            files.deletearrays();
            return true;
        }

        bool writeautoexecprofile(const char *id)
        {
            ident *profile_id = autoexecprofileident();
            if(!profile_id || !profile_id->storage.s || !id ||
               (std::strcmp(id, "original") &&
                std::strcmp(id, "none") &&
                !autoexecprofileexists(id)))
            {
                return false;
            }
            setsvarchecked(profile_id, id);
            writecfg();
            return *profile_id->storage.s &&
                !std::strcmp(*profile_id->storage.s, id);
        }

        bool runautoexecprofile(const char *id)
        {
            if(!id)
                return false;
            const bool disabled = !std::strcmp(id, "none");
            const bool original = !std::strcmp(id, "original");
            if(!disabled && !original && !autoexecprofileexists(id))
                return false;

            // Autoexec event hooks are persistent aliases. Loading another
            // profile on top of the current session must not inherit a hook
            // removed from the newly selected file.
            alias("mapstart", "");

            bool loaded = false;
            if(disabled)
                loaded = true;
            else if(original)
                loaded = execfile("autoexec.cfg", false);
            else
            {
                defformatstring(path, "autoexecs/%s.cfg", id);
                loaded = execfile(path, false);
            }

            if(loaded)
                writecfg();
            return loaded;
        }

        bool readtheme(Theme &result)
        {
            ident *id = uithemeident();
            if(!id || !id->storage.s || !*id->storage.s)
                return false;
            result = !std::strcmp(*id->storage.s, "modern-metallic")
                ? Theme::ModernMetallic
                : Theme::Default;
            return true;
        }

        bool writetheme(Theme requested, Theme &result)
        {
            ident *id = uithemeident();
            if(!id || !id->storage.s)
                return false;
            const char *name = requested == Theme::ModernMetallic
                ? "modern-metallic"
                : "default";
            setsvarchecked(id, name);
            if(!readtheme(result))
                return false;
            return result == requested;
        }

        bool readinterfaceappearance(InterfaceAppearance &result)
        {
            ident *ui_scale = interfaceappearanceident("uiscale");
            ident *text_scale = interfaceappearanceident("uitextscale");
            if(!ui_scale || !ui_scale->storage.i ||
               !text_scale || !text_scale->storage.i)
                return false;

            result.ui_scale = *ui_scale->storage.i;
            result.ui_scale_minimum = ui_scale->minval;
            result.ui_scale_maximum = ui_scale->maxval;
            result.text_scale = *text_scale->storage.i;
            result.text_scale_minimum = text_scale->minval;
            result.text_scale_maximum = text_scale->maxval;
            return true;
        }

        static bool writeinterfaceappearancescale(
            const char *name, int value, InterfaceAppearance &result)
        {
            ident *id = interfaceappearanceident(name);
            if(!id || !id->storage.i ||
               value < id->minval || value > id->maxval)
                return false;
            setvarchecked(id, value);
            return readinterfaceappearance(result) &&
                cowui::savepersistentstate();
        }

        bool writeuiscale(int value, InterfaceAppearance &result)
        {
            return writeinterfaceappearancescale("uiscale", value, result);
        }

        bool writeuitextscale(int value, InterfaceAppearance &result)
        {
            return writeinterfaceappearancescale(
                "uitextscale", value, result);
        }

        bool resetinterfaceappearance(InterfaceAppearance &result)
        {
            ident *ui_scale = interfaceappearanceident("uiscale");
            ident *text_scale = interfaceappearanceident("uitextscale");
            if(!ui_scale || !ui_scale->storage.i ||
               !text_scale || !text_scale->storage.i)
                return false;
            setvarchecked(ui_scale, 100);
            setvarchecked(text_scale, 100);
            return readinterfaceappearance(result) &&
                cowui::savepersistentstate();
        }

        // Native guibitfield "important information" uses 0x11.
        static const int console_important_mask = CON_INFO | CON_INIT;

        static void defaultconsole(Console &result)
        {
            result.important_information_in_top_feed = true;
            result.warnings_in_top_feed = true;
            result.errors_in_top_feed = true;
            result.script_messages_in_top_feed = true;
            result.game_events_in_top_feed = true;
            result.self_frags_in_top_feed = true;
            result.other_player_frags_in_top_feed = true;
            result.team_kills_in_top_feed = true;
            result.important_information_in_full_console = true;
            result.warnings_in_full_console = true;
            result.errors_in_full_console = true;
            result.script_messages_in_full_console = true;
            result.public_messages_in_full_console = true;
            result.team_messages_in_full_console = true;
            result.game_events_in_full_console = true;
            result.self_frags_in_full_console = true;
            result.other_player_frags_in_full_console = true;
            result.team_kills_in_full_console = true;
            result.size = 5;
            result.size_minimum = 0;
            result.size_maximum = 100;
        }

        static void fillconsolefilters(int top, int full, Console &result)
        {
            result.important_information_in_top_feed =
                (top & console_important_mask) != 0;
            result.warnings_in_top_feed = (top & CON_WARN) != 0;
            result.errors_in_top_feed = (top & CON_ERROR) != 0;
            result.script_messages_in_top_feed = (top & CON_ECHO) != 0;
            result.game_events_in_top_feed = (top & CON_GAMEINFO) != 0;
            result.self_frags_in_top_feed = (top & CON_FRAG_SELF) != 0;
            result.other_player_frags_in_top_feed =
                (top & CON_FRAG_OTHER) != 0;
            result.team_kills_in_top_feed = (top & CON_TEAMKILL) != 0;
            result.important_information_in_full_console =
                (full & console_important_mask) != 0;
            result.warnings_in_full_console = (full & CON_WARN) != 0;
            result.errors_in_full_console = (full & CON_ERROR) != 0;
            result.script_messages_in_full_console =
                (full & CON_ECHO) != 0;
            result.public_messages_in_full_console =
                (full & CON_CHAT) != 0;
            result.team_messages_in_full_console =
                (full & CON_TEAMCHAT) != 0;
            result.game_events_in_full_console =
                (full & CON_GAMEINFO) != 0;
            result.self_frags_in_full_console =
                (full & CON_FRAG_SELF) != 0;
            result.other_player_frags_in_full_console =
                (full & CON_FRAG_OTHER) != 0;
            result.team_kills_in_full_console =
                (full & CON_TEAMKILL) != 0;
        }

        bool readconsole(Console &result)
        {
            defaultconsole(result);

            ident *top_id = consolefilterident();
            ident *full_id = fullconsolefilterident();
            ident *size_id = consolesizeident();
            if(!top_id || !top_id->storage.i ||
               !full_id || !full_id->storage.i ||
               !size_id || !size_id->storage.i)
            {
                return false;
            }

            fillconsolefilters(*top_id->storage.i, *full_id->storage.i,
                result);
            result.size = *size_id->storage.i;
            result.size_minimum = size_id->minval;
            result.size_maximum = size_id->maxval;
            return true;
        }

        static bool writeconsolefilterbit(
            ident *id,
            int mask,
            bool enabled,
            Console &result)
        {
            const bool accepted = writefilterbit(id, mask, enabled);
            if(!readconsole(result))
                return false;
            return accepted;
        }

        bool writeimportantintopfeed(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                consolefilterident(),
                console_important_mask,
                enabled,
                result);
        }

        bool writewarningsintopfeed(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                consolefilterident(),
                CON_WARN,
                enabled,
                result);
        }

        bool writeerrorsintopfeed(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                consolefilterident(),
                CON_ERROR,
                enabled,
                result);
        }

        bool writescriptmessagesintopfeed(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                consolefilterident(),
                CON_ECHO,
                enabled,
                result);
        }

        bool writegameeventsintopfeed(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                consolefilterident(),
                CON_GAMEINFO,
                enabled,
                result);
        }

        bool writeselffragsintopfeed(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                consolefilterident(),
                CON_FRAG_SELF,
                enabled,
                result);
        }

        bool writeotherplayerfragsintopfeed(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                consolefilterident(),
                CON_FRAG_OTHER,
                enabled,
                result);
        }

        bool writeteamkillsintopfeed(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                consolefilterident(),
                CON_TEAMKILL,
                enabled,
                result);
        }

        bool writeimportantinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                console_important_mask,
                enabled,
                result);
        }

        bool writewarningsinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                CON_WARN,
                enabled,
                result);
        }

        bool writeerrorsinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                CON_ERROR,
                enabled,
                result);
        }

        bool writescriptmessagesinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                CON_ECHO,
                enabled,
                result);
        }

        bool writepublicchatinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                CON_CHAT,
                enabled,
                result);
        }

        bool writeteamchatinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                CON_TEAMCHAT,
                enabled,
                result);
        }

        bool writegameeventsinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                CON_GAMEINFO,
                enabled,
                result);
        }

        bool writeselffragsinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                CON_FRAG_SELF,
                enabled,
                result);
        }

        bool writeotherplayerfragsinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                CON_FRAG_OTHER,
                enabled,
                result);
        }

        bool writeteamkillsinfullconsole(bool enabled, Console &result)
        {
            return writeconsolefilterbit(
                fullconsolefilterident(),
                CON_TEAMKILL,
                enabled,
                result);
        }

        bool writeconsolesize(int value, Console &result)
        {
            if(!readconsole(result))
                return false;

            ident *id = consolesizeident();
            if(!id || !id->storage.i || id->flags&IDF_READONLY ||
               value < id->minval || value > id->maxval)
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            if(!readconsole(result))
                return false;
            return *id->storage.i == value;
        }
    }
}
