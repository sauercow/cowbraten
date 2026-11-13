#ifndef COWBRATEN_UI_SAUER_GAME_SETUP_BRIDGE_H
#define COWBRATEN_UI_SAUER_GAME_SETUP_BRIDGE_H

namespace cowui
{
    namespace sauergamesetup
    {
        struct State
        {
            bool connected;
            bool remote;
            bool can_change_map;
            int current_mode;
            const char *current_map;
        };

        typedef void (*ModeVisitor)(int id, const char *name,
            const char *description, void *data);
        typedef void (*MapVisitor)(const char *name, const char *image,
            void *data);
        typedef void (*CampaignVisitor)(int id, const char *name,
            const char *map, const char *series, void *data);

        bool readstate(State &result);
        void visitmodes(ModeVisitor visitor, void *data);
        void visitmaps(MapVisitor visitor, void *data);
        void visitcampaigns(CampaignVisitor visitor, void *data);
        bool launch(const char *map, int mode, int bot_count,
            int bot_min_skill, int bot_max_skill, char *error,
            int error_size);
        bool launchcampaign(int id, char *error, int error_size);
    }
}

#endif
