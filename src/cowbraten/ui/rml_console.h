#ifndef COWBRATEN_UI_RML_CONSOLE_H
#define COWBRATEN_UI_RML_CONSOLE_H

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
    class RmlConsole final
    {
    public:
        bool init(Rml::Context *context);
        void shutdown(Rml::Context *context);
        void pushline(int type, const char *text);
        void clear();
        void update();
        void scroll(int lines);
        bool visible() const { return document_visible; }

    private:
        struct Line
        {
            Rml::String text;
            Rml::String rml;
            bool warning = false;
            bool error = false;
            bool chat = false;
            bool team = false;
            bool echo = false;
            bool game = false;
            int type = 0;
        };

        bool loaddocument();
        void setvisible(bool value);
        void rebuild();
        void syncinput();
        static std::string plain(const char *text);
        static Line makeline(int type, const char *text);

        Rml::Context *context = nullptr;
        Rml::ElementDocument *document = nullptr;
        Rml::DataModelHandle model;
        std::vector<Line> archive;
        std::vector<Line> lines;
        Rml::String input_before;
        Rml::String input_after;
        Rml::String input_label = "COMMAND";
        Rml::String position_label = "LIVE";
        bool input_active = false;
        bool document_visible = false;
        bool load_attempted = false;
        int revision = 0;
        int rendered_revision = -1;
        int rendered_skip = -1;
        int rendered_filter = -1;
        int rendered_capacity = -1;
        int rendered_rows = -1;
    };
}

#endif
