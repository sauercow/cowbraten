#pragma once

// Independent, engine-agnostic visibility policy for the three HUD chips.
// "available" is computed by the real HUD scorer/timer, not guessed here.
#include <string>

namespace cowui
{
    struct MatchHudInput
    {
        bool score_enabled = false; // existing score toggle
        bool clock_enabled = false; // existing timer toggle
        bool score_available = false; // original scorer can provide two scores
        bool clock_available = false; // original timer has a valid value
        int left_score = 0;
        int right_score = 0;
        std::string clock_text;
    };

    struct MatchHudView
    {
        bool hud_visible = false;
        bool show_score = false;
        bool show_clock = false;
        int left_score = 0;
        int right_score = 0;
        std::string clock_text;
    };

    inline MatchHudView make_match_hud_view(const MatchHudInput &source)
    {
        MatchHudView result;
        result.show_score = source.score_enabled && source.score_available;
        result.show_clock = source.clock_enabled && source.clock_available;
        result.hud_visible = result.show_score || result.show_clock;
        if(result.show_score)
        {
            result.left_score = source.left_score;
            result.right_score = source.right_score;
        }
        if(result.show_clock) result.clock_text = source.clock_text;
        return result;
    }
}
