#ifndef COWBRATEN_UI_GAME_SETUP_API_H
#define COWBRATEN_UI_GAME_SETUP_API_H

#include <string>
#include <vector>

namespace cowui
{
    namespace gamesetup
    {
        struct Mode { int id = 0; std::string name, description; };
        struct Map { std::string name, image; };
        struct Campaign { int id = 0; std::string name, map, series; };
        struct State
        {
            bool connected = false, remote = false, can_change_map = true;
            int current_mode = 0;
            std::string current_map;
        };

        bool readstate(State &result);
        void readmodes(std::vector<Mode> &result);
        void readmaps(std::vector<Map> &result);
        void readcampaigns(std::vector<Campaign> &result);
        bool launch(const char *map, int mode, int bot_count,
            int bot_min_skill, int bot_max_skill, std::string &error);
        bool launchcampaign(int id, std::string &error);
    }
}

#endif
