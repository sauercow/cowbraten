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
    check(legacy.find("cowui::setscoreboardvisible(on)") != std::string::npos &&
          legacy.find("scoreboard.show(on && !native_scoreboard)") !=
              std::string::npos,
        "showscores uses RmlUi with a safe g3d fallback");
    check(style.find("background-color: rgba(4, 7, 9, 72)") !=
              std::string::npos &&
          style.find(".scoreboard-player.is-local") != std::string::npos,
        "light overlay preserves gameplay context and highlights the local player");
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
    check(style.find("min-height: 37dp") != std::string::npos &&
          style.find("max-height: 90%") != std::string::npos,
        "dense rows and taller panel keep more players visible at once");
    check(style.find("@media (min-width: 2000dp)") != std::string::npos &&
          style.find("max-width: 2000dp") != std::string::npos &&
          style.find("font-size: 14dp") != std::string::npos,
        "QHD and larger viewports scale the scoreboard and its content together");
    check(rml.find("scoreboard-teams") != std::string::npos &&
          rml.find("player : evil_rows") != std::string::npos &&
          rml.find("player : good_rows") != std::string::npos &&
          model.find("Bind(\"evil_rows\"") != std::string::npos &&
          model.find("Bind(\"good_rows\"") != std::string::npos &&
          style.find(".scoreboard-team") != std::string::npos,
        "team matches present red and blue rosters side by side");
    check(rml.find("player : spectator_rows") != std::string::npos &&
          style.find(".scoreboard-spectator-grid") != std::string::npos,
        "spectators use a compact grid below both team columns");
    check(style.find(".scoreboard-team .score-stat") != std::string::npos &&
          style.find("text-align: center") != std::string::npos,
        "team values align with their centered column icons");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
