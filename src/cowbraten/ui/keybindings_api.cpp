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

        static bool publicaction(sauerkeybindings::Action value, Action &result)
        {
            switch(value)
            {
                case sauerkeybindings::Action::MoveForward:
                    result = Action::MoveForward;
                    return true;
                case sauerkeybindings::Action::MoveBackward:
                    result = Action::MoveBackward;
                    return true;
                case sauerkeybindings::Action::MoveLeft:
                    result = Action::MoveLeft;
                    return true;
                case sauerkeybindings::Action::MoveRight:
                    result = Action::MoveRight;
                    return true;
                case sauerkeybindings::Action::Jump:
                    result = Action::Jump;
                    return true;
                case sauerkeybindings::Action::Attack:
                    result = Action::Attack;
                    return true;
                case sauerkeybindings::Action::Zoom:
                    result = Action::Zoom;
                    return true;
                case sauerkeybindings::Action::WeaponAction:
                    result = Action::WeaponAction;
                    return true;
                case sauerkeybindings::Action::Scoreboard:
                    result = Action::Scoreboard;
                    return true;
                case sauerkeybindings::Action::PublicChat:
                    result = Action::PublicChat;
                    return true;
                case sauerkeybindings::Action::TeamChat:
                    result = Action::TeamChat;
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

        bool inspectconflict(
            Action target_action,
            const char *key_name,
            Conflict &result)
        {
            result = Conflict();
            sauerkeybindings::Action target_bridge;
            if(!bridgeaction(target_action, target_bridge))
                return false;

            sauerkeybindings::Conflict bridge_result;
            if(!sauerkeybindings::inspectconflict(
                   target_bridge,
                   key_name,
                   bridge_result))
            {
                return false;
            }

            switch(bridge_result.kind)
            {
                case sauerkeybindings::ConflictKind::Unbound:
                    result.kind = ConflictKind::Unbound;
                    return true;
                case sauerkeybindings::ConflictKind::SameAction:
                    result.kind = ConflictKind::SameAction;
                    break;
                case sauerkeybindings::ConflictKind::CuratedAction:
                    result.kind = ConflictKind::CuratedAction;
                    break;
                case sauerkeybindings::ConflictKind::CustomAction:
                    result.kind = ConflictKind::CustomAction;
                    return true;
            }

            if(!publicaction(bridge_result.action, result.action))
                return false;
            return true;
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
