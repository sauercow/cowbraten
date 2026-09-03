#include "engine.h"
#include "ui.h"

ICOMMAND(ui_test, "", (), cowui::showtest());
ICOMMAND(ui_main, "", (), cowui::showmain());
ICOMMAND(ui_servers, "", (), cowui::showservers());
ICOMMAND(ui_close, "", (), cowui::close());
