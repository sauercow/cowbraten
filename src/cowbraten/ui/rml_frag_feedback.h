#pragma once

#include <RmlUi/Core.h>
#include "cube_text.h"
#include "frag_feedback_logic.h"
#include "frag_feedback_snapshot.h"
#include "frag_feedback_weapon_icons.h"

#include <array>
#include <string>

// Presentation only. The queue, filters, limits, streak accounting and
// milestone precedence all live in the original fpsgame/fragmessages code.
namespace cowui
{
    class RmlFragFeedback final
    {
    public:
        bool init(Rml::Context *context)
        {
            if(!context) return false;
            document = context->LoadDocument("data/cowbraten/ui/default/screens/frag_feedback.rml");
            if(!document) return false;
            root = document->GetElementById("cow-frag-root");
            milestone = document->GetElementById("cow-frag-milestone");
            milestone_text = document->GetElementById("cow-frag-milestone-text");
            bool valid = root && milestone && milestone_text;
            for(int i = 0; i < MaxFragFeedbackRows; ++i)
            {
                Slot &slot = slots[i];
                slot.container = document->GetElementById(Rml::CreateString("cow-frag-row-%d", i));
                slot.source = document->GetElementById(Rml::CreateString("cow-frag-source-%d", i));
                slot.combo = document->GetElementById(Rml::CreateString("cow-frag-combo-%d", i));
                slot.weapon_icon = document->GetElementById(Rml::CreateString("cow-frag-weapon-icon-%d", i));
                slot.target = document->GetElementById(Rml::CreateString("cow-frag-target-%d", i));
                valid = valid && slot.container && slot.source && slot.combo &&
                    slot.weapon_icon && slot.target;
            }
            if(!valid)
            {
                document->Close();
                document = nullptr;
                root = milestone = milestone_text = nullptr;
                slots = {};
                return false; // Enables unmodified legacy fallback.
            }
            document->SetProperty("font-family", "Lato");
            document->SetProperty("font-size", "18px");
            root->SetProperty("display", "none");
            root_visible = false;
            document->Show();
            return true;
        }

        void sync(const FragFeedbackSnapshot &state)
        {
            snapshot = state;
            have_snapshot = true;
        }

        void update(int now, bool allowed)
        {
            if(!document || !root) return;
            if(!allowed || !have_snapshot)
            {
                setrootvisible(false);
                return;
            }

            const auto mf = fragfeedback::frame(now,
                snapshot.milestone_timestamp, snapshot.duration_ms,
                snapshot.fade, snapshot.milestone > 0);
            bool any_visible = mf.visible;
            for(int i = 0; !any_visible && i < snapshot.count; ++i)
                any_visible = fragfeedback::frame(now,
                    snapshot.rows[i].timestamp, snapshot.duration_ms,
                    snapshot.fade, true).visible;
            if(!any_visible)
            {
                setrootvisible(false);
                return;
            }

            setrootvisible(true);
            // The root spans one viewport width and its children center with flex.
            // Shift its left edge so the middle matches hudfragmessagex.
            // This avoids percentage transforms, which failed to center
            // the current RmlUi cards on the configured anchor.
            root->SetProperty("left", Rml::CreateString("%.3f%%",
                (snapshot.x - 0.5f) * 100.0f));
            root->SetProperty("top", Rml::CreateString("%.3f%%", snapshot.y * 100.0f));
            // RmlUi font metrics scale with the original hudfragmessagescale
            // (0.5 by default). The actual card dimensions use em units.
            const float scale = snapshot.scale > 0.0f ? snapshot.scale : 0.5f;
            root->SetProperty("font-size", Rml::CreateString("%.2fpx", 18.0f * scale / 0.5f));

            if(mf.visible)
            {
                milestone->SetProperty("display", "flex");
                milestone->SetProperty("opacity", Rml::CreateString("%.3f", mf.opacity));
                milestone->SetProperty("font-size", Rml::CreateString("%.3fem",
                    snapshot.milestone_scale > 0.0f ? snapshot.milestone_scale : 1.0f));
                settext(milestone_text, Rml::CreateString("%d STREAK", snapshot.milestone));
            }
            else milestone->SetProperty("display", "none");

            // Legacy milestones obscure the queue for their duration.
            const int direction = fragfeedback::stackdirection(snapshot.stack_direction,
                                                                snapshot.y);
            for(int i = 0; i < MaxFragFeedbackRows; ++i)
            {
                Slot &slot = slots[i];
                const bool candidate = !mf.visible && i < snapshot.count;
                const fragfeedback::Frame frame = candidate
                    ? fragfeedback::frame(now, snapshot.rows[i].timestamp,
                                         snapshot.duration_ms, snapshot.fade, true)
                    : fragfeedback::Frame{false, 0.0f};
                if(!frame.visible)
                {
                    slot.container->SetProperty("display", "none");
                    continue;
                }
                const FragFeedbackRow &row = snapshot.rows[i];
                slot.container->SetProperty("display", "flex");
                slot.container->SetProperty("top", Rml::CreateString("%.3fem",
                    float(i * direction) * 3.1f));
                slot.container->SetProperty("opacity", Rml::CreateString("%.3f", frame.opacity));

                // Actual names already passed through the vanilla filter.
                Rml::String source = plainname(row.attacker);
                if(row.combo >= 2) source = "YOU";
                else if(source == "you") source = "YOU";
                Rml::String target = plainname(row.victim);
                if(target == "you") target = "YOU";

                settext(slot.source, source);
                settext(slot.combo, row.combo >= 2
                    ? Rml::CreateString("x%d", row.combo) : "");
                // Preserve the exact weapon ID from the legacy frag queue.
                // Updating img.src only when its ID changes avoids a texture
                // lookup and a DOM layout invalidation every frame.
                const bool suicide = source.empty();
                if(slot.cached_weapon != row.weapon || slot.cached_suicide != suicide)
                {
                    slot.weapon_icon->SetAttribute("src",
                        Rml::String("../images/hud/v1/") +
                        fragfeedbackweaponicon(row.weapon, suicide));
                    slot.cached_weapon = row.weapon;
                    slot.cached_suicide = suicide;
                }
                settext(slot.target, target);
                slot.source->SetProperty("display", source.empty() ? "none" : "inline");
                slot.combo->SetProperty("display", row.combo >= 2 ? "inline" : "none");
                applytone(slot.source, row.attacker_tone);
                applytone(slot.target, row.victim_tone);
            }
        }

        void shutdown(Rml::Context *)
        {
            if(document) document->Close();
            document = nullptr;
            root = milestone = milestone_text = nullptr;
            slots = {};
            have_snapshot = false;
            root_visible = false;
        }

    private:
        struct Slot
        {
            Rml::Element *container = nullptr, *source = nullptr;
            Rml::Element *combo = nullptr, *weapon_icon = nullptr, *target = nullptr;
            int cached_weapon = -1;
            bool cached_suicide = false;
        };

        static Rml::String plainname(const char *name)
        {
            Rml::String result;
            if(!name) return result;
            const std::string converted = plain_cube_text(name);
            result.reserve(converted.size());
            for(unsigned char c : converted)
                if(c >= 32 || c == '\t') result.push_back(char(c));
            return result;
        }

        static void settext(Rml::Element *element, const Rml::String &value)
        {
            if(!element) return;
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
            // Avoid regenerating text DOM nodes on every frame.
            if(element->GetInnerRML() != escaped) element->SetInnerRML(escaped);
        }

        static void applytone(Rml::Element *element, int tone)
        {
            if(!element) return;
            element->SetClass("cow-tone-self", tone == 1);
            element->SetClass("cow-tone-enemy", tone == 2);
            element->SetClass("cow-tone-ally", tone == 3);
            element->SetClass("cow-tone-neutral", tone == 0);
        }

        void setrootvisible(bool visible)
        {
            if(root_visible == visible) return;
            root_visible = visible;
            root->SetProperty("display", visible ? "block" : "none");
        }

        Rml::ElementDocument *document = nullptr;
        Rml::Element *root = nullptr, *milestone = nullptr, *milestone_text = nullptr;
        std::array<Slot, MaxFragFeedbackRows> slots{};
        FragFeedbackSnapshot snapshot;
        bool have_snapshot = false;
        bool root_visible = false;
    };
}
