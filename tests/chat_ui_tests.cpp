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
    const std::string text_bridge = readfile(
        "../src/cowbraten/ui/sauer_text_bridge.cpp");
    const std::string console = readfile("../src/engine/console.cpp");

    check(rml.find("data-model=\"chat\"") != std::string::npos &&
          rml.find("class=\"console-message-repeat\" data-for=\"message : console_messages\"") != std::string::npos &&
          rml.find("class=\"chat-message-repeat\" data-for=\"message : messages\"") != std::string::npos &&
          rml.find("data-rml=\"message.rml\"") != std::string::npos,
        "chat overlay binds passive console and mini-console lists");
    check(rml.find("class=\"console-message\" data-for=") == std::string::npos &&
          rml.find("class=\"chat-message\" data-for=") == std::string::npos &&
          style.find(".console-message-repeat,") != std::string::npos &&
          style.find(".chat-message-repeat") != std::string::npos,
        "repeat nodes isolate structural updates from indexed message bindings");
    check(rml.find("input_before") != std::string::npos &&
          rml.find("chat-composer-caret") != std::string::npos &&
          rml.find("input_after") != std::string::npos,
        "chat composer renders the native cursor position");
    check(style.find("pointer-events: none;") != std::string::npos &&
          style.find(".console-message.is-error") != std::string::npos &&
          style.find(".chat-message.is-team") != std::string::npos,
        "passive overlays are non-modal and style semantic messages");
    check(rml.find("data-class-is-minimap-clipped=\"console_minimap_clipped\"") !=
              std::string::npos &&
          style.find(".console-feed.is-minimap-clipped") != std::string::npos &&
          style.find("right: 30vh;") != std::string::npos &&
          style.find("width: 44%;") == std::string::npos &&
          model.find("sauerchat::topfeedminimapclipped()") != std::string::npos &&
          bridge.find("game::clipconsole(100, 100) > 0") != std::string::npos,
        "top feed uses the legacy full width with minimap clearance");
    check(style.find("width: 100%;") != std::string::npos &&
          style.find("left: 0;") != std::string::npos &&
          style.find("bottom: 0;") != std::string::npos &&
          style.find("padding: 0;") != std::string::npos &&
          style.find("font-size: 1.125rem;") != std::string::npos,
        "chat composer uses legacy-sized text flush across the bottom edge");
    check(model.find("sauerchat::classifyline") != std::string::npos &&
          model.find("sauerchat::topenabled") != std::string::npos &&
          model.find("sauerchat::minienabled") != std::string::npos &&
          model.find("consoletext::markup") != std::string::npos &&
          model.find("sauerchat::inputmode") != std::string::npos,
        "chat model uses the typed Sauer bridge");
    check(model.find("sauertext::utf8(text, position)") != std::string::npos &&
          model.find("length - position") != std::string::npos &&
          text_bridge.find("encodeutf8") != std::string::npos &&
          text_bridge.find("result.push_back(text[segment_end++])") !=
              std::string::npos,
        "chat composer converts legacy text to UTF-8 around the cursor");
    check(bridge.find("confilter & type") != std::string::npos &&
          bridge.find("miniconfilter & type") != std::string::npos &&
          console.find("cowui::filterconsole(confilter)") != std::string::npos &&
          console.find("fullconfilter") != std::string::npos,
        "RmlUi owns passive feeds without altering full console filtering");
    check(console.find("requestedchat = 1") != std::string::npos &&
          console.find("requestedchat = 2") != std::string::npos,
        "public and team bindings retain distinct native modes");
    check(model.find("!document_visible && !input_active") !=
              std::string::npos &&
          model.find("console_messages.empty()") != std::string::npos &&
          model.find("messages.empty()") != std::string::npos,
        "idle hidden chat avoids string and collection work");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
