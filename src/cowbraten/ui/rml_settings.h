#ifndef COWBRATEN_UI_RML_SETTINGS_H
#define COWBRATEN_UI_RML_SETTINGS_H

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

        enum class Category
        {
            Game,
            Graphics,
            Audio,
            MouseKeyboard
        };

        bool refresh();
        bool refreshvideo();
        bool refreshaudio();
        bool refreshmouseinput(bool update_edit_buffer = false);
        void commitsensitivity();
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
        static void changefovaction(Rml::Event &event);
        static void gamecategoryaction(Rml::Event &event);
        static void graphicscategoryaction(Rml::Event &event);
        static void audiocategoryaction(Rml::Event &event);
        static void mousekeyboardcategoryaction(Rml::Event &event);
        static void windowedaction(Rml::Event &event);
        static void exclusivefullscreenaction(Rml::Event &event);
        static void desktopfullscreenaction(Rml::Event &event);
        static void vsyncaction(Rml::Event &event);
        static void changefpslimitaction(Rml::Event &event);
        static void changesoundvolumeaction(Rml::Event &event);
        static void changemusicvolumeaction(Rml::Event &event);
        static void decreasesensitivityaction(Rml::Event &event);
        static void increasesensitivityaction(Rml::Event &event);
        static void focussensitivityaction(Rml::Event &event);
        static void blursensitivityaction(Rml::Event &event);
        static void commitsensitivityaction(Rml::Event &event);
        static void invertedmouseaction(Rml::Event &event);

        Rml::DataModelHandle model;
        std::vector<Resolution> resolutions;

        int fov = 100;
        int fov_minimum = 10;
        int fov_maximum = 150;

        Rml::String resolution_label;
        Rml::String video_status;
        Rml::String presentation_label;
        Rml::String vsync_status;
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
        bool category_game = true;
        bool category_graphics = false;
        bool category_audio = false;
        bool category_mouse_keyboard = false;

        static RmlSettings *instance;
    };
}

#endif
