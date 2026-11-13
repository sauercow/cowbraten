#include "engine.h"
#include "game.h"
#include "sauer_game_menu_bridge.h"

extern void trydisconnect(bool local);

namespace cowui
{
    namespace sauergamemenu
    {
        bool readstate(State &result)
        {
            result.connected = game::connected && game::player1;
            if(!result.connected)
                return false;

            result.spectator = game::player1->state == CS_SPECTATOR;
            result.team_mode = m_check(game::gamemode, M_TEAM) != 0;
            result.demo_playback = game::demoplayback;
            result.player_name = game::player1->name;
            result.server_name = game::servdesc[0]
                ? game::servdesc
                : (game::remote ? "Connected server" : "Local match");

            static string server_address;
            const ENetAddress *address = connectedpeer();
            if(game::remote && address)
            {
                string host;
                if(enet_address_get_host_ip(address, host, sizeof(host)) >= 0)
                    formatstring(server_address, "%s:%d", host, address->port);
                else
                    copystring(server_address, "REMOTE SERVER");
            }
            else copystring(server_address, "LOCAL HOST");
            result.server_address = server_address;

            result.mode_name = server::modename(game::gamemode, "Unknown mode");
            result.map_name = game::getclientmap();
            result.team_name = game::player1->team;

            static string time_remaining;
            if(game::maplimit >= 0)
            {
                const int seconds = max(game::maplimit - lastmillis + 999, 0) / 1000;
                formatstring(time_remaining, "%d:%02d", seconds / 60, seconds % 60);
            }
            else copystring(time_remaining, "--:--");
            result.time_remaining = time_remaining;
            result.player_count = game::players.length();
            result.ping = game::remote ? max(0, game::player1->ping) : 0;
            return true;
        }

        bool setspectator(bool enabled)
        {
            if(!game::connected || !game::player1 || game::demoplayback ||
               game::player1->clientnum < 0)
                return false;

            game::togglespectator(enabled ? 1 : 0, "");
            return true;
        }

        bool setteam(const char *name)
        {
            if(!game::connected || !game::player1 || game::demoplayback ||
               !m_check(game::gamemode, M_TEAM) || !name ||
               (strcmp(name, "good") && strcmp(name, "evil")))
                return false;

            const bool was_spectator =
                game::player1->state == CS_SPECTATOR;
            if(strcmp(game::player1->team, name))
                game::switchteam(name);
            if(was_spectator)
                game::togglespectator(0, "");
            return true;
        }

        void disconnect()
        {
            ::trydisconnect(true);
        }
    }
}
