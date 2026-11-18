#include "../src/cowbraten/ui/match_score_logic.h"
#include <cassert>
#include <iostream>
using cowui::MatchHudInput;
using cowui::make_match_hud_view;
int main()
{
    for(int score = 0; score <= 1; ++score)
        for(int clock = 0; clock <= 1; ++clock)
        {
            MatchHudInput input;
            input.score_enabled = !!score;
            input.clock_enabled = !!clock;
            input.score_available = input.clock_available = true;
            input.left_score = 12;
            input.right_score = 8;
            input.clock_text = "4:26";
            auto view = make_match_hud_view(input);
            assert(view.show_score == !!score);
            assert(view.show_clock == !!clock);
            assert(view.hud_visible == (!!score || !!clock));
            assert(view.left_score == (score ? 12 : 0));
            assert(view.right_score == (score ? 8 : 0));
            assert(view.clock_text == (clock ? "4:26" : ""));
        }
    MatchHudInput no_opponent;
    no_opponent.score_enabled = true;
    no_opponent.score_available = false;
    no_opponent.clock_enabled = true;
    no_opponent.clock_available = true;
    no_opponent.clock_text = "1:04";
    const auto solo_clock = make_match_hud_view(no_opponent);
    assert(!solo_clock.show_score && solo_clock.show_clock);
    assert(solo_clock.hud_visible);
    std::cout << "PASS: 4 combinations, unavailable score fallback\n";
}
