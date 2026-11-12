#include "../src/cowbraten/ui/demo_library.h"

#include <zlib.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace cowui::demos;

namespace
{
    int passed = 0, failed = 0;

    void check(bool condition, const char *name)
    {
        if(condition) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }

    void putint(std::vector<unsigned char> &output, int value)
    {
        if(value < 128 && value > -127) output.push_back(static_cast<unsigned char>(value));
        else if(value < 0x8000 && value >= -0x8000)
        {
            output.push_back(0x80); output.push_back(value & 255); output.push_back((value >> 8) & 255);
        }
        else
        {
            output.push_back(0x81);
            for(int shift = 0; shift < 32; shift += 8) output.push_back((value >> shift) & 255);
        }
    }

    void putstring(std::vector<unsigned char> &output, const std::string &value)
    {
        for(unsigned char c : value) putint(output, c);
        putint(output, 0);
    }

    void little32(std::vector<unsigned char> &output, int value)
    {
        for(int shift = 0; shift < 32; shift += 8) output.push_back((value >> shift) & 255);
    }

    std::vector<unsigned char> welcome_packet(const std::string &map, int mode,
        const std::vector<std::pair<std::string, int>> &players)
    {
        std::vector<unsigned char> packet;
        putint(packet, 2); // welcome
        putint(packet, 22); putstring(packet, map); putint(packet, mode); putint(packet, 0);
        putint(packet, 33); putint(packet, 600);
        putint(packet, 36); putint(packet, -1);
        putint(packet, 37);
        for(std::size_t i = 0; i < players.size(); ++i)
        {
            putint(packet, int(i)); putint(packet, players[i].second);
            for(int field = 0; field < 17; ++field) putint(packet, 0);
        }
        putint(packet, -1);
        for(std::size_t i = 0; i < players.size(); ++i)
        {
            putint(packet, 3); putint(packet, int(i)); putstring(packet, players[i].first);
            putstring(packet, ""); putint(packet, 0);
        }
        putint(packet, 35);
        putstring(packet, "recorded on Test Server (127.0.0.1:28785), starting at 2026-11-12 00:00:00 UTC");
        return packet;
    }

    void fixture(const fs::path &path, const std::string &map, int mode, int duration,
        const std::vector<std::pair<std::string, int>> &players,
        const std::vector<std::vector<unsigned char>> &events = {})
    {
        fs::create_directories(path.parent_path());
        gzFile file = gzopen(path.string().c_str(), "wb9");
        if(!file) throw std::runtime_error("cannot create fixture");
        std::vector<unsigned char> bytes;
        const char magic[] = "SAUERBRATEN_DEMO";
        bytes.insert(bytes.end(), magic, magic + 16);
        little32(bytes, 1); little32(bytes, 260);
        const std::vector<unsigned char> welcome = welcome_packet(map, mode, players);
        little32(bytes, 0); little32(bytes, 1); little32(bytes, int(welcome.size()));
        bytes.insert(bytes.end(), welcome.begin(), welcome.end());
        int event_time = 1000;
        for(const std::vector<unsigned char> &event : events)
        {
            little32(bytes, event_time); little32(bytes, 1); little32(bytes, int(event.size()));
            bytes.insert(bytes.end(), event.begin(), event.end()); event_time += 1000;
        }
        little32(bytes, duration); little32(bytes, 0); little32(bytes, 1); bytes.push_back(0);
        if(gzwrite(file, bytes.data(), unsigned(bytes.size())) != int(bytes.size())) throw std::runtime_error("cannot write fixture");
        gzclose(file);
    }

    std::vector<unsigned char> bytes(const fs::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        return std::vector<unsigned char>(std::istreambuf_iterator<char>(file), {});
    }

    void finish_refresh(Library &library)
    {
        while(!library.refresh_step(2)) {}
    }

    std::vector<unsigned char> spectator_event(int cn, bool spectator)
    {
        std::vector<unsigned char> event;
        putint(event, 62); putint(event, cn); putint(event, spectator ? 1 : 0);
        return event;
    }
}

int main(int argc, char **argv)
{
    if(argc == 3 && std::string(argv[1]) == "--fixtures")
    {
        const fs::path fixture_root = argv[2];
        fs::create_directories(fixture_root);
        fixture(fixture_root / "2026-11-12_10.00.00_instagib_turbine.dmo", "turbine", 3, 9 * 60000,
            {{"COW", 0}, {"Starman", 0}, {"Spectator One", 5}, {"Spectator Two", 5}, {"Spectator Three", 5}});
        fixture(fixture_root / "2026-11-12_11.00.00_ffa_douze.dmo", "douze", 0, 22 * 60000,
            {{"Alpha", 0}, {"Bravo", 0}, {"Charlie", 0}, {"Observer", 5}});
        fixture(fixture_root / "archive/2026-11-12_12.00.00_efficiency_complex.dmo", "complex", 5, 90 * 1000,
            {{"COW", 0}, {"Rival", 0}, {"Camera", 5}});
        std::ofstream(fixture_root / "broken.dmo") << "controlled corrupt fixture";
        std::ofstream(fixture_root / "2026-11-12_10.00.00_instagib_turbine.dmohd", std::ios::binary) << "controlled HD fixture";
        std::cout << "Created controlled fixtures in " << fixture_root << '\n';
        return 0;
    }
    if(argc == 3 && (std::string(argv[1]) == "--organize" ||
                     std::string(argv[1]) == "--trash" ||
                     std::string(argv[1]) == "--restore"))
    {
        Library utility; std::string utility_error;
        if(!utility.open(argv[2], &utility_error)) throw std::runtime_error(utility_error);
        finish_refresh(utility);
        std::vector<std::string> ids;
        for(const Entry &entry : utility.entries())
        {
            if(std::string(argv[1]) == "--restore" ? entry.trashed : (!entry.trashed && entry.playable))
                ids.push_back(entry.id);
        }
        for(const std::string &id : ids)
        {
            bool ok = true;
            if(std::string(argv[1]) == "--organize") ok = utility.organize_entry(id, &utility_error);
            else if(std::string(argv[1]) == "--trash") ok = utility.trash_entry(id, &utility_error);
            else ok = utility.restore_entry(id, &utility_error);
            if(!ok) throw std::runtime_error(utility_error);
        }
        std::cout << "Applied " << argv[1] << " to " << ids.size() << " controlled fixture(s)\n";
        return 0;
    }
    const fs::path root = fs::temp_directory_path() / ("cowbraten-demo-tests-" + std::to_string(std::uint64_t(std::time(nullptr))));
    fs::remove_all(root); fs::create_directories(root);
    try
    {
        const fs::path duel = root / "2026-11-12_10.00.00_instagib_turbine.dmo";
        const fs::path match = root / "legacy" / "2026-11-12_11.00.00_ffa_douze.dmo";
        const fs::path shortdemo = root / "short.dmo";
        const fs::path rotation = root / "rotation.dmo";
        const fs::path temporary_spectator = root / "temporary-spectator.dmo";
        fixture(duel, "turbine", 3, 9 * 60000, {{"COW", 0}, {"Starman", 0}, {"Watcher A", 5}, {"Watcher B", 5}});
        fixture(match, "douze", 0, 22 * 60000, {{"Alpha", 0}, {"Bravo", 0}, {"Charlie", 0}, {"Observer", 5}});
        fixture(shortdemo, "complex", 5, 90 * 1000, {{"Solo", 0}, {"Other", 0}});
        fixture(rotation, "ot", 3, 4 * 60000, {{"One", 0}, {"Two", 0}, {"Three", 5}},
            {spectator_event(1, true), spectator_event(2, false)});
        fixture(temporary_spectator, "academy", 3, 3 * 60000, {{"Left", 0}, {"Right", 0}, {"Observer", 5}},
            {spectator_event(1, true), spectator_event(1, false)});
        std::ofstream(root / "broken.dmo") << "not a gzip demo";
        std::ofstream(root / "2026-11-12_10.00.00_instagib_turbine.dmohd", std::ios::binary) << "HD";
        const std::vector<unsigned char> original = bytes(duel);

        Library library; std::string error;
        check(library.open(root.string(), &error), "open temporary library");
        finish_refresh(library);
        check(library.entries().size() == 6, "discover dmo files and collapse paired HD sidecar");

        const Entry *duel_entry = nullptr, *match_entry = nullptr, *broken_entry = nullptr;
        for(const Entry &entry : library.entries())
        {
            if(entry.name.find("turbine") != std::string::npos) duel_entry = &entry;
            if(entry.name.find("douze") != std::string::npos) match_entry = &entry;
            if(entry.name == "broken.dmo") broken_entry = &entry;
        }
        check(duel_entry && duel_entry->map == "turbine" && duel_entry->mode == "instagib", "index map and mode from real packet");
        check(duel_entry && duel_entry->duration_ms == 9 * 60000, "duration uses demo timestamps");
        check(duel_entry && duel_entry->format == "DMO + HD", "detect compatible HD sidecar");
        check(duel_entry && duel_entry->server.find("Test Server") != std::string::npos, "index server origin");
        check(duel_entry && duel_entry->participants.size() == 4, "index participant names and roles");
        check(duel_entry && duel_entry->classification == Classification::ConfirmedDuel, "duel detection ignores extra spectators");
        check(match_entry && match_entry->classification == Classification::Normal, "three active players are not a duel");
        const Entry *rotation_entry = nullptr, *temporary_entry = nullptr;
        for(const Entry &entry : library.entries())
        {
            if(entry.name == "rotation.dmo") rotation_entry = &entry;
            if(entry.name == "temporary-spectator.dmo") temporary_entry = &entry;
        }
        check(rotation_entry && rotation_entry->classification == Classification::Normal, "player rotation is not misclassified as a duel");
        check(temporary_entry && temporary_entry->classification == Classification::ConfirmedDuel, "temporary spectator transition preserves duel classification");
        check(broken_entry && !broken_entry->playable && broken_entry->status.find("Corrupt") == 0, "corrupt demo remains visible with error state");

        Filter filter; filter.mode = "instagib";
        check(library.query(filter, SortKey::Name, true).size() == 3, "filter by mode");
        filter = Filter(); filter.player = "starman";
        check(library.query(filter, SortKey::Name, true).size() == 1, "filter by participant metadata");
        filter.active_players_only = true; filter.player = "watcher";
        check(library.query(filter, SortKey::Name, true).empty(), "distinguish active participants from spectators");
        filter = Filter(); filter.map = "BINE";
        check(library.query(filter, SortKey::Name, true).size() == 1, "partial case-insensitive map filter");
        filter = Filter(); filter.duration_max_ms = 10 * 60000;
        check(library.query(filter, SortKey::Name, true).size() == 4, "under ten minute duration filter");
        filter.mode = "instagib"; filter.player = "Starman"; filter.map = "turb"; filter.category = 1;
        check(library.query(filter, SortKey::Name, true).size() == 1, "combined filters use logical AND");
        filter = Filter(); filter.server = "127.0.0.1";
        check(library.query(filter, SortKey::Name, true).size() == 5, "filter by known server");
        filter = Filter(); filter.format = "HD";
        check(library.query(filter, SortKey::Name, true).size() == 1, "filter by format");
        filter = Filter(); filter.date = "2026";
        check(!library.query(filter, SortKey::Name, true).empty(), "filter by indexed date");
        filter = Filter(); filter.size_max = 1024 * 1024;
        check(library.query(filter, SortKey::Name, true).size() == 6, "filter by file size");

        auto sorted = library.query(Filter(), SortKey::Duration, true);
        check(sorted.size() == 6 && library.entries()[sorted.back()].duration_ms == 22 * 60000, "sort by duration");
        const std::string duel_id = duel_entry->id;
        check(library.set_favorite(duel_id, true, &error), "persist favorite mutation");
        filter = Filter(); filter.favorites_only = true;
        check(library.query(filter, SortKey::Name, true).size() == 1, "favorites filter");
        check(library.set_classification(duel_id, Classification::Normal, &error), "manual classification override");
        check(library.find(duel_id)->classification_source == ClassificationSource::Manual, "manual classification source persists");
        check(bytes(duel) == original, "index and metadata mutations preserve original bytes");

        Library reopened; check(reopened.open(root.string(), &error), "reload persistent index"); finish_refresh(reopened);
        const Entry *persisted = reopened.find(duel_id);
        check(persisted && persisted->favorite && persisted->classification == Classification::Normal, "favorite and manual class survive restart");

        check(!reopened.rename_entry(duel_id, "short.dmo", &error), "rename collision is rejected");
        check(reopened.rename_entry(duel_id, "renamed_duel.dmo", &error), "rename demo and paired sidecar");
        const fs::path renamed = root / "renamed_duel.dmo";
        check(fs::exists(renamed) && fs::exists(root / "renamed_duel.dmohd"), "paired HD sidecar follows rename");
        const std::string renamed_id = renamed.string();
        check(reopened.move_entry(renamed_id, "custom/folder", &error), "move into physical subfolder");
        const fs::path moved = root / "custom/folder/renamed_duel.dmo";
        check(fs::exists(moved), "physical folder organization exists");
        check(reopened.trash_entry(rotation.string(), &error), "move retention fixture to temporary trash");
        Entry *expired = nullptr;
        for(const Entry &entry : reopened.entries())
            if(entry.trashed && entry.original_path == rotation.string()) expired = reopened.find(entry.id);
        if(expired) expired->deleted_date = "2000-01-01 00:00";
        check(expired && reopened.save(&error) && reopened.purge_expired(30, &error), "trash retention purges expired fixture");
        check(!fs::exists(rotation), "retention deletion is confined to temporary trash");
        const std::string moved_id = moved.string();
        check(reopened.trash_entry(moved_id, &error), "move demo to recoverable trash");
        const Entry *trashed = nullptr;
        for(const Entry &entry : reopened.entries()) if(entry.trashed) trashed = &entry;
        check(trashed && fs::exists(trashed->path) && reopened.trash_size() > 0, "trash records file and occupied size");
        const std::string trash_id = trashed ? trashed->id : "";
        check(reopened.restore_entry(trash_id, &error) && fs::exists(moved), "restore to original path without overwrite");
        check(reopened.trash_entry(moved.string(), &error), "trash restored file again");
        trashed = nullptr; for(const Entry &entry : reopened.entries()) if(entry.trashed) trashed = &entry;
        check(trashed && reopened.erase_entry(trashed->id, &error), "permanent deletion is restricted to trash fixture");
        check(!fs::exists(moved), "permanent delete removes temporary demo");

        const Entry *normal = nullptr;
        for(const Entry &entry : reopened.entries()) if(entry.name.find("douze") != std::string::npos) normal = &entry;
        const std::string normal_id = normal ? normal->id : "";
        check(normal && reopened.organize_entry(normal_id, &error), "organize classified normal match");
        check(fs::exists(root / "modes/ffa" / match.filename()), "normal match organized under modes/mode");

        fs::remove(shortdemo); reopened.begin_refresh(); finish_refresh(reopened);
        bool short_found = false; for(const Entry &entry : reopened.entries()) if(entry.name == "short.dmo") short_found = true;
        check(!short_found, "reindex removes externally deleted files");
    }
    catch(const std::exception &exception)
    {
        ++failed; std::cout << "FAIL  unexpected exception: " << exception.what() << '\n';
    }
    fs::remove_all(root);
    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
