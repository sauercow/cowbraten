#ifndef COWBRATEN_UI_TEAM_PERSPECTIVE_H
#define COWBRATEN_UI_TEAM_PERSPECTIVE_H

#include <cstring>

// Display-only presentation mapping. Never changes the actual game team.
namespace cowui
{
    namespace teamperspective
    {
        enum Team { UNKNOWN, GOOD, EVIL };

        inline Team identify(const char *name)
        {
            if(name && !std::strcmp(name, "good")) return GOOD;
            if(name && !std::strcmp(name, "evil")) return EVIL;
            return UNKNOWN;
        }

        // The local/followed team's score is blue and placed on the left.
        // For neutral spectators, GOOD is on the left and EVIL on the right.
        inline bool good_on_left(Team local_team)
        {
            return local_team != EVIL;
        }

        // Non-team legacy drawhudscore: score slots stay in ranking order,
        // but blue highlights the observed player. Tied scores use blue-left.
        inline bool blue_on_left_in_ffa(bool watching_leader,
                                        int left_score, int right_score)
        {
            return watching_leader || left_score == right_score;
        }
    }
}

#endif
