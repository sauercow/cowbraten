#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    int passed = 0, failed = 0;

    std::string readfile(const char *path)
    {
        std::ifstream input(path);
        std::ostringstream contents;
        contents << input.rdbuf();
        return contents.str();
    }

    void check(bool condition, const char *name)
    {
        if(condition) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }
}

int main()
{
    const std::string rml = readfile(
        "../data/cowbraten/ui/default/screens/games.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/games.rcss");
    const std::string model = readfile(
        "../src/cowbraten/ui/rml_game_setup.cpp");
    const std::string api = readfile(
        "../src/cowbraten/ui/game_setup_api.cpp");
    const std::string bridge = readfile(
        "../src/cowbraten/ui/sauer_game_setup_bridge.cpp");
    const std::string bridge_header = readfile(
        "../src/cowbraten/ui/sauer_game_setup_bridge.h");
    const std::string ui = readfile("../src/cowbraten/ui/ui.cpp");
    const std::string commands = readfile(
        "../src/cowbraten/ui/ui_commands.cpp");
    const std::string shell = readfile(
        "../data/cowbraten/ui/default/templates/app_shell.rml");
    const std::string game_menu = readfile(
        "../data/cowbraten/ui/default/screens/game_menu.rml");

    check(!rml.empty() && !style.empty() && !model.empty(),
        "load Play screen, style, and model");
    check(rml.find("data-model=\"game_setup\"") != std::string::npos &&
          model.find("CreateDataModel(\"game_setup\")") != std::string::npos,
        "Play markup and implementation share a typed data model");
    check(rml.find("data-for=\"mode : modes\"") != std::string::npos &&
          rml.find("data-for=\"map : maps\"") != std::string::npos &&
          rml.find("set_filter(ev.value)") != std::string::npos,
        "installed modes and maps expose searchable typed collections");
    check(rml.find("data-if=\"section_bots\"") != std::string::npos &&
          rml.find("set_bot_count(ev.value)") != std::string::npos &&
          rml.find("set_bot_min(ev.value)") != std::string::npos &&
          rml.find("set_bot_max(ev.value)") != std::string::npos,
        "bot setup exposes count and skill controls");
    check(rml.find("data-for=\"mission : campaigns\"") != std::string::npos &&
          rml.find("launch_campaign(mission.id)") != std::string::npos &&
          bridge.find("Run N' Gun") != std::string::npos &&
          bridge.find("An Army Of One") != std::string::npos,
        "campaign route preserves both original mission series");

    const std::regex action_pattern(
        "onclick=\"(games\\.[a-zA-Z0-9_.-]+)\"");
    bool actions_registered = true;
    int action_count = 0;
    for(std::sregex_iterator it(rml.begin(), rml.end(), action_pattern), end;
        it != end; ++it)
    {
        ++action_count;
        actions_registered = actions_registered &&
            model.find("\"" + (*it)[1].str() + "\"") != std::string::npos;
    }
    check(action_count == 4 && actions_registered,
        "all Play document actions are registered");
    check(bridge.find("listfiles(\"packages/base\", \"ogz\"") !=
              std::string::npos &&
          bridge.find("fileexists(image, \"rb\")") != std::string::npos,
        "map catalog comes from installed engine packages and optional previews");
    check(bridge.find("game::changemap(map, mode)") != std::string::npos &&
          bridge.find("N_ADDBOT") != std::string::npos &&
          bridge.find("game::changemap(campaign_entries[id].map, -3)") !=
              std::string::npos,
        "launch bridge supports maps, bots, and single-player missions");
    check(bridge_header.find("#include <string>") == std::string::npos &&
          bridge_header.find("ModeVisitor") != std::string::npos &&
          api.find("sauergamesetup::visitmaps") != std::string::npos,
        "legacy engine boundary stays POD and visitor based");
    check(ui.find("!std::strcmp(name, \"gamemode\")") != std::string::npos &&
          ui.find("!std::strcmp(name, \"botmatch\")") != std::string::npos &&
          ui.find("!std::strcmp(name, \"campaign\")") != std::string::npos,
        "legacy game setup names route to the native Play screen");
    check(ui.find("game_setup_model.activate(1)") != std::string::npos &&
          ui.find("game_setup_model.activate(2)") != std::string::npos,
        "bot and campaign legacy routes select their matching native tabs");
    check(shell.find("onclick=\"navigation.games\"") != std::string::npos &&
          game_menu.find("onclick=\"navigation.games\"") != std::string::npos &&
          commands.find("ICOMMAND(ui_games") != std::string::npos,
        "Play is reachable from shell, in-game menu, and console command");
    check(style.find(".map-card") != std::string::npos &&
          style.find("min-width: 132dp;") != std::string::npos &&
          style.find(".campaign-card") != std::string::npos,
        "map and campaign cards keep stable dimensions in flex layout");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
