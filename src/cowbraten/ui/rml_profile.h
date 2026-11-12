#ifndef COWBRATEN_UI_RML_PROFILE_H
#define COWBRATEN_UI_RML_PROFILE_H

#include "profile_api.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <vector>

namespace Rml
{
    class Context;
    class ElementDocument;
    class Event;
}

namespace cowui
{
    class RmlActionRouter;

    class RmlProfile final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);

        void activate();
        void update();
        void renderpreview(
            Rml::ElementDocument *document,
            int context_width,
            int context_height);

    private:
        struct Row
        {
            int id = -1;
            Rml::String name;
            Rml::String story;
            Rml::String icon;
            bool ragdoll = false;
        };

        struct NamePreset
        {
            Rml::String value;
        };

        bool loadcharacters();
        void reloadnames();
        void syncselection(int value);
        void syncname(const std::string &value, bool sync_input);
        void setanimation(profile::PreviewAnimation value, const char *name);

        void selectcharacter(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments);
        void setscale(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments);
        void setnameinput(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments);
        void selectname(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments);
        void deletename(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments);

        static void applynameaction(Rml::Event &event);
        static void savenameaction(Rml::Event &event);
        static void togglenamesaction(Rml::Event &event);
        static void idleaction(Rml::Event &event);
        static void runaction(Rml::Event &event);
        static void attackaction(Rml::Event &event);
        static void jumpaction(Rml::Event &event);
        static void deathaction(Rml::Event &event);
        static void dragstartaction(Rml::Event &event);
        static void dragaction(Rml::Event &event);
        static void dragendaction(Rml::Event &event);
        static void scrollaction(Rml::Event &event);

        std::vector<Row> characters;
        std::vector<NamePreset> saved_names;
        Rml::DataModelHandle model;

        int selected_id = 0;
        Rml::String current_name;
        Rml::String name_input;
        Rml::String name_status;
        Rml::String selected_name;
        Rml::String selected_story;
        Rml::String animation_name = "Idle";
        float preview_scale = 1.0f;
        Rml::String preview_scale_text = "1.00x";
        bool saved_names_open = false;

        profile::PreviewAnimation animation = profile::PreviewAnimation::Idle;
        float preview_yaw = 150.0f;
        float drag_x = 0.0f;
        bool dragging = false;
        bool characters_loaded = false;

        static RmlProfile *instance;
    };
}

#endif
