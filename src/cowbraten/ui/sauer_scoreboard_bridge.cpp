#include "engine.h"
#include "game.h"
#include "weaponstats.h"
#include "sauer_scoreboard_bridge.h"

namespace game
{
    extern int showclientnum, showpj, showping, showspectators;
    extern int hidefrags, showdeaths;
    namespace mod
    {
        extern int showsuicides, showflags, showkpd, showaccuracy;
        extern int showdamage, showdamagereceived;
    }
}

namespace cowui
{
    namespace sauerscoreboard
    {
        bool readstate(State &result)
        {
            result.server_name = game::servdesc[0] ? game::servdesc : "Local match";
            result.mode_name = server::modename(game::gamemode, "Unknown mode");
            result.map_name = game::getclientmap()[0] ? game::getclientmap() : "New map";
            result.intermission = game::intermission;
            result.paused = game::gamepaused;
            result.seconds_remaining = m_checknot(game::gamemode, M_DEMO|M_EDIT|M_LOCAL) && game::maplimit >= 0
                ? max(game::maplimit - lastmillis + 999, 0) / 1000 : -1;
            result.show_flags = m_check(game::gamemode, M_CTF) && game::mod::showflags;
            result.show_frags = !game::cmode || !game::cmode->hidefrags() ||
                !game::hidefrags;
            result.show_deaths = game::showdeaths;
            result.show_suicides = game::mod::showsuicides;
            result.show_kpd = game::mod::showkpd;
            result.show_accuracy = game::mod::showaccuracy;
            result.show_damage = !m_check(game::gamemode, M_INSTA) && game::mod::showdamage;
            result.show_damage_received = !m_check(game::gamemode, M_INSTA) && game::mod::showdamagereceived;
            result.show_ping = (multiplayer(false) || game::demoplayback) && game::showping;
            result.show_pj = (multiplayer(false) || game::demoplayback) && game::showpj;
            result.show_client_num = game::showclientnum ||
                (game::player1 && game::player1->privilege >= PRIV_MASTER);
            return true;
        }

        static void visitplayer(fpsent *player, int group_index,
            bool group_first, const char *group_name, int group_score,
            bool spectator, PlayerVisitor visitor, void *data)
        {
            fpsent *network_player = player->ownernum >= 0
                ? game::getclient(player->ownernum) : player;
            if(!network_player) network_player = player;
            PlayerView row = {};
            row.group_index = group_index;
            row.group_first = group_first;
            row.group_name = group_name ? group_name : "Players";
            row.group_score = group_score;
            row.client_num = player->clientnum;
            row.name = player->name;
            row.team = player->team;
            row.local = player == game::hudplayer();
            row.spectator = spectator;
            row.dead = player->state == CS_DEAD;
            row.privilege = player->privilege;
            row.flags = player->flags;
            row.frags = player->frags;
            row.deaths = player->deaths;
            row.suicides = player->suicides;
            row.ping = network_player->state == CS_LAGGED ? -1 : network_player->ping;
            row.pj = network_player == game::player1 ? 0 :
                (network_player->state == CS_LAGGED ? -1 : abs(33 - player->plag));
            row.kpd = float(player->frags) / max(1, player->deaths);
            row.accuracy = game::playeraccuracy(player);
            row.damage = game::mod::showdamage == 1
                ? game::playerdamage(player, DMG_DEALT)
                : game::playernetdamage(player);
            row.damage_received = game::playerdamage(player, DMG_RECEIVED);
            visitor(row, data);
        }

        void visitplayers(PlayerVisitor visitor, void *data)
        {
            if(!visitor) return;
            const int numgroups = game::groupplayers();
            loopi(numgroups)
            {
                game::scoregroup &group = *game::groups[i];
                const char *name = group.team ? group.team : "Players";
                loopvj(group.players)
                    visitplayer(group.players[j], i, j == 0, name,
                        group.score, false, visitor, data);
            }
            if(game::showspectators && game::spectators.length())
            {
                loopv(game::spectators)
                    visitplayer(game::spectators[i], numgroups, i == 0,
                        "Spectators", 0, true, visitor, data);
            }
        }
    }
}
