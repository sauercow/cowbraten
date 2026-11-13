#ifndef COWBRATEN_UI_SCOREBOARD_API_H
#define COWBRATEN_UI_SCOREBOARD_API_H

#include <string>
#include <vector>

namespace cowui
{
    namespace scoreboard
    {
        struct State
        {
            std::string server_name, mode_name, map_name;
            int seconds_remaining = 0;
            bool intermission = false, paused = false;
            bool show_flags = false, show_frags = true, show_deaths = false;
            bool show_suicides = false, show_kpd = false, show_accuracy = false;
            bool show_damage = false, show_damage_received = false;
            bool show_ping = true, show_pj = false, show_client_num = false;
        };

        struct Player
        {
            int group_index = 0, group_score = 0, client_num = -1;
            bool group_first = false, local = false, spectator = false;
            bool dead = false;
            int privilege = 0;
            int flags = 0, frags = 0, deaths = 0, suicides = 0;
            int ping = 0, pj = 0;
            float kpd = 0, accuracy = 0, damage = 0, damage_received = 0;
            std::string group_name, name, team;
        };

        bool readstate(State &result);
        void readplayers(std::vector<Player> &result);
    }
}

#endif
