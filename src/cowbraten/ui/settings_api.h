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

        // Grass reflects the native switch; motion blur also reflects its
        // renderer availability gate, not per-frame visibility.
        struct GraphicsQuality
        {
            bool grass;
            bool motion_blur;
            bool motion_blur_available;
        };

        enum class ShaderDetailLevel { Level0 = 0, Low = 1, Level2 = 2, High = 3 };
        struct ShaderDetailState
        {
            int requested;
            bool ready;
            // Resident, historically forced base selections; not a global effective level.
            int inspected, alternatives, unresolved;
        };
        bool readshaderdetail(ShaderDetailState &result);
        // Native write acceptance, independent of shader selection health.
        bool writeshaderdetail(ShaderDetailLevel level, ShaderDetailState &result);

        enum class MipmapBlendingStatus { Unavailable, NoTextures, Verified, Mismatch, Mixed };
        struct MipmapBlendingState
        {
            MipmapBlendingStatus status;
            bool requested;
            int textures;
        };
        bool readmipmapblending(MipmapBlendingState &result);
        // Acceptance of the native write; verification is reported in status.
        bool writemipmapblending(bool enabled, MipmapBlendingState &result);

        enum class AnisotropyLevel { Off = 0, X2 = 2, X4 = 4, X8 = 8, X16 = 16 };
        enum class AnisotropyStatus { Unavailable, Unsupported, NoTextures, Verified, Mismatch, Mixed };
        struct AnisotropyState
        {
            AnisotropyStatus status;
            int requested;
            int maximum;
            int capped;
            int textures;
            float actual; // Only a uniform loaded value when Verified/Mismatch.
        };

        struct Audio
        {
            int sound_volume;
            int sound_volume_minimum;
            int sound_volume_maximum;
            int music_volume;
            int music_volume_minimum;
            int music_volume_maximum;
            const char *active_soundpack;
            bool backend_active;
        };

        struct SoundpackChoice
        {
            const char *id;
            const char *display_name;
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

        struct Chat
        {
            bool public_messages_in_top_feed;
            bool team_messages_in_top_feed;
            bool public_messages_in_bottom_box;
            bool team_messages_in_bottom_box;
            bool team_colored_names;
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

        typedef void (*SoundpackChoiceVisitor)(
            const SoundpackChoice &choice,
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

        bool readgraphicsquality(GraphicsQuality &result);
        bool writegrass(bool enabled);
        bool writemotionblur(bool enabled);

        bool readanisotropy(AnisotropyState &result);
        // Returns the callback's verification result without a second scan.
        bool writeanisotropy(AnisotropyLevel level, AnisotropyState &result);

        bool readaudio(Audio &result);
        bool visitsoundpacks(SoundpackChoiceVisitor visitor, void *data);
        bool writesoundvolume(int value);
        bool writemusicvolume(int value);
        bool writesoundpack(const char *id);

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

        bool readchat(Chat &result);
        bool writepublicchatintopfeed(bool enabled);
        bool writeteamchatintopfeed(bool enabled);
        bool writepublicchatinbottombox(bool enabled);
        bool writeteamchatinbottombox(bool enabled);
        bool writeteamcoloredchatnames(bool enabled);
    }
}

#endif
