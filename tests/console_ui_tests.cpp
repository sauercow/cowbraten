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
        "../data/cowbraten/ui/default/screens/console.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/console.rcss");
    const std::string model = readfile(
        "../src/cowbraten/ui/rml_console.cpp");
    const std::string bridge = readfile(
        "../src/cowbraten/ui/sauer_console_bridge.cpp");
    const std::string engine = readfile("../src/engine/console.cpp");
    const std::string renderer = readfile("../src/engine/rendergl.cpp");
    const std::string ui = readfile("../src/cowbraten/ui/ui.cpp");
    const std::string engine_header = readfile("../src/engine/engine.h");

    check(rml.find("data-model=\"console\"") != std::string::npos &&
          rml.find("line : lines") != std::string::npos &&
          rml.find("data-rml=\"line.rml\"") != std::string::npos,
        "console overlay binds typed scrollback rows");
    check(rml.find("input_before") != std::string::npos &&
          rml.find("console-command-caret") != std::string::npos &&
          rml.find("input_after") != std::string::npos,
        "console renders the native editor and cursor");
    check(style.find(".console-line.is-warning") != std::string::npos &&
          style.find(".console-line.is-error") != std::string::npos &&
          style.find(".console-line.is-team") != std::string::npos,
        "console styles semantic line types");
    check(style.find(".console-line") != std::string::npos &&
          style.find("font-size: 16dp;") != std::string::npos &&
          style.find("font-size: 18dp;") != std::string::npos,
        "console scrollback and editor match legacy-readable sizing");
    check(style.find(".console-line:last-child") != std::string::npos &&
          style.find("margin-bottom: 18dp;") != std::string::npos &&
          style.find("flex: none;") != std::string::npos,
        "console reserves space between the last row and footer legend");
    check(model.find("sauerconsole::allowed") != std::string::npos &&
          model.find("sauerconsole::skip") != std::string::npos &&
          model.find("sauerconsole::filter") != std::string::npos &&
          model.find("sauerconsole::maxlines") != std::string::npos &&
          model.find("GetDensityIndependentPixelRatio") != std::string::npos &&
          model.find("end - visible_rows") != std::string::npos &&
          model.find("sauerconsole::tagwindow") != std::string::npos &&
          model.find("sauerconsole::sametag") != std::string::npos,
        "console preserves filters, scrolling, viewport rows, and tagged replacement");
    check(engine.find("cowui::clearconsole();") != std::string::npos &&
          ui.find("console_model.pushline") != std::string::npos,
        "engine buffer mutations stay synchronized with the overlay");
    check(renderer.find("!cowui::fullconsolevisible()") != std::string::npos &&
          bridge.find("commandlinetext()") != std::string::npos,
        "RmlUi replaces presentation while retaining native command input");
    check(ui.find("event.type == SDL_MOUSEWHEEL") != std::string::npos &&
          ui.find("console_model.scroll(event.wheel.y * 3)") != std::string::npos &&
          bridge.find("scrollconsolelines(lines)") != std::string::npos &&
          engine_header.find("scrollconsolelines(int lines)") != std::string::npos,
        "full console routes mouse wheel through native conskip");
    check(ui.find("!legacy_console_presentation &&\n"
                  "           active_screen->id == ScreenId::Profile") !=
              std::string::npos,
        "full console suppresses auxiliary profile preview rendering");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
