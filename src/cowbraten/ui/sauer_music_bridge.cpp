#include "engine.h"
#include "sauer_music_bridge.h"

#include <cstring>

extern int mainmenu;
extern int soundvol, musicvol;
extern bool nosound;
extern char *musicfile;
extern void stopmusic();

namespace cowui
{
namespace sauermusic
{
    static constexpr const char *track =
        "packages/cowbraten/music/menu.ogg";

    static bool owned = false;
    static bool attempted = false;

    void sync(bool menu_visible)
    {
        const bool should_play = menu_visible && mainmenu;

        if(!should_play)
        {
            // Stop only the music started by this bridge.
            if(owned && musicfile &&
               !std::strcmp(musicfile, track))
                stopmusic();

            owned = false;
            attempted = false;
            return;
        }

        // Handle audio resets or manual music replacement.
        if(owned && (!musicfile ||
           std::strcmp(musicfile, track)))
        {
            const bool replaced = musicfile != nullptr;
            owned = false;
            attempted = replaced;
        }

        // Do not override other tracks or repeatedly retry failures.
        if(attempted || nosound || !soundvol || !musicvol)
            return;

        attempted = true;
        execute("music cowbraten/music/menu.ogg");

        owned = musicfile &&
            !std::strcmp(musicfile, track);
    }
}
}
