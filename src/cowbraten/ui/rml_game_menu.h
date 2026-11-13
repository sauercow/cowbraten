#ifndef COWBRATEN_UI_RML_GAME_MENU_H
#define COWBRATEN_UI_RML_GAME_MENU_H

#include "game_menu_api.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

namespace Rml
{
    class Context;
    class Event;
}

namespace cowui
{
    class RmlActionRouter;

    class RmlGameMenu final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);
        void activate();
        void update();

    private:
        void sync(const gamemenu::State &state);
        void syncmusic();
        void syncadmin();
        void setmastermode(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);

        static void resumeaction(Rml::Event &event);
        static void goodteamaction(Rml::Event &event);
        static void evilteamaction(Rml::Event &event);
        static void toggleteamaction(Rml::Event &event);
        static void spectatoraction(Rml::Event &event);
        static void musicvolumeaction(Rml::Event &event);
        static void claimmasteraction(Rml::Event &event);
        static void relinquishmasteraction(Rml::Event &event);
        static void moreaction(Rml::Event &event);
        static void disconnectaction(Rml::Event &event);

        Rml::DataModelHandle model;
        Rml::String player_name;
        Rml::String server_name;
        Rml::String server_address;
        Rml::String mode_name;
        Rml::String map_name;
        Rml::String map_image;
        Rml::String team_name;
        Rml::String team_label;
        Rml::String time_remaining;
        Rml::String player_count_label;
        Rml::String ping_label;
        Rml::String state_label;
        bool spectator = false;
        bool music_enabled = false;
        bool team_mode = false;
        bool good_team = false;
        bool evil_team = false;
        bool actions_available = false;
        bool admin_connected = false;
        bool local_master = false;
        int master_mode = 0;

        static RmlGameMenu *instance;
    };
}

#endif
