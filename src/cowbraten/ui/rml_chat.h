#ifndef COWBRATEN_UI_RML_CHAT_H
#define COWBRATEN_UI_RML_CHAT_H

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <string>
#include <vector>

namespace Rml
{
    class Context;
    class ElementDocument;
}

namespace cowui
{
    class RmlChat final
    {
    public:
        bool init(Rml::Context *context);
        void shutdown(Rml::Context *context);
        void pushline(int type, const char *text);
        void clear();
        void update(bool menu_active, bool full_console_active);
        bool visible() const { return document_visible; }
        bool inputvisible() const { return input_active && document_visible; }
        int filterlegacyconsole(int filter) const;

    private:
        struct Message
        {
            Rml::String text;
            Rml::String rml;
            Rml::String label;
            bool team = false;
            bool chat = false;
            bool warning = false;
            bool error = false;
            bool echo = false;
            bool game = false;
            float opacity = 1.0f;
            int type = 0;
            int born = 0;
            int expires = 0;
        };

        bool loaddocument();
        void setvisible(bool value);
        void syncinput();
        static Message makemessage(int type, const char *text, int expires);
        static void appendmessage(
            std::vector<Message> &target,
            const Message &message,
            int capacity);
        static std::string plain(const char *text);

        Rml::Context *context = nullptr;
        Rml::ElementDocument *document = nullptr;
        Rml::DataModelHandle model;
        std::vector<Message> console_messages;
        std::vector<Message> messages;
        Rml::String input_before;
        Rml::String input_after;
        Rml::String input_label = "ALL";
        bool input_active = false;
        bool input_team = false;
        bool document_visible = false;
        bool load_attempted = false;
    };
}

#endif
