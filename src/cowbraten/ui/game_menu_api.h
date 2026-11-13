#ifndef COWBRATEN_UI_GAME_MENU_API_H
#define COWBRATEN_UI_GAME_MENU_API_H

#include <string>

namespace cowui
{
    namespace gamemenu
    {
        struct State
        {
            bool connected = false;
            bool spectator = false;
            bool team_mode = false;
            bool demo_playback = false;
            std::string player_name;
            std::string server_name;
            std::string server_address;
            std::string mode_name;
            std::string map_name;
            std::string team_name;
            std::string time_remaining;
            int player_count = 0;
            int ping = 0;
        };

        bool readstate(State &result);
        bool setspectator(bool enabled);
        bool setteam(const char *name);
        void disconnect();
    }
}

#endif
