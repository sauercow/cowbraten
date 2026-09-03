#include "settings_api.h"

#include "sauer_settings_bridge.h"

namespace cowui
{
    namespace settings
    {
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
            result.fullscreen = legacy.fullscreen;
            result.desktop_fullscreen = legacy.desktop_fullscreen;
            return true;
        }

        bool writefullscreen(bool enabled)
        {
            return sauersettings::writefullscreen(enabled);
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
    }
}
