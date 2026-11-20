#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    int passed = 0, failed = 0;
    std::string readfile(const char *path)
    {
        std::ifstream input(path);
        std::ostringstream out;
        out << input.rdbuf();
        return out.str();
    }
    void check(bool value, const char *name)
    {
        if(value) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }
}

int main()
{
    const std::string rml = readfile(
        "../data/cowbraten/ui/default/screens/scoreboard.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/scoreboard.rcss");
    const std::string model = readfile(
        "../src/cowbraten/ui/rml_scoreboard.cpp");
    const std::string bridge = readfile(
        "../src/cowbraten/ui/sauer_scoreboard_bridge.cpp");
    const std::string bridge_header = readfile(
        "../src/cowbraten/ui/sauer_scoreboard_bridge.h");
    const std::string ui = readfile("../src/cowbraten/ui/ui.cpp");
    const std::string cube_text = readfile(
        "../src/cowbraten/ui/cube_text.h");
    const std::string legacy = readfile("../src/fpsgame/scoreboard.cpp");

    check(!rml.empty() && !style.empty() && !model.empty(),
        "load live scoreboard document, style, and model");
    check(rml.find("data-model=\"live_scoreboard\"") != std::string::npos &&
          model.find("CreateDataModel(\"live_scoreboard\")") != std::string::npos,
        "live scoreboard uses a typed RmlUi model");
    check(bridge.find("game::groupplayers()") != std::string::npos &&
          bridge.find("game::groups") != std::string::npos &&
          bridge.find("game::spectators") != std::string::npos,
        "rows preserve native grouping, sorting, and spectators");
    check(model.find("player.group_name == \"good\"") != std::string::npos &&
          model.find("player.group_name == \"evil\"") != std::string::npos &&
          rml.find("data-class-is-good-team=\"player.good_team\"") !=
              std::string::npos &&
          rml.find("data-class-is-evil-team=\"player.evil_team\"") !=
              std::string::npos &&
          style.find(".scoreboard-player.is-good-team") != std::string::npos &&
          style.find(".scoreboard-player.is-evil-team") != std::string::npos,
        "team groups and rows retain native blue and red identities");
    check(bridge.find("game::playeraccuracy") != std::string::npos &&
          bridge.find("game::playerdamage") != std::string::npos &&
          bridge.find("game::playernetdamage") != std::string::npos,
        "advanced columns use authoritative weapon statistics");
    check(bridge_header.find("#include <string>") == std::string::npos &&
          bridge_header.find("PlayerVisitor") != std::string::npos,
        "scoreboard engine boundary remains POD and visitor based");
    check(rml.find("data-if=\"show_flags\"") != std::string::npos &&
          rml.find("data-if=\"show_frags\"") != std::string::npos &&
          rml.find("data-if=\"show_damage_received\"") != std::string::npos &&
          rml.find("data-if=\"show_client_num\"") != std::string::npos,
        "document honors all configurable scoreboard columns");
    check(model.find("requested && !menu_active") != std::string::npos &&
          ui.find("scoreboard_model.update(active_screen != nullptr)") !=
              std::string::npos,
        "menus suspend the gameplay overlay without losing key state");
    check(model.find("if(document) applyinterfacepreferences(document);") !=
              std::string::npos &&
          ui.find("void applyinterfacepreferences(") != std::string::npos,
        "scoreboard applies text scale and typeface on its first lazy load");
    check(legacy.find("cowui::setscoreboardvisible(on)") != std::string::npos &&
          legacy.find("scoreboard.show(on && !native_scoreboard)") !=
              std::string::npos,
        "showscores uses RmlUi with a safe g3d fallback");
    check(style.find(".scoreboard-overlay") != std::string::npos &&
          style.find("background-color: transparent") != std::string::npos &&
          style.find("background-color: #0B1015C2") != std::string::npos &&
          style.find(".scoreboard-player.is-local") != std::string::npos,
        "translucent panel preserves gameplay without an outer black tint");
    check(rml.find("scoreboard-kicker") != std::string::npos &&
          rml.find("cb-ico-clock") != std::string::npos &&
          rml.find("cb-ico-crosshair") != std::string::npos &&
          rml.find("scoreboard-footer-hint") != std::string::npos &&
          style.find(".scoreboard-header-rule") != std::string::npos,
        "scoreboard matches the compact editorial overlay composition");
    check(style.find(".score-column") != std::string::npos &&
          style.find("justify-content: center") != std::string::npos &&
          style.find(".scoreboard-columns .score-stat") != std::string::npos &&
          style.find("text-align: center") != std::string::npos,
        "scoreboard column labels center icons and text independently of values");
    check(rml.find("score-column score-frags") != std::string::npos &&
          rml.find("score-column score-deaths") != std::string::npos &&
          style.find(".score-frags") != std::string::npos &&
          style.find(".score-deaths") != std::string::npos,
        "long Frags and Deaths labels receive dedicated wider columns");
    check(style.find(".scoreboard-scroll") != std::string::npos &&
          style.find("flex-shrink: 1") != std::string::npos &&
          style.find("overflow-y: auto") != std::string::npos &&
          rml.find("id=\"scoreboard-scroll\"") != std::string::npos &&
          model.find("SetScrollTop") != std::string::npos &&
          ui.find("scoreboard_model.scroll") != std::string::npos,
        "large player and spectator lists scroll between fixed chrome");
    check(model.find("plain_cube_text(state.server_name)") != std::string::npos,
        "server color controls are removed before RmlUi renders the title");
    check(model.find("row.name = plain_cube_text(player.name)") !=
              std::string::npos &&
          cube_text.find("sauertext::utf8(text.c_str(), text.size())") !=
              std::string::npos,
        "player names cross the Cube-to-UTF-8 boundary before rendering");
    check(model.find("SYNC_VALUE(server_name, next_server_name)") !=
              std::string::npos &&
          model.find("for(const char *variable : variables)") ==
              std::string::npos,
        "unchanged scoreboard scalars do not dirty the data model every frame");
    check(style.find("min-height: 37dp") != std::string::npos &&
          style.find("max-height: 90%") != std::string::npos,
        "dense rows and taller panel keep more players visible at once");
    check(style.find("@media (min-width: 2000dp)") != std::string::npos &&
          style.find("max-width: 1600dp") != std::string::npos &&
          style.find("font-size: 0.875rem") != std::string::npos,
        "QHD viewports scale content without stretching the scoreboard");
    check(rml.find("scoreboard-teams") != std::string::npos &&
          rml.find("player : left_team_rows") != std::string::npos &&
          rml.find("player : right_team_rows") != std::string::npos &&
          model.find("Bind(\"left_team_rows\"") != std::string::npos &&
          model.find("Bind(\"right_team_rows\"") != std::string::npos &&
          style.find(".scoreboard-team") != std::string::npos,
        "team matches present observed and opposing rosters side by side");
    check(rml.find("player : spectator_rows") != std::string::npos &&
          style.find(".scoreboard-spectator-grid") != std::string::npos,
        "spectators use a compact grid below both team columns");
    check(style.find(".scoreboard-team .score-stat") != std::string::npos &&
          style.find("width: 44dp") != std::string::npos &&
          style.find("min-width: 34dp") != std::string::npos &&
          style.find("flex: 0 1 44dp") != std::string::npos &&
          style.find("text-align: center") != std::string::npos,
        "visible team metrics stay compact and return room to player names");
    check(style.find("min-width: 184dp") != std::string::npos &&
          style.find("min-width: 210dp") != std::string::npos &&
          style.find("width: 260dp") != std::string::npos &&
          style.find("max-width: 1600dp") != std::string::npos &&
          style.find("flex: 1") != std::string::npos,
        "player name columns preserve long names and absorb spare width");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
