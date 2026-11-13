#include "rml_utilities.h"

#include "rml_action.h"
#include "utility_api.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

namespace cowui
{
    RmlUtilities *RmlUtilities::instance = nullptr;

    bool RmlUtilities::init(Rml::Context *context, RmlActionRouter &router)
    {
        if(!context || instance) return false;
        Rml::DataModelConstructor constructor =
            context->CreateDataModel("utilities");
        if(!constructor) return false;
        const bool valid =
            constructor.Bind("filename", &filename) &&
            constructor.Bind("notepad_text", &notepad_text) &&
            constructor.Bind("scratchpad_text", &scratchpad_text) &&
            constructor.Bind("pastebuffer_text", &pastebuffer_text) &&
            constructor.Bind("status", &status) &&
            constructor.Bind("section_notepad", &section_notepad) &&
            constructor.Bind("section_scratchpad", &section_scratchpad) &&
            constructor.Bind("section_pastebuffer", &section_pastebuffer) &&
            constructor.BindEventCallback("set_filename", &RmlUtilities::setfilename, this) &&
            constructor.BindEventCallback("set_notepad", &RmlUtilities::setnotepad, this) &&
            constructor.BindEventCallback("set_scratchpad", &RmlUtilities::setscratchpad, this) &&
            constructor.BindEventCallback("set_pastebuffer", &RmlUtilities::setpastebuffer, this) &&
            router.registeraction("utilities.section.notepad", notepadsection) &&
            router.registeraction("utilities.section.scratchpad", scratchpadsection) &&
            router.registeraction("utilities.section.pastebuffer", pastebuffersection) &&
            router.registeraction("utilities.notepad.load", notepadload) &&
            router.registeraction("utilities.notepad.save", notepadsave) &&
            router.registeraction("utilities.scratch.execute", scratchexecute) &&
            router.registeraction("utilities.scratch.copy", scratchcopy) &&
            router.registeraction("utilities.scratch.paste", scratchpaste) &&
            router.registeraction("utilities.scratch.clear", scratchclear) &&
            router.registeraction("utilities.paste.execute", pasteexecute) &&
            router.registeraction("utilities.paste.copy", pastecopy) &&
            router.registeraction("utilities.paste.clipboard", pastefromclipboard) &&
            router.registeraction("utilities.paste.clear", pasteclear);
        if(!valid)
        {
            context->RemoveDataModel("utilities");
            return false;
        }
        model = constructor.GetModelHandle();
        instance = this;
        return true;
    }

    void RmlUtilities::shutdown(Rml::Context *context)
    {
        if(instance == this) instance = nullptr;
        if(context && model) context->RemoveDataModel("utilities");
        model = Rml::DataModelHandle();
    }

    void RmlUtilities::activate(int section)
    {
        if(section >= 0) setsection(section);
    }

    void RmlUtilities::setsection(int value)
    {
        section_notepad = value == 0;
        section_scratchpad = value == 1;
        section_pastebuffer = value == 2;
        model.DirtyVariable("section_notepad");
        model.DirtyVariable("section_scratchpad");
        model.DirtyVariable("section_pastebuffer");
        status.clear();
        model.DirtyVariable("status");
        if(section_notepad && notepad_text.empty()) loadnotepad();
    }

#define SET_TEXT_CALLBACK(method, member, variable) \
    void RmlUtilities::method(Rml::DataModelHandle, Rml::Event &event, \
        const Rml::VariantList &arguments) \
    { \
        if(arguments.size() == 1) member = arguments[0].Get<Rml::String>(); \
        model.DirtyVariable(variable); \
        event.StopPropagation(); \
    }
    SET_TEXT_CALLBACK(setfilename, filename, "filename")
    SET_TEXT_CALLBACK(setnotepad, notepad_text, "notepad_text")
    SET_TEXT_CALLBACK(setscratchpad, scratchpad_text, "scratchpad_text")
    SET_TEXT_CALLBACK(setpastebuffer, pastebuffer_text, "pastebuffer_text")
#undef SET_TEXT_CALLBACK

    void RmlUtilities::loadnotepad()
    {
        std::string text;
        const bool ok = utility::loadtext(filename.c_str(), text);
        if(ok)
        {
            notepad_text = text;
            model.DirtyVariable("notepad_text");
        }
        status = ok ? "File loaded." : "File not found or path is unavailable.";
        model.DirtyVariable("status");
    }

    void RmlUtilities::savenotepad()
    {
        const bool ok = utility::savetext(filename.c_str(), notepad_text.c_str());
        status = ok ? "File saved." : "Could not save this relative path.";
        model.DirtyVariable("status");
    }

    void RmlUtilities::execute(Rml::String &text, const char *label)
    {
        const bool ok = utility::executecommands(text.c_str());
        status = ok ? Rml::String(label) + " executed." : "Nothing to execute.";
        model.DirtyVariable("status");
    }

    void RmlUtilities::copy(Rml::String &text)
    {
        status = utility::writeclipboard(text.c_str())
            ? "Copied to clipboard." : "Clipboard is unavailable.";
        model.DirtyVariable("status");
    }

    void RmlUtilities::paste(Rml::String &text, const char *variable)
    {
        std::string clipboard;
        if(utility::readclipboard(clipboard))
        {
            if(!text.empty() && !clipboard.empty()) text += "\n";
            text += clipboard;
            model.DirtyVariable(variable);
            status = "Clipboard appended.";
        }
        else status = "Clipboard is empty or unavailable.";
        model.DirtyVariable("status");
    }

    void RmlUtilities::clear(Rml::String &text, const char *variable)
    {
        text.clear();
        model.DirtyVariable(variable);
        status = "Buffer cleared.";
        model.DirtyVariable("status");
    }

#define SIMPLE_SECTION_ACTION(name, value) \
    void RmlUtilities::name(Rml::Event &event) { if(instance) instance->setsection(value); event.StopPropagation(); }
    SIMPLE_SECTION_ACTION(notepadsection, 0)
    SIMPLE_SECTION_ACTION(scratchpadsection, 1)
    SIMPLE_SECTION_ACTION(pastebuffersection, 2)
#undef SIMPLE_SECTION_ACTION

    void RmlUtilities::notepadload(Rml::Event &e) { if(instance) instance->loadnotepad(); e.StopPropagation(); }
    void RmlUtilities::notepadsave(Rml::Event &e) { if(instance) instance->savenotepad(); e.StopPropagation(); }
    void RmlUtilities::scratchexecute(Rml::Event &e) { if(instance) instance->execute(instance->scratchpad_text, "Scratchpad"); e.StopPropagation(); }
    void RmlUtilities::scratchcopy(Rml::Event &e) { if(instance) instance->copy(instance->scratchpad_text); e.StopPropagation(); }
    void RmlUtilities::scratchpaste(Rml::Event &e) { if(instance) instance->paste(instance->scratchpad_text, "scratchpad_text"); e.StopPropagation(); }
    void RmlUtilities::scratchclear(Rml::Event &e) { if(instance) instance->clear(instance->scratchpad_text, "scratchpad_text"); e.StopPropagation(); }
    void RmlUtilities::pasteexecute(Rml::Event &e) { if(instance) instance->execute(instance->pastebuffer_text, "Paste buffer"); e.StopPropagation(); }
    void RmlUtilities::pastecopy(Rml::Event &e) { if(instance) instance->copy(instance->pastebuffer_text); e.StopPropagation(); }
    void RmlUtilities::pastefromclipboard(Rml::Event &e) { if(instance) instance->paste(instance->pastebuffer_text, "pastebuffer_text"); e.StopPropagation(); }
    void RmlUtilities::pasteclear(Rml::Event &e) { if(instance) instance->clear(instance->pastebuffer_text, "pastebuffer_text"); e.StopPropagation(); }
}
