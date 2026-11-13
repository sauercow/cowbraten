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
        "../data/cowbraten/ui/default/screens/editor.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/editor.rcss");
    const std::string model = readfile(
        "../src/cowbraten/ui/rml_editor.cpp");
    const std::string bridge = readfile(
        "../src/cowbraten/ui/sauer_editor_bridge.cpp");
    const std::string bridge_header = readfile(
        "../src/cowbraten/ui/sauer_editor_bridge.h");
    const std::string ui = readfile("../src/cowbraten/ui/ui.cpp");
    const std::string shell = readfile(
        "../data/cowbraten/ui/default/templates/app_shell.rml");

    check(!rml.empty() && !style.empty() && !model.empty(),
        "load native editor screen contract");
    check(rml.find("data-model=\"editor\"") != std::string::npos &&
          model.find("CreateDataModel(\"editor\")") != std::string::npos,
        "Editor uses one typed RmlUi model");
    check(rml.find("editor.section.overview") != std::string::npos &&
          rml.find("editor.section.entities") != std::string::npos &&
          rml.find("editor.section.lighting") != std::string::npos &&
          rml.find("editor.section.materials") != std::string::npos &&
          rml.find("editor.section.map") != std::string::npos &&
          rml.find("editor.section.selection") != std::string::npos,
        "Editor exposes geometry, entity, lighting, material, map, and selection tools");
    check(rml.find("create_entity(item.id)") != std::string::npos &&
          rml.find("create_mapmodel(entry.id)") != std::string::npos &&
          rml.find("editor.light.create") != std::string::npos &&
          rml.find("editor.trigger.create") != std::string::npos &&
          rml.find("editor.selection.apply") != std::string::npos,
        "dynamic entity tools remain typed and range checked");
    check(bridge.find("action_commands[]") != std::string::npos &&
          bridge.find("entity_commands[]") != std::string::npos &&
          bridge.find("material_commands[]") != std::string::npos &&
          bridge.find("postfx_commands[]") != std::string::npos &&
          bridge.find("execute(action_commands[id])") != std::string::npos,
        "legacy execution is confined to fixed command tables");
    check(rml.find("execute(") == std::string::npos &&
          rml.find("ident") == std::string::npos &&
          rml.find("CubeScript") == std::string::npos,
        "RML cannot submit commands or ident names");
    check(bridge.find("name[0] == '/'") != std::string::npos &&
          bridge.find("std::strstr(name, \"..\")") != std::string::npos &&
          bridge.find("std::isalnum") != std::string::npos,
        "map save and load names reject unsafe paths");
    check(bridge.find("mapmodelname(i)") != std::string::npos &&
          bridge.find("MapModelVisitor") != std::string::npos &&
          model.find("editor::readmapmodels") != std::string::npos,
        "mapmodel catalog comes from the configured engine list");
    check(bridge.find("entities::getents()") != std::string::npos &&
          bridge.find("entset(type") != std::string::npos,
        "selected entity attributes use native selection and undo paths");
    check(bridge_header.find("#include <string>") == std::string::npos &&
          bridge_header.find("MapModelVisitor") != std::string::npos,
        "engine editor boundary remains POD and visitor based");

    const char *routes[] = { "editing", "maplighting", "materials", "triggers",
        "trigger2", "trigger3", "newlight", "newteleport", "mapmodels",
        "monsters", "postfx", "brushes", "quickedit", "seltexinfo",
        "light", "mapmodel", "playerstart", "envmap", "particles", "sound",
        "spotlight", "shells", "quaddamage", "teleport", "teledest",
        "monster", "carrot", "jumppad", "base", "respawnpoint", "box",
        "barrel", "platform", "elevator", "flag" };
    bool all_routes = true;
    for(const char *route : routes)
        all_routes = all_routes &&
            ui.find(std::string("!std::strcmp(name, \"") + route + "\")") !=
                std::string::npos;
    check(all_routes, "all static and generated legacy editor GUIs route natively");
    check(ui.find("editor_model.activate(0)") != std::string::npos &&
          ui.find("editor_model.activate(1)") != std::string::npos &&
          ui.find("editor_model.activate(2)") != std::string::npos &&
          ui.find("editor_model.activate(3)") != std::string::npos &&
          ui.find("editor_model.activate(5)") != std::string::npos,
        "legacy routes select their matching native editor section");
    check(shell.find("onclick=\"navigation.editor\"") != std::string::npos &&
          shell.find("cb-ico-hammer") != std::string::npos,
        "shared shell exposes the native editor");
    check(style.find("slidertrack") != std::string::npos &&
          style.find("editor-card-full") != std::string::npos,
        "editor ranges and full-width catalogs have explicit layout");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
