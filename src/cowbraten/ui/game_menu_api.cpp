#include "game_menu_api.h"

#include "sauer_game_menu_bridge.h"

namespace cowui
{
    namespace gamemenu
    {
        bool readstate(State &result)
        {
            sauergamemenu::State source;
            if(!sauergamemenu::readstate(source))
                return false;

            result.connected = source.connected;
            result.spectator = source.spectator;
            result.team_mode = source.team_mode;
            result.demo_playback = source.demo_playback;
            result.player_name = source.player_name;
            result.server_name = source.server_name;
            result.server_address = source.server_address;
            result.mode_name = source.mode_name;
            result.map_name = source.map_name;
            result.team_name = source.team_name;
            result.time_remaining = source.time_remaining;
            result.player_count = source.player_count;
            result.ping = source.ping;
            return true;
        }

        bool setspectator(bool enabled)
        {
            return sauergamemenu::setspectator(enabled);
        }

        bool setteam(const char *name)
        {
            return sauergamemenu::setteam(name);
        }

        void disconnect()
        {
            sauergamemenu::disconnect();
        }
    }
}
