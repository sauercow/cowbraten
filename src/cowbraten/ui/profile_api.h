#ifndef COWBRATEN_UI_PROFILE_API_H
#define COWBRATEN_UI_PROFILE_API_H

#include <string>

namespace cowui
{
    namespace profile
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

        bool readname(std::string &result);
        bool writename(const char *value, std::string &result);
        int savednamecount();
        bool readsavedname(int index, std::string &result);
        bool savename(const char *value, std::string &result);
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
