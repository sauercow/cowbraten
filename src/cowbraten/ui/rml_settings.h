#ifndef COWBRATEN_UI_RML_SETTINGS_H
#define COWBRATEN_UI_RML_SETTINGS_H

#include "keybindings_api.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <vector>

namespace Rml
{
    class Context;
    class Event;
}

namespace cowui
{
    namespace settings
    {
        enum class PresentationMode;
        struct CrosshairChoice;
        struct SoundpackChoice;
    }

    class RmlActionRouter;

    class RmlSettings final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);

        void activate();
        void videochanged();
        void applypending();
        bool capturingkey() const { return key_capture_active; }
        void cancelkeycapture();
        void acceptkeycandidate(const char *name);

    private:
        struct Resolution
        {
            Rml::String label;
            int width = 0;
            int height = 0;
            bool selected = false;
            bool supported = false;

            bool operator==(const Resolution &other) const
            {
                return label == other.label &&
                    width == other.width &&
                    height == other.height &&
                    selected == other.selected &&
                    supported == other.supported;
            }
        };

        struct CrosshairChoice
        {
            Rml::String label;
            Rml::String resource;
            int id = -1;
            bool selected = false;

            bool operator==(const CrosshairChoice &other) const
            {
                return label == other.label &&
                    resource == other.resource &&
                    id == other.id &&
                    selected == other.selected;
            }
        };

        struct SoundpackChoice
        {
            Rml::String id;
            Rml::String label;
            bool selected = false;

            bool operator==(const SoundpackChoice &other) const
            {
                return id == other.id &&
                    label == other.label &&
                    selected == other.selected;
            }
        };

        struct KeyBindingRow
        {
            int action_id = -1;
            Rml::String label;
            std::vector<Rml::String> keys;
            bool bound = false;

            bool operator==(const KeyBindingRow &other) const
            {
                return action_id == other.action_id && label == other.label &&
                    keys == other.keys &&
                    bound == other.bound;
            }
        };

        enum class Category
        {
            Game,
            Hud,
            Graphics,
            Audio,
            MouseKeyboard,
            Crosshair,
            Chat
        };

        bool refresh();
        bool refreshvideo();
        bool refreshgraphicsquality();
        bool refreshaudio();
        bool refreshmouseinput(bool update_edit_buffer = false);
        bool refreshcrosshair(bool update_edit_buffer = false);
        bool refreshhud(bool update_edit_buffer = false);
        bool refreshchat();
        bool refreshkeybindings();
        bool refreshkeyconflict();
        void clearkeyremoval();
        void requestkeyremoval(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void decidekeyremoval(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void decidekeycandidate(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void clearkeyconflict();
        static const char *keybindinglabel(keybindings::Action action);
        void startkeycapture(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void commitsensitivity();
        void commitcrosshairsize();
        void commitstreakscale();
        void setcategory(Category category);
        void setpresentation(
            settings::PresentationMode presentation,
            Rml::Event &event
        );
        void selectresolution(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        void selectantialiasing(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        void selectcrosshair(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        void selectsoundpack(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        static void appendcrosshairchoice(
            const settings::CrosshairChoice &choice,
            void *data
        );
        static void appendsoundpackchoice(
            const settings::SoundpackChoice &choice,
            void *data
        );
        static void changefovaction(Rml::Event &event);
        static void gamecategoryaction(Rml::Event &event);
        static void hudcategoryaction(Rml::Event &event);
        static void graphicscategoryaction(Rml::Event &event);
        static void audiocategoryaction(Rml::Event &event);
        static void mousekeyboardcategoryaction(Rml::Event &event);
        static void crosshaircategoryaction(Rml::Event &event);
        static void chatcategoryaction(Rml::Event &event);
        static void windowedaction(Rml::Event &event);
        static void exclusivefullscreenaction(Rml::Event &event);
        static void desktopfullscreenaction(Rml::Event &event);
        static void vsyncaction(Rml::Event &event);
        static void grassaction(Rml::Event &event);
        static void motionbluraction(Rml::Event &event);
        static void changefpslimitaction(Rml::Event &event);
        static void changesoundvolumeaction(Rml::Event &event);
        static void changemusicvolumeaction(Rml::Event &event);
        static void decreasesensitivityaction(Rml::Event &event);
        static void increasesensitivityaction(Rml::Event &event);
        static void focussensitivityaction(Rml::Event &event);
        static void blursensitivityaction(Rml::Event &event);
        static void commitsensitivityaction(Rml::Event &event);
        static void invertedmouseaction(Rml::Event &event);
        static void decreasecrosshairsizeaction(Rml::Event &event);
        static void increasecrosshairsizeaction(Rml::Event &event);
        static void focuscrosshairsizeaction(Rml::Event &event);
        static void blurcrosshairsizeaction(Rml::Event &event);
        static void commitcrosshairsizeaction(Rml::Event &event);
        static void crosshaireffectsaction(Rml::Event &event);
        static void crosshairhealthcolorsaction(Rml::Event &event);
        static void teamcrosshairaction(Rml::Event &event);
        static void hitcrosshairaction(Rml::Event &event);
        static void crosshairreloadfadeaction(Rml::Event &event);
        static void crosshairreloadrotationaction(Rml::Event &event);
        static void damagescreenaction(Rml::Event &event);
        static void damagecompassaction(Rml::Event &event);
        static void fragmessagesenabledaction(Rml::Event &event);
        static void fragmessagesinstaaction(Rml::Event &event);
        static void decreasestreakscaleaction(Rml::Event &event);
        static void increasestreakscaleaction(Rml::Event &event);
        static void focusstreakscaleaction(Rml::Event &event);
        static void blurstreakscaleaction(Rml::Event &event);
        static void commitstreakscaleaction(Rml::Event &event);
        static void publicchatintopfeedaction(Rml::Event &event);
        static void teamchatintopfeedaction(Rml::Event &event);
        static void publicchatinbottomboxaction(Rml::Event &event);
        static void teamchatinbottomboxaction(Rml::Event &event);
        static void teamcoloredchatnamesaction(Rml::Event &event);

        Rml::DataModelHandle model;
        std::vector<Resolution> resolutions;
        std::vector<CrosshairChoice> crosshair_choices;
        std::vector<SoundpackChoice> soundpack_choices;
        std::vector<KeyBindingRow> keybindings_movement;
        std::vector<KeyBindingRow> keybindings_combat;
        std::vector<KeyBindingRow> keybindings_interface;
        bool key_capture_active = false;
        keybindings::Action key_capture_action = keybindings::Action::MoveForward;
        int key_capture_id = -1;
        Rml::String key_candidate;
        Rml::String key_capture_status;
        Rml::String key_binding_status;
        bool key_candidate_pending = false;
        bool key_removal_active = false;
        keybindings::Action key_removal_action = keybindings::Action::MoveForward;
        Rml::String key_removal_key;
        Rml::String key_removal_prompt;
        keybindings::Conflict key_conflict;
        Rml::String key_conflict_status;
        bool key_conflict_available = false;
        bool key_conflict_same_action = false;
        bool key_conflict_present = false;

        int fov = 100;
        int fov_minimum = 10;
        int fov_maximum = 150;

        Rml::String resolution_label;
        Rml::String video_status;
        Rml::String presentation_label;
        Rml::String vsync_status;
        bool grass = false;
        bool motion_blur = false;
        bool motion_blur_available = false;
        Rml::String graphics_quality_status;
        Rml::String fps_limit_label;
        bool presentation_windowed = true;
        bool presentation_exclusive = false;
        bool presentation_desktop = false;
        bool resolution_enabled = true;
        bool vsync = false;
        bool vsync_adaptive = false;
        int fps_limit = 200;
        int fps_limit_minimum = 0;
        int fps_limit_maximum = 1000;
        int antialiasing = 0;
        int antialiasing_requested = -1;
        int pending_antialiasing = -1;
        Rml::String antialiasing_label;
        Rml::String antialiasing_status;
        bool antialiasing_off = true;
        bool antialiasing_2x = false;
        bool antialiasing_4x = false;
        bool antialiasing_8x = false;
        bool antialiasing_16x = false;
        int sound_volume = 255;
        int sound_volume_minimum = 0;
        int sound_volume_maximum = 255;
        int music_volume = 128;
        int music_volume_minimum = 0;
        int music_volume_maximum = 255;
        Rml::String sound_volume_label;
        Rml::String music_volume_label;
        Rml::String audio_status;
        Rml::String active_soundpack;
        Rml::String active_soundpack_label;
        Rml::String soundpack_status;
        float mouse_sensitivity = 3.0f;
        float mouse_sensitivity_minimum = 0.001f;
        float mouse_sensitivity_maximum = 1000.0f;
        Rml::String mouse_sensitivity_label;
        Rml::String mouse_sensitivity_input;
        Rml::String mouse_sensitivity_status;
        bool mouse_sensitivity_invalid = false;
        bool mouse_sensitivity_editing = false;
        bool inverted_mouse = false;
        Rml::String mouse_input_status;
        int crosshair_selection = -1;
        Rml::String crosshair_selection_label;
        Rml::String crosshair_selection_resource;
        int crosshair_size = 30;
        int crosshair_size_minimum = 0;
        int crosshair_size_maximum = 50;
        Rml::String crosshair_size_input;
        Rml::String crosshair_status;
        bool crosshair_size_invalid = false;
        bool crosshair_size_editing = false;
        bool crosshair_effects = true;
        bool crosshair_health_colors = true;
        bool team_crosshair = true;
        bool hit_crosshair = true;
        bool crosshair_reload_fade = true;
        bool crosshair_reload_rotation = true;
        bool damage_screen = true;
        bool damage_compass = true;
        bool frag_messages_enabled = true;
        bool frag_messages_in_insta = true;
        float streak_milestone_scale = 1.25f;
        float streak_milestone_scale_minimum = 1.0f;
        float streak_milestone_scale_maximum = 2.0f;
        Rml::String streak_milestone_scale_input;
        Rml::String hud_status;
        bool streak_milestone_scale_invalid = false;
        bool streak_milestone_scale_editing = false;
        bool public_chat_in_top_feed = true;
        bool team_chat_in_top_feed = true;
        bool public_chat_in_bottom_box = false;
        bool team_chat_in_bottom_box = false;
        bool team_colored_chat_names = true;
        bool category_game = true;
        bool category_hud = false;
        bool category_graphics = false;
        bool category_audio = false;
        bool category_mouse_keyboard = false;
        bool category_crosshair = false;
        bool category_chat = false;

        static RmlSettings *instance;
    };
}

#endif
