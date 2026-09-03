#include "engine.h"

#include "sauer_input_bridge.h"

namespace cowui
{
    namespace sauerinput
    {
        void keyrepeat(bool enabled)
        {
            ::keyrepeat(enabled, KR_COWUI);
        }

        void textinput(bool enabled)
        {
            ::textinput(enabled, TI_COWUI);
        }
    }
}
