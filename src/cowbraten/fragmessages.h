#ifndef __FRAGMESSAGES_H__
#define __FRAGMESSAGES_H__

#include "game.h"

namespace game {
    // Visual roles are assigned when a frag is received. The original
    // coloured CubeScript names remain unchanged for the legacy renderer.
    enum FragMessageTone { FRAG_TONE_NEUTRAL, FRAG_TONE_SELF, FRAG_TONE_ENEMY, FRAG_TONE_ALLY };

    struct fragmessage {
        string attackername, victimname;
        int weapon;
        int fragtime;
        int attacker_tone, victim_tone, combo;

        fragmessage(const char *aname, const char *vname, int fragweapon,
                    int source_tone = FRAG_TONE_NEUTRAL,
                    int target_tone = FRAG_TONE_NEUTRAL, int streak_combo = 0)
        {
            copystring(attackername, aname ? aname : "");
            copystring(victimname, vname);
            weapon = fragweapon;
            fragtime = lastmillis;
            attacker_tone = source_tone;
            victim_tone = target_tone;
            combo = streak_combo;
        }
    };

    extern vector<fragmessage> fragmessages;
    extern int hudfragmessages;
    extern int hudfragmessageforcecolors;
    // Defined by MOD(VARP, ...) in fragmessages.cpp, used by fps.cpp.
    extern int hudfragmessagestackdir;
    extern void addfragmessage(int contype, const char *aname, const char *vname, int gun,
                               int attacker_tone = FRAG_TONE_NEUTRAL,
                               int victim_tone = FRAG_TONE_NEUTRAL, int combo = 0);
    extern void expirefragmessages();
    extern void clearfragmessages();
    extern void drawfragmessages(int w, int h);
}

#endif
