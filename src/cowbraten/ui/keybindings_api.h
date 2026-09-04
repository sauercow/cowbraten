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

        bool visit(Action action, KeyVisitor visitor, void *data);
        bool bind(Action action, const char *key_name);
        bool unbind(Action action, const char *key_name);
    }
}

#endif
