#pragma once

#include "legacy_gui_snapshot.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <vector>

namespace Rml { class Context; class Event; }

namespace cowui
{
    class RmlLegacyGui final : public LegacyGuiSink
    {
    public:
        bool init(Rml::Context *context);
        void shutdown(Rml::Context *context);
        bool activate(const char *name);
        void update();

        void begin(const char *name, const char *title) override;
        void control(LegacyGuiControlType type, int activation,
            const char *label, const char *value, const char *icon,
            int minimum, int maximum, bool active) override;
        void end() override;

    private:
        struct Row
        {
            int activation = -1, minimum = 0, maximum = 0;
            Rml::String label, value, icon;
            bool active = false;
            bool text = false, title = false, button = false;
            bool separator = false, toggle = false, slider = false;
            bool field = false, image = false, tab = false;
        };

        void run(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &arguments);
        void change(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &arguments);
        bool recapture();

        std::vector<Row> rows;
        Rml::DataModelHandle model;
        Rml::String menu_name, menu_title;
        bool pending_refresh = false;
    };
}
