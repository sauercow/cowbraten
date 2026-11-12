#include "rml_demo_manager.h"

#include "rml_action.h"
#include "sauer_demo_bridge.h"
#include "ui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace cowui
{
    RmlDemoManager *RmlDemoManager::instance = nullptr;

    namespace
    {
        Rml::String duration_text(int milliseconds)
        {
            if(milliseconds < 0) return "Unknown";
            const int seconds = milliseconds / 1000;
            return Rml::CreateString("%d:%02d", seconds / 60, seconds % 60);
        }

        Rml::String size_text(std::int64_t bytes)
        {
            if(bytes >= 1024 * 1024) return Rml::CreateString("%.1f MB", bytes / (1024.0 * 1024.0));
            if(bytes >= 1024) return Rml::CreateString("%.1f KB", bytes / 1024.0);
            return Rml::CreateString("%lld B", static_cast<long long>(bytes));
        }

        void stop(Rml::Event &event) { event.StopPropagation(); }
        Rml::String event_value(Rml::Event &event) { return event.GetParameter<Rml::String>("value", Rml::String()); }
        bool event_checked(Rml::Event &event) { return event.GetParameter<bool>("checked", false); }

        int minutes_value(const Rml::String &text)
        {
            if(text.empty()) return -1;
            char *end = nullptr;
            const double value = std::strtod(text.c_str(), &end);
            if(end == text.c_str() || *end || value < 0 || value > 100000) return -1;
            return int(value * 60000.0);
        }
    }

    bool RmlDemoManager::init(Rml::Context *context, RmlActionRouter &router)
    {
        if(!context || instance) return false;
        Rml::DataModelConstructor constructor = context->CreateDataModel("demo_manager");
        if(!constructor) return false;
        bool valid = true;
        if(auto row = constructor.RegisterStruct<Row>())
        {
            valid = row.RegisterMember("id", &Row::id) && row.RegisterMember("name", &Row::name) &&
                row.RegisterMember("format", &Row::format) && row.RegisterMember("map", &Row::map) &&
                row.RegisterMember("mode", &Row::mode) && row.RegisterMember("date", &Row::date) &&
                row.RegisterMember("duration", &Row::duration) && row.RegisterMember("players", &Row::players) &&
                row.RegisterMember("spectators", &Row::spectators) && row.RegisterMember("size", &Row::size) &&
                row.RegisterMember("server", &Row::server) && row.RegisterMember("category", &Row::category) &&
                row.RegisterMember("status", &Row::status) && row.RegisterMember("selected", &Row::selected) &&
                row.RegisterMember("favorite", &Row::favorite) && row.RegisterMember("playable", &Row::playable);
        }
        else valid = false;
        valid = valid && constructor.RegisterArray<std::vector<Row>>() && constructor.Bind("demos", &rows) &&
            constructor.Bind("status", &status) && constructor.Bind("status_error", &status_error) &&
            constructor.Bind("selected_name", &selected_name) && constructor.Bind("selected_details", &selected_details) &&
            constructor.Bind("selected_participants", &selected_participants) && constructor.Bind("rename_value", &rename_value) &&
            constructor.Bind("move_value", &move_value) && constructor.Bind("confirm_text", &confirm_text) &&
            constructor.Bind("search_value", &search_value) && constructor.Bind("mode_value", &mode_value) &&
            constructor.Bind("player_value", &player_value) && constructor.Bind("map_value", &map_value) &&
            constructor.Bind("server_value", &server_value) && constructor.Bind("category_value", &category_value) &&
            constructor.Bind("date_value", &date_value) && constructor.Bind("duration_value", &duration_value) &&
            constructor.Bind("duration_min_value", &duration_min_value) && constructor.Bind("duration_max_value", &duration_max_value) &&
            constructor.Bind("format_value", &format_value) && constructor.Bind("size_value", &size_value) &&
            constructor.Bind("retention_value", &retention_value) &&
            constructor.Bind("result_count", &result_count) && constructor.Bind("trash_usage", &trash_usage) &&
            constructor.Bind("total_count", &total_count) && constructor.Bind("visible_count", &visible_count) &&
            constructor.Bind("selected_count", &selected_count) &&
            constructor.Bind("sort_ascending", &sort_ascending) && constructor.Bind("tab_library", &tab_library_value) &&
            constructor.Bind("tab_recording", &tab_recording_value) && constructor.Bind("tab_playback", &tab_playback_value) &&
            constructor.Bind("tab_trash", &tab_trash_value) && constructor.Bind("confirm_visible", &confirm_visible) &&
            constructor.Bind("can_play", &can_play) && constructor.Bind("has_selection", &has_selection) &&
            constructor.Bind("single_selection", &single_selection) && constructor.Bind("hd_playback", &hd_playback) &&
            constructor.Bind("hd_recording", &hd_recording) && constructor.Bind("automatic_recording", &automatic_recording) &&
            constructor.Bind("recording_active", &recording_active) && constructor.Bind("playback_active", &playback_active) &&
            constructor.Bind("playback_paused", &playback_paused) && constructor.Bind("favorites_only", &favorites_only) &&
            constructor.Bind("active_players_only", &active_players_only) &&
            constructor.Bind("saved_filter_available", &saved_filter_available) &&
            constructor.BindEventCallback("select_demo", &RmlDemoManager::select_callback, this) &&
            constructor.BindEventCallback("toggle_demo", &RmlDemoManager::toggle_callback, this);

#define ACTION(name, fn) valid = valid && router.registeraction(name, fn)
        ACTION("demos.tab.library", tab_library); ACTION("demos.tab.recording", tab_recording);
        ACTION("demos.tab.playback", tab_playback); ACTION("demos.tab.trash", tab_trash);
        ACTION("demos.refresh", refresh); ACTION("demos.filters.clear", clear_filters);
        ACTION("demos.filter.search", filter_search); ACTION("demos.filter.mode", filter_mode);
        ACTION("demos.filter.player", filter_player); ACTION("demos.filter.map", filter_map);
        ACTION("demos.filter.server", filter_server); ACTION("demos.filter.category", filter_category);
        ACTION("demos.filter.duration", filter_duration); ACTION("demos.filter.format", filter_format);
        ACTION("demos.filter.date", filter_date); ACTION("demos.filter.size", filter_size);
        ACTION("demos.filter.duration-min", filter_duration_min); ACTION("demos.filter.duration-max", filter_duration_max);
        ACTION("demos.filter.favorites", filter_favorites); ACTION("demos.filter.active", filter_active_players);
        ACTION("demos.select-filtered", select_filtered); ACTION("demos.filters.save", save_filter);
        ACTION("demos.filters.load", load_filter);
        ACTION("demos.sort.name", sort_name); ACTION("demos.sort.date", sort_date);
        ACTION("demos.sort.duration", sort_duration); ACTION("demos.sort.mode", sort_mode);
        ACTION("demos.sort.map", sort_map); ACTION("demos.sort.size", sort_size);
        ACTION("demos.play", play); ACTION("demos.favorite", favorite);
        ACTION("demos.rename.input", rename_input); ACTION("demos.move.input", move_input);
        ACTION("demos.rename", rename_selected); ACTION("demos.move", move_selected);
        ACTION("demos.trash.request", request_trash); ACTION("demos.organize.request", request_organize);
        ACTION("demos.restore", restore); ACTION("demos.erase.request", request_erase);
        ACTION("demos.empty.request", request_empty_trash); ACTION("demos.confirm", confirm);
        ACTION("demos.trash.retention", trash_retention);
        ACTION("demos.confirm.cancel", cancel_confirm); ACTION("demos.classify.duel", classify_duel);
        ACTION("demos.classify.match", classify_match); ACTION("demos.classify.unknown", classify_unknown);
        ACTION("demos.open-location", open_location); ACTION("demos.settings.hd-playback", toggle_hd_playback);
        ACTION("demos.settings.hd-recording", toggle_hd_recording);
        ACTION("demos.settings.auto-recording", toggle_auto_recording);
        ACTION("demos.playback.stop", playback_stop); ACTION("demos.playback.pause", playback_pause);
        ACTION("demos.playback.back", playback_back); ACTION("demos.playback.forward", playback_forward);
        ACTION("demos.playback.speed", playback_speed);
#undef ACTION
        if(!valid)
        {
            context->RemoveDataModel("demo_manager");
            return false;
        }
        model = constructor.GetModelHandle();
        status = "Open Demos to scan the library";
        selected_name = "No demo selected";
        selected_details = "Select a row to inspect metadata";
        move_value = "unclassified";
        instance = this;
        return true;
    }

    void RmlDemoManager::shutdown(Rml::Context *context)
    {
        library.close(); rows.clear(); selected.clear();
        if(instance == this) instance = nullptr;
        if(context && model) context->RemoveDataModel("demo_manager");
        model = Rml::DataModelHandle();
    }

    void RmlDemoManager::activate()
    {
        if(library.root_path().empty())
        {
            std::string error;
            if(!library.open(sauerdemo::demo_directory(), &error)) set_status(error.c_str(), true);
            else set_status("Discovering demos...");
        }
        {
            std::ifstream saved(library.root_path() + "/.cowbraten-demo-filter-v1.txt");
            saved_filter_available = bool(saved);
            model.DirtyVariable("saved_filter_available");
        }
        {
            std::ifstream retention(library.root_path() + "/.cowbraten-demo-retention-v1.txt");
            int days = 0; if(retention >> days) retention_value = days > 0 ? Rml::CreateString("%d", days) : "never";
            std::string error; library.purge_expired(days, &error);
            model.DirtyVariable("retention_value");
            if(!error.empty()) set_status(error.c_str(), true);
        }
        sync_settings();
        rebuild();
    }

    void RmlDemoManager::update()
    {
        if(library.refreshing())
        {
            const bool finished = library.refresh_step(1);
            status = finished ? "Library index is up to date" : Rml::CreateString("Indexing %d of %d...",
                int(library.refresh_done()), int(library.refresh_total()));
            model.DirtyVariable("status");
            rebuild();
        }
        sync_settings();
    }

    void RmlDemoManager::showlibrary() { set_tab(Tab::Library); }
    void RmlDemoManager::showrecording() { set_tab(Tab::Recording); }
    void RmlDemoManager::showplayback() { set_tab(Tab::Playback); }
    void RmlDemoManager::showtrash() { set_tab(Tab::Trash); }
    void RmlDemoManager::showduels()
    {
        set_tab(Tab::Library);
        category_value = "duel";
        filter.category = 1;
        model.DirtyVariable("category_value");
        rebuild();
    }

    void RmlDemoManager::setsearch(const char *value)
    {
        search_value = value ? value : "";
        filter.search = search_value.c_str();
        model.DirtyVariable("search_value");
        rebuild();
    }

    void RmlDemoManager::selectfirst()
    {
        selected.clear();
        if(!rows.empty()) selected.insert(rows.front().id.c_str());
        rebuild();
    }

    bool RmlDemoManager::playselected()
    {
        if(selected.size() != 1) return false;
        const demos::Entry *entry = library.find(*selected.begin());
        if(!entry || !entry->playable || entry->trashed) return false;
        return sauerdemo::play(library.relative_path(*entry).c_str());
    }

    void RmlDemoManager::sync_settings()
    {
        const sauerdemo::Settings settings = sauerdemo::settings();
        const bool recording = sauerdemo::recording(), playing = sauerdemo::playback(), paused = sauerdemo::paused();
#define SYNC(variable, value) if(variable != value) { variable = value; model.DirtyVariable(#variable); }
        SYNC(hd_playback, settings.hd_playback); SYNC(hd_recording, settings.hd_recording);
        SYNC(automatic_recording, settings.automatic_recording); SYNC(recording_active, recording);
        SYNC(playback_active, playing); SYNC(playback_paused, paused);
#undef SYNC
    }

    void RmlDemoManager::set_status(const Rml::String &value, bool error)
    {
        status = value; status_error = error;
        model.DirtyVariable("status"); model.DirtyVariable("status_error");
    }

    void RmlDemoManager::rebuild()
    {
        filter.trash = tab == Tab::Trash;
        filter.favorites_only = favorites_only;
        filter.active_players_only = active_players_only;
        rows.clear();
        const std::vector<std::size_t> indices = library.query(filter, sort_key, sort_ascending);
        for(std::size_t index : indices)
        {
            const demos::Entry &entry = library.entries()[index];
            Row row;
            row.id = entry.id; row.name = entry.name; row.format = entry.format;
            row.map = entry.map.empty() ? "Unknown" : entry.map.c_str();
            row.mode = entry.mode.empty() ? "Unknown" : entry.mode.c_str();
            row.date = entry.date; row.duration = duration_text(entry.duration_ms);
            int players = 0, spectators = 0;
            for(const demos::Participant &participant : entry.participants)
            {
                if(participant.role == demos::Role::Active) ++players;
                else if(participant.role == demos::Role::Spectator) ++spectators;
            }
            row.players = entry.participants.empty() ? "Unknown" : Rml::CreateString("%d", players);
            row.spectators = entry.participants.empty() ? "Unknown" : Rml::CreateString("%d", spectators);
            row.size = size_text(entry.size); row.server = entry.server.empty() ? "Unknown" : entry.server.c_str();
            row.category = demos::classification_name(entry.classification); row.status = entry.status;
            row.selected = selected.count(entry.id) != 0; row.favorite = entry.favorite; row.playable = entry.playable;
            rows.push_back(std::move(row));
        }
        total_count = 0;
        for(const demos::Entry &entry : library.entries()) if(entry.trashed == filter.trash) ++total_count;
        visible_count = int(rows.size());
        result_count = Rml::CreateString("%d of %d demos", visible_count, total_count);
        trash_usage = Rml::CreateString("Trash: %s", size_text(library.trash_size()).c_str());
        model.DirtyVariable("demos"); model.DirtyVariable("total_count");
        model.DirtyVariable("visible_count"); model.DirtyVariable("result_count"); model.DirtyVariable("trash_usage");
        update_selection();
    }

    void RmlDemoManager::update_selection()
    {
        for(auto iterator = selected.begin(); iterator != selected.end();)
            if(!library.find(*iterator)) iterator = selected.erase(iterator); else ++iterator;
        selected_count = int(selected.size()); has_selection = selected_count > 0; single_selection = selected_count == 1;
        const demos::Entry *entry = single_selection ? library.find(*selected.begin()) : nullptr;
        can_play = entry && entry->playable && !entry->trashed && !sauerdemo::recording();
        if(entry)
        {
            selected_name = entry->name;
            selected_details = Rml::CreateString("%s  //  %s  //  %s  //  %s",
                entry->mode.empty() ? "Unknown mode" : entry->mode.c_str(), entry->map.empty() ? "Unknown map" : entry->map.c_str(),
                duration_text(entry->duration_ms).c_str(), demos::classification_name(entry->classification));
            std::ostringstream people;
            for(const demos::Participant &participant : entry->participants)
            {
                if(people.tellp() > 0) people << "  //  ";
                people << participant.name << " (" << demos::role_name(participant.role) << ')';
            }
            selected_participants = people.str().empty() ? "Participant metadata unavailable" : people.str().c_str();
            rename_value = entry->name;
        }
        else if(selected_count > 1)
        {
            selected_name = Rml::CreateString("%d demos selected", selected_count);
            selected_details = "Batch operations apply only to the current selection";
            selected_participants.clear(); rename_value.clear();
        }
        else
        {
            selected_name = "No demo selected"; selected_details = "Select a row to inspect metadata";
            selected_participants.clear(); rename_value.clear();
        }
        model.DirtyVariable("selected_count"); model.DirtyVariable("has_selection");
        model.DirtyVariable("single_selection"); model.DirtyVariable("can_play");
        model.DirtyVariable("selected_name"); model.DirtyVariable("selected_details");
        model.DirtyVariable("selected_participants"); model.DirtyVariable("rename_value");
    }

    std::vector<std::string> RmlDemoManager::selected_ids(bool trash) const
    {
        std::vector<std::string> result;
        for(const std::string &id : selected)
        {
            const demos::Entry *entry = library.find(id);
            if(entry && entry->trashed == trash) result.push_back(id);
        }
        return result;
    }

    void RmlDemoManager::set_tab(Tab value)
    {
        tab = value; tab_library_value = value == Tab::Library; tab_recording_value = value == Tab::Recording;
        tab_playback_value = value == Tab::Playback; tab_trash_value = value == Tab::Trash;
        selected.clear(); confirm_visible = false; pending = Pending::None;
        model.DirtyVariable("tab_library"); model.DirtyVariable("tab_recording");
        model.DirtyVariable("tab_playback"); model.DirtyVariable("tab_trash"); model.DirtyVariable("confirm_visible");
        rebuild();
    }

    void RmlDemoManager::select_callback(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1) return;
        const std::string id = arguments[0].Get<Rml::String>();
        selected.clear(); if(library.find(id)) selected.insert(id);
        rebuild(); stop(event);
    }

    void RmlDemoManager::toggle_callback(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &arguments)
    {
        if(arguments.size() != 1) return;
        const std::string id = arguments[0].Get<Rml::String>();
        if(selected.count(id)) selected.erase(id); else if(library.find(id)) selected.insert(id);
        rebuild(); stop(event);
    }

#define TAB_ACTION(name, value) void RmlDemoManager::name(Rml::Event &e) { if(instance) instance->set_tab(Tab::value); stop(e); }
    TAB_ACTION(tab_library, Library) TAB_ACTION(tab_recording, Recording) TAB_ACTION(tab_playback, Playback) TAB_ACTION(tab_trash, Trash)
#undef TAB_ACTION

    void RmlDemoManager::refresh(Rml::Event &e) { if(instance) { instance->library.begin_refresh(); instance->set_status("Discovering demos..."); } stop(e); }
    void RmlDemoManager::clear_filters(Rml::Event &e)
    {
        if(instance)
        {
            const bool trash = instance->filter.trash; instance->filter = demos::Filter(); instance->filter.trash = trash;
            instance->favorites_only = instance->active_players_only = false;
            instance->search_value.clear(); instance->mode_value.clear(); instance->player_value.clear();
            instance->map_value.clear(); instance->server_value.clear(); instance->date_value.clear();
            instance->format_value.clear(); instance->duration_min_value.clear(); instance->duration_max_value.clear();
            instance->category_value = "all"; instance->duration_value = "all"; instance->size_value = "all";
            instance->model.DirtyVariable("favorites_only"); instance->model.DirtyVariable("active_players_only");
            instance->model.DirtyVariable("search_value"); instance->model.DirtyVariable("mode_value");
            instance->model.DirtyVariable("player_value"); instance->model.DirtyVariable("map_value");
            instance->model.DirtyVariable("server_value"); instance->model.DirtyVariable("format_value");
            instance->model.DirtyVariable("date_value"); instance->model.DirtyVariable("category_value");
            instance->model.DirtyVariable("duration_value"); instance->model.DirtyVariable("duration_min_value");
            instance->model.DirtyVariable("duration_max_value"); instance->model.DirtyVariable("size_value");
            instance->rebuild();
        }
        stop(e);
    }
#define TEXT_FILTER(name, member, value_member) void RmlDemoManager::name(Rml::Event &e) { if(instance) { instance->value_member = event_value(e); instance->filter.member = instance->value_member.c_str(); instance->model.DirtyVariable(#value_member); instance->rebuild(); } stop(e); }
    TEXT_FILTER(filter_search, search, search_value) TEXT_FILTER(filter_mode, mode, mode_value) TEXT_FILTER(filter_player, player, player_value)
    TEXT_FILTER(filter_map, map, map_value) TEXT_FILTER(filter_server, server, server_value) TEXT_FILTER(filter_format, format, format_value)
    TEXT_FILTER(filter_date, date, date_value)
#undef TEXT_FILTER
    void RmlDemoManager::filter_category(Rml::Event &e)
    {
        if(instance)
        {
            const Rml::String value = event_value(e);
            instance->category_value = value; instance->model.DirtyVariable("category_value");
            instance->filter.category = value == "duel" ? 1 : value == "match" ? 2 : value == "unknown" ? 3 : 0;
            instance->rebuild();
        }
        stop(e);
    }
    void RmlDemoManager::filter_duration(Rml::Event &e)
    {
        if(instance)
        {
            const Rml::String value = event_value(e); int minimum = -1, maximum = -1;
            instance->duration_value = value; instance->model.DirtyVariable("duration_value");
            if(value == "lt2") maximum = 2 * 60000; else if(value == "lt5") maximum = 5 * 60000;
            else if(value == "lt10") maximum = 10 * 60000; else if(value == "lt15") maximum = 15 * 60000;
            else if(value == "10to20") { minimum = 10 * 60000; maximum = 20 * 60000; }
            else if(value == "gt20") minimum = 20 * 60000;
            else if(value == "custom") { minimum = minutes_value(instance->duration_min_value); maximum = minutes_value(instance->duration_max_value); }
            instance->filter.duration_min_ms = minimum; instance->filter.duration_max_ms = maximum; instance->rebuild();
        }
        stop(e);
    }
    void RmlDemoManager::filter_duration_min(Rml::Event &e)
    {
        if(instance) { instance->duration_min_value = event_value(e); instance->duration_value = "custom";
            instance->filter.duration_min_ms = minutes_value(instance->duration_min_value);
            instance->filter.duration_max_ms = minutes_value(instance->duration_max_value);
            instance->model.DirtyVariable("duration_min_value"); instance->model.DirtyVariable("duration_value"); instance->rebuild(); }
        stop(e);
    }
    void RmlDemoManager::filter_duration_max(Rml::Event &e)
    {
        if(instance) { instance->duration_max_value = event_value(e); instance->duration_value = "custom";
            instance->filter.duration_min_ms = minutes_value(instance->duration_min_value);
            instance->filter.duration_max_ms = minutes_value(instance->duration_max_value);
            instance->model.DirtyVariable("duration_max_value"); instance->model.DirtyVariable("duration_value"); instance->rebuild(); }
        stop(e);
    }
    void RmlDemoManager::filter_size(Rml::Event &e)
    {
        if(instance)
        {
            const Rml::String value = event_value(e); instance->size_value = value;
            instance->filter.size_min = instance->filter.size_max = -1;
            if(value == "lt1") instance->filter.size_max = 1024 * 1024;
            else if(value == "1to10") { instance->filter.size_min = 1024 * 1024; instance->filter.size_max = 10 * 1024 * 1024; }
            else if(value == "gt10") instance->filter.size_min = 10 * 1024 * 1024;
            instance->model.DirtyVariable("size_value"); instance->rebuild();
        }
        stop(e);
    }
    void RmlDemoManager::filter_favorites(Rml::Event &e) { if(instance) { instance->favorites_only = event_checked(e); instance->model.DirtyVariable("favorites_only"); instance->rebuild(); } stop(e); }
    void RmlDemoManager::filter_active_players(Rml::Event &e) { if(instance) { instance->active_players_only = event_checked(e); instance->model.DirtyVariable("active_players_only"); instance->rebuild(); } stop(e); }

    void RmlDemoManager::select_filtered(Rml::Event &e)
    {
        if(instance) { instance->selected.clear(); for(const Row &row : instance->rows) instance->selected.insert(row.id.c_str()); instance->rebuild(); }
        stop(e);
    }
    void RmlDemoManager::save_filter(Rml::Event &e)
    {
        if(instance && !instance->library.root_path().empty())
        {
            std::ofstream output(instance->library.root_path() + "/.cowbraten-demo-filter-v1.txt", std::ios::trunc);
            output << std::quoted(std::string(instance->search_value.c_str())) << ' ' << std::quoted(std::string(instance->mode_value.c_str())) << ' '
                << std::quoted(std::string(instance->player_value.c_str())) << ' ' << std::quoted(std::string(instance->map_value.c_str())) << ' '
                << std::quoted(std::string(instance->server_value.c_str())) << ' ' << std::quoted(std::string(instance->date_value.c_str())) << ' '
                << std::quoted(std::string(instance->category_value.c_str())) << ' ' << std::quoted(std::string(instance->duration_value.c_str())) << ' '
                << std::quoted(std::string(instance->duration_min_value.c_str())) << ' ' << std::quoted(std::string(instance->duration_max_value.c_str())) << ' '
                << std::quoted(std::string(instance->format_value.c_str())) << ' ' << std::quoted(std::string(instance->size_value.c_str())) << ' '
                << instance->favorites_only << ' ' << instance->active_players_only << '\n';
            instance->saved_filter_available = bool(output); instance->model.DirtyVariable("saved_filter_available");
            instance->set_status(output ? "Saved filter updated" : "Could not save filter", !output);
        }
        stop(e);
    }
    void RmlDemoManager::load_filter(Rml::Event &e)
    {
        if(instance && !instance->library.root_path().empty())
        {
            std::ifstream input(instance->library.root_path() + "/.cowbraten-demo-filter-v1.txt");
            std::string search, mode, player, map, server, date, category, duration, duration_min, duration_max, format, size;
            bool favorites = false, active = false;
            if(input >> std::quoted(search) >> std::quoted(mode) >> std::quoted(player) >> std::quoted(map) >> std::quoted(server)
                >> std::quoted(date) >> std::quoted(category) >> std::quoted(duration) >> std::quoted(duration_min)
                >> std::quoted(duration_max) >> std::quoted(format) >> std::quoted(size) >> favorites >> active)
            {
                instance->search_value = search.c_str(); instance->mode_value = mode.c_str(); instance->player_value = player.c_str();
                instance->map_value = map.c_str(); instance->server_value = server.c_str(); instance->date_value = date.c_str();
                instance->category_value = category.c_str(); instance->duration_value = duration.c_str();
                instance->duration_min_value = duration_min.c_str(); instance->duration_max_value = duration_max.c_str();
                instance->format_value = format.c_str(); instance->size_value = size.c_str();
                instance->favorites_only = favorites; instance->active_players_only = active;
                instance->filter.search = search; instance->filter.mode = mode; instance->filter.player = player; instance->filter.map = map;
                instance->filter.server = server; instance->filter.date = date; instance->filter.format = format;
                instance->filter.category = category == "duel" ? 1 : category == "match" ? 2 : category == "unknown" ? 3 : 0;
                instance->filter.duration_min_ms = instance->filter.duration_max_ms = -1;
                if(duration == "lt2") instance->filter.duration_max_ms = 2 * 60000;
                else if(duration == "lt5") instance->filter.duration_max_ms = 5 * 60000;
                else if(duration == "lt10") instance->filter.duration_max_ms = 10 * 60000;
                else if(duration == "lt15") instance->filter.duration_max_ms = 15 * 60000;
                else if(duration == "10to20") { instance->filter.duration_min_ms = 10 * 60000; instance->filter.duration_max_ms = 20 * 60000; }
                else if(duration == "gt20") instance->filter.duration_min_ms = 20 * 60000;
                else if(duration == "custom") { instance->filter.duration_min_ms = minutes_value(instance->duration_min_value); instance->filter.duration_max_ms = minutes_value(instance->duration_max_value); }
                instance->filter.size_min = size == "1to10" || size == "gt10" ? 1024 * 1024 : -1;
                if(size == "gt10") instance->filter.size_min = 10 * 1024 * 1024;
                instance->filter.size_max = size == "lt1" ? 1024 * 1024 : size == "1to10" ? 10 * 1024 * 1024 : -1;
                instance->model.DirtyAllVariables(); instance->rebuild(); instance->set_status("Saved filter restored");
            }
            else instance->set_status("No saved filter is available", true);
        }
        stop(e);
    }

#define SORT_ACTION(name, key) void RmlDemoManager::name(Rml::Event &e) { if(instance) { if(instance->sort_key == demos::SortKey::key) instance->sort_ascending = !instance->sort_ascending; else { instance->sort_key = demos::SortKey::key; instance->sort_ascending = true; } instance->model.DirtyVariable("sort_ascending"); instance->rebuild(); } stop(e); }
    SORT_ACTION(sort_name, Name) SORT_ACTION(sort_date, Date) SORT_ACTION(sort_duration, Duration)
    SORT_ACTION(sort_mode, Mode) SORT_ACTION(sort_map, Map) SORT_ACTION(sort_size, Size)
#undef SORT_ACTION

    void RmlDemoManager::play(Rml::Event &e)
    {
        if(instance && instance->single_selection)
        {
            const demos::Entry *entry = instance->library.find(*instance->selected.begin());
            if(entry && entry->playable && sauerdemo::play(instance->library.relative_path(*entry).c_str())) close();
            else instance->set_status("Demo cannot be played while recording or when corrupt", true);
        }
        stop(e);
    }
    void RmlDemoManager::favorite(Rml::Event &e)
    {
        if(instance)
        {
            std::string error;
            for(const std::string &id : instance->selected_ids(false))
            {
                const demos::Entry *entry = instance->library.find(id);
                if(entry) instance->library.set_favorite(id, !entry->favorite, &error);
            }
            instance->set_status(error.empty() ? "Favorite state updated" : error.c_str(), !error.empty()); instance->rebuild();
        }
        stop(e);
    }
    void RmlDemoManager::rename_input(Rml::Event &e) { if(instance) { instance->rename_value = event_value(e); instance->model.DirtyVariable("rename_value"); } stop(e); }
    void RmlDemoManager::move_input(Rml::Event &e) { if(instance) { instance->move_value = event_value(e); instance->model.DirtyVariable("move_value"); } stop(e); }
    void RmlDemoManager::rename_selected(Rml::Event &e)
    {
        if(instance && instance->single_selection)
        {
            std::string error;
            const std::string id = *instance->selected.begin();
            if(instance->library.rename_entry(id, instance->rename_value.c_str(), &error))
            {
                instance->selected.clear(); instance->set_status("Demo renamed"); instance->library.begin_refresh();
            }
            else instance->set_status(error.c_str(), true);
            instance->rebuild();
        }
        stop(e);
    }
    void RmlDemoManager::move_selected(Rml::Event &e)
    {
        if(instance)
        {
            std::string error;
            const std::vector<std::string> ids = instance->selected_ids(false);
            for(const std::string &id : ids) if(!instance->library.move_entry(id, instance->move_value.c_str(), &error)) break;
            instance->selected.clear(); instance->set_status(error.empty() ? "Demo files moved" : error.c_str(), !error.empty()); instance->rebuild();
        }
        stop(e);
    }
    void RmlDemoManager::request_trash(Rml::Event &e)
    {
        if(instance && instance->has_selection)
        {
            instance->pending = Pending::Trash; instance->confirm_text = Rml::CreateString("Move %d selected demo(s) to Trash?", instance->selected_count);
            instance->confirm_visible = true; instance->model.DirtyVariable("confirm_text"); instance->model.DirtyVariable("confirm_visible");
        }
        stop(e);
    }
    void RmlDemoManager::request_organize(Rml::Event &e)
    {
        if(instance && instance->has_selection)
        {
            instance->pending = Pending::Organize; instance->confirm_text = Rml::CreateString("Organize %d selected demo(s) into physical mode folders?", instance->selected_count);
            instance->confirm_visible = true; instance->model.DirtyVariable("confirm_text"); instance->model.DirtyVariable("confirm_visible");
        }
        stop(e);
    }
    void RmlDemoManager::restore(Rml::Event &e)
    {
        if(instance)
        {
            std::string error; const std::vector<std::string> ids = instance->selected_ids(true);
            for(const std::string &id : ids) if(!instance->library.restore_entry(id, &error)) break;
            instance->selected.clear(); instance->set_status(error.empty() ? "Demo files restored" : error.c_str(), !error.empty()); instance->rebuild();
        }
        stop(e);
    }
    void RmlDemoManager::request_erase(Rml::Event &e)
    {
        if(instance && instance->has_selection)
        {
            instance->pending = Pending::Erase; instance->confirm_text = "Permanently erase selected Trash items? This cannot be undone.";
            instance->confirm_visible = true; instance->model.DirtyVariable("confirm_text"); instance->model.DirtyVariable("confirm_visible");
        }
        stop(e);
    }
    void RmlDemoManager::request_empty_trash(Rml::Event &e)
    {
        if(instance)
        {
            instance->pending = Pending::EmptyTrash; instance->confirm_text = "Permanently erase every item in Trash? This cannot be undone.";
            instance->confirm_visible = true; instance->model.DirtyVariable("confirm_text"); instance->model.DirtyVariable("confirm_visible");
        }
        stop(e);
    }
    void RmlDemoManager::trash_retention(Rml::Event &e)
    {
        if(instance)
        {
            instance->retention_value = event_value(e);
            const int days = instance->retention_value == "never" ? 0 : std::atoi(instance->retention_value.c_str());
            std::ofstream output(instance->library.root_path() + "/.cowbraten-demo-retention-v1.txt", std::ios::trunc);
            output << days << '\n';
            std::string error; if(output) instance->library.purge_expired(days, &error); else error = "Could not save Trash retention";
            instance->model.DirtyVariable("retention_value");
            instance->set_status(error.empty() ? "Trash retention updated" : error.c_str(), !error.empty());
            instance->rebuild();
        }
        stop(e);
    }
    void RmlDemoManager::apply_pending()
    {
        std::string error;
        if((pending == Pending::Trash || pending == Pending::Organize) && sauerdemo::recording()) error = "File operations are disabled while a demo is being recorded";
        else if(pending == Pending::Trash)
        {
            for(const std::string &id : selected_ids(false)) if(!library.trash_entry(id, &error)) break;
        }
        else if(pending == Pending::Organize)
        {
            const std::vector<std::string> ids = selected_ids(false);
            for(const std::string &id : ids) if(!library.organize_entry(id, &error)) break;
        }
        else if(pending == Pending::Erase)
        {
            for(const std::string &id : selected_ids(true)) if(!library.erase_entry(id, &error)) break;
        }
        else if(pending == Pending::EmptyTrash) library.empty_trash(&error);
        selected.clear(); confirm_visible = false; pending = Pending::None;
        model.DirtyVariable("confirm_visible"); set_status(error.empty() ? "File operation completed" : error.c_str(), !error.empty()); rebuild();
    }
    void RmlDemoManager::confirm(Rml::Event &e) { if(instance) instance->apply_pending(); stop(e); }
    void RmlDemoManager::cancel_confirm(Rml::Event &e) { if(instance) { instance->pending = Pending::None; instance->confirm_visible = false; instance->model.DirtyVariable("confirm_visible"); } stop(e); }

#define CLASSIFY_ACTION(name, value) void RmlDemoManager::name(Rml::Event &e) { if(instance) { std::string error; for(const std::string &id : instance->selected_ids(false)) if(!instance->library.set_classification(id, demos::Classification::value, &error)) break; instance->set_status(error.empty() ? "Manual classification saved" : error.c_str(), !error.empty()); instance->rebuild(); } stop(e); }
    CLASSIFY_ACTION(classify_duel, ConfirmedDuel)
    CLASSIFY_ACTION(classify_match, Normal)
    CLASSIFY_ACTION(classify_unknown, Unknown)
#undef CLASSIFY_ACTION
    void RmlDemoManager::open_location(Rml::Event &e)
    {
        if(instance && instance->single_selection)
        {
            const demos::Entry *entry = instance->library.find(*instance->selected.begin());
            if(!entry || !sauerdemo::open_location(entry->path.c_str())) instance->set_status("The platform could not open this location", true);
        }
        stop(e);
    }

    void RmlDemoManager::toggle_hd_playback(Rml::Event &e) { if(instance) { sauerdemo::set_hd_playback(event_checked(e)); instance->sync_settings(); } stop(e); }
    void RmlDemoManager::toggle_hd_recording(Rml::Event &e) { if(instance) { sauerdemo::set_hd_recording(event_checked(e)); instance->sync_settings(); } stop(e); }
    void RmlDemoManager::toggle_auto_recording(Rml::Event &e) { if(instance) { sauerdemo::set_automatic_recording(event_checked(e)); instance->sync_settings(); } stop(e); }
    void RmlDemoManager::playback_stop(Rml::Event &e) { sauerdemo::stop(); if(instance) instance->sync_settings(); stop(e); }
    void RmlDemoManager::playback_pause(Rml::Event &e) { sauerdemo::toggle_pause(); if(instance) instance->sync_settings(); stop(e); }
    void RmlDemoManager::playback_back(Rml::Event &e) { sauerdemo::seek_relative(-5000); stop(e); }
    void RmlDemoManager::playback_forward(Rml::Event &e) { sauerdemo::seek_relative(5000); stop(e); }
    void RmlDemoManager::playback_speed(Rml::Event &e) { sauerdemo::set_speed(std::atoi(event_value(e).c_str())); stop(e); }
}
