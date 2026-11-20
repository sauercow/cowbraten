#pragma once

#include "legacy_gui_snapshot.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <vector>

namespace Rml { class Context; class Event; class ElementDocument; }

namespace cowui
{
    class RmlLegacyGui final : public LegacyGuiSink
    {
    public:
        bool init(Rml::Context *context);
        void shutdown(Rml::Context *context);
        bool activate(const char *name);
        void update();
        void renderpreviews(Rml::ElementDocument *document,
            int context_width, int context_height);

        void begin(const char *name, const char *title) override;
        void control(LegacyGuiControlType type, int activation,
            const char *label, const char *value, const char *display_value,
            const char *icon,
            int minimum, int maximum, bool active) override;
        void end() override;

    private:
        struct Row
        {
            int activation = -1, minimum = 0, maximum = 0, tab_index = 0;
            Rml::String label, value, display_value, icon, element_id;
            Rml::String progress_width = "0%";
            bool active = false;
            bool visible = true;
            bool text = false, title = false, button = false;
            bool separator = false, toggle = false, slider = false;
            bool field = false, image = false, editor = false;
            bool progress = false, preview = false, tab = false;
        };

        void run(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &arguments);
        void change(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &arguments);
        void selecttab(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &arguments);
        void updatevisibility();
        bool recapture();

        std::vector<Row> rows;
        Rml::DataModelHandle model;
        Rml::String menu_name, menu_title;
        int capture_tab = -1, selected_tab = 0;
        bool pending_refresh = false;
        bool capture_complete = true;
    };
}
