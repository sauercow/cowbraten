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
        bool demo_recording = false;
        bool demo_recording_bright = true;
        int mode_icon = 0; // 0 none, 1 blue flag, 2 red flag, 3 neutral flag, 4 token
        int mode_count = 0;
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

        bool operator==(const GameplayHudSnapshot &other) const
        {
            if(enabled != other.enabled || show != other.show ||
               alive != other.alive || quad != other.quad ||
               demo_recording != other.demo_recording ||
               (demo_recording &&
                demo_recording_bright != other.demo_recording_bright) ||
               mode_icon != other.mode_icon || mode_count != other.mode_count ||
               health != other.health || max_health != other.max_health ||
               armour != other.armour || armour_type != other.armour_type ||
               ammo != other.ammo || gun != other.gun ||
               health_tone != other.health_tone ||
               ammo_tone != other.ammo_tone ||
               show_secondary_ammo != other.show_secondary_ammo ||
               show_ammo_bar != other.show_ammo_bar ||
               ammo_bar_horizontal != other.ammo_bar_horizontal ||
               ammo_bar_flip != other.ammo_bar_flip ||
               ammo_bar_icon_size != other.ammo_bar_icon_size ||
               ammo_bar_label_gap != other.ammo_bar_label_gap ||
               ammo_bar_label_size != other.ammo_bar_label_size ||
               x != other.x || y != other.y)
                return false;
            for(int i = 0; i < SecondaryUpSlots; ++i)
                if(secondary_up[i] != other.secondary_up[i]) return false;
            for(int i = 0; i < SecondaryDownSlots; ++i)
                if(secondary_down[i] != other.secondary_down[i]) return false;
            for(int i = 0; i < SecondaryCycleSlots; ++i)
                if(secondary_cycle[i] != other.secondary_cycle[i]) return false;
            for(int i = 0; i < AmmoBarSlots; ++i)
                if(ammo_bar_visible[i] != other.ammo_bar_visible[i] ||
                   ammo_bar_count[i] != other.ammo_bar_count[i] ||
                   ammo_bar_tone[i] != other.ammo_bar_tone[i] ||
                   ammo_bar_x[i] != other.ammo_bar_x[i] ||
                   ammo_bar_y[i] != other.ammo_bar_y[i])
                    return false;
            return true;
        }
    };
}
