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
        "../data/cowbraten/ui/default/screens/system.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/system.rcss");
    const std::string model = readfile(
        "../src/cowbraten/ui/rml_system.cpp");
    const std::string bridge = readfile(
        "../src/cowbraten/ui/sauer_admin_bridge.cpp");
    const std::string bridge_header = readfile(
        "../src/cowbraten/ui/sauer_admin_bridge.h");
    const std::string ui = readfile("../src/cowbraten/ui/ui.cpp");
    const std::string settings = readfile(
        "../src/cowbraten/ui/rml_settings.cpp");
    const std::string settings_rml = readfile(
        "../data/cowbraten/ui/default/screens/settings.rml");
    const std::string shell = readfile(
        "../data/cowbraten/ui/default/templates/app_shell.rml");

    check(!rml.empty() && !style.empty() && !model.empty(),
        "load Administration and About screen contract");
    check(rml.find("data-model=\"system_admin\"") != std::string::npos &&
          model.find("CreateDataModel(\"system_admin\")") != std::string::npos,
        "System screen uses a typed RmlUi model");
    check(rml.find("select_master_mode(0)") != std::string::npos &&
          rml.find("select_master_mode(3)") != std::string::npos &&
          rml.find("kick_client(client.client_num)") != std::string::npos &&
          rml.find("toggle_spectator(client.client_num)") != std::string::npos &&
          rml.find("switch_team(client.client_num)") != std::string::npos &&
          rml.find("toggle_master(client.client_num)") != std::string::npos,
        "Administration exposes all legacy moderation actions");
    check(bridge.find("N_SETMASTER") != std::string::npos &&
          bridge.find("N_MASTERMODE") != std::string::npos &&
          bridge.find("N_KICK") != std::string::npos &&
          bridge.find("N_SPECTATOR") != std::string::npos &&
          bridge.find("N_SETTEAM") != std::string::npos,
        "moderation actions use native network messages");
    check(bridge_header.find("#include <string>") == std::string::npos &&
          bridge_header.find("ClientVisitor") != std::string::npos,
        "engine boundary remains POD and visitor based");
    check(model.find("plain_cube_text(state.server_name)") !=
              std::string::npos &&
          model.find("plain_cube_text(client.name)") != std::string::npos,
        "Administration converts native server and player names to UTF-8");
    check(ui.find("!std::strcmp(name, \"master\")") != std::string::npos &&
          ui.find("system_model.activate(0)") != std::string::npos &&
          ui.find("!std::strcmp(name, \"about\")") != std::string::npos &&
          ui.find("system_model.activate(1)") != std::string::npos,
        "master and about legacy routes select native tabs");
    check(ui.find("!std::strcmp(name, \"cowbratenusage\")") != std::string::npos &&
          rml.find("client.version") != std::string::npos &&
          bridge.find("getclientp1xbratenversion") != std::string::npos,
        "compatible-client versions route into native Administration");
    check(rml.find("Cube 2: Sauerbraten") != std::string::npos &&
          rml.find("RmlUi") != std::string::npos &&
          rml.find("sauerbraten.org") != std::string::npos,
        "About retains engine, interface, contributor, and website credits");
    check(rml.find("Version {{ cowbraten_version }}") != std::string::npos &&
          model.find("Bind(\"cowbraten_version\"") != std::string::npos &&
          model.find("admin::clientversion()") != std::string::npos &&
          model.find("DirtyVariable(\"cowbraten_version\")") != std::string::npos &&
          bridge.find("cowbratenversion") != std::string::npos,
        "About refreshes the engine-owned Cowbraten release version");
    check(shell.find("onclick=\"navigation.about\"") != std::string::npos,
        "shared shell exposes About");
    check(style.find(".system-tab") != std::string::npos &&
          style.find("min-width: 160dp;") != std::string::npos &&
          style.find("line-height: 32dp;") != std::string::npos &&
          style.find("text-align: center;") != std::string::npos &&
          style.find("background-color: #2F7CCB35;") != std::string::npos,
        "System sections render as aligned segmented buttons");
    check(rml.find("cb-ico-unlock") != std::string::npos &&
          rml.find("cb-ico-shield") != std::string::npos &&
          rml.find("cb-ico-lock") != std::string::npos &&
          rml.find("cb-ico-eye-off") != std::string::npos &&
          rml.find("admin-mode-row") != std::string::npos &&
          style.find("justify-content: center;") != std::string::npos &&
          style.find(".admin-mode-row { display: flex; gap: 7dp; }") !=
              std::string::npos &&
          style.find("flex: 1;") != std::string::npos,
        "server access modes are centered icon-labelled segmented controls");

    const char *routes[] = { "crosshair", "soundpacks", "trailcolors",
        "ammobar", "gameclock", "hudscore", "scoreboard",
        "hudfragmessages" };
    bool all_routes = true;
    for(const char *route : routes)
        all_routes = all_routes &&
            ui.find(std::string("!std::strcmp(name, \"") + route + "\")") !=
                std::string::npos;
    check(all_routes, "all standalone Settings aliases route natively");
    check(settings.find("pending_legacy_anchor") != std::string::npos &&
          settings.find("ScrollIntoView(true)") != std::string::npos &&
          settings_rml.find("id=\"settings-scoreboard\"") != std::string::npos &&
          settings_rml.find("id=\"settings-soundpacks\"") != std::string::npos,
        "legacy Settings routes scroll to their exact native controls");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
