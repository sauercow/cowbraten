#include "game.h"
#include "sauer_demo_bridge.h"

#include <SDL.h>
#ifdef WIN32
#include <shellapi.h>
#endif

extern int screenw;

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
        bool playback_hud_enabled() { return readbool("huddemoplayback", true); }
        bool playback_interactive()
        {
            return playback() && playback_hud_enabled() &&
                (SDL_GetModState() & KMOD_ALT);
        }

        int playback_position() { return max(server::gamemillis, 0); }
        int playback_duration()
        {
            return server::demoplaybackduration > 0
                ? server::demoplaybackduration
                : max(server::gamelimit, playback_position());
        }
        int playback_speed() { return game::gamespeed; }
        const char *playback_map() { return game::clientmap[0] ? game::clientmap : "Unknown map"; }
        const char *playback_mode() { return server::modename(game::gamemode, "Unknown mode"); }
        const char *playback_camera()
        {
            fpsent *target = game::followingplayer();
            return target && target->name[0] ? target->name : "Free camera";
        }
        bool xray_enabled() { return game::demoxrayactive(); }
        bool hitbox_enabled() { return game::demohitboxactive(); }
        void toggle_xray() { game::toggledemoxray(); }
        void toggle_hitbox() { game::toggledemohitbox(); }
        void move_playback_cursor(int x, int y) { g3d_movecursor(x, y); }
        void playback_cursor(float &x, float &y) { g3d_cursorpos(x, y); }
        float playback_cursor_size()
        {
            ident *id = getident("cursorsize");
            const int size = id && id->type == ID_VAR && id->storage.i
                ? *id->storage.i
                : 20;
            return size * screenw / 900.0f;
        }

        bool play(const char *relative_path)
        {
            if(!relative_path || !relative_path[0] || recording()) return false;
            if(game::remote) disconnect(false, true);
            game::changemap(relative_path, -1);
            return true;
        }

        void stop() { game::stopdemo(); }
        void toggle_pause() { server::toggledemopause(); }
        void seek_absolute(int milliseconds) { server::demoseekabsolute(max(milliseconds, 0)); }
        void seek_relative(int milliseconds)
        {
            if(milliseconds < 0) server::demorewind(-milliseconds);
            else server::demoforward(milliseconds);
        }
        void step(int direction) { server::demostep(direction); }
        void cycle_speed()
        {
            static const int speeds[] = { 25, 50, 100, 200, 400 };
            loopi(sizeof(speeds) / sizeof(speeds[0])) if(speeds[i] > game::gamespeed)
            {
                set_speed(speeds[i]);
                return;
            }
            set_speed(speeds[0]);
        }
        bool set_speed(int percent) { return server::setdemoplaybackspeed(percent); }

        bool open_location(const char *absolute_path)
        {
            if(!absolute_path || !absolute_path[0]) return false;
#ifdef WIN32
            return reinterpret_cast<intptr_t>(ShellExecuteA(nullptr, "open",
                parentdir(absolute_path), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
#else
            string uri;
            formatstring(uri, "file://%s", parentdir(absolute_path));
            return SDL_OpenURL(uri) == 0;
#endif
        }
    }
}
