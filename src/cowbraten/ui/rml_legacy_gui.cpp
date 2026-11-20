#include "rml_legacy_gui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace cowui
{
    bool RmlLegacyGui::init(Rml::Context *context)
    {
        if(!context) return false;
        Rml::DataModelConstructor constructor =
            context->CreateDataModel("legacy_gui");
        if(!constructor) return false;

        bool valid = true;
        if(auto row = constructor.RegisterStruct<Row>())
            valid = row.RegisterMember("activation", &Row::activation) &&
                row.RegisterMember("minimum", &Row::minimum) &&
                row.RegisterMember("maximum", &Row::maximum) &&
                row.RegisterMember("tab_index", &Row::tab_index) &&
                row.RegisterMember("label", &Row::label) &&
                row.RegisterMember("value", &Row::value) &&
                row.RegisterMember("display_value", &Row::display_value) &&
                row.RegisterMember("icon", &Row::icon) &&
                row.RegisterMember("element_id", &Row::element_id) &&
                row.RegisterMember("progress_width", &Row::progress_width) &&
                row.RegisterMember("active", &Row::active) &&
                row.RegisterMember("visible", &Row::visible) &&
                row.RegisterMember("text", &Row::text) &&
                row.RegisterMember("title", &Row::title) &&
                row.RegisterMember("button", &Row::button) &&
                row.RegisterMember("separator", &Row::separator) &&
                row.RegisterMember("toggle", &Row::toggle) &&
                row.RegisterMember("slider", &Row::slider) &&
                row.RegisterMember("field", &Row::field) &&
                row.RegisterMember("image", &Row::image) &&
                row.RegisterMember("editor", &Row::editor) &&
                row.RegisterMember("progress", &Row::progress) &&
                row.RegisterMember("preview", &Row::preview) &&
                row.RegisterMember("tab", &Row::tab);
        else valid = false;

        valid = valid && constructor.RegisterArray<std::vector<Row>>() &&
            constructor.Bind("menu_title", &menu_title) &&
            constructor.Bind("controls", &rows) &&
            constructor.BindEventCallback("run_control", &RmlLegacyGui::run, this) &&
            constructor.BindEventCallback("change_control", &RmlLegacyGui::change, this) &&
            constructor.BindEventCallback("select_tab", &RmlLegacyGui::selecttab, this);
        if(!valid)
        {
            context->RemoveDataModel("legacy_gui");
            return false;
        }
        model = constructor.GetModelHandle();
        return true;
    }

    void RmlLegacyGui::shutdown(Rml::Context *context)
    {
        if(context && model) context->RemoveDataModel("legacy_gui");
        model = Rml::DataModelHandle();
        rows.clear(); menu_name.clear(); menu_title.clear();
        pending_refresh = false;
        capture_complete = true;
    }

    bool RmlLegacyGui::activate(const char *name)
    {
        menu_name = name ? name : "";
        selected_tab = 0;
        if(!openlegacygui(menu_name.c_str())) return false;
        if(recapture()) return true;
        releaselegacygui();
        return false;
    }

    void RmlLegacyGui::update()
    {
        if(pending_refresh) recapture();
    }

    bool RmlLegacyGui::recapture()
    {
        pending_refresh = false;
        capture_complete = true;
        rows.clear();
        const bool result = !menu_name.empty() &&
            capturelegacygui(menu_name.c_str(), *this);
        if(model)
        {
            model.DirtyVariable("menu_title");
            model.DirtyVariable("controls");
        }
        return result && capture_complete;
    }

    void RmlLegacyGui::begin(const char *, const char *title)
    {
        capture_complete = true;
        capture_tab = -1;
        menu_title = title ? title : "Menu";
    }

    void RmlLegacyGui::control(LegacyGuiControlType type, int activation,
        const char *label, const char *value, const char *display_value,
        const char *icon,
        int minimum, int maximum, bool active)
    {
        if(type == LegacyGuiControlType::Unsupported)
        {
            capture_complete = false;
            return;
        }
        if(type == LegacyGuiControlType::ListBegin ||
           type == LegacyGuiControlType::ListEnd) return;
        if(type == LegacyGuiControlType::Tab)
        {
            ++capture_tab;
            if(label && menu_title == label && capture_tab == 0) return;
        }
        if(capture_tab < 0) capture_tab = 0;
        Row row;
        row.activation = activation; row.minimum = minimum; row.maximum = maximum;
        row.tab_index = capture_tab;
        row.label = label ? label : ""; row.value = value ? value : "";
        row.display_value = display_value && display_value[0]
            ? display_value : row.value;
        row.icon = icon ? icon : ""; row.active = active;
        row.visible = type == LegacyGuiControlType::Tab ||
            capture_tab == selected_tab;
        row.text = type == LegacyGuiControlType::Text;
        row.title = type == LegacyGuiControlType::Title;
        row.button = type == LegacyGuiControlType::Button;
        row.separator = type == LegacyGuiControlType::Separator;
        row.toggle = type == LegacyGuiControlType::Checkbox ||
            type == LegacyGuiControlType::Radio || type == LegacyGuiControlType::Bitfield;
        row.slider = type == LegacyGuiControlType::Slider;
        row.field = type == LegacyGuiControlType::Field || type == LegacyGuiControlType::KeyField;
        row.image = type == LegacyGuiControlType::Image;
        if(row.image && !row.value.empty() && row.value[0] != '?')
            row.value = Rml::String("?cowbraten-resource:") + row.value;
        row.editor = type == LegacyGuiControlType::Editor;
        row.progress = type == LegacyGuiControlType::Progress;
        if(row.progress)
            row.progress_width = Rml::CreateString("%.2f%%",
                std::atof(row.value.c_str()) * 100.0);
        row.preview = type == LegacyGuiControlType::Preview;
        if(row.preview)
            row.element_id = Rml::CreateString("legacy-preview-%d",
                int(rows.size()));
        row.tab = type == LegacyGuiControlType::Tab;
        rows.push_back(row);
    }

    void RmlLegacyGui::end()
    {
        bool selected_exists = false;
        for(const Row &row : rows)
            if(!row.tab && row.tab_index == selected_tab)
            {
                selected_exists = true;
                break;
            }
        if(!selected_exists) selected_tab = 0;
        updatevisibility();
    }

    void RmlLegacyGui::updatevisibility()
    {
        for(Row &row : rows)
            row.visible = row.tab || row.tab_index == selected_tab;
    }

    void RmlLegacyGui::selecttab(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1) return;
        event.StopPropagation();
        selected_tab = arguments[0].Get<int>();
        updatevisibility();
        model.DirtyVariable("controls");
    }

    void RmlLegacyGui::renderpreviews(Rml::ElementDocument *document,
        int context_width, int context_height)
    {
        if(!document) return;
        for(const Row &row : rows)
        {
            if(!row.preview || row.element_id.empty()) continue;
            Rml::Element *element = document->GetElementById(row.element_id);
            if(!element) continue;
            const Rml::Vector2f offset =
                element->GetAbsoluteOffset(Rml::BoxArea::Content);
            const Rml::Vector2f size =
                element->GetBox().GetSize(Rml::BoxArea::Content);
            int x = int(std::floor(offset.x + 0.5f));
            int top = int(std::floor(offset.y + 0.5f));
            int width = int(std::floor(size.x + 0.5f));
            int height = int(std::floor(size.y + 0.5f));
            x = std::max(0, std::min(context_width, x));
            top = std::max(0, std::min(context_height, top));
            width = std::max(0, std::min(context_width - x, width));
            height = std::max(0, std::min(context_height - top, height));
            const int third = row.icon == "player"
                ? std::atoi(row.value.c_str()) : 0;
            renderlegacyguipreview(row.icon.c_str(),
                row.icon == "player" ? "" : row.value.c_str(),
                row.minimum, row.maximum, third,
                x, context_height - top - height, width, height);
        }
    }

    void RmlLegacyGui::run(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1) return;
        event.StopPropagation();
        if(activatelegacygui(arguments[0].Get<int>())) pending_refresh = true;
    }

    void RmlLegacyGui::change(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 2) return;
        event.StopPropagation();
        const Rml::String value = arguments[1].Get<Rml::String>();
        if(activatelegacygui(arguments[0].Get<int>(), value.c_str()))
            pending_refresh = true;
    }
}
