#include "engine.h"

#include "sauer_keybindings_bridge.h"

#include <cstring>

namespace cowui
{
    namespace sauerkeybindings
    {
        static const char *actionname(Action action)
        {
            switch(action)
            {
                case Action::MoveForward: return "forward";
                case Action::MoveBackward: return "backward";
                case Action::MoveLeft: return "left";
                case Action::MoveRight: return "right";
                case Action::Jump: return "jump";
                case Action::Attack: return "attack";
                case Action::Zoom: return "togglezoom";
                case Action::WeaponAction: return "weapon";
                case Action::Scoreboard: return "showscores";
                case Action::PublicChat: return "saycommand";
                case Action::TeamChat: return "sayteamcommand";
            }
            return nullptr;
        }

        bool visit(Action action, KeyVisitor visitor, void *data)
        {
            const char *name = actionname(action);
            if(!name || !visitor)
                return false;

            visitdefaultkeybinds(name, visitor, data);
            return true;
        }

        bool bind(Action action, const char *key_name)
        {
            const char *name = actionname(action);
            if(!name || !key_name || !key_name[0])
                return false;

            return setdefaultkeybind(key_name, name);
        }

        bool unbind(Action action, const char *key_name)
        {
            const char *name = actionname(action);
            if(!name || !key_name || !key_name[0])
                return false;

            return cleardefaultkeybind(key_name, name);
        }
    }
}
