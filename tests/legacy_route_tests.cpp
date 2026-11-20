#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>

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

    void check(bool condition, const std::string &message)
    {
        std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
        condition ? ++passed : ++failed;
    }

    void collectstaticguis(const std::string &source, std::set<std::string> &names)
    {
        const std::regex declaration(
            R"re(\bnewgui\s+(?:"([^"]+)"|([A-Za-z_][A-Za-z0-9_]*)))re");
        for(std::sregex_iterator it(source.begin(), source.end(), declaration), end;
            it != end; ++it)
        {
            const std::string name = (*it)[1].matched ? (*it)[1].str() : (*it)[2].str();
            if(!name.empty()) names.insert(name);
        }
    }

    bool hasroute(const std::string &ui, const std::string &name)
    {
        return ui.find("strcmp(name, \"" + name + "\")") != std::string::npos;
    }
}

int main()
{
    const std::string menus = readfile("../data/cowbraten/menus.cfg");
    const std::string master = readfile("../data/cowbraten/master.cfg");
    const std::string ui = readfile("../src/cowbraten/ui/ui.cpp");
    const std::string engine_menus = readfile("../src/engine/menus.cpp");
    const std::string snapshot = readfile(
        "../src/cowbraten/ui/legacy_gui_snapshot.h");

    check(!menus.empty() && !master.empty() && !ui.empty(),
        "load legacy declarations and native route table");

    std::set<std::string> names;
    collectstaticguis(menus, names);
    collectstaticguis(master, names);
    check(names.size() >= 35, "discover the built-in static legacy GUI catalog");

    bool all_routed = true;
    for(const std::string &name : names)
    {
        if(hasroute(ui, name)) continue;
        all_routed = false;
        std::cout << "UNROUTED  " << name << '\n';
    }
    check(all_routed, "every built-in static legacy GUI has an RmlUi route");

    const char *generated_editor_routes[] = {
        "light", "spotlight", "playerstart", "teleport", "teledest",
        "monster", "mapmodel", "envmap", "jumppad", "sound", "particles",
        "shells", "bullets", "rockets", "riflerounds", "grenades",
        "cartridges", "health", "healthboost", "greenarmour", "yellowarmour",
        "quaddamage", "carrot", "base", "respawnpoint", "box", "barrel",
        "platform", "elevator", "flag"
    };
    bool generated_routed = true;
    for(const char *name : generated_editor_routes)
        generated_routed = generated_routed && hasroute(ui, name);
    check(generated_routed,
        "every generated entity editor GUI has an RmlUi route");

    check(hasroute(ui, "autoignore"),
        "known Autoexec Auto Ignore menu routes to native Settings");
    check(hasroute(ui, "aliases"),
        "known Autoexec alias selector routes to native Profile");
    check(snapshot.find("struct LegacyGuiSink") != std::string::npos &&
          snapshot.find("opaque") != std::string::npos &&
          snapshot.find("activation indices") != std::string::npos &&
          engine_menus.find("capturelegacygui") != std::string::npos &&
          engine_menus.find("LegacyGuiControlType::Checkbox") != std::string::npos &&
          engine_menus.find("LegacyGuiControlType::Slider") != std::string::npos &&
          engine_menus.find("LegacyGuiControlType::Field") != std::string::npos,
        "generic newgui capture exposes typed controls and opaque actions");
    check(hasroute(ui, "ss"),
        "known Autoexec screen preset menu routes to native Settings");

    check(ui.find("else\n            return false;") != std::string::npos,
        "unknown third-party GUI names retain an explicit compatibility handoff");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
