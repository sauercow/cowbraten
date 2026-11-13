#ifndef COWBRATEN_UI_RML_EDITOR_H
#define COWBRATEN_UI_RML_EDITOR_H

#include "editor_api.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <vector>

namespace Rml { class Context; class Event; }

namespace cowui
{
    class RmlActionRouter;

    class RmlEditor final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);
        void activate(int requested_section = -1);

    private:
        struct ItemRow
        {
            int id = 0;
            Rml::String label;
            Rml::String detail;
        };
        struct ModelRow
        {
            int id = 0;
            Rml::String name;
        };
        struct ChoiceRow
        {
            int value = 0;
            Rml::String label;
            bool selected = false;
        };

        void buildcatalog();
        void refresh();
        void setsection(int value);
        void rebuildmodels();
        void syncchoices();
        void setstatus(bool ok, const char *success, const char *failure);

        void runaction(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void settoggle(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void createentity(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void applymaterial(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void applypostfx(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void createmapmodel(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setmodelfilter(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setcolor(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setnumeric(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setlightvalue(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void selecttriggermodel(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void selecttriggerbehavior(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void selecttriggertag(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setmapname(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setmapsize(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setselectionattr(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);

        static void overviewsection(Rml::Event &);
        static void entitiessection(Rml::Event &);
        static void lightingsection(Rml::Event &);
        static void materialssection(Rml::Event &);
        static void mapsection(Rml::Event &);
        static void selectionsection(Rml::Event &);
        static void createlightaction(Rml::Event &);
        static void createtriggeraction(Rml::Event &);
        static void savemapaction(Rml::Event &);
        static void loadmapaction(Rml::Event &);
        static void newmapaction(Rml::Event &);
        static void applyselectionaction(Rml::Event &);
        static void refreshselectionaction(Rml::Event &);
        static RmlEditor *instance;

        Rml::DataModelHandle model;
        std::vector<ItemRow> edit_actions, brush_actions, entity_presets;
        std::vector<ItemRow> materials, postfx;
        std::vector<ModelRow> all_models, visible_models;
        std::vector<ChoiceRow> trigger_models, trigger_behaviors, trigger_tags;

        Rml::String status, map_name, model_filter;
        Rml::String selection_type;
        int section = 0, visible_model_count = 0, new_map_size = 12;
        int sun_r = 0, sun_g = 0, sun_b = 0;
        int sky_r = 0, sky_g = 0, sky_b = 0;
        int ambient_r = 0, ambient_g = 0, ambient_b = 0;
        int fog_r = 0, fog_g = 0, fog_b = 0;
        int sunlight_pitch = 0, sunlight_yaw = 0, fog_distance = 4000;
        int light_radius = 128, light_r = 255, light_g = 255, light_b = 255;
        int trigger_model = -1, trigger_behavior = 0, trigger_tag = 0;
        int selection_attr0 = 0, selection_attr1 = 0, selection_attr2 = 0;
        int selection_attr3 = 0, selection_attr4 = 0;
        bool editing = false, all_faces = false, outline = false;
        bool wireframe = false, entity_snap = false, entity_editing = false;
        bool fullbright = false, show_materials = false;
        bool selection_available = false;
        bool section_overview = true, section_entities = false;
        bool section_lighting = false, section_materials = false;
        bool section_map = false, section_selection = false;
        bool catalog_built = false;
    };
}

#endif
