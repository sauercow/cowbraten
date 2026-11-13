#include "scoreboard_api.h"
#include "sauer_scoreboard_bridge.h"

namespace cowui
{
    namespace scoreboard
    {
        bool readstate(State &result)
        {
            sauerscoreboard::State state = {};
            if(!sauerscoreboard::readstate(state)) return false;
            result.server_name = state.server_name ? state.server_name : "";
            result.mode_name = state.mode_name ? state.mode_name : "";
            result.map_name = state.map_name ? state.map_name : "";
            result.seconds_remaining = state.seconds_remaining;
            result.intermission = state.intermission;
            result.paused = state.paused;
            result.show_flags = state.show_flags;
            result.show_frags = state.show_frags;
            result.show_deaths = state.show_deaths;
            result.show_suicides = state.show_suicides;
            result.show_kpd = state.show_kpd;
            result.show_accuracy = state.show_accuracy;
            result.show_damage = state.show_damage;
            result.show_damage_received = state.show_damage_received;
            result.show_ping = state.show_ping;
            result.show_pj = state.show_pj;
            result.show_client_num = state.show_client_num;
            return true;
        }

        void readplayers(std::vector<Player> &result)
        {
            result.clear();
            sauerscoreboard::visitplayers(
                [](const sauerscoreboard::PlayerView &source, void *data)
                {
                    Player row;
                    row.group_index = source.group_index;
                    row.group_first = source.group_first;
                    row.group_name = source.group_name ? source.group_name : "";
                    row.group_score = source.group_score;
                    row.client_num = source.client_num;
                    row.name = source.name ? source.name : "";
                    row.team = source.team ? source.team : "";
                    row.local = source.local;
                    row.spectator = source.spectator;
                    row.dead = source.dead;
                    row.privilege = source.privilege;
                    row.flags = source.flags;
                    row.frags = source.frags;
                    row.deaths = source.deaths;
                    row.suicides = source.suicides;
                    row.ping = source.ping;
                    row.pj = source.pj;
                    row.kpd = source.kpd;
                    row.accuracy = source.accuracy;
                    row.damage = source.damage;
                    row.damage_received = source.damage_received;
                    static_cast<std::vector<Player> *>(data)->push_back(row);
                }, &result);
        }
    }
}
