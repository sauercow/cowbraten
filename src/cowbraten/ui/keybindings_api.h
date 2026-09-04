#ifndef COWBRATEN_UI_KEYBINDINGS_API_H
#define COWBRATEN_UI_KEYBINDINGS_API_H

namespace cowui
{
    namespace keybindings
    {
        enum class Action
        {
            MoveForward,
            MoveBackward,
            MoveLeft,
            MoveRight,
            Jump,
            Attack,
            Zoom,
            WeaponAction,
            Scoreboard,
            PublicChat,
            TeamChat
        };

        typedef void (*KeyVisitor)(const char *key_name, void *data);

        enum class ConflictKind
        {
            Unbound,
            SameAction,
            CuratedAction,
            CustomAction
        };

        struct Conflict
        {
            ConflictKind kind = ConflictKind::Unbound;
            Action action = Action::MoveForward;
        };

        bool visit(Action action, KeyVisitor visitor, void *data);
        bool inspectconflict(
            Action target_action,
            const char *key_name,
            Conflict &result
        );
        bool bind(Action action, const char *key_name);
        bool unbind(Action action, const char *key_name);
    }
}

#endif
