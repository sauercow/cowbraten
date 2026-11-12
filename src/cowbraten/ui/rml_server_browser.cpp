#include "rml_server_browser.h"

#include "rml_action.h"
#include "ui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/StringUtilities.h>

#include <algorithm>
#include <climits>
#include <cstdio>

namespace cowui
{
    RmlServerBrowser *RmlServerBrowser::instance = nullptr;

    static Rml::String lower(const Rml::String &value)
    {
        return Rml::StringUtilities::ToLower(value);
    }

    static bool contains(
        const Rml::String &value,
        const Rml::String &needle)
    {
        return needle.empty() ||
            lower(value).find(needle) != Rml::String::npos;
    }

    bool RmlServerBrowser::init(
        Rml::Context *context,
        RmlActionRouter &router)
    {
        if(!context || instance)
            return false;

        Rml::DataModelConstructor constructor =
            context->CreateDataModel("server_browser");

        if(!constructor)
            return false;

        bool valid = true;

        if(auto row = constructor.RegisterStruct<Row>())
        {
            valid =
                row.RegisterMember("id", &Row::id) &&
                row.RegisterMember("host", &Row::host) &&
                row.RegisterMember("description", &Row::description) &&
                row.RegisterMember("map", &Row::map) &&
                row.RegisterMember("mode", &Row::mode) &&
                row.RegisterMember("players", &Row::players) &&
                row.RegisterMember("ping", &Row::ping) &&
                row.RegisterMember("status", &Row::status) &&
                row.RegisterMember("selectable", &Row::selectable) &&
                row.RegisterMember("compatible", &Row::compatible) &&
                row.RegisterMember("full", &Row::full) &&
                row.RegisterMember("waiting", &Row::waiting) &&
                row.RegisterMember("incompatible", &Row::incompatible) &&
                row.RegisterMember("ping_good", &Row::ping_good) &&
                row.RegisterMember("ping_medium", &Row::ping_medium) &&
                row.RegisterMember("ping_slow", &Row::ping_slow);
        }
        else
        {
            valid = false;
        }

        valid = valid &&
            constructor.RegisterArray<std::vector<Row>>() &&
            constructor.Bind("servers", &visible_rows) &&
            constructor.Bind("filter", &filter) &&
            constructor.Bind("selected_id", &selected_id) &&
            constructor.Bind("selected_name", &selected_name) &&
            constructor.Bind("selected_details", &selected_details) &&
            constructor.Bind("list_status", &list_status) &&
            constructor.Bind("sort_status", &sort_status) &&
            constructor.Bind("total_count", &total_count) &&
            constructor.Bind("visible_count", &visible_count) &&
            constructor.Bind("can_connect", &can_connect) &&
            constructor.Bind("hide_empty", &hide_empty) &&
            constructor.Bind("compatible_only", &compatible_only) &&
            constructor.Bind("sort_name", &sort_name) &&
            constructor.Bind("sort_mode", &sort_mode) &&
            constructor.Bind("sort_map", &sort_map) &&
            constructor.Bind("sort_players", &sort_players) &&
            constructor.Bind("sort_ping", &sort_ping) &&
            constructor.Bind("sort_ascending", &sort_ascending) &&
            constructor.BindEventCallback(
                "select_server",
                &RmlServerBrowser::selectaction,
                this
            );

        valid = valid &&
            router.registeraction("servers.connect", connectaction) &&
            router.registeraction("servers.refresh", refreshaction) &&
            router.registeraction("servers.filter", filteraction) &&
            router.registeraction(
                "servers.toggle.hide-empty",
                hideemptyaction
            ) &&
            router.registeraction(
                "servers.toggle.compatible-only",
                compatibleonlyaction
            ) &&
            router.registeraction("servers.sort.name", sortnameaction) &&
            router.registeraction("servers.sort.mode", sortmodeaction) &&
            router.registeraction("servers.sort.map", sortmapaction) &&
            router.registeraction("servers.sort.players", sortplayersaction) &&
            router.registeraction("servers.sort.ping", sortpingaction);

        if(!valid)
        {
            context->RemoveDataModel("server_browser");
            return false;
        }

        model = constructor.GetModelHandle();
        list_status = "Open the browser to discover servers";
        sort_status = "Ping / Ascending";
        instance = this;
        return true;
    }

    void RmlServerBrowser::shutdown(Rml::Context *context)
    {
        if(instance == this)
            instance = nullptr;

        if(context && model)
            context->RemoveDataModel("server_browser");

        model = Rml::DataModelHandle();
        all_rows.clear();
        visible_rows.clear();
        filter.clear();
        selected_id.clear();
        selected_name.clear();
        selected_details.clear();
        list_status.clear();
        sort_status.clear();
        total_count = 0;
        visible_count = 0;
        can_connect = false;
        hide_empty = true;
        compatible_only = true;
        sort_name = false;
        sort_mode = false;
        sort_map = false;
        sort_players = false;
        sort_ping = true;
        sort_ascending = true;
        sort_key = SortKey::Ping;
        backend_revision = 0;
        backend_initialized = false;
    }

    void RmlServerBrowser::activate()
    {
        if(!backend_initialized)
        {
            list_status = "Discovering servers...";
            model.DirtyVariable("list_status");

            sauerservers::initialise();
            backend_initialized = true;
        }

        syncbackend(sauerservers::update());
    }

    void RmlServerBrowser::update()
    {
        if(!backend_initialized)
            return;

        syncbackend(sauerservers::update());
    }

    bool RmlServerBrowser::syncbackend(
        unsigned long long revision)
    {
        if(!revision || revision == backend_revision)
            return false;

        backend_revision = revision;
        all_rows.clear();
        sauerservers::visit(appendentry, this);
        total_count = int(all_rows.size());

        updateselection();
        rebuildvisible();
        model.DirtyVariable("total_count");
        return true;
    }

    void RmlServerBrowser::appendentry(
        const sauerservers::Entry &entry,
        void *user)
    {
        RmlServerBrowser &browser =
            *static_cast<RmlServerBrowser *>(user);

        Row row;
        row.id = entry.id;
        row.host = entry.host;
        row.description = entry.description[0]
            ? entry.description
            : entry.host;
        row.map = entry.map[0] ? entry.map : "--";
        row.mode = entry.mode[0] ? entry.mode : "--";
        row.status = entry.status;
        row.player_count = entry.players;
        row.ping_value = entry.ping;
        row.selectable = entry.selectable;
        row.compatible = entry.compatible;
        row.full = entry.full;
        row.waiting = entry.waiting;
        row.incompatible = entry.incompatible;

        row.players = entry.capacity >= 0
            ? Rml::CreateString(
                "%d / %d",
                entry.players,
                entry.capacity)
            : Rml::CreateString("%d", entry.players);

        row.ping = entry.ping >= 0
            ? Rml::CreateString("%d ms", entry.ping)
            : "--";

        row.ping_good = entry.ping >= 0 && entry.ping <= 80;
        row.ping_medium = entry.ping > 80 && entry.ping <= 150;
        row.ping_slow = entry.ping > 150;

        browser.all_rows.push_back(row);
    }

    void RmlServerBrowser::rebuildvisible()
    {
        visible_rows.clear();

        const Rml::String needle = lower(filter);

        for(const Row &row : all_rows)
        {
            if(hide_empty && row.player_count == 0)
                continue;

            if(compatible_only && !row.compatible)
                continue;

            if(!contains(row.description, needle) &&
               !contains(row.host, needle) &&
               !contains(row.mode, needle) &&
               !contains(row.map, needle))
                continue;

            visible_rows.push_back(row);
        }

        const SortKey key = sort_key;
        const bool ascending = sort_ascending;

        std::stable_sort(
            visible_rows.begin(),
            visible_rows.end(),
            [key, ascending](const Row &left, const Row &right)
            {
                int comparison = 0;

                switch(key)
                {
                    case SortKey::Name:
                        comparison = lower(left.description).compare(
                            lower(right.description));
                        break;

                    case SortKey::Mode:
                        comparison = lower(left.mode).compare(
                            lower(right.mode));
                        break;

                    case SortKey::Map:
                        comparison = lower(left.map).compare(
                            lower(right.map));
                        break;

                    case SortKey::Players:
                        comparison = left.player_count < right.player_count
                            ? -1
                            : left.player_count > right.player_count ? 1 : 0;
                        break;

                    case SortKey::Ping:
                    {
                        const int left_ping = left.ping_value < 0
                            ? INT_MAX
                            : left.ping_value;
                        const int right_ping = right.ping_value < 0
                            ? INT_MAX
                            : right.ping_value;
                        comparison = left_ping < right_ping
                            ? -1
                            : left_ping > right_ping ? 1 : 0;
                        break;
                    }
                }

                if(comparison == 0)
                {
                    comparison = lower(left.description).compare(
                        lower(right.description));
                }

                return ascending
                    ? comparison < 0
                    : comparison > 0;
            }
        );

        visible_count = int(visible_rows.size());

        if(total_count == 0)
            list_status = "No servers discovered";
        else if(visible_count == 0)
            list_status = "No servers match the active filters";
        else
            list_status = Rml::CreateString(
                "%d of %d servers shown",
                visible_count,
                total_count
            );

        model.DirtyVariable("servers");
        model.DirtyVariable("visible_count");
        model.DirtyVariable("list_status");
    }

    void RmlServerBrowser::updateselection()
    {
        Row *selected = nullptr;

        for(Row &row : all_rows)
        {
            if(!selected_id.empty() && row.id == selected_id)
                selected = &row;
        }

        if(selected)
        {
            selected_name = selected->description;
            selected_details = Rml::CreateString(
                "%s  //  %s  //  %s  //  %s",
                selected->id.c_str(),
                selected->mode.c_str(),
                selected->map.c_str(),
                selected->status.c_str()
            );
            can_connect = selected->selectable;
        }
        else
        {
            selected_id.clear();
            selected_name = "No server selected";
            selected_details =
                "Select a compatible server to inspect and connect";
            can_connect = false;
        }

        model.DirtyVariable("selected_name");
        model.DirtyVariable("selected_details");
        model.DirtyVariable("can_connect");
        model.DirtyVariable("selected_id");
    }

    void RmlServerBrowser::setsort(SortKey key)
    {
        if(sort_key == key)
        {
            sort_ascending = !sort_ascending;
        }
        else
        {
            sort_key = key;
            sort_ascending = key != SortKey::Players;
        }

        sort_name = key == SortKey::Name;
        sort_mode = key == SortKey::Mode;
        sort_map = key == SortKey::Map;
        sort_players = key == SortKey::Players;
        sort_ping = key == SortKey::Ping;

        const char *name = "Ping";
        switch(key)
        {
            case SortKey::Name: name = "Server"; break;
            case SortKey::Mode: name = "Mode"; break;
            case SortKey::Map: name = "Map"; break;
            case SortKey::Players: name = "Players"; break;
            case SortKey::Ping: name = "Ping"; break;
        }

        sort_status = Rml::CreateString(
            "%s / %s",
            name,
            sort_ascending ? "Ascending" : "Descending"
        );

        rebuildvisible();

        model.DirtyVariable("sort_name");
        model.DirtyVariable("sort_mode");
        model.DirtyVariable("sort_map");
        model.DirtyVariable("sort_players");
        model.DirtyVariable("sort_ping");
        model.DirtyVariable("sort_ascending");
        model.DirtyVariable("sort_status");
    }

    void RmlServerBrowser::selectaction(
        Rml::DataModelHandle,
        Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1)
            return;

        const Rml::String id = arguments[0].Get<Rml::String>();

        if(id.empty())
            return;

        selected_id = id;
        updateselection();
        event.StopPropagation();
    }

    void RmlServerBrowser::connectaction(Rml::Event &event)
    {
        if(!instance ||
           !instance->can_connect ||
           instance->selected_id.empty())
        {
            return;
        }

        const Rml::String id = instance->selected_id;
        if(sauerservers::connect(id.c_str()))
            close();
        else
            std::fprintf(
                stderr,
                "Cowbraten UI: selected server disappeared before connect\n"
            );

        event.StopPropagation();
    }

    void RmlServerBrowser::refreshaction(Rml::Event &event)
    {
        if(!instance)
            return;

        instance->list_status = "Refreshing master server list...";
        instance->model.DirtyVariable("list_status");

        sauerservers::refreshfrommaster();
        instance->syncbackend(sauerservers::update());
        event.StopPropagation();
    }

    void RmlServerBrowser::filteraction(Rml::Event &event)
    {
        if(!instance)
            return;

        instance->filter = event.GetParameter<Rml::String>(
            "value",
            Rml::String()
        );
        instance->rebuildvisible();
        event.StopPropagation();
    }

    void RmlServerBrowser::hideemptyaction(Rml::Event &event)
    {
        if(!instance)
            return;

        instance->hide_empty = event.GetParameter<bool>(
            "checked",
            false
        );
        instance->model.DirtyVariable("hide_empty");
        instance->rebuildvisible();
        event.StopPropagation();
    }

    void RmlServerBrowser::compatibleonlyaction(Rml::Event &event)
    {
        if(!instance)
            return;

        instance->compatible_only = event.GetParameter<bool>(
            "checked",
            false
        );
        instance->model.DirtyVariable("compatible_only");
        instance->rebuildvisible();
        event.StopPropagation();
    }

    void RmlServerBrowser::sortnameaction(Rml::Event &event)
    {
        if(instance)
            instance->setsort(SortKey::Name);
        event.StopPropagation();
    }

    void RmlServerBrowser::sortmodeaction(Rml::Event &event)
    {
        if(instance)
            instance->setsort(SortKey::Mode);
        event.StopPropagation();
    }

    void RmlServerBrowser::sortmapaction(Rml::Event &event)
    {
        if(instance)
            instance->setsort(SortKey::Map);
        event.StopPropagation();
    }

    void RmlServerBrowser::sortplayersaction(Rml::Event &event)
    {
        if(instance)
            instance->setsort(SortKey::Players);
        event.StopPropagation();
    }

    void RmlServerBrowser::sortpingaction(Rml::Event &event)
    {
        if(instance)
            instance->setsort(SortKey::Ping);
        event.StopPropagation();
    }
}
