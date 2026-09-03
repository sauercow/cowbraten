#include "rml_settings.h"

#include "rml_action.h"
#include "settings_api.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <cstdio>

namespace cowui
{
    RmlSettings *RmlSettings::instance = nullptr;

    static void appendresolution(
        const settings::Resolution &resolution,
        void *data)
    {
        std::vector<settings::Resolution> &resolutions =
            *static_cast<std::vector<settings::Resolution> *>(data);
        resolutions.push_back(resolution);
    }

    static bool resolutionorder(
        const settings::Resolution &left,
        const settings::Resolution &right)
    {
        if(left.width != right.width)
            return left.width > right.width;
        return left.height > right.height;
    }

    static bool sameresolution(
        const settings::Resolution &left,
        const settings::Resolution &right)
    {
        return left.width == right.width &&
            left.height == right.height;
    }

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

        bool valid = true;
        if(auto resolution = constructor.RegisterStruct<Resolution>())
        {
            valid =
                resolution.RegisterMember("label", &Resolution::label) &&
                resolution.RegisterMember("width", &Resolution::width) &&
                resolution.RegisterMember("height", &Resolution::height) &&
                resolution.RegisterMember(
                    "selected",
                    &Resolution::selected
                ) &&
                resolution.RegisterMember(
                    "supported",
                    &Resolution::supported
                );
        }
        else
        {
            valid = false;
        }

        valid = valid &&
            constructor.RegisterArray<std::vector<Resolution>>() &&
            constructor.Bind("resolutions", &resolutions) &&
            constructor.Bind("fov", &fov) &&
            constructor.Bind("fov_minimum", &fov_minimum) &&
            constructor.Bind("fov_maximum", &fov_maximum) &&
            constructor.Bind("resolution_label", &resolution_label) &&
            constructor.Bind("video_status", &video_status) &&
            constructor.Bind("fullscreen", &fullscreen) &&
            constructor.Bind(
                "desktop_fullscreen",
                &desktop_fullscreen
            ) &&
            constructor.Bind(
                "resolution_enabled",
                &resolution_enabled
            ) &&
            constructor.Bind("category_game", &category_game) &&
            constructor.Bind(
                "category_graphics",
                &category_graphics
            ) &&
            constructor.BindEventCallback(
                "select_resolution",
                &RmlSettings::selectresolution,
                this
            );

        valid = valid &&
            router.registeraction(
                "settings.fov",
                changefovaction
            ) &&
            router.registeraction(
                "settings.category.game",
                gamecategoryaction
            ) &&
            router.registeraction(
                "settings.category.graphics",
                graphicscategoryaction
            ) &&
            router.registeraction(
                "settings.fullscreen",
                fullscreenaction
            );

        if(!valid)
        {
            context->RemoveDataModel("settings");
            return false;
        }

        model = constructor.GetModelHandle();
        instance = this;
        video_status = "Live engine state";
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
        resolutions.clear();
        fov = 100;
        fov_minimum = 10;
        fov_maximum = 150;
        resolution_label.clear();
        video_status.clear();
        fullscreen = false;
        desktop_fullscreen = false;
        resolution_enabled = true;
        category_game = true;
        category_graphics = false;
    }

    void RmlSettings::activate()
    {
        refresh();
    }

    void RmlSettings::videochanged()
    {
        refreshvideo();
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

        return refreshvideo();
    }

    bool RmlSettings::refreshvideo()
    {
        settings::Video current;
        if(!settings::readvideo(current))
            return false;

        if(fullscreen != current.fullscreen)
        {
            fullscreen = current.fullscreen;
            model.DirtyVariable("fullscreen");
        }

        const bool desktop_fullscreen_active =
            current.fullscreen && current.desktop_fullscreen;
        if(desktop_fullscreen != desktop_fullscreen_active)
        {
            desktop_fullscreen = desktop_fullscreen_active;
            model.DirtyVariable("desktop_fullscreen");
        }

        const bool can_select_resolution = !desktop_fullscreen_active;
        if(resolution_enabled != can_select_resolution)
        {
            resolution_enabled = can_select_resolution;
            model.DirtyVariable("resolution_enabled");
        }

        char label[64];
        std::snprintf(
            label,
            sizeof(label),
            "%d x %d",
            current.width,
            current.height
        );
        if(resolution_label != label)
        {
            resolution_label = label;
            model.DirtyVariable("resolution_label");
        }

        std::vector<settings::Resolution> available;
        settings::visitresolutions(appendresolution, &available);
        std::sort(available.begin(), available.end(), resolutionorder);
        available.erase(
            std::unique(
                available.begin(),
                available.end(),
                sameresolution
            ),
            available.end()
        );

        std::vector<Resolution> next;
        bool current_is_supported = false;
        for(std::vector<settings::Resolution>::const_iterator resolution =
                available.begin();
            resolution != available.end();
            ++resolution)
        {
            Resolution row;
            row.width = resolution->width;
            row.height = resolution->height;
            row.selected = row.width == current.width &&
                row.height == current.height;
            row.supported = true;
            std::snprintf(
                label,
                sizeof(label),
                "%d x %d",
                row.width,
                row.height
            );
            row.label = label;
            current_is_supported = current_is_supported || row.selected;
            next.push_back(row);
        }

        if(!current_is_supported)
        {
            Resolution row;
            row.width = current.width;
            row.height = current.height;
            row.selected = true;
            row.supported = false;
            std::snprintf(
                label,
                sizeof(label),
                "%d x %d / custom",
                row.width,
                row.height
            );
            row.label = label;
            next.push_back(row);
        }

        if(resolutions != next)
        {
            resolutions.swap(next);
            model.DirtyVariable("resolutions");
        }
        return true;
    }

    void RmlSettings::setcategory(Category category)
    {
        const bool game = category == Category::Game;
        const bool graphics = category == Category::Graphics;
        if(category_game != game)
        {
            category_game = game;
            model.DirtyVariable("category_game");
        }
        if(category_graphics != graphics)
        {
            category_graphics = graphics;
            model.DirtyVariable("category_graphics");
        }
    }

    void RmlSettings::selectresolution(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 2 || !resolution_enabled)
            return;

        const int width = arguments[0].Get<int>();
        const int height = arguments[1].Get<int>();
        bool supported = false;
        for(std::vector<Resolution>::const_iterator resolution =
                resolutions.begin();
            resolution != resolutions.end();
            ++resolution)
        {
            if(resolution->width == width &&
               resolution->height == height)
            {
                supported = resolution->supported;
                break;
            }
        }

        if(supported && settings::writeresolution(width, height))
            video_status = "Display resolution applied";
        else
            video_status = "Resolution change was not accepted";

        model.DirtyVariable("video_status");
        refreshvideo();
        event.StopPropagation();
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

    void RmlSettings::gamecategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Game);
        event.StopPropagation();
    }

    void RmlSettings::graphicscategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Graphics);
        event.StopPropagation();
    }

    void RmlSettings::fullscreenaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const bool requested = !instance->fullscreen;
        if(settings::writefullscreen(requested))
        {
            instance->video_status = requested
                ? "Fullscreen enabled"
                : "Windowed mode enabled";
        }
        else
        {
            instance->video_status = requested
                ? "Fullscreen is unavailable at this resolution"
                : "Windowed mode was not accepted";
        }

        instance->model.DirtyVariable("video_status");
        instance->refreshvideo();
        instance->model.DirtyVariable("fullscreen");
        event.StopPropagation();
    }
}
