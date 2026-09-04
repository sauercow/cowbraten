#include "keybindings_api.h"

#include "sauer_keybindings_bridge.h"

namespace cowui
{
    namespace keybindings
    {
        static bool bridgeaction(Action value, sauerkeybindings::Action &result)
        {
            switch(value)
            {
                case Action::MoveForward:
                    result = sauerkeybindings::Action::MoveForward;
                    return true;
                case Action::MoveBackward:
                    result = sauerkeybindings::Action::MoveBackward;
                    return true;
                case Action::MoveLeft:
                    result = sauerkeybindings::Action::MoveLeft;
                    return true;
                case Action::MoveRight:
                    result = sauerkeybindings::Action::MoveRight;
                    return true;
                case Action::Jump:
                    result = sauerkeybindings::Action::Jump;
                    return true;
                case Action::Attack:
                    result = sauerkeybindings::Action::Attack;
                    return true;
                case Action::Zoom:
                    result = sauerkeybindings::Action::Zoom;
                    return true;
                case Action::WeaponAction:
                    result = sauerkeybindings::Action::WeaponAction;
                    return true;
                case Action::Scoreboard:
                    result = sauerkeybindings::Action::Scoreboard;
                    return true;
                case Action::PublicChat:
                    result = sauerkeybindings::Action::PublicChat;
                    return true;
                case Action::TeamChat:
                    result = sauerkeybindings::Action::TeamChat;
                    return true;
            }

            return false;
        }

        bool visit(Action value, KeyVisitor visitor, void *data)
        {
            if(!visitor)
                return false;

            sauerkeybindings::Action bridge;
            if(!bridgeaction(value, bridge))
                return false;

            return sauerkeybindings::visit(bridge, visitor, data);
        }

        bool bind(Action value, const char *key_name)
        {
            sauerkeybindings::Action bridge;
            if(!bridgeaction(value, bridge))
                return false;

            return sauerkeybindings::bind(bridge, key_name);
        }

        bool unbind(Action value, const char *key_name)
        {
            sauerkeybindings::Action bridge;
            if(!bridgeaction(value, bridge))
                return false;

            return sauerkeybindings::unbind(bridge, key_name);
        }
    }
}
