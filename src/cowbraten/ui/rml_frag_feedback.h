#pragma once

#include <RmlUi/Core.h>
#include "frag_feedback_logic.h"

// Visual-only overlay. All streak scoring remains in fpsgame/fps.cpp.
// Initialized lazily from the normal RmlUi update path, with legacy fallback.
namespace cowui
{
    class RmlFragFeedback final
    {
    public:
        bool init(Rml::Context *context)
        {
            if(!context) return false;
            auto constructor = context->CreateDataModel("cow_frag_feedback");
            if(!constructor ||
               !constructor.Bind("show_frag", &show_frag) ||
               !constructor.Bind("show_combo", &show_combo) ||
               !constructor.Bind("show_milestone", &show_milestone) ||
               !constructor.Bind("actor", &actor) ||
               !constructor.Bind("victim", &victim) ||
               !constructor.Bind("weapon", &weapon) ||
               !constructor.Bind("combo_text", &combo_text) ||
               !constructor.Bind("milestone_text", &milestone_text))
            {
                context->RemoveDataModel("cow_frag_feedback");
                return false;
            }
            model = constructor.GetModelHandle();
            document = context->LoadDocument("data/cowbraten/ui/default/screens/frag_feedback.rml");
            if(!document)
            {
                context->RemoveDataModel("cow_frag_feedback");
                model = Rml::DataModelHandle();
                return false;
            }
            document->SetProperty("font-family", "Lato");
            document->SetProperty("font-size", "18px");
            document->Show();
            return true;
        }

        void submitfrag(const char *source, const char *target, const char *gun,
                        int combo, int now, int duration, bool fade)
        {
            if(!document) return;
            set(show_milestone, false, "show_milestone");
            set(show_frag, true, "show_frag");
            set(show_combo, combo >= 2, "show_combo");
            set(actor, plainname(source), "actor");
            settext("cow-frag-source", actor);
            set(victim, plainname(target), "victim");
            settext("cow-frag-target", victim);
            set(weapon, Rml::String(gun ? gun : "FRAG"), "weapon");
            settext("cow-frag-weapon-text", weapon);
            set(combo_text, Rml::CreateString("x%d", combo >= 2 ? combo : 2), "combo_text");
            settext("cow-frag-combo", combo_text);
            start = now;
            duration_ms = duration;
            fade_enabled = fade;
        }

        void submitmilestone(int value, int now, int duration, bool fade)
        {
            if(!document) return;
            set(show_frag, false, "show_frag");
            set(show_combo, false, "show_combo");
            set(show_milestone, true, "show_milestone");
            set(milestone_text, Rml::CreateString("%d STREAK", value), "milestone_text");
            settext("cow-frag-milestone-text", milestone_text);
            start = now;
            duration_ms = duration;
            fade_enabled = fade;
        }

        void update(int now, bool allowed)
        {
            if(!document) return;
            const auto f = fragfeedback::frame(now, start, duration_ms, fade_enabled,
                                               allowed && (show_frag || show_milestone));
            if(!f.visible)
            {
                set(show_frag, false, "show_frag");
                set(show_milestone, false, "show_milestone");
            }
            if(Rml::Element *root = document->GetElementById("cow-frag-root"))
                root->SetProperty("opacity", Rml::CreateString("%.3f", f.opacity));
        }

        void shutdown(Rml::Context *context)
        {
            if(document)
            {
                document->Close();
                document = nullptr;
            }
            if(context) context->RemoveDataModel("cow_frag_feedback");
            model = Rml::DataModelHandle();
            start = -1;
            show_frag = show_combo = show_milestone = false;
        }

    private:
        template<class T>
        void set(T &field, const T &next, const char *name)
        {
            if(field != next) { field = next; model.DirtyVariable(name); }
        }

        // Strip Cube color escapes while keeping names as bound RmlUi text,
        // never as untrusted RML/HTML markup.
        static Rml::String plainname(const char *name)
        {
            Rml::String result;
            if(!name) return result;
            for(const unsigned char *p = (const unsigned char *)name; *p; ++p)
            {
                if(*p == '\f' && p[1]) { ++p; continue; }
                if(*p < 32 && *p != '\t') continue;
                result.push_back(char(*p));
            }
            return result;
        }

        // Write escaped text directly instead of relying on data-model interpolation.
        void settext(const char *id, const Rml::String &value)
        {
            if(!document) return;
            Rml::String escaped;
            for(char c : value)
            {
                switch(c)
                {
                    case '&': escaped += "&amp;"; break;
                    case '<': escaped += "&lt;"; break;
                    case '>': escaped += "&gt;"; break;
                    case '"': escaped += "&quot;"; break;
                    case 39: escaped += "&#39;"; break;
                    default: escaped.push_back(c); break;
                }
            }
            if(Rml::Element *element = document->GetElementById(id))
                element->SetInnerRML(escaped);
        }

        Rml::ElementDocument *document = nullptr;
        Rml::DataModelHandle model;
        bool show_frag = false, show_combo = false, show_milestone = false;
        Rml::String actor, victim, weapon, combo_text, milestone_text;
        int start = -1, duration_ms = 1200;
        bool fade_enabled = true;
    };
}
