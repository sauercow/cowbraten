#ifndef COWBRATEN_UI_RML_DEMO_MANAGER_H
#define COWBRATEN_UI_RML_DEMO_MANAGER_H

#include "demo_library.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <set>
#include <vector>

namespace Rml { class Context; class Event; }

namespace cowui
{
    class RmlActionRouter;

    class RmlDemoManager final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);
        void activate();
        void update();
        void showlibrary();
        void showrecording();
        void showplayback();
        void showtrash();
        void showduels();
        void setsearch(const char *value);
        void selectfirst();
        bool playselected();

    private:
        struct Row
        {
            Rml::String id, name, format, map, mode, date, duration;
            Rml::String players, spectators, size, server, category, status;
            bool selected = false, favorite = false, playable = false;
        };

        enum class Tab { Library, Recording, Trash };
        enum class Pending { None, Trash, Erase, EmptyTrash, Organize };

        void rebuild();
        void update_selection();
        void sync_settings();
        void set_tab(Tab value);
        void set_status(const Rml::String &value, bool error = false);
        std::vector<std::string> selected_ids(bool trash) const;
        void apply_pending();

        void select_callback(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &);
        void toggle_callback(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &);

        static void tab_library(Rml::Event &); static void tab_recording(Rml::Event &);
        static void tab_trash(Rml::Event &);
        static void refresh(Rml::Event &); static void clear_filters(Rml::Event &);
        static void filter_search(Rml::Event &); static void filter_mode(Rml::Event &);
        static void filter_player(Rml::Event &); static void filter_map(Rml::Event &);
        static void filter_server(Rml::Event &); static void filter_category(Rml::Event &);
        static void filter_duration(Rml::Event &); static void filter_format(Rml::Event &);
        static void filter_date(Rml::Event &); static void filter_size(Rml::Event &);
        static void filter_duration_min(Rml::Event &); static void filter_duration_max(Rml::Event &);
        static void filter_favorites(Rml::Event &); static void filter_active_players(Rml::Event &);
        static void select_filtered(Rml::Event &); static void save_filter(Rml::Event &);
        static void load_filter(Rml::Event &);
        static void sort_name(Rml::Event &); static void sort_date(Rml::Event &);
        static void sort_duration(Rml::Event &); static void sort_mode(Rml::Event &);
        static void sort_map(Rml::Event &); static void sort_size(Rml::Event &);
        static void play(Rml::Event &); static void favorite(Rml::Event &);
        static void rename_input(Rml::Event &); static void move_input(Rml::Event &);
        static void rename_selected(Rml::Event &); static void move_selected(Rml::Event &);
        static void request_trash(Rml::Event &); static void request_organize(Rml::Event &);
        static void restore(Rml::Event &); static void request_erase(Rml::Event &);
        static void request_empty_trash(Rml::Event &); static void confirm(Rml::Event &);
        static void trash_retention(Rml::Event &);
        static void cancel_confirm(Rml::Event &); static void classify_duel(Rml::Event &);
        static void classify_match(Rml::Event &); static void classify_unknown(Rml::Event &);
        static void open_location(Rml::Event &);
        static void toggle_hd_playback(Rml::Event &); static void toggle_hd_recording(Rml::Event &);
        static void toggle_auto_recording(Rml::Event &);
        static void toggle_auto_recording_online_only(Rml::Event &);

        demos::Library library;
        demos::Filter filter;
        demos::SortKey sort_key = demos::SortKey::Date;
        std::set<std::string> selected;
        std::vector<Row> rows;
        Rml::DataModelHandle model;

        Rml::String status, selected_name, selected_details, selected_participants;
        Rml::String rename_value, move_value, confirm_text, result_count, trash_usage;
        Rml::String search_value, mode_value, player_value, map_value, server_value, date_value;
        Rml::String category_value = "all", duration_value = "all", format_value, size_value = "all";
        Rml::String duration_min_value, duration_max_value;
        Rml::String retention_value = "never";
        int total_count = 0, visible_count = 0, selected_count = 0;
        bool status_error = false, sort_ascending = false;
        bool tab_library_value = true, tab_recording_value = false;
        bool tab_trash_value = false;
        bool confirm_visible = false, can_play = false, has_selection = false;
        bool single_selection = false, hd_playback = true, hd_recording = true;
        bool automatic_recording = false, recording_active = false;
        bool automatic_recording_online_only = false;
        bool favorites_only = false, active_players_only = false;
        bool saved_filter_available = false;
        Tab tab = Tab::Library;
        Pending pending = Pending::None;
        static RmlDemoManager *instance;
    };
}

#endif
