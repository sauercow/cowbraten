#ifndef COWBRATEN_UI_RML_SERVER_BROWSER_H
#define COWBRATEN_UI_RML_SERVER_BROWSER_H

#include "sauer_server_bridge.h"

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

    class RmlServerBrowser final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);

        void activate();
        void update();

    private:
        struct Row
        {
            Rml::String id;
            Rml::String host;
            Rml::String description;
            Rml::String map;
            Rml::String mode;
            Rml::String players;
            Rml::String ping;
            Rml::String status;

            int player_count = 0;
            int ping_value = -1;

            bool selectable = false;
            bool compatible = false;
            bool full = false;
            bool waiting = false;
            bool incompatible = false;
            bool ping_good = false;
            bool ping_medium = false;
            bool ping_slow = false;
        };

        enum class SortKey
        {
            Name,
            Mode,
            Map,
            Players,
            Ping
        };

        bool syncbackend(unsigned long long revision);
        void rebuildvisible();
        void updateselection();
        void setsort(SortKey key);

        static void appendentry(
            const sauerservers::Entry &entry,
            void *);
        void selectaction(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments);
        static void connectaction(Rml::Event &event);
        static void refreshaction(Rml::Event &event);
        static void filteraction(Rml::Event &event);
        static void hideemptyaction(Rml::Event &event);
        static void compatibleonlyaction(Rml::Event &event);
        static void sortnameaction(Rml::Event &event);
        static void sortmodeaction(Rml::Event &event);
        static void sortmapaction(Rml::Event &event);
        static void sortplayersaction(Rml::Event &event);
        static void sortpingaction(Rml::Event &event);

        std::vector<Row> all_rows;
        std::vector<Row> visible_rows;
        Rml::DataModelHandle model;

        Rml::String filter;
        Rml::String selected_id;
        Rml::String selected_name;
        Rml::String selected_details;
        Rml::String list_status;
        Rml::String sort_status;

        int total_count = 0;
        int visible_count = 0;

        bool can_connect = false;
        bool hide_empty = false;
        bool compatible_only = false;
        bool sort_name = false;
        bool sort_mode = false;
        bool sort_map = false;
        bool sort_players = false;
        bool sort_ping = true;
        bool sort_ascending = true;

        SortKey sort_key = SortKey::Ping;
        unsigned long long backend_revision = 0;
        bool backend_initialized = false;

        static RmlServerBrowser *instance;
    };
}

#endif
