#include "engine.h"
#include "game.h"
#include "sauer_admin_bridge.h"

namespace game { const char *getclientp1xbratenversion(int client_num); }
extern char *cowbratenversion;

namespace cowui
{
    namespace saueradmin
    {
        bool readstate(State &result)
        {
            result.connected = game::connected && game::player1;
            result.local_master = result.connected && game::player1->privilege != PRIV_NONE;
            result.team_mode = result.connected && m_check(game::gamemode, M_TEAM);
            result.local_client_num = result.connected ? game::player1->clientnum : -1;
            result.master_mode = game::mastermode;
            result.server_name = game::servdesc[0] ? game::servdesc : "Local server";
            return true;
        }

        const char *clientversion()
        {
            return cowbratenversion ? cowbratenversion : "";
        }

        void visitclients(ClientVisitor visitor, void *data)
        {
            if(!visitor) return;
            loopv(game::players)
            {
                fpsent *client = game::players[i];
                if(!client || client->aitype != AI_NONE) continue;
                visitor(client->clientnum, client->name, client->team,
                    game::getclientp1xbratenversion(client->clientnum),
                    client->state == CS_SPECTATOR, client->privilege,
                    client == game::player1, data);
            }
        }

        bool setlocalmaster(bool enabled)
        {
            if(!game::connected || !game::player1) return false;
            game::addmsg(N_SETMASTER, "riis", game::player1->clientnum,
                enabled ? 1 : 0, "");
            return true;
        }

        bool setmastermode(int mode)
        {
            if(!game::connected || !game::player1 ||
               game::player1->privilege == PRIV_NONE ||
               mode < MM_OPEN || mode > MM_PRIVATE) return false;
            game::addmsg(N_MASTERMODE, "ri", mode);
            return true;
        }

        bool kick(int client_num)
        {
            if(!game::connected || !game::player1 ||
               client_num == game::player1->clientnum ||
               !game::getclient(client_num)) return false;
            game::addmsg(N_KICK, "ris", client_num, "");
            return true;
        }

        bool togglespectator(int client_num)
        {
            fpsent *client = game::getclient(client_num);
            if(!game::connected || !client) return false;
            game::addmsg(N_SPECTATOR, "rii", client_num,
                client->state == CS_SPECTATOR ? 0 : 1);
            return true;
        }

        bool switchteam(int client_num)
        {
            fpsent *client = game::getclient(client_num);
            if(!game::connected || !client || !m_check(game::gamemode, M_TEAM))
                return false;
            game::addmsg(N_SETTEAM, "ris", client_num,
                !strcmp(client->team, "good") ? "evil" : "good");
            return true;
        }

        bool togglemaster(int client_num)
        {
            fpsent *client = game::getclient(client_num);
            if(!game::connected || !client) return false;
            game::addmsg(N_SETMASTER, "riis", client_num,
                client->privilege == PRIV_NONE ? 1 : 0, "");
            return true;
        }
    }
}
