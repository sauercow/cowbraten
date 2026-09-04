#include "settings_api.h"

#include "sauer_settings_bridge.h"

namespace cowui
{
    namespace settings
    {
        static PresentationMode presentationmode(
            sauersettings::PresentationMode mode)
        {
            switch(mode)
            {
                case sauersettings::PresentationMode::Windowed:
                    return PresentationMode::Windowed;
                case sauersettings::PresentationMode::ExclusiveFullscreen:
                    return PresentationMode::ExclusiveFullscreen;
                case sauersettings::PresentationMode::DesktopFullscreen:
                    return PresentationMode::DesktopFullscreen;
            }
            return PresentationMode::Windowed;
        }

        static sauersettings::PresentationMode presentationmode(
            PresentationMode mode)
        {
            switch(mode)
            {
                case PresentationMode::Windowed:
                    return sauersettings::PresentationMode::Windowed;
                case PresentationMode::ExclusiveFullscreen:
                    return sauersettings::PresentationMode::ExclusiveFullscreen;
                case PresentationMode::DesktopFullscreen:
                    return sauersettings::PresentationMode::DesktopFullscreen;
            }
            return sauersettings::PresentationMode::Windowed;
        }

        static FragMessagesMode fragmessagesmode(
            sauersettings::FragMessagesMode mode)
        {
            switch(mode)
            {
                case sauersettings::FragMessagesMode::Off:
                    return FragMessagesMode::Off;
                case sauersettings::FragMessagesMode::AllModes:
                    return FragMessagesMode::AllModes;
                case sauersettings::FragMessagesMode::OutsideInsta:
                    return FragMessagesMode::OutsideInsta;
            }
            return FragMessagesMode::Off;
        }

        static sauersettings::FragMessagesMode fragmessagesmode(
            FragMessagesMode mode)
        {
            switch(mode)
            {
                case FragMessagesMode::Off:
                    return sauersettings::FragMessagesMode::Off;
                case FragMessagesMode::AllModes:
                    return sauersettings::FragMessagesMode::AllModes;
                case FragMessagesMode::OutsideInsta:
                    return sauersettings::FragMessagesMode::OutsideInsta;
            }
            return sauersettings::FragMessagesMode::Off;
        }

        bool readfov(Fov &result)
        {
            sauersettings::Fov legacy;
            if(!sauersettings::readfov(legacy))
                return false;

            result.value = legacy.value;
            result.minimum = legacy.minimum;
            result.maximum = legacy.maximum;
            return true;
        }

        bool writefov(int value)
        {
            return sauersettings::writefov(value);
        }

        bool readvideo(Video &result)
        {
            sauersettings::Video legacy;
            if(!sauersettings::readvideo(legacy))
                return false;

            result.width = legacy.width;
            result.height = legacy.height;
            result.presentation = presentationmode(legacy.presentation);
            return true;
        }

        bool writepresentation(PresentationMode mode)
        {
            return sauersettings::writepresentation(
                presentationmode(mode)
            );
        }

        bool writeresolution(int width, int height)
        {
            return sauersettings::writeresolution(width, height);
        }

        struct ResolutionAdapter
        {
            ResolutionVisitor visitor;
            void *data;
        };

        static void visitresolution(
            const sauersettings::Resolution &legacy,
            void *data)
        {
            ResolutionAdapter &adapter =
                *static_cast<ResolutionAdapter *>(data);
            Resolution resolution;
            resolution.width = legacy.width;
            resolution.height = legacy.height;
            adapter.visitor(resolution, adapter.data);
        }

        bool visitresolutions(ResolutionVisitor visitor, void *data)
        {
            if(!visitor)
                return false;

            ResolutionAdapter adapter;
            adapter.visitor = visitor;
            adapter.data = data;
            return sauersettings::visitresolutions(
                visitresolution,
                &adapter
            );
        }

        bool readvsync(Vsync &result)
        {
            sauersettings::Vsync legacy;
            if(!sauersettings::readvsync(legacy))
                return false;

            result.enabled = legacy.enabled;
            result.adaptive = legacy.adaptive;
            return true;
        }

        bool writevsync(bool enabled)
        {
            return sauersettings::writevsync(enabled);
        }

        bool readfpslimit(FpsLimit &result)
        {
            sauersettings::FpsLimit legacy;
            if(!sauersettings::readfpslimit(legacy))
                return false;

            result.value = legacy.value;
            result.minimum = legacy.minimum;
            result.maximum = legacy.maximum;
            return true;
        }

        bool writefpslimit(int value)
        {
            return sauersettings::writefpslimit(value);
        }

        bool readantialiasing(Antialiasing &result)
        {
            sauersettings::Antialiasing legacy;
            if(!sauersettings::readantialiasing(legacy))
                return false;

            result.requested_samples = legacy.requested_samples;
            result.actual_samples = legacy.actual_samples;
            return true;
        }

        bool writeantialiasing(int samples)
        {
            return sauersettings::writeantialiasing(samples);
        }

        void resetgraphics()
        {
            sauersettings::resetgraphics();
        }

        bool readaudio(Audio &result)
        {
            sauersettings::Audio legacy;
            if(!sauersettings::readaudio(legacy))
                return false;

            result.sound_volume = legacy.sound_volume;
            result.sound_volume_minimum =
                legacy.sound_volume_minimum;
            result.sound_volume_maximum =
                legacy.sound_volume_maximum;
            result.music_volume = legacy.music_volume;
            result.music_volume_minimum =
                legacy.music_volume_minimum;
            result.music_volume_maximum =
                legacy.music_volume_maximum;
            result.active_soundpack = legacy.active_soundpack;
            result.backend_active = legacy.backend_active;
            return true;
        }

        struct SoundpackChoiceAdapter
        {
            SoundpackChoiceVisitor visitor;
            void *data;
        };

        static void visitsoundpackchoice(
            const sauersettings::SoundpackChoice &legacy,
            void *data)
        {
            SoundpackChoiceAdapter &adapter =
                *static_cast<SoundpackChoiceAdapter *>(data);
            SoundpackChoice choice;
            choice.id = legacy.id;
            choice.display_name = legacy.display_name;
            adapter.visitor(choice, adapter.data);
        }

        bool visitsoundpacks(SoundpackChoiceVisitor visitor, void *data)
        {
            if(!visitor)
                return false;

            SoundpackChoiceAdapter adapter;
            adapter.visitor = visitor;
            adapter.data = data;
            return sauersettings::visitsoundpacks(
                visitsoundpackchoice,
                &adapter
            );
        }

        bool writesoundvolume(int value)
        {
            return sauersettings::writesoundvolume(value);
        }

        bool writemusicvolume(int value)
        {
            return sauersettings::writemusicvolume(value);
        }

        bool writesoundpack(const char *id)
        {
            return sauersettings::writesoundpack(id);
        }

        bool readmouseinput(MouseInput &result)
        {
            sauersettings::MouseInput legacy;
            if(!sauersettings::readmouseinput(legacy))
                return false;

            result.sensitivity = legacy.sensitivity;
            result.sensitivity_minimum =
                legacy.sensitivity_minimum;
            result.sensitivity_maximum =
                legacy.sensitivity_maximum;
            result.inverted = legacy.inverted;
            return true;
        }

        bool writesensitivity(float value)
        {
            return sauersettings::writesensitivity(value);
        }

        bool writeinvertedmouse(bool enabled)
        {
            return sauersettings::writeinvertedmouse(enabled);
        }

        bool readcrosshair(Crosshair &result)
        {
            sauersettings::Crosshair legacy;
            if(!sauersettings::readcrosshair(legacy))
                return false;

            result.selection = legacy.selection;
            result.resource = legacy.resource;
            result.size = legacy.size;
            result.size_minimum = legacy.size_minimum;
            result.size_maximum = legacy.size_maximum;
            result.effects = legacy.effects;
            result.health_colors = legacy.health_colors;
            result.teammates = legacy.teammates;
            result.hit_marker = legacy.hit_marker;
            result.reload_fade = legacy.reload_fade;
            result.reload_rotation = legacy.reload_rotation;
            return true;
        }

        struct CrosshairChoiceAdapter
        {
            CrosshairChoiceVisitor visitor;
            void *data;
        };

        static void visitcrosshairchoice(
            const sauersettings::CrosshairChoice &legacy,
            void *data)
        {
            CrosshairChoiceAdapter &adapter =
                *static_cast<CrosshairChoiceAdapter *>(data);
            CrosshairChoice choice;
            choice.id = legacy.id;
            choice.label = legacy.label;
            choice.resource = legacy.resource;
            adapter.visitor(choice, adapter.data);
        }

        bool visitcrosshairs(CrosshairChoiceVisitor visitor, void *data)
        {
            if(!visitor)
                return false;

            CrosshairChoiceAdapter adapter;
            adapter.visitor = visitor;
            adapter.data = data;
            return sauersettings::visitcrosshairs(
                visitcrosshairchoice,
                &adapter
            );
        }

        bool writecrosshairselection(int selection)
        {
            return sauersettings::writecrosshairselection(selection);
        }

        bool writecrosshairsize(int size)
        {
            return sauersettings::writecrosshairsize(size);
        }

        bool writecrosshaireffects(bool enabled)
        {
            return sauersettings::writecrosshaireffects(enabled);
        }

        bool writecrosshairhealthcolors(bool enabled)
        {
            return sauersettings::writecrosshairhealthcolors(enabled);
        }

        bool writeteamcrosshair(bool enabled)
        {
            return sauersettings::writeteamcrosshair(enabled);
        }

        bool writehitcrosshair(bool enabled)
        {
            return sauersettings::writehitcrosshair(enabled);
        }

        bool writecrosshairreloadfade(bool enabled)
        {
            return sauersettings::writecrosshairreloadfade(enabled);
        }

        bool writecrosshairreloadrotation(bool enabled)
        {
            return sauersettings::writecrosshairreloadrotation(enabled);
        }

        bool readhud(Hud &result)
        {
            sauersettings::Hud legacy;
            if(!sauersettings::readhud(legacy))
                return false;

            result.damage_screen = legacy.damage_screen;
            result.damage_compass = legacy.damage_compass;
            result.frag_messages = fragmessagesmode(legacy.frag_messages);
            result.streak_milestone_scale = legacy.streak_milestone_scale;
            result.streak_milestone_scale_minimum =
                legacy.streak_milestone_scale_minimum;
            result.streak_milestone_scale_maximum =
                legacy.streak_milestone_scale_maximum;
            return true;
        }

        bool writedamagescreen(bool enabled)
        {
            return sauersettings::writedamagescreen(enabled);
        }

        bool writedamagecompass(bool enabled)
        {
            return sauersettings::writedamagecompass(enabled);
        }

        bool writefragmessages(FragMessagesMode mode)
        {
            return sauersettings::writefragmessages(
                fragmessagesmode(mode)
            );
        }

        bool writestreakmilestonescale(float value)
        {
            return sauersettings::writestreakmilestonescale(value);
        }

        bool readchat(Chat &result)
        {
            sauersettings::Chat legacy;
            if(!sauersettings::readchat(legacy))
                return false;

            result.public_messages_in_top_feed =
                legacy.public_messages_in_top_feed;
            result.team_messages_in_top_feed =
                legacy.team_messages_in_top_feed;
            result.public_messages_in_bottom_box =
                legacy.public_messages_in_bottom_box;
            result.team_messages_in_bottom_box =
                legacy.team_messages_in_bottom_box;
            result.team_colored_names = legacy.team_colored_names;
            return true;
        }

        bool writepublicchatintopfeed(bool enabled)
        {
            return sauersettings::writepublicchatintopfeed(enabled);
        }

        bool writeteamchatintopfeed(bool enabled)
        {
            return sauersettings::writeteamchatintopfeed(enabled);
        }

        bool writepublicchatinbottombox(bool enabled)
        {
            return sauersettings::writepublicchatinbottombox(enabled);
        }

        bool writeteamchatinbottombox(bool enabled)
        {
            return sauersettings::writeteamchatinbottombox(enabled);
        }

        bool writeteamcoloredchatnames(bool enabled)
        {
            return sauersettings::writeteamcoloredchatnames(enabled);
        }
    }
}
