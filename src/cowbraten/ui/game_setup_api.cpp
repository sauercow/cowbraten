#include "game_setup_api.h"
#include "sauer_game_setup_bridge.h"

namespace cowui
{
    namespace gamesetup
    {
        bool readstate(State &result)
        {
            sauergamesetup::State state = {};
            if(!sauergamesetup::readstate(state)) return false;
            result.connected = state.connected;
            result.remote = state.remote;
            result.can_change_map = state.can_change_map;
            result.current_mode = state.current_mode;
            result.current_map = state.current_map ? state.current_map : "";
            return true;
        }

        void readmodes(std::vector<Mode> &result)
        {
            result.clear();
            sauergamesetup::visitmodes(
                [](int id, const char *name, const char *description, void *data)
                {
                    static_cast<std::vector<Mode> *>(data)->push_back(
                        { id, name ? name : "", description ? description : "" });
                }, &result);
        }

        void readmaps(std::vector<Map> &result)
        {
            result.clear();
            sauergamesetup::visitmaps(
                [](const char *name, const char *image, void *data)
                {
                    static_cast<std::vector<Map> *>(data)->push_back(
                        { name ? name : "", image ? image : "" });
                }, &result);
        }

        void readcampaigns(std::vector<Campaign> &result)
        {
            result.clear();
            sauergamesetup::visitcampaigns(
                [](int id, const char *name, const char *map,
                    const char *series, void *data)
                {
                    static_cast<std::vector<Campaign> *>(data)->push_back(
                        { id, name ? name : "", map ? map : "",
                            series ? series : "" });
                }, &result);
        }

        bool launch(const char *map, int mode, int bot_count,
            int bot_min_skill, int bot_max_skill, std::string &error)
        {
            char message[256] = {};
            const bool result = sauergamesetup::launch(map, mode, bot_count,
                bot_min_skill, bot_max_skill, message, sizeof(message));
            error = message;
            return result;
        }

        bool launchcampaign(int id, std::string &error)
        {
            char message[256] = {};
            const bool result = sauergamesetup::launchcampaign(
                id, message, sizeof(message));
            error = message;
            return result;
        }
    }
}
