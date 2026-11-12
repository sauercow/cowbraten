#ifndef COWBRATEN_UI_DEMO_LIBRARY_H
#define COWBRATEN_UI_DEMO_LIBRARY_H

#include <cstdint>
#include <string>
#include <vector>

namespace cowui
{
    namespace demos
    {
        enum class Role { Unknown, Active, Spectator };
        enum class Classification { Unknown, Normal, ProbableDuel, ConfirmedDuel };
        enum class ClassificationSource { Automatic, Manual };

        struct Participant
        {
            std::string name;
            Role role = Role::Unknown;
        };

        struct Entry
        {
            std::string id;
            std::string path;
            std::string original_path;
            std::string name;
            std::string format;
            std::string map;
            std::string mode;
            std::string server;
            std::string date;
            std::string deleted_date;
            std::string status;
            std::vector<Participant> participants;
            std::int64_t size = 0;
            std::int64_t modified = 0;
            int duration_ms = -1;
            int active_participants = 0;
            int peak_active = 0;
            Classification classification = Classification::Unknown;
            ClassificationSource classification_source = ClassificationSource::Automatic;
            bool favorite = false;
            bool trashed = false;
            bool playable = false;
        };

        struct Filter
        {
            std::string search;
            std::string mode;
            std::string player;
            std::string map;
            std::string server;
            std::string date;
            std::string format;
            int category = 0; // 0 all, 1 duel, 2 normal, 3 unknown
            int duration_min_ms = -1;
            int duration_max_ms = -1;
            std::int64_t size_min = -1;
            std::int64_t size_max = -1;
            bool favorites_only = false;
            bool active_players_only = false;
            bool trash = false;
        };

        enum class SortKey { Name, Date, Duration, Mode, Map, Size };

        class Library
        {
        public:
            bool open(const std::string &root, std::string *error = nullptr);
            void close();

            void begin_refresh();
            bool refresh_step(std::size_t budget = 1);
            bool refreshing() const { return refresh_active; }
            std::size_t refresh_done() const { return scan_cursor; }
            std::size_t refresh_total() const { return scan_paths.size(); }

            const std::vector<Entry> &entries() const { return records; }
            std::vector<std::size_t> query(const Filter &filter, SortKey key, bool ascending) const;
            Entry *find(const std::string &id);
            const Entry *find(const std::string &id) const;

            bool set_favorite(const std::string &id, bool value, std::string *error = nullptr);
            bool set_classification(const std::string &id, Classification value, std::string *error = nullptr);
            bool rename_entry(const std::string &id, const std::string &new_name, std::string *error = nullptr);
            bool move_entry(const std::string &id, const std::string &relative_folder, std::string *error = nullptr);
            bool organize_entry(const std::string &id, std::string *error = nullptr);
            bool trash_entry(const std::string &id, std::string *error = nullptr);
            bool restore_entry(const std::string &id, std::string *error = nullptr);
            bool erase_entry(const std::string &id, std::string *error = nullptr);
            bool empty_trash(std::string *error = nullptr);
            bool purge_expired(int retention_days, std::string *error = nullptr);
            std::int64_t trash_size() const;

            bool save(std::string *error = nullptr) const;
            const std::string &root_path() const { return root; }
            std::string relative_path(const Entry &entry) const;

        private:
            bool load(std::string *error);
            bool index_path(const std::string &path, Entry &entry, std::string *error) const;
            bool relocate(Entry &entry, const std::string &destination, std::string *error);
            bool valid_relative_folder(const std::string &folder) const;
            void remove_missing();

            std::string root;
            std::string index_pathname;
            std::vector<Entry> records;
            std::vector<std::string> scan_paths;
            std::size_t scan_cursor = 0;
            bool refresh_active = false;
        };

        const char *role_name(Role role);
        const char *classification_name(Classification classification);
    }
}

#endif
