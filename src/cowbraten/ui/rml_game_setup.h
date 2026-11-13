#ifndef COWBRATEN_UI_RML_GAME_SETUP_H
#define COWBRATEN_UI_RML_GAME_SETUP_H

#include "game_setup_api.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <vector>

namespace Rml
{
    class Context;
    class Event;
}

namespace cowui
{
    class RmlActionRouter;

    class RmlGameSetup final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);
        void activate(int requested_section = -1);
        void update();

    private:
        struct ModeRow
        {
            int id = 0;
            Rml::String name;
            Rml::String description;
            bool selected = false;
        };

        struct MapRow
        {
            Rml::String name;
            Rml::String image;
            bool selected = false;
        };

        struct CampaignRow
        {
            int id = 0;
            Rml::String name;
            Rml::String map;
            Rml::String series;
        };

        bool loadcatalog();
        void rebuildmaps();
        void syncselection();
        void setsection(int value);
        void selectmode(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void selectmap(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void setfilter(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void setbotcount(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void setbotmin(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void setbotmax(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void launchcampaign(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);

        static void multiplayeraction(Rml::Event &event);
        static void botsaction(Rml::Event &event);
        static void campaignaction(Rml::Event &event);
        static void launchaction(Rml::Event &event);

        std::vector<ModeRow> modes;
        std::vector<MapRow> all_maps;
        std::vector<MapRow> visible_maps;
        std::vector<CampaignRow> campaigns;
        Rml::DataModelHandle model;

        Rml::String filter;
        Rml::String selected_map;
        Rml::String selected_mode_name;
        Rml::String selected_mode_description;
        Rml::String status;
        Rml::String current_match;
        int selected_mode = 0;
        int section = 0;
        int visible_count = 0;
        int bot_count = 0;
        int bot_min_skill = 50;
        int bot_max_skill = 100;
        bool can_launch = false;
        bool section_multiplayer = true;
        bool section_bots = false;
        bool section_campaign = false;
        bool catalog_loaded = false;

        static RmlGameSetup *instance;
    };
}

#endif
