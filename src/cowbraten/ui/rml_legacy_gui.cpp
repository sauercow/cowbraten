#include "rml_legacy_gui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

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
                row.RegisterMember("label", &Row::label) &&
                row.RegisterMember("value", &Row::value) &&
                row.RegisterMember("display_value", &Row::display_value) &&
                row.RegisterMember("icon", &Row::icon) &&
                row.RegisterMember("active", &Row::active) &&
                row.RegisterMember("text", &Row::text) &&
                row.RegisterMember("title", &Row::title) &&
                row.RegisterMember("button", &Row::button) &&
                row.RegisterMember("separator", &Row::separator) &&
                row.RegisterMember("toggle", &Row::toggle) &&
                row.RegisterMember("slider", &Row::slider) &&
                row.RegisterMember("field", &Row::field) &&
                row.RegisterMember("image", &Row::image) &&
                row.RegisterMember("tab", &Row::tab);
        else valid = false;

        valid = valid && constructor.RegisterArray<std::vector<Row>>() &&
            constructor.Bind("menu_title", &menu_title) &&
            constructor.Bind("controls", &rows) &&
            constructor.BindEventCallback("run_control", &RmlLegacyGui::run, this) &&
            constructor.BindEventCallback("change_control", &RmlLegacyGui::change, this);
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
        return recapture();
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
        if(type == LegacyGuiControlType::Tab && label && menu_title == label)
            return;
        Row row;
        row.activation = activation; row.minimum = minimum; row.maximum = maximum;
        row.label = label ? label : ""; row.value = value ? value : "";
        row.display_value = display_value && display_value[0]
            ? display_value : row.value;
        row.icon = icon ? icon : ""; row.active = active;
        row.text = type == LegacyGuiControlType::Text;
        row.title = type == LegacyGuiControlType::Title;
        row.button = type == LegacyGuiControlType::Button;
        row.separator = type == LegacyGuiControlType::Separator;
        row.toggle = type == LegacyGuiControlType::Checkbox ||
            type == LegacyGuiControlType::Radio || type == LegacyGuiControlType::Bitfield;
        row.slider = type == LegacyGuiControlType::Slider;
        row.field = type == LegacyGuiControlType::Field || type == LegacyGuiControlType::KeyField;
        row.image = type == LegacyGuiControlType::Image;
        row.tab = type == LegacyGuiControlType::Tab;
        rows.push_back(row);
    }

    void RmlLegacyGui::end() {}

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
