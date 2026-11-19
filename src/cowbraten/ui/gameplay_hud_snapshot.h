#pragma once

// Shared HUD state: deliberately no STL/RmlUi includes in fpsgame/fps.cpp.
// All values come from hudplayer(); no duplicated gameplay calculations.
namespace cowui
{
    struct GameplayHudSnapshot
    {
        bool enabled = false;
        bool show = false;
        bool alive = false;
        bool quad = false;
        int health = 0;
        int max_health = 100;
        int armour = 0;
        int armour_type = 0;
        int ammo = 0;
        int gun = 0;
        int health_tone = 0; // 0 neutral, 1 warning, 2 critical, 3 boosted
        int ammo_tone = 0;   // 0 neutral, 1 warning, 2 empty
        float x = 0.5f;     // HUD viewport normalized; matches legacy dock center
        float y = 0.923333f;// legacy STATUS_DOCK_Y / 1800
    };
}
