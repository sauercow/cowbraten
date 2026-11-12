#include "game.h"
#include "sauer_demo_bridge.h"

#include <SDL.h>

namespace cowui
{
    namespace sauerdemo
    {
        namespace
        {
            bool readbool(const char *name, bool fallback)
            {
                ident *id = getident(name);
                return id && id->type == ID_VAR && id->storage.i ? *id->storage.i != 0 : fallback;
            }

            bool writebool(const char *name, bool value)
            {
                ident *id = getident(name);
                if(!id || id->type != ID_VAR || !id->storage.i) return false;
                setvarchecked(id, value ? 1 : 0);
                return *id->storage.i == (value ? 1 : 0);
            }
        }

        const char *demo_directory()
        {
            static string result;
            formatstring(result, "%s%s", homedir[0] ? homedir : "", server::demodir[0] ? server::demodir : "demo");
            path(result);
            return result;
        }

        Settings settings()
        {
            Settings result;
            result.hd_playback = readbool("demohdplay", true);
            result.hd_recording = readbool("demohdrecord", true);
            result.automatic_recording = readbool("autorecordclientdemo", false);
            return result;
        }

        bool set_hd_playback(bool value) { return writebool("demohdplay", value); }
        bool set_hd_recording(bool value) { return writebool("demohdrecord", value); }
        bool set_automatic_recording(bool value) { return writebool("autorecordclientdemo", value); }

        bool recording() { return game::demorecord != nullptr; }
        bool playback() { return server::demoplaybackactive(); }
        bool paused() { return game::gamepaused; }

        bool play(const char *relative_path)
        {
            if(!relative_path || !relative_path[0] || recording()) return false;
            if(game::remote) disconnect(false, true);
            game::changemap(relative_path, -1);
            return true;
        }

        void stop() { game::stopdemo(); }
        void toggle_pause() { server::toggledemopause(); }
        void seek_relative(int milliseconds)
        {
            if(milliseconds < 0) server::demorewind(-milliseconds);
            else server::demoforward(milliseconds);
        }
        bool set_speed(int percent) { return server::setdemoplaybackspeed(percent); }

        bool open_location(const char *absolute_path)
        {
            if(!absolute_path || !absolute_path[0]) return false;
            string uri;
            formatstring(uri, "file://%s", parentdir(absolute_path));
            return SDL_OpenURL(uri) == 0;
        }
    }
}
