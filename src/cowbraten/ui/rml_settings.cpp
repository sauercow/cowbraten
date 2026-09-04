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
            constructor.Bind("presentation_label", &presentation_label) &&
            constructor.Bind(
                "presentation_windowed",
                &presentation_windowed
            ) &&
            constructor.Bind(
                "presentation_exclusive",
                &presentation_exclusive
            ) &&
            constructor.Bind(
                "presentation_desktop",
                &presentation_desktop
            ) &&
            constructor.Bind(
                "resolution_enabled",
                &resolution_enabled
            ) &&
            constructor.Bind("vsync", &vsync) &&
            constructor.Bind("vsync_adaptive", &vsync_adaptive) &&
            constructor.Bind("vsync_status", &vsync_status) &&
            constructor.Bind("fps_limit", &fps_limit) &&
            constructor.Bind(
                "fps_limit_minimum",
                &fps_limit_minimum
            ) &&
            constructor.Bind(
                "fps_limit_maximum",
                &fps_limit_maximum
            ) &&
            constructor.Bind("fps_limit_label", &fps_limit_label) &&
            constructor.Bind("antialiasing", &antialiasing) &&
            constructor.Bind(
                "antialiasing_label",
                &antialiasing_label
            ) &&
            constructor.Bind(
                "antialiasing_status",
                &antialiasing_status
            ) &&
            constructor.Bind("antialiasing_off", &antialiasing_off) &&
            constructor.Bind("antialiasing_2x", &antialiasing_2x) &&
            constructor.Bind("antialiasing_4x", &antialiasing_4x) &&
            constructor.Bind("antialiasing_8x", &antialiasing_8x) &&
            constructor.Bind("antialiasing_16x", &antialiasing_16x) &&
            constructor.Bind("sound_volume", &sound_volume) &&
            constructor.Bind(
                "sound_volume_minimum",
                &sound_volume_minimum
            ) &&
            constructor.Bind(
                "sound_volume_maximum",
                &sound_volume_maximum
            ) &&
            constructor.Bind(
                "sound_volume_label",
                &sound_volume_label
            ) &&
            constructor.Bind("music_volume", &music_volume) &&
            constructor.Bind(
                "music_volume_minimum",
                &music_volume_minimum
            ) &&
            constructor.Bind(
                "music_volume_maximum",
                &music_volume_maximum
            ) &&
            constructor.Bind(
                "music_volume_label",
                &music_volume_label
            ) &&
            constructor.Bind("audio_status", &audio_status) &&
            constructor.Bind(
                "mouse_sensitivity",
                &mouse_sensitivity
            ) &&
            constructor.Bind(
                "mouse_sensitivity_minimum",
                &mouse_sensitivity_minimum
            ) &&
            constructor.Bind(
                "mouse_sensitivity_maximum",
                &mouse_sensitivity_maximum
            ) &&
            constructor.Bind(
                "mouse_sensitivity_label",
                &mouse_sensitivity_label
            ) &&
            constructor.Bind("inverted_mouse", &inverted_mouse) &&
            constructor.Bind(
                "mouse_input_status",
                &mouse_input_status
            ) &&
            constructor.Bind("category_game", &category_game) &&
            constructor.Bind(
                "category_graphics",
                &category_graphics
            ) &&
            constructor.Bind(
                "category_audio",
                &category_audio
            ) &&
            constructor.Bind(
                "category_mouse_keyboard",
                &category_mouse_keyboard
            ) &&
            constructor.BindEventCallback(
                "select_resolution",
                &RmlSettings::selectresolution,
                this
            ) &&
            constructor.BindEventCallback(
                "select_antialiasing",
                &RmlSettings::selectantialiasing,
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
                "settings.category.audio",
                audiocategoryaction
            ) &&
            router.registeraction(
                "settings.category.mouse-keyboard",
                mousekeyboardcategoryaction
            ) &&
            router.registeraction(
                "settings.presentation.windowed",
                windowedaction
            ) &&
            router.registeraction(
                "settings.presentation.exclusive",
                exclusivefullscreenaction
            ) &&
            router.registeraction(
                "settings.presentation.desktop",
                desktopfullscreenaction
            ) &&
            router.registeraction(
                "settings.vsync",
                vsyncaction
            ) &&
            router.registeraction(
                "settings.fps-limit",
                changefpslimitaction
            ) &&
            router.registeraction(
                "settings.audio.sound-volume",
                changesoundvolumeaction
            ) &&
            router.registeraction(
                "settings.audio.music-volume",
                changemusicvolumeaction
            ) &&
            router.registeraction(
                "settings.input.sensitivity.decrease",
                decreasesensitivityaction
            ) &&
            router.registeraction(
                "settings.input.sensitivity.increase",
                increasesensitivityaction
            ) &&
            router.registeraction(
                "settings.input.inverted-mouse",
                invertedmouseaction
            );

        if(!valid)
        {
            context->RemoveDataModel("settings");
            return false;
        }

        model = constructor.GetModelHandle();
        instance = this;
        video_status = "Live engine state";
        vsync_status = "Runtime state";
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
        presentation_label.clear();
        vsync_status.clear();
        fps_limit_label.clear();
        presentation_windowed = true;
        presentation_exclusive = false;
        presentation_desktop = false;
        resolution_enabled = true;
        vsync = false;
        vsync_adaptive = false;
        fps_limit = 200;
        fps_limit_minimum = 0;
        fps_limit_maximum = 1000;
        antialiasing = 0;
        antialiasing_requested = -1;
        pending_antialiasing = -1;
        antialiasing_label.clear();
        antialiasing_status.clear();
        antialiasing_off = true;
        antialiasing_2x = false;
        antialiasing_4x = false;
        antialiasing_8x = false;
        antialiasing_16x = false;
        sound_volume = 255;
        sound_volume_minimum = 0;
        sound_volume_maximum = 255;
        music_volume = 128;
        music_volume_minimum = 0;
        music_volume_maximum = 255;
        sound_volume_label.clear();
        music_volume_label.clear();
        audio_status.clear();
        mouse_sensitivity = 3.0f;
        mouse_sensitivity_minimum = 0.001f;
        mouse_sensitivity_maximum = 1000.0f;
        mouse_sensitivity_label.clear();
        inverted_mouse = false;
        mouse_input_status.clear();
        category_game = true;
        category_graphics = false;
        category_audio = false;
        category_mouse_keyboard = false;
    }

    void RmlSettings::activate()
    {
        refresh();
    }

    void RmlSettings::videochanged()
    {
        refreshvideo();
    }

    void RmlSettings::applypending()
    {
        if(pending_antialiasing < 0)
            return;

        const int requested = pending_antialiasing;
        pending_antialiasing = -1;
        settings::resetgraphics();
        refreshvideo();

        char status[96];
        if(antialiasing == requested)
        {
            if(requested)
            {
                std::snprintf(
                    status,
                    sizeof(status),
                    "%dx multisample antialiasing active",
                    requested
                );
            }
            else std::snprintf(status, sizeof(status), "Antialiasing disabled");
        }
        else
        {
            std::snprintf(
                status,
                sizeof(status),
                "%dx was unavailable; runtime is %s",
                requested,
                antialiasing ? antialiasing_label.c_str() : "off"
            );
        }

        antialiasing_status = status;
        model.DirtyVariable("antialiasing_status");
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

        if(!refreshvideo())
            return false;
        if(!refreshaudio())
            return false;
        return refreshmouseinput();
    }

    bool RmlSettings::refreshvideo()
    {
        settings::Video current;
        if(!settings::readvideo(current))
            return false;

        const bool windowed = current.presentation ==
            settings::PresentationMode::Windowed;
        const bool exclusive = current.presentation ==
            settings::PresentationMode::ExclusiveFullscreen;
        const bool desktop = current.presentation ==
            settings::PresentationMode::DesktopFullscreen;

        if(presentation_windowed != windowed)
        {
            presentation_windowed = windowed;
            model.DirtyVariable("presentation_windowed");
        }
        if(presentation_exclusive != exclusive)
        {
            presentation_exclusive = exclusive;
            model.DirtyVariable("presentation_exclusive");
        }
        if(presentation_desktop != desktop)
        {
            presentation_desktop = desktop;
            model.DirtyVariable("presentation_desktop");
        }

        const char *mode_label = windowed
            ? "Windowed"
            : (desktop ? "Desktop fullscreen" : "Exclusive fullscreen");
        if(presentation_label != mode_label)
        {
            presentation_label = mode_label;
            model.DirtyVariable("presentation_label");
        }

        const bool can_select_resolution = !desktop;
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

        settings::Vsync current_vsync;
        if(!settings::readvsync(current_vsync))
            return false;

        if(vsync != current_vsync.enabled)
        {
            vsync = current_vsync.enabled;
            model.DirtyVariable("vsync");
        }
        if(vsync_adaptive != current_vsync.adaptive)
        {
            vsync_adaptive = current_vsync.adaptive;
            model.DirtyVariable("vsync_adaptive");
        }

        const char *current_vsync_status = current_vsync.adaptive
            ? "Adaptive VSync active"
            : (current_vsync.enabled ? "VSync active" : "VSync disabled");
        if(vsync_status != current_vsync_status)
        {
            vsync_status = current_vsync_status;
            model.DirtyVariable("vsync_status");
        }

        settings::FpsLimit current_fps_limit;
        if(!settings::readfpslimit(current_fps_limit))
            return false;

        if(fps_limit != current_fps_limit.value)
        {
            fps_limit = current_fps_limit.value;
            model.DirtyVariable("fps_limit");
        }
        if(fps_limit_minimum != current_fps_limit.minimum)
        {
            fps_limit_minimum = current_fps_limit.minimum;
            model.DirtyVariable("fps_limit_minimum");
        }
        if(fps_limit_maximum != current_fps_limit.maximum)
        {
            fps_limit_maximum = current_fps_limit.maximum;
            model.DirtyVariable("fps_limit_maximum");
        }

        if(current_fps_limit.value)
        {
            std::snprintf(
                label,
                sizeof(label),
                "%d FPS",
                current_fps_limit.value
            );
        }
        else std::snprintf(label, sizeof(label), "Unlimited");

        if(fps_limit_label != label)
        {
            fps_limit_label = label;
            model.DirtyVariable("fps_limit_label");
        }

        settings::Antialiasing current_antialiasing;
        if(!settings::readantialiasing(current_antialiasing))
            return false;

        antialiasing_requested =
            current_antialiasing.requested_samples;

        if(antialiasing != current_antialiasing.actual_samples)
        {
            antialiasing = current_antialiasing.actual_samples;
            model.DirtyVariable("antialiasing");
        }

        const bool aa_off = current_antialiasing.actual_samples == 0;
        const bool aa_2x = current_antialiasing.actual_samples == 2;
        const bool aa_4x = current_antialiasing.actual_samples == 4;
        const bool aa_8x = current_antialiasing.actual_samples == 8;
        const bool aa_16x = current_antialiasing.actual_samples == 16;
        if(antialiasing_off != aa_off)
        {
            antialiasing_off = aa_off;
            model.DirtyVariable("antialiasing_off");
        }
        if(antialiasing_2x != aa_2x)
        {
            antialiasing_2x = aa_2x;
            model.DirtyVariable("antialiasing_2x");
        }
        if(antialiasing_4x != aa_4x)
        {
            antialiasing_4x = aa_4x;
            model.DirtyVariable("antialiasing_4x");
        }
        if(antialiasing_8x != aa_8x)
        {
            antialiasing_8x = aa_8x;
            model.DirtyVariable("antialiasing_8x");
        }
        if(antialiasing_16x != aa_16x)
        {
            antialiasing_16x = aa_16x;
            model.DirtyVariable("antialiasing_16x");
        }

        if(current_antialiasing.actual_samples)
        {
            std::snprintf(
                label,
                sizeof(label),
                "%dx MSAA",
                current_antialiasing.actual_samples
            );
        }
        else std::snprintf(label, sizeof(label), "Off");

        if(antialiasing_label != label)
        {
            antialiasing_label = label;
            model.DirtyVariable("antialiasing_label");
        }

        char aa_status[96];
        if(current_antialiasing.requested_samples > 0 &&
           current_antialiasing.requested_samples !=
               current_antialiasing.actual_samples)
        {
            std::snprintf(
                aa_status,
                sizeof(aa_status),
                "%dx requested; runtime is %s",
                current_antialiasing.requested_samples,
                current_antialiasing.actual_samples
                    ? label
                    : "off"
            );
        }
        else if(current_antialiasing.actual_samples)
        {
            std::snprintf(
                aa_status,
                sizeof(aa_status),
                "%dx multisample antialiasing active",
                current_antialiasing.actual_samples
            );
        }
        else std::snprintf(aa_status, sizeof(aa_status), "Antialiasing disabled");

        if(antialiasing_status != aa_status)
        {
            antialiasing_status = aa_status;
            model.DirtyVariable("antialiasing_status");
        }
        return true;
    }

    bool RmlSettings::refreshaudio()
    {
        settings::Audio current;
        if(!settings::readaudio(current))
            return false;

        if(sound_volume != current.sound_volume)
        {
            sound_volume = current.sound_volume;
            model.DirtyVariable("sound_volume");
        }
        if(sound_volume_minimum != current.sound_volume_minimum)
        {
            sound_volume_minimum = current.sound_volume_minimum;
            model.DirtyVariable("sound_volume_minimum");
        }
        if(sound_volume_maximum != current.sound_volume_maximum)
        {
            sound_volume_maximum = current.sound_volume_maximum;
            model.DirtyVariable("sound_volume_maximum");
        }
        if(music_volume != current.music_volume)
        {
            music_volume = current.music_volume;
            model.DirtyVariable("music_volume");
        }
        if(music_volume_minimum != current.music_volume_minimum)
        {
            music_volume_minimum = current.music_volume_minimum;
            model.DirtyVariable("music_volume_minimum");
        }
        if(music_volume_maximum != current.music_volume_maximum)
        {
            music_volume_maximum = current.music_volume_maximum;
            model.DirtyVariable("music_volume_maximum");
        }

        char label[32];
        const int sound_percent = current.sound_volume_maximum > 0
            ? (current.sound_volume * 100 +
                current.sound_volume_maximum / 2) /
                current.sound_volume_maximum
            : 0;
        std::snprintf(label, sizeof(label), "%d%%", sound_percent);
        if(sound_volume_label != label)
        {
            sound_volume_label = label;
            model.DirtyVariable("sound_volume_label");
        }

        const int music_percent = current.music_volume_maximum > 0
            ? (current.music_volume * 100 +
                current.music_volume_maximum / 2) /
                current.music_volume_maximum
            : 0;
        std::snprintf(label, sizeof(label), "%d%%", music_percent);
        if(music_volume_label != label)
        {
            music_volume_label = label;
            model.DirtyVariable("music_volume_label");
        }

        const char *status = !current.backend_active
            ? "Audio backend unavailable"
            : (current.sound_volume
                ? "Audio backend active"
                : "All audio muted by Sound Volume");
        if(audio_status != status)
        {
            audio_status = status;
            model.DirtyVariable("audio_status");
        }
        return true;
    }

    bool RmlSettings::refreshmouseinput()
    {
        settings::MouseInput current;
        if(!settings::readmouseinput(current))
            return false;

        if(mouse_sensitivity != current.sensitivity)
        {
            mouse_sensitivity = current.sensitivity;
            model.DirtyVariable("mouse_sensitivity");
        }
        if(mouse_sensitivity_minimum != current.sensitivity_minimum)
        {
            mouse_sensitivity_minimum = current.sensitivity_minimum;
            model.DirtyVariable("mouse_sensitivity_minimum");
        }
        if(mouse_sensitivity_maximum != current.sensitivity_maximum)
        {
            mouse_sensitivity_maximum = current.sensitivity_maximum;
            model.DirtyVariable("mouse_sensitivity_maximum");
        }
        if(inverted_mouse != current.inverted)
        {
            inverted_mouse = current.inverted;
            model.DirtyVariable("inverted_mouse");
        }

        char label[32];
        std::snprintf(label, sizeof(label), "%.2f", current.sensitivity);
        if(mouse_sensitivity_label != label)
        {
            mouse_sensitivity_label = label;
            model.DirtyVariable("mouse_sensitivity_label");
        }

        const char *status = current.inverted
            ? "Vertical look inverted"
            : "Standard vertical look";
        if(mouse_input_status != status)
        {
            mouse_input_status = status;
            model.DirtyVariable("mouse_input_status");
        }
        return true;
    }

    void RmlSettings::setcategory(Category category)
    {
        const bool game = category == Category::Game;
        const bool graphics = category == Category::Graphics;
        const bool audio = category == Category::Audio;
        const bool mouse_keyboard = category ==
            Category::MouseKeyboard;
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
        if(category_audio != audio)
        {
            category_audio = audio;
            model.DirtyVariable("category_audio");
        }
        if(category_mouse_keyboard != mouse_keyboard)
        {
            category_mouse_keyboard = mouse_keyboard;
            model.DirtyVariable("category_mouse_keyboard");
        }
    }

    void RmlSettings::setpresentation(
        settings::PresentationMode presentation,
        Rml::Event &event)
    {
        if(settings::writepresentation(presentation))
        {
            switch(presentation)
            {
                case settings::PresentationMode::Windowed:
                    video_status = "Windowed mode active";
                    break;
                case settings::PresentationMode::ExclusiveFullscreen:
                    video_status = "Exclusive fullscreen active";
                    break;
                case settings::PresentationMode::DesktopFullscreen:
                    video_status = "Desktop fullscreen active";
                    break;
            }
        }
        else video_status = "Display mode change was not accepted";

        model.DirtyVariable("video_status");
        refreshvideo();
        event.StopPropagation();
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

    void RmlSettings::selectantialiasing(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        const int samples = arguments[0].Get<int>();
        const bool supported_value = samples == 0 ||
            samples == 2 ||
            samples == 4 ||
            samples == 8 ||
            samples == 16;
        const bool request_changed = samples !=
            antialiasing_requested;

        if(!supported_value || !settings::writeantialiasing(samples))
        {
            antialiasing_status = "Antialiasing request was rejected";
            model.DirtyVariable("antialiasing_status");
            event.StopPropagation();
            return;
        }

        antialiasing_requested = samples;
        if(!request_changed && samples == antialiasing)
        {
            antialiasing_status = samples
                ? "Requested value already active"
                : "Antialiasing disabled";
            model.DirtyVariable("antialiasing_status");
        }
        else
        {
            pending_antialiasing = samples;
            antialiasing_status = "Rebuilding the graphics context";
            model.DirtyVariable("antialiasing_status");
        }

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

    void RmlSettings::audiocategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::Audio);
        event.StopPropagation();
    }

    void RmlSettings::mousekeyboardcategoryaction(Rml::Event &event)
    {
        if(instance)
            instance->setcategory(Category::MouseKeyboard);
        event.StopPropagation();
    }

    void RmlSettings::windowedaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->setpresentation(
                settings::PresentationMode::Windowed,
                event
            );
        }
        else event.StopPropagation();
    }

    void RmlSettings::exclusivefullscreenaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->setpresentation(
                settings::PresentationMode::ExclusiveFullscreen,
                event
            );
        }
        else event.StopPropagation();
    }

    void RmlSettings::desktopfullscreenaction(Rml::Event &event)
    {
        if(instance)
        {
            instance->setpresentation(
                settings::PresentationMode::DesktopFullscreen,
                event
            );
        }
        else event.StopPropagation();
    }

    void RmlSettings::vsyncaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const bool requested = !instance->vsync;
        const bool accepted = settings::writevsync(requested);
        instance->refreshvideo();
        if(!accepted)
        {
            instance->vsync_status =
                "Driver rejected the VSync change";
            instance->model.DirtyVariable("vsync_status");
        }
        event.StopPropagation();
    }

    void RmlSettings::changefpslimitaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->fps_limit
        );

        if(!settings::writefpslimit(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected the FPS limit %d\n",
                requested
            );
        }

        instance->refreshvideo();
        event.StopPropagation();
    }

    void RmlSettings::changesoundvolumeaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->sound_volume
        );
        if(!settings::writesoundvolume(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected the sound volume %d\n",
                requested
            );
        }

        instance->refreshaudio();
        event.StopPropagation();
    }

    void RmlSettings::changemusicvolumeaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const int requested = event.GetParameter<int>(
            "value",
            instance->music_volume
        );
        if(!settings::writemusicvolume(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected the music volume %d\n",
                requested
            );
        }

        instance->refreshaudio();
        event.StopPropagation();
    }

    void RmlSettings::decreasesensitivityaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::max(
            instance->mouse_sensitivity_minimum,
            instance->mouse_sensitivity - 0.1f
        );
        if(!settings::writesensitivity(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected mouse sensitivity %.3f\n",
                requested
            );
        }

        instance->refreshmouseinput();
        event.StopPropagation();
    }

    void RmlSettings::increasesensitivityaction(Rml::Event &event)
    {
        if(!instance)
            return;

        const float requested = std::min(
            instance->mouse_sensitivity_maximum,
            instance->mouse_sensitivity + 0.1f
        );
        if(!settings::writesensitivity(requested))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected mouse sensitivity %.3f\n",
                requested
            );
        }

        instance->refreshmouseinput();
        event.StopPropagation();
    }

    void RmlSettings::invertedmouseaction(Rml::Event &event)
    {
        if(!instance)
            return;

        if(!settings::writeinvertedmouse(!instance->inverted_mouse))
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected inverted mouse change\n"
            );
        }

        instance->refreshmouseinput();
        event.StopPropagation();
    }
}
