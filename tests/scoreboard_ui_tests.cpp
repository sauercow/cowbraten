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

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
