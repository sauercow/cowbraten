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
    const std::string engine_main = readfile("../src/engine/main.cpp");
    const std::string embedded = readfile(
        "../src/cowbraten/embedded_cfgs.cpp");
    const std::string bootstrap = readfile(
        "../data/cowbraten/ui-bootstrap.cfg");
    const std::string snapshot = readfile(
        "../src/cowbraten/ui/legacy_gui_snapshot.h");
    const std::string adapter = readfile(
        "../src/cowbraten/ui/rml_legacy_gui.cpp");
    const std::string adapter_rml = readfile(
        "../data/cowbraten/ui/default/screens/legacy_gui.rml");

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

    check(ui.find("legacy_gui_model.activate(name)") != std::string::npos &&
          ui.find("show(ScreenId::LegacyGui)") != std::string::npos,
        "configurable newgui names route through the generic RmlUi adapter");
    check(snapshot.find("struct LegacyGuiSink") != std::string::npos &&
          snapshot.find("opaque") != std::string::npos &&
          snapshot.find("activation indices") != std::string::npos &&
          engine_menus.find("capturelegacygui") != std::string::npos &&
          engine_menus.find("LegacyGuiControlType::Checkbox") != std::string::npos &&
          engine_menus.find("LegacyGuiControlType::Slider") != std::string::npos &&
          engine_menus.find("LegacyGuiControlType::Field") != std::string::npos &&
          engine_menus.find("LegacyGuiControlType::Editor") != std::string::npos &&
          engine_menus.find("LegacyGuiControlType::Progress") != std::string::npos &&
          engine_menus.find("LegacyGuiControlType::Preview") != std::string::npos &&
          engine_menus.find("capturedactiontype::ListSlider") != std::string::npos &&
          adapter.find("activatelegacygui") != std::string::npos &&
          adapter_rml.find("data-for=\"control : controls\"") != std::string::npos &&
          adapter_rml.find("control.display_value") != std::string::npos,
        "generic newgui capture exposes typed controls and opaque actions");
    check(adapter.find("renderpreviews") != std::string::npos &&
          adapter.find("selecttab") != std::string::npos &&
          adapter_rml.find("control.editor") != std::string::npos &&
          adapter_rml.find("control.progress") != std::string::npos &&
          adapter_rml.find("control.preview") != std::string::npos &&
          engine_menus.find("openlegacygui") != std::string::npos &&
          engine_menus.find("releaselegacygui") != std::string::npos,
        "configurable menus preserve editors, progress, previews, tabs, and lifecycle");
    check(!hasroute(ui, "aliases") && !hasroute(ui, "autoignore") &&
          !hasroute(ui, "ss"),
        "Autoexec menus use their actual declarations instead of named shortcuts");

    check(ui.find("if(!legacy_gui_model.activate(name)) return false;") !=
              std::string::npos,
        "unknown third-party GUI names retain an explicit compatibility handoff");
    check(engine_menus.find("LegacyGuiControlType::Unsupported") !=
              std::string::npos &&
          adapter.find("result && capture_complete") != std::string::npos,
        "partially supported menus fall back whole instead of losing controls");
    check(ui.find("legacy_gui_underlay") != std::string::npos &&
          ui.find("PullToFront()") != std::string::npos &&
          ui.find("Rml::ModalFlag::Modal") != std::string::npos,
        "compact menus are modal overlays above the active RmlUi screen");
    check(engine_menus.find("routeapplydialog") != std::string::npos &&
          engine_menus.find("__cow_apply_changes") != std::string::npos &&
          engine_menus.find("cowapplychanges") != std::string::npos &&
          ui.find("routeapplydialog();") != std::string::npos,
        "engine apply confirmation routes through the RmlUi compact adapter");
    check(engine_main.find("event.key.keysym.sym >= SDLK_F1") !=
              std::string::npos &&
          engine_main.find("cowui::receivesinput() && !function_bind") !=
              std::string::npos,
        "unhandled function-key binds can open compact menus above native UI");
    check(engine_main.find("game::loadlegacymenustate();") !=
              std::string::npos &&
          engine_main.find("if(game::loadlegacymenus)") !=
              std::string::npos &&
          embedded.find("loadlegacymenus, 0, 0, 1") != std::string::npos &&
          embedded.find("if(!loadlegacymenus) return;") !=
              std::string::npos,
        "startup switch bypasses both legacy menu files before config loading");
    check(bootstrap.find("newgui") == std::string::npos &&
          bootstrap.find("togglemainmenu") != std::string::npos &&
          bootstrap.find("bind ESCAPE") != std::string::npos,
        "non-legacy startup retains the essential menu binding without GUI definitions");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
