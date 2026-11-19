// Standalone test; build with: c++ -std=c++17 tests/test_frag_feedback_weapon_icons.cpp -o /tmp/frag-icons-test
#include <cassert>
#include <cstring>
#include "../src/cowbraten/ui/frag_feedback_weapon_icons.h"

int main()
{
    const char *expected[] = {"fist.png", "shotgun.png", "chaingun.png",
        "rocket_launcher.png", "rifle.png", "grenade_launcher.png",
        "pistol.png", "fireball.png", "iceball.png", "slimeball.png",
        "bite.png", "barrel.png"};
    for(int i = 0; i < 12; ++i)
        assert(std::strcmp(cowui::fragfeedbackweaponicon(i, false), expected[i]) == 0);
    // Legacy suicide slot overlaps GUN_BARREL; the missing attacker resolves it.
    assert(std::strcmp(cowui::fragfeedbackweaponicon(11, true), "suicide.png") == 0);
    assert(std::strcmp(cowui::fragfeedbackweaponicon(-1, false), "unknown.png") == 0);
    assert(std::strcmp(cowui::fragfeedbackweaponicon(99, false), "unknown.png") == 0);
}
