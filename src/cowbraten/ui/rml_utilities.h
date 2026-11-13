#ifndef COWBRATEN_UI_RML_UTILITIES_H
#define COWBRATEN_UI_RML_UTILITIES_H

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

namespace Rml { class Context; class Event; }

namespace cowui
{
    class RmlActionRouter;

    class RmlUtilities final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);
        void activate(int section = -1);

    private:
        void setsection(int value);
        void setfilename(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setnotepad(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setscratchpad(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void setpastebuffer(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void loadnotepad();
        void savenotepad();
        void execute(Rml::String &text, const char *label);
        void copy(Rml::String &text);
        void paste(Rml::String &text, const char *variable);
        void clear(Rml::String &text, const char *variable);

        static void notepadsection(Rml::Event &);
        static void scratchpadsection(Rml::Event &);
        static void pastebuffersection(Rml::Event &);
        static void notepadload(Rml::Event &);
        static void notepadsave(Rml::Event &);
        static void scratchexecute(Rml::Event &);
        static void scratchcopy(Rml::Event &);
        static void scratchpaste(Rml::Event &);
        static void scratchclear(Rml::Event &);
        static void pasteexecute(Rml::Event &);
        static void pastecopy(Rml::Event &);
        static void pastefromclipboard(Rml::Event &);
        static void pasteclear(Rml::Event &);
        static RmlUtilities *instance;

        Rml::DataModelHandle model;
        Rml::String filename = "untitled.txt";
        Rml::String notepad_text, scratchpad_text, pastebuffer_text, status;
        bool section_notepad = true, section_scratchpad = false;
        bool section_pastebuffer = false;
    };
}

#endif
