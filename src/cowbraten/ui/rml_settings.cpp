#include "rml_settings.h"

#include "keybindings_api.h"
#include "rml_action.h"
#include "settings_api.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Input.h>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cmath>
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
            constructor.Bind("resolution_label", &resolution_label) &&
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
            constructor.Bind("damage_screen", &damage_screen) &&
            constructor.Bind("damage_compass", &damage_compass) &&
            constructor.Bind(
                "frag_messages_enabled",
                &frag_messages_enabled
            ) &&
            constructor.Bind(
                "frag_messages_in_insta",
                &frag_messages_in_insta
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
                "settings.fps-limit",
                changefpslimitaction
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
                "settings.hud.damage-screen",
                damagescreenaction
            ) &&
            router.registeraction(
                "settings.hud.damage-compass",
                damagecompassaction
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
        resolution_label.clear();
        video_status.clear();
        presentation_label.clear();
        vsync_status.clear();
        fps_limit_label.clear();
        presentation_windowed = true;
        presentation_exclusive = false;
        presentation_desktop = false;
        resolution_enabled = true;
        vsync = false;
        vsync_adaptive = false;
        fps_limit = 200;
        fps_limit_minimum = 0;
        fps_limit_maximum = 1000;
        antialiasing = 0;
        antialiasing_requested = -1;
        pending_antialiasing = -1;
        antialiasing_label.clear();
        antialiasing_status.clear();
        antialiasing_off = true;
        antialiasing_2x = false;
        antialiasing_4x = false;
        antialiasing_8x = false;
        antialiasing_16x = false;
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
        damage_screen = true;
        damage_compass = true;
        frag_messages_enabled = true;
        frag_messages_in_insta = true;
        streak_milestone_scale = 1.25f;
        streak_milestone_scale_minimum = 1.0f;
        streak_milestone_scale_maximum = 2.0f;
        streak_milestone_scale_input.clear();
        hud_status.clear();
        streak_milestone_scale_invalid = false;
        streak_milestone_scale_editing = false;
        public_chat_in_top_feed = true;
        team_chat_in_top_feed = true;
        public_chat_in_bottom_box = false;
        team_chat_in_bottom_box = false;
        team_colored_chat_names = true;
        category_game = true;
        category_hud = false;
        category_graphics = false;
        category_audio = false;
        category_mouse_keyboard = false;
        category_crosshair = false;
        category_chat = false;
    }

    void RmlSettings::activate()
    {
        refresh();
    }

    void RmlSettings::videochanged()
    {
        refreshvideo();
    }

    void RmlSettings::applypending()
    {
        if(pending_antialiasing < 0)
            return;

        const int requested = pending_antialiasing;
        pending_antialiasing = -1;
        settings::resetgraphics();
        refreshvideo();

        char status[96];
        if(antialiasing == requested)
        {
            if(requested)
            {
                std::snprintf(
                    status,
                    sizeof(status),
                    "%dx multisample antialiasing active",
                    requested
                );
            }
            else std::snprintf(status, sizeof(status), "Antialiasing disabled");
        }
        else
        {
            std::snprintf(
                status,
                sizeof(status),
                "%dx was unavailable; runtime is %s",
                requested,
                antialiasing ? antialiasing_label.c_str() : "off"
            );
        }

        antialiasing_status = status;
        model.DirtyVariable("antialiasing_status");
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

        if(!refreshvideo())
            return false;
        if(!refreshaudio())
            return false;
        if(!refreshmouseinput(!mouse_sensitivity_editing))
            return false;
        if(!refreshcrosshair(!crosshair_size_editing))
            return false;
        if(!refreshhud(!streak_milestone_scale_editing))
            return false;
        if(!refreshchat())
            return false;
        return refreshkeybindings();
    }

    bool RmlSettings::refreshvideo()
    {
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

        const bool messages_enabled = current.frag_messages !=
            settings::FragMessagesMode::Off;
        const bool messages_in_insta = current.frag_messages ==
            settings::FragMessagesMode::AllModes;
        if(frag_messages_enabled != messages_enabled)
        {
            frag_messages_enabled = messages_enabled;
            model.DirtyVariable("frag_messages_enabled");
        }
        if(frag_messages_in_insta != messages_in_insta)
        {
            frag_messages_in_insta = messages_in_insta;
            model.DirtyVariable("frag_messages_in_insta");
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
        return true;
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

        if(audio)
            refreshaudio();
        else if(hud)
            refreshhud(!streak_milestone_scale_editing);
        else if(crosshair)
            refreshcrosshair(!crosshair_size_editing);
        else if(chat)
            refreshchat();
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
}
