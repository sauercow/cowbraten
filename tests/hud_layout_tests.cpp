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
    const std::string hud = readfile("../src/fpsgame/fps.cpp");
    const std::string game = readfile("../src/fpsgame/game.h");
    const std::string ctf = readfile("../src/fpsgame/ctf.h");
    const std::string collect = readfile("../src/fpsgame/collect.h");
    const std::string snapshot = readfile(
        "../src/cowbraten/ui/gameplay_hud_snapshot.h");
    const std::string native_hud = readfile(
        "../src/cowbraten/ui/rml_gameplay_hud.h");
    const std::string native_rml = readfile(
        "../data/cowbraten/ui/default/screens/gameplay_hud.rml");

    check(hud.find("statusdockcenter") != std::string::npos &&
          hud.find("0.46f*virtual_width") != std::string::npos &&
          hud.find("clear_of_chat") != std::string::npos,
        "status dock stays clear of the bottom-left chat width");
    check(hud.find("drawstatusvalue(HICON_HEALTH") != std::string::npos &&
          hud.find("HICON_BLUE_ARMOUR+d->armourtype") != std::string::npos &&
          hud.find("HICON_FIST+d->gunselect") != std::string::npos &&
          hud.find("STATUS_BLOCK_WIDTH = 300.0f") != std::string::npos,
        "health armour and active ammo share the stable primary row");
    check(hud.find("status.secondary_up[count++] = gun") != std::string::npos &&
          hud.find("status.secondary_down[count++] = gun") != std::string::npos &&
          hud.find("status.secondary_cycle[count++] = gun") != std::string::npos &&
          hud.find("Keep the legacy secondary ammo HUD") == std::string::npos &&
          snapshot.find("show_secondary_ammo") != std::string::npos &&
          native_hud.find("setweapon(secondary_cycle") != std::string::npos &&
          native_rml.find("cow-status-secondary-cycle") != std::string::npos,
        "tiny available-weapon icons migrate into the native status dock");
    check(hud.find("status.show_ammo_bar") != std::string::npos &&
          hud.find("if(ammobar && !modern_status) drawammobar") != std::string::npos &&
          snapshot.find("AmmoBarSlots = 6") != std::string::npos &&
          native_hud.find("updateammobar()") != std::string::npos &&
          native_rml.find("cow-ammo-bar-root") != std::string::npos,
        "optional full ammo bar uses the native overlay with GL fallback");
    check(game.find("statushudnextx") != std::string::npos &&
          ctf.find("statushudnextx(w, h, d)") != std::string::npos &&
          collect.find("statushudnextx(w, h, d)") != std::string::npos,
        "mode-specific flag and token indicators extend the same dock");
    check(snapshot.find("demo_recording") != std::string::npos &&
          native_hud.find("cow-demo-recording") != std::string::npos &&
          native_rml.find("cow-demo-recording") != std::string::npos &&
          hud.find("if(!modern_status) drawdemorecording") != std::string::npos,
        "demo recording indicator uses RmlUi with an OpenGL failure fallback");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
