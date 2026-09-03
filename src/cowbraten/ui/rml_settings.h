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
    class RmlActionRouter;

    class RmlSettings final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);

        void activate();
        void videochanged();

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
            Graphics
        };

        bool refresh();
        bool refreshvideo();
        void setcategory(Category category);
        void selectresolution(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        static void changefovaction(Rml::Event &event);
        static void gamecategoryaction(Rml::Event &event);
        static void graphicscategoryaction(Rml::Event &event);
        static void fullscreenaction(Rml::Event &event);

        Rml::DataModelHandle model;
        std::vector<Resolution> resolutions;

        int fov = 100;
        int fov_minimum = 10;
        int fov_maximum = 150;

        Rml::String resolution_label;
        Rml::String video_status;
        bool fullscreen = false;
        bool desktop_fullscreen = false;
        bool resolution_enabled = true;
        bool category_game = true;
        bool category_graphics = false;

        static RmlSettings *instance;
    };
}

#endif
