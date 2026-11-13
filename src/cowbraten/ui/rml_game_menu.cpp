#include "rml_game_menu.h"

#include "admin_api.h"
#include "rml_action.h"
#include "settings_api.h"
#include "ui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

#include <cctype>

namespace cowui
{
    RmlGameMenu *RmlGameMenu::instance = nullptr;

    bool RmlGameMenu::init(
        Rml::Context *context,
        RmlActionRouter &router)
    {
        if(!context || instance)
            return false;

        Rml::DataModelConstructor constructor =
            context->CreateDataModel("game_menu");
        if(!constructor)
            return false;

        const bool valid =
            constructor.Bind("player_name", &player_name) &&
            constructor.Bind("server_name", &server_name) &&
            constructor.Bind("server_address", &server_address) &&
            constructor.Bind("mode_name", &mode_name) &&
            constructor.Bind("map_name", &map_name) &&
            constructor.Bind("map_image", &map_image) &&
            constructor.Bind("team_name", &team_name) &&
            constructor.Bind("team_label", &team_label) &&
            constructor.Bind("time_remaining", &time_remaining) &&
            constructor.Bind("player_count_label", &player_count_label) &&
            constructor.Bind("ping_label", &ping_label) &&
            constructor.Bind("state_label", &state_label) &&
            constructor.Bind("spectator", &spectator) &&
            constructor.Bind("music_enabled", &music_enabled) &&
            constructor.Bind("team_mode", &team_mode) &&
            constructor.Bind("good_team", &good_team) &&
            constructor.Bind("evil_team", &evil_team) &&
            constructor.Bind("actions_available", &actions_available) &&
            constructor.Bind("admin_connected", &admin_connected) &&
            constructor.Bind("local_master", &local_master) &&
            constructor.Bind("master_mode", &master_mode) &&
            constructor.BindEventCallback("select_master_mode",
                &RmlGameMenu::setmastermode, this) &&
            router.registeraction("game.resume", resumeaction) &&
            router.registeraction("game.team.good", goodteamaction) &&
            router.registeraction("game.team.evil", evilteamaction) &&
            router.registeraction("game.team.toggle", toggleteamaction) &&
            router.registeraction("game.spectator.toggle", spectatoraction) &&
            router.registeraction("game.music.toggle", musicvolumeaction) &&
            router.registeraction("game.master.claim", claimmasteraction) &&
            router.registeraction("game.master.relinquish", relinquishmasteraction) &&
            router.registeraction("game.more", moreaction) &&
            router.registeraction("game.disconnect", disconnectaction);

        if(!valid)
        {
            context->RemoveDataModel("game_menu");
            return false;
        }

        model = constructor.GetModelHandle();
        instance = this;
        return true;
    }

    void RmlGameMenu::shutdown(Rml::Context *context)
    {
        if(instance == this)
            instance = nullptr;
        if(context && model)
            context->RemoveDataModel("game_menu");
        model = Rml::DataModelHandle();
    }

    void RmlGameMenu::sync(const gamemenu::State &state)
    {
        Rml::String next_map_image;
        if(!state.map_name.empty() &&
           state.map_name.find("..") == std::string::npos &&
           state.map_name.front() != '/')
        {
            bool valid = true;
            for(const unsigned char character : state.map_name)
            {
                if(!std::isalnum(character) && character != '_' &&
                   character != '-' && character != '/')
                {
                    valid = false;
                    break;
                }
            }
            if(valid)
                next_map_image = Rml::String(
                    "?cowbraten-resource:packages/base/") +
                    state.map_name.c_str() + ".jpg";
        }

        const bool next_good_team =
            state.team_mode && state.team_name == "good";
        const bool next_evil_team =
            state.team_mode && state.team_name == "evil";
        const Rml::String next_team_label = state.spectator
            ? "SPECTATOR"
            : next_good_team
                ? "BLUE"
                : next_evil_team
                    ? "RED"
                    : state.team_mode
                        ? state.team_name.c_str()
                        : "FFA";

#define SYNC_VALUE(member, value) \
        do { if(member != (value)) { member = (value); model.DirtyVariable(#member); } } while(0)

        SYNC_VALUE(player_name, state.player_name);
        SYNC_VALUE(server_name, state.server_name);
        SYNC_VALUE(server_address, state.server_address);
        SYNC_VALUE(mode_name, state.mode_name);
        SYNC_VALUE(map_name, state.map_name.empty() ? "Untitled map" : state.map_name);
        SYNC_VALUE(map_image, next_map_image);
        SYNC_VALUE(team_name, state.team_name);
        SYNC_VALUE(team_label, next_team_label);
        SYNC_VALUE(time_remaining, state.time_remaining);
        SYNC_VALUE(player_count_label,
            Rml::String(std::to_string(state.player_count)) +
            (state.player_count == 1 ? " PLAYER" : " PLAYERS"));
        SYNC_VALUE(ping_label, state.ping > 0
            ? Rml::String(std::to_string(state.ping)) + " ms"
            : Rml::String("LOCAL"));
        SYNC_VALUE(spectator, state.spectator);
        SYNC_VALUE(team_mode, state.team_mode);
        SYNC_VALUE(good_team, next_good_team);
        SYNC_VALUE(evil_team, next_evil_team);
        SYNC_VALUE(actions_available, !state.demo_playback);
        SYNC_VALUE(state_label, state.spectator ? "Spectator" : "Playing");

#undef SYNC_VALUE
    }

    void RmlGameMenu::activate()
    {
        update();
    }

    void RmlGameMenu::syncmusic()
    {
        settings::Audio audio = {};
        if(!settings::readaudio(audio))
            return;

        const bool next_music_enabled = audio.music_volume > 0;
        if(music_enabled != next_music_enabled)
        {
            music_enabled = next_music_enabled;
            model.DirtyVariable("music_enabled");
        }
    }

    void RmlGameMenu::syncadmin()
    {
        admin::State state;
        if(!admin::readstate(state))
            return;

#define SYNC_ADMIN_VALUE(member, value) \
        do { if(member != (value)) { member = (value); model.DirtyVariable(#member); } } while(0)

        SYNC_ADMIN_VALUE(admin_connected, state.connected);
        SYNC_ADMIN_VALUE(local_master, state.local_master);
        SYNC_ADMIN_VALUE(master_mode, state.master_mode);

#undef SYNC_ADMIN_VALUE
    }

    void RmlGameMenu::update()
    {
        gamemenu::State state;
        if(gamemenu::readstate(state))
            sync(state);
        syncmusic();
        syncadmin();
    }

    void RmlGameMenu::resumeaction(Rml::Event &event)
    {
        event.StopPropagation();
        close();
    }

    void RmlGameMenu::goodteamaction(Rml::Event &event)
    {
        event.StopPropagation();
        if(instance && gamemenu::setteam("good"))
            close();
    }

    void RmlGameMenu::evilteamaction(Rml::Event &event)
    {
        event.StopPropagation();
        if(instance && gamemenu::setteam("evil"))
            close();
    }

    void RmlGameMenu::toggleteamaction(Rml::Event &event)
    {
        event.StopPropagation();
        if(instance && instance->team_mode &&
           gamemenu::setteam(instance->good_team ? "evil" : "good"))
            close();
    }

    void RmlGameMenu::spectatoraction(Rml::Event &event)
    {
        event.StopPropagation();
        if(instance && gamemenu::setspectator(!instance->spectator))
            close();
    }

    void RmlGameMenu::musicvolumeaction(Rml::Event &event)
    {
        event.StopPropagation();
        if(!instance)
            return;

        settings::writemusicvolume(instance->music_enabled ? 0 : 125);
        instance->syncmusic();
    }

    void RmlGameMenu::setmastermode(Rml::DataModelHandle,
        Rml::Event &event, const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
            admin::setmastermode(arguments[0].Get<int>(0));
        syncadmin();
        event.StopPropagation();
    }

    void RmlGameMenu::claimmasteraction(Rml::Event &event)
    {
        if(instance)
        {
            admin::setlocalmaster(true);
            instance->syncadmin();
        }
        event.StopPropagation();
    }

    void RmlGameMenu::relinquishmasteraction(Rml::Event &event)
    {
        if(instance)
        {
            admin::setlocalmaster(false);
            instance->syncadmin();
        }
        event.StopPropagation();
    }

    void RmlGameMenu::moreaction(Rml::Event &event)
    {
        event.StopPropagation();
        showfullmenu();
    }

    void RmlGameMenu::disconnectaction(Rml::Event &event)
    {
        event.StopPropagation();
        close();
        gamemenu::disconnect();
    }
}
