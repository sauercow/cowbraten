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
            result.drawable_width = legacy.drawable_width;
            result.drawable_height = legacy.drawable_height;
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

        bool readgraphicsquality(GraphicsQuality &result)
        {
            sauersettings::GraphicsQuality legacy;
            if(!sauersettings::readgraphicsquality(legacy))
                return false;

            result.grass = legacy.grass;
            result.motion_blur = legacy.motion_blur;
            result.motion_blur_available = legacy.motion_blur_available;
            result.glare = legacy.glare;
            return true;
        }

        bool writegrass(bool enabled)
        {
            return sauersettings::writegrass(enabled);
        }

        bool writemotionblur(bool enabled)
        {
            return sauersettings::writemotionblur(enabled);
        }

        bool writeglare(bool enabled)
        {
            return sauersettings::writeglare(enabled);
        }

        static GrassDistance grassdistance(
            sauersettings::GrassDistance distance)
        {
            switch(distance)
            {
                case sauersettings::GrassDistance::Quick:
                    return GrassDistance::Quick;
                case sauersettings::GrassDistance::Moderate:
                    return GrassDistance::Moderate;
                case sauersettings::GrassDistance::Slow:
                    return GrassDistance::Slow;
                case sauersettings::GrassDistance::Custom:
                    return GrassDistance::Custom;
            }
            return GrassDistance::Custom;
        }

        static sauersettings::GrassDistance grassdistance(
            GrassDistance distance)
        {
            switch(distance)
            {
                case GrassDistance::Quick:
                    return sauersettings::GrassDistance::Quick;
                case GrassDistance::Moderate:
                    return sauersettings::GrassDistance::Moderate;
                case GrassDistance::Slow:
                    return sauersettings::GrassDistance::Slow;
                case GrassDistance::Custom:
                    return sauersettings::GrassDistance::Custom;
            }
            return sauersettings::GrassDistance::Custom;
        }

        static void copygrassdistance(
            const sauersettings::GrassDistanceState &source,
            GrassDistanceState &result)
        {
            result.distance = grassdistance(source.distance);
            result.value = source.value;
        }

        bool readgrassdistance(GrassDistanceState &result)
        {
            sauersettings::GrassDistanceState legacy = {};
            const bool read = sauersettings::readgrassdistance(legacy);
            copygrassdistance(legacy, result);
            return read;
        }

        bool writegrassdistance(
            GrassDistance distance,
            GrassDistanceState &result)
        {
            sauersettings::GrassDistanceState legacy = {};
            const bool accepted = sauersettings::writegrassdistance(
                grassdistance(distance), legacy);
            copygrassdistance(legacy, result);
            return accepted;
        }

        static MotionBlurIntensity motionblurintensity(
            sauersettings::MotionBlurIntensity intensity)
        {
            switch(intensity)
            {
                case sauersettings::MotionBlurIntensity::Subtle:
                    return MotionBlurIntensity::Subtle;
                case sauersettings::MotionBlurIntensity::Moderate:
                    return MotionBlurIntensity::Moderate;
                case sauersettings::MotionBlurIntensity::Intense:
                    return MotionBlurIntensity::Intense;
                case sauersettings::MotionBlurIntensity::Custom:
                    return MotionBlurIntensity::Custom;
            }
            return MotionBlurIntensity::Custom;
        }

        static sauersettings::MotionBlurIntensity motionblurintensity(
            MotionBlurIntensity intensity)
        {
            switch(intensity)
            {
                case MotionBlurIntensity::Subtle:
                    return sauersettings::MotionBlurIntensity::Subtle;
                case MotionBlurIntensity::Moderate:
                    return sauersettings::MotionBlurIntensity::Moderate;
                case MotionBlurIntensity::Intense:
                    return sauersettings::MotionBlurIntensity::Intense;
                case MotionBlurIntensity::Custom:
                    return sauersettings::MotionBlurIntensity::Custom;
            }
            return sauersettings::MotionBlurIntensity::Custom;
        }

        static void copymotionblurintensity(
            const sauersettings::MotionBlurIntensityState &source,
            MotionBlurIntensityState &result)
        {
            result.intensity = motionblurintensity(source.intensity);
            result.scale = source.scale;
        }

        bool readmotionblurintensity(MotionBlurIntensityState &result)
        {
            sauersettings::MotionBlurIntensityState legacy = {};
            const bool read = sauersettings::readmotionblurintensity(legacy);
            copymotionblurintensity(legacy, result);
            return read;
        }

        bool writemotionblurintensity(
            MotionBlurIntensity intensity,
            MotionBlurIntensityState &result)
        {
            sauersettings::MotionBlurIntensityState legacy = {};
            const bool accepted = sauersettings::writemotionblurintensity(
                motionblurintensity(intensity), legacy);
            copymotionblurintensity(legacy, result);
            return accepted;
        }

        static GlarePreset glarepreset(sauersettings::GlarePreset preset)
        {
            switch(preset)
            {
                case sauersettings::GlarePreset::Subtle:
                    return GlarePreset::Subtle;
                case sauersettings::GlarePreset::Glowy:
                    return GlarePreset::Glowy;
                case sauersettings::GlarePreset::Soft:
                    return GlarePreset::Soft;
                case sauersettings::GlarePreset::Intense:
                    return GlarePreset::Intense;
                case sauersettings::GlarePreset::Custom:
                    return GlarePreset::Custom;
            }
            return GlarePreset::Custom;
        }

        static sauersettings::GlarePreset glarepreset(GlarePreset preset)
        {
            switch(preset)
            {
                case GlarePreset::Subtle:
                    return sauersettings::GlarePreset::Subtle;
                case GlarePreset::Glowy:
                    return sauersettings::GlarePreset::Glowy;
                case GlarePreset::Soft:
                    return sauersettings::GlarePreset::Soft;
                case GlarePreset::Intense:
                    return sauersettings::GlarePreset::Intense;
                case GlarePreset::Custom:
                    return sauersettings::GlarePreset::Custom;
            }
            return sauersettings::GlarePreset::Custom;
        }

        static void copyglarepreset(
            const sauersettings::GlarePresetState &source,
            GlarePresetState &result)
        {
            result.preset = glarepreset(source.preset);
            result.scale = source.scale;
            result.blur_radius = source.blur_radius;
        }

        bool readglarepreset(GlarePresetState &result)
        {
            sauersettings::GlarePresetState legacy = {};
            const bool read = sauersettings::readglarepreset(legacy);
            copyglarepreset(legacy, result);
            return read;
        }

        bool writeglarepreset(GlarePreset preset, GlarePresetState &result)
        {
            sauersettings::GlarePresetState legacy = {};
            const bool accepted = sauersettings::writeglarepreset(
                glarepreset(preset), legacy);
            copyglarepreset(legacy, result);
            return accepted;
        }

        static void copyshaderdetail(const sauersettings::ShaderDetailState &source, ShaderDetailState &result)
        {
            result.requested = source.requested;
            result.ready = source.ready;
            result.inspected = source.inspected;
            result.alternatives = source.alternatives;
            result.unresolved = source.unresolved;
        }

        bool readshaderdetail(ShaderDetailState &result)
        {
            sauersettings::ShaderDetailState legacy = {};
            const bool read = sauersettings::readshaderdetail(legacy);
            copyshaderdetail(legacy, result);
            return read;
        }

        bool writeshaderdetail(ShaderDetailLevel level, ShaderDetailState &result)
        {
            sauersettings::ShaderDetailState legacy = {};
            const bool accepted = sauersettings::writeshaderdetail(
                static_cast<sauersettings::ShaderDetailLevel>(static_cast<int>(level)), legacy);
            copyshaderdetail(legacy, result);
            return accepted;
        }

        static void copymipmapblending(const sauersettings::MipmapBlendingState &source, MipmapBlendingState &result)
        {
            switch(source.status)
            {
                case sauersettings::MipmapBlendingStatus::Unavailable: result.status = MipmapBlendingStatus::Unavailable; break;
                case sauersettings::MipmapBlendingStatus::NoTextures: result.status = MipmapBlendingStatus::NoTextures; break;
                case sauersettings::MipmapBlendingStatus::Verified: result.status = MipmapBlendingStatus::Verified; break;
                case sauersettings::MipmapBlendingStatus::Mismatch: result.status = MipmapBlendingStatus::Mismatch; break;
                case sauersettings::MipmapBlendingStatus::Mixed: result.status = MipmapBlendingStatus::Mixed; break;
            }
            result.requested = source.requested;
            result.textures = source.textures;
        }

        bool readmipmapblending(MipmapBlendingState &result)
        {
            sauersettings::MipmapBlendingState legacy = {};
            const bool read = sauersettings::readmipmapblending(legacy);
            copymipmapblending(legacy, result);
            return read;
        }

        bool writemipmapblending(bool enabled, MipmapBlendingState &result)
        {
            sauersettings::MipmapBlendingState legacy = {};
            const bool accepted = sauersettings::writemipmapblending(enabled, legacy);
            copymipmapblending(legacy, result);
            return accepted;
        }

        static void copyanisotropy(const sauersettings::AnisotropyState &source, AnisotropyState &result)
        {
            switch(source.status)
            {
                case sauersettings::AnisotropyStatus::Unavailable: result.status = AnisotropyStatus::Unavailable; break;
                case sauersettings::AnisotropyStatus::Unsupported: result.status = AnisotropyStatus::Unsupported; break;
                case sauersettings::AnisotropyStatus::NoTextures: result.status = AnisotropyStatus::NoTextures; break;
                case sauersettings::AnisotropyStatus::Verified: result.status = AnisotropyStatus::Verified; break;
                case sauersettings::AnisotropyStatus::Mismatch: result.status = AnisotropyStatus::Mismatch; break;
                case sauersettings::AnisotropyStatus::Mixed: result.status = AnisotropyStatus::Mixed; break;
            }
            result.requested = source.requested;
            result.maximum = source.maximum;
            result.capped = source.capped;
            result.textures = source.textures;
            result.actual = source.actual;
        }

        bool readanisotropy(AnisotropyState &result)
        {
            sauersettings::AnisotropyState legacy;
            const bool read = sauersettings::readanisotropy(legacy);
            copyanisotropy(legacy, result);
            return read;
        }

        bool writeanisotropy(AnisotropyLevel level, AnisotropyState &result)
        {
            sauersettings::AnisotropyState legacy;
            const bool accepted = sauersettings::writeanisotropy(
                static_cast<sauersettings::AnisotropyLevel>(static_cast<int>(level)), legacy);
            copyanisotropy(legacy, result);
            return accepted;
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
