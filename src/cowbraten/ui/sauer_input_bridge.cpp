#include "engine.h"

#include "sauer_input_bridge.h"

extern void setinputgrabinhibited(bool inhibit);

namespace cowui
{
    namespace sauerinput
    {
        void beginmenu()
        {
            setinputgrabinhibited(true);
        }

        void endmenu()
        {
            setinputgrabinhibited(false);
        }

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
