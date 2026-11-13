#include "engine.h"
#include "ui.h"
#include "ui_persistence.h"

SVARFP(uitheme, "default", {
    cowui::settheme(uitheme);
    if(initing == NOT_INITING) writecfg();
});
SVARP(profile_name_1, "");
SVARP(profile_name_2, "");
SVARP(profile_name_3, "");
SVARP(profile_name_4, "");
SVARP(profile_name_5, "");

namespace cowui
{
    static const char *persistentstatefile = "cowbraten-ui.cfg";

    bool loadpersistentstate()
    {
        return execfile(persistentstatefile, false);
    }

    bool savepersistentstate()
    {
        stream *f = openutf8file(persistentstatefile, "w");
        if(!f)
        {
            conoutf(CON_ERROR, "could not save Cowbraten UI state");
            return false;
        }

        f->printf("// Cowbraten-only UI state; legacy clients do not rewrite this file.\n");
        f->printf("uitheme %s\n", escapestring(uitheme));
        f->printf("profile_name_1 %s\n", escapestring(profile_name_1));
        f->printf("profile_name_2 %s\n", escapestring(profile_name_2));
        f->printf("profile_name_3 %s\n", escapestring(profile_name_3));
        f->printf("profile_name_4 %s\n", escapestring(profile_name_4));
        f->printf("profile_name_5 %s\n", escapestring(profile_name_5));
        delete f;
        return true;
    }
}

ICOMMAND(ui_test, "", (), cowui::showtest());
ICOMMAND(ui_main, "", (), cowui::showmain());
ICOMMAND(ui_servers, "", (), cowui::showservers());
ICOMMAND(ui_demos, "", (), cowui::showdemos());
ICOMMAND(ui_demos_recording, "", (), cowui::showdemosrecording());
ICOMMAND(ui_demos_playback, "", (), cowui::showdemosplayback());
ICOMMAND(ui_demos_trash, "", (), cowui::showdemostrash());
ICOMMAND(ui_demos_duels, "", (), cowui::showdemosduels());
ICOMMAND(ui_demos_filter, "s", (char *value), cowui::filterdemos(value));
ICOMMAND(ui_demos_selectfirst, "", (), cowui::selectfirstdemo());
ICOMMAND(ui_demos_playselected, "", (), intret(cowui::playselecteddemo() ? 1 : 0));
ICOMMAND(ui_settings, "", (), cowui::showsettings());
ICOMMAND(ui_profile, "", (), cowui::showprofile());
ICOMMAND(ui_games, "", (), cowui::showgames());
ICOMMAND(ui_admin, "", (), cowui::showadmin());
ICOMMAND(ui_about, "", (), cowui::showabout());
ICOMMAND(ui_utilities, "", (), cowui::showutilities());
ICOMMAND(ui_scratchpad, "", (), cowui::showscratchpad());
ICOMMAND(ui_pastebuffer, "", (), cowui::showpastebuffer());
ICOMMAND(ui_close, "", (), cowui::close());
