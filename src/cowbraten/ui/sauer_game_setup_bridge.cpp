#include "engine.h"
#include "game.h"
#include "sauer_game_setup_bridge.h"

#include <cstring>

namespace cowui
{
    namespace sauergamesetup
    {
        namespace
        {
            struct CampaignEntry
            {
                const char *name;
                const char *map;
                const char *series;
            };

            static const CampaignEntry campaign_entries[] =
            {
                { "Run N' Gun — Part I", "mpsp9a", "Private Stan Sauer" },
                { "Run N' Gun — Part II", "mpsp9b", "Private Stan Sauer" },
                { "Run N' Gun — Part III", "mpsp9c", "Private Stan Sauer" },
                { "The Seriously Big Valley", "mpsp10", "Private Stan Sauer" },
                { "An Army Of One — Part I", "mpsp6a", "An Army Of One" },
                { "An Army Of One — Part II", "mpsp6b", "An Army Of One" },
                { "An Army Of One — Part III", "mpsp6c", "An Army Of One" },
                { "Lost", "lost", "Standalone" },
                { "Meltdown", "skrsp1", "Standalone" },
                { "Missile Pass", "crnsp1", "Standalone" },
                { "Level 9", "level9", "Standalone" }
            };

            bool validmapname(const char *name)
            {
                if(!name || !name[0] || std::strstr(name, "..") || name[0] == '/')
                    return false;
                for(const uchar *c = (const uchar *)name; *c; ++c)
                    if(!isalnum(*c) && *c != '_' && *c != '-' && *c != '/')
                        return false;
                return true;
            }
        }

        bool readstate(State &result)
        {
            result.connected = game::connected && game::player1;
            result.remote = result.connected && game::remote;
            result.can_change_map = !result.remote ||
                (game::player1 &&
                 (game::player1->state != CS_SPECTATOR || game::player1->privilege));
            result.current_mode = m_valid(game::gamemode) ? game::gamemode : 0;
            result.current_map = game::getclientmap();
            return true;
        }

        void visitmodes(ModeVisitor visitor, void *data)
        {
            if(!visitor) return;
            for(int id = 0; id <= 22; ++id)
            {
                const char *description = m_valid(id) && gamemodes[id - STARTGAMEMODE].info
                    ? gamemodes[id - STARTGAMEMODE].info
                    : "";
                visitor(id, server::modename(id, "Unknown"), description, data);
            }
        }

        void visitmaps(MapVisitor visitor, void *data)
        {
            if(!visitor) return;
            vector<char *> files;
            listfiles("packages/base", "ogz", files);
            files.sort();
            const char *previous = nullptr;
            loopv(files)
            {
                const char *name = files[i];
                if(!validmapname(name) || (previous && !std::strcmp(previous, name)))
                    continue;
                defformatstring(image, "packages/base/%s.jpg", name);
                string resource = "";
                if(fileexists(image, "rb"))
                    formatstring(resource, "?cowbraten-resource:%s", image);
                visitor(name, resource, data);
                previous = name;
            }
            files.deletearrays();
        }

        void visitcampaigns(CampaignVisitor visitor, void *data)
        {
            if(!visitor) return;
            for(int i = 0; i < int(sizeof(campaign_entries) / sizeof(campaign_entries[0])); ++i)
            {
                visitor(i, campaign_entries[i].name, campaign_entries[i].map,
                    campaign_entries[i].series, data);
            }
        }

        bool launch(const char *map, int mode, int bot_count,
            int bot_min_skill, int bot_max_skill, char *error, int error_size)
        {
            if(!validmapname(map) || mode < 0 || mode > 22)
            {
                copystring(error, "Select a valid mode and map.", error_size);
                return false;
            }
            State state;
            readstate(state);
            if(!state.can_change_map)
            {
                copystring(error, "Spectators need master privileges to vote for a map.", error_size);
                return false;
            }
            if(bot_count > 0 && state.remote)
            {
                copystring(error, "Bot matches can only be started on the local server.", error_size);
                return false;
            }

            game::changemap(map, mode);
            if(bot_count > 0)
            {
                const int low = clamp(bot_min_skill, 1, 101);
                const int high = clamp(bot_max_skill, low, 101);
                loopi(clamp(bot_count, 0, 32))
                    game::addmsg(N_ADDBOT, "ri", low == high ? low : rnd(high - low + 1) + low);
            }
            if(error_size > 0) error[0] = '\0';
            return true;
        }

        bool launchcampaign(int id, char *error, int error_size)
        {
            if(id < 0 || id >= int(sizeof(campaign_entries) / sizeof(campaign_entries[0])))
            {
                copystring(error, "Campaign mission is unavailable.", error_size);
                return false;
            }
            if(game::remote)
            {
                copystring(error, "Disconnect from the multiplayer server before starting a campaign.", error_size);
                return false;
            }
            game::changemap(campaign_entries[id].map, -3);
            if(error_size > 0) error[0] = '\0';
            return true;
        }
    }
}
