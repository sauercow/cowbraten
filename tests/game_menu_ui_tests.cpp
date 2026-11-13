#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    int passed = 0, failed = 0;

    void check(bool condition, const std::string &name)
    {
        if(condition) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }

    std::string readfile(const std::string &path)
    {
        std::ifstream input(path);
        std::ostringstream contents;
        contents << input.rdbuf();
        return contents.str();
    }
}

int main()
{
    const std::string root = "../data/cowbraten/ui/default/";
    const std::string screen = readfile(root + "screens/game_menu.rml");
    const std::string style = readfile(root + "styles/game_menu.rcss");
    const std::string input_bridge = readfile(
        "../src/cowbraten/ui/sauer_input_bridge.cpp");

    check(!screen.empty() && !style.empty(), "load match menu assets");
    check(screen.find("data-model=\"game_menu\"") != std::string::npos,
        "match menu binds its typed data model");
    check(screen.find("game.resume") != std::string::npos &&
          screen.find("navigation.servers") != std::string::npos &&
          screen.find("navigation.settings") != std::string::npos &&
          screen.find("navigation.profile") != std::string::npos &&
          screen.find("game.disconnect") != std::string::npos,
        "match menu exposes primary navigation actions");
    check(screen.find("game.spectator.toggle") != std::string::npos &&
          screen.find("spectator ? 'ON' : 'OFF'") != std::string::npos,
        "spectator action reflects the current player state");
    check(screen.find("data-if=\"team_mode\"") != std::string::npos &&
          screen.find("game.team.toggle") != std::string::npos &&
          screen.find("game.team.good") != std::string::npos &&
          screen.find("game.team.evil") != std::string::npos,
        "team controls only appear for supported team matches");
    check(screen.find("map_image") != std::string::npos &&
          screen.find("server_address") != std::string::npos &&
          screen.find("time_remaining") != std::string::npos &&
          screen.find("player_count_label") != std::string::npos &&
          screen.find("ping_label") != std::string::npos,
        "match status presents live server and map information");
    check(style.find("background-color: rgba(5, 7, 10, 0.66)") != std::string::npos &&
          style.find("width: 1220dp") != std::string::npos &&
          style.find(".game-menu-dashboard") != std::string::npos,
        "menu uses the wide dashboard composition over gameplay");
    check(style.find("@media (max-width: 1040dp)") != std::string::npos &&
          style.find("overflow-y: auto") != std::string::npos,
        "dashboard stacks and scrolls on compact resolutions");
    check(style.find(":focus-visible") != std::string::npos,
        "match controls expose keyboard focus states");
    check(input_bridge.find("isconnected(true, true)") != std::string::npos,
        "local and remote matches both open the in-game menu");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
