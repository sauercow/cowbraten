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
        std::ostringstream output;
        output << input.rdbuf();
        return output.str();
    }

    void check(bool condition, const char *name)
    {
        if(condition) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }
}

int main()
{
    const std::string render = readfile("../src/fpsgame/render.cpp");
    const std::string demohd = readfile("../src/cowbraten/demohd.cpp");
    const std::string engine = readfile("../src/engine/rendergl.cpp");
    const std::string model = readfile("../src/engine/rendermodel.cpp");
    const std::string shader = readfile("../data/cowbraten/demoxray.cfg");
    const std::string screen = readfile(
        "../data/cowbraten/ui/default/screens/demo_player.rml");
    const std::string base_style = readfile(
        "../data/cowbraten/ui/default/styles/base.rcss");
    const std::string player_style = readfile(
        "../data/cowbraten/ui/default/styles/demo_player.rcss");
    const std::string player = readfile(
        "../src/cowbraten/ui/rml_demo_player.cpp");

    check(render.find("return connected && demoplayback && !remote;") !=
              std::string::npos,
        "X-RAY and hitboxes require a locally played demo");
    check(render.find("if(!demoxrayallowed())") != std::string::npos &&
          demohd.find("if(!demoxrayallowed())") != std::string::npos &&
          demohd.find("return demohitbox && demoxrayallowed();") !=
              std::string::npos,
        "both toggles fail closed outside the demo-only guard");
    check(engine.find("demoxraysnapdepth();") != std::string::npos &&
          engine.find("demoxraydraw();") != std::string::npos &&
          model.find("if(demoxraypass)") != std::string::npos &&
          shader.find("demoxraymodel") != std::string::npos &&
          shader.find("demoxrayedge") != std::string::npos,
        "X-RAY renders hidden model outlines from the scene depth");
    check(demohd.find("static const int sides = 24") != std::string::npos &&
          demohd.find("d->o.z - d->eyeheight") != std::string::npos &&
          demohd.find("d->o.z + d->aboveeye") != std::string::npos &&
          demohd.find("d->radius") != std::string::npos,
        "hitboxes use the real gameplay cylinder dimensions");
    check(screen.find("onclick=\"demo-player.xray\"") != std::string::npos &&
          screen.find("onclick=\"demo-player.hitbox\"") != std::string::npos &&
          screen.find("data-class-is-active=\"xray_enabled\"") !=
              std::string::npos &&
          player.find("sauerdemo::toggle_xray()") != std::string::npos &&
          player.find("sauerdemo::toggle_hitbox()") != std::string::npos,
        "demo playback bar exposes stateful X-RAY and Hitbox controls");
    check(render.find("demoxrayreset();") == std::string::npos &&
          demohd.find("demoxrayreset();") != std::string::npos,
        "a new or closed demo resets visualization state");
    check(base_style.find("--font-interface: Lato;") != std::string::npos &&
          base_style.find("--font-size-control:") != std::string::npos &&
          player_style.find("--demo-control-height:") != std::string::npos &&
          player_style.find("font-family: var(--font-interface);") !=
              std::string::npos &&
          player_style.find("font-size: var(--demo-control-size);") !=
              std::string::npos,
        "demo player consumes global typography and local density tokens");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
