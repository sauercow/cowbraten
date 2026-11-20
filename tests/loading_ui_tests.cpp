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

    void check(bool condition, const char *message)
    {
        std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
        condition ? ++passed : ++failed;
    }
}

int main()
{
    const std::string rml = readfile(
        "../data/cowbraten/ui/default/screens/loading.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/loading.rcss");
    const std::string model = readfile("../src/cowbraten/ui/rml_loading.h");
    const std::string ui = readfile("../src/cowbraten/ui/ui.cpp");
    const std::string engine = readfile("../src/engine/main.cpp");

    check(rml.find("cow-loading-progress-fill") != std::string::npos &&
          rml.find("cow-loading-mapshot") != std::string::npos &&
          rml.find("cow-loading-map-info") != std::string::npos,
        "loading document exposes progress and map presentation");
    check(style.find(".cow-loading-root") != std::string::npos &&
          style.find(".cow-loading-root.cow-loading-compact") != std::string::npos &&
          style.find(".cow-loading-indeterminate") != std::string::npos,
        "loading presentation supports full, compact and indeterminate states");
    check(style.find("linear-gradient") == std::string::npos,
        "loading presentation stays within the engine renderer feature set");
    check(model.find("StringUtilities::EncodeRml") != std::string::npos &&
          model.find("SetAttribute(\"src\", mapshot)") != std::string::npos,
        "loading text is escaped and map previews use package paths");
    check(model.find("document->PullToFront()") != std::string::npos &&
          model.find("ctx->Update(false)") != std::string::npos &&
          model.find("renderer.beginframe()") != std::string::npos &&
          model.find("renderer.endframe()") != std::string::npos,
        "blocking loads update and render RmlUi synchronously");
    check(ui.find("loading_model.init(context)") != std::string::npos &&
          ui.find("renderloadingbackground") != std::string::npos &&
          ui.find("renderloadingprogress") != std::string::npos,
        "UI runtime owns the cached loading document");
    check(engine.find("cowui::renderloadingbackground") != std::string::npos &&
          engine.find("cowui::renderloadingprogress") != std::string::npos &&
          engine.find("if(!tex &&") != std::string::npos,
        "engine loading routes file-backed presentation through RmlUi");
    check(engine.find("clientkeepalive()") != std::string::npos &&
          engine.find("SDL_PumpEvents()") != std::string::npos &&
          engine.find("swapbuffers(false)") != std::string::npos,
        "native loading preserves keepalive, event pumping and buffer swaps");
    check(engine.find("raw engine GL textures") != std::string::npos &&
          engine.find("settexture(\"data/loading_frame.png\"") != std::string::npos,
        "raw generated textures retain the explicit OpenGL fallback");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
