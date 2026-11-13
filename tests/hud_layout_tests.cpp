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

    check(hud.find("statusdockcenter") != std::string::npos &&
          hud.find("0.46f*virtual_width") != std::string::npos &&
          hud.find("clear_of_chat") != std::string::npos,
        "status dock stays clear of the bottom-left chat width");
    check(hud.find("drawstatusvalue(HICON_HEALTH") != std::string::npos &&
          hud.find("HICON_BLUE_ARMOUR+d->armourtype") != std::string::npos &&
          hud.find("HICON_FIST+d->gunselect") != std::string::npos &&
          hud.find("STATUS_BLOCK_WIDTH = 300.0f") != std::string::npos,
        "health armour and active ammo share the stable primary row");
    check(hud.find("ammohudup[i]") != std::string::npos &&
          hud.find("ammohuddown[i]") != std::string::npos &&
          hud.find("ammohudcycle[(i + offset)%7]") != std::string::npos &&
          hud.find("if(ammohud && !ammobar)") != std::string::npos,
        "tiny available-weapon icons retain the configurable Cowbraten layout");
    check(game.find("statushudnextx") != std::string::npos &&
          ctf.find("statushudnextx(w, h, d)") != std::string::npos &&
          collect.find("statushudnextx(w, h, d)") != std::string::npos,
        "mode-specific flag and token indicators extend the same dock");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
