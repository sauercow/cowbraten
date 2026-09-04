#ifndef COWBRATEN_UI_SETTINGS_API_H
#define COWBRATEN_UI_SETTINGS_API_H

namespace cowui
{
    namespace settings
    {
        enum class PresentationMode
        {
            Windowed,
            ExclusiveFullscreen,
            DesktopFullscreen
        };

        struct Fov
        {
            int value;
            int minimum;
            int maximum;
        };

        struct Video
        {
            int width;
            int height;
            PresentationMode presentation;
        };

        struct Vsync
        {
            bool enabled;
            bool adaptive;
        };

        struct FpsLimit
        {
            int value;
            int minimum;
            int maximum;
        };

        struct Antialiasing
        {
            int requested_samples;
            int actual_samples;
        };

        struct Audio
        {
            int sound_volume;
            int sound_volume_minimum;
            int sound_volume_maximum;
            int music_volume;
            int music_volume_minimum;
            int music_volume_maximum;
            bool backend_active;
        };

        struct MouseInput
        {
            float sensitivity;
            float sensitivity_minimum;
            float sensitivity_maximum;
            bool inverted;
        };

        struct Crosshair
        {
            int selection;
            const char *resource;
            int size;
            int size_minimum;
            int size_maximum;
            bool effects;
            bool health_colors;
            bool teammates;
            bool hit_marker;
            bool reload_fade;
            bool reload_rotation;
        };

        struct CrosshairChoice
        {
            int id;
            const char *label;
            const char *resource;
        };

        enum class FragMessagesMode
        {
            Off,
            AllModes,
            OutsideInsta
        };

        struct Hud
        {
            bool damage_screen;
            bool damage_compass;
            FragMessagesMode frag_messages;
            float streak_milestone_scale;
            float streak_milestone_scale_minimum;
            float streak_milestone_scale_maximum;
        };

        struct Resolution
        {
            int width;
            int height;
        };

        typedef void (*ResolutionVisitor)(
            const Resolution &resolution,
            void *data
        );

        typedef void (*CrosshairChoiceVisitor)(
            const CrosshairChoice &choice,
            void *data
        );

        bool readfov(Fov &result);
        bool writefov(int value);

        bool readvideo(Video &result);
        bool writepresentation(PresentationMode mode);
        bool writeresolution(int width, int height);
        bool visitresolutions(ResolutionVisitor visitor, void *data);

        bool readvsync(Vsync &result);
        bool writevsync(bool enabled);

        bool readfpslimit(FpsLimit &result);
        bool writefpslimit(int value);

        bool readantialiasing(Antialiasing &result);
        bool writeantialiasing(int samples);
        void resetgraphics();

        bool readaudio(Audio &result);
        bool writesoundvolume(int value);
        bool writemusicvolume(int value);

        bool readmouseinput(MouseInput &result);
        bool writesensitivity(float value);
        bool writeinvertedmouse(bool enabled);

        bool readcrosshair(Crosshair &result);
        bool visitcrosshairs(CrosshairChoiceVisitor visitor, void *data);
        bool writecrosshairselection(int selection);
        bool writecrosshairsize(int size);
        bool writecrosshaireffects(bool enabled);
        bool writecrosshairhealthcolors(bool enabled);
        bool writeteamcrosshair(bool enabled);
        bool writehitcrosshair(bool enabled);
        bool writecrosshairreloadfade(bool enabled);
        bool writecrosshairreloadrotation(bool enabled);

        bool readhud(Hud &result);
        bool writedamagescreen(bool enabled);
        bool writedamagecompass(bool enabled);
        bool writefragmessages(FragMessagesMode mode);
        bool writestreakmilestonescale(float value);
    }
}

#endif
