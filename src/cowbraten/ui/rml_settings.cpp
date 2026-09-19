#include "rml_settings.h"

#include "keybindings_api.h"
#include "rml_action.h"
#include "settings_api.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Input.h>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace cowui
{
    RmlSettings *RmlSettings::instance = nullptr;

    static void appendresolution(
        const settings::Resolution &resolution,
        void *data)
    {
        std::vector<settings::Resolution> &resolutions =
            *static_cast<std::vector<settings::Resolution> *>(data);
        resolutions.push_back(resolution);
    }

    static int fullbrightintensityselected(int value)
    {
        switch(value)
        {
            case 60:
            case 100:
            case 150:
            case 200:
                return value;
            default:
                return -1;
        }
    }

    static int packtrailcolor(int red, int green, int blue)
    {
        return (red << 16) | (green << 8) | blue;
    }

    static int unpacktrailred(int packed)
    {
        return (packed >> 16) & 0xFF;
    }

    static int unpacktrailgreen(int packed)
    {
        return (packed >> 8) & 0xFF;
    }

    static int unpacktrailblue(int packed)
    {
        return packed & 0xFF;
    }

    static bool resolutionorder(
        const settings::Resolution &left,
        const settings::Resolution &right)
    {
        if(left.width != right.width)
            return left.width > right.width;
        return left.height > right.height;
    }

    static bool sameresolution(
        const settings::Resolution &left,
        const settings::Resolution &right)
    {
        return left.width == right.width &&
            left.height == right.height;
    }

    static bool validdepthbufferchoice(int bits)
    {
        return bits == 0 || bits == 16 || bits == 24 || bits == 32;
    }

    static void formatdepthbufferlabel(
        int actual_bits,
        char *out,
        std::size_t out_size)
    {
        if(actual_bits > 0)
        {
            std::snprintf(out, out_size, "%d-bit", actual_bits);
            return;
        }

        std::snprintf(out, out_size, "Unavailable");
    }

    static void formatdepthbufferstatus(
        int requested_bits,
        int actual_bits,
        const char *actual_label,
        char *out,
        std::size_t out_size)
    {
        if(requested_bits == 0)
        {
            if(actual_bits == 24)
            {
                std::snprintf(
                    out,
                    out_size,
                    "Engine default 24-bit depth active"
                );
            }
            else if(actual_bits > 0)
            {
                std::snprintf(
                    out,
                    out_size,
                    "Engine default requested; runtime is %d-bit",
                    actual_bits
                );
            }
            else
            {
                std::snprintf(
                    out,
                    out_size,
                    "Engine default requested; runtime depth is unavailable"
                );
            }
            return;
        }

        if(requested_bits != actual_bits)
        {
            std::snprintf(
                out,
                out_size,
                "%d-bit requested; runtime is %s",
                requested_bits,
                actual_label
            );
            return;
        }

        std::snprintf(
            out,
            out_size,
            "%d-bit depth buffer active",
            actual_bits
        );
    }

    static void appendkeyname(const char *key_name, void *data)
    {
        if(!key_name || !data)
            return;

        std::vector<Rml::String> &keys =
            *static_cast<std::vector<Rml::String> *>(data);
        keys.push_back(key_name);
    }

    static Rml::String formatsensitivity(float value)
    {
        char text[32];
        std::snprintf(text, sizeof(text), "%.6f", double(value));

        char *decimal = std::strchr(text, '.');
        if(decimal)
        {
            char *end = text + std::strlen(text);
            while(end > decimal + 3 && end[-1] == '0')
                *--end = '\0';
        }
        return text;
    }

    static bool parsefloat(const Rml::String &text, float &value)
    {
        if(text.empty())
            return false;

        char *end = nullptr;
        errno = 0;
        const float parsed = std::strtof(text.c_str(), &end);
        if(end == text.c_str() || errno == ERANGE)
            return false;

        while(*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
            ++end;
        if(*end)
            return false;

        value = parsed;
        return true;
    }

    static bool parseinteger(const Rml::String &text, int &value)
    {
        if(text.empty())
            return false;

        char *end = nullptr;
        errno = 0;
        const long parsed = std::strtol(text.c_str(), &end, 10);
        if(end == text.c_str() || errno == ERANGE ||
           parsed < INT_MIN || parsed > INT_MAX)
        {
            return false;
        }

        while(*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
            ++end;
        if(*end)
            return false;

        value = int(parsed);
        return true;
    }

    static float stepsensitivity(float value, int direction)
    {
        const double stepped = std::round(
            (double(value) + double(direction) * 0.01) * 100.0
        ) / 100.0;
        return float(stepped);
    }

    static float stepstreakscale(float value, int direction)
    {
        const double stepped = std::round(
            (double(value) + double(direction) * 0.05) * 100.0
        ) / 100.0;
        return float(stepped);
    }

    static float stepfragmessagefloat(float value, int direction, double step)
    {
        const double stepped = std::round(
            (double(value) + double(direction) * step) / step
        ) * step;
        return float(stepped);
    }

    static float stepammobarfloat(float value, int direction)
    {
        const double stepped = std::round(
            (double(value) + double(direction) * 0.01) * 100.0
        ) / 100.0;
        return float(stepped);
    }

    static float stepgameclockfloat(float value, int direction)
    {
        const double stepped = std::round(
            (double(value) + double(direction) * 0.01) * 100.0
        ) / 100.0;
        return float(stepped);
    }

    static float stephudscore(float value, int direction)
    {
        const double stepped = std::round(
            (double(value) + double(direction) * 0.01) * 100.0
        ) / 100.0;
        return float(stepped);
    }

    static Rml::String rmlcrosshairresource(const char *path)
    {
        if(!path || !path[0])
            return Rml::String();
        return Rml::String("?cowbraten-crosshair:") + path;
    }

    void RmlSettings::appendcrosshairchoice(
        const settings::CrosshairChoice &choice,
        void *data)
    {
        std::vector<RmlSettings::CrosshairChoice> &choices =
            *static_cast<std::vector<RmlSettings::CrosshairChoice> *>(data);
        RmlSettings::CrosshairChoice row;
        row.id = choice.id;
        row.label = choice.label;
        row.resource = rmlcrosshairresource(choice.resource);
        choices.push_back(row);
    }

    void RmlSettings::appendsoundpackchoice(
        const settings::SoundpackChoice &choice,
        void *data)
    {
        std::vector<RmlSettings::SoundpackChoice> &choices =
            *static_cast<std::vector<RmlSettings::SoundpackChoice> *>(data);
        RmlSettings::SoundpackChoice row;
        row.id = choice.id;
        row.label = choice.display_name;
        choices.push_back(row);
    }

    bool RmlSettings::init(
        Rml::Context *context,
        RmlActionRouter &router)
    {
        if(!context || instance)
            return false;

        Rml::DataModelConstructor constructor =
            context->CreateDataModel("settings");

        if(!constructor)
            return false;

        bool valid = constructor.RegisterArray<std::vector<Rml::String>>();
        if(auto resolution = constructor.RegisterStruct<Resolution>())
        {
            valid = valid &&
                resolution.RegisterMember("label", &Resolution::label) &&
                resolution.RegisterMember("width", &Resolution::width) &&
                resolution.RegisterMember("height", &Resolution::height) &&
                resolution.RegisterMember(
                    "selected",
                    &Resolution::selected
                ) &&
                resolution.RegisterMember(
                    "supported",
                    &Resolution::supported
                );
        }
        else
        {
            valid = false;
        }

        if(auto choice = constructor.RegisterStruct<CrosshairChoice>())
        {
            valid = valid &&
                choice.RegisterMember(
                    "label",
                    &CrosshairChoice::label
                ) &&
                choice.RegisterMember(
                    "resource",
                    &CrosshairChoice::resource
                ) &&
                choice.RegisterMember("id", &CrosshairChoice::id) &&
                choice.RegisterMember(
                    "selected",
                    &CrosshairChoice::selected
                );
        }
        else valid = false;

        if(auto choice = constructor.RegisterStruct<SoundpackChoice>())
        {
            valid = valid &&
                choice.RegisterMember("id", &SoundpackChoice::id) &&
                choice.RegisterMember(
                    "label",
                    &SoundpackChoice::label
                ) &&
                choice.RegisterMember(
                    "selected",
                    &SoundpackChoice::selected
                );
        }
        else valid = false;

        if(auto row = constructor.RegisterStruct<KeyBindingRow>())
        {
            valid = valid &&
                row.RegisterMember("label", &KeyBindingRow::label) &&
                row.RegisterMember("action_id", &KeyBindingRow::action_id) &&
                row.RegisterMember("keys", &KeyBindingRow::keys) &&
                row.RegisterMember("bound", &KeyBindingRow::bound);
        }
        else valid = false;

        if(valid)
        {
            valid = settings::visitcrosshairs(
                appendcrosshairchoice,
                &crosshair_choices
            );
        }

        valid = valid &&
            constructor.RegisterArray<std::vector<Resolution>>() &&
            constructor.RegisterArray<std::vector<CrosshairChoice>>() &&
            constructor.RegisterArray<std::vector<SoundpackChoice>>() &&
            constructor.RegisterArray<std::vector<KeyBindingRow>>() &&
            constructor.Bind("resolutions", &resolutions) &&
            constructor.Bind("key_capture_id", &key_capture_id) &&
            constructor.Bind("key_capture_status", &key_capture_status) &&
            constructor.Bind("key_binding_status", &key_binding_status) &&
            constructor.Bind("key_candidate_pending", &key_candidate_pending) &&
            constructor.Bind("key_removal_active", &key_removal_active) &&
            constructor.BindFunc("key_removal_action_id", [this](Rml::Variant &value) {
                value = static_cast<int>(key_removal_action);
            }) &&
            constructor.Bind("key_removal_prompt", &key_removal_prompt) &&
            constructor.BindEventCallback("request_key_removal", &RmlSettings::requestkeyremoval, this) &&
            constructor.BindEventCallback("decide_key_removal", &RmlSettings::decidekeyremoval, this) &&
            constructor.BindEventCallback("decide_key", &RmlSettings::decidekeycandidate, this) &&
            constructor.Bind("key_conflict_status", &key_conflict_status) &&
            constructor.Bind("key_conflict_available", &key_conflict_available) &&
            constructor.Bind("key_conflict_same_action", &key_conflict_same_action) &&
            constructor.Bind("key_conflict_present", &key_conflict_present) &&
            constructor.BindEventCallback("capture_key", &RmlSettings::startkeycapture, this) &&
            constructor.Bind("crosshair_choices", &crosshair_choices) &&
            constructor.Bind("soundpack_choices", &soundpack_choices) &&
            constructor.Bind(
                "keybindings_movement",
                &keybindings_movement
            ) &&
            constructor.Bind(
                "keybindings_combat",
                &keybindings_combat
            ) &&
            constructor.Bind(
                "keybindings_interface",
                &keybindings_interface
            ) &&
            constructor.Bind("fov", &fov) &&
            constructor.Bind("fov_minimum", &fov_minimum) &&
            constructor.Bind("fov_maximum", &fov_maximum) &&
            constructor.Bind("game_feedback_available", &game_feedback_available) &&
            constructor.Bind("blood", &blood) &&
            constructor.Bind("hit_sound", &hit_sound) &&
            constructor.Bind("outline_capture_meters", &outline_capture_meters) &&
            constructor.Bind("show_scoreboard_at_death", &show_scoreboard_at_death) &&
            constructor.Bind("game_feedback_status", &game_feedback_status) &&
            constructor.BindEventCallback("select_blood", &RmlSettings::selectblood, this) &&
            constructor.BindEventCallback("select_hit_sound", &RmlSettings::selecthitsound, this) &&
            constructor.BindEventCallback("select_outline_capture_meters", &RmlSettings::selectoutlinecapturemeters, this) &&
            constructor.BindEventCallback("select_show_scoreboard_at_death", &RmlSettings::selectshowscoreboardatdeath, this) &&
            constructor.Bind("trail_colors_available", &trail_colors_available) &&
            constructor.Bind(
                "trail_color_channel_minimum",
                &trail_color_channel_minimum
            ) &&
            constructor.Bind(
                "trail_color_channel_maximum",
                &trail_color_channel_maximum
            ) &&
            constructor.Bind("trail_color_shotgun_red", &trail_color_shotgun_red) &&
            constructor.Bind("trail_color_shotgun_green", &trail_color_shotgun_green) &&
            constructor.Bind("trail_color_shotgun_blue", &trail_color_shotgun_blue) &&
            constructor.Bind("trail_color_chaingun_red", &trail_color_chaingun_red) &&
            constructor.Bind("trail_color_chaingun_green", &trail_color_chaingun_green) &&
            constructor.Bind("trail_color_chaingun_blue", &trail_color_chaingun_blue) &&
            constructor.Bind("trail_color_rocket_launcher_red", &trail_color_rocket_launcher_red) &&
            constructor.Bind("trail_color_rocket_launcher_green", &trail_color_rocket_launcher_green) &&
            constructor.Bind("trail_color_rocket_launcher_blue", &trail_color_rocket_launcher_blue) &&
            constructor.Bind("trail_color_rifle_red", &trail_color_rifle_red) &&
            constructor.Bind("trail_color_rifle_green", &trail_color_rifle_green) &&
            constructor.Bind("trail_color_rifle_blue", &trail_color_rifle_blue) &&
            constructor.Bind("trail_color_grenade_launcher_red", &trail_color_grenade_launcher_red) &&
            constructor.Bind("trail_color_grenade_launcher_green", &trail_color_grenade_launcher_green) &&
            constructor.Bind("trail_color_grenade_launcher_blue", &trail_color_grenade_launcher_blue) &&
            constructor.Bind("trail_color_pistol_red", &trail_color_pistol_red) &&
            constructor.Bind("trail_color_pistol_green", &trail_color_pistol_green) &&
            constructor.Bind("trail_color_pistol_blue", &trail_color_pistol_blue) &&
            constructor.Bind("trail_colors_status", &trail_colors_status) &&
            constructor.BindEventCallback(
                "select_trail_color_channel",
                &RmlSettings::selecttrailcolorchannel,
                this
            ) &&
            constructor.Bind("game_radar_available", &game_radar_available) &&
            constructor.Bind("numbered_capture_bases", &numbered_capture_bases) &&
            constructor.Bind("show_teammates_on_radar", &show_teammates_on_radar) &&
            constructor.Bind("game_radar_status", &game_radar_status) &&
            constructor.BindEventCallback("select_numbered_capture_bases", &RmlSettings::selectnumberedcapturebases, this) &&
            constructor.BindEventCallback("select_show_teammates_on_radar", &RmlSettings::selectshowteammatesonradar, this) &&
            constructor.Bind("game_models_available", &game_models_available) &&
            constructor.Bind("ragdoll_deaths", &ragdoll_deaths) &&
            constructor.Bind("keep_ragdoll_after_respawn", &keep_ragdoll_after_respawn) &&
            constructor.Bind("deadpush", &deadpush) &&
            constructor.Bind("deadpush_minimum", &deadpush_minimum) &&
            constructor.Bind("deadpush_maximum", &deadpush_maximum) &&
            constructor.Bind("hide_dead_players", &hide_dead_players) &&
            constructor.Bind("fullbright_player_models", &fullbright_player_models) &&
            constructor.Bind("fullbright_intensity_selected", &fullbright_intensity_selected) &&
            constructor.Bind("force_matching_player_models", &force_matching_player_models) &&
            constructor.Bind("always_use_team_skins", &always_use_team_skins) &&
            constructor.Bind("game_models_status", &game_models_status) &&
            constructor.BindEventCallback("select_ragdoll_deaths", &RmlSettings::selectragdolldeaths, this) &&
            constructor.BindEventCallback("select_keep_ragdoll_after_respawn", &RmlSettings::selectkeepragdollafterrespawn, this) &&
            constructor.BindEventCallback("select_hide_dead_players", &RmlSettings::selecthidedeadplayers, this) &&
            constructor.BindEventCallback("select_fullbright_player_models", &RmlSettings::selectfullbrightplayermodels, this) &&
            constructor.BindEventCallback("select_fullbright_intensity", &RmlSettings::selectfullbrightintensity, this) &&
            constructor.BindEventCallback("select_force_player_models", &RmlSettings::selectforceplayermodels, this) &&
            constructor.BindEventCallback("select_always_use_team_skins", &RmlSettings::selectalwaysuseteamskins, this) &&
            constructor.Bind("ammobar_available", &ammobar_available) &&
            constructor.Bind("ammobar_visible", &ammobar_visible) &&
            constructor.Bind("ammobar_horizontal", &ammobar_horizontal) &&
            constructor.Bind("ammobar_flip", &ammobar_flip) &&
            constructor.Bind("ammobar_scale", &ammobar_scale) &&
            constructor.Bind("ammobar_scale_minimum", &ammobar_scale_minimum) &&
            constructor.Bind("ammobar_scale_maximum", &ammobar_scale_maximum) &&
            constructor.Bind("ammobar_scale_input", &ammobar_scale_input) &&
            constructor.Bind("ammobar_scale_invalid", &ammobar_scale_invalid) &&
            constructor.Bind("ammobar_icon_spacing", &ammobar_icon_spacing) &&
            constructor.Bind(
                "ammobar_icon_spacing_minimum",
                &ammobar_icon_spacing_minimum
            ) &&
            constructor.Bind(
                "ammobar_icon_spacing_maximum",
                &ammobar_icon_spacing_maximum
            ) &&
            constructor.Bind("ammobar_label_size", &ammobar_label_size) &&
            constructor.Bind(
                "ammobar_label_size_minimum",
                &ammobar_label_size_minimum
            ) &&
            constructor.Bind(
                "ammobar_label_size_maximum",
                &ammobar_label_size_maximum
            ) &&
            constructor.Bind(
                "ammobar_label_size_input",
                &ammobar_label_size_input
            ) &&
            constructor.Bind(
                "ammobar_label_size_invalid",
                &ammobar_label_size_invalid
            ) &&
            constructor.Bind("ammobar_label_offset", &ammobar_label_offset) &&
            constructor.Bind(
                "ammobar_label_offset_minimum",
                &ammobar_label_offset_minimum
            ) &&
            constructor.Bind(
                "ammobar_label_offset_maximum",
                &ammobar_label_offset_maximum
            ) &&
            constructor.Bind("ammobar_align_first", &ammobar_align_first) &&
            constructor.Bind("ammobar_align_center", &ammobar_align_center) &&
            constructor.Bind("ammobar_align_last", &ammobar_align_last) &&
            constructor.Bind("ammobar_x", &ammobar_x) &&
            constructor.Bind("ammobar_x_minimum", &ammobar_x_minimum) &&
            constructor.Bind("ammobar_x_maximum", &ammobar_x_maximum) &&
            constructor.Bind("ammobar_x_input", &ammobar_x_input) &&
            constructor.Bind("ammobar_x_invalid", &ammobar_x_invalid) &&
            constructor.Bind("ammobar_y", &ammobar_y) &&
            constructor.Bind("ammobar_y_minimum", &ammobar_y_minimum) &&
            constructor.Bind("ammobar_y_maximum", &ammobar_y_maximum) &&
            constructor.Bind("ammobar_y_input", &ammobar_y_input) &&
            constructor.Bind("ammobar_y_invalid", &ammobar_y_invalid) &&
            constructor.Bind("ammobar_hide_empty", &ammobar_hide_empty) &&
            constructor.Bind("ammobar_status", &ammobar_status) &&
            constructor.BindEventCallback("select_ammobar", &RmlSettings::selectammobar, this) &&
            constructor.BindEventCallback("select_ammobar_horizontal", &RmlSettings::selectammobarhorizontal, this) &&
            constructor.BindEventCallback("select_ammobar_flip", &RmlSettings::selectammobarflip, this) &&
            constructor.BindEventCallback("select_ammobar_hide_empty", &RmlSettings::selectammobarhideempty, this) &&
            constructor.BindEventCallback("select_ammobar_align", &RmlSettings::selectammobaralign, this) &&
            constructor.Bind("scoreboard_available", &scoreboard_available) &&
            constructor.Bind("scoreboard_flags", &scoreboard_flags) &&
            constructor.Bind("scoreboard_frags", &scoreboard_frags) &&
            constructor.Bind("scoreboard_deaths", &scoreboard_deaths) &&
            constructor.Bind("scoreboard_suicides", &scoreboard_suicides) &&
            constructor.Bind("scoreboard_kpd", &scoreboard_kpd) &&
            constructor.Bind("scoreboard_accuracy", &scoreboard_accuracy) &&
            constructor.Bind("scoreboard_damage", &scoreboard_damage) &&
            constructor.Bind("scoreboard_damage_dealt", &scoreboard_damage_dealt) &&
            constructor.Bind("scoreboard_damage_net", &scoreboard_damage_net) &&
            constructor.Bind("scoreboard_damage_received", &scoreboard_damage_received) &&
            constructor.Bind("scoreboard_ping", &scoreboard_ping) &&
            constructor.Bind("scoreboard_pj", &scoreboard_pj) &&
            constructor.Bind("scoreboard_clientnum", &scoreboard_clientnum) &&
            constructor.Bind("scoreboard_status", &scoreboard_status) &&
            constructor.BindEventCallback("select_scoreboard_flags", &RmlSettings::selectscoreboardflags, this) &&
            constructor.BindEventCallback("select_scoreboard_frags", &RmlSettings::selectscoreboardfrags, this) &&
            constructor.BindEventCallback("select_scoreboard_deaths", &RmlSettings::selectscoreboarddeaths, this) &&
            constructor.BindEventCallback("select_scoreboard_suicides", &RmlSettings::selectscoreboardsuicides, this) &&
            constructor.BindEventCallback("select_scoreboard_kpd", &RmlSettings::selectscoreboardkpd, this) &&
            constructor.BindEventCallback("select_scoreboard_accuracy", &RmlSettings::selectscoreboardaccuracy, this) &&
            constructor.BindEventCallback("select_scoreboard_damage", &RmlSettings::selectscoreboarddamage, this) &&
            constructor.BindEventCallback("select_scoreboard_damage_mode", &RmlSettings::selectscoreboarddamagemode, this) &&
            constructor.BindEventCallback("select_scoreboard_damage_received", &RmlSettings::selectscoreboarddamagereceived, this) &&
            constructor.BindEventCallback("select_scoreboard_ping", &RmlSettings::selectscoreboardping, this) &&
            constructor.BindEventCallback("select_scoreboard_pj", &RmlSettings::selectscoreboardpj, this) &&
            constructor.BindEventCallback("select_scoreboard_clientnum", &RmlSettings::selectscoreboardclientnum, this) &&
            constructor.BindEventCallback("select_frag_message_stack_dir", &RmlSettings::selectfragmessagestackdir, this) &&
            constructor.Bind("resolution_label", &resolution_label) &&
            constructor.Bind(
                "render_resolution_label",
                &render_resolution_label
            ) &&
            constructor.Bind(
                "render_resolution_visible",
                &render_resolution_visible
            ) &&
            constructor.Bind("video_status", &video_status) &&
            constructor.Bind("presentation_label", &presentation_label) &&
            constructor.Bind(
                "presentation_windowed",
                &presentation_windowed
            ) &&
            constructor.Bind(
                "presentation_exclusive",
                &presentation_exclusive
            ) &&
            constructor.Bind(
                "presentation_desktop",
                &presentation_desktop
            ) &&
            constructor.Bind(
                "resolution_enabled",
                &resolution_enabled
            ) &&
            constructor.Bind("vsync", &vsync) &&
            constructor.Bind("vsync_adaptive", &vsync_adaptive) &&
            constructor.Bind("vsync_status", &vsync_status) &&
            constructor.Bind("gamma", &gamma) &&
            constructor.Bind("gamma_minimum", &gamma_minimum) &&
            constructor.Bind("gamma_maximum", &gamma_maximum) &&
            constructor.Bind("gamma_label", &gamma_label) &&
            constructor.Bind("gamma_status", &gamma_status) &&
            constructor.Bind("shadow_state_available", &shadow_state_available) &&
            constructor.Bind("shadow_maps", &shadow_maps) &&
            constructor.Bind("shadow_maps_available", &shadow_maps_available) &&
            constructor.Bind("blob_shadows", &blob_shadows) &&
            constructor.Bind("shadow_status", &shadow_status) &&
            constructor.BindEventCallback("select_shadow_maps", &RmlSettings::selectshadowmaps, this) &&
            constructor.BindEventCallback("select_blob_shadows", &RmlSettings::selectblobshadows, this) &&
            constructor.Bind("shadow_quality_available", &shadow_quality_available) &&
            constructor.Bind("shadow_quality_selected", &shadow_quality_selected) &&
            constructor.Bind("shadow_quality_label", &shadow_quality_label) &&
            constructor.Bind("shadow_quality_status", &shadow_quality_status) &&
            constructor.BindEventCallback("select_shadow_quality", &RmlSettings::selectshadowquality, this) &&
            constructor.Bind("water_surface_state_available", &water_surface_state_available) &&
            constructor.Bind("water_refraction", &water_refraction) &&
            constructor.Bind("water_refraction_available", &water_refraction_available) &&
            constructor.Bind("water_reflections", &water_reflections) &&
            constructor.Bind("water_reflections_available", &water_reflections_available) &&
            constructor.Bind("water_caustics", &water_caustics) &&
            constructor.Bind("liquid_surface_animation", &liquid_surface_animation) &&
            constructor.Bind("water_surface_status", &water_surface_status) &&
            constructor.BindEventCallback("select_water_refraction", &RmlSettings::selectwaterrefraction, this) &&
            constructor.BindEventCallback("select_water_reflections", &RmlSettings::selectwaterreflections, this) &&
            constructor.BindEventCallback("select_water_caustics", &RmlSettings::selectwatercaustics, this) &&
            constructor.BindEventCallback("select_liquid_animation", &RmlSettings::selectliquidanimation, this) &&
            constructor.Bind("water_environment_reflections", &water_environment_reflections) &&
            constructor.Bind("water_environment_reflections_available", &water_environment_reflections_available) &&
            constructor.Bind("water_environment_reflections_status", &water_environment_reflections_status) &&
            constructor.BindEventCallback("select_water_environment_reflections", &RmlSettings::selectwaterenvironmentreflections, this) &&
            constructor.Bind("water_render_resolution_available", &water_render_resolution_available) &&
            constructor.Bind("water_render_resolution_selected", &water_render_resolution_selected) &&
            constructor.Bind("water_render_resolution_label", &water_render_resolution_label) &&
            constructor.Bind("water_render_resolution_dimension", &water_render_resolution_dimension) &&
            constructor.Bind("water_render_resolution_status", &water_render_resolution_status) &&
            constructor.BindEventCallback("select_water_render_resolution", &RmlSettings::selectwaterrenderresolution, this) &&
            constructor.Bind("waterfall_state_available", &waterfall_state_available) &&
            constructor.Bind("waterfall_refraction", &waterfall_refraction) &&
            constructor.Bind("waterfall_refraction_available", &waterfall_refraction_available) &&
            constructor.Bind("waterfall_reflections", &waterfall_reflections) &&
            constructor.Bind("waterfall_status", &waterfall_status) &&
            constructor.BindEventCallback("select_waterfall_refraction", &RmlSettings::selectwaterfallrefraction, this) &&
            constructor.BindEventCallback("select_waterfall_reflections", &RmlSettings::selectwaterfallreflections, this) &&
            constructor.Bind("glass_reflections_available", &glass_reflections_available) &&
            constructor.Bind("glass_reflections", &glass_reflections) &&
            constructor.Bind("glass_reflections_status", &glass_reflections_status) &&
            constructor.BindEventCallback("select_glass_reflections", &RmlSettings::selectglassreflections, this) &&
            constructor.Bind("model_reflections_available", &model_reflections_available) &&
            constructor.Bind("model_reflections", &model_reflections) &&
            constructor.Bind("model_reflections_status", &model_reflections_status) &&
            constructor.BindEventCallback("select_model_reflections", &RmlSettings::selectmodelreflections, this) &&
            constructor.Bind("bump_models_available", &bump_models_available) &&
            constructor.Bind("bump_models", &bump_models) &&
            constructor.Bind("bump_models_status", &bump_models_status) &&
            constructor.BindEventCallback("select_bump_models", &RmlSettings::selectbumpmodels, this) &&
            constructor.Bind("environment_map_resolution_available", &environment_map_resolution_available) &&
            constructor.Bind("environment_map_resolution_selected", &environment_map_resolution_selected) &&
            constructor.Bind("environment_map_resolution_label", &environment_map_resolution_label) &&
            constructor.Bind("environment_map_resolution_dimension", &environment_map_resolution_dimension) &&
            constructor.Bind("environment_map_resolution_status", &environment_map_resolution_status) &&
            constructor.BindEventCallback("select_environment_map_resolution", &RmlSettings::selectenvironmentmapresolution, this) &&
            constructor.Bind("dynamic_lights_available", &dynamic_lights_available) &&
            constructor.Bind("dynamic_lights_selected", &dynamic_lights_selected) &&
            constructor.Bind("dynamic_lights_label", &dynamic_lights_label) &&
            constructor.Bind("dynamic_lights_status", &dynamic_lights_status) &&
            constructor.BindEventCallback("select_dynamic_lights", &RmlSettings::selectdynamiclights, this) &&
            constructor.Bind("soft_particles_available", &soft_particles_available) &&
            constructor.Bind("soft_particles", &soft_particles) &&
            constructor.Bind("soft_particles_status", &soft_particles_status) &&
            constructor.BindEventCallback("select_soft_particles", &RmlSettings::selectsoftparticles, this) &&
            constructor.Bind("soft_particle_quality_available", &soft_particle_quality_available) &&
            constructor.Bind("soft_particle_quality_selected", &soft_particle_quality_selected) &&
            constructor.Bind("soft_particle_quality_label", &soft_particle_quality_label) &&
            constructor.Bind("soft_particle_quality_status", &soft_particle_quality_status) &&
            constructor.BindEventCallback("select_soft_particle_quality", &RmlSettings::selectsoftparticlequality, this) &&
            constructor.Bind("decals_available", &decals_available) &&
            constructor.Bind("decals", &decals) &&
            constructor.Bind("decals_status", &decals_status) &&
            constructor.BindEventCallback("select_decals", &RmlSettings::selectdecals, this) &&
            constructor.Bind("decal_persistence_available", &decal_persistence_available) &&
            constructor.Bind("decal_persistence_selected", &decal_persistence_selected) &&
            constructor.Bind("decal_persistence_label", &decal_persistence_label) &&
            constructor.Bind("decal_persistence_status", &decal_persistence_status) &&
            constructor.BindEventCallback("select_decal_persistence", &RmlSettings::selectdecalpersistence, this) &&
            constructor.Bind("fix_t_joints_available", &fix_t_joints_available) &&
            constructor.Bind("fix_t_joints", &fix_t_joints) &&
            constructor.Bind("fix_t_joints_status", &fix_t_joints_status) &&
            constructor.BindEventCallback("select_fix_t_joints", &RmlSettings::selectfixtjoints, this) &&
            constructor.Bind("grass", &grass) &&
            constructor.Bind("grass_distance_available", &grass_distance_available) &&
            constructor.Bind("grass_distance_selected", &grass_distance_selected) &&
            constructor.Bind("grass_distance_label", &grass_distance_label) &&
            constructor.Bind("grass_distance_status", &grass_distance_status) &&
            constructor.BindEventCallback("select_grass_distance", &RmlSettings::selectgrassdistance, this) &&
            constructor.Bind("motion_blur", &motion_blur) &&
            constructor.Bind("motion_blur_available", &motion_blur_available) &&
            constructor.Bind("motion_blur_intensity_available", &motion_blur_intensity_available) &&
            constructor.Bind("motion_blur_intensity_selected", &motion_blur_intensity_selected) &&
            constructor.Bind("motion_blur_intensity_label", &motion_blur_intensity_label) &&
            constructor.Bind("motion_blur_intensity_status", &motion_blur_intensity_status) &&
            constructor.BindEventCallback("select_motion_blur_intensity", &RmlSettings::selectmotionblurintensity, this) &&
            constructor.Bind("glare", &glare) &&
            constructor.BindEventCallback("select_glare", &RmlSettings::selectglare, this) &&
            constructor.Bind("glare_preset_available", &glare_preset_available) &&
            constructor.Bind("glare_preset_selected", &glare_preset_selected) &&
            constructor.Bind("glare_preset_label", &glare_preset_label) &&
            constructor.Bind("glare_preset_status", &glare_preset_status) &&
            constructor.BindEventCallback("select_glare_preset", &RmlSettings::selectglarepreset, this) &&
            constructor.Bind("glare_intensity_available", &glare_intensity_available) &&
            constructor.Bind("glare_intensity_slider", &glare_intensity_slider) &&
            constructor.Bind("glare_intensity_label", &glare_intensity_label) &&
            constructor.Bind("glare_intensity_status", &glare_intensity_status) &&
            constructor.Bind("glare_resolution_available", &glare_resolution_available) &&
            constructor.Bind("glare_resolution_selected", &glare_resolution_selected) &&
            constructor.Bind("glare_resolution_label", &glare_resolution_label) &&
            constructor.Bind("glare_resolution_requested_width", &glare_resolution_requested_width) &&
            constructor.Bind("glare_resolution_status", &glare_resolution_status) &&
            constructor.BindEventCallback("select_glare_resolution", &RmlSettings::selectglareresolution, this) &&
            constructor.Bind("texture_quality_available", &texture_quality_available) &&
            constructor.Bind("texture_quality_selected", &texture_quality_selected) &&
            constructor.Bind("texture_quality_label", &texture_quality_label) &&
            constructor.Bind("texture_quality_status", &texture_quality_status) &&
            constructor.BindEventCallback("select_texture_quality", &RmlSettings::selecttexturequality, this) &&
            constructor.Bind("texture_reduction", &texture_reduction) &&
            constructor.Bind("texture_reduction_minimum", &texture_reduction_minimum) &&
            constructor.Bind("texture_reduction_maximum", &texture_reduction_maximum) &&
            constructor.Bind("texture_reduction_available", &texture_reduction_available) &&
            constructor.Bind("texture_reduction_label", &texture_reduction_label) &&
            constructor.Bind("texture_reduction_status", &texture_reduction_status) &&
            constructor.Bind("texture_compression_selected", &texture_compression_selected) &&
            constructor.Bind("texture_compression_available", &texture_compression_available) &&
            constructor.Bind("texture_compression_supported", &texture_compression_supported) &&
            constructor.Bind("texture_compression_label", &texture_compression_label) &&
            constructor.Bind("texture_compression_status", &texture_compression_status) &&
            constructor.BindEventCallback("select_texture_compression", &RmlSettings::selecttexturecompression, this) &&
            constructor.Bind("graphics_quality_status", &graphics_quality_status) &&
            constructor.Bind("anisotropy_enabled", &anisotropy_enabled) &&
            constructor.Bind("shader_detail_available", &shader_detail_available) &&
            constructor.Bind("shader_detail_selected", &shader_detail_selected) &&
            constructor.Bind("shader_detail_label", &shader_detail_label) &&
            constructor.Bind("shader_detail_status", &shader_detail_status) &&
            constructor.BindEventCallback("select_shader_detail", &RmlSettings::selectshaderdetail, this) &&
            constructor.Bind("bilinear_filtering", &bilinear_filtering) &&
            constructor.Bind("bilinear_filtering_available", &bilinear_filtering_available) &&
            constructor.Bind("bilinear_filtering_status", &bilinear_filtering_status) &&
            constructor.BindEventCallback("select_bilinear_filtering", &RmlSettings::selectbilinearfiltering, this) &&
            constructor.Bind("mipmap_blending", &mipmap_blending) &&
            constructor.Bind("mipmap_blending_available", &mipmap_blending_available) &&
            constructor.Bind("mipmap_blending_status", &mipmap_blending_status) &&
            constructor.BindEventCallback("select_mipmap_blending", &RmlSettings::selectmipmapblending, this) &&
            constructor.Bind("anisotropy_maximum", &anisotropy_maximum) &&
            constructor.Bind("anisotropy_selected", &anisotropy_selected) &&
            constructor.Bind("anisotropy_label", &anisotropy_label) &&
            constructor.Bind("anisotropy_status", &anisotropy_status) &&
            constructor.BindEventCallback("select_anisotropy", &RmlSettings::selectanisotropy, this) &&
            constructor.Bind("fps_limit", &fps_limit) &&
            constructor.Bind(
                "fps_limit_minimum",
                &fps_limit_minimum
            ) &&
            constructor.Bind(
                "fps_limit_maximum",
                &fps_limit_maximum
            ) &&
            constructor.Bind("fps_limit_label", &fps_limit_label) &&
            constructor.Bind("antialiasing", &antialiasing) &&
            constructor.Bind(
                "antialiasing_label",
                &antialiasing_label
            ) &&
            constructor.Bind(
                "antialiasing_status",
                &antialiasing_status
            ) &&
            constructor.Bind("antialiasing_off", &antialiasing_off) &&
            constructor.Bind("antialiasing_2x", &antialiasing_2x) &&
            constructor.Bind("antialiasing_4x", &antialiasing_4x) &&
            constructor.Bind("antialiasing_8x", &antialiasing_8x) &&
            constructor.Bind("antialiasing_16x", &antialiasing_16x) &&
            constructor.Bind("depth_buffer", &depth_buffer) &&
            constructor.Bind(
                "depth_buffer_label",
                &depth_buffer_label
            ) &&
            constructor.Bind(
                "depth_buffer_status",
                &depth_buffer_status
            ) &&
            constructor.Bind("depth_buffer_default", &depth_buffer_default) &&
            constructor.Bind("depth_buffer_16", &depth_buffer_16) &&
            constructor.Bind("depth_buffer_24", &depth_buffer_24) &&
            constructor.Bind("depth_buffer_32", &depth_buffer_32) &&
            constructor.Bind("sound_volume", &sound_volume) &&
            constructor.Bind(
                "sound_volume_minimum",
                &sound_volume_minimum
            ) &&
            constructor.Bind(
                "sound_volume_maximum",
                &sound_volume_maximum
            ) &&
            constructor.Bind(
                "sound_volume_label",
                &sound_volume_label
            ) &&
            constructor.Bind("music_volume", &music_volume) &&
            constructor.Bind(
                "music_volume_minimum",
                &music_volume_minimum
            ) &&
            constructor.Bind(
                "music_volume_maximum",
                &music_volume_maximum
            ) &&
            constructor.Bind(
                "music_volume_label",
                &music_volume_label
            ) &&
            constructor.Bind("audio_status", &audio_status) &&
            constructor.Bind("active_soundpack", &active_soundpack) &&
            constructor.Bind(
                "active_soundpack_label",
                &active_soundpack_label
            ) &&
            constructor.Bind("soundpack_status", &soundpack_status) &&
            constructor.Bind(
                "mouse_sensitivity",
                &mouse_sensitivity
            ) &&
            constructor.Bind(
                "mouse_sensitivity_minimum",
                &mouse_sensitivity_minimum
            ) &&
            constructor.Bind(
                "mouse_sensitivity_maximum",
                &mouse_sensitivity_maximum
            ) &&
            constructor.Bind(
                "mouse_sensitivity_label",
                &mouse_sensitivity_label
            ) &&
            constructor.Bind(
                "mouse_sensitivity_input",
                &mouse_sensitivity_input
            ) &&
            constructor.Bind(
                "mouse_sensitivity_status",
                &mouse_sensitivity_status
            ) &&
            constructor.Bind(
                "mouse_sensitivity_invalid",
                &mouse_sensitivity_invalid
            ) &&
            constructor.Bind("inverted_mouse", &inverted_mouse) &&
            constructor.Bind(
                "mouse_input_status",
                &mouse_input_status
            ) &&
            constructor.Bind(
                "crosshair_selection_label",
                &crosshair_selection_label
            ) &&
            constructor.Bind(
                "crosshair_selection_resource",
                &crosshair_selection_resource
            ) &&
            constructor.Bind("crosshair_size", &crosshair_size) &&
            constructor.Bind(
                "crosshair_size_minimum",
                &crosshair_size_minimum
            ) &&
            constructor.Bind(
                "crosshair_size_maximum",
                &crosshair_size_maximum
            ) &&
            constructor.Bind(
                "crosshair_size_input",
                &crosshair_size_input
            ) &&
            constructor.Bind(
                "crosshair_size_invalid",
                &crosshair_size_invalid
            ) &&
            constructor.Bind("crosshair_status", &crosshair_status) &&
            constructor.Bind("crosshair_effects", &crosshair_effects) &&
            constructor.Bind(
                "crosshair_health_colors",
                &crosshair_health_colors
            ) &&
            constructor.Bind("team_crosshair", &team_crosshair) &&
            constructor.Bind("hit_crosshair", &hit_crosshair) &&
            constructor.Bind(
                "crosshair_reload_fade",
                &crosshair_reload_fade
            ) &&
            constructor.Bind(
                "crosshair_reload_rotation",
                &crosshair_reload_rotation
            ) &&
            constructor.Bind("show_fps", &show_fps) &&
            constructor.Bind("wall_clock", &wall_clock) &&
            constructor.Bind("wall_clock_24_hour", &wall_clock_24_hour) &&
            constructor.Bind(
                "wall_clock_seconds",
                &wall_clock_seconds
            ) &&
            constructor.Bind("damage_screen", &damage_screen) &&
            constructor.Bind("damage_compass", &damage_compass) &&
            constructor.Bind("hud_guns", &hud_guns) &&
            constructor.Bind("hud_gun_sway", &hud_gun_sway) &&
            constructor.Bind("muzzle_flash", &muzzle_flash) &&
            constructor.Bind("muzzle_light", &muzzle_light) &&
            constructor.Bind(
                "frag_messages_enabled",
                &frag_messages_enabled
            ) &&
            constructor.Bind(
                "frag_messages_in_insta",
                &frag_messages_in_insta
            ) &&
            constructor.Bind(
                "max_frag_messages",
                &max_frag_messages
            ) &&
            constructor.Bind(
                "max_frag_messages_minimum",
                &max_frag_messages_minimum
            ) &&
            constructor.Bind(
                "max_frag_messages_maximum",
                &max_frag_messages_maximum
            ) &&
            constructor.Bind(
                "frag_message_duration",
                &frag_message_duration
            ) &&
            constructor.Bind(
                "frag_message_duration_minimum",
                &frag_message_duration_minimum
            ) &&
            constructor.Bind(
                "frag_message_duration_maximum",
                &frag_message_duration_maximum
            ) &&
            constructor.Bind(
                "frag_message_duration_input",
                &frag_message_duration_input
            ) &&
            constructor.Bind(
                "frag_message_duration_invalid",
                &frag_message_duration_invalid
            ) &&
            constructor.Bind(
                "frag_message_self_frags",
                &frag_message_self_frags
            ) &&
            constructor.Bind(
                "frag_message_other_frags",
                &frag_message_other_frags
            ) &&
            constructor.Bind(
                "frag_message_team_kills",
                &frag_message_team_kills
            ) &&
            constructor.Bind(
                "frag_message_scale",
                &frag_message_scale
            ) &&
            constructor.Bind(
                "frag_message_scale_minimum",
                &frag_message_scale_minimum
            ) &&
            constructor.Bind(
                "frag_message_scale_maximum",
                &frag_message_scale_maximum
            ) &&
            constructor.Bind(
                "frag_message_scale_input",
                &frag_message_scale_input
            ) &&
            constructor.Bind(
                "frag_message_scale_invalid",
                &frag_message_scale_invalid
            ) &&
            constructor.Bind("frag_message_fade", &frag_message_fade) &&
            constructor.Bind("frag_message_x", &frag_message_x) &&
            constructor.Bind(
                "frag_message_x_minimum",
                &frag_message_x_minimum
            ) &&
            constructor.Bind(
                "frag_message_x_maximum",
                &frag_message_x_maximum
            ) &&
            constructor.Bind(
                "frag_message_x_input",
                &frag_message_x_input
            ) &&
            constructor.Bind(
                "frag_message_x_invalid",
                &frag_message_x_invalid
            ) &&
            constructor.Bind("frag_message_y", &frag_message_y) &&
            constructor.Bind(
                "frag_message_y_minimum",
                &frag_message_y_minimum
            ) &&
            constructor.Bind(
                "frag_message_y_maximum",
                &frag_message_y_maximum
            ) &&
            constructor.Bind(
                "frag_message_y_input",
                &frag_message_y_input
            ) &&
            constructor.Bind(
                "frag_message_y_invalid",
                &frag_message_y_invalid
            ) &&
            constructor.Bind(
                "frag_message_stack_up",
                &frag_message_stack_up
            ) &&
            constructor.Bind(
                "frag_message_stack_auto",
                &frag_message_stack_auto
            ) &&
            constructor.Bind(
                "frag_message_stack_down",
                &frag_message_stack_down
            ) &&
            constructor.Bind(
                "streak_milestone_scale",
                &streak_milestone_scale
            ) &&
            constructor.Bind(
                "streak_milestone_scale_minimum",
                &streak_milestone_scale_minimum
            ) &&
            constructor.Bind(
                "streak_milestone_scale_maximum",
                &streak_milestone_scale_maximum
            ) &&
            constructor.Bind(
                "streak_milestone_scale_input",
                &streak_milestone_scale_input
            ) &&
            constructor.Bind(
                "streak_milestone_scale_invalid",
                &streak_milestone_scale_invalid
            ) &&
            constructor.Bind("hud_status", &hud_status) &&
            constructor.Bind(
                "game_clock_available",
                &game_clock_available
            ) &&
            constructor.Bind("game_clock", &game_clock) &&
            constructor.Bind(
                "game_clock_scale",
                &game_clock_scale
            ) &&
            constructor.Bind(
                "game_clock_scale_minimum",
                &game_clock_scale_minimum
            ) &&
            constructor.Bind(
                "game_clock_scale_maximum",
                &game_clock_scale_maximum
            ) &&
            constructor.Bind(
                "game_clock_scale_input",
                &game_clock_scale_input
            ) &&
            constructor.Bind(
                "game_clock_scale_invalid",
                &game_clock_scale_invalid
            ) &&
            constructor.Bind(
                "game_clock_alpha",
                &game_clock_alpha
            ) &&
            constructor.Bind(
                "game_clock_alpha_minimum",
                &game_clock_alpha_minimum
            ) &&
            constructor.Bind(
                "game_clock_alpha_maximum",
                &game_clock_alpha_maximum
            ) &&
            constructor.Bind(
                "game_clock_align",
                &game_clock_align
            ) &&
            constructor.Bind("game_clock_x", &game_clock_x) &&
            constructor.Bind(
                "game_clock_x_minimum",
                &game_clock_x_minimum
            ) &&
            constructor.Bind(
                "game_clock_x_maximum",
                &game_clock_x_maximum
            ) &&
            constructor.Bind(
                "game_clock_x_input",
                &game_clock_x_input
            ) &&
            constructor.Bind(
                "game_clock_x_invalid",
                &game_clock_x_invalid
            ) &&
            constructor.Bind("game_clock_y", &game_clock_y) &&
            constructor.Bind(
                "game_clock_y_minimum",
                &game_clock_y_minimum
            ) &&
            constructor.Bind(
                "game_clock_y_maximum",
                &game_clock_y_maximum
            ) &&
            constructor.Bind(
                "game_clock_y_input",
                &game_clock_y_input
            ) &&
            constructor.Bind(
                "game_clock_y_invalid",
                &game_clock_y_invalid
            ) &&
            constructor.Bind(
                "game_clock_status",
                &game_clock_status
            ) &&
            constructor.BindEventCallback(
                "select_game_clock",
                &RmlSettings::selectgameclock,
                this
            ) &&
            constructor.BindEventCallback(
                "select_game_clock_align",
                &RmlSettings::selectgameclockalign,
                this
            ) &&
            constructor.Bind("hudscore_available", &hudscore_available) &&
            constructor.Bind("hudscore", &hudscore) &&
            constructor.Bind("hudscore_scale", &hudscore_scale) &&
            constructor.Bind(
                "hudscore_scale_minimum",
                &hudscore_scale_minimum
            ) &&
            constructor.Bind(
                "hudscore_scale_maximum",
                &hudscore_scale_maximum
            ) &&
            constructor.Bind(
                "hudscore_scale_input",
                &hudscore_scale_input
            ) &&
            constructor.Bind(
                "hudscore_scale_invalid",
                &hudscore_scale_invalid
            ) &&
            constructor.Bind("hudscore_alpha", &hudscore_alpha) &&
            constructor.Bind(
                "hudscore_alpha_minimum",
                &hudscore_alpha_minimum
            ) &&
            constructor.Bind(
                "hudscore_alpha_maximum",
                &hudscore_alpha_maximum
            ) &&
            constructor.Bind("hudscore_sep", &hudscore_sep) &&
            constructor.Bind(
                "hudscore_sep_minimum",
                &hudscore_sep_minimum
            ) &&
            constructor.Bind(
                "hudscore_sep_maximum",
                &hudscore_sep_maximum
            ) &&
            constructor.Bind("hudscore_align", &hudscore_align) &&
            constructor.Bind("hudscore_x", &hudscore_x) &&
            constructor.Bind("hudscore_x_minimum", &hudscore_x_minimum) &&
            constructor.Bind("hudscore_x_maximum", &hudscore_x_maximum) &&
            constructor.Bind("hudscore_x_input", &hudscore_x_input) &&
            constructor.Bind("hudscore_x_invalid", &hudscore_x_invalid) &&
            constructor.Bind("hudscore_y", &hudscore_y) &&
            constructor.Bind("hudscore_y_minimum", &hudscore_y_minimum) &&
            constructor.Bind("hudscore_y_maximum", &hudscore_y_maximum) &&
            constructor.Bind("hudscore_y_input", &hudscore_y_input) &&
            constructor.Bind("hudscore_y_invalid", &hudscore_y_invalid) &&
            constructor.Bind("hudscore_status", &hudscore_status) &&
            constructor.BindEventCallback(
                "select_hudscore",
                &RmlSettings::selecthudscore,
                this
            ) &&
            constructor.BindEventCallback(
                "select_hudscore_align",
                &RmlSettings::selecthudscorealign,
                this
            ) &&
            constructor.Bind(
                "public_chat_in_top_feed",
                &public_chat_in_top_feed
            ) &&
            constructor.Bind(
                "team_chat_in_top_feed",
                &team_chat_in_top_feed
            ) &&
            constructor.Bind(
                "public_chat_in_bottom_box",
                &public_chat_in_bottom_box
            ) &&
            constructor.Bind(
                "team_chat_in_bottom_box",
                &team_chat_in_bottom_box
            ) &&
            constructor.Bind(
                "team_colored_chat_names",
                &team_colored_chat_names
            ) &&
            constructor.Bind("console_available", &console_available) &&
            constructor.Bind(
                "console_top_important",
                &console_top_important
            ) &&
            constructor.Bind(
                "console_top_warnings",
                &console_top_warnings
            ) &&
            constructor.Bind(
                "console_top_errors",
                &console_top_errors
            ) &&
            constructor.Bind(
                "console_top_script",
                &console_top_script
            ) &&
            constructor.Bind(
                "console_top_game_events",
                &console_top_game_events
            ) &&
            constructor.Bind(
                "console_top_self_frags",
                &console_top_self_frags
            ) &&
            constructor.Bind(
                "console_top_other_frags",
                &console_top_other_frags
            ) &&
            constructor.Bind(
                "console_top_team_kills",
                &console_top_team_kills
            ) &&
            constructor.Bind(
                "console_full_important",
                &console_full_important
            ) &&
            constructor.Bind(
                "console_full_warnings",
                &console_full_warnings
            ) &&
            constructor.Bind(
                "console_full_errors",
                &console_full_errors
            ) &&
            constructor.Bind(
                "console_full_script",
                &console_full_script
            ) &&
            constructor.Bind("console_full_chat", &console_full_chat) &&
            constructor.Bind(
                "console_full_team_chat",
                &console_full_team_chat
            ) &&
            constructor.Bind(
                "console_full_game_events",
                &console_full_game_events
            ) &&
            constructor.Bind(
                "console_full_self_frags",
                &console_full_self_frags
            ) &&
            constructor.Bind(
                "console_full_other_frags",
                &console_full_other_frags
            ) &&
            constructor.Bind(
                "console_full_team_kills",
                &console_full_team_kills
            ) &&
            constructor.Bind("console_size", &console_size) &&
            constructor.Bind(
                "console_size_minimum",
                &console_size_minimum
            ) &&
            constructor.Bind(
                "console_size_maximum",
                &console_size_maximum
            ) &&
            constructor.Bind("console_size_input", &console_size_input) &&
            constructor.Bind(
                "console_size_invalid",
                &console_size_invalid
            ) &&
            constructor.Bind("console_status", &console_status) &&
            constructor.Bind("category_game", &category_game) &&
            constructor.Bind("category_hud", &category_hud) &&
            constructor.Bind(
                "category_graphics",
                &category_graphics
            ) &&
            constructor.Bind(
                "category_audio",
                &category_audio
            ) &&
            constructor.Bind(
                "category_mouse_keyboard",
                &category_mouse_keyboard
            ) &&
            constructor.Bind(
                "category_crosshair",
                &category_crosshair
            ) &&
            constructor.Bind(
                "category_chat",
                &category_chat
            ) &&
            constructor.Bind(
                "category_console",
                &category_console
            ) &&
            constructor.BindEventCallback(
                "select_resolution",
                &RmlSettings::selectresolution,
                this
            ) &&
            constructor.BindEventCallback(
                "select_antialiasing",
                &RmlSettings::selectantialiasing,
                this
            ) &&
            constructor.BindEventCallback(
                "select_depth_buffer",
                &RmlSettings::selectdepthbuffer,
                this
            ) &&
            constructor.BindEventCallback(
                "select_crosshair",
                &RmlSettings::selectcrosshair,
                this
            ) &&
            constructor.BindEventCallback(
                "select_soundpack",
                &RmlSettings::selectsoundpack,
                this
            );

        valid = valid &&
            router.registeraction(
                "settings.fov",
                changefovaction
            ) &&
            router.registeraction(
                "settings.deadpush",
                changedeadpushaction
            ) &&
            router.registeraction(
                "settings.category.game",
                gamecategoryaction
            ) &&
            router.registeraction(
                "settings.category.hud",
                hudcategoryaction
            ) &&
            router.registeraction(
                "settings.category.graphics",
                graphicscategoryaction
            ) &&
            router.registeraction(
                "settings.category.audio",
                audiocategoryaction
            ) &&
            router.registeraction(
                "settings.category.mouse-keyboard",
                mousekeyboardcategoryaction
            ) &&
            router.registeraction(
                "settings.category.crosshair",
                crosshaircategoryaction
            ) &&
            router.registeraction(
                "settings.category.chat",
                chatcategoryaction
            ) &&
            router.registeraction(
                "settings.category.console",
                consolecategoryaction
            ) &&
            router.registeraction(
                "settings.presentation.windowed",
                windowedaction
            ) &&
            router.registeraction(
                "settings.presentation.exclusive",
                exclusivefullscreenaction
            ) &&
            router.registeraction(
                "settings.presentation.desktop",
                desktopfullscreenaction
            ) &&
            router.registeraction(
                "settings.vsync",
                vsyncaction
            ) &&
            router.registeraction(
                "settings.gamma",
                changegammaaction
            ) &&
            router.registeraction(
                "settings.grass",
                grassaction
            ) &&
            router.registeraction(
                "settings.motion-blur",
                motionbluraction
            ) &&
            router.registeraction(
                "settings.glare-intensity",
                changeglareintensityaction
            ) &&
            router.registeraction(
                "settings.fps-limit",
                changefpslimitaction
            ) &&
            router.registeraction(
                "settings.texture-reduction",
                changetexturereductionaction
            ) &&
            router.registeraction(
                "settings.audio.sound-volume",
                changesoundvolumeaction
            ) &&
            router.registeraction(
                "settings.audio.music-volume",
                changemusicvolumeaction
            ) &&
            router.registeraction(
                "settings.input.sensitivity.decrease",
                decreasesensitivityaction
            ) &&
            router.registeraction(
                "settings.input.sensitivity.increase",
                increasesensitivityaction
            ) &&
            router.registeraction(
                "settings.input.sensitivity.focus",
                focussensitivityaction
            ) &&
            router.registeraction(
                "settings.input.sensitivity.blur",
                blursensitivityaction
            ) &&
            router.registeraction(
                "settings.input.sensitivity.commit",
                commitsensitivityaction
            ) &&
            router.registeraction(
                "settings.input.inverted-mouse",
                invertedmouseaction
            ) &&
            router.registeraction(
                "settings.crosshair.size.decrease",
                decreasecrosshairsizeaction
            ) &&
            router.registeraction(
                "settings.crosshair.size.increase",
                increasecrosshairsizeaction
            ) &&
            router.registeraction(
                "settings.crosshair.size.focus",
                focuscrosshairsizeaction
            ) &&
            router.registeraction(
                "settings.crosshair.size.blur",
                blurcrosshairsizeaction
            ) &&
            router.registeraction(
                "settings.crosshair.size.commit",
                commitcrosshairsizeaction
            ) &&
            router.registeraction(
                "settings.crosshair.effects",
                crosshaireffectsaction
            ) &&
            router.registeraction(
                "settings.crosshair.health-colors",
                crosshairhealthcolorsaction
            ) &&
            router.registeraction(
                "settings.crosshair.teammates",
                teamcrosshairaction
            ) &&
            router.registeraction(
                "settings.crosshair.hit-marker",
                hitcrosshairaction
            ) &&
            router.registeraction(
                "settings.crosshair.reload-fade",
                crosshairreloadfadeaction
            ) &&
            router.registeraction(
                "settings.crosshair.reload-rotation",
                crosshairreloadrotationaction
            ) &&
            router.registeraction(
                "settings.hud.show-fps",
                showfpsaction
            ) &&
            router.registeraction(
                "settings.hud.wall-clock",
                wallclockaction
            ) &&
            router.registeraction(
                "settings.hud.wall-clock.24-hour",
                wallclock24action
            ) &&
            router.registeraction(
                "settings.hud.wall-clock.seconds",
                wallclocksecondsaction
            ) &&
            router.registeraction(
                "settings.hud.damage-screen",
                damagescreenaction
            ) &&
            router.registeraction(
                "settings.hud.damage-compass",
                damagecompassaction
            ) &&
            router.registeraction(
                "settings.hud.guns",
                hudgunsaction
            ) &&
            router.registeraction(
                "settings.hud.guns.sway",
                hudgunswayaction
            ) &&
            router.registeraction(
                "settings.hud.guns.muzzle-flash",
                muzzleflashaction
            ) &&
            router.registeraction(
                "settings.hud.guns.muzzle-light",
                muzzlelightaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.enabled",
                fragmessagesenabledaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.insta",
                fragmessagesinstaaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.self",
                fragmessageselffragsaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.other",
                fragmessageotherfragsaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.team-kills",
                fragmessageteamkillsaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.fade",
                fragmessagefadeaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.max",
                changemaxfragmessagesaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.duration.decrease",
                decreasefragmessagedurationaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.duration.increase",
                increasefragmessagedurationaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.duration.focus",
                focusfragmessagedurationaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.duration.blur",
                blurfragmessagedurationaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.duration.commit",
                commitfragmessagedurationaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.scale.decrease",
                decreasefragmessagescaleaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.scale.increase",
                increasefragmessagescaleaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.scale.focus",
                focusfragmessagescaleaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.scale.blur",
                blurfragmessagescaleaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.scale.commit",
                commitfragmessagescaleaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.x.decrease",
                decreasefragmessagexaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.x.increase",
                increasefragmessagexaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.x.focus",
                focusfragmessagexaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.x.blur",
                blurfragmessagexaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.x.commit",
                commitfragmessagexaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.y.decrease",
                decreasefragmessageyaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.y.increase",
                increasefragmessageyaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.y.focus",
                focusfragmessageyaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.y.blur",
                blurfragmessageyaction
            ) &&
            router.registeraction(
                "settings.hud.frag-messages.y.commit",
                commitfragmessageyaction
            ) &&
            router.registeraction(
                "settings.hud.streak-scale.decrease",
                decreasestreakscaleaction
            ) &&
            router.registeraction(
                "settings.hud.streak-scale.increase",
                increasestreakscaleaction
            ) &&
            router.registeraction(
                "settings.hud.streak-scale.focus",
                focusstreakscaleaction
            ) &&
            router.registeraction(
                "settings.hud.streak-scale.blur",
                blurstreakscaleaction
            ) &&
            router.registeraction(
                "settings.hud.streak-scale.commit",
                commitstreakscaleaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.icon-spacing",
                changeammobariconspacingaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.label-offset",
                changeammobarlabeloffsetaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.scale.decrease",
                decreaseammobarscaleaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.scale.increase",
                increaseammobarscaleaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.scale.focus",
                focusammobarscaleaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.scale.blur",
                blurammobarscaleaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.scale.commit",
                commitammobarscaleaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.label-size.decrease",
                decreaseammobarlabelsizeaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.label-size.increase",
                increaseammobarlabelsizeaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.label-size.focus",
                focusammobarlabelsizeaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.label-size.blur",
                blurammobarlabelsizeaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.label-size.commit",
                commitammobarlabelsizeaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.x.decrease",
                decreaseammobarxaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.x.increase",
                increaseammobarxaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.x.focus",
                focusammobarxaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.x.blur",
                blurammobarxaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.x.commit",
                commitammobarxaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.y.decrease",
                decreaseammobaryaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.y.increase",
                increaseammobaryaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.y.focus",
                focusammobaryaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.y.blur",
                blurammobaryaction
            ) &&
            router.registeraction(
                "settings.hud.ammobar.y.commit",
                commitammobaryaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.alpha",
                changegameclockalphaaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.scale.decrease",
                decreasegameclockscaleaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.scale.increase",
                increasegameclockscaleaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.scale.focus",
                focusgameclockscaleaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.scale.blur",
                blurgameclockscaleaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.scale.commit",
                commitgameclockscaleaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.x.decrease",
                decreasegameclockxaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.x.increase",
                increasegameclockxaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.x.focus",
                focusgameclockxaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.x.blur",
                blurgameclockxaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.x.commit",
                commitgameclockxaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.y.decrease",
                decreasegameclockyaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.y.increase",
                increasegameclockyaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.y.focus",
                focusgameclockyaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.y.blur",
                blurgameclockyaction
            ) &&
            router.registeraction(
                "settings.hud.gameclock.y.commit",
                commitgameclockyaction
            ) &&
            router.registeraction(
                "settings.hud.score.alpha",
                changehudscorealphaaction
            ) &&
            router.registeraction(
                "settings.hud.score.spacing",
                changehudscoresepaction
            ) &&
            router.registeraction(
                "settings.hud.score.scale.decrease",
                decreasehudscorescaleaction
            ) &&
            router.registeraction(
                "settings.hud.score.scale.increase",
                increasehudscorescaleaction
            ) &&
            router.registeraction(
                "settings.hud.score.scale.focus",
                focushudscorescaleaction
            ) &&
            router.registeraction(
                "settings.hud.score.scale.blur",
                blurhudscorescaleaction
            ) &&
            router.registeraction(
                "settings.hud.score.scale.commit",
                commithudscorescaleaction
            ) &&
            router.registeraction(
                "settings.hud.score.x.decrease",
                decreasehudscorexaction
            ) &&
            router.registeraction(
                "settings.hud.score.x.increase",
                increasehudscorexaction
            ) &&
            router.registeraction(
                "settings.hud.score.x.focus",
                focushudscorexaction
            ) &&
            router.registeraction(
                "settings.hud.score.x.blur",
                blurhudscorexaction
            ) &&
            router.registeraction(
                "settings.hud.score.x.commit",
                commithudscorexaction
            ) &&
            router.registeraction(
                "settings.hud.score.y.decrease",
                decreasehudscoreyaction
            ) &&
            router.registeraction(
                "settings.hud.score.y.increase",
                increasehudscoreyaction
            ) &&
            router.registeraction(
                "settings.hud.score.y.focus",
                focushudscoreyaction
            ) &&
            router.registeraction(
                "settings.hud.score.y.blur",
                blurhudscoreyaction
            ) &&
            router.registeraction(
                "settings.hud.score.y.commit",
                commithudscoreyaction
            ) &&
            router.registeraction(
                "settings.chat.top.public",
                publicchatintopfeedaction
            ) &&
            router.registeraction(
                "settings.chat.top.team",
                teamchatintopfeedaction
            ) &&
            router.registeraction(
                "settings.chat.bottom.public",
                publicchatinbottomboxaction
            ) &&
            router.registeraction(
                "settings.chat.bottom.team",
                teamchatinbottomboxaction
            ) &&
            router.registeraction(
                "settings.chat.team-colors",
                teamcoloredchatnamesaction
            ) &&
            router.registeraction(
                "settings.console.top.important",
                consoletopimportantaction
            ) &&
            router.registeraction(
                "settings.console.top.warnings",
                consoletopwarningsaction
            ) &&
            router.registeraction(
                "settings.console.top.errors",
                consoletoperrorsaction
            ) &&
            router.registeraction(
                "settings.console.top.script",
                consoletopscriptaction
            ) &&
            router.registeraction(
                "settings.console.top.game-events",
                consoletopgameeventsaction
            ) &&
            router.registeraction(
                "settings.console.top.self-frags",
                consoletopselffragsaction
            ) &&
            router.registeraction(
                "settings.console.top.other-frags",
                consoletopotherfragsaction
            ) &&
            router.registeraction(
                "settings.console.top.team-kills",
                consoletopteamkillsaction
            ) &&
            router.registeraction(
                "settings.console.full.important",
                consolefullimportantaction
            ) &&
            router.registeraction(
                "settings.console.full.warnings",
                consolefullwarningsaction
            ) &&
            router.registeraction(
                "settings.console.full.errors",
                consolefullerrorsaction
            ) &&
            router.registeraction(
                "settings.console.full.script",
                consolefullscriptaction
            ) &&
            router.registeraction(
                "settings.console.full.chat",
                consolefullchataction
            ) &&
            router.registeraction(
                "settings.console.full.team-chat",
                consolefullteamchataction
            ) &&
            router.registeraction(
                "settings.console.full.game-events",
                consolefullgameeventsaction
            ) &&
            router.registeraction(
                "settings.console.full.self-frags",
                consolefullselffragsaction
            ) &&
            router.registeraction(
                "settings.console.full.other-frags",
                consolefullotherfragsaction
            ) &&
            router.registeraction(
                "settings.console.full.team-kills",
                consolefullteamkillsaction
            ) &&
            router.registeraction(
                "settings.console.size.decrease",
                decreaseconsolesizeaction
            ) &&
            router.registeraction(
                "settings.console.size.increase",
                increaseconsolesizeaction
            ) &&
            router.registeraction(
                "settings.console.size.focus",
                focusconsolesizeaction
            ) &&
            router.registeraction(
                "settings.console.size.blur",
                blurconsolesizeaction
            ) &&
            router.registeraction(
                "settings.console.size.commit",
                commitconsolesizeaction
            );

        if(!valid)
        {
            context->RemoveDataModel("settings");
            return false;
        }

        model = constructor.GetModelHandle();
        instance = this;
        video_status = "Live engine state";
        vsync_status = "Runtime state";
        crosshair_status = "Immediate / existing Sauer config";
        soundpack_status = "Available through Sauer package roots";
        refresh();
        return true;
    }

    void RmlSettings::shutdown(Rml::Context *context)
    {
        cancelkeycapture();
        if(instance == this)
            instance = nullptr;

        if(context && model)
            context->RemoveDataModel("settings");

        model = Rml::DataModelHandle();
        resolutions.clear();
        crosshair_choices.clear();
        soundpack_choices.clear();
        keybindings_movement.clear();
        keybindings_combat.clear();
        keybindings_interface.clear();
        fov = 100;
        fov_minimum = 10;
        fov_maximum = 150;
        game_feedback_available = false;
        blood = true;
        hit_sound = false;
        outline_capture_meters = false;
        show_scoreboard_at_death = true;
        game_feedback_status.clear();
        trail_colors_available = false;
        trail_color_channel_minimum = 0;
        trail_color_channel_maximum = 255;
        trail_color_shotgun_red = 255;
        trail_color_shotgun_green = 200;
        trail_color_shotgun_blue = 100;
        trail_color_chaingun_red = 255;
        trail_color_chaingun_green = 200;
        trail_color_chaingun_blue = 100;
        trail_color_rocket_launcher_red = 64;
        trail_color_rocket_launcher_green = 64;
        trail_color_rocket_launcher_blue = 64;
        trail_color_rifle_red = 64;
        trail_color_rifle_green = 64;
        trail_color_rifle_blue = 64;
        trail_color_grenade_launcher_red = 64;
        trail_color_grenade_launcher_green = 64;
        trail_color_grenade_launcher_blue = 64;
        trail_color_pistol_red = 255;
        trail_color_pistol_green = 200;
        trail_color_pistol_blue = 100;
        trail_colors_status.clear();
        game_radar_available = false;
        numbered_capture_bases = false;
        show_teammates_on_radar = true;
        game_radar_status.clear();
        game_models_available = false;
        ragdoll_deaths = true;
        keep_ragdoll_after_respawn = true;
        deadpush = 2;
        deadpush_minimum = 1;
        deadpush_maximum = 20;
        hide_dead_players = false;
        fullbright_player_models = true;
        fullbright_intensity_selected = 60;
        force_matching_player_models = false;
        always_use_team_skins = false;
        game_models_status.clear();
        ammobar_available = false;
        ammobar_visible = false;
        ammobar_horizontal = false;
        ammobar_flip = false;
        ammobar_scale = 0.5f;
        ammobar_scale_minimum = 0.1f;
        ammobar_scale_maximum = 1.0f;
        ammobar_scale_input.clear();
        ammobar_scale_invalid = false;
        ammobar_scale_editing = false;
        ammobar_icon_spacing = 20;
        ammobar_icon_spacing_minimum = 0;
        ammobar_icon_spacing_maximum = 500;
        ammobar_icon_spacing_armed = false;
        ammobar_label_size = 1.5f;
        ammobar_label_size_minimum = 0.5f;
        ammobar_label_size_maximum = 2.0f;
        ammobar_label_size_input.clear();
        ammobar_label_size_invalid = false;
        ammobar_label_size_editing = false;
        ammobar_label_offset = 20;
        ammobar_label_offset_minimum = 0;
        ammobar_label_offset_maximum = 500;
        ammobar_label_offset_armed = false;
        ammobar_align_first = false;
        ammobar_align_center = true;
        ammobar_align_last = false;
        ammobar_x = 0.025f;
        ammobar_x_minimum = 0.0f;
        ammobar_x_maximum = 1.0f;
        ammobar_x_input.clear();
        ammobar_x_invalid = false;
        ammobar_x_editing = false;
        ammobar_y = 0.5f;
        ammobar_y_minimum = 0.0f;
        ammobar_y_maximum = 1.0f;
        ammobar_y_input.clear();
        ammobar_y_invalid = false;
        ammobar_y_editing = false;
        ammobar_hide_empty = true;
        ammobar_status.clear();
        scoreboard_available = false;
        scoreboard_flags = false;
        scoreboard_frags = false;
        scoreboard_deaths = false;
        scoreboard_suicides = false;
        scoreboard_kpd = false;
        scoreboard_accuracy = false;
        scoreboard_damage = false;
        scoreboard_damage_dealt = false;
        scoreboard_damage_net = false;
        scoreboard_damage_received = false;
        scoreboard_ping = true;
        scoreboard_pj = false;
        scoreboard_clientnum = false;
        scoreboard_status.clear();
        resolution_label.clear();
        render_resolution_label.clear();
        video_status.clear();
        presentation_label.clear();
        vsync_status.clear();
        gamma = 100;
        gamma_minimum = 30;
        gamma_maximum = 300;
        gamma_label.clear();
        gamma_status.clear();
        shadow_state_available = false;
        shadow_maps = false;
        shadow_maps_available = false;
        blob_shadows = false;
        shadow_status.clear();
        shadow_quality_available = false;
        shadow_quality_selected = -1;
        shadow_quality_label.clear();
        shadow_quality_status.clear();
        water_surface_state_available = false;
        water_refraction = false;
        water_refraction_available = false;
        water_reflections = false;
        water_reflections_available = false;
        water_caustics = false;
        liquid_surface_animation = false;
        water_surface_status.clear();
        water_environment_reflections = false;
        water_environment_reflections_available = false;
        water_environment_reflections_status.clear();
        water_render_resolution_available = false;
        water_render_resolution_selected = -1;
        water_render_resolution_label.clear();
        water_render_resolution_dimension.clear();
        water_render_resolution_status.clear();
        waterfall_state_available = false;
        waterfall_refraction = false;
        waterfall_refraction_available = false;
        waterfall_reflections = false;
        waterfall_status.clear();
        glass_reflections_available = false;
        glass_reflections = false;
        glass_reflections_status.clear();
        model_reflections_available = false;
        model_reflections = false;
        model_reflections_status.clear();
        bump_models_available = false;
        bump_models = false;
        bump_models_status.clear();
        environment_map_resolution_available = false;
        environment_map_resolution_selected = -1;
        environment_map_resolution_label.clear();
        environment_map_resolution_dimension.clear();
        environment_map_resolution_status.clear();
        dynamic_lights_available = false;
        dynamic_lights_selected = -1;
        dynamic_lights_label.clear();
        dynamic_lights_status.clear();
        soft_particles_available = false;
        soft_particles = false;
        soft_particles_status.clear();
        soft_particle_quality_available = false;
        soft_particle_quality_selected = -1;
        soft_particle_quality_label.clear();
        soft_particle_quality_status.clear();
        decals_available = false;
        decals = false;
        decals_status.clear();
        decal_persistence_available = false;
        decal_persistence_selected = -1;
        decal_persistence_label.clear();
        decal_persistence_status.clear();
        fix_t_joints_available = false;
        fix_t_joints = false;
        fix_t_joints_status.clear();
        grass = false;
        grass_distance_available = false;
        grass_distance_selected = -1;
        grass_distance_label.clear();
        grass_distance_status.clear();
        motion_blur = motion_blur_available = false;
        motion_blur_intensity_available = false;
        motion_blur_intensity_selected = -1;
        motion_blur_intensity_label.clear();
        motion_blur_intensity_status.clear();
        glare = false;
        glare_preset_available = false;
        glare_preset_selected = -1;
        glare_preset_label.clear();
        glare_preset_status.clear();
        glare_resolution_available = false;
        glare_resolution_selected = -1;
        glare_resolution_label.clear();
        glare_resolution_requested_width = 0;
        glare_resolution_status.clear();
        texture_quality_available = false;
        texture_quality_selected = -1;
        texture_quality_label.clear();
        texture_quality_status.clear();
        texture_compression_selected = -1;
        texture_compression_available = false;
        texture_compression_supported = false;
        texture_compression_label.clear();
        texture_compression_status.clear();
        graphics_quality_status.clear();
        anisotropy_enabled = false;
        bilinear_filtering = bilinear_filtering_available = false;
        bilinear_filtering_status.clear();
        mipmap_blending = mipmap_blending_available = false;
        mipmap_blending_status.clear();
        anisotropy_maximum = 0;
        anisotropy_selected = -1;
        anisotropy_label.clear();
        anisotropy_status.clear();
        fps_limit_label.clear();
        presentation_windowed = true;
        presentation_exclusive = false;
        presentation_desktop = false;
        render_resolution_visible = false;
        resolution_enabled = true;
        vsync = false;
        vsync_adaptive = false;
        fps_limit = 200;
        fps_limit_minimum = 0;
        fps_limit_maximum = 1000;
        antialiasing = 0;
        antialiasing_requested = -1;
        pending_antialiasing = -1;
        depth_buffer = 0;
        depth_buffer_requested = -1;
        pending_depth_buffer = -1;
        pending_shader_detail = shader_detail_selected = -1;
        shader_detail_available = false;
        shader_detail_label.clear();
        shader_detail_status.clear();
        antialiasing_label.clear();
        antialiasing_status.clear();
        antialiasing_off = true;
        antialiasing_2x = false;
        antialiasing_4x = false;
        antialiasing_8x = false;
        antialiasing_16x = false;
        depth_buffer_label.clear();
        depth_buffer_status.clear();
        depth_buffer_default = true;
        depth_buffer_16 = false;
        depth_buffer_24 = false;
        depth_buffer_32 = false;
        sound_volume = 255;
        sound_volume_minimum = 0;
        sound_volume_maximum = 255;
        music_volume = 128;
        music_volume_minimum = 0;
        music_volume_maximum = 255;
        sound_volume_label.clear();
        music_volume_label.clear();
        audio_status.clear();
        active_soundpack.clear();
        active_soundpack_label.clear();
        soundpack_status.clear();
        mouse_sensitivity = 3.0f;
        mouse_sensitivity_minimum = 0.001f;
        mouse_sensitivity_maximum = 1000.0f;
        mouse_sensitivity_label.clear();
        mouse_sensitivity_input.clear();
        mouse_sensitivity_status.clear();
        mouse_sensitivity_invalid = false;
        mouse_sensitivity_editing = false;
        inverted_mouse = false;
        mouse_input_status.clear();
        crosshair_selection = -1;
        crosshair_selection_label.clear();
        crosshair_selection_resource.clear();
        crosshair_size = 30;
        crosshair_size_minimum = 0;
        crosshair_size_maximum = 50;
        crosshair_size_input.clear();
        crosshair_status.clear();
        crosshair_size_invalid = false;
        crosshair_size_editing = false;
        crosshair_effects = true;
        crosshair_health_colors = true;
        team_crosshair = true;
        hit_crosshair = true;
        crosshair_reload_fade = true;
        crosshair_reload_rotation = true;
        show_fps = true;
        wall_clock = false;
        wall_clock_24_hour = false;
        wall_clock_seconds = false;
        damage_screen = true;
        damage_compass = true;
        hud_guns = true;
        hud_gun_sway = true;
        muzzle_flash = true;
        muzzle_light = true;
        frag_messages_enabled = true;
        frag_messages_in_insta = true;
        max_frag_messages = 3;
        max_frag_messages_minimum = 1;
        max_frag_messages_maximum = 10;
        max_frag_messages_armed = false;
        frag_message_duration = 2.0f;
        frag_message_duration_minimum = 0.0f;
        frag_message_duration_maximum = 10.0f;
        frag_message_duration_input.clear();
        frag_message_duration_invalid = false;
        frag_message_duration_editing = false;
        frag_message_self_frags = true;
        frag_message_other_frags = false;
        frag_message_team_kills = true;
        frag_message_scale = 0.5f;
        frag_message_scale_minimum = 0.1f;
        frag_message_scale_maximum = 1.0f;
        frag_message_scale_input.clear();
        frag_message_scale_invalid = false;
        frag_message_scale_editing = false;
        frag_message_fade = true;
        frag_message_x = 0.5f;
        frag_message_x_minimum = 0.0f;
        frag_message_x_maximum = 1.0f;
        frag_message_x_input.clear();
        frag_message_x_invalid = false;
        frag_message_x_editing = false;
        frag_message_y = 0.25f;
        frag_message_y_minimum = 0.0f;
        frag_message_y_maximum = 1.0f;
        frag_message_y_input.clear();
        frag_message_y_invalid = false;
        frag_message_y_editing = false;
        frag_message_stack_up = false;
        frag_message_stack_auto = true;
        frag_message_stack_down = false;
        streak_milestone_scale = 1.25f;
        streak_milestone_scale_minimum = 1.0f;
        streak_milestone_scale_maximum = 2.0f;
        streak_milestone_scale_input.clear();
        hud_status.clear();
        streak_milestone_scale_invalid = false;
        streak_milestone_scale_editing = false;
        game_clock_available = false;
        game_clock = false;
        game_clock_scale = 1.0f;
        game_clock_scale_minimum = 0.5f;
        game_clock_scale_maximum = 2.0f;
        game_clock_scale_input.clear();
        game_clock_scale_invalid = false;
        game_clock_scale_editing = false;
        game_clock_alpha = 255;
        game_clock_alpha_minimum = 0;
        game_clock_alpha_maximum = 255;
        game_clock_align = 0;
        game_clock_x = 0.50f;
        game_clock_x_minimum = 0.0f;
        game_clock_x_maximum = 1.0f;
        game_clock_x_input.clear();
        game_clock_x_invalid = false;
        game_clock_x_editing = false;
        game_clock_y = 0.03f;
        game_clock_y_minimum = 0.0f;
        game_clock_y_maximum = 1.0f;
        game_clock_y_input.clear();
        game_clock_y_invalid = false;
        game_clock_y_editing = false;
        game_clock_status.clear();
        hudscore_available = false;
        hudscore = false;
        hudscore_scale = 1.0f;
        hudscore_scale_minimum = 0.5f;
        hudscore_scale_maximum = 2.0f;
        hudscore_scale_input.clear();
        hudscore_scale_invalid = false;
        hudscore_scale_editing = false;
        hudscore_alpha = 255;
        hudscore_alpha_minimum = 0;
        hudscore_alpha_maximum = 255;
        hudscore_sep = 200;
        hudscore_sep_minimum = 0;
        hudscore_sep_maximum = 1000;
        hudscore_align = 0;
        hudscore_x = 0.50f;
        hudscore_x_minimum = 0.0f;
        hudscore_x_maximum = 1.0f;
        hudscore_x_input.clear();
        hudscore_x_invalid = false;
        hudscore_x_editing = false;
        hudscore_y = 0.03f;
        hudscore_y_minimum = 0.0f;
        hudscore_y_maximum = 1.0f;
        hudscore_y_input.clear();
        hudscore_y_invalid = false;
        hudscore_y_editing = false;
        hudscore_status.clear();
        public_chat_in_top_feed = true;
        team_chat_in_top_feed = true;
        public_chat_in_bottom_box = false;
        team_chat_in_bottom_box = false;
        team_colored_chat_names = true;
        console_available = false;
        console_top_important = true;
        console_top_warnings = true;
        console_top_errors = true;
        console_top_script = true;
        console_top_game_events = true;
        console_top_self_frags = true;
        console_top_other_frags = true;
        console_top_team_kills = true;
        console_full_important = true;
        console_full_warnings = true;
        console_full_errors = true;
        console_full_script = true;
        console_full_chat = true;
        console_full_team_chat = true;
        console_full_game_events = true;
        console_full_self_frags = true;
        console_full_other_frags = true;
        console_full_team_kills = true;
        console_size = 5;
        console_size_minimum = 0;
        console_size_maximum = 100;
        console_size_input.clear();
        console_size_invalid = false;
        console_size_editing = false;
        console_status.clear();
        category_game = true;
        category_hud = false;
        category_graphics = false;
        category_audio = false;
        category_mouse_keyboard = false;
        category_crosshair = false;
        category_chat = false;
        category_console = false;
    }

    void RmlSettings::activate()
    {
        pending_shader_detail = -1;
        refresh();
    }

    void RmlSettings::videochanged()
    {
        refreshvideo();
        // afterglreset precedes reloadshaders: the helper reports unavailable
        // here, never a final post-reset health claim.
        refreshshaderdetail();
    }

    void RmlSettings::applypending()
    {
        const bool reset_antialiasing = pending_antialiasing >= 0;
        const bool reset_depth_buffer = pending_depth_buffer >= 0;
        if(!reset_antialiasing && !reset_depth_buffer)
        {
            applypendingshaderdetail();
            return;
        }

        const int requested_antialiasing = pending_antialiasing;
        const int requested_depth_buffer = pending_depth_buffer;
        pending_antialiasing = -1;
        pending_depth_buffer = -1;
        settings::resetgraphics();
        refreshvideo();

        if(reset_antialiasing)
        {
            char status[96];
            if(antialiasing == requested_antialiasing)
            {
                if(requested_antialiasing)
                {
                    std::snprintf(
                        status,
                        sizeof(status),
                        "%dx multisample antialiasing active",
                        requested_antialiasing
                    );
                }
                else std::snprintf(
                    status,
                    sizeof(status),
                    "Antialiasing disabled"
                );
            }
            else
            {
                std::snprintf(
                    status,
                    sizeof(status),
                    "%dx was unavailable; runtime is %s",
                    requested_antialiasing,
                    antialiasing ? antialiasing_label.c_str() : "off"
                );
            }

            antialiasing_status = status;
            model.DirtyVariable("antialiasing_status");
        }

        if(reset_depth_buffer)
        {
            const int engine_bits = requested_depth_buffer
                ? requested_depth_buffer
                : 24;
            char status[96];
            if(depth_buffer == engine_bits)
            {
                if(requested_depth_buffer == 0)
                {
                    std::snprintf(
                        status,
                        sizeof(status),
                        "Engine default 24-bit depth active"
                    );
                }
                else
                {
                    std::snprintf(
                        status,
                        sizeof(status),
                        "%d-bit depth buffer active",
                        requested_depth_buffer
                    );
                }
            }
            else if(requested_depth_buffer == 0)
            {
                std::snprintf(
                    status,
                    sizeof(status),
                    "Engine default 24-bit was unavailable; runtime is %s",
                    depth_buffer_label.c_str()
                );
            }
            else
            {
                std::snprintf(
                    status,
                    sizeof(status),
                    "%d-bit was unavailable; runtime is %s",
                    requested_depth_buffer,
                    depth_buffer_label.c_str()
                );
            }

            depth_buffer_status = status;
            model.DirtyVariable("depth_buffer_status");
        }

        refreshshaderdetail(); // resetgraphics has now completed reloadshaders.
        applypendingshaderdetail();
    }

    bool RmlSettings::refresh()
    {
        settings::Fov current;
        if(!model || !settings::readfov(current))
            return false;

        if(fov != current.value)
        {
            fov = current.value;
            model.DirtyVariable("fov");
        }

        if(fov_minimum != current.minimum)
        {
            fov_minimum = current.minimum;
            model.DirtyVariable("fov_minimum");
        }

        if(fov_maximum != current.maximum)
        {
            fov_maximum = current.maximum;
            model.DirtyVariable("fov_maximum");
        }

        if(!refreshgamefeedback())
            return false;
        if(!refreshtrailcolors())
            return false;
        if(!refreshgameradar())
            return false;
        if(!refreshgamemodels())
            return false;
        if(!refreshammobar())
            return false;
        if(!refreshscoreboardcolumns())
            return false;
        if(!refreshvideo())
            return false;
        refreshshaderdetail();
        if(!refreshaudio())
            return false;
        if(!refreshmouseinput(!mouse_sensitivity_editing))
            return false;
        if(!refreshcrosshair(!crosshair_size_editing))
            return false;
        if(!refreshhud(!streak_milestone_scale_editing))
            return false;
        if(!refreshgameclock())
            return false;
        if(!refreshhudscore())
            return false;
        if(!refreshchat())
            return false;
        if(!refreshconsole())
            return false;
        return refreshkeybindings();
    }

    bool RmlSettings::refreshvideo()
    {
        refreshgraphicsquality();
        refreshshadows();
        refreshshadowquality();
        refreshwatersurfaceeffects();
        refreshwaterenvironmentreflections();
        refreshwaterrenderresolution();
        refreshwaterfalleffects();
        refreshglassreflections();
        refreshmodelreflections();
        refreshbumpmodels();
        refreshenvironmentmapresolution();
        refreshdynamiclights();
        refreshsoftparticles();
        refreshsoftparticlequality();
        refreshdecals();
        refreshdecalpersistence();
        refreshfixtjoints();
        refreshgrassdistance();
        refreshmotionblurintensity();
        refreshglarepreset();
        refreshglareintensity();
        refreshglareresolution();
        refreshtexturequality();
        refreshtexturereduction();
        refreshtexturecompression();
        refreshanisotropy();
        refreshbilinearfiltering();
        refreshmipmapblending();
        refreshgamma();
        settings::Video current;
        if(!settings::readvideo(current))
            return false;

        const bool windowed = current.presentation ==
            settings::PresentationMode::Windowed;
        const bool exclusive = current.presentation ==
            settings::PresentationMode::ExclusiveFullscreen;
        const bool desktop = current.presentation ==
            settings::PresentationMode::DesktopFullscreen;

        if(presentation_windowed != windowed)
        {
            presentation_windowed = windowed;
            model.DirtyVariable("presentation_windowed");
        }
        if(presentation_exclusive != exclusive)
        {
            presentation_exclusive = exclusive;
            model.DirtyVariable("presentation_exclusive");
        }
        if(presentation_desktop != desktop)
        {
            presentation_desktop = desktop;
            model.DirtyVariable("presentation_desktop");
        }

        const char *mode_label = windowed
            ? "Windowed"
            : (desktop ? "Desktop fullscreen" : "Exclusive fullscreen");
        if(presentation_label != mode_label)
        {
            presentation_label = mode_label;
            model.DirtyVariable("presentation_label");
        }

        const bool can_select_resolution = !desktop;
        if(resolution_enabled != can_select_resolution)
        {
            resolution_enabled = can_select_resolution;
            model.DirtyVariable("resolution_enabled");
        }

        char label[64];
        std::snprintf(
            label,
            sizeof(label),
            "%d x %d",
            current.width,
            current.height
        );
        if(resolution_label != label)
        {
            resolution_label = label;
            model.DirtyVariable("resolution_label");
        }

        const bool show_render_resolution =
            current.width != current.drawable_width ||
            current.height != current.drawable_height;
        if(render_resolution_visible != show_render_resolution)
        {
            render_resolution_visible = show_render_resolution;
            model.DirtyVariable("render_resolution_visible");
        }
        if(show_render_resolution)
        {
            std::snprintf(
                label,
                sizeof(label),
                "%d x %d",
                current.drawable_width,
                current.drawable_height
            );
            if(render_resolution_label != label)
            {
                render_resolution_label = label;
                model.DirtyVariable("render_resolution_label");
            }
        }

        std::vector<settings::Resolution> available;
        settings::visitresolutions(appendresolution, &available);
        std::sort(available.begin(), available.end(), resolutionorder);
        available.erase(
            std::unique(
                available.begin(),
                available.end(),
                sameresolution
            ),
            available.end()
        );

        std::vector<Resolution> next;
        bool current_is_supported = false;
        for(std::vector<settings::Resolution>::const_iterator resolution =
                available.begin();
            resolution != available.end();
            ++resolution)
        {
            Resolution row;
            row.width = resolution->width;
            row.height = resolution->height;
            row.selected = row.width == current.width &&
                row.height == current.height;
            row.supported = true;
            std::snprintf(
                label,
                sizeof(label),
                "%d x %d",
                row.width,
                row.height
            );
            row.label = label;
            current_is_supported = current_is_supported || row.selected;
            next.push_back(row);
        }

        if(!current_is_supported)
        {
            Resolution row;
            row.width = current.width;
            row.height = current.height;
            row.selected = true;
            row.supported = false;
            std::snprintf(
                label,
                sizeof(label),
                "%d x %d / custom",
                row.width,
                row.height
            );
            row.label = label;
            next.push_back(row);
        }

        if(resolutions != next)
        {
            resolutions.swap(next);
            model.DirtyVariable("resolutions");
        }

        settings::Vsync current_vsync;
        if(!settings::readvsync(current_vsync))
            return false;

        if(vsync != current_vsync.enabled)
        {
            vsync = current_vsync.enabled;
            model.DirtyVariable("vsync");
        }
        if(vsync_adaptive != current_vsync.adaptive)
        {
            vsync_adaptive = current_vsync.adaptive;
            model.DirtyVariable("vsync_adaptive");
        }

        const char *current_vsync_status = current_vsync.adaptive
            ? "Adaptive VSync active"
            : (current_vsync.enabled ? "VSync active" : "VSync disabled");
        if(vsync_status != current_vsync_status)
        {
            vsync_status = current_vsync_status;
            model.DirtyVariable("vsync_status");
        }

        settings::FpsLimit current_fps_limit;
        if(!settings::readfpslimit(current_fps_limit))
            return false;

        if(fps_limit != current_fps_limit.value)
        {
            fps_limit = current_fps_limit.value;
            model.DirtyVariable("fps_limit");
        }
        if(fps_limit_minimum != current_fps_limit.minimum)
        {
            fps_limit_minimum = current_fps_limit.minimum;
            model.DirtyVariable("fps_limit_minimum");
        }
        if(fps_limit_maximum != current_fps_limit.maximum)
        {
            fps_limit_maximum = current_fps_limit.maximum;
            model.DirtyVariable("fps_limit_maximum");
        }

        if(current_fps_limit.value)
        {
            std::snprintf(
                label,
                sizeof(label),
                "%d FPS",
                current_fps_limit.value
            );
        }
        else std::snprintf(label, sizeof(label), "Unlimited");

        if(fps_limit_label != label)
        {
            fps_limit_label = label;
            model.DirtyVariable("fps_limit_label");
        }

        settings::Antialiasing current_antialiasing;
        if(!settings::readantialiasing(current_antialiasing))
            return false;

        antialiasing_requested =
            current_antialiasing.requested_samples;

        if(antialiasing != current_antialiasing.actual_samples)
        {
            antialiasing = current_antialiasing.actual_samples;
            model.DirtyVariable("antialiasing");
        }

        const bool aa_off = current_antialiasing.actual_samples == 0;
        const bool aa_2x = current_antialiasing.actual_samples == 2;
        const bool aa_4x = current_antialiasing.actual_samples == 4;
        const bool aa_8x = current_antialiasing.actual_samples == 8;
        const bool aa_16x = current_antialiasing.actual_samples == 16;
        if(antialiasing_off != aa_off)
        {
            antialiasing_off = aa_off;
            model.DirtyVariable("antialiasing_off");
        }
        if(antialiasing_2x != aa_2x)
        {
            antialiasing_2x = aa_2x;
            model.DirtyVariable("antialiasing_2x");
        }
        if(antialiasing_4x != aa_4x)
        {
            antialiasing_4x = aa_4x;
            model.DirtyVariable("antialiasing_4x");
        }
        if(antialiasing_8x != aa_8x)
        {
            antialiasing_8x = aa_8x;
            model.DirtyVariable("antialiasing_8x");
        }
        if(antialiasing_16x != aa_16x)
        {
            antialiasing_16x = aa_16x;
            model.DirtyVariable("antialiasing_16x");
        }

        if(current_antialiasing.actual_samples)
        {
            std::snprintf(
                label,
                sizeof(label),
                "%dx MSAA",
                current_antialiasing.actual_samples
            );
        }
        else std::snprintf(label, sizeof(label), "Off");

        if(antialiasing_label != label)
        {
            antialiasing_label = label;
            model.DirtyVariable("antialiasing_label");
        }

        char aa_status[96];
        if(current_antialiasing.requested_samples > 0 &&
           current_antialiasing.requested_samples !=
               current_antialiasing.actual_samples)
        {
            std::snprintf(
                aa_status,
                sizeof(aa_status),
                "%dx requested; runtime is %s",
                current_antialiasing.requested_samples,
                current_antialiasing.actual_samples
                    ? label
                    : "off"
            );
        }
        else if(current_antialiasing.actual_samples)
        {
            std::snprintf(
                aa_status,
                sizeof(aa_status),
                "%dx multisample antialiasing active",
                current_antialiasing.actual_samples
            );
        }
        else std::snprintf(aa_status, sizeof(aa_status), "Antialiasing disabled");

        if(antialiasing_status != aa_status)
        {
            antialiasing_status = aa_status;
            model.DirtyVariable("antialiasing_status");
        }

        settings::DepthBuffer current_depth_buffer;
        if(!settings::readdepthbits(current_depth_buffer))
            return false;

        depth_buffer_requested = current_depth_buffer.requested_bits;

        if(depth_buffer != current_depth_buffer.actual_bits)
        {
            depth_buffer = current_depth_buffer.actual_bits;
            model.DirtyVariable("depth_buffer");
        }

        const bool depth_default = current_depth_buffer.requested_bits == 0;
        const bool depth_16 = current_depth_buffer.requested_bits == 16;
        const bool depth_24 = current_depth_buffer.requested_bits == 24;
        const bool depth_32 = current_depth_buffer.requested_bits == 32;
        if(depth_buffer_default != depth_default)
        {
            depth_buffer_default = depth_default;
            model.DirtyVariable("depth_buffer_default");
        }
        if(depth_buffer_16 != depth_16)
        {
            depth_buffer_16 = depth_16;
            model.DirtyVariable("depth_buffer_16");
        }
        if(depth_buffer_24 != depth_24)
        {
            depth_buffer_24 = depth_24;
            model.DirtyVariable("depth_buffer_24");
        }
        if(depth_buffer_32 != depth_32)
        {
            depth_buffer_32 = depth_32;
            model.DirtyVariable("depth_buffer_32");
        }

        formatdepthbufferlabel(
            current_depth_buffer.actual_bits,
            label,
            sizeof(label)
        );
        if(depth_buffer_label != label)
        {
            depth_buffer_label = label;
            model.DirtyVariable("depth_buffer_label");
        }

        char depth_status[96];
        formatdepthbufferstatus(
            current_depth_buffer.requested_bits,
            current_depth_buffer.actual_bits,
            label,
            depth_status,
            sizeof(depth_status)
        );
        if(depth_buffer_status != depth_status)
        {
            depth_buffer_status = depth_status;
            model.DirtyVariable("depth_buffer_status");
        }
        return true;
    }

    bool RmlSettings::refreshaudio()
    {
        settings::Audio current;
        if(!settings::readaudio(current))
            return false;

        std::vector<SoundpackChoice> available_soundpacks;
        if(!settings::visitsoundpacks(
                appendsoundpackchoice,
                &available_soundpacks))
        {
            return false;
        }

        const Rml::String current_soundpack = current.active_soundpack;
        Rml::String current_soundpack_label = current_soundpack;
        bool active_found = false;
        for(std::vector<SoundpackChoice>::iterator choice =
                available_soundpacks.begin();
            choice != available_soundpacks.end();
            ++choice)
        {
            choice->selected = choice->id == current_soundpack;
            if(choice->selected)
            {
                current_soundpack_label = choice->label;
                active_found = true;
            }
        }

        if(soundpack_choices != available_soundpacks)
        {
            soundpack_choices.swap(available_soundpacks);
            model.DirtyVariable("soundpack_choices");
        }
        if(active_soundpack != current_soundpack)
        {
            active_soundpack = current_soundpack;
            model.DirtyVariable("active_soundpack");
        }
        if(active_soundpack_label != current_soundpack_label)
        {
            active_soundpack_label = current_soundpack_label;
            model.DirtyVariable("active_soundpack_label");
        }

        char pack_status[128];
        if(active_found)
        {
            std::snprintf(
                pack_status,
                sizeof(pack_status),
                "%d available // gameplay sounds reload on change",
                int(soundpack_choices.size())
            );
        }
        else
        {
            std::snprintf(
                pack_status,
                sizeof(pack_status),
                "Active pack is unavailable; select a discovered pack"
            );
        }
        if(soundpack_status != pack_status)
        {
            soundpack_status = pack_status;
            model.DirtyVariable("soundpack_status");
        }

        if(sound_volume != current.sound_volume)
        {
            sound_volume = current.sound_volume;
            model.DirtyVariable("sound_volume");
        }
        if(sound_volume_minimum != current.sound_volume_minimum)
        {
            sound_volume_minimum = current.sound_volume_minimum;
            model.DirtyVariable("sound_volume_minimum");
        }
        if(sound_volume_maximum != current.sound_volume_maximum)
        {
            sound_volume_maximum = current.sound_volume_maximum;
            model.DirtyVariable("sound_volume_maximum");
        }
        if(music_volume != current.music_volume)
        {
            music_volume = current.music_volume;
            model.DirtyVariable("music_volume");
        }
        if(music_volume_minimum != current.music_volume_minimum)
        {
            music_volume_minimum = current.music_volume_minimum;
            model.DirtyVariable("music_volume_minimum");
        }
        if(music_volume_maximum != current.music_volume_maximum)
        {
            music_volume_maximum = current.music_volume_maximum;
            model.DirtyVariable("music_volume_maximum");
        }

        char label[32];
        const int sound_percent = current.sound_volume_maximum > 0
            ? (current.sound_volume * 100 +
                current.sound_volume_maximum / 2) /
                current.sound_volume_maximum
            : 0;
        std::snprintf(label, sizeof(label), "%d%%", sound_percent);
        if(sound_volume_label != label)
        {
            sound_volume_label = label;
            model.DirtyVariable("sound_volume_label");
        }

        const int music_percent = current.music_volume_maximum > 0
            ? (current.music_volume * 100 +
                current.music_volume_maximum / 2) /
                current.music_volume_maximum
            : 0;
        std::snprintf(label, sizeof(label), "%d%%", music_percent);
        if(music_volume_label != label)
        {
            music_volume_label = label;
            model.DirtyVariable("music_volume_label");
        }

        const char *status = !current.backend_active
            ? "Audio backend unavailable"
            : (current.sound_volume
                ? "Audio backend active"
                : "All audio muted by Sound Volume");
        if(audio_status != status)
        {
            audio_status = status;
            model.DirtyVariable("audio_status");
        }
        return true;
    }

    bool RmlSettings::refreshmouseinput(bool update_edit_buffer)
    {
        settings::MouseInput current;
        if(!settings::readmouseinput(current))
            return false;

        if(mouse_sensitivity != current.sensitivity)
        {
            mouse_sensitivity = current.sensitivity;
            model.DirtyVariable("mouse_sensitivity");
        }
        if(mouse_sensitivity_minimum != current.sensitivity_minimum)
        {
            mouse_sensitivity_minimum = current.sensitivity_minimum;
            model.DirtyVariable("mouse_sensitivity_minimum");
        }
        if(mouse_sensitivity_maximum != current.sensitivity_maximum)
        {
            mouse_sensitivity_maximum = current.sensitivity_maximum;
            model.DirtyVariable("mouse_sensitivity_maximum");
        }
        if(inverted_mouse != current.inverted)
        {
            inverted_mouse = current.inverted;
            model.DirtyVariable("inverted_mouse");
        }

        const Rml::String label = formatsensitivity(current.sensitivity);
        if(mouse_sensitivity_label != label)
        {
            mouse_sensitivity_label = label;
            model.DirtyVariable("mouse_sensitivity_label");
        }
        if(update_edit_buffer && mouse_sensitivity_input != label)
        {
            mouse_sensitivity_input = label;
            model.DirtyVariable("mouse_sensitivity_input");
        }

        const char *status = current.inverted
            ? "Vertical look inverted"
            : "Standard vertical look";
        if(mouse_input_status != status)
        {
            mouse_input_status = status;
            model.DirtyVariable("mouse_input_status");
        }
        return true;
    }

    bool RmlSettings::refreshcrosshair(bool update_edit_buffer)
    {
        settings::Crosshair current;
        if(!settings::readcrosshair(current))
            return false;

        if(crosshair_selection != current.selection)
        {
            crosshair_selection = current.selection;
        }

        Rml::String selection_label = "Custom";
        bool choices_changed = false;
        for(std::vector<CrosshairChoice>::iterator choice =
                crosshair_choices.begin();
            choice != crosshair_choices.end();
            ++choice)
        {
            const bool selected = choice->id == current.selection;
            if(choice->selected != selected)
            {
                choice->selected = selected;
                choices_changed = true;
            }
            if(selected)
                selection_label = choice->label;
        }
        if(choices_changed)
            model.DirtyVariable("crosshair_choices");
        if(crosshair_selection_label != selection_label)
        {
            crosshair_selection_label = selection_label;
            model.DirtyVariable("crosshair_selection_label");
        }
        const Rml::String selection_resource =
            rmlcrosshairresource(current.resource);
        if(crosshair_selection_resource != selection_resource)
        {
            crosshair_selection_resource = selection_resource;
            model.DirtyVariable("crosshair_selection_resource");
        }

        if(crosshair_size != current.size)
        {
            crosshair_size = current.size;
            model.DirtyVariable("crosshair_size");
        }
        if(crosshair_size_minimum != current.size_minimum)
        {
            crosshair_size_minimum = current.size_minimum;
            model.DirtyVariable("crosshair_size_minimum");
        }
        if(crosshair_size_maximum != current.size_maximum)
        {
            crosshair_size_maximum = current.size_maximum;
            model.DirtyVariable("crosshair_size_maximum");
        }

        char size_text[16];
        std::snprintf(size_text, sizeof(size_text), "%d", current.size);
        if(update_edit_buffer && crosshair_size_input != size_text)
        {
            crosshair_size_input = size_text;
            model.DirtyVariable("crosshair_size_input");
        }

        if(crosshair_effects != current.effects)
        {
            crosshair_effects = current.effects;
            model.DirtyVariable("crosshair_effects");
        }
        if(crosshair_health_colors != current.health_colors)
        {
            crosshair_health_colors = current.health_colors;
            model.DirtyVariable("crosshair_health_colors");
        }
        if(team_crosshair != current.teammates)
        {
            team_crosshair = current.teammates;
            model.DirtyVariable("team_crosshair");
        }
        if(hit_crosshair != current.hit_marker)
        {
            hit_crosshair = current.hit_marker;
            model.DirtyVariable("hit_crosshair");
        }
        if(crosshair_reload_fade != current.reload_fade)
        {
            crosshair_reload_fade = current.reload_fade;
            model.DirtyVariable("crosshair_reload_fade");
        }
        if(crosshair_reload_rotation != current.reload_rotation)
        {
            crosshair_reload_rotation = current.reload_rotation;
            model.DirtyVariable("crosshair_reload_rotation");
        }
        return true;
    }

    bool RmlSettings::refreshhud(bool update_edit_buffer)
    {
        settings::Hud current;
        if(!settings::readhud(current))
            return false;

        presenthud(current, update_edit_buffer);
        return true;
    }

    void RmlSettings::presenthud(
        const settings::Hud &current,
        bool update_edit_buffer)
    {
        if(show_fps != current.show_fps)
        {
            show_fps = current.show_fps;
            model.DirtyVariable("show_fps");
        }
        if(wall_clock != current.wall_clock)
        {
            wall_clock = current.wall_clock;
            model.DirtyVariable("wall_clock");
        }
        if(wall_clock_24_hour != current.wall_clock_24_hour)
        {
            wall_clock_24_hour = current.wall_clock_24_hour;
            model.DirtyVariable("wall_clock_24_hour");
        }
        if(wall_clock_seconds != current.wall_clock_seconds)
        {
            wall_clock_seconds = current.wall_clock_seconds;
            model.DirtyVariable("wall_clock_seconds");
        }
        if(damage_screen != current.damage_screen)
        {
            damage_screen = current.damage_screen;
            model.DirtyVariable("damage_screen");
        }
        if(damage_compass != current.damage_compass)
        {
            damage_compass = current.damage_compass;
            model.DirtyVariable("damage_compass");
        }
        if(hud_guns != current.hud_guns)
        {
            hud_guns = current.hud_guns;
            model.DirtyVariable("hud_guns");
        }
        if(hud_gun_sway != current.hud_gun_sway)
        {
            hud_gun_sway = current.hud_gun_sway;
            model.DirtyVariable("hud_gun_sway");
        }
        if(muzzle_flash != current.muzzle_flash)
        {
            muzzle_flash = current.muzzle_flash;
            model.DirtyVariable("muzzle_flash");
        }
        if(muzzle_light != current.muzzle_light)
        {
            muzzle_light = current.muzzle_light;
            model.DirtyVariable("muzzle_light");
        }

        const bool messages_enabled = current.frag_messages !=
            settings::FragMessagesMode::Off;
        const bool messages_in_insta = current.frag_messages ==
            settings::FragMessagesMode::AllModes;
        if(frag_messages_enabled != messages_enabled)
        {
            frag_messages_enabled = messages_enabled;
            model.DirtyVariable("frag_messages_enabled");
            if(messages_enabled)
                max_frag_messages_armed = false;
        }
        if(frag_messages_in_insta != messages_in_insta)
        {
            frag_messages_in_insta = messages_in_insta;
            model.DirtyVariable("frag_messages_in_insta");
        }

        if(max_frag_messages != current.max_frag_messages)
        {
            max_frag_messages = current.max_frag_messages;
            model.DirtyVariable("max_frag_messages");
        }
        if(max_frag_messages_minimum != current.max_frag_messages_minimum)
        {
            max_frag_messages_minimum = current.max_frag_messages_minimum;
            model.DirtyVariable("max_frag_messages_minimum");
        }
        if(max_frag_messages_maximum != current.max_frag_messages_maximum)
        {
            max_frag_messages_maximum = current.max_frag_messages_maximum;
            model.DirtyVariable("max_frag_messages_maximum");
        }

        if(frag_message_duration != current.frag_message_duration)
        {
            frag_message_duration = current.frag_message_duration;
            model.DirtyVariable("frag_message_duration");
        }
        if(frag_message_duration_minimum !=
           current.frag_message_duration_minimum)
        {
            frag_message_duration_minimum =
                current.frag_message_duration_minimum;
            model.DirtyVariable("frag_message_duration_minimum");
        }
        if(frag_message_duration_maximum !=
           current.frag_message_duration_maximum)
        {
            frag_message_duration_maximum =
                current.frag_message_duration_maximum;
            model.DirtyVariable("frag_message_duration_maximum");
        }
        const Rml::String duration_text = formatsensitivity(
            current.frag_message_duration
        );
        if(!frag_message_duration_editing &&
           frag_message_duration_input != duration_text)
        {
            frag_message_duration_input = duration_text;
            model.DirtyVariable("frag_message_duration_input");
        }

        if(frag_message_self_frags != current.frag_message_self_frags)
        {
            frag_message_self_frags = current.frag_message_self_frags;
            model.DirtyVariable("frag_message_self_frags");
        }
        if(frag_message_other_frags != current.frag_message_other_frags)
        {
            frag_message_other_frags = current.frag_message_other_frags;
            model.DirtyVariable("frag_message_other_frags");
        }
        if(frag_message_team_kills != current.frag_message_team_kills)
        {
            frag_message_team_kills = current.frag_message_team_kills;
            model.DirtyVariable("frag_message_team_kills");
        }

        if(frag_message_scale != current.frag_message_scale)
        {
            frag_message_scale = current.frag_message_scale;
            model.DirtyVariable("frag_message_scale");
        }
        if(frag_message_scale_minimum != current.frag_message_scale_minimum)
        {
            frag_message_scale_minimum = current.frag_message_scale_minimum;
            model.DirtyVariable("frag_message_scale_minimum");
        }
        if(frag_message_scale_maximum != current.frag_message_scale_maximum)
        {
            frag_message_scale_maximum = current.frag_message_scale_maximum;
            model.DirtyVariable("frag_message_scale_maximum");
        }
        const Rml::String message_scale_text = formatsensitivity(
            current.frag_message_scale
        );
        if(!frag_message_scale_editing &&
           frag_message_scale_input != message_scale_text)
        {
            frag_message_scale_input = message_scale_text;
            model.DirtyVariable("frag_message_scale_input");
        }

        if(frag_message_fade != current.frag_message_fade)
        {
            frag_message_fade = current.frag_message_fade;
            model.DirtyVariable("frag_message_fade");
        }

        if(frag_message_x != current.frag_message_x)
        {
            frag_message_x = current.frag_message_x;
            model.DirtyVariable("frag_message_x");
        }
        if(frag_message_x_minimum != current.frag_message_x_minimum)
        {
            frag_message_x_minimum = current.frag_message_x_minimum;
            model.DirtyVariable("frag_message_x_minimum");
        }
        if(frag_message_x_maximum != current.frag_message_x_maximum)
        {
            frag_message_x_maximum = current.frag_message_x_maximum;
            model.DirtyVariable("frag_message_x_maximum");
        }
        const Rml::String message_x_text = formatsensitivity(
            current.frag_message_x
        );
        if(!frag_message_x_editing && frag_message_x_input != message_x_text)
        {
            frag_message_x_input = message_x_text;
            model.DirtyVariable("frag_message_x_input");
        }

        if(frag_message_y != current.frag_message_y)
        {
            frag_message_y = current.frag_message_y;
            model.DirtyVariable("frag_message_y");
        }
        if(frag_message_y_minimum != current.frag_message_y_minimum)
        {
            frag_message_y_minimum = current.frag_message_y_minimum;
            model.DirtyVariable("frag_message_y_minimum");
        }
        if(frag_message_y_maximum != current.frag_message_y_maximum)
        {
            frag_message_y_maximum = current.frag_message_y_maximum;
            model.DirtyVariable("frag_message_y_maximum");
        }
        const Rml::String message_y_text = formatsensitivity(
            current.frag_message_y
        );
        if(!frag_message_y_editing && frag_message_y_input != message_y_text)
        {
            frag_message_y_input = message_y_text;
            model.DirtyVariable("frag_message_y_input");
        }

        const bool stack_up = current.frag_message_stack_dir == -1;
        const bool stack_auto = current.frag_message_stack_dir == 0;
        const bool stack_down = current.frag_message_stack_dir == 1;
        if(frag_message_stack_up != stack_up)
        {
            frag_message_stack_up = stack_up;
            model.DirtyVariable("frag_message_stack_up");
        }
        if(frag_message_stack_auto != stack_auto)
        {
            frag_message_stack_auto = stack_auto;
            model.DirtyVariable("frag_message_stack_auto");
        }
        if(frag_message_stack_down != stack_down)
        {
            frag_message_stack_down = stack_down;
            model.DirtyVariable("frag_message_stack_down");
        }

        if(streak_milestone_scale != current.streak_milestone_scale)
        {
            streak_milestone_scale = current.streak_milestone_scale;
            model.DirtyVariable("streak_milestone_scale");
        }
        if(streak_milestone_scale_minimum !=
           current.streak_milestone_scale_minimum)
        {
            streak_milestone_scale_minimum =
                current.streak_milestone_scale_minimum;
            model.DirtyVariable("streak_milestone_scale_minimum");
        }
        if(streak_milestone_scale_maximum !=
           current.streak_milestone_scale_maximum)
        {
            streak_milestone_scale_maximum =
                current.streak_milestone_scale_maximum;
            model.DirtyVariable("streak_milestone_scale_maximum");
        }

        const Rml::String scale_text = formatsensitivity(
            current.streak_milestone_scale
        );
        if(update_edit_buffer &&
           streak_milestone_scale_input != scale_text)
        {
            streak_milestone_scale_input = scale_text;
            model.DirtyVariable("streak_milestone_scale_input");
        }

        const char *status = !messages_enabled
            ? "Frag and streak messages disabled"
            : (!messages_in_insta
                ? "Messages hidden in instagib modes"
                : "Frag and streak messages active in all modes");
        if(hud_status != status)
        {
            hud_status = status;
            model.DirtyVariable("hud_status");
        }
    }

    void RmlSettings::finishhudwrite(bool accepted, settings::Hud &state)
    {
        const bool available = accepted || settings::readhud(state);
        if(available)
        {
            presenthud(
                state,
                !streak_milestone_scale_editing
            );
        }
        if(!accepted && available)
        {
            hud_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("hud_status");
        }
    }

    bool RmlSettings::refreshgameclock()
    {
        settings::GameClock state = {};
        const bool available = settings::readgameclock(state);
        presentgameclock(state, available);
        return available;
    }

    void RmlSettings::presentgameclock(
        const settings::GameClock &state,
        bool available)
    {
        if(game_clock_available != available)
        {
            game_clock_available = available;
            model.DirtyVariable("game_clock_available");
        }
        if(game_clock != state.enabled)
        {
            game_clock = state.enabled;
            model.DirtyVariable("game_clock");
        }
        if(game_clock_scale != state.scale)
        {
            game_clock_scale = state.scale;
            model.DirtyVariable("game_clock_scale");
        }
        if(game_clock_scale_minimum != state.scale_minimum)
        {
            game_clock_scale_minimum = state.scale_minimum;
            model.DirtyVariable("game_clock_scale_minimum");
        }
        if(game_clock_scale_maximum != state.scale_maximum)
        {
            game_clock_scale_maximum = state.scale_maximum;
            model.DirtyVariable("game_clock_scale_maximum");
        }
        const Rml::String scale_text = formatsensitivity(state.scale);
        if(!game_clock_scale_editing &&
           game_clock_scale_input != scale_text)
        {
            game_clock_scale_input = scale_text;
            model.DirtyVariable("game_clock_scale_input");
        }
        if(game_clock_alpha != state.alpha)
        {
            game_clock_alpha = state.alpha;
            model.DirtyVariable("game_clock_alpha");
        }
        if(game_clock_alpha_minimum != state.alpha_minimum)
        {
            game_clock_alpha_minimum = state.alpha_minimum;
            model.DirtyVariable("game_clock_alpha_minimum");
        }
        if(game_clock_alpha_maximum != state.alpha_maximum)
        {
            game_clock_alpha_maximum = state.alpha_maximum;
            model.DirtyVariable("game_clock_alpha_maximum");
        }
        if(game_clock_align != state.align)
        {
            game_clock_align = state.align;
            model.DirtyVariable("game_clock_align");
        }
        if(game_clock_x != state.x)
        {
            game_clock_x = state.x;
            model.DirtyVariable("game_clock_x");
        }
        if(game_clock_x_minimum != state.x_minimum)
        {
            game_clock_x_minimum = state.x_minimum;
            model.DirtyVariable("game_clock_x_minimum");
        }
        if(game_clock_x_maximum != state.x_maximum)
        {
            game_clock_x_maximum = state.x_maximum;
            model.DirtyVariable("game_clock_x_maximum");
        }
        const Rml::String x_text = formatsensitivity(state.x);
        if(!game_clock_x_editing && game_clock_x_input != x_text)
        {
            game_clock_x_input = x_text;
            model.DirtyVariable("game_clock_x_input");
        }
        if(game_clock_y != state.y)
        {
            game_clock_y = state.y;
            model.DirtyVariable("game_clock_y");
        }
        if(game_clock_y_minimum != state.y_minimum)
        {
            game_clock_y_minimum = state.y_minimum;
            model.DirtyVariable("game_clock_y_minimum");
        }
        if(game_clock_y_maximum != state.y_maximum)
        {
            game_clock_y_maximum = state.y_maximum;
            model.DirtyVariable("game_clock_y_maximum");
        }
        const Rml::String y_text = formatsensitivity(state.y);
        if(!game_clock_y_editing && game_clock_y_input != y_text)
        {
            game_clock_y_input = y_text;
            model.DirtyVariable("game_clock_y_input");
        }

        const char *status = available
            ? ""
            : "Gameclock settings are unavailable";
        if(game_clock_status != status)
        {
            game_clock_status = status;
            model.DirtyVariable("game_clock_status");
        }
    }

    void RmlSettings::selectgameclock(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_clock_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameClock state = {};
        const bool accepted = settings::writegameclock(enabled, state);
        const bool available = accepted ||
            settings::readgameclock(state);
        presentgameclock(state, available);
        if(!accepted && available)
        {
            game_clock_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_clock_status");
        }
    }

    void RmlSettings::selectgameclockalign(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_clock_available || !game_clock)
            return;

        const int value = arguments[0].Get<int>();
        if(value != -1 && value != 0 && value != 1)
            return;

        settings::GameClock state = {};
        const bool accepted = settings::writegameclockalign(value, state);
        const bool available = accepted ||
            settings::readgameclock(state);
        presentgameclock(state, available);
        if(!accepted && available)
        {
            game_clock_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_clock_status");
        }
    }

    bool RmlSettings::refreshhudscore()
    {
        settings::HudScore state = {};
        const bool available = settings::readhudscore(state);
        presenthudscore(
            state,
            available,
            !hudscore_scale_editing,
            !hudscore_x_editing,
            !hudscore_y_editing);
        return available;
    }

    void RmlSettings::presenthudscore(
        const settings::HudScore &state,
        bool available,
        bool update_scale_input,
        bool update_x_input,
        bool update_y_input)
    {
        if(hudscore_available != available)
        {
            hudscore_available = available;
            model.DirtyVariable("hudscore_available");
        }
        if(hudscore != state.enabled)
        {
            hudscore = state.enabled;
            model.DirtyVariable("hudscore");
        }
        if(hudscore_scale != state.scale)
        {
            hudscore_scale = state.scale;
            model.DirtyVariable("hudscore_scale");
        }
        if(hudscore_scale_minimum != state.scale_minimum)
        {
            hudscore_scale_minimum = state.scale_minimum;
            model.DirtyVariable("hudscore_scale_minimum");
        }
        if(hudscore_scale_maximum != state.scale_maximum)
        {
            hudscore_scale_maximum = state.scale_maximum;
            model.DirtyVariable("hudscore_scale_maximum");
        }
        if(hudscore_alpha != state.alpha)
        {
            hudscore_alpha = state.alpha;
            model.DirtyVariable("hudscore_alpha");
        }
        if(hudscore_alpha_minimum != state.alpha_minimum)
        {
            hudscore_alpha_minimum = state.alpha_minimum;
            model.DirtyVariable("hudscore_alpha_minimum");
        }
        if(hudscore_alpha_maximum != state.alpha_maximum)
        {
            hudscore_alpha_maximum = state.alpha_maximum;
            model.DirtyVariable("hudscore_alpha_maximum");
        }
        if(hudscore_sep != state.spacing)
        {
            hudscore_sep = state.spacing;
            model.DirtyVariable("hudscore_sep");
        }
        if(hudscore_sep_minimum != state.spacing_minimum)
        {
            hudscore_sep_minimum = state.spacing_minimum;
            model.DirtyVariable("hudscore_sep_minimum");
        }
        if(hudscore_sep_maximum != state.spacing_maximum)
        {
            hudscore_sep_maximum = state.spacing_maximum;
            model.DirtyVariable("hudscore_sep_maximum");
        }
        if(hudscore_align != state.align)
        {
            hudscore_align = state.align;
            model.DirtyVariable("hudscore_align");
        }
        if(hudscore_x != state.x)
        {
            hudscore_x = state.x;
            model.DirtyVariable("hudscore_x");
        }
        if(hudscore_x_minimum != state.x_minimum)
        {
            hudscore_x_minimum = state.x_minimum;
            model.DirtyVariable("hudscore_x_minimum");
        }
        if(hudscore_x_maximum != state.x_maximum)
        {
            hudscore_x_maximum = state.x_maximum;
            model.DirtyVariable("hudscore_x_maximum");
        }
        if(hudscore_y != state.y)
        {
            hudscore_y = state.y;
            model.DirtyVariable("hudscore_y");
        }
        if(hudscore_y_minimum != state.y_minimum)
        {
            hudscore_y_minimum = state.y_minimum;
            model.DirtyVariable("hudscore_y_minimum");
        }
        if(hudscore_y_maximum != state.y_maximum)
        {
            hudscore_y_maximum = state.y_maximum;
            model.DirtyVariable("hudscore_y_maximum");
        }

        if(update_scale_input)
        {
            const Rml::String text = formatsensitivity(state.scale);
            if(hudscore_scale_input != text)
            {
                hudscore_scale_input = text;
                model.DirtyVariable("hudscore_scale_input");
            }
        }
        if(update_x_input)
        {
            const Rml::String text = formatsensitivity(state.x);
            if(hudscore_x_input != text)
            {
                hudscore_x_input = text;
                model.DirtyVariable("hudscore_x_input");
            }
        }
        if(update_y_input)
        {
            const Rml::String text = formatsensitivity(state.y);
            if(hudscore_y_input != text)
            {
                hudscore_y_input = text;
                model.DirtyVariable("hudscore_y_input");
            }
        }

        const char *status = !available
            ? "Score overlay settings are unavailable"
            : (state.enabled
                ? "Score overlay drawn through the existing HUD path"
                : "Score overlay off");
        if(hudscore_status != status)
        {
            hudscore_status = status;
            model.DirtyVariable("hudscore_status");
        }
    }

    void RmlSettings::selecthudscore(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !hudscore_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::HudScore state = {};
        const bool accepted = settings::writehudscore(enabled, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        presenthudscore(
            state,
            available,
            !hudscore_scale_editing,
            !hudscore_x_editing,
            !hudscore_y_editing);
        if(!accepted && available)
        {
            hudscore_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("hudscore_status");
        }
    }

    void RmlSettings::selecthudscorealign(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !hudscore_available)
            return;

        const int value = arguments[0].Get<int>();
        if(value != -1 && value != 0 && value != 1)
            return;

        settings::HudScore state = {};
        const bool accepted = settings::writehudscorealign(value, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        presenthudscore(
            state,
            available,
            !hudscore_scale_editing,
            !hudscore_x_editing,
            !hudscore_y_editing);
        if(!accepted && available)
        {
            hudscore_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("hudscore_status");
        }
    }

    bool RmlSettings::refreshchat()
    {
        settings::Chat current;
        if(!settings::readchat(current))
            return false;

        if(public_chat_in_top_feed !=
           current.public_messages_in_top_feed)
        {
            public_chat_in_top_feed =
                current.public_messages_in_top_feed;
            model.DirtyVariable("public_chat_in_top_feed");
        }
        if(team_chat_in_top_feed != current.team_messages_in_top_feed)
        {
            team_chat_in_top_feed = current.team_messages_in_top_feed;
            model.DirtyVariable("team_chat_in_top_feed");
        }
        if(public_chat_in_bottom_box !=
           current.public_messages_in_bottom_box)
        {
            public_chat_in_bottom_box =
                current.public_messages_in_bottom_box;
            model.DirtyVariable("public_chat_in_bottom_box");
        }
        if(team_chat_in_bottom_box !=
           current.team_messages_in_bottom_box)
        {
            team_chat_in_bottom_box =
                current.team_messages_in_bottom_box;
            model.DirtyVariable("team_chat_in_bottom_box");
        }
        if(team_colored_chat_names != current.team_colored_names)
        {
            team_colored_chat_names = current.team_colored_names;
            model.DirtyVariable("team_colored_chat_names");
        }
        return true;
    }

    static Rml::String formatconsoleinteger(int value)
    {
        char text[16];
        std::snprintf(text, sizeof(text), "%d", value);
        return text;
    }

    bool RmlSettings::refreshconsole()
    {
        settings::Console state = {};
        const bool available = settings::readconsole(state);
        presentconsole(state, available, !console_size_editing);
        return available;
    }

    void RmlSettings::presentconsole(
        const settings::Console &state,
        bool available,
        bool update_size_input)
    {
        if(console_available != available)
        {
            console_available = available;
            model.DirtyVariable("console_available");
        }

        const auto setbool = [this](bool &field, const char *name,
            bool value)
        {
            if(field != value)
            {
                field = value;
                model.DirtyVariable(name);
            }
        };
        const auto setint = [this](int &field, const char *name,
            int value)
        {
            if(field != value)
            {
                field = value;
                model.DirtyVariable(name);
            }
        };

        setbool(console_top_important, "console_top_important",
            state.important_information_in_top_feed);
        setbool(console_top_warnings, "console_top_warnings",
            state.warnings_in_top_feed);
        setbool(console_top_errors, "console_top_errors",
            state.errors_in_top_feed);
        setbool(console_top_script, "console_top_script",
            state.script_messages_in_top_feed);
        setbool(console_top_game_events, "console_top_game_events",
            state.game_events_in_top_feed);
        setbool(console_top_self_frags, "console_top_self_frags",
            state.self_frags_in_top_feed);
        setbool(console_top_other_frags, "console_top_other_frags",
            state.other_player_frags_in_top_feed);
        setbool(console_top_team_kills, "console_top_team_kills",
            state.team_kills_in_top_feed);
        setbool(console_full_important, "console_full_important",
            state.important_information_in_full_console);
        setbool(console_full_warnings, "console_full_warnings",
            state.warnings_in_full_console);
        setbool(console_full_errors, "console_full_errors",
            state.errors_in_full_console);
        setbool(console_full_script, "console_full_script",
            state.script_messages_in_full_console);
        setbool(console_full_chat, "console_full_chat",
            state.public_messages_in_full_console);
        setbool(console_full_team_chat, "console_full_team_chat",
            state.team_messages_in_full_console);
        setbool(console_full_game_events, "console_full_game_events",
            state.game_events_in_full_console);
        setbool(console_full_self_frags, "console_full_self_frags",
            state.self_frags_in_full_console);
        setbool(console_full_other_frags, "console_full_other_frags",
            state.other_player_frags_in_full_console);
        setbool(console_full_team_kills, "console_full_team_kills",
            state.team_kills_in_full_console);
        setint(console_size, "console_size", state.size);
        setint(console_size_minimum, "console_size_minimum",
            state.size_minimum);
        setint(console_size_maximum, "console_size_maximum",
            state.size_maximum);

        if(update_size_input)
        {
            const Rml::String text = formatconsoleinteger(state.size);
            if(console_size_input != text)
            {
                console_size_input = text;
                model.DirtyVariable("console_size_input");
            }
        }

        const char *status = !available
            ? "Console settings are unavailable"
            : "Filters apply immediately to the existing console feeds";
        if(console_status != status)
        {
            console_status = status;
            model.DirtyVariable("console_status");
        }
    }

    void RmlSettings::applyconsolefilter(
        bool (*writer)(bool, settings::Console &),
        bool current)
    {
        if(!writer)
            return;

        settings::Console state = {};
        const bool accepted = writer(!current, state);
        const bool available = accepted || settings::readconsole(state);
        presentconsole(state, available, !console_size_editing);
        if(!accepted && available)
        {
            console_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("console_status");
        }
    }

    void RmlSettings::commitconsolesize()
    {
        int requested = 0;
        settings::Console state = {};
        const bool accepted = parseinteger(console_size_input, requested) &&
            console_available &&
            settings::writeconsolesize(requested, state);
        const bool available = accepted || settings::readconsole(state);
        console_size_invalid = !accepted;
        model.DirtyVariable("console_size_invalid");
        presentconsole(state, available, true);
        console_status = accepted
            ? "Top-left console size applied"
            : (available
                ? "Invalid size; restored the last effective setting"
                : "Console settings are unavailable");
        model.DirtyVariable("console_status");
    }

    bool RmlSettings::refreshkeybindings()
    {
        std::vector<KeyBindingRow> movement;
        std::vector<KeyBindingRow> combat;
        std::vector<KeyBindingRow> interface_rows;

        const auto append = [](keybindings::Action action,
                               const char *label,
                               std::vector<KeyBindingRow> &rows)
        {
            KeyBindingRow row;
            row.action_id = static_cast<int>(action);
            row.label = label;
            if(!keybindings::visit(action, appendkeyname, &row.keys))
                return false;
            row.bound = !row.keys.empty();
            rows.push_back(row);
            return true;
        };

        if(!append(
                keybindings::Action::MoveForward,
                "Move Forward",
                movement
            ) ||
           !append(
                keybindings::Action::MoveBackward,
                "Move Backward",
                movement
            ) ||
           !append(
                keybindings::Action::MoveLeft,
                "Move Left",
                movement
            ) ||
           !append(
                keybindings::Action::MoveRight,
                "Move Right",
                movement
            ) ||
           !append(
                keybindings::Action::Jump,
                "Jump",
                movement
            ) ||
           !append(
                keybindings::Action::Attack,
                "Attack",
                combat
            ) ||
           !append(
                keybindings::Action::Zoom,
                "Zoom",
                combat
            ) ||
           !append(
                keybindings::Action::WeaponAction,
                "Weapon Action",
                combat
            ) ||
           !append(
                keybindings::Action::Scoreboard,
                "Scoreboard",
                interface_rows
            ) ||
           !append(
                keybindings::Action::PublicChat,
                "Public Chat",
                interface_rows
            ) ||
           !append(
                keybindings::Action::TeamChat,
                "Team Chat",
                interface_rows
            ))
        {
            return false;
        }

        if(keybindings_movement != movement)
        {
            keybindings_movement.swap(movement);
            model.DirtyVariable("keybindings_movement");
        }
        if(keybindings_combat != combat)
        {
            keybindings_combat.swap(combat);
            model.DirtyVariable("keybindings_combat");
        }
        if(keybindings_interface != interface_rows)
        {
            keybindings_interface.swap(interface_rows);
            model.DirtyVariable("keybindings_interface");
        }
        return key_candidate.empty() || refreshkeyconflict();
    }

    const char *RmlSettings::keybindinglabel(keybindings::Action action)
    {
        switch(action)
        {
            case keybindings::Action::MoveForward: return "Move Forward";
            case keybindings::Action::MoveBackward: return "Move Backward";
            case keybindings::Action::MoveLeft: return "Move Left";
            case keybindings::Action::MoveRight: return "Move Right";
            case keybindings::Action::Jump: return "Jump";
            case keybindings::Action::Attack: return "Attack";
            case keybindings::Action::Zoom: return "Zoom";
            case keybindings::Action::WeaponAction: return "Weapon Action";
            case keybindings::Action::Scoreboard: return "Scoreboard";
            case keybindings::Action::PublicChat: return "Public Chat";
            case keybindings::Action::TeamChat: return "Team Chat";
        }
        return "This action";
    }

    void RmlSettings::clearkeyconflict()
    {
        key_conflict = keybindings::Conflict();
        key_conflict_status.clear();
        key_conflict_available = false;
        key_conflict_same_action = false;
        key_conflict_present = false;
        if(model)
        {
            model.DirtyVariable("key_conflict_status");
            model.DirtyVariable("key_conflict_available");
            model.DirtyVariable("key_conflict_same_action");
            model.DirtyVariable("key_conflict_present");
        }
    }

    bool RmlSettings::refreshkeyconflict()
    {
        if(key_candidate.empty())
        {
            clearkeyconflict();
            return true;
        }

        keybindings::Conflict conflict;
        if(!keybindings::inspectconflict(
               key_capture_action,
               key_candidate.c_str(),
               conflict))
        {
            clearkeyconflict();
            return false;
        }

        key_conflict = conflict;
        key_conflict_available = conflict.kind ==
            keybindings::ConflictKind::Unbound;
        key_conflict_same_action = conflict.kind ==
            keybindings::ConflictKind::SameAction;
        key_conflict_present = !key_conflict_available &&
            !key_conflict_same_action;

        switch(conflict.kind)
        {
            case keybindings::ConflictKind::Unbound:
                key_conflict_status = "Available";
                break;
            case keybindings::ConflictKind::SameAction:
                key_conflict_status = "Already bound to this action";
                break;
            case keybindings::ConflictKind::CuratedAction:
                key_conflict_status = "Currently bound to: ";
                key_conflict_status += keybindinglabel(conflict.action);
                break;
            case keybindings::ConflictKind::CustomAction:
                key_conflict_status = "Currently bound to a custom action";
                break;
        }

        model.DirtyVariable("key_conflict_status");
        model.DirtyVariable("key_conflict_available");
        model.DirtyVariable("key_conflict_same_action");
        model.DirtyVariable("key_conflict_present");
        return true;
    }

    void RmlSettings::startkeycapture(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1 || !category_mouse_keyboard)
            return;

        // Only IDs of typed rows supplied by C++ are accepted.
        const int id = arguments[0].Get<int>(-1);
        const KeyBindingRow *selected = nullptr;
        for(const auto *rows : { &keybindings_movement, &keybindings_combat,
                                &keybindings_interface })
            for(const KeyBindingRow &row : *rows)
                if(row.action_id == id) selected = &row;
        if(!selected)
            return;

        event.GetCurrentElement()->GetOwnerDocument()->Focus();
        clearkeyremoval();
        key_capture_action = static_cast<keybindings::Action>(id);
        key_capture_id = id;
        key_candidate.clear();
        key_candidate_pending = false;
        key_binding_status.clear();
        model.DirtyVariable("key_candidate_pending");
        model.DirtyVariable("key_binding_status");
        clearkeyconflict();
        key_capture_active = true;
        key_capture_status = "Press a key... (Esc to cancel)";
        model.DirtyVariable("key_capture_id");
        model.DirtyVariable("key_capture_status");
        event.StopPropagation();
    }

    void RmlSettings::cancelkeycapture()
    {
        // Settings close/suspension/category changes invalidate either decision.
        clearkeyremoval();
        key_capture_active = false;
        key_capture_action = keybindings::Action::MoveForward;
        key_capture_id = -1;
        key_candidate.clear();
        key_capture_status.clear();
        key_candidate_pending = false;
        key_binding_status.clear();
        clearkeyconflict();
        if(model)
        {
            model.DirtyVariable("key_capture_id");
            model.DirtyVariable("key_capture_status");
            model.DirtyVariable("key_candidate_pending");
            model.DirtyVariable("key_binding_status");
        }
    }

    void RmlSettings::acceptkeycandidate(const char *name)
    {
        if(!key_capture_active || !name || !name[0])
            return;
        key_candidate = name;
        key_candidate_pending = true;
        model.DirtyVariable("key_candidate_pending");
        key_capture_active = false;
        key_capture_status = "Captured: " + key_candidate;
        model.DirtyVariable("key_capture_status");
        refreshkeyconflict();
    }

    void RmlSettings::decidekeycandidate(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 2 || key_capture_active || !key_candidate_pending ||
           arguments[0].Get<int>(-1) != key_capture_id)
            return;

        // Fixed UI decisions: 0 cancels/dismisses, 1 applies, 2 replaces.
        const int decision = arguments[1].Get<int>(-1);
        if(decision == 0)
        {
            cancelkeycapture();
            return;
        }
        if((decision != 1 || !key_conflict_available) &&
           (decision != 2 || !key_conflict_present))
            return;

        keybindings::Conflict fresh;
        if(!keybindings::inspectconflict(key_capture_action, key_candidate.c_str(), fresh))
        {
            refreshkeybindings();
            key_binding_status = "Could not inspect this key; no binding changed.";
            model.DirtyVariable("key_binding_status");
            return;
        }

        const bool already_bound = decision == 1 &&
            fresh.kind == keybindings::ConflictKind::SameAction;
        if(!already_bound && !key_conflict.matches(fresh))
        {
            refreshkeybindings();
            key_binding_status = "Key ownership changed. Review it and confirm again.";
            model.DirtyVariable("key_binding_status");
            return;
        }

        // Inspection and mutation are synchronous; no event dispatch in between.
        const bool written = already_bound ||
            keybindings::bind(key_capture_action, key_candidate.c_str());
        const bool refreshed = refreshkeybindings();
        keybindings::Conflict effective;
        const bool verified = keybindings::inspectconflict(
            key_capture_action, key_candidate.c_str(), effective) &&
            effective.kind == keybindings::ConflictKind::SameAction;
        if(written && refreshed && verified)
        {
            const Rml::String success = key_candidate + " bound to " +
                keybindinglabel(key_capture_action);
            cancelkeycapture();
            key_binding_status = success;
        }
        else
            key_binding_status = "Binding could not be confirmed. Review the current bindings.";
        model.DirtyVariable("key_binding_status");
    }

    void RmlSettings::clearkeyremoval()
    {
        key_removal_active = false;
        key_removal_action = keybindings::Action::MoveForward;
        key_removal_key.clear();
        key_removal_prompt.clear();
        if(model)
        {
            model.DirtyVariable("key_removal_active");
            model.DirtyVariable("key_removal_action_id");
            model.DirtyVariable("key_removal_prompt");
        }
    }

    void RmlSettings::requestkeyremoval(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 2 || !category_mouse_keyboard)
            return;
        const int id = arguments[0].Get<int>(-1);
        const Rml::String key = arguments[1].Get<Rml::String>();
        // Accept only a typed row and exact canonical key supplied by C++.
        bool valid = false;
        for(const auto *rows : { &keybindings_movement, &keybindings_combat,
                                &keybindings_interface })
            for(const KeyBindingRow &row : *rows)
                if(row.action_id == id &&
                   std::find(row.keys.begin(), row.keys.end(), key) != row.keys.end())
                    valid = true;
        if(!valid)
            return;

        cancelkeycapture();
        key_removal_action = static_cast<keybindings::Action>(id);
        key_removal_key = key;
        key_removal_active = true;
        key_removal_prompt = "Remove " + key + " from " +
            keybindinglabel(key_removal_action) + "?";
        model.DirtyVariable("key_removal_active");
        model.DirtyVariable("key_removal_action_id");
        model.DirtyVariable("key_removal_prompt");
    }

    void RmlSettings::decidekeyremoval(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !key_removal_active ||
           !category_mouse_keyboard)
            return;
        const int decision = arguments[0].Get<int>(-1);
        if(decision == 0)
        {
            clearkeyremoval();
            return;
        }
        if(decision != 1)
            return;

        const keybindings::Action action = key_removal_action;
        const Rml::String key = key_removal_key;
        keybindings::Conflict fresh;
        const bool inspected = keybindings::inspectconflict(action, key.c_str(), fresh);
        if(!inspected || fresh.kind != keybindings::ConflictKind::SameAction)
        {
            clearkeyremoval();
            refreshkeybindings();
            key_binding_status = inspected
                ? "Binding changed; nothing was removed."
                : "Could not inspect this key; nothing was removed.";
        }
        else
        {
            // No event dispatch between inspection and this exact-key mutation.
            const bool removed = keybindings::unbind(action, key.c_str());
            clearkeyremoval();
            const bool refreshed = refreshkeybindings();
            keybindings::Conflict effective;
            const bool verified = keybindings::inspectconflict(action, key.c_str(), effective) &&
                effective.kind != keybindings::ConflictKind::SameAction;
            key_binding_status = removed && refreshed && verified
                ? key + " removed from " + keybindinglabel(action)
                : "Removal could not be confirmed. Review the current bindings.";
        }
        model.DirtyVariable("key_binding_status");
    }

    void RmlSettings::commitsensitivity()
    {
        float requested = 0.0f;
        const bool accepted = parsefloat(
                mouse_sensitivity_input,
                requested
            ) &&
            settings::writesensitivity(requested);

        mouse_sensitivity_invalid = !accepted;
        model.DirtyVariable("mouse_sensitivity_invalid");
        mouse_sensitivity_status = accepted
            ? "Exact value applied"
            : "Invalid value; restored the last effective setting";
        model.DirtyVariable("mouse_sensitivity_status");
        refreshmouseinput(true);
    }

    void RmlSettings::commitcrosshairsize()
    {
        int requested = 0;
        const bool accepted = parseinteger(
                crosshair_size_input,
                requested
            ) &&
            settings::writecrosshairsize(requested);

        crosshair_size_invalid = !accepted;
        model.DirtyVariable("crosshair_size_invalid");
        crosshair_status = accepted
            ? "Crosshair size applied"
            : "Invalid size; restored the last effective setting";
        model.DirtyVariable("crosshair_status");
        refreshcrosshair(true);
    }

    void RmlSettings::commitstreakscale()
    {
        float requested = 0.0f;
        const bool accepted = parsefloat(
                streak_milestone_scale_input,
                requested
            ) &&
            settings::writestreakmilestonescale(requested);

        streak_milestone_scale_invalid = !accepted;
        model.DirtyVariable("streak_milestone_scale_invalid");
        refreshhud(true);
        hud_status = accepted
            ? "Streak milestone scale applied"
            : "Invalid scale; restored the last effective setting";
        model.DirtyVariable("hud_status");
    }

    void RmlSettings::commitfragmessageduration()
    {
        float requested = 0.0f;
        settings::Hud state = {};
        const bool accepted = parsefloat(
                frag_message_duration_input,
                requested
            ) &&
            frag_messages_enabled &&
            settings::writefragmessageduration(requested, state);
        frag_message_duration_invalid = !accepted;
        model.DirtyVariable("frag_message_duration_invalid");
        const bool was_editing = frag_message_duration_editing;
        frag_message_duration_editing = false;
        finishhudwrite(accepted, state);
        frag_message_duration_editing = was_editing;
        if(accepted)
        {
            hud_status = "Duration applied";
            model.DirtyVariable("hud_status");
        }
        else if(frag_messages_enabled)
        {
            hud_status =
                "Invalid duration; restored the last effective setting";
            model.DirtyVariable("hud_status");
        }
    }

    void RmlSettings::commitfragmessagescale()
    {
        float requested = 0.0f;
        settings::Hud state = {};
        const bool accepted = parsefloat(
                frag_message_scale_input,
                requested
            ) &&
            frag_messages_enabled &&
            settings::writefragmessagescale(requested, state);
        frag_message_scale_invalid = !accepted;
        model.DirtyVariable("frag_message_scale_invalid");
        const bool was_editing = frag_message_scale_editing;
        frag_message_scale_editing = false;
        finishhudwrite(accepted, state);
        frag_message_scale_editing = was_editing;
        if(accepted)
        {
            hud_status = "Size applied";
            model.DirtyVariable("hud_status");
        }
        else if(frag_messages_enabled)
        {
            hud_status =
                "Invalid size; restored the last effective setting";
            model.DirtyVariable("hud_status");
        }
    }

    void RmlSettings::commitfragmessagex()
    {
        float requested = 0.0f;
        settings::Hud state = {};
        const bool accepted = parsefloat(
                frag_message_x_input,
                requested
            ) &&
            frag_messages_enabled &&
            settings::writefragmessagex(requested, state);
        frag_message_x_invalid = !accepted;
        model.DirtyVariable("frag_message_x_invalid");
        const bool was_editing = frag_message_x_editing;
        frag_message_x_editing = false;
        finishhudwrite(accepted, state);
        frag_message_x_editing = was_editing;
        if(accepted)
        {
            hud_status = "X position applied";
            model.DirtyVariable("hud_status");
        }
        else if(frag_messages_enabled)
        {
            hud_status =
                "Invalid X; restored the last effective setting";
            model.DirtyVariable("hud_status");
        }
    }

    void RmlSettings::commitfragmessagey()
    {
        float requested = 0.0f;
        settings::Hud state = {};
        const bool accepted = parsefloat(
                frag_message_y_input,
                requested
            ) &&
            frag_messages_enabled &&
            settings::writefragmessagey(requested, state);
        frag_message_y_invalid = !accepted;
        model.DirtyVariable("frag_message_y_invalid");
        const bool was_editing = frag_message_y_editing;
        frag_message_y_editing = false;
        finishhudwrite(accepted, state);
        frag_message_y_editing = was_editing;
        if(accepted)
        {
            hud_status = "Y position applied";
            model.DirtyVariable("hud_status");
        }
        else if(frag_messages_enabled)
        {
            hud_status =
                "Invalid Y; restored the last effective setting";
            model.DirtyVariable("hud_status");
        }
    }

    void RmlSettings::commitammobarscale()
    {
        float requested = 0.0f;
        settings::Ammobar state = {};
        const bool accepted = parsefloat(ammobar_scale_input, requested) &&
            ammobar_available &&
            settings::writeammobarscale(requested, state);
        ammobar_scale_invalid = !accepted;
        model.DirtyVariable("ammobar_scale_invalid");
        const bool was_editing = ammobar_scale_editing;
        ammobar_scale_editing = false;
        finishammobarwrite(accepted, state);
        ammobar_scale_editing = was_editing;
        if(accepted)
        {
            ammobar_status = "Size applied";
            model.DirtyVariable("ammobar_status");
        }
        else if(ammobar_available)
        {
            ammobar_status =
                "Invalid size; restored the last effective setting";
            model.DirtyVariable("ammobar_status");
        }
    }

    void RmlSettings::commitammobarlabelsize()
    {
        float requested = 0.0f;
        settings::Ammobar state = {};
        const bool accepted =
            parsefloat(ammobar_label_size_input, requested) &&
            ammobar_available &&
            settings::writeammobarcountscale(requested, state);
        ammobar_label_size_invalid = !accepted;
        model.DirtyVariable("ammobar_label_size_invalid");
        const bool was_editing = ammobar_label_size_editing;
        ammobar_label_size_editing = false;
        finishammobarwrite(accepted, state);
        ammobar_label_size_editing = was_editing;
        if(accepted)
        {
            ammobar_status = "Label size applied";
            model.DirtyVariable("ammobar_status");
        }
        else if(ammobar_available)
        {
            ammobar_status =
                "Invalid label size; restored the last effective setting";
            model.DirtyVariable("ammobar_status");
        }
    }

    void RmlSettings::commitammobarx()
    {
        float requested = 0.0f;
        settings::Ammobar state = {};
        const bool accepted = parsefloat(ammobar_x_input, requested) &&
            ammobar_available &&
            settings::writeammobarx(requested, state);
        ammobar_x_invalid = !accepted;
        model.DirtyVariable("ammobar_x_invalid");
        const bool was_editing = ammobar_x_editing;
        ammobar_x_editing = false;
        finishammobarwrite(accepted, state);
        ammobar_x_editing = was_editing;
        if(accepted)
        {
            ammobar_status = "X position applied";
            model.DirtyVariable("ammobar_status");
        }
        else if(ammobar_available)
        {
            ammobar_status =
                "Invalid x; restored the last effective setting";
            model.DirtyVariable("ammobar_status");
        }
    }

    void RmlSettings::commitammobary()
    {
        float requested = 0.0f;
        settings::Ammobar state = {};
        const bool accepted = parsefloat(ammobar_y_input, requested) &&
            ammobar_available &&
            settings::writeammobary(requested, state);
        ammobar_y_invalid = !accepted;
        model.DirtyVariable("ammobar_y_invalid");
        const bool was_editing = ammobar_y_editing;
        ammobar_y_editing = false;
        finishammobarwrite(accepted, state);
        ammobar_y_editing = was_editing;
        if(accepted)
        {
            ammobar_status = "Y position applied";
            model.DirtyVariable("ammobar_status");
        }
        else if(ammobar_available)
        {
            ammobar_status =
                "Invalid y; restored the last effective setting";
            model.DirtyVariable("ammobar_status");
        }
    }

    void RmlSettings::commitgameclockscale()
    {
        float requested = 0.0f;
        settings::GameClock state = {};
        const bool accepted = parsefloat(
                game_clock_scale_input,
                requested
            ) &&
            settings::writegameclockscale(requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);

        game_clock_scale_invalid = !accepted;
        model.DirtyVariable("game_clock_scale_invalid");
        const bool editing = game_clock_scale_editing;
        game_clock_scale_editing = false;
        presentgameclock(state, available);
        game_clock_scale_editing = editing;
        game_clock_status = accepted
            ? "Size applied"
            : "Invalid size; restored the last effective setting";
        model.DirtyVariable("game_clock_status");
    }

    void RmlSettings::commitgameclockx()
    {
        float requested = 0.0f;
        settings::GameClock state = {};
        const bool accepted = parsefloat(
                game_clock_x_input,
                requested
            ) &&
            settings::writegameclockx(requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);

        game_clock_x_invalid = !accepted;
        model.DirtyVariable("game_clock_x_invalid");
        const bool editing = game_clock_x_editing;
        game_clock_x_editing = false;
        presentgameclock(state, available);
        game_clock_x_editing = editing;
        game_clock_status = accepted
            ? "X applied"
            : "Invalid x; restored the last effective setting";
        model.DirtyVariable("game_clock_status");
    }

    void RmlSettings::commitgameclocky()
    {
        float requested = 0.0f;
        settings::GameClock state = {};
        const bool accepted = parsefloat(
                game_clock_y_input,
                requested
            ) &&
            settings::writegameclocky(requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);

        game_clock_y_invalid = !accepted;
        model.DirtyVariable("game_clock_y_invalid");
        const bool editing = game_clock_y_editing;
        game_clock_y_editing = false;
        presentgameclock(state, available);
        game_clock_y_editing = editing;
        game_clock_status = accepted
            ? "Y applied"
            : "Invalid y; restored the last effective setting";
        model.DirtyVariable("game_clock_status");
    }

    void RmlSettings::commithudscorescale()
    {
        float requested = 0.0f;
        settings::HudScore state = {};
        const bool accepted = parsefloat(
                hudscore_scale_input,
                requested
            ) &&
            settings::writehudscorescale(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        hudscore_scale_invalid = !accepted;
        model.DirtyVariable("hudscore_scale_invalid");
        presenthudscore(
            state,
            available,
            true,
            !hudscore_x_editing,
            !hudscore_y_editing);
        hudscore_status = accepted
            ? "Score overlay size applied"
            : (available
                ? "Invalid size; restored the last effective setting"
                : "Score overlay settings are unavailable");
        model.DirtyVariable("hudscore_status");
    }

    void RmlSettings::commithudscorex()
    {
        float requested = 0.0f;
        settings::HudScore state = {};
        const bool accepted = parsefloat(
                hudscore_x_input,
                requested
            ) &&
            settings::writehudscorex(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        hudscore_x_invalid = !accepted;
        model.DirtyVariable("hudscore_x_invalid");
        presenthudscore(
            state,
            available,
            !hudscore_scale_editing,
            true,
            !hudscore_y_editing);
        hudscore_status = accepted
            ? "Score overlay x applied"
            : (available
                ? "Invalid x; restored the last effective setting"
                : "Score overlay settings are unavailable");
        model.DirtyVariable("hudscore_status");
    }

    void RmlSettings::commithudscorey()
    {
        float requested = 0.0f;
        settings::HudScore state = {};
        const bool accepted = parsefloat(
                hudscore_y_input,
                requested
            ) &&
            settings::writehudscorey(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        hudscore_y_invalid = !accepted;
        model.DirtyVariable("hudscore_y_invalid");
        presenthudscore(
            state,
            available,
            !hudscore_scale_editing,
            !hudscore_x_editing,
            true);
        hudscore_status = accepted
            ? "Score overlay y applied"
            : (available
                ? "Invalid y; restored the last effective setting"
                : "Score overlay settings are unavailable");
        model.DirtyVariable("hudscore_status");
    }

    void RmlSettings::setcategory(Category category)
    {
        if(category != Category::MouseKeyboard)
            cancelkeycapture();
        const bool game = category == Category::Game;
        const bool hud = category == Category::Hud;
        const bool graphics = category == Category::Graphics;
        const bool audio = category == Category::Audio;
        const bool mouse_keyboard = category ==
            Category::MouseKeyboard;
        const bool crosshair = category == Category::Crosshair;
        const bool chat = category == Category::Chat;
        const bool console = category == Category::Console;
        if(category_game != game)
        {
            category_game = game;
            model.DirtyVariable("category_game");
        }
        if(category_hud != hud)
        {
            category_hud = hud;
            model.DirtyVariable("category_hud");
        }
        if(category_graphics != graphics)
        {
            category_graphics = graphics;
            model.DirtyVariable("category_graphics");
        }
        if(category_audio != audio)
        {
            category_audio = audio;
            model.DirtyVariable("category_audio");
        }
        if(category_mouse_keyboard != mouse_keyboard)
        {
            category_mouse_keyboard = mouse_keyboard;
            model.DirtyVariable("category_mouse_keyboard");
        }
        if(category_crosshair != crosshair)
        {
            category_crosshair = crosshair;
            model.DirtyVariable("category_crosshair");
        }
        if(category_chat != chat)
        {
            category_chat = chat;
            model.DirtyVariable("category_chat");
        }
        if(category_console != console)
        {
            category_console = console;
            model.DirtyVariable("category_console");
        }

        if(audio)
            refreshaudio();
        else if(hud)
        {
            refreshhud(!streak_milestone_scale_editing);
            refreshammobar();
            refreshgameclock();
            refreshhudscore();
            refreshscoreboardcolumns();
        }
        else if(game)
        {
            refreshgamefeedback();
            refreshtrailcolors();
            refreshgameradar();
            refreshgamemodels();
        }
        else if(crosshair)
            refreshcrosshair(!crosshair_size_editing);
        else if(chat)
            refreshchat();
        else if(console)
            refreshconsole();
    }

    void RmlSettings::setpresentation(
        settings::PresentationMode presentation,
        Rml::Event &event)
    {
        if(settings::writepresentation(presentation))
        {
            switch(presentation)
            {
                case settings::PresentationMode::Windowed:
                    video_status = "Windowed mode active";
                    break;
                case settings::PresentationMode::ExclusiveFullscreen:
                    video_status = "Exclusive fullscreen active";
                    break;
                case settings::PresentationMode::DesktopFullscreen:
                    video_status = "Desktop fullscreen active";
                    break;
            }
        }
        else video_status = "Display mode change was not accepted";

        model.DirtyVariable("video_status");
        refreshvideo();
        event.StopPropagation();
    }

    void RmlSettings::selectresolution(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 2 || !resolution_enabled)
            return;

        const int width = arguments[0].Get<int>();
        const int height = arguments[1].Get<int>();
        bool supported = false;
        for(std::vector<Resolution>::const_iterator resolution =
                resolutions.begin();
            resolution != resolutions.end();
            ++resolution)
        {
            if(resolution->width == width &&
               resolution->height == height)
            {
                supported = resolution->supported;
                break;
            }
        }

        if(supported && settings::writeresolution(width, height))
            video_status = "Display resolution applied";
        else
            video_status = "Resolution change was not accepted";

        model.DirtyVariable("video_status");
        refreshvideo();
        event.StopPropagation();
    }

    void RmlSettings::selectantialiasing(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        const int samples = arguments[0].Get<int>();
        const bool supported_value = samples == 0 ||
            samples == 2 ||
            samples == 4 ||
            samples == 8 ||
            samples == 16;
        const bool request_changed = samples !=
            antialiasing_requested;

        if(!supported_value || !settings::writeantialiasing(samples))
        {
            antialiasing_status = "Antialiasing request was rejected";
            model.DirtyVariable("antialiasing_status");
            event.StopPropagation();
            return;
        }

        antialiasing_requested = samples;
        if(!request_changed && samples == antialiasing)
        {
            antialiasing_status = samples
                ? "Requested value already active"
                : "Antialiasing disabled";
            model.DirtyVariable("antialiasing_status");
        }
        else
        {
            pending_antialiasing = samples;
            antialiasing_status = "Rebuilding the graphics context";
            model.DirtyVariable("antialiasing_status");
        }

        event.StopPropagation();
    }

    void RmlSettings::selectdepthbuffer(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        const int bits = arguments[0].Get<int>();
        const bool request_changed = bits != depth_buffer_requested;

        if(!validdepthbufferchoice(bits) || !settings::writedepthbits(bits))
        {
            depth_buffer_status = "Depth buffer request was rejected";
            model.DirtyVariable("depth_buffer_status");
            event.StopPropagation();
            return;
        }

        depth_buffer_requested = bits;
        const bool depth_default = bits == 0;
        const bool depth_16 = bits == 16;
        const bool depth_24 = bits == 24;
        const bool depth_32 = bits == 32;
        if(depth_buffer_default != depth_default)
        {
            depth_buffer_default = depth_default;
            model.DirtyVariable("depth_buffer_default");
        }
        if(depth_buffer_16 != depth_16)
        {
            depth_buffer_16 = depth_16;
            model.DirtyVariable("depth_buffer_16");
        }
        if(depth_buffer_24 != depth_24)
        {
            depth_buffer_24 = depth_24;
            model.DirtyVariable("depth_buffer_24");
        }
        if(depth_buffer_32 != depth_32)
        {
            depth_buffer_32 = depth_32;
            model.DirtyVariable("depth_buffer_32");
        }

        if(!request_changed)
        {
            char status[96];
            formatdepthbufferstatus(
                bits,
                depth_buffer,
                depth_buffer_label.c_str(),
                status,
                sizeof(status)
            );
            depth_buffer_status = status;
            model.DirtyVariable("depth_buffer_status");
        }
        else
        {
            pending_depth_buffer = bits;
            depth_buffer_status = "Rebuilding the graphics context";
            model.DirtyVariable("depth_buffer_status");
        }

        event.StopPropagation();
    }

    void RmlSettings::selectcrosshair(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        const int selection = arguments[0].Get<int>();
        bool advertised = false;
        for(std::vector<CrosshairChoice>::const_iterator choice =
                crosshair_choices.begin();
            choice != crosshair_choices.end();
            ++choice)
        {
            if(choice->id == selection)
            {
                advertised = true;
                break;
            }
        }

        if(!advertised || !settings::writecrosshairselection(selection))
        {
            crosshair_status = "Crosshair selection was rejected";
            model.DirtyVariable("crosshair_status");
        }
        else
        {
            crosshair_status = "Crosshair textures loaded by Sauer";
            model.DirtyVariable("crosshair_status");
        }
        refreshcrosshair();
        event.StopPropagation();
    }

    void RmlSettings::selectsoundpack(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
        {
            event.StopPropagation();
            return;
        }

        const Rml::String requested = arguments[0].Get<Rml::String>();
        bool advertised = false;
        for(std::vector<SoundpackChoice>::const_iterator choice =
                soundpack_choices.begin();
            choice != soundpack_choices.end();
            ++choice)
        {
            if(choice->id == requested)
            {
                advertised = true;
                break;
            }
        }

        if(advertised && requested == active_soundpack)
        {
            soundpack_status = "Selected sound pack is already active";
            model.DirtyVariable("soundpack_status");
            event.StopPropagation();
            return;
        }

        const bool accepted = advertised &&
            settings::writesoundpack(requested.c_str());
        refreshaudio();
        soundpack_status = accepted
            ? "Gameplay sounds reloaded from the selected pack"
            : "Sound pack change failed; effective selection restored";
        model.DirtyVariable("soundpack_status");
        event.StopPropagation();
    }

    void RmlSettings::changefovaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->fov
        );

        if(!settings::writefov(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: failed to write the FOV setting\n"
            );
        }

        instance->refresh();
        event.StopPropagation();
    }

    bool RmlSettings::refreshgamefeedback()
    {
        settings::GameFeedback state = {};
        const bool available = settings::readgamefeedback(state);
        presentgamefeedback(state, available);
        return available;
    }

    void RmlSettings::presentgamefeedback(
        const settings::GameFeedback &state,
        bool available)
    {
        if(game_feedback_available != available)
        {
            game_feedback_available = available;
            model.DirtyVariable("game_feedback_available");
        }
        if(blood != state.blood)
        {
            blood = state.blood;
            model.DirtyVariable("blood");
        }
        if(hit_sound != state.hit_sound)
        {
            hit_sound = state.hit_sound;
            model.DirtyVariable("hit_sound");
        }
        if(outline_capture_meters != state.outline_capture_meters)
        {
            outline_capture_meters = state.outline_capture_meters;
            model.DirtyVariable("outline_capture_meters");
        }
        if(show_scoreboard_at_death != state.show_scoreboard_at_death)
        {
            show_scoreboard_at_death = state.show_scoreboard_at_death;
            model.DirtyVariable("show_scoreboard_at_death");
        }

        const char *status = available
            ? ""
            : "Combat feedback settings are unavailable";
        if(game_feedback_status != status)
        {
            game_feedback_status = status;
            model.DirtyVariable("game_feedback_status");
        }
    }

    void RmlSettings::selectblood(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_feedback_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameFeedback state = {};
        const bool accepted = settings::writeblood(enabled, state);
        const bool available = accepted ||
            settings::readgamefeedback(state);
        presentgamefeedback(state, available);
        if(!accepted && available)
        {
            game_feedback_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_feedback_status");
        }
    }

    void RmlSettings::selecthitsound(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_feedback_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameFeedback state = {};
        const bool accepted = settings::writehitsound(enabled, state);
        const bool available = accepted ||
            settings::readgamefeedback(state);
        presentgamefeedback(state, available);
        if(!accepted && available)
        {
            game_feedback_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_feedback_status");
        }
    }

    void RmlSettings::selectoutlinecapturemeters(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_feedback_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameFeedback state = {};
        const bool accepted = settings::writeoutlinemeters(enabled, state);
        const bool available = accepted ||
            settings::readgamefeedback(state);
        presentgamefeedback(state, available);
        if(!accepted && available)
        {
            game_feedback_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_feedback_status");
        }
    }

    void RmlSettings::selectshowscoreboardatdeath(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_feedback_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameFeedback state = {};
        const bool accepted = settings::writedeathscore(enabled, state);
        const bool available = accepted ||
            settings::readgamefeedback(state);
        presentgamefeedback(state, available);
        if(!accepted && available)
        {
            game_feedback_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_feedback_status");
        }
    }

    bool RmlSettings::refreshtrailcolors()
    {
        settings::TrailColors state = {};
        const bool available = settings::readtrailcolors(state);
        presenttrailcolors(state, available);
        return available;
    }

    void RmlSettings::presenttrailcolors(
        const settings::TrailColors &state,
        bool available)
    {
        if(trail_colors_available != available)
        {
            trail_colors_available = available;
            model.DirtyVariable("trail_colors_available");
        }
        if(trail_color_channel_minimum != state.channel_minimum)
        {
            trail_color_channel_minimum = state.channel_minimum;
            model.DirtyVariable("trail_color_channel_minimum");
        }
        if(trail_color_channel_maximum != state.channel_maximum)
        {
            trail_color_channel_maximum = state.channel_maximum;
            model.DirtyVariable("trail_color_channel_maximum");
        }

        const int shotgun_red = unpacktrailred(state.shotgun);
        const int shotgun_green = unpacktrailgreen(state.shotgun);
        const int shotgun_blue = unpacktrailblue(state.shotgun);
        if(trail_color_shotgun_red != shotgun_red)
        {
            trail_color_shotgun_red = shotgun_red;
            model.DirtyVariable("trail_color_shotgun_red");
        }
        if(trail_color_shotgun_green != shotgun_green)
        {
            trail_color_shotgun_green = shotgun_green;
            model.DirtyVariable("trail_color_shotgun_green");
        }
        if(trail_color_shotgun_blue != shotgun_blue)
        {
            trail_color_shotgun_blue = shotgun_blue;
            model.DirtyVariable("trail_color_shotgun_blue");
        }

        const int chaingun_red = unpacktrailred(state.chaingun);
        const int chaingun_green = unpacktrailgreen(state.chaingun);
        const int chaingun_blue = unpacktrailblue(state.chaingun);
        if(trail_color_chaingun_red != chaingun_red)
        {
            trail_color_chaingun_red = chaingun_red;
            model.DirtyVariable("trail_color_chaingun_red");
        }
        if(trail_color_chaingun_green != chaingun_green)
        {
            trail_color_chaingun_green = chaingun_green;
            model.DirtyVariable("trail_color_chaingun_green");
        }
        if(trail_color_chaingun_blue != chaingun_blue)
        {
            trail_color_chaingun_blue = chaingun_blue;
            model.DirtyVariable("trail_color_chaingun_blue");
        }

        const int rocket_red = unpacktrailred(state.rocketlauncher);
        const int rocket_green = unpacktrailgreen(state.rocketlauncher);
        const int rocket_blue = unpacktrailblue(state.rocketlauncher);
        if(trail_color_rocket_launcher_red != rocket_red)
        {
            trail_color_rocket_launcher_red = rocket_red;
            model.DirtyVariable("trail_color_rocket_launcher_red");
        }
        if(trail_color_rocket_launcher_green != rocket_green)
        {
            trail_color_rocket_launcher_green = rocket_green;
            model.DirtyVariable("trail_color_rocket_launcher_green");
        }
        if(trail_color_rocket_launcher_blue != rocket_blue)
        {
            trail_color_rocket_launcher_blue = rocket_blue;
            model.DirtyVariable("trail_color_rocket_launcher_blue");
        }

        const int rifle_red = unpacktrailred(state.rifle);
        const int rifle_green = unpacktrailgreen(state.rifle);
        const int rifle_blue = unpacktrailblue(state.rifle);
        if(trail_color_rifle_red != rifle_red)
        {
            trail_color_rifle_red = rifle_red;
            model.DirtyVariable("trail_color_rifle_red");
        }
        if(trail_color_rifle_green != rifle_green)
        {
            trail_color_rifle_green = rifle_green;
            model.DirtyVariable("trail_color_rifle_green");
        }
        if(trail_color_rifle_blue != rifle_blue)
        {
            trail_color_rifle_blue = rifle_blue;
            model.DirtyVariable("trail_color_rifle_blue");
        }

        const int grenade_red = unpacktrailred(state.grenadelauncher);
        const int grenade_green = unpacktrailgreen(state.grenadelauncher);
        const int grenade_blue = unpacktrailblue(state.grenadelauncher);
        if(trail_color_grenade_launcher_red != grenade_red)
        {
            trail_color_grenade_launcher_red = grenade_red;
            model.DirtyVariable("trail_color_grenade_launcher_red");
        }
        if(trail_color_grenade_launcher_green != grenade_green)
        {
            trail_color_grenade_launcher_green = grenade_green;
            model.DirtyVariable("trail_color_grenade_launcher_green");
        }
        if(trail_color_grenade_launcher_blue != grenade_blue)
        {
            trail_color_grenade_launcher_blue = grenade_blue;
            model.DirtyVariable("trail_color_grenade_launcher_blue");
        }

        const int pistol_red = unpacktrailred(state.pistol);
        const int pistol_green = unpacktrailgreen(state.pistol);
        const int pistol_blue = unpacktrailblue(state.pistol);
        if(trail_color_pistol_red != pistol_red)
        {
            trail_color_pistol_red = pistol_red;
            model.DirtyVariable("trail_color_pistol_red");
        }
        if(trail_color_pistol_green != pistol_green)
        {
            trail_color_pistol_green = pistol_green;
            model.DirtyVariable("trail_color_pistol_green");
        }
        if(trail_color_pistol_blue != pistol_blue)
        {
            trail_color_pistol_blue = pistol_blue;
            model.DirtyVariable("trail_color_pistol_blue");
        }

        const char *status = available
            ? ""
            : "Trail color settings are unavailable";
        if(trail_colors_status != status)
        {
            trail_colors_status = status;
            model.DirtyVariable("trail_colors_status");
        }
    }

    void RmlSettings::selecttrailcolorchannel(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 2 && arguments.size() != 3)
        {
            event.StopPropagation();
            return;
        }

        const int weapon = arguments[0].Get<int>(-1);
        const int channel = arguments[1].Get<int>(-1);

        // Native trailcolors tabs: shotgun, chaingun, rocketlauncher,
        // rifle, grenadelauncher, pistol. Channels are red, green, blue.

        int red = 0;
        int green = 0;
        int blue = 0;
        switch(weapon)
        {
            case 0:
                red = trail_color_shotgun_red;
                green = trail_color_shotgun_green;
                blue = trail_color_shotgun_blue;
                break;
            case 1:
                red = trail_color_chaingun_red;
                green = trail_color_chaingun_green;
                blue = trail_color_chaingun_blue;
                break;
            case 2:
                red = trail_color_rocket_launcher_red;
                green = trail_color_rocket_launcher_green;
                blue = trail_color_rocket_launcher_blue;
                break;
            case 3:
                red = trail_color_rifle_red;
                green = trail_color_rifle_green;
                blue = trail_color_rifle_blue;
                break;
            case 4:
                red = trail_color_grenade_launcher_red;
                green = trail_color_grenade_launcher_green;
                blue = trail_color_grenade_launcher_blue;
                break;
            case 5:
                red = trail_color_pistol_red;
                green = trail_color_pistol_green;
                blue = trail_color_pistol_blue;
                break;
            default:
                event.StopPropagation();
                return;
        }

        int current = 0;
        switch(channel)
        {
            case 0:
                current = red;
                break;
            case 1:
                current = green;
                break;
            case 2:
                current = blue;
                break;
            default:
                event.StopPropagation();
                return;
        }

        int requested = current;
        if(arguments.size() == 3)
            requested = arguments[2].Get<int>(current);
        else
            requested = event.GetParameter<int>("value", current);

        // Ignore programmatic range updates so opening Settings cannot
        // rewrite the native ident. RmlUi emits change for min/max clamps
        // during InstanceElement, before the range is parented. There is
        // no hardcoded RML value default.
        if(requested == current)
        {
            event.StopPropagation();
            return;
        }

        Rml::Element *element = event.GetCurrentElement();
        if(!element || !element->GetParentNode())
        {
            event.StopPropagation();
            return;
        }

        if(!trail_colors_available)
        {
            event.StopPropagation();
            return;
        }

        if(requested < trail_color_channel_minimum ||
           requested > trail_color_channel_maximum)
        {
            event.StopPropagation();
            return;
        }

        switch(channel)
        {
            case 0:
                red = requested;
                break;
            case 1:
                green = requested;
                break;
            case 2:
                blue = requested;
                break;
        }

        const int packed = packtrailcolor(red, green, blue);
        settings::TrailColors state = {};
        bool accepted = false;
        switch(weapon)
        {
            case 0:
                accepted = settings::writetrailcolorshotgun(packed, state);
                break;
            case 1:
                accepted = settings::writetrailcolorchaingun(packed, state);
                break;
            case 2:
                accepted = settings::writetrailcolorrocketlauncher(
                    packed, state);
                break;
            case 3:
                accepted = settings::writetrailcolorrifle(packed, state);
                break;
            case 4:
                accepted = settings::writetrailcolorgrenadelauncher(
                    packed, state);
                break;
            case 5:
                accepted = settings::writetrailcolorpistol(packed, state);
                break;
        }

        const bool available = accepted ||
            settings::readtrailcolors(state);
        presenttrailcolors(state, available);
        if(!accepted && available)
        {
            trail_colors_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("trail_colors_status");
        }
        event.StopPropagation();
    }

    bool RmlSettings::refreshgameradar()
    {
        settings::GameRadar state = {};
        const bool available = settings::readgameradar(state);
        presentgameradar(state, available);
        return available;
    }

    void RmlSettings::presentgameradar(
        const settings::GameRadar &state,
        bool available)
    {
        if(game_radar_available != available)
        {
            game_radar_available = available;
            model.DirtyVariable("game_radar_available");
        }
        if(numbered_capture_bases != state.numbered_capture_bases)
        {
            numbered_capture_bases = state.numbered_capture_bases;
            model.DirtyVariable("numbered_capture_bases");
        }
        if(show_teammates_on_radar != state.show_teammates_on_radar)
        {
            show_teammates_on_radar = state.show_teammates_on_radar;
            model.DirtyVariable("show_teammates_on_radar");
        }

        const char *status = available
            ? ""
            : "Radar settings are unavailable";
        if(game_radar_status != status)
        {
            game_radar_status = status;
            model.DirtyVariable("game_radar_status");
        }
    }

    void RmlSettings::selectnumberedcapturebases(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_radar_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameRadar state = {};
        const bool accepted = settings::writebasenumbers(enabled, state);
        const bool available = accepted ||
            settings::readgameradar(state);
        presentgameradar(state, available);
        if(!accepted && available)
        {
            game_radar_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_radar_status");
        }
    }

    void RmlSettings::selectshowteammatesonradar(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_radar_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameRadar state = {};
        const bool accepted = settings::writeradarteammates(enabled, state);
        const bool available = accepted ||
            settings::readgameradar(state);
        presentgameradar(state, available);
        if(!accepted && available)
        {
            game_radar_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_radar_status");
        }
    }

    bool RmlSettings::refreshgamemodels()
    {
        settings::GameModels state = {};
        const bool available = settings::readgamemodels(state);
        presentgamemodels(state, available);
        return available;
    }

    void RmlSettings::presentgamemodels(
        const settings::GameModels &state,
        bool available)
    {
        if(game_models_available != available)
        {
            game_models_available = available;
            model.DirtyVariable("game_models_available");
        }
        if(ragdoll_deaths != state.ragdoll_deaths)
        {
            ragdoll_deaths = state.ragdoll_deaths;
            model.DirtyVariable("ragdoll_deaths");
        }
        if(keep_ragdoll_after_respawn != state.keep_ragdoll_after_respawn)
        {
            keep_ragdoll_after_respawn = state.keep_ragdoll_after_respawn;
            model.DirtyVariable("keep_ragdoll_after_respawn");
        }
        if(deadpush != state.deadpush)
        {
            deadpush = state.deadpush;
            model.DirtyVariable("deadpush");
        }
        if(deadpush_minimum != state.deadpush_minimum)
        {
            deadpush_minimum = state.deadpush_minimum;
            model.DirtyVariable("deadpush_minimum");
        }
        if(deadpush_maximum != state.deadpush_maximum)
        {
            deadpush_maximum = state.deadpush_maximum;
            model.DirtyVariable("deadpush_maximum");
        }
        if(hide_dead_players != state.hide_dead_players)
        {
            hide_dead_players = state.hide_dead_players;
            model.DirtyVariable("hide_dead_players");
        }
        if(fullbright_player_models != state.fullbright_player_models)
        {
            fullbright_player_models = state.fullbright_player_models;
            model.DirtyVariable("fullbright_player_models");
        }
        const int intensity = fullbrightintensityselected(
            state.fullbrightmodels);
        if(fullbright_intensity_selected != intensity)
        {
            fullbright_intensity_selected = intensity;
            model.DirtyVariable("fullbright_intensity_selected");
        }
        if(force_matching_player_models !=
           state.force_matching_player_models)
        {
            force_matching_player_models =
                state.force_matching_player_models;
            model.DirtyVariable("force_matching_player_models");
        }
        if(always_use_team_skins != state.always_use_team_skins)
        {
            always_use_team_skins = state.always_use_team_skins;
            model.DirtyVariable("always_use_team_skins");
        }

        Rml::String status;
        if(!available)
            status = "Player model settings are unavailable";
        else
        {
            if(state.hidedead > 1)
            {
                char text[160];
                std::snprintf(
                    text,
                    sizeof(text),
                    "Hide-dead is %d; shown on without rewriting. A toggle writes 0 or 1.",
                    state.hidedead
                );
                status = text;
            }
            if(state.ragdoll_millis != 0 &&
               state.ragdoll_millis != 10000)
            {
                char text[192];
                std::snprintf(
                    text,
                    sizeof(text),
                    "Ragdolls stay for %d ms; shown on without rewriting. A toggle writes 10000 or 0.",
                    state.ragdoll_millis
                );
                if(!status.empty())
                    status += " ";
                status += text;
            }
            if(state.fullbrightmodels != 0 &&
               fullbrightintensityselected(state.fullbrightmodels) < 0)
            {
                char text[192];
                std::snprintf(
                    text,
                    sizeof(text),
                    "Fullbright intensity is %d; no radio is selected. Opening Settings does not rewrite.",
                    state.fullbrightmodels
                );
                if(!status.empty())
                    status += " ";
                status += text;
            }
        }
        if(game_models_status != status)
        {
            game_models_status = status;
            model.DirtyVariable("game_models_status");
        }
    }

    void RmlSettings::selectragdolldeaths(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_models_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameModels state = {};
        const bool accepted = settings::writeragdoll(enabled, state);
        const bool available = accepted ||
            settings::readgamemodels(state);
        presentgamemodels(state, available);
        if(!accepted && available)
        {
            game_models_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_models_status");
        }
    }

    void RmlSettings::selectkeepragdollafterrespawn(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_models_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameModels state = {};
        const bool accepted = settings::writeragdollmillis(enabled, state);
        const bool available = accepted ||
            settings::readgamemodels(state);
        presentgamemodels(state, available);
        if(!accepted && available)
        {
            game_models_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_models_status");
        }
    }

    void RmlSettings::selecthidedeadplayers(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_models_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameModels state = {};
        const bool accepted = settings::writehidedead(enabled, state);
        const bool available = accepted ||
            settings::readgamemodels(state);
        presentgamemodels(state, available);
        if(!accepted && available)
        {
            game_models_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_models_status");
        }
    }

    void RmlSettings::selectfullbrightplayermodels(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_models_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameModels state = {};
        const bool accepted = settings::writefullbrightmodels(
            enabled, state);
        const bool available = accepted ||
            settings::readgamemodels(state);
        presentgamemodels(state, available);
        if(!accepted && available)
        {
            game_models_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_models_status");
        }
    }

    void RmlSettings::selectfullbrightintensity(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_models_available)
            return;

        const int value = arguments[0].Get<int>();
        if(fullbrightintensityselected(value) < 0)
            return;

        settings::GameModels state = {};
        const bool accepted = settings::writefullbrightintensity(
            value, state);
        const bool available = accepted ||
            settings::readgamemodels(state);
        presentgamemodels(state, available);
        if(!accepted && available)
        {
            game_models_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_models_status");
        }
    }

    void RmlSettings::selectforceplayermodels(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_models_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameModels state = {};
        const bool accepted = settings::writeforceplayermodels(
            enabled, state);
        const bool available = accepted ||
            settings::readgamemodels(state);
        presentgamemodels(state, available);
        if(!accepted && available)
        {
            game_models_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_models_status");
        }
    }

    void RmlSettings::selectalwaysuseteamskins(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !game_models_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GameModels state = {};
        const bool accepted = settings::writeteamskins(enabled, state);
        const bool available = accepted ||
            settings::readgamemodels(state);
        presentgamemodels(state, available);
        if(!accepted && available)
        {
            game_models_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("game_models_status");
        }
    }

    bool RmlSettings::refreshammobar()
    {
        settings::Ammobar state = {};
        const bool available = settings::readammobar(state);
        presentammobar(state, available);
        return available;
    }

    void RmlSettings::presentammobar(
        const settings::Ammobar &state,
        bool available)
    {
        if(ammobar_available != available)
        {
            ammobar_available = available;
            model.DirtyVariable("ammobar_available");
        }

        const char *status = available
            ? ""
            : "Ammobar settings are unavailable";
        if(ammobar_status != status)
        {
            ammobar_status = status;
            model.DirtyVariable("ammobar_status");
        }

        if(!available)
            return;

        if(ammobar_visible != state.visible)
        {
            ammobar_visible = state.visible;
            model.DirtyVariable("ammobar_visible");
            if(ammobar_visible)
            {
                ammobar_icon_spacing_armed = false;
                ammobar_label_offset_armed = false;
            }
        }
        if(ammobar_horizontal != state.horizontal)
        {
            ammobar_horizontal = state.horizontal;
            model.DirtyVariable("ammobar_horizontal");
        }
        if(ammobar_flip != state.flip)
        {
            ammobar_flip = state.flip;
            model.DirtyVariable("ammobar_flip");
        }
        if(ammobar_scale != state.scale)
        {
            ammobar_scale = state.scale;
            model.DirtyVariable("ammobar_scale");
        }
        if(ammobar_scale_minimum != state.scale_minimum)
        {
            ammobar_scale_minimum = state.scale_minimum;
            model.DirtyVariable("ammobar_scale_minimum");
        }
        if(ammobar_scale_maximum != state.scale_maximum)
        {
            ammobar_scale_maximum = state.scale_maximum;
            model.DirtyVariable("ammobar_scale_maximum");
        }
        if(ammobar_icon_spacing != state.icon_spacing)
        {
            ammobar_icon_spacing = state.icon_spacing;
            model.DirtyVariable("ammobar_icon_spacing");
        }
        if(ammobar_icon_spacing_minimum != state.icon_spacing_minimum)
        {
            ammobar_icon_spacing_minimum = state.icon_spacing_minimum;
            model.DirtyVariable("ammobar_icon_spacing_minimum");
        }
        if(ammobar_icon_spacing_maximum != state.icon_spacing_maximum)
        {
            ammobar_icon_spacing_maximum = state.icon_spacing_maximum;
            model.DirtyVariable("ammobar_icon_spacing_maximum");
        }
        if(ammobar_label_size != state.label_size)
        {
            ammobar_label_size = state.label_size;
            model.DirtyVariable("ammobar_label_size");
        }
        if(ammobar_label_size_minimum != state.label_size_minimum)
        {
            ammobar_label_size_minimum = state.label_size_minimum;
            model.DirtyVariable("ammobar_label_size_minimum");
        }
        if(ammobar_label_size_maximum != state.label_size_maximum)
        {
            ammobar_label_size_maximum = state.label_size_maximum;
            model.DirtyVariable("ammobar_label_size_maximum");
        }
        if(ammobar_label_offset != state.label_offset)
        {
            ammobar_label_offset = state.label_offset;
            model.DirtyVariable("ammobar_label_offset");
        }
        if(ammobar_label_offset_minimum != state.label_offset_minimum)
        {
            ammobar_label_offset_minimum = state.label_offset_minimum;
            model.DirtyVariable("ammobar_label_offset_minimum");
        }
        if(ammobar_label_offset_maximum != state.label_offset_maximum)
        {
            ammobar_label_offset_maximum = state.label_offset_maximum;
            model.DirtyVariable("ammobar_label_offset_maximum");
        }

        const bool align_first = state.align == -1;
        const bool align_center = state.align == 0;
        const bool align_last = state.align == 1;
        if(ammobar_align_first != align_first)
        {
            ammobar_align_first = align_first;
            model.DirtyVariable("ammobar_align_first");
        }
        if(ammobar_align_center != align_center)
        {
            ammobar_align_center = align_center;
            model.DirtyVariable("ammobar_align_center");
        }
        if(ammobar_align_last != align_last)
        {
            ammobar_align_last = align_last;
            model.DirtyVariable("ammobar_align_last");
        }

        if(ammobar_x != state.x)
        {
            ammobar_x = state.x;
            model.DirtyVariable("ammobar_x");
        }
        if(ammobar_x_minimum != state.x_minimum)
        {
            ammobar_x_minimum = state.x_minimum;
            model.DirtyVariable("ammobar_x_minimum");
        }
        if(ammobar_x_maximum != state.x_maximum)
        {
            ammobar_x_maximum = state.x_maximum;
            model.DirtyVariable("ammobar_x_maximum");
        }
        if(ammobar_y != state.y)
        {
            ammobar_y = state.y;
            model.DirtyVariable("ammobar_y");
        }
        if(ammobar_y_minimum != state.y_minimum)
        {
            ammobar_y_minimum = state.y_minimum;
            model.DirtyVariable("ammobar_y_minimum");
        }
        if(ammobar_y_maximum != state.y_maximum)
        {
            ammobar_y_maximum = state.y_maximum;
            model.DirtyVariable("ammobar_y_maximum");
        }
        if(ammobar_hide_empty != state.hide_empty)
        {
            ammobar_hide_empty = state.hide_empty;
            model.DirtyVariable("ammobar_hide_empty");
        }

        const Rml::String scale_text = formatsensitivity(state.scale);
        if(!ammobar_scale_editing && ammobar_scale_input != scale_text)
        {
            ammobar_scale_input = scale_text;
            model.DirtyVariable("ammobar_scale_input");
        }
        const Rml::String label_size_text =
            formatsensitivity(state.label_size);
        if(!ammobar_label_size_editing &&
           ammobar_label_size_input != label_size_text)
        {
            ammobar_label_size_input = label_size_text;
            model.DirtyVariable("ammobar_label_size_input");
        }
        const Rml::String x_text = formatsensitivity(state.x);
        if(!ammobar_x_editing && ammobar_x_input != x_text)
        {
            ammobar_x_input = x_text;
            model.DirtyVariable("ammobar_x_input");
        }
        const Rml::String y_text = formatsensitivity(state.y);
        if(!ammobar_y_editing && ammobar_y_input != y_text)
        {
            ammobar_y_input = y_text;
            model.DirtyVariable("ammobar_y_input");
        }
    }

    void RmlSettings::finishammobarwrite(
        bool accepted,
        settings::Ammobar &state)
    {
        const bool available = accepted || settings::readammobar(state);
        presentammobar(state, available);
        if(!accepted && available)
        {
            ammobar_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("ammobar_status");
        }
    }

    void RmlSettings::selectammobar(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !ammobar_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobar(enabled, state);
        finishammobarwrite(accepted, state);
    }

    void RmlSettings::selectammobarhorizontal(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !ammobar_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarhorizontal(enabled, state);
        finishammobarwrite(accepted, state);
    }

    void RmlSettings::selectammobarflip(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !ammobar_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarflip(enabled, state);
        finishammobarwrite(accepted, state);
    }

    void RmlSettings::selectammobarhideempty(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !ammobar_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarhideempty(enabled, state);
        finishammobarwrite(accepted, state);
    }

    void RmlSettings::selectammobaralign(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !ammobar_available)
            return;

        const int requested = arguments[0].Get<int>();
        if(requested != -1 && requested != 0 && requested != 1)
            return;

        settings::Ammobar state = {};
        const bool accepted = settings::writeammobaralign(requested, state);
        finishammobarwrite(accepted, state);
    }

    bool RmlSettings::refreshscoreboardcolumns()
    {
        settings::ScoreboardColumns state = {};
        const bool available = settings::readscoreboardcolumns(state);
        presentscoreboardcolumns(state, available);
        return available;
    }

    void RmlSettings::presentscoreboardcolumns(
        const settings::ScoreboardColumns &state,
        bool available)
    {
        if(scoreboard_available != available)
        {
            scoreboard_available = available;
            model.DirtyVariable("scoreboard_available");
        }

        if(scoreboard_flags != state.show_flags)
        {
            scoreboard_flags = state.show_flags;
            model.DirtyVariable("scoreboard_flags");
        }
        if(scoreboard_frags != state.show_frags)
        {
            scoreboard_frags = state.show_frags;
            model.DirtyVariable("scoreboard_frags");
        }
        if(scoreboard_deaths != state.show_deaths)
        {
            scoreboard_deaths = state.show_deaths;
            model.DirtyVariable("scoreboard_deaths");
        }
        if(scoreboard_suicides != state.show_suicides)
        {
            scoreboard_suicides = state.show_suicides;
            model.DirtyVariable("scoreboard_suicides");
        }
        if(scoreboard_kpd != state.show_kpd)
        {
            scoreboard_kpd = state.show_kpd;
            model.DirtyVariable("scoreboard_kpd");
        }
        if(scoreboard_accuracy != state.show_accuracy)
        {
            scoreboard_accuracy = state.show_accuracy;
            model.DirtyVariable("scoreboard_accuracy");
        }
        if(scoreboard_damage != state.show_damage)
        {
            scoreboard_damage = state.show_damage;
            model.DirtyVariable("scoreboard_damage");
        }
        if(scoreboard_damage_dealt != state.show_damage_dealt)
        {
            scoreboard_damage_dealt = state.show_damage_dealt;
            model.DirtyVariable("scoreboard_damage_dealt");
        }
        if(scoreboard_damage_net != state.show_damage_net)
        {
            scoreboard_damage_net = state.show_damage_net;
            model.DirtyVariable("scoreboard_damage_net");
        }
        if(scoreboard_damage_received != state.show_damage_received)
        {
            scoreboard_damage_received = state.show_damage_received;
            model.DirtyVariable("scoreboard_damage_received");
        }
        if(scoreboard_ping != state.show_ping)
        {
            scoreboard_ping = state.show_ping;
            model.DirtyVariable("scoreboard_ping");
        }
        if(scoreboard_pj != state.show_pj)
        {
            scoreboard_pj = state.show_pj;
            model.DirtyVariable("scoreboard_pj");
        }
        if(scoreboard_clientnum != state.show_clientnum)
        {
            scoreboard_clientnum = state.show_clientnum;
            model.DirtyVariable("scoreboard_clientnum");
        }

        Rml::String status;
        if(!available)
            status = "Scoreboard column settings are unavailable";
        else
        {
            if(state.showping == 2)
            {
                status =
                    "Ping is 2; shown on without rewriting. A toggle off writes 0, on from 0 writes 1.";
            }
            if(state.show_damage &&
               !state.show_damage_dealt &&
               !state.show_damage_net)
            {
                char text[192];
                std::snprintf(
                    text,
                    sizeof(text),
                    "Damage is %d; no radio is selected. Opening Settings does not rewrite.",
                    state.showdamage
                );
                if(!status.empty())
                    status += " ";
                status += text;
            }
        }
        if(scoreboard_status != status)
        {
            scoreboard_status = status;
            model.DirtyVariable("scoreboard_status");
        }
    }

    void RmlSettings::finishscoreboardwrite(
        bool accepted,
        settings::ScoreboardColumns &state)
    {
        const bool available = accepted ||
            settings::readscoreboardcolumns(state);
        presentscoreboardcolumns(state, available);
        if(!accepted && available)
        {
            scoreboard_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("scoreboard_status");
        }
    }

    void RmlSettings::selectscoreboardflags(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowflags(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboardfrags(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writehidefrags(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboarddeaths(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowdeaths(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboardsuicides(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowsuicides(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboardkpd(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowkpd(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboardaccuracy(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowaccuracy(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboarddamage(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowdamage(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboarddamagemode(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const int value = arguments[0].Get<int>();
        if(value != 1 && value != 2)
            return;

        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowdamagemode(value, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboarddamagereceived(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowdamagereceived(
            enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboardping(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowping(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboardpj(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowpj(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectscoreboardclientnum(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !scoreboard_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ScoreboardColumns state = {};
        const bool accepted = settings::writeshowclientnum(enabled, state);
        finishscoreboardwrite(accepted, state);
    }

    void RmlSettings::selectfragmessagestackdir(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !frag_messages_enabled)
            return;

        const int requested = arguments[0].Get<int>();
        if(requested != -1 && requested != 0 && requested != 1)
            return;

        settings::Hud state = {};
        const bool accepted = settings::writefragmessagestackdir(
            requested, state);
        finishhudwrite(accepted, state);
    }

    void RmlSettings::gamecategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Game);
        event.StopPropagation();
    }

    void RmlSettings::hudcategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Hud);
        event.StopPropagation();
    }

    void RmlSettings::graphicscategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Graphics);
        event.StopPropagation();
    }

    void RmlSettings::audiocategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Audio);
        event.StopPropagation();
    }

    void RmlSettings::mousekeyboardcategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::MouseKeyboard);
        event.StopPropagation();
    }

    void RmlSettings::crosshaircategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Crosshair);
        event.StopPropagation();
    }

    void RmlSettings::chatcategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Chat);
        event.StopPropagation();
    }

    void RmlSettings::consolecategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Console);
        event.StopPropagation();
    }

    void RmlSettings::windowedaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->setpresentation(
                settings::PresentationMode::Windowed,
                event
            );
        }
        else event.StopPropagation();
    }

    void RmlSettings::exclusivefullscreenaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->setpresentation(
                settings::PresentationMode::ExclusiveFullscreen,
                event
            );
        }
        else event.StopPropagation();
    }

    void RmlSettings::desktopfullscreenaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->setpresentation(
                settings::PresentationMode::DesktopFullscreen,
                event
            );
        }
        else event.StopPropagation();
    }

    void RmlSettings::refreshanisotropy()
    {
        settings::AnisotropyState state = {};
        settings::readanisotropy(state);
        presentanisotropy(state);
    }

    void RmlSettings::refreshbilinearfiltering()
    {
        settings::BilinearFilteringState state = {};
        const bool available = settings::readbilinearfiltering(state);
        presentbilinearfiltering(state, available);
    }

    void RmlSettings::refreshmipmapblending()
    {
        settings::MipmapBlendingState state = {};
        settings::readmipmapblending(state);
        presentmipmapblending(state);
    }

    void RmlSettings::refreshshaderdetail()
    {
        settings::ShaderDetailState state = {};
        settings::readshaderdetail(state);
        presentshaderdetail(state);
    }

    void RmlSettings::presentshaderdetail(const settings::ShaderDetailState &state)
    {
        shader_detail_available = state.ready;
        shader_detail_selected = state.requested >= 0 && state.requested <= 3 ?
            state.requested : -1;
        switch(state.requested)
        {
            case 0: shader_detail_label = "Performance"; break;
            case 1: shader_detail_label = "Low"; break;
            case 2: shader_detail_label = "Medium"; break;
            case 3: shader_detail_label = "Full"; break;
            default:
            {
                char label[32];
                std::snprintf(label, sizeof(label), "Level %d", state.requested);
                shader_detail_label = label;
                shader_detail_selected = -1;
                break;
            }
        }
        if(!state.ready) shader_detail_status = "Shader renderer is not ready";
        else if(state.unresolved) shader_detail_status = "Some requested shaders are unavailable";
        else if(state.alternatives) shader_detail_status = "Some shaders are using fallback implementations";
        else shader_detail_status.clear();
        model.DirtyVariable("shader_detail_available");
        model.DirtyVariable("shader_detail_selected");
        model.DirtyVariable("shader_detail_label");
        model.DirtyVariable("shader_detail_status");
    }

    void RmlSettings::selectshaderdetail(
        Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !shader_detail_available) return;
        const int value = arguments[0].Get<int>();
        if(value < 0 || value > 3) return;
        // No native write/compilation while RmlUi is dispatching this event.
        pending_shader_detail = value;
    }

    void RmlSettings::applypendingshaderdetail()
    {
        if(pending_shader_detail < 0) return;
        const int value = pending_shader_detail;
        pending_shader_detail = -1;
        settings::ShaderDetailState state = {};
        const bool accepted = settings::writeshaderdetail(
            static_cast<settings::ShaderDetailLevel>(value), state);
        presentshaderdetail(state);
        if(!accepted)
        {
            shader_detail_status = "Change not accepted. " + shader_detail_status;
            model.DirtyVariable("shader_detail_status");
        }
    }

    void RmlSettings::presentbilinearfiltering(
        const settings::BilinearFilteringState &state,
        bool available)
    {
        bilinear_filtering_available = available;
        bilinear_filtering = state.enabled;
        bilinear_filtering_status = available
            ? ""
            : "Bilinear filtering state is unavailable";

        model.DirtyVariable("bilinear_filtering_available");
        model.DirtyVariable("bilinear_filtering");
        model.DirtyVariable("bilinear_filtering_status");
    }

    void RmlSettings::selectbilinearfiltering(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !bilinear_filtering_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::BilinearFilteringState state = {};
        const bool accepted = settings::writebilinearfiltering(
            enabled, state);
        const bool available = accepted ||
            settings::readbilinearfiltering(state);
        presentbilinearfiltering(state, available);
        if(!accepted && available)
        {
            bilinear_filtering_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("bilinear_filtering_status");
        }
    }

    void RmlSettings::presentmipmapblending(const settings::MipmapBlendingState &state)
    {
        using Status = settings::MipmapBlendingStatus;
        mipmap_blending = state.requested;
        mipmap_blending_available = state.status != Status::Unavailable;
        switch(state.status)
        {
            case Status::Unavailable: mipmap_blending_status = "Graphics context is unavailable"; break;
            case Status::NoTextures: mipmap_blending_status = "Configured for new textures; no eligible textures loaded"; break;
            case Status::Verified: mipmap_blending_status = "Loaded texture filtering verified"; break;
            case Status::Mismatch: mipmap_blending_status = "Loaded texture filtering differs from the configured value"; break;
            case Status::Mixed: mipmap_blending_status = "Loaded textures have inconsistent mipmap blending"; break;
        }
        model.DirtyVariable("mipmap_blending");
        model.DirtyVariable("mipmap_blending_available");
        model.DirtyVariable("mipmap_blending_status");
    }

    void RmlSettings::selectmipmapblending(
        Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1) return;
        const bool enabled = arguments[0].Get<bool>();
        settings::MipmapBlendingState state = {};
        const bool accepted = settings::writemipmapblending(enabled, state);
        presentmipmapblending(state); // Synchronous callback result, no rescan.
        if(!accepted)
        {
            mipmap_blending_status = "Change not accepted. " + mipmap_blending_status;
            model.DirtyVariable("mipmap_blending_status");
        }
    }

    void RmlSettings::presentanisotropy(const settings::AnisotropyState &state)
    {
        using Status = settings::AnisotropyStatus;
        anisotropy_enabled = state.status != Status::Unavailable &&
            state.status != Status::Unsupported;
        anisotropy_maximum = state.maximum;
        anisotropy_selected = -1;
        if(state.status == Status::Verified || state.status == Status::NoTextures)
            anisotropy_selected = state.requested <= 1 ? 0 : state.requested;

        char value[80], request[80];
        const float actual = state.status == Status::NoTextures
            ? float(state.capped) : state.actual;
        if(actual <= 1) std::snprintf(value, sizeof(value), "Off");
        else std::snprintf(value, sizeof(value), "%gx", double(actual));
        anisotropy_label = value;
        const bool custom = state.requested > 1 && state.requested != 2 &&
            state.requested != 4 && state.requested != 8 && state.requested != 16;
        if(custom && actual == float(state.requested))
            anisotropy_label += " (custom)";

        switch(state.status)
        {
            case Status::Unavailable:
                anisotropy_label = "Unavailable";
                anisotropy_status = "Graphics context is unavailable";
                break;
            case Status::Unsupported:
                anisotropy_label = "Unsupported";
                anisotropy_status = "Anisotropic filtering is not supported";
                break;
            case Status::NoTextures:
                anisotropy_status = "Configured for new textures; no eligible textures loaded";
                break;
            case Status::Verified:
                anisotropy_status = "Loaded texture filtering verified";
                break;
            case Status::Mismatch:
                anisotropy_status = "Loaded texture filtering differs from the configured value";
                break;
            case Status::Mixed:
                anisotropy_label = "Mixed";
                anisotropy_status = "Loaded textures have inconsistent filtering";
                break;
        }
        if(state.requested > 1 &&
           (actual != float(state.requested) || state.status == Status::Mixed ||
            state.status == Status::Unavailable || state.status == Status::Unsupported))
        {
            std::snprintf(request, sizeof(request), " | Requested: %dx%s",
                state.requested, anisotropy_enabled && state.requested > state.maximum
                    ? "; limited by hardware" : "");
            anisotropy_status += request;
        }
        model.DirtyVariable("anisotropy_enabled");
        model.DirtyVariable("anisotropy_maximum");
        model.DirtyVariable("anisotropy_selected");
        model.DirtyVariable("anisotropy_label");
        model.DirtyVariable("anisotropy_status");
    }

    void RmlSettings::selectanisotropy(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1) return;
        settings::AnisotropyLevel level;
        switch(arguments[0].Get<int>())
        {
            case 0: level = settings::AnisotropyLevel::Off; break;
            case 2: level = settings::AnisotropyLevel::X2; break;
            case 4: level = settings::AnisotropyLevel::X4; break;
            case 8: level = settings::AnisotropyLevel::X8; break;
            case 16: level = settings::AnisotropyLevel::X16; break;
            default: return;
        }
        settings::AnisotropyState state = {};
        const bool accepted = settings::writeanisotropy(level, state);
        presentanisotropy(state); // Callback readback, not another texture scan.
        if(!accepted)
        {
            anisotropy_status = "Change not accepted. " + anisotropy_status;
            model.DirtyVariable("anisotropy_status");
        }
    }

    bool RmlSettings::refreshgraphicsquality()
    {
        settings::GraphicsQuality current = {};
        const bool read = settings::readgraphicsquality(current);
        grass = current.grass;
        motion_blur = current.motion_blur;
        motion_blur_available = read && current.motion_blur_available;
        glare = current.glare;
        graphics_quality_status = read ? "" : "Graphics quality state is unavailable";
        model.DirtyVariable("grass");
        model.DirtyVariable("motion_blur");
        model.DirtyVariable("motion_blur_available");
        model.DirtyVariable("glare");
        model.DirtyVariable("graphics_quality_status");
        return read;
    }

    void RmlSettings::refreshshadows()
    {
        settings::ShadowState state = {};
        const bool available = settings::readshadows(state);
        presentshadows(state, available);
    }

    void RmlSettings::presentshadows(
        const settings::ShadowState &state,
        bool available)
    {
        shadow_state_available = available;
        shadow_maps = state.shadow_maps;
        shadow_maps_available = available && state.shadow_maps_available;
        blob_shadows = state.blob_shadows;
        shadow_status = available ? "" : "Shadow state is unavailable";

        model.DirtyVariable("shadow_state_available");
        model.DirtyVariable("shadow_maps");
        model.DirtyVariable("shadow_maps_available");
        model.DirtyVariable("blob_shadows");
        model.DirtyVariable("shadow_status");
    }

    void RmlSettings::selectshadowmaps(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !shadow_state_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ShadowState state = {};
        const bool accepted = settings::writeshadowmaps(enabled, state);
        const bool available = accepted || settings::readshadows(state);
        presentshadows(state, available);
        if(!accepted && available)
        {
            shadow_status =
                "Shadow Maps change was not accepted; current engine state is shown";
            model.DirtyVariable("shadow_status");
        }
    }

    void RmlSettings::selectblobshadows(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !shadow_state_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ShadowState state = {};
        const bool accepted = settings::writeblobshadows(enabled, state);
        const bool available = accepted || settings::readshadows(state);
        presentshadows(state, available);
        if(!accepted && available)
        {
            shadow_status =
                "Blob Shadows change was not accepted; current engine state is shown";
            model.DirtyVariable("shadow_status");
        }
    }

    void RmlSettings::refreshshadowquality()
    {
        settings::ShadowQualityState state = {};
        const bool available = settings::readshadowquality(state);
        presentshadowquality(state, available);
    }

    void RmlSettings::presentshadowquality(
        const settings::ShadowQualityState &state,
        bool available)
    {
        shadow_quality_available = available;
        shadow_quality_selected = -1;
        shadow_quality_status.clear();

        if(!available)
        {
            shadow_quality_label = "Unavailable";
            shadow_quality_status =
                "Shadow map quality state is unavailable";
        }
        else switch(state.quality)
        {
            case settings::ShadowQuality::Medium:
                shadow_quality_selected = 0;
                shadow_quality_label = "Medium";
                break;
            case settings::ShadowQuality::High:
                shadow_quality_selected = 1;
                shadow_quality_label = "High";
                break;
            case settings::ShadowQuality::Custom:
            {
                shadow_quality_label = "Custom";
                char status[96];
                if(state.map_size >= 7 && state.map_size <= 11)
                {
                    std::snprintf(status, sizeof(status),
                        "Size %d (%d px) | Blur %d", state.map_size,
                        1 << state.map_size, state.blur);
                }
                else
                {
                    std::snprintf(status, sizeof(status),
                        "Size %d | Blur %d", state.map_size, state.blur);
                }
                shadow_quality_status = status;
                break;
            }
        }

        model.DirtyVariable("shadow_quality_available");
        model.DirtyVariable("shadow_quality_selected");
        model.DirtyVariable("shadow_quality_label");
        model.DirtyVariable("shadow_quality_status");
    }

    void RmlSettings::selectshadowquality(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !shadow_quality_available)
            return;

        settings::ShadowQuality quality;
        switch(arguments[0].Get<int>())
        {
            case 0: quality = settings::ShadowQuality::Medium; break;
            case 1: quality = settings::ShadowQuality::High; break;
            default: return;
        }

        settings::ShadowQualityState state = {};
        const bool accepted = settings::writeshadowquality(quality, state);
        const bool available = accepted || settings::readshadowquality(state);
        presentshadowquality(state, available);
        if(!accepted && available)
        {
            if(shadow_quality_status.empty())
            {
                shadow_quality_status =
                    "Change not accepted; current engine state is shown";
            }
            else
            {
                shadow_quality_status = "Change not accepted. " +
                    shadow_quality_status;
            }
            model.DirtyVariable("shadow_quality_status");
        }
    }

    void RmlSettings::refreshwatersurfaceeffects()
    {
        settings::WaterSurfaceEffectsState state = {};
        const bool available = settings::readwatersurfaceeffects(state);
        presentwatersurfaceeffects(state, available);
    }

    void RmlSettings::presentwatersurfaceeffects(
        const settings::WaterSurfaceEffectsState &state,
        bool available)
    {
        water_surface_state_available = available;
        water_refraction = state.refraction;
        water_refraction_available = available &&
            state.render_targets_available;
        water_reflections = state.reflections;
        water_reflections_available = available &&
            state.render_targets_available;
        water_caustics = state.caustics;
        liquid_surface_animation = state.liquid_animation;
        water_surface_status = available
            ? ""
            : "Water effects state is unavailable";

        model.DirtyVariable("water_surface_state_available");
        model.DirtyVariable("water_refraction");
        model.DirtyVariable("water_refraction_available");
        model.DirtyVariable("water_reflections");
        model.DirtyVariable("water_reflections_available");
        model.DirtyVariable("water_caustics");
        model.DirtyVariable("liquid_surface_animation");
        model.DirtyVariable("water_surface_status");
    }

    void RmlSettings::selectwaterrefraction(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !water_surface_state_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::WaterSurfaceEffectsState state = {};
        const bool accepted = settings::writewaterrefraction(enabled, state);
        const bool available = accepted ||
            settings::readwatersurfaceeffects(state);
        presentwatersurfaceeffects(state, available);
        if(!accepted && available)
        {
            water_surface_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("water_surface_status");
        }
    }

    void RmlSettings::selectwaterreflections(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !water_surface_state_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::WaterSurfaceEffectsState state = {};
        const bool accepted = settings::writewaterreflections(enabled, state);
        const bool available = accepted ||
            settings::readwatersurfaceeffects(state);
        presentwatersurfaceeffects(state, available);
        if(!accepted && available)
        {
            water_surface_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("water_surface_status");
        }
    }

    void RmlSettings::selectwatercaustics(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !water_surface_state_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::WaterSurfaceEffectsState state = {};
        const bool accepted = settings::writecaustics(enabled, state);
        const bool available = accepted ||
            settings::readwatersurfaceeffects(state);
        presentwatersurfaceeffects(state, available);
        if(!accepted && available)
        {
            water_surface_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("water_surface_status");
        }
    }

    void RmlSettings::selectliquidanimation(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !water_surface_state_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::WaterSurfaceEffectsState state = {};
        const bool accepted = settings::writeliquidanimation(enabled, state);
        const bool available = accepted ||
            settings::readwatersurfaceeffects(state);
        presentwatersurfaceeffects(state, available);
        if(!accepted && available)
        {
            water_surface_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("water_surface_status");
        }
    }

    void RmlSettings::refreshwaterenvironmentreflections()
    {
        settings::WaterEnvironmentReflectionsState state = {};
        const bool available =
            settings::readwaterenvironmentreflections(state);
        presentwaterenvironmentreflections(state, available);
    }

    void RmlSettings::presentwaterenvironmentreflections(
        const settings::WaterEnvironmentReflectionsState &state,
        bool available)
    {
        water_environment_reflections_available = available;
        water_environment_reflections = state.enabled;
        water_environment_reflections_status = available
            ? ""
            : "Water environment reflections state is unavailable";

        model.DirtyVariable("water_environment_reflections_available");
        model.DirtyVariable("water_environment_reflections");
        model.DirtyVariable("water_environment_reflections_status");
    }

    void RmlSettings::selectwaterenvironmentreflections(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 ||
           !water_environment_reflections_available)
        {
            return;
        }

        const bool enabled = arguments[0].Get<bool>();
        settings::WaterEnvironmentReflectionsState state = {};
        const bool accepted =
            settings::writewaterenvironmentreflections(enabled, state);
        const bool available = accepted ||
            settings::readwaterenvironmentreflections(state);
        presentwaterenvironmentreflections(state, available);
        if(!accepted && available)
        {
            water_environment_reflections_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("water_environment_reflections_status");
        }
    }

    void RmlSettings::refreshwaterrenderresolution()
    {
        settings::WaterRenderResolutionState state = {};
        const bool available = settings::readwaterrenderresolution(state);
        presentwaterrenderresolution(state, available);
    }

    void RmlSettings::presentwaterrenderresolution(
        const settings::WaterRenderResolutionState &state,
        bool available)
    {
        water_render_resolution_available = available;
        water_render_resolution_selected = -1;
        water_render_resolution_dimension.clear();
        water_render_resolution_status.clear();

        if(!available)
        {
            water_render_resolution_label = "Unavailable";
            water_render_resolution_status =
                "Water reflection/refraction resolution state is unavailable";
        }
        else
        {
            switch(state.level)
            {
                case settings::WaterRenderResolution::Minimum:
                    water_render_resolution_selected = 0;
                    water_render_resolution_label = "Minimum";
                    break;
                case settings::WaterRenderResolution::Low:
                    water_render_resolution_selected = 1;
                    water_render_resolution_label = "Low";
                    break;
                case settings::WaterRenderResolution::Medium:
                    water_render_resolution_selected = 2;
                    water_render_resolution_label = "Medium";
                    break;
                case settings::WaterRenderResolution::High:
                    water_render_resolution_selected = 3;
                    water_render_resolution_label = "High";
                    break;
                case settings::WaterRenderResolution::VeryHigh:
                    water_render_resolution_selected = 4;
                    water_render_resolution_label = "Very High";
                    break;
                case settings::WaterRenderResolution::Ultra:
                    water_render_resolution_selected = 5;
                    water_render_resolution_label = "Ultra";
                    break;
            }

            char dimension[32];
            std::snprintf(dimension, sizeof(dimension), "%dx%d",
                state.requested_dimension, state.requested_dimension);
            water_render_resolution_dimension = dimension;
        }

        model.DirtyVariable("water_render_resolution_available");
        model.DirtyVariable("water_render_resolution_selected");
        model.DirtyVariable("water_render_resolution_label");
        model.DirtyVariable("water_render_resolution_dimension");
        model.DirtyVariable("water_render_resolution_status");
    }

    void RmlSettings::selectwaterrenderresolution(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !water_render_resolution_available)
            return;

        settings::WaterRenderResolution level;
        switch(arguments[0].Get<int>())
        {
            case 0: level = settings::WaterRenderResolution::Minimum; break;
            case 1: level = settings::WaterRenderResolution::Low; break;
            case 2: level = settings::WaterRenderResolution::Medium; break;
            case 3: level = settings::WaterRenderResolution::High; break;
            case 4: level = settings::WaterRenderResolution::VeryHigh; break;
            case 5: level = settings::WaterRenderResolution::Ultra; break;
            default: return;
        }

        settings::WaterRenderResolutionState state = {};
        const bool accepted = settings::writewaterrenderresolution(
            level, state);
        const bool available = accepted ||
            settings::readwaterrenderresolution(state);
        presentwaterrenderresolution(state, available);
        if(!accepted && available)
        {
            water_render_resolution_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("water_render_resolution_status");
        }
    }

    void RmlSettings::refreshwaterfalleffects()
    {
        settings::WaterfallEffectsState state = {};
        const bool available = settings::readwaterfalleffects(state);
        presentwaterfalleffects(state, available);
    }

    void RmlSettings::presentwaterfalleffects(
        const settings::WaterfallEffectsState &state,
        bool available)
    {
        waterfall_state_available = available;
        waterfall_refraction = state.refraction;
        waterfall_refraction_available = available &&
            state.render_targets_available;
        waterfall_reflections = state.reflections;
        waterfall_status = available
            ? ""
            : "Waterfall effects state is unavailable";

        model.DirtyVariable("waterfall_state_available");
        model.DirtyVariable("waterfall_refraction");
        model.DirtyVariable("waterfall_refraction_available");
        model.DirtyVariable("waterfall_reflections");
        model.DirtyVariable("waterfall_status");
    }

    void RmlSettings::selectwaterfallrefraction(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !waterfall_state_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::WaterfallEffectsState state = {};
        const bool accepted = settings::writewaterfallrefraction(
            enabled, state);
        const bool available = accepted ||
            settings::readwaterfalleffects(state);
        presentwaterfalleffects(state, available);
        if(!accepted && available)
        {
            waterfall_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("waterfall_status");
        }
    }

    void RmlSettings::selectwaterfallreflections(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !waterfall_state_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::WaterfallEffectsState state = {};
        const bool accepted = settings::writewaterfallreflections(
            enabled, state);
        const bool available = accepted ||
            settings::readwaterfalleffects(state);
        presentwaterfalleffects(state, available);
        if(!accepted && available)
        {
            waterfall_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("waterfall_status");
        }
    }

    void RmlSettings::refreshglassreflections()
    {
        settings::GlassReflectionsState state = {};
        const bool available = settings::readglassreflections(state);
        presentglassreflections(state, available);
    }

    void RmlSettings::presentglassreflections(
        const settings::GlassReflectionsState &state,
        bool available)
    {
        glass_reflections_available = available;
        glass_reflections = state.enabled;
        glass_reflections_status = available
            ? ""
            : "Glass reflections state is unavailable";

        model.DirtyVariable("glass_reflections_available");
        model.DirtyVariable("glass_reflections");
        model.DirtyVariable("glass_reflections_status");
    }

    void RmlSettings::selectglassreflections(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !glass_reflections_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::GlassReflectionsState state = {};
        const bool accepted = settings::writeglassreflections(
            enabled, state);
        const bool available = accepted ||
            settings::readglassreflections(state);
        presentglassreflections(state, available);
        if(!accepted && available)
        {
            glass_reflections_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("glass_reflections_status");
        }
    }

    void RmlSettings::refreshmodelreflections()
    {
        settings::ModelReflectionsState state = {};
        const bool available = settings::readmodelreflections(state);
        presentmodelreflections(state, available);
    }

    void RmlSettings::presentmodelreflections(
        const settings::ModelReflectionsState &state,
        bool available)
    {
        model_reflections_available = available;
        model_reflections = state.enabled;
        model_reflections_status = available
            ? ""
            : "Model reflections state is unavailable";

        model.DirtyVariable("model_reflections_available");
        model.DirtyVariable("model_reflections");
        model.DirtyVariable("model_reflections_status");
    }

    void RmlSettings::selectmodelreflections(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !model_reflections_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::ModelReflectionsState state = {};
        const bool accepted = settings::writemodelreflections(
            enabled, state);
        const bool available = accepted ||
            settings::readmodelreflections(state);
        presentmodelreflections(state, available);
        if(!accepted && available)
        {
            model_reflections_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("model_reflections_status");
        }
    }

    void RmlSettings::refreshbumpmodels()
    {
        settings::BumpModelsState state = {};
        const bool available = settings::readbumpmodels(state);
        presentbumpmodels(state, available);
    }

    void RmlSettings::presentbumpmodels(
        const settings::BumpModelsState &state,
        bool available)
    {
        bump_models_available = available;
        bump_models = state.enabled;
        bump_models_status = available
            ? ""
            : "Model bump maps state is unavailable";

        model.DirtyVariable("bump_models_available");
        model.DirtyVariable("bump_models");
        model.DirtyVariable("bump_models_status");
    }

    void RmlSettings::selectbumpmodels(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !bump_models_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::BumpModelsState state = {};
        const bool accepted = settings::writebumpmodels(enabled, state);
        const bool available = accepted ||
            settings::readbumpmodels(state);
        presentbumpmodels(state, available);
        if(!accepted && available)
        {
            bump_models_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("bump_models_status");
        }
    }

    void RmlSettings::refreshenvironmentmapresolution()
    {
        settings::EnvironmentMapResolutionState state = {};
        const bool available =
            settings::readenvironmentmapresolution(state);
        presentenvironmentmapresolution(state, available);
    }

    void RmlSettings::presentenvironmentmapresolution(
        const settings::EnvironmentMapResolutionState &state,
        bool available)
    {
        environment_map_resolution_available = available;
        environment_map_resolution_selected = -1;
        environment_map_resolution_dimension.clear();
        environment_map_resolution_status.clear();

        if(!available)
        {
            environment_map_resolution_label = "Unavailable";
            environment_map_resolution_status =
                "Environment reflection resolution state is unavailable";
        }
        else
        {
            switch(state.level)
            {
                case settings::EnvironmentMapResolution::Minimum:
                    environment_map_resolution_selected = 0;
                    environment_map_resolution_label = "Minimum";
                    break;
                case settings::EnvironmentMapResolution::VeryLow:
                    environment_map_resolution_selected = 1;
                    environment_map_resolution_label = "Very Low";
                    break;
                case settings::EnvironmentMapResolution::Low:
                    environment_map_resolution_selected = 2;
                    environment_map_resolution_label = "Low";
                    break;
                case settings::EnvironmentMapResolution::Medium:
                    environment_map_resolution_selected = 3;
                    environment_map_resolution_label = "Medium";
                    break;
                case settings::EnvironmentMapResolution::High:
                    environment_map_resolution_selected = 4;
                    environment_map_resolution_label = "High";
                    break;
                case settings::EnvironmentMapResolution::VeryHigh:
                    environment_map_resolution_selected = 5;
                    environment_map_resolution_label = "Very High";
                    break;
                case settings::EnvironmentMapResolution::Ultra:
                    environment_map_resolution_selected = 6;
                    environment_map_resolution_label = "Ultra";
                    break;
            }

            char dimension[32];
            std::snprintf(dimension, sizeof(dimension), "%dx%d",
                state.requested_dimension, state.requested_dimension);
            environment_map_resolution_dimension = dimension;
        }

        model.DirtyVariable("environment_map_resolution_available");
        model.DirtyVariable("environment_map_resolution_selected");
        model.DirtyVariable("environment_map_resolution_label");
        model.DirtyVariable("environment_map_resolution_dimension");
        model.DirtyVariable("environment_map_resolution_status");
    }

    void RmlSettings::selectenvironmentmapresolution(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 ||
           !environment_map_resolution_available)
        {
            return;
        }

        settings::EnvironmentMapResolution level;
        switch(arguments[0].Get<int>())
        {
            case 0:
                level = settings::EnvironmentMapResolution::Minimum;
                break;
            case 1:
                level = settings::EnvironmentMapResolution::VeryLow;
                break;
            case 2:
                level = settings::EnvironmentMapResolution::Low;
                break;
            case 3:
                level = settings::EnvironmentMapResolution::Medium;
                break;
            case 4:
                level = settings::EnvironmentMapResolution::High;
                break;
            case 5:
                level = settings::EnvironmentMapResolution::VeryHigh;
                break;
            case 6:
                level = settings::EnvironmentMapResolution::Ultra;
                break;
            default:
                return;
        }

        settings::EnvironmentMapResolutionState state = {};
        const bool accepted = settings::writeenvironmentmapresolution(
            level, state);
        const bool available = accepted ||
            settings::readenvironmentmapresolution(state);
        presentenvironmentmapresolution(state, available);
        if(!accepted && available)
        {
            environment_map_resolution_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("environment_map_resolution_status");
        }
    }

    void RmlSettings::refreshdynamiclights()
    {
        settings::DynamicLightsState state = {};
        const bool available = settings::readdynamiclights(state);
        presentdynamiclights(state, available);
    }

    void RmlSettings::presentdynamiclights(
        const settings::DynamicLightsState &state,
        bool available)
    {
        dynamic_lights_available = available;
        dynamic_lights_selected = -1;
        dynamic_lights_status.clear();

        if(!available)
        {
            dynamic_lights_label = "Unavailable";
            dynamic_lights_status = "Dynamic lights state is unavailable";
        }
        else switch(state.setting)
        {
            case settings::DynamicLights::Off:
                dynamic_lights_selected = 0;
                dynamic_lights_label = "Off";
                break;
            case settings::DynamicLights::Medium:
                dynamic_lights_selected = 1;
                dynamic_lights_label = "Medium";
                break;
            case settings::DynamicLights::High:
                dynamic_lights_selected = 2;
                dynamic_lights_label = "High";
                break;
            case settings::DynamicLights::Custom:
            {
                dynamic_lights_label = "Custom";
                char status[64];
                std::snprintf(status, sizeof(status),
                    "World-surface light limit: %d", state.max_lights);
                dynamic_lights_status = status;
                break;
            }
        }

        model.DirtyVariable("dynamic_lights_available");
        model.DirtyVariable("dynamic_lights_selected");
        model.DirtyVariable("dynamic_lights_label");
        model.DirtyVariable("dynamic_lights_status");
    }

    void RmlSettings::selectdynamiclights(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !dynamic_lights_available)
            return;

        settings::DynamicLights setting;
        switch(arguments[0].Get<int>())
        {
            case 0: setting = settings::DynamicLights::Off; break;
            case 1: setting = settings::DynamicLights::Medium; break;
            case 2: setting = settings::DynamicLights::High; break;
            default: return;
        }

        settings::DynamicLightsState state = {};
        const bool accepted = settings::writedynamiclights(setting, state);
        const bool available = accepted ||
            settings::readdynamiclights(state);
        presentdynamiclights(state, available);
        if(!accepted && available)
        {
            if(dynamic_lights_status.empty())
            {
                dynamic_lights_status =
                    "Change not accepted; current engine state is shown";
            }
            else
            {
                dynamic_lights_status = "Change not accepted. " +
                    dynamic_lights_status;
            }
            model.DirtyVariable("dynamic_lights_status");
        }
    }

    void RmlSettings::refreshsoftparticles()
    {
        settings::SoftParticlesState state = {};
        const bool available = settings::readsoftparticles(state);
        presentsoftparticles(state, available);
    }

    void RmlSettings::presentsoftparticles(
        const settings::SoftParticlesState &state,
        bool available)
    {
        soft_particles_available = available;
        soft_particles = state.enabled;
        soft_particles_status.clear();
        if(!available)
            soft_particles_status = "Soft particles state is unavailable";

        model.DirtyVariable("soft_particles_available");
        model.DirtyVariable("soft_particles");
        model.DirtyVariable("soft_particles_status");
    }

    void RmlSettings::selectsoftparticles(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !soft_particles_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::SoftParticlesState state = {};
        const bool accepted = settings::writesoftparticles(enabled, state);
        const bool available = accepted || settings::readsoftparticles(state);
        presentsoftparticles(state, available);
        if(!accepted && available)
        {
            soft_particles_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("soft_particles_status");
        }
    }

    void RmlSettings::refreshsoftparticlequality()
    {
        settings::SoftParticleQualityState state = {};
        const bool available = settings::readsoftparticlequality(state);
        presentsoftparticlequality(state, available);
    }

    void RmlSettings::presentsoftparticlequality(
        const settings::SoftParticleQualityState &state,
        bool available)
    {
        soft_particle_quality_available = available;
        soft_particle_quality_selected = -1;
        soft_particle_quality_status.clear();

        if(!available)
        {
            soft_particle_quality_label = "Unavailable";
            soft_particle_quality_status =
                "Soft particle quality state is unavailable";
        }
        else switch(state.quality)
        {
            case settings::SoftParticleQuality::Low:
                soft_particle_quality_selected = 0;
                soft_particle_quality_label = "Low";
                break;
            case settings::SoftParticleQuality::Medium:
                soft_particle_quality_selected = 1;
                soft_particle_quality_label = "Medium";
                break;
            case settings::SoftParticleQuality::High:
                soft_particle_quality_selected = 2;
                soft_particle_quality_label = "High";
                break;
            case settings::SoftParticleQuality::Custom:
            {
                soft_particle_quality_label = "Custom";
                char status[96];
                std::snprintf(status, sizeof(status),
                    "Depth target level: %d · Blur radius: %d",
                    state.depth_size, state.blur);
                soft_particle_quality_status = status;
                break;
            }
        }

        model.DirtyVariable("soft_particle_quality_available");
        model.DirtyVariable("soft_particle_quality_selected");
        model.DirtyVariable("soft_particle_quality_label");
        model.DirtyVariable("soft_particle_quality_status");
    }

    void RmlSettings::selectsoftparticlequality(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !soft_particle_quality_available)
            return;

        settings::SoftParticleQuality quality;
        switch(arguments[0].Get<int>())
        {
            case 0: quality = settings::SoftParticleQuality::Low; break;
            case 1: quality = settings::SoftParticleQuality::Medium; break;
            case 2: quality = settings::SoftParticleQuality::High; break;
            default: return;
        }

        settings::SoftParticleQualityState state = {};
        const bool accepted = settings::writesoftparticlequality(
            quality, state);
        const bool available = accepted ||
            settings::readsoftparticlequality(state);
        presentsoftparticlequality(state, available);
        if(!accepted && available)
        {
            if(soft_particle_quality_status.empty())
            {
                soft_particle_quality_status =
                    "Change not accepted; current engine state is shown";
            }
            else
            {
                soft_particle_quality_status = "Change not accepted. " +
                    soft_particle_quality_status;
            }
            model.DirtyVariable("soft_particle_quality_status");
        }
    }

    void RmlSettings::refreshdecals()
    {
        settings::DecalsState state = {};
        const bool available = settings::readdecals(state);
        presentdecals(state, available);
    }

    void RmlSettings::presentdecals(
        const settings::DecalsState &state,
        bool available)
    {
        decals_available = available;
        decals = state.enabled;
        decals_status.clear();
        if(!available)
            decals_status = "Decals state is unavailable";

        model.DirtyVariable("decals_available");
        model.DirtyVariable("decals");
        model.DirtyVariable("decals_status");
    }

    void RmlSettings::selectdecals(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !decals_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::DecalsState state = {};
        const bool accepted = settings::writedecals(enabled, state);
        const bool available = accepted || settings::readdecals(state);
        presentdecals(state, available);
        if(!accepted && available)
        {
            decals_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("decals_status");
        }
    }

    void RmlSettings::refreshdecalpersistence()
    {
        settings::DecalPersistenceState state = {};
        const bool available = settings::readdecalpersistence(state);
        presentdecalpersistence(state, available);
    }

    void RmlSettings::presentdecalpersistence(
        const settings::DecalPersistenceState &state,
        bool available)
    {
        decal_persistence_available = available;
        decal_persistence_selected = -1;
        decal_persistence_status.clear();

        if(!available)
        {
            decal_persistence_label = "Unavailable";
            decal_persistence_status =
                "Decal Persistence state is unavailable";
        }
        else switch(state.persistence)
        {
            case settings::DecalPersistence::Short:
                decal_persistence_selected = 0;
                decal_persistence_label = "Short";
                break;
            case settings::DecalPersistence::Long:
                decal_persistence_selected = 1;
                decal_persistence_label = "Long";
                break;
            case settings::DecalPersistence::Custom:
            {
                decal_persistence_label = "Custom";
                char status[128];
                std::snprintf(status, sizeof(status),
                    "Fade start: %d ms · Triangle budget per type: %d",
                    state.fade_start_milliseconds,
                    state.triangle_budget_per_type);
                decal_persistence_status = status;
                break;
            }
        }

        model.DirtyVariable("decal_persistence_available");
        model.DirtyVariable("decal_persistence_selected");
        model.DirtyVariable("decal_persistence_label");
        model.DirtyVariable("decal_persistence_status");
    }

    void RmlSettings::selectdecalpersistence(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !decal_persistence_available)
            return;

        settings::DecalPersistence persistence;
        switch(arguments[0].Get<int>())
        {
            case 0: persistence = settings::DecalPersistence::Short; break;
            case 1: persistence = settings::DecalPersistence::Long; break;
            default: return;
        }

        settings::DecalPersistenceState state = {};
        const bool accepted = settings::writedecalpersistence(
            persistence, state);
        const bool available = accepted ||
            settings::readdecalpersistence(state);
        presentdecalpersistence(state, available);
        if(!accepted && available)
        {
            const Rml::String rejected =
                "Change not accepted; current engine state is shown";
            if(decal_persistence_status.empty())
            {
                decal_persistence_status = rejected;
            }
            else
            {
                decal_persistence_status = rejected + " · " +
                    decal_persistence_status;
            }
            model.DirtyVariable("decal_persistence_status");
        }
    }

    void RmlSettings::refreshfixtjoints()
    {
        settings::FixTJointsState state = {};
        const bool available = settings::readfixtjoints(state);
        presentfixtjoints(state, available);
    }

    void RmlSettings::presentfixtjoints(
        const settings::FixTJointsState &state,
        bool available)
    {
        fix_t_joints_available = available;
        fix_t_joints = state.enabled;
        fix_t_joints_status = available
            ? ""
            : "Fix T-Joints state is unavailable";

        model.DirtyVariable("fix_t_joints_available");
        model.DirtyVariable("fix_t_joints");
        model.DirtyVariable("fix_t_joints_status");
    }

    void RmlSettings::selectfixtjoints(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !fix_t_joints_available)
            return;

        const bool enabled = arguments[0].Get<bool>();
        settings::FixTJointsState state = {};
        const bool accepted = settings::writefixtjoints(enabled, state);
        const bool available = accepted || settings::readfixtjoints(state);
        presentfixtjoints(state, available);
        if(!accepted && available)
        {
            fix_t_joints_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("fix_t_joints_status");
        }
    }

    void RmlSettings::grassaction(Rml::Event &event)
    {
        event.StopPropagation();
        if(!instance)
            return;

        settings::GraphicsQuality current;
        const bool accepted = settings::readgraphicsquality(current) &&
            settings::writegrass(!current.grass);
        const bool refreshed = instance->refreshgraphicsquality();
        if(!accepted && refreshed)
        {
            instance->graphics_quality_status = "Grass change was not accepted";
            instance->model.DirtyVariable("graphics_quality_status");
        }
    }

    void RmlSettings::motionbluraction(Rml::Event &event)
    {
        event.StopPropagation();
        if(!instance)
            return;

        settings::GraphicsQuality current;
        const bool accepted = settings::readgraphicsquality(current) &&
            settings::writemotionblur(!current.motion_blur);
        const bool refreshed = instance->refreshgraphicsquality();
        if(!accepted && refreshed)
        {
            instance->graphics_quality_status = "Motion blur change was not accepted";
            instance->model.DirtyVariable("graphics_quality_status");
        }
    }

    void RmlSettings::selectglare(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1)
            return;

        const bool enabled = arguments[0].Get<bool>();
        const bool accepted = settings::writeglare(enabled);
        const bool refreshed = refreshgraphicsquality();
        if(!accepted && refreshed)
        {
            graphics_quality_status = "Glare change was not accepted";
            model.DirtyVariable("graphics_quality_status");
        }
    }

    void RmlSettings::refreshgrassdistance()
    {
        settings::GrassDistanceState state = {};
        const bool available = settings::readgrassdistance(state);
        presentgrassdistance(state, available);
    }

    void RmlSettings::presentgrassdistance(
        const settings::GrassDistanceState &state,
        bool available)
    {
        grass_distance_available = available;
        grass_distance_selected = -1;
        grass_distance_status.clear();

        if(!available)
        {
            grass_distance_label = "Unavailable";
            grass_distance_status = "Grass distance state is unavailable";
        }
        else switch(state.distance)
        {
            case settings::GrassDistance::Quick:
                grass_distance_selected = 0;
                grass_distance_label = "Quick";
                break;
            case settings::GrassDistance::Moderate:
                grass_distance_selected = 1;
                grass_distance_label = "Moderate";
                break;
            case settings::GrassDistance::Slow:
                grass_distance_selected = 2;
                grass_distance_label = "Slow";
                break;
            case settings::GrassDistance::Custom:
            {
                grass_distance_label = "Custom";
                char status[64];
                std::snprintf(status, sizeof(status),
                    "Distance %d", state.value);
                grass_distance_status = status;
                break;
            }
        }

        model.DirtyVariable("grass_distance_available");
        model.DirtyVariable("grass_distance_selected");
        model.DirtyVariable("grass_distance_label");
        model.DirtyVariable("grass_distance_status");
    }

    void RmlSettings::selectgrassdistance(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !grass_distance_available)
            return;

        settings::GrassDistance distance;
        switch(arguments[0].Get<int>())
        {
            case 0: distance = settings::GrassDistance::Quick; break;
            case 1: distance = settings::GrassDistance::Moderate; break;
            case 2: distance = settings::GrassDistance::Slow; break;
            default: return;
        }

        settings::GrassDistanceState state = {};
        const bool accepted = settings::writegrassdistance(distance, state);
        const bool available = accepted || settings::readgrassdistance(state);
        presentgrassdistance(state, available);
        if(!accepted && available)
        {
            if(grass_distance_status.empty())
            {
                grass_distance_status =
                    "Change not accepted; current engine state is shown";
            }
            else
            {
                grass_distance_status = "Change not accepted. " +
                    grass_distance_status;
            }
            model.DirtyVariable("grass_distance_status");
        }
    }

    void RmlSettings::refreshmotionblurintensity()
    {
        settings::MotionBlurIntensityState state = {};
        const bool available = settings::readmotionblurintensity(state);
        presentmotionblurintensity(state, available);
    }

    void RmlSettings::presentmotionblurintensity(
        const settings::MotionBlurIntensityState &state,
        bool available)
    {
        motion_blur_intensity_available = available;
        motion_blur_intensity_selected = -1;
        motion_blur_intensity_status.clear();

        if(!available)
        {
            motion_blur_intensity_label = "Unavailable";
            motion_blur_intensity_status =
                "Motion blur intensity state is unavailable";
        }
        else switch(state.intensity)
        {
            case settings::MotionBlurIntensity::Subtle:
                motion_blur_intensity_selected = 0;
                motion_blur_intensity_label = "Subtle";
                break;
            case settings::MotionBlurIntensity::Moderate:
                motion_blur_intensity_selected = 1;
                motion_blur_intensity_label = "Moderate";
                break;
            case settings::MotionBlurIntensity::Intense:
                motion_blur_intensity_selected = 2;
                motion_blur_intensity_label = "Intense";
                break;
            case settings::MotionBlurIntensity::Custom:
            {
                motion_blur_intensity_label = "Custom";
                char status[64];
                std::snprintf(status, sizeof(status),
                    "Scale %.9g", double(state.scale));
                motion_blur_intensity_status = status;
                break;
            }
        }

        model.DirtyVariable("motion_blur_intensity_available");
        model.DirtyVariable("motion_blur_intensity_selected");
        model.DirtyVariable("motion_blur_intensity_label");
        model.DirtyVariable("motion_blur_intensity_status");
    }

    void RmlSettings::selectmotionblurintensity(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !motion_blur_intensity_available)
            return;

        settings::MotionBlurIntensity intensity;
        switch(arguments[0].Get<int>())
        {
            case 0: intensity = settings::MotionBlurIntensity::Subtle; break;
            case 1: intensity = settings::MotionBlurIntensity::Moderate; break;
            case 2: intensity = settings::MotionBlurIntensity::Intense; break;
            default: return;
        }

        settings::MotionBlurIntensityState state = {};
        const bool accepted = settings::writemotionblurintensity(
            intensity, state);
        const bool available = accepted ||
            settings::readmotionblurintensity(state);
        presentmotionblurintensity(state, available);
        if(!accepted && available)
        {
            if(motion_blur_intensity_status.empty())
            {
                motion_blur_intensity_status =
                    "Change not accepted; current engine state is shown";
            }
            else
            {
                motion_blur_intensity_status = "Change not accepted. " +
                    motion_blur_intensity_status;
            }
            model.DirtyVariable("motion_blur_intensity_status");
        }
    }

    void RmlSettings::refreshglarepreset()
    {
        settings::GlarePresetState state = {};
        const bool available = settings::readglarepreset(state);
        presentglarepreset(state, available);
    }

    void RmlSettings::presentglarepreset(
        const settings::GlarePresetState &state,
        bool available)
    {
        glare_preset_available = available;
        glare_preset_selected = -1;
        glare_preset_status.clear();

        if(!available)
        {
            glare_preset_label = "Unavailable";
            glare_preset_status = "Glare preset state is unavailable";
        }
        else switch(state.preset)
        {
            case settings::GlarePreset::Subtle:
                glare_preset_selected = 0;
                glare_preset_label = "Subtle";
                break;
            case settings::GlarePreset::Glowy:
                glare_preset_selected = 1;
                glare_preset_label = "Glowy";
                break;
            case settings::GlarePreset::Soft:
                glare_preset_selected = 2;
                glare_preset_label = "Soft";
                break;
            case settings::GlarePreset::Intense:
                glare_preset_selected = 3;
                glare_preset_label = "Intense";
                break;
            case settings::GlarePreset::Custom:
            {
                glare_preset_label = "Custom";
                char status[96];
                std::snprintf(status, sizeof(status),
                    "Scale %g | Blur %d",
                    double(state.scale), state.blur_radius);
                glare_preset_status = status;
                break;
            }
        }

        model.DirtyVariable("glare_preset_available");
        model.DirtyVariable("glare_preset_selected");
        model.DirtyVariable("glare_preset_label");
        model.DirtyVariable("glare_preset_status");
    }

    void RmlSettings::selectglarepreset(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !glare_preset_available)
            return;

        settings::GlarePreset preset;
        switch(arguments[0].Get<int>())
        {
            case 0: preset = settings::GlarePreset::Subtle; break;
            case 1: preset = settings::GlarePreset::Glowy; break;
            case 2: preset = settings::GlarePreset::Soft; break;
            case 3: preset = settings::GlarePreset::Intense; break;
            default: return;
        }

        settings::GlarePresetState state = {};
        const bool accepted = settings::writeglarepreset(preset, state);
        const bool available = accepted || settings::readglarepreset(state);
        presentglarepreset(state, available);
        if(!accepted && available)
        {
            if(glare_preset_status.empty())
                glare_preset_status = "Change not accepted; current engine state is shown";
            else glare_preset_status = "Change not accepted. " + glare_preset_status;
            model.DirtyVariable("glare_preset_status");
        }
        refreshglareintensity();
    }

    void RmlSettings::refreshglareintensity()
    {
        settings::GlareIntensityState state = {};
        const bool available = settings::readglareintensity(state);
        presentglareintensity(state, available);
    }

    void RmlSettings::presentglareintensity(
        const settings::GlareIntensityState &state,
        bool available)
    {
        glare_intensity_available = available;
        glare_intensity_status.clear();

        if(!available)
        {
            glare_intensity_slider = 0.0f;
            glare_intensity_label = "Unavailable";
            glare_intensity_status = "Glare Intensity state is unavailable";
        }
        else
        {
            const float step = 0.1f;
            glare_intensity_slider =
                std::round(state.configured_scale / step) * step;

            char label[32];
            std::snprintf(label, sizeof(label), "%.6gx",
                double(state.configured_scale));
            glare_intensity_label = label;
        }

        model.DirtyVariable("glare_intensity_available");
        model.DirtyVariable("glare_intensity_slider");
        model.DirtyVariable("glare_intensity_label");
        model.DirtyVariable("glare_intensity_status");
    }

    void RmlSettings::changeglareintensityaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = event.GetParameter<float>(
            "value", instance->glare_intensity_slider);

        // RmlUi snaps programmatic range values to the declared step and
        // emits change. Ignore that presentation event so an off-step native
        // value remains authoritative until the user moves the slider.
        if(requested == instance->glare_intensity_slider)
        {
            event.StopPropagation();
            return;
        }

        settings::GlareIntensityState state = {};
        const bool accepted = settings::writeglareintensity(requested, state);
        const bool available = accepted ||
            settings::readglareintensity(state);
        instance->presentglareintensity(state, available);
        instance->refreshglarepreset();
        if(!accepted && available)
        {
            instance->glare_intensity_status =
                "Change not accepted; current engine state is shown";
            instance->model.DirtyVariable("glare_intensity_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::refreshglareresolution()
    {
        settings::GlareResolutionState state = {};
        const bool available = settings::readglareresolution(state);
        presentglareresolution(state, available);
    }

    void RmlSettings::presentglareresolution(
        const settings::GlareResolutionState &state,
        bool available)
    {
        glare_resolution_available = available;
        glare_resolution_selected = -1;
        glare_resolution_requested_width = 0;
        glare_resolution_status.clear();

        if(!available)
        {
            glare_resolution_label = "Unavailable";
            glare_resolution_status =
                "Glare render resolution state is unavailable";
        }
        else
        {
            switch(state.level)
            {
                case settings::GlareResolution::Minimum:
                    glare_resolution_selected = 0;
                    glare_resolution_label = "Minimum";
                    break;
                case settings::GlareResolution::Low:
                    glare_resolution_selected = 1;
                    glare_resolution_label = "Low";
                    break;
                case settings::GlareResolution::Medium:
                    glare_resolution_selected = 2;
                    glare_resolution_label = "Medium";
                    break;
                case settings::GlareResolution::High:
                    glare_resolution_selected = 3;
                    glare_resolution_label = "High";
                    break;
                case settings::GlareResolution::Ultra:
                    glare_resolution_selected = 4;
                    glare_resolution_label = "Ultra";
                    break;
            }
            glare_resolution_requested_width = state.requested_width;
        }

        model.DirtyVariable("glare_resolution_available");
        model.DirtyVariable("glare_resolution_selected");
        model.DirtyVariable("glare_resolution_label");
        model.DirtyVariable("glare_resolution_requested_width");
        model.DirtyVariable("glare_resolution_status");
    }

    void RmlSettings::selectglareresolution(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !glare_resolution_available)
            return;

        settings::GlareResolution level;
        switch(arguments[0].Get<int>())
        {
            case 0: level = settings::GlareResolution::Minimum; break;
            case 1: level = settings::GlareResolution::Low; break;
            case 2: level = settings::GlareResolution::Medium; break;
            case 3: level = settings::GlareResolution::High; break;
            case 4: level = settings::GlareResolution::Ultra; break;
            default: return;
        }

        settings::GlareResolutionState state = {};
        const bool accepted = settings::writeglareresolution(level, state);
        const bool available = accepted ||
            settings::readglareresolution(state);
        presentglareresolution(state, available);
        if(!accepted && available)
        {
            glare_resolution_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("glare_resolution_status");
        }
    }

    void RmlSettings::refreshtexturequality()
    {
        settings::TextureQualityState state = {};
        const bool available = settings::readtexturequality(state);
        presenttexturequality(state, available);
    }

    void RmlSettings::presenttexturequality(
        const settings::TextureQualityState &state,
        bool available)
    {
        texture_quality_available = available;
        texture_quality_selected = -1;
        texture_quality_status.clear();

        if(!available)
        {
            texture_quality_label = "Unavailable";
            texture_quality_status =
                "Texture Quality state is unavailable";
        }
        else switch(state.quality)
        {
            case settings::TextureQuality::Low:
                texture_quality_selected = 0;
                texture_quality_label = "Low";
                break;
            case settings::TextureQuality::Medium:
                texture_quality_selected = 1;
                texture_quality_label = "Medium";
                break;
            case settings::TextureQuality::Full:
                texture_quality_selected = 2;
                texture_quality_label = "Full";
                break;
            case settings::TextureQuality::Custom:
            {
                texture_quality_label = "Custom";
                char status[64];
                std::snprintf(status, sizeof(status),
                    "Requested limit: %d px", state.texture_size_limit);
                texture_quality_status = status;
                break;
            }
        }

        model.DirtyVariable("texture_quality_available");
        model.DirtyVariable("texture_quality_selected");
        model.DirtyVariable("texture_quality_label");
        model.DirtyVariable("texture_quality_status");
    }

    void RmlSettings::selecttexturequality(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !texture_quality_available)
            return;

        settings::TextureQuality quality;
        switch(arguments[0].Get<int>())
        {
            case 0: quality = settings::TextureQuality::Low; break;
            case 1: quality = settings::TextureQuality::Medium; break;
            case 2: quality = settings::TextureQuality::Full; break;
            default: return;
        }

        settings::TextureQualityState state = {};
        const bool accepted = settings::writetexturequality(quality, state);
        const bool available = accepted ||
            settings::readtexturequality(state);
        presenttexturequality(state, available);
        if(!accepted && available)
        {
            const Rml::String rejected =
                "Change not accepted; current engine state is shown";
            if(texture_quality_status.empty())
            {
                texture_quality_status = rejected;
            }
            else
            {
                texture_quality_status = rejected + " · " +
                    texture_quality_status;
            }
            model.DirtyVariable("texture_quality_status");
        }
    }

    void RmlSettings::refreshtexturereduction()
    {
        settings::TextureReductionState state = {};
        const bool available = settings::readtexturereduction(state);
        presenttexturereduction(state, available);
    }

    void RmlSettings::presenttexturereduction(
        const settings::TextureReductionState &state,
        bool available)
    {
        texture_reduction_available = available;
        texture_reduction_status.clear();

        if(!available)
        {
            texture_reduction_label = "Unavailable";
            texture_reduction_status =
                "Texture Reduction state is unavailable";
        }
        else
        {
            texture_reduction = state.levels;
            texture_reduction_minimum = state.minimum;
            texture_reduction_maximum = state.maximum;
            if(!state.levels)
            {
                texture_reduction_label = "Off";
            }
            else
            {
                char label[64];
                std::snprintf(label, sizeof(label),
                    "1/%d width & height", 1 << state.levels);
                texture_reduction_label = label;
            }
        }

        model.DirtyVariable("texture_reduction");
        model.DirtyVariable("texture_reduction_minimum");
        model.DirtyVariable("texture_reduction_maximum");
        model.DirtyVariable("texture_reduction_available");
        model.DirtyVariable("texture_reduction_label");
        model.DirtyVariable("texture_reduction_status");
    }

    void RmlSettings::changetexturereductionaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->texture_reduction
        );
        settings::TextureReductionState state = {};
        const bool accepted = settings::writetexturereduction(
            requested, state);
        const bool available = accepted ||
            settings::readtexturereduction(state);
        instance->presenttexturereduction(state, available);
        if(!accepted && available)
        {
            instance->texture_reduction_status =
                "Change not accepted; current engine state is shown";
            instance->model.DirtyVariable("texture_reduction_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::refreshtexturecompression()
    {
        settings::TextureCompressionState state = {};
        const bool available = settings::readtexturecompression(state);
        presenttexturecompression(state, available);
    }

    void RmlSettings::presenttexturecompression(
        const settings::TextureCompressionState &state,
        bool available)
    {
        using Preset = settings::TextureCompressionPreset;
        texture_compression_available = available;
        texture_compression_supported = available && state.supported;
        texture_compression_selected = -1;
        texture_compression_status.clear();

        if(!available)
        {
            texture_compression_label = "Unavailable";
            texture_compression_status =
                "Automatic Texture Compression state is unavailable";
        }
        else
        {
            switch(state.preset)
            {
                case Preset::Off:
                    texture_compression_selected = 0;
                    texture_compression_label = "Off";
                    break;
                case Preset::AllEligible:
                    texture_compression_selected = 1;
                    texture_compression_label = "All eligible textures";
                    break;
                case Preset::AtLeast256:
                    texture_compression_selected = 2;
                    texture_compression_label = "256 px and larger";
                    break;
                case Preset::AtLeast512:
                    texture_compression_selected = 3;
                    texture_compression_label = "512 px and larger";
                    break;
                case Preset::AtLeast1024:
                    texture_compression_selected = 4;
                    texture_compression_label = "1024 px and larger";
                    break;
                case Preset::Above1024:
                    texture_compression_selected = 5;
                    texture_compression_label = "Above 1024 px";
                    break;
                case Preset::AtLeast2048:
                    texture_compression_selected = 6;
                    texture_compression_label = "2048 px and larger";
                    break;
                case Preset::AtLeast4096:
                    texture_compression_selected = 7;
                    texture_compression_label = "4096 px and larger";
                    break;
                case Preset::Custom:
                {
                    char label[64];
                    std::snprintf(label, sizeof(label),
                        "Custom: %d px and larger", state.threshold);
                    texture_compression_label = label;
                    break;
                }
            }

            if(!state.supported)
            {
                texture_compression_status =
                    "Automatic texture compression is unavailable on the current renderer; the configured threshold is retained.";
            }
        }

        model.DirtyVariable("texture_compression_selected");
        model.DirtyVariable("texture_compression_available");
        model.DirtyVariable("texture_compression_supported");
        model.DirtyVariable("texture_compression_label");
        model.DirtyVariable("texture_compression_status");
    }

    void RmlSettings::selecttexturecompression(
        Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        event.StopPropagation();
        if(arguments.size() != 1 || !texture_compression_available ||
           !texture_compression_supported)
        {
            return;
        }

        int threshold = 0;
        switch(arguments[0].Get<int>())
        {
            case 0: threshold = 0; break;
            case 1: threshold = 1; break;
            case 2: threshold = 256; break;
            case 3: threshold = 512; break;
            case 4: threshold = 1024; break;
            case 5: threshold = 1025; break;
            case 6: threshold = 2048; break;
            case 7: threshold = 4096; break;
            default: return;
        }

        settings::TextureCompressionState state = {};
        const bool accepted = settings::writetexturecompression(
            threshold, state);
        const bool available = accepted ||
            settings::readtexturecompression(state);
        presenttexturecompression(state, available);
        if(!accepted && available && state.supported)
        {
            texture_compression_status =
                "Change not accepted; current engine state is shown";
            model.DirtyVariable("texture_compression_status");
        }
    }

    void RmlSettings::vsyncaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const bool requested = !instance->vsync;
        const bool accepted = settings::writevsync(requested);
        instance->refreshvideo();
        if(!accepted)
        {
            instance->vsync_status =
                "Driver rejected the VSync change";
            instance->model.DirtyVariable("vsync_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::refreshgamma()
    {
        settings::Gamma state = {};
        const bool available = settings::readgamma(state);
        presentgamma(state, available);
    }

    void RmlSettings::presentgamma(const settings::Gamma &state, bool available)
    {
        const int requested = available ? state.requested : 100;
        const int minimum = available ? state.minimum : 30;
        const int maximum = available ? state.maximum : 300;

        if(gamma != requested)
        {
            gamma = requested;
            model.DirtyVariable("gamma");
        }
        if(gamma_minimum != minimum)
        {
            gamma_minimum = minimum;
            model.DirtyVariable("gamma_minimum");
        }
        if(gamma_maximum != maximum)
        {
            gamma_maximum = maximum;
            model.DirtyVariable("gamma_maximum");
        }

        char value_label[32];
        if(available)
        {
            std::snprintf(
                value_label,
                sizeof(value_label),
                "%d",
                state.requested
            );
        }
        else std::snprintf(value_label, sizeof(value_label), "Unavailable");

        if(gamma_label != value_label)
        {
            gamma_label = value_label;
            model.DirtyVariable("gamma_label");
        }

        Rml::String status;
        if(!available)
            status = "Gamma state is unavailable";
        else if(state.requested != state.effective)
        {
            char text[96];
            std::snprintf(
                text,
                sizeof(text),
                "Requested %d; display reports %d",
                state.requested,
                state.effective
            );
            status = text;
        }
        else status = "Immediate window brightness";

        if(gamma_status != status)
        {
            gamma_status = status;
            model.DirtyVariable("gamma_status");
        }
    }

    void RmlSettings::changegammaaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->gamma
        );

        // Ignore programmatic range updates so opening Settings cannot
        // rewrite the native ident. RmlUi emits change for the RML default
        // (100) and min/max clamps during InstanceElement, before the range
        // is parented.
        if(requested == instance->gamma)
        {
            event.StopPropagation();
            return;
        }

        Rml::Element *element = event.GetCurrentElement();
        if(!element || !element->GetParentNode())
        {
            event.StopPropagation();
            return;
        }

        settings::Gamma state = {};
        const bool accepted = settings::writegamma(requested, state);
        const bool available = accepted || settings::readgamma(state);
        instance->presentgamma(state, available);
        if(!accepted && available)
        {
            instance->gamma_status =
                "Change not accepted; current engine state is shown";
            instance->model.DirtyVariable("gamma_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::changedeadpushaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->deadpush
        );

        // Ignore programmatic range updates so opening Settings cannot
        // rewrite the native ident. RmlUi emits change for min/max clamps
        // during InstanceElement, before the range is parented. There is
        // no hardcoded RML value default.
        if(requested == instance->deadpush)
        {
            event.StopPropagation();
            return;
        }

        Rml::Element *element = event.GetCurrentElement();
        if(!element || !element->GetParentNode())
        {
            event.StopPropagation();
            return;
        }

        if(!instance->game_models_available || !instance->ragdoll_deaths)
        {
            event.StopPropagation();
            return;
        }

        settings::GameModels state = {};
        const bool accepted = settings::writedeadpush(requested, state);
        const bool available = accepted ||
            settings::readgamemodels(state);
        instance->presentgamemodels(state, available);
        if(!accepted && available)
        {
            instance->game_models_status =
                "Change not accepted; current engine state is shown";
            instance->model.DirtyVariable("game_models_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::changeammobariconspacingaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->ammobar_icon_spacing
        );

        // Ignore programmatic range updates so opening Settings or revealing
        // the data-if layout cannot rewrite the native ident. RmlUi emits
        // change for an RML default during InstanceElement. Do not arm until
        // the bound engine value has been applied.
        if(!instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }
        if(!instance->ammobar_icon_spacing_armed)
        {
            if(requested == instance->ammobar_icon_spacing)
                instance->ammobar_icon_spacing_armed = true;
            event.StopPropagation();
            return;
        }
        if(requested == instance->ammobar_icon_spacing)
        {
            event.StopPropagation();
            return;
        }

        Rml::Element *element = event.GetCurrentElement();
        if(!element || !element->GetParentNode())
        {
            event.StopPropagation();
            return;
        }

        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarsep(requested, state);
        instance->finishammobarwrite(accepted, state);
        event.StopPropagation();
    }

    void RmlSettings::changeammobarlabeloffsetaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->ammobar_label_offset
        );

        if(!instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }
        if(!instance->ammobar_label_offset_armed)
        {
            if(requested == instance->ammobar_label_offset)
                instance->ammobar_label_offset_armed = true;
            event.StopPropagation();
            return;
        }
        if(requested == instance->ammobar_label_offset)
        {
            event.StopPropagation();
            return;
        }

        Rml::Element *element = event.GetCurrentElement();
        if(!element || !element->GetParentNode())
        {
            event.StopPropagation();
            return;
        }

        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarcountsep(requested, state);
        instance->finishammobarwrite(accepted, state);
        event.StopPropagation();
    }

    void RmlSettings::changefpslimitaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->fps_limit
        );

        if(!settings::writefpslimit(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected the FPS limit %d\n",
                requested
            );
        }

        instance->refreshvideo();
        event.StopPropagation();
    }

    void RmlSettings::changesoundvolumeaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->sound_volume
        );
        if(!settings::writesoundvolume(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected the sound volume %d\n",
                requested
            );
        }

        instance->refreshaudio();
        event.StopPropagation();
    }

    void RmlSettings::changemusicvolumeaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->music_volume
        );
        if(!settings::writemusicvolume(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected the music volume %d\n",
                requested
            );
        }

        instance->refreshaudio();
        event.StopPropagation();
    }

    void RmlSettings::decreasesensitivityaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::max(
            instance->mouse_sensitivity_minimum,
            stepsensitivity(instance->mouse_sensitivity, -1)
        );
        if(!settings::writesensitivity(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected mouse sensitivity %.3f\n",
                requested
            );
        }

        instance->mouse_sensitivity_invalid = false;
        instance->model.DirtyVariable("mouse_sensitivity_invalid");
        instance->mouse_sensitivity_status = "Adjusted by 0.01";
        instance->model.DirtyVariable("mouse_sensitivity_status");
        instance->refreshmouseinput(true);
        event.StopPropagation();
    }

    void RmlSettings::increasesensitivityaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::min(
            instance->mouse_sensitivity_maximum,
            stepsensitivity(instance->mouse_sensitivity, 1)
        );
        if(!settings::writesensitivity(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected mouse sensitivity %.3f\n",
                requested
            );
        }

        instance->mouse_sensitivity_invalid = false;
        instance->model.DirtyVariable("mouse_sensitivity_invalid");
        instance->mouse_sensitivity_status = "Adjusted by 0.01";
        instance->model.DirtyVariable("mouse_sensitivity_status");
        instance->refreshmouseinput(true);
        event.StopPropagation();
    }

    void RmlSettings::focussensitivityaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->mouse_sensitivity_editing = true;
            if(instance->mouse_sensitivity_invalid)
            {
                instance->mouse_sensitivity_invalid = false;
                instance->model.DirtyVariable("mouse_sensitivity_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blursensitivityaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitsensitivity();
            instance->mouse_sensitivity_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitsensitivityaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitsensitivity();
            event.StopPropagation();
        }
    }

    void RmlSettings::invertedmouseaction(Rml::Event &event)
    {
        if(!instance)
            return;

        if(!settings::writeinvertedmouse(!instance->inverted_mouse))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected inverted mouse change\n"
            );
        }

        instance->refreshmouseinput();
        event.StopPropagation();
    }

    void RmlSettings::decreasecrosshairsizeaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = std::max(
            instance->crosshair_size_minimum,
            instance->crosshair_size - 1
        );
        const bool accepted = settings::writecrosshairsize(requested);
        instance->crosshair_size_invalid = false;
        instance->model.DirtyVariable("crosshair_size_invalid");
        instance->crosshair_status = accepted
            ? "Crosshair size adjusted by 1"
            : "Crosshair size change was rejected";
        instance->model.DirtyVariable("crosshair_status");
        instance->refreshcrosshair(true);
        event.StopPropagation();
    }

    void RmlSettings::increasecrosshairsizeaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = std::min(
            instance->crosshair_size_maximum,
            instance->crosshair_size + 1
        );
        const bool accepted = settings::writecrosshairsize(requested);
        instance->crosshair_size_invalid = false;
        instance->model.DirtyVariable("crosshair_size_invalid");
        instance->crosshair_status = accepted
            ? "Crosshair size adjusted by 1"
            : "Crosshair size change was rejected";
        instance->model.DirtyVariable("crosshair_status");
        instance->refreshcrosshair(true);
        event.StopPropagation();
    }

    void RmlSettings::focuscrosshairsizeaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->crosshair_size_editing = true;
            if(instance->crosshair_size_invalid)
            {
                instance->crosshair_size_invalid = false;
                instance->model.DirtyVariable("crosshair_size_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurcrosshairsizeaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitcrosshairsize();
            instance->crosshair_size_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitcrosshairsizeaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitcrosshairsize();
            event.StopPropagation();
        }
    }

    void RmlSettings::crosshaireffectsaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writecrosshaireffects(!instance->crosshair_effects);
            instance->refreshcrosshair();
        }
        event.StopPropagation();
    }

    void RmlSettings::crosshairhealthcolorsaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writecrosshairhealthcolors(
                !instance->crosshair_health_colors
            );
            instance->refreshcrosshair();
        }
        event.StopPropagation();
    }

    void RmlSettings::teamcrosshairaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writeteamcrosshair(!instance->team_crosshair);
            instance->refreshcrosshair();
        }
        event.StopPropagation();
    }

    void RmlSettings::hitcrosshairaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writehitcrosshair(!instance->hit_crosshair);
            instance->refreshcrosshair();
        }
        event.StopPropagation();
    }

    void RmlSettings::crosshairreloadfadeaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writecrosshairreloadfade(
                !instance->crosshair_reload_fade
            );
            instance->refreshcrosshair();
        }
        event.StopPropagation();
    }

    void RmlSettings::crosshairreloadrotationaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writecrosshairreloadrotation(
                !instance->crosshair_reload_rotation
            );
            instance->refreshcrosshair();
        }
        event.StopPropagation();
    }

    void RmlSettings::showfpsaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writeshowfps(!instance->show_fps);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::wallclockaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writewallclock(!instance->wall_clock);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::wallclock24action(Rml::Event &event)
    {
        if(instance && instance->wall_clock)
        {
            settings::writewallclock24hour(!instance->wall_clock_24_hour);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::wallclocksecondsaction(Rml::Event &event)
    {
        if(instance && instance->wall_clock)
        {
            settings::writewallclockseconds(!instance->wall_clock_seconds);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::damagescreenaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writedamagescreen(!instance->damage_screen);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::damagecompassaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writedamagecompass(!instance->damage_compass);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::hudgunsaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writehudguns(!instance->hud_guns);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::hudgunswayaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writehudgunsway(!instance->hud_gun_sway);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::muzzleflashaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writemuzzleflash(!instance->muzzle_flash);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::muzzlelightaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writemuzzlelight(!instance->muzzle_light);
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::fragmessagesenabledaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writefragmessages(
                instance->frag_messages_enabled
                    ? settings::FragMessagesMode::Off
                    : settings::FragMessagesMode::AllModes
            );
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::fragmessagesinstaaction(Rml::Event &event)
    {
        if(instance && instance->frag_messages_enabled)
        {
            settings::writefragmessages(
                instance->frag_messages_in_insta
                    ? settings::FragMessagesMode::OutsideInsta
                    : settings::FragMessagesMode::AllModes
            );
            instance->refreshhud();
        }
        event.StopPropagation();
    }

    void RmlSettings::fragmessageselffragsaction(Rml::Event &event)
    {
        if(instance && instance->frag_messages_enabled)
        {
            settings::Hud state = {};
            const bool accepted = settings::writefragmessageselffrags(
                !instance->frag_message_self_frags,
                state
            );
            instance->finishhudwrite(accepted, state);
        }
        event.StopPropagation();
    }

    void RmlSettings::fragmessageotherfragsaction(Rml::Event &event)
    {
        if(instance && instance->frag_messages_enabled)
        {
            settings::Hud state = {};
            const bool accepted = settings::writefragmessageotherfrags(
                !instance->frag_message_other_frags,
                state
            );
            instance->finishhudwrite(accepted, state);
        }
        event.StopPropagation();
    }

    void RmlSettings::fragmessageteamkillsaction(Rml::Event &event)
    {
        if(instance && instance->frag_messages_enabled)
        {
            settings::Hud state = {};
            const bool accepted = settings::writefragmessageteamkills(
                !instance->frag_message_team_kills,
                state
            );
            instance->finishhudwrite(accepted, state);
        }
        event.StopPropagation();
    }

    void RmlSettings::fragmessagefadeaction(Rml::Event &event)
    {
        if(instance && instance->frag_messages_enabled)
        {
            settings::Hud state = {};
            const bool accepted = settings::writefragmessagefade(
                !instance->frag_message_fade,
                state
            );
            instance->finishhudwrite(accepted, state);
        }
        event.StopPropagation();
    }

    void RmlSettings::changemaxfragmessagesaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->max_frag_messages
        );

        if(!instance->frag_messages_enabled)
        {
            event.StopPropagation();
            return;
        }
        if(!instance->max_frag_messages_armed)
        {
            if(requested == instance->max_frag_messages)
                instance->max_frag_messages_armed = true;
            event.StopPropagation();
            return;
        }
        if(requested == instance->max_frag_messages)
        {
            event.StopPropagation();
            return;
        }

        Rml::Element *element = event.GetCurrentElement();
        if(!element || !element->GetParentNode())
        {
            event.StopPropagation();
            return;
        }

        settings::Hud state = {};
        const bool accepted = settings::writemaxfragmessages(
            requested, state);
        instance->finishhudwrite(accepted, state);
        event.StopPropagation();
    }

    void RmlSettings::decreasefragmessagedurationaction(Rml::Event &event)
    {
        if(!instance || !instance->frag_messages_enabled)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->frag_message_duration_minimum,
            stepfragmessagefloat(instance->frag_message_duration, -1, 0.1)
        );
        settings::Hud state = {};
        const bool accepted = settings::writefragmessageduration(
            requested, state);
        instance->frag_message_duration_invalid = false;
        instance->model.DirtyVariable("frag_message_duration_invalid");
        instance->finishhudwrite(accepted, state);
        if(accepted)
        {
            instance->hud_status = "Duration adjusted by 0.1";
            instance->model.DirtyVariable("hud_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::increasefragmessagedurationaction(Rml::Event &event)
    {
        if(!instance || !instance->frag_messages_enabled)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->frag_message_duration_maximum,
            stepfragmessagefloat(instance->frag_message_duration, 1, 0.1)
        );
        settings::Hud state = {};
        const bool accepted = settings::writefragmessageduration(
            requested, state);
        instance->frag_message_duration_invalid = false;
        instance->model.DirtyVariable("frag_message_duration_invalid");
        instance->finishhudwrite(accepted, state);
        if(accepted)
        {
            instance->hud_status = "Duration adjusted by 0.1";
            instance->model.DirtyVariable("hud_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::focusfragmessagedurationaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->frag_message_duration_editing = true;
            if(instance->frag_message_duration_invalid)
            {
                instance->frag_message_duration_invalid = false;
                instance->model.DirtyVariable(
                    "frag_message_duration_invalid"
                );
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurfragmessagedurationaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitfragmessageduration();
            instance->frag_message_duration_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitfragmessagedurationaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitfragmessageduration();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreasefragmessagescaleaction(Rml::Event &event)
    {
        if(!instance || !instance->frag_messages_enabled)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->frag_message_scale_minimum,
            stepfragmessagefloat(instance->frag_message_scale, -1, 0.1)
        );
        settings::Hud state = {};
        const bool accepted = settings::writefragmessagescale(
            requested, state);
        instance->frag_message_scale_invalid = false;
        instance->model.DirtyVariable("frag_message_scale_invalid");
        instance->finishhudwrite(accepted, state);
        if(accepted)
        {
            instance->hud_status = "Size adjusted by 0.1";
            instance->model.DirtyVariable("hud_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::increasefragmessagescaleaction(Rml::Event &event)
    {
        if(!instance || !instance->frag_messages_enabled)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->frag_message_scale_maximum,
            stepfragmessagefloat(instance->frag_message_scale, 1, 0.1)
        );
        settings::Hud state = {};
        const bool accepted = settings::writefragmessagescale(
            requested, state);
        instance->frag_message_scale_invalid = false;
        instance->model.DirtyVariable("frag_message_scale_invalid");
        instance->finishhudwrite(accepted, state);
        if(accepted)
        {
            instance->hud_status = "Size adjusted by 0.1";
            instance->model.DirtyVariable("hud_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::focusfragmessagescaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->frag_message_scale_editing = true;
            if(instance->frag_message_scale_invalid)
            {
                instance->frag_message_scale_invalid = false;
                instance->model.DirtyVariable("frag_message_scale_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurfragmessagescaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitfragmessagescale();
            instance->frag_message_scale_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitfragmessagescaleaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitfragmessagescale();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreasefragmessagexaction(Rml::Event &event)
    {
        if(!instance || !instance->frag_messages_enabled)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->frag_message_x_minimum,
            stepfragmessagefloat(instance->frag_message_x, -1, 0.01)
        );
        settings::Hud state = {};
        const bool accepted = settings::writefragmessagex(requested, state);
        instance->frag_message_x_invalid = false;
        instance->model.DirtyVariable("frag_message_x_invalid");
        instance->finishhudwrite(accepted, state);
        if(accepted)
        {
            instance->hud_status = "X adjusted by 0.01";
            instance->model.DirtyVariable("hud_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::increasefragmessagexaction(Rml::Event &event)
    {
        if(!instance || !instance->frag_messages_enabled)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->frag_message_x_maximum,
            stepfragmessagefloat(instance->frag_message_x, 1, 0.01)
        );
        settings::Hud state = {};
        const bool accepted = settings::writefragmessagex(requested, state);
        instance->frag_message_x_invalid = false;
        instance->model.DirtyVariable("frag_message_x_invalid");
        instance->finishhudwrite(accepted, state);
        if(accepted)
        {
            instance->hud_status = "X adjusted by 0.01";
            instance->model.DirtyVariable("hud_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::focusfragmessagexaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->frag_message_x_editing = true;
            if(instance->frag_message_x_invalid)
            {
                instance->frag_message_x_invalid = false;
                instance->model.DirtyVariable("frag_message_x_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurfragmessagexaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitfragmessagex();
            instance->frag_message_x_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitfragmessagexaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitfragmessagex();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreasefragmessageyaction(Rml::Event &event)
    {
        if(!instance || !instance->frag_messages_enabled)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->frag_message_y_minimum,
            stepfragmessagefloat(instance->frag_message_y, -1, 0.01)
        );
        settings::Hud state = {};
        const bool accepted = settings::writefragmessagey(requested, state);
        instance->frag_message_y_invalid = false;
        instance->model.DirtyVariable("frag_message_y_invalid");
        instance->finishhudwrite(accepted, state);
        if(accepted)
        {
            instance->hud_status = "Y adjusted by 0.01";
            instance->model.DirtyVariable("hud_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::increasefragmessageyaction(Rml::Event &event)
    {
        if(!instance || !instance->frag_messages_enabled)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->frag_message_y_maximum,
            stepfragmessagefloat(instance->frag_message_y, 1, 0.01)
        );
        settings::Hud state = {};
        const bool accepted = settings::writefragmessagey(requested, state);
        instance->frag_message_y_invalid = false;
        instance->model.DirtyVariable("frag_message_y_invalid");
        instance->finishhudwrite(accepted, state);
        if(accepted)
        {
            instance->hud_status = "Y adjusted by 0.01";
            instance->model.DirtyVariable("hud_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::focusfragmessageyaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->frag_message_y_editing = true;
            if(instance->frag_message_y_invalid)
            {
                instance->frag_message_y_invalid = false;
                instance->model.DirtyVariable("frag_message_y_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurfragmessageyaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitfragmessagey();
            instance->frag_message_y_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitfragmessageyaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitfragmessagey();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreasestreakscaleaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::max(
            instance->streak_milestone_scale_minimum,
            stepstreakscale(instance->streak_milestone_scale, -1)
        );
        const bool accepted = settings::writestreakmilestonescale(requested);
        instance->streak_milestone_scale_invalid = false;
        instance->model.DirtyVariable(
            "streak_milestone_scale_invalid"
        );
        instance->refreshhud(true);
        instance->hud_status = accepted
            ? "Streak scale adjusted by 0.05"
            : "Streak scale change was rejected";
        instance->model.DirtyVariable("hud_status");
        event.StopPropagation();
    }

    void RmlSettings::increasestreakscaleaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::min(
            instance->streak_milestone_scale_maximum,
            stepstreakscale(instance->streak_milestone_scale, 1)
        );
        const bool accepted = settings::writestreakmilestonescale(requested);
        instance->streak_milestone_scale_invalid = false;
        instance->model.DirtyVariable(
            "streak_milestone_scale_invalid"
        );
        instance->refreshhud(true);
        instance->hud_status = accepted
            ? "Streak scale adjusted by 0.05"
            : "Streak scale change was rejected";
        instance->model.DirtyVariable("hud_status");
        event.StopPropagation();
    }

    void RmlSettings::focusstreakscaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->streak_milestone_scale_editing = true;
            if(instance->streak_milestone_scale_invalid)
            {
                instance->streak_milestone_scale_invalid = false;
                instance->model.DirtyVariable(
                    "streak_milestone_scale_invalid"
                );
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurstreakscaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitstreakscale();
            instance->streak_milestone_scale_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitstreakscaleaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitstreakscale();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreaseammobarscaleaction(Rml::Event &event)
    {
        if(!instance || !instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->ammobar_scale_minimum,
            stepammobarfloat(instance->ammobar_scale, -1)
        );
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarscale(requested, state);
        instance->ammobar_scale_invalid = false;
        instance->model.DirtyVariable("ammobar_scale_invalid");
        instance->finishammobarwrite(accepted, state);
        if(accepted)
        {
            instance->ammobar_status = "Size adjusted by 0.01";
            instance->model.DirtyVariable("ammobar_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::increaseammobarscaleaction(Rml::Event &event)
    {
        if(!instance || !instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->ammobar_scale_maximum,
            stepammobarfloat(instance->ammobar_scale, 1)
        );
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarscale(requested, state);
        instance->ammobar_scale_invalid = false;
        instance->model.DirtyVariable("ammobar_scale_invalid");
        instance->finishammobarwrite(accepted, state);
        if(accepted)
        {
            instance->ammobar_status = "Size adjusted by 0.01";
            instance->model.DirtyVariable("ammobar_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::focusammobarscaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->ammobar_scale_editing = true;
            if(instance->ammobar_scale_invalid)
            {
                instance->ammobar_scale_invalid = false;
                instance->model.DirtyVariable("ammobar_scale_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurammobarscaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitammobarscale();
            instance->ammobar_scale_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitammobarscaleaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitammobarscale();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreaseammobarlabelsizeaction(Rml::Event &event)
    {
        if(!instance || !instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->ammobar_label_size_minimum,
            stepammobarfloat(instance->ammobar_label_size, -1)
        );
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarcountscale(
            requested, state);
        instance->ammobar_label_size_invalid = false;
        instance->model.DirtyVariable("ammobar_label_size_invalid");
        instance->finishammobarwrite(accepted, state);
        if(accepted)
        {
            instance->ammobar_status = "Label size adjusted by 0.01";
            instance->model.DirtyVariable("ammobar_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::increaseammobarlabelsizeaction(Rml::Event &event)
    {
        if(!instance || !instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->ammobar_label_size_maximum,
            stepammobarfloat(instance->ammobar_label_size, 1)
        );
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarcountscale(
            requested, state);
        instance->ammobar_label_size_invalid = false;
        instance->model.DirtyVariable("ammobar_label_size_invalid");
        instance->finishammobarwrite(accepted, state);
        if(accepted)
        {
            instance->ammobar_status = "Label size adjusted by 0.01";
            instance->model.DirtyVariable("ammobar_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::focusammobarlabelsizeaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->ammobar_label_size_editing = true;
            if(instance->ammobar_label_size_invalid)
            {
                instance->ammobar_label_size_invalid = false;
                instance->model.DirtyVariable("ammobar_label_size_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurammobarlabelsizeaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitammobarlabelsize();
            instance->ammobar_label_size_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitammobarlabelsizeaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitammobarlabelsize();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreaseammobarxaction(Rml::Event &event)
    {
        if(!instance || !instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->ammobar_x_minimum,
            stepammobarfloat(instance->ammobar_x, -1)
        );
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarx(requested, state);
        instance->ammobar_x_invalid = false;
        instance->model.DirtyVariable("ammobar_x_invalid");
        instance->finishammobarwrite(accepted, state);
        if(accepted)
        {
            instance->ammobar_status = "X adjusted by 0.01";
            instance->model.DirtyVariable("ammobar_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::increaseammobarxaction(Rml::Event &event)
    {
        if(!instance || !instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->ammobar_x_maximum,
            stepammobarfloat(instance->ammobar_x, 1)
        );
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobarx(requested, state);
        instance->ammobar_x_invalid = false;
        instance->model.DirtyVariable("ammobar_x_invalid");
        instance->finishammobarwrite(accepted, state);
        if(accepted)
        {
            instance->ammobar_status = "X adjusted by 0.01";
            instance->model.DirtyVariable("ammobar_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::focusammobarxaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->ammobar_x_editing = true;
            if(instance->ammobar_x_invalid)
            {
                instance->ammobar_x_invalid = false;
                instance->model.DirtyVariable("ammobar_x_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurammobarxaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitammobarx();
            instance->ammobar_x_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitammobarxaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitammobarx();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreaseammobaryaction(Rml::Event &event)
    {
        if(!instance || !instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->ammobar_y_minimum,
            stepammobarfloat(instance->ammobar_y, -1)
        );
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobary(requested, state);
        instance->ammobar_y_invalid = false;
        instance->model.DirtyVariable("ammobar_y_invalid");
        instance->finishammobarwrite(accepted, state);
        if(accepted)
        {
            instance->ammobar_status = "Y adjusted by 0.01";
            instance->model.DirtyVariable("ammobar_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::increaseammobaryaction(Rml::Event &event)
    {
        if(!instance || !instance->ammobar_available)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->ammobar_y_maximum,
            stepammobarfloat(instance->ammobar_y, 1)
        );
        settings::Ammobar state = {};
        const bool accepted = settings::writeammobary(requested, state);
        instance->ammobar_y_invalid = false;
        instance->model.DirtyVariable("ammobar_y_invalid");
        instance->finishammobarwrite(accepted, state);
        if(accepted)
        {
            instance->ammobar_status = "Y adjusted by 0.01";
            instance->model.DirtyVariable("ammobar_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::focusammobaryaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->ammobar_y_editing = true;
            if(instance->ammobar_y_invalid)
            {
                instance->ammobar_y_invalid = false;
                instance->model.DirtyVariable("ammobar_y_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurammobaryaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitammobary();
            instance->ammobar_y_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitammobaryaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitammobary();
            event.StopPropagation();
        }
    }

    void RmlSettings::changegameclockalphaaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->game_clock_alpha
        );

        // Ignore programmatic range updates so opening Settings cannot
        // rewrite the native ident. RmlUi emits change for min/max clamps
        // during InstanceElement, before the range is parented. There is
        // no hardcoded RML value default.
        if(requested == instance->game_clock_alpha)
        {
            event.StopPropagation();
            return;
        }

        Rml::Element *element = event.GetCurrentElement();
        if(!element || !element->GetParentNode())
        {
            event.StopPropagation();
            return;
        }

        if(!instance->game_clock_available || !instance->game_clock)
        {
            event.StopPropagation();
            return;
        }

        settings::GameClock state = {};
        const bool accepted = settings::writegameclockalpha(
            requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);
        instance->presentgameclock(state, available);
        if(!accepted && available)
        {
            instance->game_clock_status =
                "Change not accepted; current engine state is shown";
            instance->model.DirtyVariable("game_clock_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::decreasegameclockscaleaction(Rml::Event &event)
    {
        if(!instance || !instance->game_clock_available ||
           !instance->game_clock)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->game_clock_scale_minimum,
            stepgameclockfloat(instance->game_clock_scale, -1)
        );
        settings::GameClock state = {};
        const bool accepted = settings::writegameclockscale(
            requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);
        instance->game_clock_scale_invalid = false;
        instance->model.DirtyVariable("game_clock_scale_invalid");
        const bool editing = instance->game_clock_scale_editing;
        instance->game_clock_scale_editing = false;
        instance->presentgameclock(state, available);
        instance->game_clock_scale_editing = editing;
        instance->game_clock_status = accepted
            ? "Size adjusted by 0.01"
            : "Change not accepted; current engine state is shown";
        instance->model.DirtyVariable("game_clock_status");
        event.StopPropagation();
    }

    void RmlSettings::increasegameclockscaleaction(Rml::Event &event)
    {
        if(!instance || !instance->game_clock_available ||
           !instance->game_clock)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->game_clock_scale_maximum,
            stepgameclockfloat(instance->game_clock_scale, 1)
        );
        settings::GameClock state = {};
        const bool accepted = settings::writegameclockscale(
            requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);
        instance->game_clock_scale_invalid = false;
        instance->model.DirtyVariable("game_clock_scale_invalid");
        const bool editing = instance->game_clock_scale_editing;
        instance->game_clock_scale_editing = false;
        instance->presentgameclock(state, available);
        instance->game_clock_scale_editing = editing;
        instance->game_clock_status = accepted
            ? "Size adjusted by 0.01"
            : "Change not accepted; current engine state is shown";
        instance->model.DirtyVariable("game_clock_status");
        event.StopPropagation();
    }

    void RmlSettings::focusgameclockscaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->game_clock_scale_editing = true;
            if(instance->game_clock_scale_invalid)
            {
                instance->game_clock_scale_invalid = false;
                instance->model.DirtyVariable(
                    "game_clock_scale_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurgameclockscaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitgameclockscale();
            instance->game_clock_scale_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitgameclockscaleaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitgameclockscale();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreasegameclockxaction(Rml::Event &event)
    {
        if(!instance || !instance->game_clock_available ||
           !instance->game_clock)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->game_clock_x_minimum,
            stepgameclockfloat(instance->game_clock_x, -1)
        );
        settings::GameClock state = {};
        const bool accepted = settings::writegameclockx(
            requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);
        instance->game_clock_x_invalid = false;
        instance->model.DirtyVariable("game_clock_x_invalid");
        const bool editing = instance->game_clock_x_editing;
        instance->game_clock_x_editing = false;
        instance->presentgameclock(state, available);
        instance->game_clock_x_editing = editing;
        instance->game_clock_status = accepted
            ? "X adjusted by 0.01"
            : "Change not accepted; current engine state is shown";
        instance->model.DirtyVariable("game_clock_status");
        event.StopPropagation();
    }

    void RmlSettings::increasegameclockxaction(Rml::Event &event)
    {
        if(!instance || !instance->game_clock_available ||
           !instance->game_clock)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->game_clock_x_maximum,
            stepgameclockfloat(instance->game_clock_x, 1)
        );
        settings::GameClock state = {};
        const bool accepted = settings::writegameclockx(
            requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);
        instance->game_clock_x_invalid = false;
        instance->model.DirtyVariable("game_clock_x_invalid");
        const bool editing = instance->game_clock_x_editing;
        instance->game_clock_x_editing = false;
        instance->presentgameclock(state, available);
        instance->game_clock_x_editing = editing;
        instance->game_clock_status = accepted
            ? "X adjusted by 0.01"
            : "Change not accepted; current engine state is shown";
        instance->model.DirtyVariable("game_clock_status");
        event.StopPropagation();
    }

    void RmlSettings::focusgameclockxaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->game_clock_x_editing = true;
            if(instance->game_clock_x_invalid)
            {
                instance->game_clock_x_invalid = false;
                instance->model.DirtyVariable("game_clock_x_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurgameclockxaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitgameclockx();
            instance->game_clock_x_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitgameclockxaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitgameclockx();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreasegameclockyaction(Rml::Event &event)
    {
        if(!instance || !instance->game_clock_available ||
           !instance->game_clock)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::max(
            instance->game_clock_y_minimum,
            stepgameclockfloat(instance->game_clock_y, -1)
        );
        settings::GameClock state = {};
        const bool accepted = settings::writegameclocky(
            requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);
        instance->game_clock_y_invalid = false;
        instance->model.DirtyVariable("game_clock_y_invalid");
        const bool editing = instance->game_clock_y_editing;
        instance->game_clock_y_editing = false;
        instance->presentgameclock(state, available);
        instance->game_clock_y_editing = editing;
        instance->game_clock_status = accepted
            ? "Y adjusted by 0.01"
            : "Change not accepted; current engine state is shown";
        instance->model.DirtyVariable("game_clock_status");
        event.StopPropagation();
    }

    void RmlSettings::increasegameclockyaction(Rml::Event &event)
    {
        if(!instance || !instance->game_clock_available ||
           !instance->game_clock)
        {
            event.StopPropagation();
            return;
        }

        const float requested = std::min(
            instance->game_clock_y_maximum,
            stepgameclockfloat(instance->game_clock_y, 1)
        );
        settings::GameClock state = {};
        const bool accepted = settings::writegameclocky(
            requested, state);
        const bool available = accepted ||
            settings::readgameclock(state);
        instance->game_clock_y_invalid = false;
        instance->model.DirtyVariable("game_clock_y_invalid");
        const bool editing = instance->game_clock_y_editing;
        instance->game_clock_y_editing = false;
        instance->presentgameclock(state, available);
        instance->game_clock_y_editing = editing;
        instance->game_clock_status = accepted
            ? "Y adjusted by 0.01"
            : "Change not accepted; current engine state is shown";
        instance->model.DirtyVariable("game_clock_status");
        event.StopPropagation();
    }

    void RmlSettings::focusgameclockyaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->game_clock_y_editing = true;
            if(instance->game_clock_y_invalid)
            {
                instance->game_clock_y_invalid = false;
                instance->model.DirtyVariable("game_clock_y_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurgameclockyaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitgameclocky();
            instance->game_clock_y_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitgameclockyaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitgameclocky();
            event.StopPropagation();
        }
    }

    void RmlSettings::changehudscorealphaaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->hudscore_alpha
        );

        // Ignore programmatic range updates so opening Settings cannot
        // rewrite the native ident. RmlUi emits change for min/max clamps
        // during InstanceElement, before the range is parented. There is
        // no hardcoded RML value default.
        if(requested == instance->hudscore_alpha)
        {
            event.StopPropagation();
            return;
        }

        Rml::Element *element = event.GetCurrentElement();
        if(!element || !element->GetParentNode())
        {
            event.StopPropagation();
            return;
        }

        if(!instance->hudscore_available)
        {
            event.StopPropagation();
            return;
        }

        settings::HudScore state = {};
        const bool accepted = settings::writehudscorealpha(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        instance->presenthudscore(
            state,
            available,
            !instance->hudscore_scale_editing,
            !instance->hudscore_x_editing,
            !instance->hudscore_y_editing);
        if(!accepted && available)
        {
            instance->hudscore_status =
                "Change not accepted; current engine state is shown";
            instance->model.DirtyVariable("hudscore_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::changehudscoresepaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->hudscore_sep
        );

        // Ignore programmatic range updates so opening Settings cannot
        // rewrite the native ident. RmlUi emits change for min/max clamps
        // during InstanceElement, before the range is parented. There is
        // no hardcoded RML value default.
        if(requested == instance->hudscore_sep)
        {
            event.StopPropagation();
            return;
        }

        Rml::Element *element = event.GetCurrentElement();
        if(!element || !element->GetParentNode())
        {
            event.StopPropagation();
            return;
        }

        if(!instance->hudscore_available)
        {
            event.StopPropagation();
            return;
        }

        settings::HudScore state = {};
        const bool accepted = settings::writehudscoresep(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        instance->presenthudscore(
            state,
            available,
            !instance->hudscore_scale_editing,
            !instance->hudscore_x_editing,
            !instance->hudscore_y_editing);
        if(!accepted && available)
        {
            instance->hudscore_status =
                "Change not accepted; current engine state is shown";
            instance->model.DirtyVariable("hudscore_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::decreasehudscorescaleaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::max(
            instance->hudscore_scale_minimum,
            stephudscore(instance->hudscore_scale, -1)
        );
        settings::HudScore state = {};
        const bool accepted = settings::writehudscorescale(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        instance->hudscore_scale_invalid = false;
        instance->model.DirtyVariable("hudscore_scale_invalid");
        instance->presenthudscore(
            state,
            available,
            true,
            !instance->hudscore_x_editing,
            !instance->hudscore_y_editing);
        instance->hudscore_status = accepted
            ? "Score overlay size adjusted by 0.01"
            : (available
                ? "Score overlay size change was rejected"
                : "Score overlay settings are unavailable");
        instance->model.DirtyVariable("hudscore_status");
        event.StopPropagation();
    }

    void RmlSettings::increasehudscorescaleaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::min(
            instance->hudscore_scale_maximum,
            stephudscore(instance->hudscore_scale, 1)
        );
        settings::HudScore state = {};
        const bool accepted = settings::writehudscorescale(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        instance->hudscore_scale_invalid = false;
        instance->model.DirtyVariable("hudscore_scale_invalid");
        instance->presenthudscore(
            state,
            available,
            true,
            !instance->hudscore_x_editing,
            !instance->hudscore_y_editing);
        instance->hudscore_status = accepted
            ? "Score overlay size adjusted by 0.01"
            : (available
                ? "Score overlay size change was rejected"
                : "Score overlay settings are unavailable");
        instance->model.DirtyVariable("hudscore_status");
        event.StopPropagation();
    }

    void RmlSettings::focushudscorescaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->hudscore_scale_editing = true;
            if(instance->hudscore_scale_invalid)
            {
                instance->hudscore_scale_invalid = false;
                instance->model.DirtyVariable("hudscore_scale_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurhudscorescaleaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commithudscorescale();
            instance->hudscore_scale_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commithudscorescaleaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commithudscorescale();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreasehudscorexaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::max(
            instance->hudscore_x_minimum,
            stephudscore(instance->hudscore_x, -1)
        );
        settings::HudScore state = {};
        const bool accepted = settings::writehudscorex(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        instance->hudscore_x_invalid = false;
        instance->model.DirtyVariable("hudscore_x_invalid");
        instance->presenthudscore(
            state,
            available,
            !instance->hudscore_scale_editing,
            true,
            !instance->hudscore_y_editing);
        instance->hudscore_status = accepted
            ? "Score overlay x adjusted by 0.01"
            : (available
                ? "Score overlay x change was rejected"
                : "Score overlay settings are unavailable");
        instance->model.DirtyVariable("hudscore_status");
        event.StopPropagation();
    }

    void RmlSettings::increasehudscorexaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::min(
            instance->hudscore_x_maximum,
            stephudscore(instance->hudscore_x, 1)
        );
        settings::HudScore state = {};
        const bool accepted = settings::writehudscorex(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        instance->hudscore_x_invalid = false;
        instance->model.DirtyVariable("hudscore_x_invalid");
        instance->presenthudscore(
            state,
            available,
            !instance->hudscore_scale_editing,
            true,
            !instance->hudscore_y_editing);
        instance->hudscore_status = accepted
            ? "Score overlay x adjusted by 0.01"
            : (available
                ? "Score overlay x change was rejected"
                : "Score overlay settings are unavailable");
        instance->model.DirtyVariable("hudscore_status");
        event.StopPropagation();
    }

    void RmlSettings::focushudscorexaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->hudscore_x_editing = true;
            if(instance->hudscore_x_invalid)
            {
                instance->hudscore_x_invalid = false;
                instance->model.DirtyVariable("hudscore_x_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurhudscorexaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commithudscorex();
            instance->hudscore_x_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commithudscorexaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commithudscorex();
            event.StopPropagation();
        }
    }

    void RmlSettings::decreasehudscoreyaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::max(
            instance->hudscore_y_minimum,
            stephudscore(instance->hudscore_y, -1)
        );
        settings::HudScore state = {};
        const bool accepted = settings::writehudscorey(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        instance->hudscore_y_invalid = false;
        instance->model.DirtyVariable("hudscore_y_invalid");
        instance->presenthudscore(
            state,
            available,
            !instance->hudscore_scale_editing,
            !instance->hudscore_x_editing,
            true);
        instance->hudscore_status = accepted
            ? "Score overlay y adjusted by 0.01"
            : (available
                ? "Score overlay y change was rejected"
                : "Score overlay settings are unavailable");
        instance->model.DirtyVariable("hudscore_status");
        event.StopPropagation();
    }

    void RmlSettings::increasehudscoreyaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::min(
            instance->hudscore_y_maximum,
            stephudscore(instance->hudscore_y, 1)
        );
        settings::HudScore state = {};
        const bool accepted = settings::writehudscorey(requested, state);
        const bool available = accepted ||
            settings::readhudscore(state);
        instance->hudscore_y_invalid = false;
        instance->model.DirtyVariable("hudscore_y_invalid");
        instance->presenthudscore(
            state,
            available,
            !instance->hudscore_scale_editing,
            !instance->hudscore_x_editing,
            true);
        instance->hudscore_status = accepted
            ? "Score overlay y adjusted by 0.01"
            : (available
                ? "Score overlay y change was rejected"
                : "Score overlay settings are unavailable");
        instance->model.DirtyVariable("hudscore_status");
        event.StopPropagation();
    }

    void RmlSettings::focushudscoreyaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->hudscore_y_editing = true;
            if(instance->hudscore_y_invalid)
            {
                instance->hudscore_y_invalid = false;
                instance->model.DirtyVariable("hudscore_y_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurhudscoreyaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commithudscorey();
            instance->hudscore_y_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commithudscoreyaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commithudscorey();
            event.StopPropagation();
        }
    }

    void RmlSettings::publicchatintopfeedaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writepublicchatintopfeed(
                !instance->public_chat_in_top_feed
            );
            instance->refreshchat();
        }
        event.StopPropagation();
    }

    void RmlSettings::teamchatintopfeedaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writeteamchatintopfeed(
                !instance->team_chat_in_top_feed
            );
            instance->refreshchat();
        }
        event.StopPropagation();
    }

    void RmlSettings::publicchatinbottomboxaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writepublicchatinbottombox(
                !instance->public_chat_in_bottom_box
            );
            instance->refreshchat();
        }
        event.StopPropagation();
    }

    void RmlSettings::teamchatinbottomboxaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writeteamchatinbottombox(
                !instance->team_chat_in_bottom_box
            );
            instance->refreshchat();
        }
        event.StopPropagation();
    }

    void RmlSettings::teamcoloredchatnamesaction(Rml::Event &event)
    {
        if(instance)
        {
            settings::writeteamcoloredchatnames(
                !instance->team_colored_chat_names
            );
            instance->refreshchat();
        }
        event.StopPropagation();
    }

    void RmlSettings::consoletopimportantaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeimportantintopfeed,
                instance->console_top_important);
        }
        event.StopPropagation();
    }

    void RmlSettings::consoletopwarningsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writewarningsintopfeed,
                instance->console_top_warnings);
        }
        event.StopPropagation();
    }

    void RmlSettings::consoletoperrorsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeerrorsintopfeed,
                instance->console_top_errors);
        }
        event.StopPropagation();
    }

    void RmlSettings::consoletopscriptaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writescriptmessagesintopfeed,
                instance->console_top_script);
        }
        event.StopPropagation();
    }

    void RmlSettings::consoletopgameeventsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writegameeventsintopfeed,
                instance->console_top_game_events);
        }
        event.StopPropagation();
    }

    void RmlSettings::consoletopselffragsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeselffragsintopfeed,
                instance->console_top_self_frags);
        }
        event.StopPropagation();
    }

    void RmlSettings::consoletopotherfragsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeotherplayerfragsintopfeed,
                instance->console_top_other_frags);
        }
        event.StopPropagation();
    }

    void RmlSettings::consoletopteamkillsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeteamkillsintopfeed,
                instance->console_top_team_kills);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullimportantaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeimportantinfullconsole,
                instance->console_full_important);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullwarningsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writewarningsinfullconsole,
                instance->console_full_warnings);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullerrorsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeerrorsinfullconsole,
                instance->console_full_errors);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullscriptaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writescriptmessagesinfullconsole,
                instance->console_full_script);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullchataction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writepublicchatinfullconsole,
                instance->console_full_chat);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullteamchataction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeteamchatinfullconsole,
                instance->console_full_team_chat);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullgameeventsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writegameeventsinfullconsole,
                instance->console_full_game_events);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullselffragsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeselffragsinfullconsole,
                instance->console_full_self_frags);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullotherfragsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeotherplayerfragsinfullconsole,
                instance->console_full_other_frags);
        }
        event.StopPropagation();
    }

    void RmlSettings::consolefullteamkillsaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->applyconsolefilter(
                settings::writeteamkillsinfullconsole,
                instance->console_full_team_kills);
        }
        event.StopPropagation();
    }

    void RmlSettings::decreaseconsolesizeaction(Rml::Event &event)
    {
        if(instance && instance->console_available)
        {
            const int requested = std::max(
                instance->console_size_minimum,
                instance->console_size - 1
            );
            settings::Console state = {};
            const bool accepted = settings::writeconsolesize(
                requested, state);
            const bool available = accepted ||
                settings::readconsole(state);
            instance->console_size_invalid = false;
            instance->model.DirtyVariable("console_size_invalid");
            instance->presentconsole(state, available, true);
            instance->console_status = accepted
                ? "Top-left console size adjusted by 1"
                : (available
                    ? "Console size change was rejected"
                    : "Console settings are unavailable");
            instance->model.DirtyVariable("console_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::increaseconsolesizeaction(Rml::Event &event)
    {
        if(instance && instance->console_available)
        {
            const int requested = std::min(
                instance->console_size_maximum,
                instance->console_size + 1
            );
            settings::Console state = {};
            const bool accepted = settings::writeconsolesize(
                requested, state);
            const bool available = accepted ||
                settings::readconsole(state);
            instance->console_size_invalid = false;
            instance->model.DirtyVariable("console_size_invalid");
            instance->presentconsole(state, available, true);
            instance->console_status = accepted
                ? "Top-left console size adjusted by 1"
                : (available
                    ? "Console size change was rejected"
                    : "Console settings are unavailable");
            instance->model.DirtyVariable("console_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::focusconsolesizeaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->console_size_editing = true;
            if(instance->console_size_invalid)
            {
                instance->console_size_invalid = false;
                instance->model.DirtyVariable("console_size_invalid");
            }
        }
        event.StopPropagation();
    }

    void RmlSettings::blurconsolesizeaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->commitconsolesize();
            instance->console_size_editing = false;
        }
        event.StopPropagation();
    }

    void RmlSettings::commitconsolesizeaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int key = event.GetParameter<int>("key_identifier", 0);
        if(key == Rml::Input::KI_RETURN ||
           key == Rml::Input::KI_NUMPADENTER)
        {
            instance->commitconsolesize();
            event.StopPropagation();
        }
    }
}
