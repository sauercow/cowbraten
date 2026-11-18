#pragma once

#include <RmlUi/Core.h>
#include "match_score_logic.h"

// Standalone RmlUi overlay controller. It does NOT replace or override the
// legacy score rendering on its own; integration must route the original
// scoreboard/timer data and disable the legacy drawing paths once initialized.
namespace cowui
{
    class RmlMatchScoreHud
    {
    public:
        bool init(Rml::Context *ctx)
        {
            if(!ctx) return false;
            auto constructor = ctx->CreateDataModel("cow_match_hud");
            if(!constructor ||
               !constructor.Bind("hud_visible", &hud_visible) ||
               !constructor.Bind("show_score", &show_score) ||
               !constructor.Bind("show_clock", &show_clock) ||
               !constructor.Bind("left_score", &left_score) ||
               !constructor.Bind("right_score", &right_score) ||
               !constructor.Bind("clock_text", &clock_text))
            {
                ctx->RemoveDataModel("cow_match_hud");
                return false;
            }
            model = constructor.GetModelHandle();
            document = ctx->LoadDocument(
                "data/cowbraten/ui/default/screens/match_score_hud.rml");
            if(!document)
            {
                ctx->RemoveDataModel("cow_match_hud");
                return false;
            }
            document->Show();
            return true;
        }

        void update(const MatchHudInput &input)
        {
            const MatchHudView next = make_match_hud_view(input);
#define MATCH_UPDATE(var, val) do { const auto &v = (val); \
            if(var != v) { var = v; model.DirtyVariable(#var); } } while(0)
            MATCH_UPDATE(hud_visible, next.hud_visible);
            MATCH_UPDATE(show_score, next.show_score);
            MATCH_UPDATE(show_clock, next.show_clock);
            MATCH_UPDATE(left_score, next.left_score);
            MATCH_UPDATE(right_score, next.right_score);
            MATCH_UPDATE(clock_text, Rml::String(next.clock_text.c_str()));
#undef MATCH_UPDATE
        }

        void shutdown(Rml::Context *ctx)
        {
            if(document)
            {
                document->Close();
                document = nullptr;
            }
            if(ctx) ctx->RemoveDataModel("cow_match_hud");
            model = Rml::DataModelHandle();
        }

    private:
        Rml::ElementDocument *document = nullptr;
        Rml::DataModelHandle model;
        bool hud_visible = false, show_score = false, show_clock = false;
        int left_score = 0, right_score = 0;
        Rml::String clock_text;
    };
}
