#include "rml_console.h"

#include "rml_console_text.h"
#include "sauer_console_bridge.h"
#include "ui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace cowui
{
    bool RmlConsole::init(Rml::Context *new_context)
    {
        if(!new_context || context)
            return false;

        Rml::DataModelConstructor constructor =
            new_context->CreateDataModel("console");
        if(!constructor)
            return false;

        bool valid = true;
        if(auto line = constructor.RegisterStruct<Line>())
        {
            valid = line.RegisterMember("text", &Line::text) &&
                line.RegisterMember("rml", &Line::rml) &&
                line.RegisterMember("warning", &Line::warning) &&
                line.RegisterMember("error", &Line::error) &&
                line.RegisterMember("chat", &Line::chat) &&
                line.RegisterMember("team", &Line::team) &&
                line.RegisterMember("echo", &Line::echo) &&
                line.RegisterMember("game", &Line::game);
        }
        else valid = false;

        valid = valid &&
            constructor.RegisterArray<std::vector<Line>>() &&
            constructor.Bind("lines", &lines) &&
            constructor.Bind("input_active", &input_active) &&
            constructor.Bind("input_label", &input_label) &&
            constructor.Bind("input_before", &input_before) &&
            constructor.Bind("input_after", &input_after) &&
            constructor.Bind("position_label", &position_label);

        if(!valid)
        {
            new_context->RemoveDataModel("console");
            return false;
        }

        context = new_context;
        model = constructor.GetModelHandle();
        return true;
    }

    void RmlConsole::shutdown(Rml::Context *old_context)
    {
        if(document) document->Close();
        document = nullptr;
        document_visible = false;
        load_attempted = false;
        if(old_context && model)
            old_context->RemoveDataModel("console");
        model = Rml::DataModelHandle();
        context = nullptr;
        archive.clear();
        lines.clear();
        input_active = false;
        rendered_revision = -1;
        rendered_skip = -1;
        rendered_filter = -1;
        rendered_capacity = -1;
        rendered_rows = -1;
    }

    std::string RmlConsole::plain(const char *text)
    {
        std::string result;
        if(!text) return result;
        result.reserve(std::strlen(text));
        for(const char *cursor = text; *cursor; ++cursor)
        {
            if(*cursor == '\f' && cursor[1])
            {
                ++cursor;
                continue;
            }
            result.push_back(*cursor);
        }
        return result;
    }

    RmlConsole::Line RmlConsole::makeline(int type, const char *text)
    {
        Line line;
        line.text = plain(text);
        line.rml = consoletext::markup(text);
        line.warning = sauerconsole::warning(type);
        line.error = sauerconsole::error(type);
        line.chat = sauerconsole::chat(type);
        line.team = sauerconsole::teamchat(type);
        line.echo = sauerconsole::echo(type);
        line.game = sauerconsole::gameinfo(type);
        line.type = type;
        return line;
    }

    void RmlConsole::pushline(int type, const char *text)
    {
        Line next = makeline(type, text);
        if(sauerconsole::tagged(type))
        {
            int inspected = 0;
            for(auto current = archive.rbegin(); current != archive.rend(); ++current)
            {
                if(!sauerconsole::tagged(current->type) ||
                   inspected++ >= sauerconsole::tagwindow())
                    break;
                if(sauerconsole::sametag(current->type, type))
                {
                    *current = next;
                    ++revision;
                    return;
                }
            }
        }

        archive.push_back(next);
        while(int(archive.size()) > sauerconsole::maxlines())
            archive.erase(archive.begin());
        ++revision;
    }

    void RmlConsole::clear()
    {
        archive.clear();
        lines.clear();
        ++revision;
        if(model) model.DirtyVariable("lines");
    }

    void RmlConsole::scroll(int lines)
    {
        if(!document_visible || !lines) return;
        sauerconsole::scroll(lines);
        rebuild();
    }

    bool RmlConsole::loaddocument()
    {
        if(document) return true;
        if(load_attempted || !context) return false;
        load_attempted = true;
        document = context->LoadDocument(
            "data/cowbraten/ui/default/screens/console.rml");
        if(document && !applytheme(document))
        {
            document->Close();
            document = nullptr;
        }
        if(!document)
            std::fprintf(stderr, "Cowbraten UI: failed to load console overlay\n");
        return document != nullptr;
    }

    void RmlConsole::setvisible(bool value)
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

    void RmlConsole::rebuild()
    {
        const int skip = std::max(0, sauerconsole::skip());
        const int filter = sauerconsole::filter();
        const int capacity = sauerconsole::maxlines();
        const float density = std::max(
            0.01f,
            context->GetDensityIndependentPixelRatio());
        const float height_dp = context->GetDimensions().y / density;
        const float chrome_dp = 142.0f +
            (sauerconsole::inputactive() ? 54.0f : 0.0f);
        const int visible_rows = std::max(
            1,
            std::min(80, int((height_dp - chrome_dp) / 21.0f)));
        if(rendered_revision == revision &&
           rendered_skip == skip &&
           rendered_filter == filter &&
           rendered_capacity == capacity &&
           rendered_rows == visible_rows)
            return;

        std::vector<Line> filtered;
        const int archive_begin = std::max(
            0,
            int(archive.size()) - capacity);
        filtered.reserve(archive.size() - archive_begin);
        for(auto line = archive.begin() + archive_begin;
            line != archive.end();
            ++line)
            if(sauerconsole::allowed(line->type))
                filtered.push_back(*line);

        const int end = std::max(0, int(filtered.size()) - skip);
        const int begin = std::max(0, end - visible_rows);
        lines.assign(filtered.begin() + begin, filtered.begin() + end);
        rendered_revision = revision;
        rendered_skip = skip;
        rendered_filter = filter;
        rendered_capacity = capacity;
        rendered_rows = visible_rows;
        position_label = skip > 0
            ? Rml::CreateString("SCROLL +%d", skip)
            : Rml::String("LIVE");
        model.DirtyVariable("lines");
        model.DirtyVariable("position_label");
    }

    void RmlConsole::syncinput()
    {
        const bool active = sauerconsole::inputactive();
        const char *text = active ? sauerconsole::inputtext() : "";
        const int length = int(std::strlen(text));
        const int position = active
            ? std::max(0, std::min(length,
                sauerconsole::inputposition() < 0
                    ? length
                    : sauerconsole::inputposition()))
            : 0;

#define SYNC(variable, value) do { const auto next = (value); if(variable != next) { variable = next; model.DirtyVariable(#variable); } } while(0)
        SYNC(input_active, active);
        SYNC(input_label, Rml::String(plain(sauerconsole::inputlabel())));
        SYNC(input_before, Rml::String(text, position));
        SYNC(input_after, Rml::String(text + position));
#undef SYNC
    }

    void RmlConsole::update()
    {
        const bool active = sauerconsole::active();
        if(active)
        {
            rebuild();
            syncinput();
        }
        setvisible(active);
    }
}
