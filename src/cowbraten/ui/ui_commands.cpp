#include "engine.h"
#include "ui.h"
#include "ui_persistence.h"

SVARFP(uitheme, "default", {
    cowui::settheme(uitheme);
    if(initing == NOT_INITING) writecfg();
});
VARFP(uiscale, 75, 100, 150, cowui::interfacepreferenceschanged());
VARFP(uitextscale, 75, 100, 150, cowui::interfacepreferenceschanged());
SVARP(profile_name_1, "");
SVARP(profile_name_2, "");
SVARP(profile_name_3, "");
SVARP(profile_name_4, "");
SVARP(profile_name_5, "");
SVARP(serverfavorites, "");

namespace cowui
{
    static const char *persistentstatefile = "cowbraten-ui.cfg";

    namespace
    {
        bool validfavoriteid(const char *id)
        {
            return id && id[0] &&
                strlen(id) < 320 &&
                !strchr(id, '\n') &&
                !strchr(id, '\r');
        }

        void readserverfavorites(vector<char *> &result)
        {
            const char *begin = serverfavorites;

            while(begin && *begin)
            {
                const char *end = strchr(begin, '\n');
                if(!end)
                    end = begin + strlen(begin);

                if(end != begin)
                    result.add(newstring(begin, size_t(end - begin)));

                begin = *end ? end + 1 : end;
            }

            result.sort();
            for(int i = result.length() - 1; i > 0; --i)
            {
                if(strcmp(result[i - 1], result[i]))
                    continue;
                delete[] result.remove(i);
            }
        }
    }

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
        f->printf("uiscale %d\n", uiscale);
        f->printf("uitextscale %d\n", uitextscale);
        f->printf("profile_name_1 %s\n", escapestring(profile_name_1));
        f->printf("profile_name_2 %s\n", escapestring(profile_name_2));
        f->printf("profile_name_3 %s\n", escapestring(profile_name_3));
        f->printf("profile_name_4 %s\n", escapestring(profile_name_4));
        f->printf("profile_name_5 %s\n", escapestring(profile_name_5));
        f->printf("serverfavorites %s\n", escapestring(serverfavorites));
        delete f;
        return true;
    }

    bool isserverfavorite(const char *id)
    {
        if(!validfavoriteid(id))
            return false;

        vector<char *> favorites;
        readserverfavorites(favorites);

        bool result = false;
        loopv(favorites)
        {
            if(!strcmp(favorites[i], id))
            {
                result = true;
                break;
            }
        }

        favorites.deletearrays();
        return result;
    }

    bool setserverfavorite(const char *id, bool favorite)
    {
        if(!validfavoriteid(id))
            return false;

        vector<char *> favorites;
        readserverfavorites(favorites);

        int found = -1;
        loopv(favorites)
        {
            if(!strcmp(favorites[i], id))
            {
                found = i;
                break;
            }
        }

        if(favorite)
        {
            if(found < 0)
            {
                favorites.add(newstring(id));
                favorites.sort();
            }
        }
        else if(found >= 0)
        {
            delete[] favorites.remove(found);
        }

        vector<char> serialized;
        loopv(favorites)
        {
            if(i)
                serialized.put('\n');
            serialized.put(favorites[i], int(strlen(favorites[i])));
        }
        serialized.put('\0');

        setsvar("serverfavorites", serialized.getbuf());
        const bool saved = savepersistentstate();
        favorites.deletearrays();
        return saved;
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
ICOMMAND(ui_editor, "", (), cowui::showeditor());
ICOMMAND(ui_close, "", (), cowui::close());
