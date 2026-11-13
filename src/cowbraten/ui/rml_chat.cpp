#include "rml_chat.h"

#include "sauer_chat_bridge.h"
#include "sauer_console_bridge.h"
#include "rml_console_text.h"
#include "ui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cowui
{
    bool RmlChat::init(Rml::Context *new_context)
    {
        if(!new_context || context)
            return false;

        Rml::DataModelConstructor constructor =
            new_context->CreateDataModel("chat");
        if(!constructor)
            return false;

        bool valid = true;
        if(auto message = constructor.RegisterStruct<Message>())
        {
            valid = message.RegisterMember("text", &Message::text) &&
                message.RegisterMember("rml", &Message::rml) &&
                message.RegisterMember("label", &Message::label) &&
                message.RegisterMember("team", &Message::team) &&
                message.RegisterMember("chat", &Message::chat) &&
                message.RegisterMember("warning", &Message::warning) &&
                message.RegisterMember("error", &Message::error) &&
                message.RegisterMember("echo", &Message::echo) &&
                message.RegisterMember("game", &Message::game) &&
                message.RegisterMember("opacity", &Message::opacity);
        }
        else valid = false;

        valid = valid &&
            constructor.RegisterArray<std::vector<Message>>() &&
            constructor.Bind("console_messages", &console_messages) &&
            constructor.Bind("messages", &messages) &&
            constructor.Bind("input_active", &input_active) &&
            constructor.Bind("input_team", &input_team) &&
            constructor.Bind("input_label", &input_label) &&
            constructor.Bind("input_before", &input_before) &&
            constructor.Bind("input_after", &input_after);

        if(!valid)
        {
            new_context->RemoveDataModel("chat");
            return false;
        }

        context = new_context;
        model = constructor.GetModelHandle();
        return true;
    }

    void RmlChat::shutdown(Rml::Context *old_context)
    {
        if(document) document->Close();
        document = nullptr;
        document_visible = false;
        load_attempted = false;
        if(old_context && model)
            old_context->RemoveDataModel("chat");
        model = Rml::DataModelHandle();
        context = nullptr;
        console_messages.clear();
        messages.clear();
        input_active = false;
    }

    std::string RmlChat::plain(const char *text)
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

    RmlChat::Message RmlChat::makemessage(
        int type,
        const char *text,
        int expires)
    {
        bool team = false;
        Message message;
        message.text = plain(text);
        message.rml = consoletext::markup(text);
        message.chat = sauerchat::classifyline(type, team);
        message.team = team;
        message.label = message.chat
            ? Rml::String(team ? "TEAM" : "ALL")
            : Rml::String();
        message.warning = sauerconsole::warning(type);
        message.error = sauerconsole::error(type);
        message.echo = sauerconsole::echo(type);
        message.game = sauerconsole::gameinfo(type);
        message.type = type;
        message.born = sauerchat::now();
        message.expires = expires;
        return message;
    }

    void RmlChat::appendmessage(
        std::vector<Message> &target,
        const Message &message,
        int capacity)
    {
        if(sauerconsole::tagged(message.type))
        {
            int inspected = 0;
            for(auto current = target.rbegin(); current != target.rend(); ++current)
            {
                if(!sauerconsole::tagged(current->type) ||
                   inspected++ >= sauerconsole::tagwindow())
                    break;
                if(sauerconsole::sametag(current->type, message.type))
                {
                    *current = message;
                    return;
                }
            }
        }

        target.push_back(message);
        const int limit = std::max(1, capacity);
        while(int(target.size()) > limit)
            target.erase(target.begin());
    }

    void RmlChat::pushline(int type, const char *text)
    {
        const int current = sauerchat::now();
        bool changed = false;
        if(sauerchat::topenabled(type))
        {
            const int fade = sauerchat::topfade();
            appendmessage(
                console_messages,
                makemessage(type, text, fade ? current + fade * 1000 : 0),
                sauerchat::topcapacity());
            changed = true;
        }
        if(sauerchat::minienabled(type))
        {
            const int fade = sauerchat::minifade();
            appendmessage(
                messages,
                makemessage(type, text, fade ? current + fade * 1000 : 0),
                sauerchat::minicapacity());
            changed = true;
        }
        if(!changed) return;

        if(model) model.DirtyVariable("console_messages");
        if(model) model.DirtyVariable("messages");
    }

    void RmlChat::clear()
    {
        console_messages.clear();
        messages.clear();
        if(model) model.DirtyVariable("console_messages");
        if(model) model.DirtyVariable("messages");
    }

    bool RmlChat::loaddocument()
    {
        if(document) return true;
        if(load_attempted || !context) return false;
        load_attempted = true;
        document = context->LoadDocument(
            "data/cowbraten/ui/default/screens/chat.rml");
        if(document && !applytheme(document))
        {
            document->Close();
            document = nullptr;
        }
        if(!document)
            std::fprintf(stderr, "Cowbraten UI: failed to load chat overlay\n");
        return document != nullptr;
    }

    void RmlChat::setvisible(bool value)
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

    void RmlChat::syncinput()
    {
        const sauerchat::InputMode mode = sauerchat::inputmode();
        const bool active = mode != sauerchat::InputMode::None;
        const bool team = mode == sauerchat::InputMode::Team;
        const char *text = active ? sauerchat::inputtext() : "";
        const int length = int(std::strlen(text));
        const int position = active
            ? std::max(0, std::min(length,
                sauerchat::inputposition() < 0
                    ? length
                    : sauerchat::inputposition()))
            : 0;

#define SYNC(variable, value) do { const auto next = (value); if(variable != next) { variable = next; model.DirtyVariable(#variable); } } while(0)
        SYNC(input_active, active);
        SYNC(input_team, team);
        SYNC(input_label, Rml::String(team ? "TEAM" : "ALL"));
        SYNC(input_before, Rml::String(text, position));
        SYNC(input_after, Rml::String(text + position));
#undef SYNC
    }

    void RmlChat::update(bool menu_active, bool full_console_active)
    {
        syncinput();
        const int current = sauerchat::now();
        const auto updateopacity = [current](std::vector<Message> &target)
        {
            bool changed = false;
            for(Message &message : target)
            {
                float next = 1.0f;
                if(message.expires)
                {
                    const int duration = std::max(1, message.expires - message.born);
                    const int fade_window = std::max(250, std::min(2000, duration / 3));
                    const int remaining = message.expires - current;
                    if(remaining < fade_window)
                        next = std::max(0.0f, float(remaining) / fade_window);
                }
                if(std::fabs(message.opacity - next) >= 0.025f)
                {
                    message.opacity = next;
                    changed = true;
                }
            }
            return changed;
        };

        const bool console_opacity_changed = updateopacity(console_messages);
        const bool message_opacity_changed = updateopacity(messages);
        const auto first_console_expired = std::remove_if(
            console_messages.begin(),
            console_messages.end(),
            [current](const Message &message)
            {
                return message.expires && message.expires <= current;
            });
        if(first_console_expired != console_messages.end())
        {
            console_messages.erase(
                first_console_expired,
                console_messages.end());
            model.DirtyVariable("console_messages");
        }
        else if(console_opacity_changed)
            model.DirtyVariable("console_messages");
        const auto first_expired = std::remove_if(
            messages.begin(),
            messages.end(),
            [current](const Message &message)
            {
                return message.expires && message.expires <= current;
            });
        if(first_expired != messages.end())
        {
            messages.erase(first_expired, messages.end());
            model.DirtyVariable("messages");
        }
        else if(message_opacity_changed)
            model.DirtyVariable("messages");

        setvisible(
            !menu_active &&
            !full_console_active &&
            (input_active || !console_messages.empty() || !messages.empty()));
    }

    int RmlChat::filterlegacyconsole(int filter) const
    {
        return document ? 0 : filter;
    }
}
