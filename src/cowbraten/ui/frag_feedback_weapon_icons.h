#pragma once

// The same gun IDs carried by fragmessage.weapon; the legacy renderer
// draws drawicon(HICON_FIST + weapon). Suicides use the HICON_TOKEN slot,
// numerically overlapping GUN_BARREL, so the empty attacker distinguishes
// the two cases (exactly as addfragmessage() does).
namespace cowui
{
    inline const char *fragfeedbackweaponicon(int weapon, bool suicide)
    {
        if(suicide) return "suicide.png";
        switch(weapon)
        {
            case 0: return "fist.png";            // GUN_FIST
            case 1: return "shotgun.png";         // GUN_SG
            case 2: return "chaingun.png";        // GUN_CG
            case 3: return "rocket_launcher.png"; // GUN_RL
            case 4: return "rifle.png";           // GUN_RIFLE
            case 5: return "grenade_launcher.png";// GUN_GL
            case 6: return "pistol.png";          // GUN_PISTOL
            case 7: return "fireball.png";        // GUN_FIREBALL
            case 8: return "iceball.png";         // GUN_ICEBALL
            case 9: return "slimeball.png";       // GUN_SLIMEBALL
            case 10: return "bite.png";           // GUN_BITE
            case 11: return "barrel.png";         // GUN_BARREL
            default: return "unknown.png";
        }
    }
}
