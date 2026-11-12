#include "rml_demo_player.h"

#include "rml_action.h"
#include "sauer_demo_bridge.h"
#include "ui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <cstdio>

namespace cowui
{
    RmlDemoPlayer *RmlDemoPlayer::instance = nullptr;

    namespace
    {
        void stop_event(Rml::Event &event) { event.StopPropagation(); }

        Rml::String time_text(int milliseconds)
        {
            milliseconds = std::max(milliseconds, 0);
            const int seconds = milliseconds / 1000;
            return Rml::CreateString(
                "%d:%02d.%03d",
                seconds / 60,
                seconds % 60,
                milliseconds % 1000
            );
        }
    }

    bool RmlDemoPlayer::init(Rml::Context *new_context, RmlActionRouter &router)
    {
        if(!new_context || instance) return false;
        Rml::DataModelConstructor constructor = new_context->CreateDataModel("demo_player");
        if(!constructor) return false;

        bool valid = constructor.Bind("active", &active) &&
            constructor.Bind("interactive", &interactive) &&
            constructor.Bind("paused", &paused) &&
            constructor.Bind("map_name", &map_name) &&
            constructor.Bind("mode_name", &mode_name) &&
            constructor.Bind("camera_name", &camera_name) &&
            constructor.Bind("position_text", &position_text) &&
            constructor.Bind("duration_text", &duration_text) &&
            constructor.Bind("speed_text", &speed_text) &&
            constructor.Bind("progress_width", &progress_width) &&
            constructor.Bind("cursor_left", &cursor_left) &&
            constructor.Bind("cursor_top", &cursor_top) &&
            constructor.Bind("cursor_size", &cursor_size) &&
            router.registeraction("demo-player.toggle", togglepause) &&
            router.registeraction("demo-player.rewind", rewind) &&
            router.registeraction("demo-player.forward", forward) &&
            router.registeraction("demo-player.step-back", stepback) &&
            router.registeraction("demo-player.step-forward", stepforward) &&
            router.registeraction("demo-player.speed", cyclespeed) &&
            router.registeraction("demo-player.seek", seek) &&
            router.registeraction("demo-player.stop", stop) &&
            router.registeraction("demo-player.library", openlibrary);

        if(!valid)
        {
            new_context->RemoveDataModel("demo_player");
            return false;
        }

        context = new_context;
        model = constructor.GetModelHandle();
        instance = this;
        return true;
    }

    void RmlDemoPlayer::shutdown(Rml::Context *old_context)
    {
        if(document) document->Close();
        document = nullptr;
        document_visible = false;
        if(instance == this) instance = nullptr;
        if(old_context && model) old_context->RemoveDataModel("demo_player");
        model = Rml::DataModelHandle();
        context = nullptr;
    }

    bool RmlDemoPlayer::loaddocument()
    {
        if(document) return true;
        if(load_attempted || !context) return false;
        load_attempted = true;
        document = context->LoadDocument("data/cowbraten/ui/default/screens/demo_player.rml");
        if(document && !applytheme(document))
        {
            document->Close();
            document = nullptr;
        }
        if(!document)
            std::fprintf(stderr, "Cowbraten UI: failed to load demo player overlay\n");
        return document != nullptr;
    }

    void RmlDemoPlayer::setvisible(bool value)
    {
        if(value == document_visible) return;
        if(value)
        {
            if(!loaddocument()) return;
            document->Show();
        }
        else if(document) document->Hide();
        document_visible = value;
    }

    void RmlDemoPlayer::sync()
    {
#define SYNC(variable, value) do { const auto next = (value); if(variable != next) { variable = next; model.DirtyVariable(#variable); } } while(0)
        SYNC(active, sauerdemo::playback() && sauerdemo::playback_hud_enabled());
        SYNC(interactive, active && sauerdemo::playback_interactive());
        if(!active)
            return;
        SYNC(paused, sauerdemo::paused());
        SYNC(position, sauerdemo::playback_position());
        SYNC(duration, std::max(sauerdemo::playback_duration(), 1));
        SYNC(map_name, Rml::String(sauerdemo::playback_map()));
        SYNC(mode_name, Rml::String(sauerdemo::playback_mode()));
        SYNC(camera_name, Rml::String(sauerdemo::playback_camera()));
        SYNC(position_text, time_text(position));
        SYNC(duration_text, time_text(duration));
        SYNC(progress_width, Rml::CreateString("%.3f%%", 100.0 * position / duration));
        const int speed = sauerdemo::playback_speed();
        SYNC(speed_text, Rml::CreateString("%d.%02dx", speed / 100, speed % 100));
        float cursor_x = 0.5f, cursor_y = 0.5f;
        sauerdemo::playback_cursor(cursor_x, cursor_y);
        SYNC(cursor_left, Rml::CreateString("%.3f%%", cursor_x * 100.0f));
        SYNC(cursor_top, Rml::CreateString("%.3f%%", cursor_y * 100.0f));
        SYNC(cursor_size, Rml::CreateString("%.3fpx", sauerdemo::playback_cursor_size()));
#undef SYNC
    }

    void RmlDemoPlayer::update(bool menu_active)
    {
        sync();
        setvisible(active && !menu_active);
    }

    bool RmlDemoPlayer::receivesinput() const
    {
        return document_visible && sauerdemo::playback_interactive();
    }

    void RmlDemoPlayer::togglepause(Rml::Event &event) { sauerdemo::toggle_pause(); stop_event(event); }
    void RmlDemoPlayer::rewind(Rml::Event &event) { sauerdemo::seek_relative(-5000); stop_event(event); }
    void RmlDemoPlayer::forward(Rml::Event &event) { sauerdemo::seek_relative(5000); stop_event(event); }
    void RmlDemoPlayer::stepback(Rml::Event &event) { sauerdemo::step(-1); stop_event(event); }
    void RmlDemoPlayer::stepforward(Rml::Event &event) { sauerdemo::step(1); stop_event(event); }
    void RmlDemoPlayer::cyclespeed(Rml::Event &event) { sauerdemo::cycle_speed(); stop_event(event); }
    void RmlDemoPlayer::seek(Rml::Event &event)
    {
        Rml::Element *element = event.GetCurrentElement();
        if(element)
        {
            const float width = element->GetClientWidth();
            const float mouse = event.GetParameter<float>("mouse_x", element->GetAbsoluteLeft());
            const float ratio = width > 0
                ? std::max(0.0f, std::min(1.0f, (mouse - element->GetAbsoluteLeft()) / width))
                : 0.0f;
            sauerdemo::seek_absolute(int(ratio * sauerdemo::playback_duration() + 0.5f));
        }
        stop_event(event);
    }
    void RmlDemoPlayer::stop(Rml::Event &event) { sauerdemo::stop(); stop_event(event); }
    void RmlDemoPlayer::openlibrary(Rml::Event &event) { showdemos(); stop_event(event); }
}
