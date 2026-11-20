#pragma once

// Shared HUD state: deliberately no STL/RmlUi includes in fpsgame/fps.cpp.
// All values come from hudplayer(); no duplicated gameplay calculations.
namespace cowui
{
    struct GameplayHudSnapshot
    {
        enum
        {
            SecondaryUpSlots = 3,
            SecondaryDownSlots = 3,
            SecondaryCycleSlots = 6,
            AmmoBarSlots = 6
        };

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
        bool show_secondary_ammo = false;
        int secondary_up[SecondaryUpSlots] = { -1, -1, -1 };
        int secondary_down[SecondaryDownSlots] = { -1, -1, -1 };
        int secondary_cycle[SecondaryCycleSlots] =
            { -1, -1, -1, -1, -1, -1 };
        bool show_ammo_bar = false;
        bool ammo_bar_horizontal = false;
        bool ammo_bar_flip = false;
        bool ammo_bar_visible[AmmoBarSlots] = {};
        int ammo_bar_count[AmmoBarSlots] = {};
        int ammo_bar_tone[AmmoBarSlots] = {};
        float ammo_bar_x[AmmoBarSlots] = {};
        float ammo_bar_y[AmmoBarSlots] = {};
        float ammo_bar_icon_size = 0.0f;
        float ammo_bar_label_gap = 0.0f;
        float ammo_bar_label_size = 0.0f;
        float x = 0.5f;     // HUD viewport normalized; matches legacy dock center
        float y = 0.923333f;// legacy STATUS_DOCK_Y / 1800
    };
}
