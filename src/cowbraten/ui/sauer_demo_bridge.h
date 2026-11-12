#ifndef COWBRATEN_UI_SAUER_DEMO_BRIDGE_H
#define COWBRATEN_UI_SAUER_DEMO_BRIDGE_H

namespace cowui
{
    namespace sauerdemo
    {
        struct Settings
        {
            bool hd_playback = true;
            bool hd_recording = true;
            bool automatic_recording = false;
        };

        const char *demo_directory();
        Settings settings();
        bool set_hd_playback(bool value);
        bool set_hd_recording(bool value);
        bool set_automatic_recording(bool value);

        bool recording();
        bool playback();
        bool paused();
        bool playback_hud_enabled();
        bool playback_interactive();
        int playback_position();
        int playback_duration();
        int playback_speed();
        const char *playback_map();
        const char *playback_mode();
        const char *playback_camera();
        void move_playback_cursor(int x, int y);
        void playback_cursor(float &x, float &y);
        float playback_cursor_size();
        bool play(const char *relative_path);
        void stop();
        void toggle_pause();
        void seek_absolute(int milliseconds);
        void seek_relative(int milliseconds);
        void step(int direction);
        void cycle_speed();
        bool set_speed(int percent);
        bool open_location(const char *absolute_path);
    }
}

#endif
