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
        "../data/cowbraten/ui/default/screens/utilities.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/utilities.rcss");
    const std::string model = readfile(
        "../src/cowbraten/ui/rml_utilities.cpp");
    const std::string bridge = readfile(
        "../src/cowbraten/ui/sauer_utility_bridge.cpp");
    const std::string bridge_header = readfile(
        "../src/cowbraten/ui/sauer_utility_bridge.h");
    const std::string ui = readfile("../src/cowbraten/ui/ui.cpp");
    const std::string shell = readfile(
        "../data/cowbraten/ui/default/templates/app_shell.rml");

    check(!rml.empty() && !style.empty() && !model.empty(),
        "load native text utilities contract");
    check(rml.find("data-model=\"utilities\"") != std::string::npos &&
          model.find("CreateDataModel(\"utilities\")") != std::string::npos,
        "Utilities use a typed RmlUi model");
    check(model.find("utilities.notepad.load") != std::string::npos &&
          model.find("utilities.notepad.save") != std::string::npos &&
          model.find("utilities.scratch.execute") != std::string::npos &&
          model.find("utilities.paste.execute") != std::string::npos,
        "all file, buffer, clipboard, and execution actions are whitelisted");
    check(bridge.find("loadfile(filename") != std::string::npos &&
          bridge.find("openutf8file(filename, \"w\")") != std::string::npos &&
          bridge.find("execute(text)") != std::string::npos &&
          bridge.find("SDL_GetClipboardText") != std::string::npos &&
          bridge.find("SDL_SetClipboardText") != std::string::npos,
        "legacy bridge retains native file, execution, and clipboard behavior");
    check(bridge.find("filename[0] == '/'") != std::string::npos &&
          bridge.find("strstr(filename, \"..\")") != std::string::npos &&
          bridge.find("strchr(filename, ':')") != std::string::npos,
        "notepad rejects absolute and parent traversal paths");
    check(bridge_header.find("#include <string>") == std::string::npos &&
          bridge_header.find("TextVisitor") != std::string::npos,
        "engine boundary remains POD and visitor based");
    check(rml.find("data-value=\"notepad_text\"") != std::string::npos &&
          rml.find("data-value=\"scratchpad_text\"") != std::string::npos &&
          rml.find("data-value=\"pastebuffer_text\"") != std::string::npos &&
          rml.find("set_notepad(ev.value)") != std::string::npos &&
          rml.find("set_scratchpad(ev.value)") != std::string::npos &&
          rml.find("set_pastebuffer(ev.value)") != std::string::npos,
        "multiline controls synchronize loaded and edited text through the model");
    check(style.find("font-family: var(--font-interface)") != std::string::npos &&
          style.find("font-family: mono") == std::string::npos,
        "editors use an installed font face");

    const char *routes[] = { "notepad", "scratchpad", "pastebuffer" };
    bool all_routes = true;
    for(const char *route : routes)
        all_routes = all_routes &&
            ui.find(std::string("!std::strcmp(name, \"") + route + "\")") !=
                std::string::npos;
    check(all_routes &&
          ui.find("utilities_model.activate(0)") != std::string::npos &&
          ui.find("utilities_model.activate(1)") != std::string::npos &&
          ui.find("utilities_model.activate(2)") != std::string::npos,
        "all legacy utility routes select their exact native tab");
    check(shell.find("onclick=\"navigation.utilities\"") != std::string::npos,
        "shared shell exposes text utilities");
    check(style.find(".utilities-tabs button") != std::string::npos &&
          style.find("min-width: 136dp;") != std::string::npos &&
          style.find("line-height: 32dp;") != std::string::npos &&
          style.find("text-align: center;") != std::string::npos &&
          style.find("background-color: #2F7CCB35;") != std::string::npos,
        "utility sections render as aligned segmented buttons");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
