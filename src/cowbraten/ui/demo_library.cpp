#include "demo_library.h"

#include <zlib.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

namespace cowui
{
    namespace demos
    {
        namespace
        {
            constexpr std::size_t max_demo_bytes = 512u << 20;
            constexpr int cs_spectator = 5;
            constexpr int n_welcome = 2, n_initclient = 3, n_cdis = 7;
            constexpr int n_mapchange = 22, n_teaminfo = 24, n_timeup = 33;
            constexpr int n_servmsg = 35, n_itemlist = 36, n_resume = 37;
            constexpr int n_currentmaster = 61, n_spectator = 62;
            constexpr int n_pausegame = 94, n_gamespeed = 95, n_initai = 98;
            constexpr const char *demo_magic = "SAUERBRATEN_DEMO";

            const char *mode_names[] = {
                "SP", "DMSP", "demo", "ffa", "coop edit", "teamplay",
                "instagib", "insta team", "efficiency", "effic team",
                "tactics", "tac team", "capture", "regen capture", "ctf",
                "insta ctf", "protect", "insta protect", "hold", "insta hold",
                "effic ctf", "effic protect", "effic hold", "collect",
                "insta collect", "effic collect"
            };

            std::string lowercase(std::string value)
            {
                std::transform(value.begin(), value.end(), value.begin(),
                    [](unsigned char c) { return char(std::tolower(c)); });
                return value;
            }

            bool contains(const std::string &value, const std::string &needle)
            {
                return needle.empty() || lowercase(value).find(lowercase(needle)) != std::string::npos;
            }

            bool is_demo_extension(const fs::path &path)
            {
                const std::string extension = lowercase(path.extension().string());
                return extension == ".dmo" || extension == ".dmohd";
            }

            std::int64_t unix_time(const fs::file_time_type &time)
            {
                using namespace std::chrono;
                const auto system_time = time_point_cast<system_clock::duration>(
                    time - fs::file_time_type::clock::now() + system_clock::now());
                return duration_cast<seconds>(system_time.time_since_epoch()).count();
            }

            std::string date_string(std::int64_t seconds)
            {
                std::time_t value = std::time_t(seconds);
                std::tm result{};
#ifdef _WIN32
                localtime_s(&result, &value);
#else
                localtime_r(&value, &result);
#endif
                char buffer[32] = {};
                std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &result);
                return buffer;
            }

            std::string percent_encode(const std::string &value)
            {
                static const char digits[] = "0123456789ABCDEF";
                std::string result;
                for(unsigned char c : value)
                {
                    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                       (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == ' ')
                        result.push_back(char(c));
                    else
                    {
                        result.push_back('%');
                        result.push_back(digits[c >> 4]);
                        result.push_back(digits[c & 15]);
                    }
                }
                return result;
            }

            int hex_value(char c)
            {
                if(c >= '0' && c <= '9') return c - '0';
                if(c >= 'a' && c <= 'f') return c - 'a' + 10;
                if(c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            }

            std::string percent_decode(const std::string &value)
            {
                std::string result;
                for(std::size_t i = 0; i < value.size(); ++i)
                {
                    if(value[i] == '%' && i + 2 < value.size())
                    {
                        const int high = hex_value(value[i + 1]), low = hex_value(value[i + 2]);
                        if(high >= 0 && low >= 0)
                        {
                            result.push_back(char((high << 4) | low));
                            i += 2;
                            continue;
                        }
                    }
                    result.push_back(value[i]);
                }
                return result;
            }

            std::vector<std::string> split(const std::string &line, char delimiter)
            {
                std::vector<std::string> result;
                std::size_t start = 0;
                for(;;)
                {
                    const std::size_t end = line.find(delimiter, start);
                    result.push_back(line.substr(start, end == std::string::npos ? end : end - start));
                    if(end == std::string::npos) break;
                    start = end + 1;
                }
                return result;
            }

            std::string participants_encode(const std::vector<Participant> &participants)
            {
                std::string result;
                for(const Participant &participant : participants)
                {
                    if(!result.empty()) result.push_back(';');
                    result += std::to_string(int(participant.role));
                    result.push_back(':');
                    result += percent_encode(participant.name);
                }
                return result;
            }

            std::vector<Participant> participants_decode(const std::string &value)
            {
                std::vector<Participant> result;
                for(const std::string &item : split(value, ';'))
                {
                    const std::size_t separator = item.find(':');
                    if(separator == std::string::npos) continue;
                    Participant participant;
                    participant.role = Role(std::clamp(std::atoi(item.substr(0, separator).c_str()), 0, 2));
                    participant.name = percent_decode(item.substr(separator + 1));
                    if(!participant.name.empty()) result.push_back(participant);
                }
                return result;
            }

            struct Reader
            {
                const unsigned char *data = nullptr;
                std::size_t size = 0, cursor = 0;
                bool failed = false;

                int integer()
                {
                    if(cursor >= size) { failed = true; return 0; }
                    const signed char first = static_cast<signed char>(data[cursor++]);
                    if(first == -128)
                    {
                        if(cursor + 2 > size) { failed = true; return 0; }
                        const int value = data[cursor] | (data[cursor + 1] << 8);
                        cursor += 2;
                        return std::int16_t(value);
                    }
                    if(first == -127)
                    {
                        if(cursor + 4 > size) { failed = true; return 0; }
                        const std::uint32_t value = std::uint32_t(data[cursor]) |
                            (std::uint32_t(data[cursor + 1]) << 8) |
                            (std::uint32_t(data[cursor + 2]) << 16) |
                            (std::uint32_t(data[cursor + 3]) << 24);
                        cursor += 4;
                        return int(std::int32_t(value));
                    }
                    return first;
                }

                std::string string()
                {
                    std::string result;
                    while(!failed && result.size() < 4096)
                    {
                        const int value = integer();
                        if(!value) break;
                        if(value < 0 || value > 255) { failed = true; break; }
                        result.push_back(char(value));
                    }
                    return result;
                }

                void integers(int count) { while(count-- > 0 && !failed) integer(); }
            };

            void update_participant(std::map<int, Participant> &participants, int cn,
                const std::string *name, const Role *role)
            {
                Participant &participant = participants[cn];
                if(name && !name->empty()) participant.name = *name;
                if(role) participant.role = *role;
            }

            bool parse_welcome(const unsigned char *data, std::size_t size, Entry &entry,
                std::map<int, Participant> &participants, std::set<int> &active,
                std::set<int> &ever_active, int &peak_active)
            {
                Reader reader{data, size};
                bool saw_map = false, saw_resume = false;
                while(!reader.failed && reader.cursor < reader.size)
                {
                    const int type = reader.integer();
                    switch(type)
                    {
                        case n_welcome: break;
                        case n_mapchange:
                        {
                            entry.map = reader.string();
                            const int mode = reader.integer();
                            reader.integer();
                            if(mode >= -3 && mode < -3 + int(std::size(mode_names)))
                                entry.mode = mode_names[mode + 3];
                            saw_map = true;
                            break;
                        }
                        case n_timeup: reader.integer(); break;
                        case n_itemlist:
                            while(!reader.failed && reader.integer() >= 0) reader.integer();
                            break;
                        case n_currentmaster:
                            reader.integer();
                            while(!reader.failed && reader.integer() >= 0) reader.integer();
                            break;
                        case n_pausegame:
                        case n_gamespeed: reader.integers(2); break;
                        case n_teaminfo:
                            for(;;)
                            {
                                const std::string team = reader.string();
                                if(team.empty() || reader.failed) break;
                                reader.integer();
                            }
                            break;
                        case n_resume:
                            for(;;)
                            {
                                const int cn = reader.integer();
                                if(cn < 0 || reader.failed) break;
                                const int state = reader.integer();
                                const Role role = state == cs_spectator ? Role::Spectator : Role::Active;
                                update_participant(participants, cn, nullptr, &role);
                                if(role == Role::Active) { active.insert(cn); ever_active.insert(cn); }
                                else active.erase(cn);
                                peak_active = std::max(peak_active, int(active.size()));
                                reader.integers(17);
                            }
                            saw_resume = true;
                            break;
                        case n_initclient:
                        {
                            const int cn = reader.integer();
                            const std::string name = reader.string();
                            reader.string();
                            reader.integer();
                            update_participant(participants, cn, &name, nullptr);
                            break;
                        }
                        case n_initai:
                        {
                            const int cn = reader.integer();
                            reader.integers(5);
                            const std::string name = reader.string();
                            reader.string();
                            update_participant(participants, cn, &name, nullptr);
                            break;
                        }
                        case n_servmsg:
                        {
                            const std::string message = reader.string();
                            const std::string prefix = "recorded on ";
                            const std::size_t begin = message.find(prefix), end = message.find(", starting at ");
                            if(begin != std::string::npos)
                                entry.server = message.substr(begin + prefix.size(),
                                    end == std::string::npos ? end : end - begin - prefix.size());
                            break;
                        }
                        default:
                            // Mode-specific initializers follow the stable welcome snapshot.
                            reader.cursor = reader.size;
                            break;
                    }
                }
                if(saw_resume)
                {
                    entry.participants.clear();
                    for(const auto &item : participants)
                    {
                        Participant participant = item.second;
                        if(participant.name.empty()) participant.name = "Client " + std::to_string(item.first);
                        entry.participants.push_back(std::move(participant));
                    }
                }
                return saw_map;
            }

            void parse_participation_event(const unsigned char *data, std::size_t size,
                std::map<int, Participant> &participants, std::set<int> &active,
                std::set<int> &ever_active, int &peak_active)
            {
                Reader reader{data, size};
                const int type = reader.integer();
                if(type == n_spectator)
                {
                    const int cn = reader.integer(), spectator = reader.integer();
                    if(reader.failed || reader.cursor != reader.size) return;
                    const Role role = spectator ? Role::Spectator : Role::Active;
                    update_participant(participants, cn, nullptr, &role);
                    if(role == Role::Active) { active.insert(cn); ever_active.insert(cn); }
                    else active.erase(cn);
                    peak_active = std::max(peak_active, int(active.size()));
                }
                else if(type == n_cdis)
                {
                    const int cn = reader.integer();
                    if(!reader.failed && reader.cursor == reader.size) active.erase(cn);
                }
                else if(type == n_initclient)
                {
                    const int cn = reader.integer(); const std::string name = reader.string();
                    reader.string(); reader.integer();
                    if(!reader.failed && reader.cursor == reader.size) update_participant(participants, cn, &name, nullptr);
                }
            }

            void classify(Entry &entry)
            {
                if(entry.classification_source == ClassificationSource::Manual) return;
                int active = 0, unknown = 0;
                for(const Participant &participant : entry.participants)
                {
                    if(participant.role == Role::Active) ++active;
                    else if(participant.role == Role::Unknown) ++unknown;
                }
                const int active_people = entry.active_participants ? entry.active_participants : active;
                if(active_people > 2) entry.classification = Classification::Normal;
                else if(!entry.participants.empty() && unknown == 0 && active_people == 2 && entry.peak_active >= 2)
                    entry.classification = Classification::ConfirmedDuel;
                else if(active_people == 2 && entry.peak_active >= 2)
                    entry.classification = Classification::ProbableDuel;
                else if(!entry.participants.empty() && unknown == 0 && active_people != 2)
                    entry.classification = Classification::Normal;
                else
                    entry.classification = Classification::Unknown;
            }

            void infer_filename(Entry &entry)
            {
                std::string stem = fs::path(entry.name).stem().string();
                std::string lowered = lowercase(stem);
                std::size_t best = 0;
                for(const char *mode : mode_names)
                {
                    std::string token = lowercase(mode);
                    std::replace(token.begin(), token.end(), ' ', '_');
                    const std::string marker = "_" + token + "_";
                    const std::size_t position = lowered.find(marker);
                    if(position != std::string::npos && marker.size() > best)
                    {
                        best = marker.size();
                        entry.mode = mode;
                        entry.map = stem.substr(position + marker.size());
                    }
                }
            }

            bool read_demo(const std::string &path, Entry &entry, std::string *error)
            {
                gzFile file = gzopen(path.c_str(), "rb");
                if(!file)
                {
                    if(error) *error = "Unable to open compressed demo";
                    return false;
                }
                std::array<unsigned char, 24> header{};
                if(gzread(file, header.data(), unsigned(header.size())) != int(header.size()) ||
                   std::memcmp(header.data(), demo_magic, 16) != 0)
                {
                    gzclose(file);
                    if(error) *error = "Invalid or truncated demo header";
                    return false;
                }
                auto little32 = [](const unsigned char *value) {
                    return int(std::uint32_t(value[0]) | (std::uint32_t(value[1]) << 8) |
                        (std::uint32_t(value[2]) << 16) | (std::uint32_t(value[3]) << 24));
                };
                if(little32(header.data() + 16) != 1)
                {
                    gzclose(file);
                    if(error) *error = "Unsupported demo version";
                    return false;
                }
                int duration = 0;
                bool first_reliable = true;
                std::map<int, Participant> participants;
                std::set<int> active, ever_active;
                int peak_active = 0;
                std::size_t expanded = header.size();
                for(;;)
                {
                    std::array<unsigned char, 12> stamp{};
                    const int count = gzread(file, stamp.data(), unsigned(stamp.size()));
                    if(count == 0) break;
                    if(count != int(stamp.size()))
                    {
                        gzclose(file);
                        if(error) *error = "Truncated demo record";
                        return false;
                    }
                    const int millis = little32(stamp.data());
                    const int channel = little32(stamp.data() + 4);
                    const int length = little32(stamp.data() + 8);
                    if(millis < 0 || length < 0 || std::size_t(length) > max_demo_bytes ||
                       expanded + std::size_t(length) > max_demo_bytes)
                    {
                        gzclose(file);
                        if(error) *error = "Invalid demo record length";
                        return false;
                    }
                    std::vector<unsigned char> payload(static_cast<std::size_t>(length));
                    if(length && gzread(file, payload.data(), unsigned(length)) != length)
                    {
                        gzclose(file);
                        if(error) *error = "Truncated demo packet";
                        return false;
                    }
                    expanded += std::size_t(length) + stamp.size();
                    duration = std::max(duration, millis);
                    if(first_reliable && channel == 1)
                    {
                        parse_welcome(payload.data(), payload.size(), entry, participants, active, ever_active, peak_active);
                        first_reliable = false;
                    }
                    else if(channel == 1) parse_participation_event(payload.data(), payload.size(), participants, active, ever_active, peak_active);
                }
                gzclose(file);
                entry.duration_ms = duration;
                if(!participants.empty())
                {
                    entry.participants.clear();
                    for(const auto &item : participants)
                    {
                        Participant participant = item.second;
                        if(participant.name.empty()) participant.name = "Client " + std::to_string(item.first);
                        entry.participants.push_back(std::move(participant));
                    }
                }
                entry.active_participants = int(ever_active.size());
                entry.peak_active = peak_active;
                return true;
            }

            std::string safe_component(std::string value)
            {
                for(char &c : value)
                    if(!(std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.')) c = '_';
                while(!value.empty() && (value.front() == '.' || value.front() == ' ')) value.erase(value.begin());
                return value;
            }

            std::string mode_folder(const std::string &mode)
            {
                std::string value = lowercase(mode);
                for(char &c : value) if(!std::isalnum(static_cast<unsigned char>(c))) c = '_';
                return value.empty() ? "unknown" : value;
            }
        }

        const char *role_name(Role role)
        {
            switch(role)
            {
                case Role::Active: return "Player";
                case Role::Spectator: return "Spectator";
                default: return "Unknown";
            }
        }

        const char *classification_name(Classification classification)
        {
            switch(classification)
            {
                case Classification::Normal: return "Match";
                case Classification::ProbableDuel: return "Probable duel";
                case Classification::ConfirmedDuel: return "Duel";
                default: return "Unknown";
            }
        }

        bool Library::open(const std::string &requested_root, std::string *error)
        {
            close();
            std::error_code ec;
            fs::path absolute = fs::absolute(fs::path(requested_root), ec).lexically_normal();
            if(ec || absolute.empty())
            {
                if(error) *error = "Invalid demo directory";
                return false;
            }
            fs::create_directories(absolute, ec);
            if(ec)
            {
                if(error) *error = "Cannot create demo directory: " + ec.message();
                return false;
            }
            root = absolute.string();
            index_pathname = (absolute / ".cowbraten-demo-index-v1.tsv").string();
            if(!load(error)) return false;
            begin_refresh();
            return true;
        }

        void Library::close()
        {
            root.clear();
            index_pathname.clear();
            records.clear();
            scan_paths.clear();
            scan_cursor = 0;
            refresh_active = false;
        }

        bool Library::load(std::string *error)
        {
            records.clear();
            std::ifstream file(index_pathname);
            if(!file) return true;
            std::string line;
            if(!std::getline(file, line) || line != "COWBRATEN_DEMO_INDEX\t1")
            {
                if(error) *error = "Ignoring incompatible demo index";
                return true;
            }
            while(std::getline(file, line))
            {
                const std::vector<std::string> f = split(line, '\t');
                if(f.size() != 21) continue;
                Entry entry;
                entry.id = percent_decode(f[0]); entry.path = percent_decode(f[1]);
                entry.original_path = percent_decode(f[2]); entry.name = percent_decode(f[3]);
                entry.format = percent_decode(f[4]); entry.map = percent_decode(f[5]);
                entry.mode = percent_decode(f[6]); entry.server = percent_decode(f[7]);
                entry.date = percent_decode(f[8]); entry.deleted_date = percent_decode(f[9]);
                entry.status = percent_decode(f[10]); entry.size = std::atoll(f[11].c_str());
                entry.modified = std::atoll(f[12].c_str()); entry.duration_ms = std::atoi(f[13].c_str());
                entry.classification = Classification(std::clamp(std::atoi(f[14].c_str()), 0, 3));
                entry.classification_source = ClassificationSource(std::clamp(std::atoi(f[15].c_str()), 0, 1));
                entry.favorite = std::atoi(f[16].c_str()) != 0; entry.trashed = std::atoi(f[17].c_str()) != 0;
                entry.playable = std::atoi(f[18].c_str()) != 0;
                entry.participants = participants_decode(f[19]);
                const std::vector<std::string> participation = split(f[20], ',');
                entry.active_participants = participation.empty() ? 0 : std::atoi(participation[0].c_str());
                entry.peak_active = participation.size() < 2 ? 0 : std::atoi(participation[1].c_str());
                records.push_back(std::move(entry));
            }
            return true;
        }

        bool Library::save(std::string *error) const
        {
            if(index_pathname.empty()) return false;
            const fs::path temporary = fs::path(index_pathname).concat(".tmp");
            std::ofstream file(temporary, std::ios::trunc);
            if(!file)
            {
                if(error) *error = "Cannot write demo index";
                return false;
            }
            file << "COWBRATEN_DEMO_INDEX\t1\n";
            for(const Entry &entry : records)
            {
                file << percent_encode(entry.id) << '\t' << percent_encode(entry.path) << '\t'
                    << percent_encode(entry.original_path) << '\t' << percent_encode(entry.name) << '\t'
                    << percent_encode(entry.format) << '\t' << percent_encode(entry.map) << '\t'
                    << percent_encode(entry.mode) << '\t' << percent_encode(entry.server) << '\t'
                    << percent_encode(entry.date) << '\t' << percent_encode(entry.deleted_date) << '\t'
                    << percent_encode(entry.status) << '\t' << entry.size << '\t' << entry.modified << '\t'
                    << entry.duration_ms << '\t' << int(entry.classification) << '\t'
                    << int(entry.classification_source) << '\t' << int(entry.favorite) << '\t'
                    << int(entry.trashed) << '\t' << int(entry.playable) << '\t'
                    << participants_encode(entry.participants) << '\t' << entry.active_participants << ',' << entry.peak_active << '\n';
            }
            file.close();
            if(!file)
            {
                std::error_code ignored; fs::remove(temporary, ignored);
                if(error) *error = "Failed while writing demo index";
                return false;
            }
            std::error_code ec;
            fs::rename(temporary, index_pathname, ec);
            if(ec)
            {
                fs::remove(index_pathname, ec); ec.clear();
                fs::rename(temporary, index_pathname, ec);
            }
            if(ec)
            {
                if(error) *error = "Cannot replace demo index: " + ec.message();
                return false;
            }
            return true;
        }

        void Library::begin_refresh()
        {
            scan_paths.clear();
            scan_cursor = 0;
            if(root.empty()) { refresh_active = false; return; }
            std::error_code ec;
            for(fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
                it != end; it.increment(ec))
            {
                if(ec) { ec.clear(); continue; }
                if(it->is_directory(ec) && it->path().filename() == ".trash")
                {
                    it.disable_recursion_pending();
                    continue;
                }
                if(it->is_regular_file(ec) && is_demo_extension(it->path()))
                {
                    const fs::path p = it->path().lexically_normal();
                    if(lowercase(p.extension().string()) == ".dmohd")
                    {
                        fs::path paired = p; paired.replace_extension(".dmo");
                        if(fs::exists(paired, ec)) continue;
                    }
                    scan_paths.push_back(p.string());
                }
            }
            std::sort(scan_paths.begin(), scan_paths.end());
            refresh_active = true;
        }

        bool Library::refresh_step(std::size_t budget)
        {
            if(!refresh_active) return true;
            while(budget-- && scan_cursor < scan_paths.size())
            {
                const std::string path = scan_paths[scan_cursor++];
                std::error_code ec;
                const std::int64_t size = std::int64_t(fs::file_size(path, ec));
                const std::int64_t modified = ec ? 0 : unix_time(fs::last_write_time(path, ec));
                Entry *existing = nullptr;
                for(Entry &entry : records) if(!entry.trashed && entry.path == path) { existing = &entry; break; }
                if(existing && existing->size == size && existing->modified == modified) continue;
                Entry indexed;
                std::string parse_error;
                index_path(path, indexed, &parse_error);
                if(existing)
                {
                    indexed.favorite = existing->favorite;
                    if(existing->classification_source == ClassificationSource::Manual)
                    {
                        indexed.classification = existing->classification;
                        indexed.classification_source = existing->classification_source;
                    }
                    indexed.id = existing->id;
                    *existing = std::move(indexed);
                }
                else records.push_back(std::move(indexed));
            }
            if(scan_cursor >= scan_paths.size())
            {
                remove_missing();
                refresh_active = false;
                save();
                return true;
            }
            return false;
        }

        bool Library::index_path(const std::string &path, Entry &entry, std::string *error) const
        {
            std::error_code ec;
            const fs::path file(path);
            entry.id = path;
            entry.path = path;
            entry.name = file.filename().string();
            entry.size = std::int64_t(fs::file_size(file, ec));
            if(ec) entry.size = 0;
            ec.clear();
            entry.modified = unix_time(fs::last_write_time(file, ec));
            if(ec) entry.modified = 0;
            entry.date = date_string(entry.modified);
            const std::string extension = lowercase(file.extension().string());
            entry.format = extension == ".dmohd" ? "DMOHD" : "DMO";
            entry.playable = extension == ".dmo";
            if(entry.playable)
            {
                fs::path sidecar = file; sidecar.replace_extension(".dmohd");
                if(fs::exists(sidecar, ec)) entry.format = "DMO + HD";
                infer_filename(entry);
                std::string parse_error;
                if(read_demo(path, entry, &parse_error)) entry.status = "Ready";
                else
                {
                    entry.status = "Corrupt: " + parse_error;
                    entry.playable = false;
                    if(error) *error = parse_error;
                }
            }
            else entry.status = "Orphan HD sidecar";
            classify(entry);
            return entry.playable;
        }

        void Library::remove_missing()
        {
            std::set<std::string> paths(scan_paths.begin(), scan_paths.end());
            records.erase(std::remove_if(records.begin(), records.end(), [&](const Entry &entry) {
                return !entry.trashed && !paths.count(entry.path);
            }), records.end());
        }

        Entry *Library::find(const std::string &id)
        {
            for(Entry &entry : records) if(entry.id == id) return &entry;
            return nullptr;
        }

        const Entry *Library::find(const std::string &id) const
        {
            for(const Entry &entry : records) if(entry.id == id) return &entry;
            return nullptr;
        }

        std::vector<std::size_t> Library::query(const Filter &filter, SortKey key, bool ascending) const
        {
            std::vector<std::size_t> result;
            const std::string mode = lowercase(filter.mode), player = lowercase(filter.player);
            for(std::size_t i = 0; i < records.size(); ++i)
            {
                const Entry &entry = records[i];
                if(entry.trashed != filter.trash) continue;
                if(!filter.search.empty() && !contains(entry.name, filter.search) && !contains(entry.map, filter.search) &&
                   !contains(entry.mode, filter.search) && !contains(entry.server, filter.search)) continue;
                if(!mode.empty() && lowercase(entry.mode) != mode) continue;
                if(!contains(entry.map, filter.map) || !contains(entry.server, filter.server) ||
                   !contains(entry.date, filter.date)) continue;
                if(!filter.format.empty() && !contains(entry.format, filter.format)) continue;
                if(filter.favorites_only && !entry.favorite) continue;
                const bool duel = entry.classification == Classification::ConfirmedDuel ||
                    entry.classification == Classification::ProbableDuel;
                if(filter.category == 1 && !duel) continue;
                if(filter.category == 2 && entry.classification != Classification::Normal) continue;
                if(filter.category == 3 && entry.classification != Classification::Unknown) continue;
                if(filter.duration_min_ms >= 0 && entry.duration_ms < filter.duration_min_ms) continue;
                if(filter.duration_max_ms >= 0 && (entry.duration_ms < 0 || entry.duration_ms >= filter.duration_max_ms)) continue;
                if(filter.size_min >= 0 && entry.size < filter.size_min) continue;
                if(filter.size_max >= 0 && entry.size > filter.size_max) continue;
                if(!player.empty())
                {
                    bool matched = false;
                    for(const Participant &participant : entry.participants)
                        if((!filter.active_players_only || participant.role == Role::Active) && contains(participant.name, player))
                        { matched = true; break; }
                    if(!matched) continue;
                }
                result.push_back(i);
            }
            auto compare = [&](std::size_t a, std::size_t b) {
                const Entry &left = records[a], &right = records[b];
                int order = 0;
                switch(key)
                {
                    case SortKey::Date: order = (left.modified > right.modified) - (left.modified < right.modified); break;
                    case SortKey::Duration: order = (left.duration_ms > right.duration_ms) - (left.duration_ms < right.duration_ms); break;
                    case SortKey::Mode: order = lowercase(left.mode).compare(lowercase(right.mode)); break;
                    case SortKey::Map: order = lowercase(left.map).compare(lowercase(right.map)); break;
                    case SortKey::Size: order = (left.size > right.size) - (left.size < right.size); break;
                    default: order = lowercase(left.name).compare(lowercase(right.name)); break;
                }
                if(!order) order = left.path.compare(right.path);
                return ascending ? order < 0 : order > 0;
            };
            std::stable_sort(result.begin(), result.end(), compare);
            return result;
        }

        bool Library::set_favorite(const std::string &id, bool value, std::string *error)
        {
            Entry *entry = find(id);
            if(!entry) { if(error) *error = "Demo no longer exists"; return false; }
            entry->favorite = value;
            return save(error);
        }

        bool Library::set_classification(const std::string &id, Classification value, std::string *error)
        {
            Entry *entry = find(id);
            if(!entry) { if(error) *error = "Demo no longer exists"; return false; }
            entry->classification = value;
            entry->classification_source = ClassificationSource::Manual;
            return save(error);
        }

        bool Library::valid_relative_folder(const std::string &folder) const
        {
            const fs::path path(folder);
            return !folder.empty() && !path.is_absolute() && path.lexically_normal().string().find("..") == std::string::npos;
        }

        bool Library::relocate(Entry &entry, const std::string &destination, std::string *error)
        {
            std::error_code ec;
            const fs::path source(entry.path), target(destination);
            if(source == target) return true;
            if(fs::exists(target, ec)) { if(error) *error = "Destination already exists"; return false; }
            fs::create_directories(target.parent_path(), ec);
            if(ec) { if(error) *error = "Cannot create destination folder: " + ec.message(); return false; }
            fs::rename(source, target, ec);
            if(ec) { if(error) *error = "Cannot move demo: " + ec.message(); return false; }
            fs::path source_hd = source; source_hd.replace_extension(".dmohd");
            fs::path target_hd = target; target_hd.replace_extension(".dmohd");
            if(lowercase(source.extension().string()) == ".dmo" && fs::exists(source_hd, ec))
            {
                ec.clear(); fs::rename(source_hd, target_hd, ec);
                if(ec)
                {
                    std::error_code rollback; fs::rename(target, source, rollback);
                    if(error) *error = "Cannot move HD sidecar: " + ec.message();
                    return false;
                }
            }
            entry.path = target.lexically_normal().string();
            entry.id = entry.path;
            entry.name = target.filename().string();
            return save(error);
        }

        bool Library::rename_entry(const std::string &id, const std::string &new_name, std::string *error)
        {
            Entry *entry = find(id);
            if(!entry) { if(error) *error = "Demo no longer exists"; return false; }
            std::string component = safe_component(new_name);
            if(component.empty() || component != new_name || fs::path(component).has_parent_path())
            { if(error) *error = "Invalid file name"; return false; }
            fs::path target = fs::path(entry->path).parent_path() / component;
            if(target.extension().empty()) target.replace_extension(fs::path(entry->path).extension());
            if(lowercase(target.extension().string()) != lowercase(fs::path(entry->path).extension().string()))
            { if(error) *error = "The demo extension cannot be changed"; return false; }
            return relocate(*entry, target.string(), error);
        }

        bool Library::move_entry(const std::string &id, const std::string &relative_folder, std::string *error)
        {
            Entry *entry = find(id);
            if(!entry) { if(error) *error = "Demo no longer exists"; return false; }
            if(!valid_relative_folder(relative_folder)) { if(error) *error = "Invalid destination folder"; return false; }
            return relocate(*entry, (fs::path(root) / relative_folder / entry->name).string(), error);
        }

        bool Library::organize_entry(const std::string &id, std::string *error)
        {
            Entry *entry = find(id);
            if(!entry) { if(error) *error = "Demo no longer exists"; return false; }
            std::string folder;
            if(entry->classification == Classification::ConfirmedDuel || entry->classification == Classification::ProbableDuel)
                folder = "duels/" + mode_folder(entry->mode);
            else if(entry->classification == Classification::Normal && !entry->mode.empty())
                folder = "modes/" + mode_folder(entry->mode);
            else folder = "unclassified";
            return move_entry(id, folder, error);
        }

        bool Library::trash_entry(const std::string &id, std::string *error)
        {
            Entry *entry = find(id);
            if(!entry || entry->trashed) { if(error) *error = "Demo is unavailable"; return false; }
            const std::string original = entry->path;
            fs::path target = fs::path(root) / ".trash" / entry->name;
            for(int suffix = 1; fs::exists(target); ++suffix)
                target = fs::path(root) / ".trash" /
                    (fs::path(entry->name).stem().string() + "_" + std::to_string(suffix) + fs::path(entry->name).extension().string());
            if(!relocate(*entry, target.string(), error)) return false;
            entry->original_path = original;
            entry->trashed = true;
            entry->deleted_date = date_string(std::time(nullptr));
            return save(error);
        }

        bool Library::restore_entry(const std::string &id, std::string *error)
        {
            Entry *entry = find(id);
            if(!entry || !entry->trashed) { if(error) *error = "Trash item is unavailable"; return false; }
            if(entry->original_path.empty()) { if(error) *error = "Original location is unknown"; return false; }
            if(!relocate(*entry, entry->original_path, error)) return false;
            entry->trashed = false;
            entry->original_path.clear();
            entry->deleted_date.clear();
            return save(error);
        }

        bool Library::erase_entry(const std::string &id, std::string *error)
        {
            auto iterator = std::find_if(records.begin(), records.end(), [&](const Entry &entry) { return entry.id == id; });
            if(iterator == records.end() || !iterator->trashed) { if(error) *error = "Only trash items can be erased"; return false; }
            std::error_code ec;
            fs::remove(iterator->path, ec);
            if(ec) { if(error) *error = "Cannot erase demo: " + ec.message(); return false; }
            fs::path sidecar = fs::path(iterator->path); sidecar.replace_extension(".dmohd");
            if(lowercase(fs::path(iterator->path).extension().string()) == ".dmo") { ec.clear(); fs::remove(sidecar, ec); }
            records.erase(iterator);
            return save(error);
        }

        bool Library::empty_trash(std::string *error)
        {
            std::vector<std::string> ids;
            for(const Entry &entry : records) if(entry.trashed) ids.push_back(entry.id);
            for(const std::string &id : ids) if(!erase_entry(id, error)) return false;
            return true;
        }

        bool Library::purge_expired(int retention_days, std::string *error)
        {
            if(retention_days <= 0) return true;
            const std::time_t cutoff = std::time(nullptr) - std::time_t(retention_days) * 24 * 60 * 60;
            std::vector<std::string> ids;
            for(const Entry &entry : records)
            {
                if(!entry.trashed || entry.deleted_date.empty()) continue;
                std::tm parsed{}; std::istringstream input(entry.deleted_date);
                input >> std::get_time(&parsed, "%Y-%m-%d %H:%M");
                if(!input.fail() && std::mktime(&parsed) < cutoff) ids.push_back(entry.id);
            }
            for(const std::string &id : ids) if(!erase_entry(id, error)) return false;
            return true;
        }

        std::int64_t Library::trash_size() const
        {
            std::int64_t result = 0;
            for(const Entry &entry : records) if(entry.trashed) result += entry.size;
            return result;
        }

        std::string Library::relative_path(const Entry &entry) const
        {
            std::error_code ec;
            const fs::path relative = fs::relative(entry.path, root, ec);
            return ec ? entry.path : relative.generic_string();
        }
    }
}
