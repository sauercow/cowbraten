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

        static bool actionfromname(const char *name, Action &result)
        {
            if(!name || !name[0])
                return false;

            const Action actions[] =
            {
                Action::MoveForward,
                Action::MoveBackward,
                Action::MoveLeft,
                Action::MoveRight,
                Action::Jump,
                Action::Attack,
                Action::Zoom,
                Action::WeaponAction,
                Action::Scoreboard,
                Action::PublicChat,
                Action::TeamChat
            };

            for(Action action : actions)
            {
                if(!std::strcmp(name, actionname(action)))
                {
                    result = action;
                    return true;
                }
            }
            return false;
        }

        bool visit(Action action, KeyVisitor visitor, void *data)
        {
            const char *name = actionname(action);
            if(!name || !visitor)
                return false;

            visitdefaultkeybinds(name, visitor, data);
            return true;
        }

        bool inspectconflict(
            Action target_action,
            const char *key_name,
            Conflict &result)
        {
            result = Conflict();
            if(!actionname(target_action) || !key_name || !key_name[0])
                return false;

            char owner[MAXSTRLEN];
            bool bound = false;

            if(!getdefaultkeybind(key_name, owner, sizeof(owner), bound))
                return false;

            if(!bound)
                return true;

            Action owner_action;
            if(!actionfromname(owner, owner_action))
            {
                result.kind = ConflictKind::CustomAction;
                return true;
            }

            result.action = owner_action;
            result.kind = owner_action == target_action
                ? ConflictKind::SameAction
                : ConflictKind::CuratedAction;
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
