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
    const std::string implementation = readfile(
        "../src/cowbraten/ui/rml_game_menu.cpp");
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
    check(screen.find("cb-ico-play") != std::string::npos &&
          screen.find("cb-ico-spectator") != std::string::npos &&
          screen.find("cb-ico-team-switch") != std::string::npos &&
          screen.find("cb-ico-server") != std::string::npos &&
          screen.find("cb-ico-settings") != std::string::npos &&
          screen.find("cb-ico-log-out") != std::string::npos,
        "match actions use semantic icons from the shared pack");
    const std::size_t spectator_toggle = screen.find("game.spectator.toggle");
    check(spectator_toggle != std::string::npos &&
          screen.find("game.spectator.toggle", spectator_toggle + 1) == std::string::npos &&
          screen.find("spectator ? 'ON' : 'OFF'") != std::string::npos,
        "spectator action appears once and reflects the current player state");
    check(screen.find("onclick=\"game.music.toggle\"") != std::string::npos &&
          screen.find("data-class-is-on=\"music_enabled\"") != std::string::npos &&
          screen.find("cb-ico-music-off") != std::string::npos &&
          implementation.find("music_enabled ? 0 : 125") != std::string::npos,
        "quick toggle switches native music volume between zero and 125");
    check(screen.find("data-if=\"team_mode\"") != std::string::npos &&
          screen.find("game.team.toggle") != std::string::npos &&
          screen.find("game.team.good") != std::string::npos &&
          screen.find("game.team.evil") != std::string::npos,
        "team controls only appear for supported team matches");
    check(style.find(".game-menu-team-buttons button") != std::string::npos &&
          screen.find(">Blue</strong>") != std::string::npos &&
          screen.find(">Red</strong>") != std::string::npos &&
          style.find("align-items: center") != std::string::npos &&
          style.find("justify-content: center") != std::string::npos,
        "Blue and Red team labels remain centered inside their buttons");
    check(screen.find("onclick=\"game.master.claim\"") != std::string::npos &&
          screen.find("onclick=\"game.master.relinquish\"") != std::string::npos &&
          screen.find("select_master_mode(0)") != std::string::npos &&
          screen.find("select_master_mode(3)") != std::string::npos &&
          screen.find("data-class-is-selected=\"master_mode == 0\"") != std::string::npos,
        "in-game quick actions expose master ownership and server access modes");
    check(screen.find("cb-ico-crown") != std::string::npos &&
          screen.find("cb-ico-unlock") != std::string::npos &&
          screen.find("cb-ico-lock") != std::string::npos &&
          screen.find("cb-ico-eye-off") != std::string::npos &&
          style.find(".game-menu-master-modes button") != std::string::npos &&
          style.find("justify-content: center") != std::string::npos,
        "in-game access buttons align icon and label from the shared pack");
    check(implementation.find("admin::readstate") != std::string::npos &&
          implementation.find("admin::setlocalmaster(true)") != std::string::npos &&
          implementation.find("admin::setlocalmaster(false)") != std::string::npos &&
          implementation.find("admin::setmastermode") != std::string::npos,
        "master quick actions use the typed native administration API");
    check(screen.find("map_image") != std::string::npos &&
          screen.find("server_address") != std::string::npos &&
          screen.find("time_remaining") != std::string::npos &&
          screen.find("player_count_label") != std::string::npos &&
          screen.find("ping_label") != std::string::npos,
        "match status presents live server and map information");
    check(implementation.find("plain_cube_text(state.server_name)") !=
              std::string::npos &&
          implementation.find("#include \"cube_text.h\"") !=
              std::string::npos,
        "match status removes Cube color controls from the server title");
    check(style.find("background-color: rgba(5, 7, 10, 0.66)") != std::string::npos &&
          style.find("width: 1220dp") != std::string::npos &&
          style.find(".game-menu-dashboard") != std::string::npos,
        "menu uses the wide dashboard composition over gameplay");
    check(style.find("@media (max-width: 1040dp)") != std::string::npos &&
          style.find("overflow-y: auto") != std::string::npos,
        "dashboard stacks and scrolls on compact resolutions");
    check(style.find(".game-menu-navigation") != std::string::npos &&
          style.find("min-height: 0") != std::string::npos &&
          style.find("overflow-y: auto") != std::string::npos,
        "team-mode navigation scrolls instead of clipping Disconnect");
    check(style.find(".game-menu-master-modes") != std::string::npos &&
          style.find(".game-menu-quick-card") != std::string::npos &&
          style.find("overflow-y: auto") != std::string::npos,
        "compact master controls remain reachable when quick actions overflow");
    check(style.find("font-size: 0.5rem") == std::string::npos &&
          style.find("font-size: 0.5625rem") == std::string::npos &&
          style.find(".game-menu-quick-copy small") != std::string::npos &&
          style.find(".game-menu-stat small") != std::string::npos &&
          style.find("font-size: var(--font-size-control)") !=
              std::string::npos,
        "secondary match text keeps a readable ten-pixel baseline");
    check(style.find(":focus-visible") != std::string::npos,
        "match controls expose keyboard focus states");
    check(input_bridge.find("isconnected(true, true)") != std::string::npos,
        "local and remote matches both open the in-game menu");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
