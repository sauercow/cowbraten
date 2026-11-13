#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    int passed = 0, failed = 0;

    void check(bool condition, const std::string &name)
    {
        if(condition) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }

    std::string readfile(const char *path)
    {
        std::ifstream input(path);
        std::ostringstream contents;
        contents << input.rdbuf();
        return contents.str();
    }
}

int main()
{
    const std::string rml = readfile(
        "../data/cowbraten/ui/default/screens/profile.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/profile.rcss");
    const std::string implementation = readfile(
        "../src/cowbraten/ui/rml_profile.cpp");
    const std::string bridge = readfile(
        "../src/cowbraten/ui/sauer_profile_bridge.cpp");
    const std::string client = readfile(
        "../src/fpsgame/client.cpp");
    const std::string detection = readfile(
        "../src/cowbraten/detection.cpp");
    const std::string theme = readfile(
        "../data/cowbraten/ui/themes/modern-metallic/theme.rcss");
    const std::string shell = readfile(
        "../data/cowbraten/ui/default/templates/app_shell.rml");
    const std::string ui = readfile(
        "../src/cowbraten/ui/ui.cpp");
    const std::string commands = readfile(
        "../src/cowbraten/ui/ui_commands.cpp");
    const std::string renderer = readfile(
        "../src/cowbraten/ui/rml_render.cpp");
    const std::string engine = readfile("../src/engine/main.cpp");
    const std::string command = readfile("../src/engine/command.cpp");

    check(!rml.empty() && !implementation.empty() && !bridge.empty(),
        "load Profile contract and implementation");
    check(theme.find("@media (theme: modern-metallic)") != std::string::npos,
        "Modern Metallic uses the native RmlUi theme mechanism");

    const char *tokens[] =
    {
        "--color-background", "--color-surface", "--color-border",
        "--color-text", "--color-text-muted", "--color-primary",
        "--color-secondary", "--color-success", "--color-error"
    };
    bool all_tokens = true;
    for(const char *token : tokens)
        all_tokens = all_tokens && theme.find(token) != std::string::npos;
    check(all_tokens, "Modern Metallic declares the core semantic tokens");

    const std::regex action_pattern(
        "on(?:click|dragstart|drag|dragend|mousescroll)=\"(profile\\.[a-zA-Z0-9_.-]+)\"");
    std::vector<std::string> missing_actions;
    std::size_t action_count = 0;
    for(std::sregex_iterator match(rml.begin(), rml.end(), action_pattern), end;
        match != end; ++match)
    {
        ++action_count;
        const std::string action = (*match)[1].str();
        if(implementation.find("\"" + action + "\"") == std::string::npos)
            missing_actions.push_back(action);
    }
    check(action_count == 12, "audit all twelve Profile actions");
    check(missing_actions.empty(),
        "every Profile RML action has a registered implementation");

    check(rml.find("select_character(character.id)") != std::string::npos &&
          implementation.find("\"select_character\"") != std::string::npos,
        "character selection uses a typed data-model callback");
    check(rml.find("set_preview_scale(ev.value)") != std::string::npos &&
          implementation.find("\"set_preview_scale\"") != std::string::npos,
        "preview scale uses a typed data-model callback");
    check(rml.find("set_model_spoofing(ev.checked)") != std::string::npos &&
          rml.find("select_spoof_model(ev.value)") != std::string::npos &&
          rml.find("data-attrif-disabled=\"!model_spoofing\"") !=
              std::string::npos &&
          implementation.find("\"set_model_spoofing\"") !=
              std::string::npos &&
          implementation.find("\"select_spoof_model\"") !=
              std::string::npos,
        "Profile exposes typed model-spoofing controls");
    check(rml.find("set_name_input(ev.value)") != std::string::npos &&
          rml.find("select_name(preset.value)") !=
              std::string::npos &&
          rml.find("delete_name(preset.value)") !=
              std::string::npos &&
          rml.find("profile-name-popover") !=
              std::string::npos &&
          implementation.find("\"set_name_input\"") != std::string::npos &&
          implementation.find("\"select_name\"") != std::string::npos &&
          implementation.find("\"delete_name\"") != std::string::npos,
        "saved-name menu exposes independent load and remove callbacks");
    check(rml.find("playermodel") == std::string::npos,
        "Profile markup does not expose the legacy ident name");
    check(rml.find("id=\"profile-preview\"") != std::string::npos &&
          rml.find("profile.preview.drag") != std::string::npos &&
          rml.find("profile.preview.scroll") != std::string::npos,
        "3D preview owns rotate and zoom interaction hooks");
    check(shell.find("onclick=\"navigation.profile\"") != std::string::npos,
        "application shell exposes the Profile screen");
    check(ui.find("!std::strcmp(name, \"playermodel\")") != std::string::npos &&
          ui.find("screen = ScreenId::Profile") != std::string::npos &&
          commands.find("ICOMMAND(ui_profile") != std::string::npos,
        "Profile is reachable from legacy and native navigation");
    check(bridge.find("setvarchecked(id, value)") != std::string::npos &&
          bridge.find("game::renderplayerpreview") != std::string::npos,
        "legacy writes and 3D rendering stay behind the bridge");
    check(bridge.find("game::switchname(value)") != std::string::npos &&
          bridge.find("profile_name_1") != std::string::npos &&
          bridge.find("bool deletename(const char *value)") !=
              std::string::npos &&
          bridge.find("retained_count") != std::string::npos &&
          bridge.find("writecfg();") != std::string::npos,
        "saved names persist immediately through the legacy bridge");
    check(bridge.find("\"modelspoofing\"") != std::string::npos &&
          bridge.find("\"spoofplayermodel\"") != std::string::npos &&
          bridge.find("writemodelspoofingenabled") != std::string::npos &&
          bridge.find("writemodelspoofingselection") != std::string::npos &&
          bridge.find("writecfg();") != std::string::npos,
        "model spoofing uses fixed persistent idents behind the bridge");
    check(client.find("VARFP(modelspoofing, 0, 0, 1") !=
              std::string::npos &&
          client.find("VARFP(spoofplayermodel, 0, 0, 4") !=
              std::string::npos &&
          client.find("putint(p, reportedplayermodel());") !=
              std::string::npos &&
          client.find("addmsg(N_SWITCHMODEL, \"ri\", reportedplayermodel());") !=
              std::string::npos &&
          detection.find("addmsg(N_SWITCHMODEL, \"ri\", reportedplayermodel());") !=
              std::string::npos,
        "all outgoing model paths honor the reported model");
    check(commands.find("if(initing == NOT_INITING) writecfg();") !=
              std::string::npos,
        "theme changes persist immediately outside startup");
    check(commands.find("cowbraten-ui.cfg") != std::string::npos &&
          commands.find("profile_name_5") != std::string::npos &&
          commands.find("uitheme %s") != std::string::npos &&
          engine.find("cowui::loadpersistentstate();") !=
              std::string::npos &&
          command.find("cowui::savepersistentstate();") !=
              std::string::npos,
        "names and theme survive config rewrites by legacy clients");
    check(bridge.find("?cowbraten-resource:packages/icons/") != std::string::npos &&
          renderer.find("?cowbraten-resource:") != std::string::npos,
        "character portraits resolve through the engine resource root");
    check(ui.find("if(!text_input_handler.active() &&\n"
                  "           event.type == SDL_KEYDOWN") != std::string::npos &&
          ui.find("sauerinput::openconsolefromkey(event.key.keysym.sym)") !=
              std::string::npos,
        "focused text inputs suppress legacy console shortcuts");
    check(style.find(".profile-identity") != std::string::npos &&
          style.find("overflow: visible;") != std::string::npos &&
          style.find("z-index: 10;") != std::string::npos &&
          style.find(".profile-name-popover") != std::string::npos &&
          style.find("bottom: 38dp;") != std::string::npos &&
          style.find(".profile-name-remove") != std::string::npos &&
          style.find("color: var(--color-error);") != std::string::npos,
        "saved-name popover opens above the preview with a red remove action");
    check(style.find(".profile-spoof-panel") != std::string::npos &&
          style.find(".profile-spoof-select:disabled") !=
              std::string::npos &&
          rml.find("{{ reported_model_name }}") != std::string::npos,
        "Profile distinguishes local and reported model presentation");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
