#ifndef COWBRATEN_UI_SAUER_SCOREBOARD_BRIDGE_H
#define COWBRATEN_UI_SAUER_SCOREBOARD_BRIDGE_H

namespace cowui
{
    namespace sauerscoreboard
    {
        struct State
        {
            const char *server_name;
            const char *mode_name;
            const char *map_name;
            int seconds_remaining;
            bool intermission;
            bool paused;
            bool show_flags, show_frags, show_deaths, show_suicides;
            bool show_kpd, show_accuracy, show_damage, show_damage_received;
            bool show_ping, show_pj, show_client_num;
        };

        struct PlayerView
        {
            int group_index;
            bool group_first;
            const char *group_name;
            int group_score;
            int client_num;
            const char *name;
            const char *team;
            bool local;
            bool spectator;
            bool dead;
            int privilege;
            int flags, frags, deaths, suicides, ping, pj;
            float kpd, accuracy, damage, damage_received;
        };

        typedef void (*PlayerVisitor)(const PlayerView &player, void *data);

        bool readstate(State &result);
        void visitplayers(PlayerVisitor visitor, void *data);
    }
}

#endif
