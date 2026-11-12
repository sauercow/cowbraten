#include "engine.h"
#include "ui.h"

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
ICOMMAND(ui_close, "", (), cowui::close());
