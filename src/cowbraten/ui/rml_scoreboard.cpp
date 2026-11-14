#include "rml_scoreboard.h"

#include "cube_text.h"
#include "rml_action.h"
#include "scoreboard_api.h"
#include "ui.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cowui
{
    bool RmlScoreboard::Row::operator==(const Row &other) const
    {
        return group_index == other.group_index &&
            group_first == other.group_first && local == other.local &&
            spectator == other.spectator && dead == other.dead &&
            privileged == other.privileged && good_team == other.good_team &&
            evil_team == other.evil_team && group_title == other.group_title &&
            name == other.name && role == other.role && flags == other.flags &&
            frags == other.frags && deaths == other.deaths &&
            suicides == other.suicides && kpd == other.kpd &&
            accuracy == other.accuracy && damage == other.damage &&
            damage_received == other.damage_received && ping == other.ping &&
            pj == other.pj && client_num == other.client_num;
    }

    bool RmlScoreboard::init(Rml::Context *new_context,
        RmlActionRouter &)
    {
        if(!new_context) return false;
        Rml::DataModelConstructor constructor =
            new_context->CreateDataModel("live_scoreboard");
        if(!constructor) return false;
        bool valid = true;
        if(auto row = constructor.RegisterStruct<Row>())
            valid = row.RegisterMember("group_index", &Row::group_index) &&
                row.RegisterMember("group_first", &Row::group_first) &&
                row.RegisterMember("local", &Row::local) &&
                row.RegisterMember("spectator", &Row::spectator) &&
                row.RegisterMember("dead", &Row::dead) &&
                row.RegisterMember("privileged", &Row::privileged) &&
                row.RegisterMember("good_team", &Row::good_team) &&
                row.RegisterMember("evil_team", &Row::evil_team) &&
                row.RegisterMember("group_title", &Row::group_title) &&
                row.RegisterMember("name", &Row::name) &&
                row.RegisterMember("role", &Row::role) &&
                row.RegisterMember("flags", &Row::flags) &&
                row.RegisterMember("frags", &Row::frags) &&
                row.RegisterMember("deaths", &Row::deaths) &&
                row.RegisterMember("suicides", &Row::suicides) &&
                row.RegisterMember("kpd", &Row::kpd) &&
                row.RegisterMember("accuracy", &Row::accuracy) &&
                row.RegisterMember("damage", &Row::damage) &&
                row.RegisterMember("damage_received", &Row::damage_received) &&
                row.RegisterMember("ping", &Row::ping) &&
                row.RegisterMember("pj", &Row::pj) &&
                row.RegisterMember("client_num", &Row::client_num);
        else valid = false;
        valid = valid && constructor.RegisterArray<std::vector<Row>>() &&
            constructor.Bind("rows", &rows) &&
            constructor.Bind("evil_rows", &evil_rows) &&
            constructor.Bind("good_rows", &good_rows) &&
            constructor.Bind("spectator_rows", &spectator_rows) &&
            constructor.Bind("server_name", &server_name) &&
            constructor.Bind("match_line", &match_line) &&
            constructor.Bind("time_text", &time_text) &&
            constructor.Bind("evil_title", &evil_title) &&
            constructor.Bind("good_title", &good_title) &&
            constructor.Bind("team_mode", &team_mode) &&
            constructor.Bind("has_spectators", &has_spectators) &&
            constructor.Bind("show_flags", &show_flags) &&
            constructor.Bind("show_frags", &show_frags) &&
            constructor.Bind("show_deaths", &show_deaths) &&
            constructor.Bind("show_suicides", &show_suicides) &&
            constructor.Bind("show_kpd", &show_kpd) &&
            constructor.Bind("show_accuracy", &show_accuracy) &&
            constructor.Bind("show_damage", &show_damage) &&
            constructor.Bind("show_damage_received", &show_damage_received) &&
            constructor.Bind("show_ping", &show_ping) &&
            constructor.Bind("show_pj", &show_pj) &&
            constructor.Bind("show_client_num", &show_client_num);
        if(!valid)
        {
            new_context->RemoveDataModel("live_scoreboard");
            return false;
        }
        context = new_context;
        model = constructor.GetModelHandle();
        return true;
    }

    void RmlScoreboard::shutdown(Rml::Context *old_context)
    {
        if(document) document->Close();
        document = nullptr;
        document_visible = requested = false;
        if(old_context && model) old_context->RemoveDataModel("live_scoreboard");
        model = Rml::DataModelHandle();
        context = nullptr;
        rows.clear();
        evil_rows.clear();
        good_rows.clear();
        spectator_rows.clear();
    }

    bool RmlScoreboard::loaddocument()
    {
        if(document) return true;
        if(load_attempted || !context) return false;
        load_attempted = true;
        document = context->LoadDocument(
            "data/cowbraten/ui/default/screens/scoreboard.rml");
        if(document && !applytheme(document))
        {
            document->Close();
            document = nullptr;
        }
        if(!document)
            std::fprintf(stderr, "Cowbraten UI: failed to load scoreboard overlay\n");
        return document != nullptr;
    }

    void RmlScoreboard::setvisible(bool value)
    {
        if(value == document_visible) return;
        if(value)
        {
            if(!loaddocument()) return;
            document->Show();
        }
        else if(document) document->Hide();
        document_visible = value;
    }

    void RmlScoreboard::request(bool value)
    {
        requested = value;
        if(!value) setvisible(false);
    }

    static Rml::String damage_text(float value)
    {
        if(std::fabs(value) >= 1000.0f)
            return Rml::CreateString("%.1fk", value / 1000.0f);
        return Rml::CreateString("%.0f", value);
    }

    void RmlScoreboard::sync()
    {
        scoreboard::State state;
        scoreboard::readstate(state);
        const Rml::String next_server_name = plain_cube_text(state.server_name);
        const Rml::String next_match_line = plain_cube_text(state.mode_name) +
            "  //  " + plain_cube_text(state.map_name);
        const Rml::String next_time_text = state.intermission ? "INTERMISSION" :
            (state.paused ? "PAUSED" :
                (state.seconds_remaining >= 0
                    ? Rml::CreateString("%d:%02d", state.seconds_remaining / 60,
                        state.seconds_remaining % 60)
                    : Rml::String("UNTIMED")));
#define SYNC_VALUE(variable, value) do { const auto &next_value = (value); \
        if(variable != next_value) { variable = next_value; \
        model.DirtyVariable(#variable); } } while(0)
        SYNC_VALUE(server_name, next_server_name);
        SYNC_VALUE(match_line, next_match_line);
        SYNC_VALUE(time_text, next_time_text);
        SYNC_VALUE(show_flags, state.show_flags);
        SYNC_VALUE(show_frags, state.show_frags);
        SYNC_VALUE(show_deaths, state.show_deaths);
        SYNC_VALUE(show_suicides, state.show_suicides);
        SYNC_VALUE(show_kpd, state.show_kpd);
        SYNC_VALUE(show_accuracy, state.show_accuracy);
        SYNC_VALUE(show_damage, state.show_damage);
        SYNC_VALUE(show_damage_received, state.show_damage_received);
        SYNC_VALUE(show_ping, state.show_ping);
        SYNC_VALUE(show_pj, state.show_pj);
        SYNC_VALUE(show_client_num, state.show_client_num);
#undef SYNC_VALUE

        std::vector<scoreboard::Player> players;
        scoreboard::readplayers(players);
        const bool next_team_mode = std::any_of(players.begin(), players.end(),
            [](const scoreboard::Player &player)
            {
                return !player.spectator &&
                    (player.group_name == "good" || player.team == "good" ||
                     player.group_name == "evil" || player.team == "evil");
            });
        std::vector<Row> next, next_evil, next_good, next_spectators;
        Rml::String next_evil_title = "EVIL", next_good_title = "GOOD";
        for(const scoreboard::Player &player : players)
        {
            Row row;
            row.group_index = player.group_index;
            row.group_first = player.group_first;
            row.group_title = player.spectator ? "Spectators" :
                (player.group_name == "Players" ? Rml::String("Players") :
                    Rml::CreateString("%s  //  %d", player.group_name.c_str(),
                        player.group_score));
            row.local = player.local;
            row.spectator = player.spectator;
            row.dead = player.dead;
            row.privileged = player.privilege != 0;
            row.good_team = player.group_name == "good" ||
                player.team == "good";
            row.evil_team = player.group_name == "evil" ||
                player.team == "evil";
            row.name = plain_cube_text(player.name);
            row.role = player.privilege >= 2 ? "ADMIN" :
                (player.privilege ? "MASTER" :
                    (player.spectator ? "SPECTATOR" : ""));
            row.flags = Rml::CreateString("%d", player.flags);
            row.frags = Rml::CreateString("%d", player.frags);
            row.deaths = Rml::CreateString("%d", player.deaths);
            row.suicides = Rml::CreateString("%d", player.suicides);
            row.kpd = Rml::CreateString("%.1f", player.kpd);
            row.accuracy = Rml::CreateString("%.0f%%", player.accuracy);
            row.damage = damage_text(player.damage);
            row.damage_received = damage_text(player.damage_received);
            row.ping = player.ping < 0 ? "LAG" : Rml::CreateString("%d", player.ping);
            row.pj = player.pj < 0 ? "LAG" : Rml::CreateString("%d", player.pj);
            row.client_num = Rml::CreateString("%d", player.client_num);
            if(player.spectator)
                next_spectators.push_back(row);
            else if(next_team_mode && row.good_team)
            {
                if(player.group_first) next_good_title = row.group_title;
                row.group_first = false;
                next_good.push_back(row);
            }
            else if(next_team_mode && row.evil_team)
            {
                if(player.group_first) next_evil_title = row.group_title;
                row.group_first = false;
                next_evil.push_back(row);
            }
            else if(next_team_mode)
            {
                row.group_first = false;
                std::vector<Row> &shorter = next_evil.size() <= next_good.size()
                    ? next_evil : next_good;
                shorter.push_back(row);
            }
            else next.push_back(row);
        }
        if(next != rows)
        {
            rows.swap(next);
            model.DirtyVariable("rows");
        }
        if(next_evil != evil_rows)
        {
            evil_rows.swap(next_evil);
            model.DirtyVariable("evil_rows");
        }
        if(next_good != good_rows)
        {
            good_rows.swap(next_good);
            model.DirtyVariable("good_rows");
        }
        if(next_spectators != spectator_rows)
        {
            spectator_rows.swap(next_spectators);
            model.DirtyVariable("spectator_rows");
        }
        const bool next_has_spectators = !spectator_rows.empty();
        if(has_spectators != next_has_spectators)
        {
            has_spectators = next_has_spectators;
            model.DirtyVariable("has_spectators");
        }
        if(team_mode != next_team_mode)
        {
            team_mode = next_team_mode;
            model.DirtyVariable("team_mode");
        }
        if(evil_title != next_evil_title)
        {
            evil_title = next_evil_title;
            model.DirtyVariable("evil_title");
        }
        if(good_title != next_good_title)
        {
            good_title = next_good_title;
            model.DirtyVariable("good_title");
        }
    }

    void RmlScoreboard::update(bool menu_active)
    {
        const bool should_show = requested && !menu_active;
        if(should_show) sync();
        setvisible(should_show);
    }

    void RmlScoreboard::scroll(float wheel_delta)
    {
        if(!document_visible || !document || wheel_delta == 0.0f) return;
        if(Rml::Element *list = document->GetElementById("scoreboard-scroll"))
            list->SetScrollTop(list->GetScrollTop() - wheel_delta * 64.0f);
    }
}
