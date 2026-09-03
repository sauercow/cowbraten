#include "rml_settings.h"

#include "rml_action.h"
#include "settings_api.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

#include <cstdio>

namespace cowui
{
    RmlSettings *RmlSettings::instance = nullptr;

    bool RmlSettings::init(
        Rml::Context *context,
        RmlActionRouter &router)
    {
        if(!context || instance)
            return false;

        Rml::DataModelConstructor constructor =
            context->CreateDataModel("settings");

        if(!constructor)
            return false;

        const bool valid =
            constructor.Bind("fov", &fov) &&
            constructor.Bind("fov_minimum", &fov_minimum) &&
            constructor.Bind("fov_maximum", &fov_maximum) &&
            router.registeraction(
                "settings.fov",
                changefovaction
            );

        if(!valid)
        {
            context->RemoveDataModel("settings");
            return false;
        }

        model = constructor.GetModelHandle();
        instance = this;
        refresh();
        return true;
    }

    void RmlSettings::shutdown(Rml::Context *context)
    {
        if(instance == this)
            instance = nullptr;

        if(context && model)
            context->RemoveDataModel("settings");

        model = Rml::DataModelHandle();
        fov = 100;
        fov_minimum = 10;
        fov_maximum = 150;
    }

    void RmlSettings::activate()
    {
        refresh();
    }

    bool RmlSettings::refresh()
    {
        settings::Fov current;
        if(!model || !settings::readfov(current))
            return false;

        if(fov != current.value)
        {
            fov = current.value;
            model.DirtyVariable("fov");
        }

        if(fov_minimum != current.minimum)
        {
            fov_minimum = current.minimum;
            model.DirtyVariable("fov_minimum");
        }

        if(fov_maximum != current.maximum)
        {
            fov_maximum = current.maximum;
            model.DirtyVariable("fov_maximum");
        }

        return true;
    }

    void RmlSettings::changefovaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->fov
        );

        if(!settings::writefov(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: failed to write the FOV setting\n"
            );
        }

        instance->refresh();
        event.StopPropagation();
    }
}
