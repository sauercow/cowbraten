#include "rml_game_setup.h"

#include "rml_action.h"
#include "ui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <cctype>

namespace cowui
{
    RmlGameSetup *RmlGameSetup::instance = nullptr;

    namespace
    {
        Rml::String lowercase(Rml::String value)
        {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return char(std::tolower(c)); });
            return value;
        }
    }

    bool RmlGameSetup::init(Rml::Context *context, RmlActionRouter &router)
    {
        if(!context || instance) return false;
        Rml::DataModelConstructor constructor =
            context->CreateDataModel("game_setup");
        if(!constructor) return false;

        bool valid = true;
        if(auto row = constructor.RegisterStruct<ModeRow>())
            valid = row.RegisterMember("id", &ModeRow::id) &&
                row.RegisterMember("name", &ModeRow::name) &&
                row.RegisterMember("description", &ModeRow::description) &&
                row.RegisterMember("selected", &ModeRow::selected);
        else valid = false;
        if(auto row = constructor.RegisterStruct<MapRow>())
            valid = valid && row.RegisterMember("name", &MapRow::name) &&
                row.RegisterMember("image", &MapRow::image) &&
                row.RegisterMember("selected", &MapRow::selected);
        else valid = false;
        if(auto row = constructor.RegisterStruct<CampaignRow>())
            valid = valid && row.RegisterMember("id", &CampaignRow::id) &&
                row.RegisterMember("name", &CampaignRow::name) &&
                row.RegisterMember("map", &CampaignRow::map) &&
                row.RegisterMember("series", &CampaignRow::series);
        else valid = false;

        valid = valid &&
            constructor.RegisterArray<std::vector<ModeRow>>() &&
            constructor.RegisterArray<std::vector<MapRow>>() &&
            constructor.RegisterArray<std::vector<CampaignRow>>() &&
            constructor.Bind("modes", &modes) &&
            constructor.Bind("maps", &visible_maps) &&
            constructor.Bind("campaigns", &campaigns) &&
            constructor.Bind("filter", &filter) &&
            constructor.Bind("selected_map", &selected_map) &&
            constructor.Bind("selected_mode", &selected_mode) &&
            constructor.Bind("selected_mode_name", &selected_mode_name) &&
            constructor.Bind("selected_mode_description", &selected_mode_description) &&
            constructor.Bind("status", &status) &&
            constructor.Bind("current_match", &current_match) &&
            constructor.Bind("visible_count", &visible_count) &&
            constructor.Bind("bot_count", &bot_count) &&
            constructor.Bind("bot_min_skill", &bot_min_skill) &&
            constructor.Bind("bot_max_skill", &bot_max_skill) &&
            constructor.Bind("can_launch", &can_launch) &&
            constructor.Bind("section_multiplayer", &section_multiplayer) &&
            constructor.Bind("section_bots", &section_bots) &&
            constructor.Bind("section_campaign", &section_campaign) &&
            constructor.BindEventCallback("select_mode", &RmlGameSetup::selectmode, this) &&
            constructor.BindEventCallback("select_map", &RmlGameSetup::selectmap, this) &&
            constructor.BindEventCallback("set_filter", &RmlGameSetup::setfilter, this) &&
            constructor.BindEventCallback("set_bot_count", &RmlGameSetup::setbotcount, this) &&
            constructor.BindEventCallback("set_bot_min", &RmlGameSetup::setbotmin, this) &&
            constructor.BindEventCallback("set_bot_max", &RmlGameSetup::setbotmax, this) &&
            constructor.BindEventCallback("launch_campaign", &RmlGameSetup::launchcampaign, this) &&
            router.registeraction("games.section.multiplayer", multiplayeraction) &&
            router.registeraction("games.section.bots", botsaction) &&
            router.registeraction("games.section.campaign", campaignaction) &&
            router.registeraction("games.launch", launchaction);

        if(!valid)
        {
            context->RemoveDataModel("game_setup");
            return false;
        }
        model = constructor.GetModelHandle();
        instance = this;
        return true;
    }

    void RmlGameSetup::shutdown(Rml::Context *context)
    {
        if(instance == this) instance = nullptr;
        if(context && model) context->RemoveDataModel("game_setup");
        model = Rml::DataModelHandle();
        modes.clear();
        all_maps.clear();
        visible_maps.clear();
        campaigns.clear();
        catalog_loaded = false;
    }

    bool RmlGameSetup::loadcatalog()
    {
        if(catalog_loaded) return true;
        std::vector<gamesetup::Mode> source_modes;
        std::vector<gamesetup::Map> source_maps;
        std::vector<gamesetup::Campaign> source_campaigns;
        gamesetup::readmodes(source_modes);
        gamesetup::readmaps(source_maps);
        gamesetup::readcampaigns(source_campaigns);
        for(const gamesetup::Mode &source : source_modes)
        {
            ModeRow row;
            row.id = source.id;
            row.name = source.name;
            row.description = source.description;
            modes.push_back(row);
        }
        for(const gamesetup::Map &source : source_maps)
        {
            MapRow row;
            row.name = source.name;
            row.image = source.image;
            all_maps.push_back(row);
        }
        for(const gamesetup::Campaign &source : source_campaigns)
        {
            CampaignRow row;
            row.id = source.id;
            row.name = source.name;
            row.map = source.map;
            row.series = source.series;
            campaigns.push_back(row);
        }
        catalog_loaded = true;
        model.DirtyVariable("modes");
        model.DirtyVariable("campaigns");
        rebuildmaps();
        return !modes.empty() && !all_maps.empty();
    }

    void RmlGameSetup::rebuildmaps()
    {
        visible_maps.clear();
        const Rml::String needle = lowercase(filter);
        for(MapRow row : all_maps)
        {
            row.selected = row.name == selected_map;
            if(needle.empty() || lowercase(row.name).find(needle) != Rml::String::npos)
                visible_maps.push_back(row);
        }
        visible_count = int(visible_maps.size());
        model.DirtyVariable("maps");
        model.DirtyVariable("visible_count");
    }

    void RmlGameSetup::syncselection()
    {
        for(ModeRow &row : modes)
        {
            row.selected = row.id == selected_mode;
            if(row.selected)
            {
                selected_mode_name = row.name;
                selected_mode_description = row.description;
            }
        }
        can_launch = !selected_map.empty();
        model.DirtyVariable("modes");
        model.DirtyVariable("selected_mode");
        model.DirtyVariable("selected_mode_name");
        model.DirtyVariable("selected_mode_description");
        model.DirtyVariable("selected_map");
        model.DirtyVariable("can_launch");
        rebuildmaps();
    }

    void RmlGameSetup::activate(int requested_section)
    {
        loadcatalog();
        gamesetup::State state;
        gamesetup::readstate(state);
        selected_mode = state.current_mode >= 0 && state.current_mode <= 22
            ? state.current_mode : 0;
        if(!state.current_map.empty()) selected_map = state.current_map;
        if(selected_map.empty() && !all_maps.empty()) selected_map = all_maps[0].name;
        syncselection();
        current_match = state.current_map.empty()
            ? "No active map"
            : Rml::String(state.current_map) + " / " + selected_mode_name;
        status = state.remote
            ? "Selecting a map sends a vote to the current server."
            : "The selected map starts on your local server.";
        model.DirtyVariable("current_match");
        model.DirtyVariable("status");
        if(requested_section >= 0) setsection(requested_section);
    }

    void RmlGameSetup::update() {}

    void RmlGameSetup::setsection(int value)
    {
        section = value;
        section_multiplayer = section == 0;
        section_bots = section == 1;
        section_campaign = section == 2;
        model.DirtyVariable("section_multiplayer");
        model.DirtyVariable("section_bots");
        model.DirtyVariable("section_campaign");
    }

    void RmlGameSetup::selectmode(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
        {
            selected_mode = arguments[0].Get<int>(0);
            syncselection();
        }
        event.StopPropagation();
    }

    void RmlGameSetup::selectmap(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
        {
            selected_map = arguments[0].Get<Rml::String>();
            syncselection();
        }
        event.StopPropagation();
    }

    void RmlGameSetup::setfilter(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
        {
            filter = arguments[0].Get<Rml::String>();
            model.DirtyVariable("filter");
            rebuildmaps();
        }
        event.StopPropagation();
    }

    void RmlGameSetup::setbotcount(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
        {
            bot_count = std::max(0, std::min(32, arguments[0].Get<int>()));
            model.DirtyVariable("bot_count");
        }
        event.StopPropagation();
    }

    void RmlGameSetup::setbotmin(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
        {
            bot_min_skill = std::max(1, std::min(101, arguments[0].Get<int>()));
            bot_max_skill = std::max(bot_max_skill, bot_min_skill);
            model.DirtyVariable("bot_min_skill");
            model.DirtyVariable("bot_max_skill");
        }
        event.StopPropagation();
    }

    void RmlGameSetup::setbotmax(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
        {
            bot_max_skill = std::max(1, std::min(101, arguments[0].Get<int>()));
            bot_min_skill = std::min(bot_min_skill, bot_max_skill);
            model.DirtyVariable("bot_min_skill");
            model.DirtyVariable("bot_max_skill");
        }
        event.StopPropagation();
    }

    void RmlGameSetup::launchcampaign(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() == 1)
        {
            std::string error;
            if(gamesetup::launchcampaign(arguments[0].Get<int>(-1), error)) close();
            else
            {
                status = error;
                model.DirtyVariable("status");
            }
        }
        event.StopPropagation();
    }

    void RmlGameSetup::multiplayeraction(Rml::Event &event)
    {
        if(instance) instance->setsection(0);
        event.StopPropagation();
    }
    void RmlGameSetup::botsaction(Rml::Event &event)
    {
        if(instance) instance->setsection(1);
        event.StopPropagation();
    }
    void RmlGameSetup::campaignaction(Rml::Event &event)
    {
        if(instance) instance->setsection(2);
        event.StopPropagation();
    }
    void RmlGameSetup::launchaction(Rml::Event &event)
    {
        if(instance && instance->can_launch)
        {
            std::string error;
            const int bots = instance->section_bots ? instance->bot_count : 0;
            if(gamesetup::launch(instance->selected_map.c_str(), instance->selected_mode,
                bots, instance->bot_min_skill, instance->bot_max_skill, error)) close();
            else
            {
                instance->status = error;
                instance->model.DirtyVariable("status");
            }
        }
        event.StopPropagation();
    }
}
