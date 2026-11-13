#ifndef COWBRATEN_UI_SAUER_GAME_MENU_BRIDGE_H
#define COWBRATEN_UI_SAUER_GAME_MENU_BRIDGE_H

namespace cowui
{
    namespace sauergamemenu
    {
        struct State
        {
            bool connected;
            bool spectator;
            bool team_mode;
            bool demo_playback;
            const char *player_name;
            const char *server_name;
            const char *server_address;
            const char *mode_name;
            const char *map_name;
            const char *team_name;
            const char *time_remaining;
            int player_count;
            int ping;
        };

        bool readstate(State &result);
        bool setspectator(bool enabled);
        bool setteam(const char *name);
        void disconnect();
    }
}

#endif
