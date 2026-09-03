#include "engine.h"

#include "sauer_settings_bridge.h"

namespace cowui
{
    namespace sauersettings
    {
        static ident *fovident()
        {
            ident *id = getident("fov");
            return id && id->type == ID_VAR
                ? id
                : nullptr;
        }

        bool readfov(Fov &result)
        {
            ident *id = fovident();
            if(!id || !id->storage.i)
                return false;

            result.value = *id->storage.i;
            result.minimum = id->minval;
            result.maximum = id->maxval;
            return true;
        }

        bool writefov(int value)
        {
            ident *id = fovident();
            if(!id || !id->storage.i)
                return false;

            setvarchecked(id, value);
            return true;
        }
    }
}
