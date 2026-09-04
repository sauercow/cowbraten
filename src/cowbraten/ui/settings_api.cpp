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
            result.backend_active = legacy.backend_active;
            return true;
        }

        bool writesoundvolume(int value)
        {
            return sauersettings::writesoundvolume(value);
        }

        bool writemusicvolume(int value)
        {
            return sauersettings::writemusicvolume(value);
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
    }
}
