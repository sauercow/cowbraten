#include "rml_profile.h"

#include "rml_action.h"

#include <RmlUi/Core/Box.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <cmath>

namespace cowui
{
    RmlProfile *RmlProfile::instance = nullptr;

    namespace
    {
        void stop(Rml::Event &event)
        {
            event.StopPropagation();
        }

        float clamp_scale(float value)
        {
            return std::max(0.70f, std::min(1.45f, value));
        }
    }

    bool RmlProfile::init(
        Rml::Context *context,
        RmlActionRouter &router)
    {
        if(!context || instance)
            return false;

        Rml::DataModelConstructor constructor =
            context->CreateDataModel("profile");
        if(!constructor)
            return false;

        bool valid = true;
        if(auto row = constructor.RegisterStruct<Row>())
        {
            valid = row.RegisterMember("id", &Row::id) &&
                row.RegisterMember("name", &Row::name) &&
                row.RegisterMember("story", &Row::story) &&
                row.RegisterMember("icon", &Row::icon) &&
                row.RegisterMember("ragdoll", &Row::ragdoll);
        }
        else valid = false;

        if(auto preset = constructor.RegisterStruct<NamePreset>())
            valid = valid && preset.RegisterMember("value", &NamePreset::value);
        else valid = false;

        valid = valid &&
            constructor.RegisterArray<std::vector<Row>>() &&
            constructor.RegisterArray<std::vector<NamePreset>>() &&
            constructor.Bind("characters", &characters) &&
            constructor.Bind("saved_names", &saved_names) &&
            constructor.Bind("selected_id", &selected_id) &&
            constructor.Bind("current_name", &current_name) &&
            constructor.Bind("name_input", &name_input) &&
            constructor.Bind("name_status", &name_status) &&
            constructor.Bind("saved_names_open", &saved_names_open) &&
            constructor.Bind("selected_name", &selected_name) &&
            constructor.Bind("selected_story", &selected_story) &&
            constructor.Bind("model_spoofing", &model_spoofing) &&
            constructor.Bind("spoof_model_value", &spoof_model_value) &&
            constructor.Bind("spoof_model_name", &spoof_model_name) &&
            constructor.Bind("reported_model_name", &reported_model_name) &&
            constructor.Bind("animation_name", &animation_name) &&
            constructor.Bind("preview_scale", &preview_scale) &&
            constructor.Bind("preview_scale_text", &preview_scale_text) &&
            constructor.BindEventCallback(
                "select_character",
                &RmlProfile::selectcharacter,
                this) &&
            constructor.BindEventCallback(
                "set_preview_scale",
                &RmlProfile::setscale,
                this) &&
            constructor.BindEventCallback(
                "set_model_spoofing",
                &RmlProfile::setmodelspoofing,
                this) &&
            constructor.BindEventCallback(
                "select_spoof_model",
                &RmlProfile::selectspoofmodel,
                this) &&
            constructor.BindEventCallback(
                "set_name_input",
                &RmlProfile::setnameinput,
                this) &&
            constructor.BindEventCallback(
                "select_name",
                &RmlProfile::selectname,
                this) &&
            constructor.BindEventCallback(
                "delete_name",
                &RmlProfile::deletename,
                this);

        valid = valid &&
            router.registeraction("profile.name.apply", applynameaction) &&
            router.registeraction("profile.name.save", savenameaction) &&
            router.registeraction("profile.names.toggle", togglenamesaction) &&
            router.registeraction("profile.animation.idle", idleaction) &&
            router.registeraction("profile.animation.run", runaction) &&
            router.registeraction("profile.animation.attack", attackaction) &&
            router.registeraction("profile.animation.jump", jumpaction) &&
            router.registeraction("profile.animation.death", deathaction) &&
            router.registeraction("profile.preview.drag-start", dragstartaction) &&
            router.registeraction("profile.preview.drag", dragaction) &&
            router.registeraction("profile.preview.drag-end", dragendaction) &&
            router.registeraction("profile.preview.scroll", scrollaction);

        if(!valid)
        {
            context->RemoveDataModel("profile");
            return false;
        }

        model = constructor.GetModelHandle();
        instance = this;
        return true;
    }

    void RmlProfile::shutdown(Rml::Context *context)
    {
        if(instance == this)
            instance = nullptr;
        if(context && model)
            context->RemoveDataModel("profile");

        model = Rml::DataModelHandle();
        characters.clear();
        saved_names.clear();
        saved_names_open = false;
        characters_loaded = false;
        dragging = false;
    }

    bool RmlProfile::loadcharacters()
    {
        if(characters_loaded)
            return true;

        characters.clear();
        for(int index = 0; index < profile::charactercount(); ++index)
        {
            profile::Character source = {};
            if(!profile::readcharacter(index, source))
                return false;

            Row row;
            row.id = source.id;
            row.name = source.name;
            row.story = source.story;
            row.icon = source.icon;
            row.ragdoll = source.ragdoll;
            characters.push_back(row);
        }

        characters_loaded = true;
        model.DirtyVariable("characters");
        return true;
    }

    void RmlProfile::reloadnames()
    {
        saved_names.clear();
        for(int index = 0; index < profile::savednamecount(); ++index)
        {
            std::string value;
            if(!profile::readsavedname(index, value))
                continue;

            NamePreset preset;
            preset.value = value;
            saved_names.push_back(preset);
        }
        model.DirtyVariable("saved_names");
    }

    void RmlProfile::syncname(const std::string &value, bool sync_input)
    {
        if(current_name != value)
        {
            current_name = value;
            model.DirtyVariable("current_name");
        }
        if(sync_input && name_input != value)
        {
            name_input = value;
            model.DirtyVariable("name_input");
        }
    }

    void RmlProfile::syncselection(int value)
    {
        if(characters.empty())
            return;

        value = std::max(0, std::min(int(characters.size()) - 1, value));
        const Row &row = characters[value];
        if(selected_id != row.id)
        {
            selected_id = row.id;
            model.DirtyVariable("selected_id");
        }
        if(selected_name != row.name)
        {
            selected_name = row.name;
            model.DirtyVariable("selected_name");
        }
        if(selected_story != row.story)
        {
            selected_story = row.story;
            model.DirtyVariable("selected_story");
        }
        syncreportedmodelname();
    }

    void RmlProfile::syncreportedmodelname()
    {
        const Rml::String &value =
            model_spoofing ? spoof_model_name : selected_name;
        if(reported_model_name != value)
        {
            reported_model_name = value;
            model.DirtyVariable("reported_model_name");
        }
    }

    void RmlProfile::syncmodelspoofing(
        const profile::ModelSpoofing &value)
    {
        if(characters.empty())
            return;

        const int accepted =
            std::max(0, std::min(int(characters.size()) - 1, value.model));
        if(model_spoofing != value.enabled)
        {
            model_spoofing = value.enabled;
            model.DirtyVariable("model_spoofing");
        }
        if(spoof_model_id != accepted)
            spoof_model_id = accepted;

        const Rml::String selected_value = Rml::CreateString("%d", accepted);
        if(spoof_model_value != selected_value)
        {
            spoof_model_value = selected_value;
            model.DirtyVariable("spoof_model_value");
        }

        const Rml::String &name = characters[accepted].name;
        if(spoof_model_name != name)
        {
            spoof_model_name = name;
            model.DirtyVariable("spoof_model_name");
        }
        syncreportedmodelname();
    }

    void RmlProfile::activate()
    {
        if(!loadcharacters())
            return;

        int value = selected_id;
        if(profile::readselection(value))
            syncselection(value);
        profile::ModelSpoofing spoofing = {};
        if(profile::readmodelspoofing(spoofing))
            syncmodelspoofing(spoofing);
        std::string player_name;
        if(profile::readname(player_name))
            syncname(player_name, true);
        reloadnames();
        saved_names_open = false;
        model.DirtyVariable("saved_names_open");
        name_status.clear();
        model.DirtyVariable("name_status");
        dragging = false;
    }

    void RmlProfile::update()
    {
        if(!characters_loaded)
            return;

        int value = selected_id;
        if(profile::readselection(value) && value != selected_id)
            syncselection(value);

        profile::ModelSpoofing spoofing = {};
        if(profile::readmodelspoofing(spoofing) &&
           (spoofing.enabled != model_spoofing ||
            spoofing.model != spoof_model_id))
        {
            syncmodelspoofing(spoofing);
        }

        std::string player_name;
        if(profile::readname(player_name) && current_name != player_name)
        {
            const bool input_was_current = name_input == current_name;
            syncname(player_name, input_was_current);
        }
    }

    void RmlProfile::renderpreview(
        Rml::ElementDocument *document,
        int context_width,
        int context_height)
    {
        if(!document || characters.empty())
            return;

        Rml::Element *element = document->GetElementById("profile-preview");
        if(!element)
            return;

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
        const int y = context_height - top - height;

        profile::renderpreview(
            selected_id,
            animation,
            preview_yaw,
            preview_scale,
            x,
            y,
            width,
            height);
    }

    void RmlProfile::setanimation(
        profile::PreviewAnimation value,
        const char *name)
    {
        animation = value;
        if(animation_name != name)
        {
            animation_name = name;
            model.DirtyVariable("animation_name");
        }
    }

    void RmlProfile::selectcharacter(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        const int requested = arguments[0].Get<int>();
        int accepted = selected_id;
        if(profile::writeselection(requested, accepted))
            syncselection(accepted);
        stop(event);
    }

    void RmlProfile::setscale(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        const float next = clamp_scale(arguments[0].Get<float>());
        if(std::fabs(next - preview_scale) > 0.0001f)
        {
            preview_scale = next;
            preview_scale_text = Rml::CreateString("%.2fx", preview_scale);
            model.DirtyVariable("preview_scale");
            model.DirtyVariable("preview_scale_text");
        }
        stop(event);
    }

    void RmlProfile::setmodelspoofing(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
        {
            profile::ModelSpoofing accepted = {};
            if(profile::writemodelspoofingenabled(
                   arguments[0].Get<bool>(), accepted))
            {
                syncmodelspoofing(accepted);
            }
        }
        stop(event);
    }

    void RmlProfile::selectspoofmodel(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
        {
            profile::ModelSpoofing accepted = {};
            if(profile::writemodelspoofingselection(
                   arguments[0].Get<int>(), accepted))
            {
                syncmodelspoofing(accepted);
            }
        }
        stop(event);
    }

    void RmlProfile::setnameinput(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        name_input = arguments[0].Get<Rml::String>();
        model.DirtyVariable("name_input");
        stop(event);
    }

    void RmlProfile::selectname(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        std::string accepted;
        const Rml::String requested = arguments[0].Get<Rml::String>();
        if(requested.empty())
        {
            stop(event);
            return;
        }
        if(profile::writename(requested.c_str(), accepted))
        {
            syncname(accepted, true);
            saved_names_open = false;
            model.DirtyVariable("saved_names_open");
            name_status = "Name applied";
            model.DirtyVariable("name_status");
        }
        stop(event);
    }

    void RmlProfile::deletename(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        const Rml::String requested = arguments[0].Get<Rml::String>();
        if(!requested.empty() && profile::deletename(requested.c_str()))
        {
            reloadnames();
            saved_names_open = true;
            model.DirtyVariable("saved_names_open");
            name_status = "Name removed";
            model.DirtyVariable("name_status");
        }
        stop(event);
    }

    void RmlProfile::applynameaction(Rml::Event &event)
    {
        if(instance)
        {
            std::string accepted;
            if(profile::writename(instance->name_input.c_str(), accepted))
            {
                instance->syncname(accepted, true);
                instance->name_status = "Name applied";
                instance->model.DirtyVariable("name_status");
            }
        }
        stop(event);
    }

    void RmlProfile::savenameaction(Rml::Event &event)
    {
        if(instance)
        {
            std::string accepted;
            if(profile::savename(instance->name_input.c_str(), accepted))
            {
                instance->syncname(accepted, true);
                instance->reloadnames();
                instance->name_status = "Name saved";
                instance->model.DirtyVariable("name_status");
            }
        }
        stop(event);
    }

    void RmlProfile::togglenamesaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->saved_names_open = !instance->saved_names_open;
            instance->model.DirtyVariable("saved_names_open");
        }
        stop(event);
    }

    void RmlProfile::idleaction(Rml::Event &event)
    {
        if(instance) instance->setanimation(profile::PreviewAnimation::Idle, "Idle");
        stop(event);
    }

    void RmlProfile::runaction(Rml::Event &event)
    {
        if(instance) instance->setanimation(profile::PreviewAnimation::Run, "Run");
        stop(event);
    }

    void RmlProfile::attackaction(Rml::Event &event)
    {
        if(instance) instance->setanimation(profile::PreviewAnimation::Attack, "Attack");
        stop(event);
    }

    void RmlProfile::jumpaction(Rml::Event &event)
    {
        if(instance) instance->setanimation(profile::PreviewAnimation::Jump, "Jump");
        stop(event);
    }

    void RmlProfile::deathaction(Rml::Event &event)
    {
        if(instance) instance->setanimation(profile::PreviewAnimation::Death, "Death");
        stop(event);
    }

    void RmlProfile::dragstartaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->dragging = true;
            instance->drag_x = event.GetParameter<float>("mouse_x", 0.0f);
        }
        stop(event);
    }

    void RmlProfile::dragaction(Rml::Event &event)
    {
        if(instance && instance->dragging)
        {
            const float mouse_x = event.GetParameter<float>("mouse_x", instance->drag_x);
            instance->preview_yaw += (mouse_x - instance->drag_x) * 0.45f;
            instance->drag_x = mouse_x;
        }
        stop(event);
    }

    void RmlProfile::dragendaction(Rml::Event &event)
    {
        if(instance) instance->dragging = false;
        stop(event);
    }

    void RmlProfile::scrollaction(Rml::Event &event)
    {
        if(instance)
        {
            const float delta = event.GetParameter<float>("wheel_delta_y", 0.0f);
            const float next = clamp_scale(instance->preview_scale - delta * 0.05f);
            if(std::fabs(next - instance->preview_scale) > 0.0001f)
            {
                instance->preview_scale = next;
                instance->preview_scale_text = Rml::CreateString("%.2fx", next);
                instance->model.DirtyVariable("preview_scale");
                instance->model.DirtyVariable("preview_scale_text");
            }
        }
        stop(event);
    }
}
