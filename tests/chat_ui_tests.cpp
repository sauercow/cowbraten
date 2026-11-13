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
        "../data/cowbraten/ui/default/screens/chat.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/chat.rcss");
    const std::string model = readfile(
        "../src/cowbraten/ui/rml_chat.cpp");
    const std::string bridge = readfile(
        "../src/cowbraten/ui/sauer_chat_bridge.cpp");
    const std::string console = readfile("../src/engine/console.cpp");

    check(rml.find("data-model=\"chat\"") != std::string::npos &&
          rml.find("message : console_messages") != std::string::npos &&
          rml.find("message : messages") != std::string::npos &&
          rml.find("data-rml=\"message.rml\"") != std::string::npos,
        "chat overlay binds passive console and mini-console lists");
    check(rml.find("input_before") != std::string::npos &&
          rml.find("chat-composer-caret") != std::string::npos &&
          rml.find("input_after") != std::string::npos,
        "chat composer renders the native cursor position");
    check(style.find("pointer-events: none;") != std::string::npos &&
          style.find(".console-message.is-error") != std::string::npos &&
          style.find(".chat-message.is-team") != std::string::npos,
        "passive overlays are non-modal and style semantic messages");
    check(style.find("width: 100%;") != std::string::npos &&
          style.find("left: 0;") != std::string::npos &&
          style.find("bottom: 0;") != std::string::npos &&
          style.find("padding: 0;") != std::string::npos &&
          style.find("font-size: 18dp;") != std::string::npos,
        "chat composer uses legacy-sized text flush across the bottom edge");
    check(model.find("sauerchat::classifyline") != std::string::npos &&
          model.find("sauerchat::topenabled") != std::string::npos &&
          model.find("sauerchat::minienabled") != std::string::npos &&
          model.find("consoletext::markup") != std::string::npos &&
          model.find("sauerchat::inputmode") != std::string::npos,
        "chat model uses the typed Sauer bridge");
    check(bridge.find("confilter & type") != std::string::npos &&
          bridge.find("miniconfilter & type") != std::string::npos &&
          console.find("cowui::filterconsole(confilter)") != std::string::npos &&
          console.find("fullconfilter") != std::string::npos,
        "RmlUi owns passive feeds without altering full console filtering");
    check(console.find("requestedchat = 1") != std::string::npos &&
          console.find("requestedchat = 2") != std::string::npos,
        "public and team bindings retain distinct native modes");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
