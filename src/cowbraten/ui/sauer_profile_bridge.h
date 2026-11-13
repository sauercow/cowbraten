#ifndef COWBRATEN_UI_SAUER_PROFILE_BRIDGE_H
#define COWBRATEN_UI_SAUER_PROFILE_BRIDGE_H

namespace cowui
{
    namespace sauerprofile
    {
        enum class PreviewAnimation
        {
            Idle,
            Run,
            Attack,
            Jump,
            Death
        };

        struct Character
        {
            int id;
            const char *name;
            const char *story;
            const char *icon;
            bool ragdoll;
        };

        struct ModelSpoofing
        {
            bool enabled;
            int model;
        };

        int charactercount();
        bool readcharacter(int index, Character &result);
        bool readselection(int &result);
        bool writeselection(int value, int &result);
        bool readmodelspoofing(ModelSpoofing &result);
        bool writemodelspoofingenabled(bool value, ModelSpoofing &result);
        bool writemodelspoofingselection(int value, ModelSpoofing &result);

        bool readname(char *result, int result_size);
        bool writename(const char *value, char *result, int result_size);
        int savednamecount();
        bool readsavedname(int index, char *result, int result_size);
        bool savename(const char *value, char *result, int result_size);
        bool deletename(const char *value);

        void renderpreview(
            int character,
            PreviewAnimation animation,
            float yaw,
            float zoom,
            int x,
            int y,
            int width,
            int height);
    }
}

#endif
