#include "profile_api.h"

#include "sauer_profile_bridge.h"
#include "sauer_text_bridge.h"

namespace cowui
{
    namespace profile
    {
        static sauerprofile::PreviewAnimation legacyanimation(
            PreviewAnimation animation)
        {
            switch(animation)
            {
                case PreviewAnimation::Idle:
                    return sauerprofile::PreviewAnimation::Idle;
                case PreviewAnimation::Run:
                    return sauerprofile::PreviewAnimation::Run;
                case PreviewAnimation::Attack:
                    return sauerprofile::PreviewAnimation::Attack;
                case PreviewAnimation::Jump:
                    return sauerprofile::PreviewAnimation::Jump;
                case PreviewAnimation::Death:
                    return sauerprofile::PreviewAnimation::Death;
            }
            return sauerprofile::PreviewAnimation::Idle;
        }

        int charactercount()
        {
            return sauerprofile::charactercount();
        }

        bool readcharacter(int index, Character &result)
        {
            sauerprofile::Character source = {};
            if(!sauerprofile::readcharacter(index, source))
                return false;

            result.id = source.id;
            result.name = source.name;
            result.story = source.story;
            result.icon = source.icon;
            result.ragdoll = source.ragdoll;
            return true;
        }

        bool readselection(int &result)
        {
            return sauerprofile::readselection(result);
        }

        bool writeselection(int value, int &result)
        {
            return sauerprofile::writeselection(value, result);
        }

        bool readmodelspoofing(ModelSpoofing &result)
        {
            sauerprofile::ModelSpoofing source = {};
            if(!sauerprofile::readmodelspoofing(source))
                return false;

            result.enabled = source.enabled;
            result.model = source.model;
            return true;
        }

        bool writemodelspoofingenabled(bool value, ModelSpoofing &result)
        {
            sauerprofile::ModelSpoofing accepted = {};
            if(!sauerprofile::writemodelspoofingenabled(value, accepted))
                return false;

            result.enabled = accepted.enabled;
            result.model = accepted.model;
            return true;
        }

        bool writemodelspoofingselection(int value, ModelSpoofing &result)
        {
            sauerprofile::ModelSpoofing accepted = {};
            if(!sauerprofile::writemodelspoofingselection(value, accepted))
                return false;

            result.enabled = accepted.enabled;
            result.model = accepted.model;
            return true;
        }

        bool readname(std::string &result)
        {
            char value[64] = "";
            if(!sauerprofile::readname(value, sizeof(value)))
                return false;
            result = sauertext::utf8(value);
            return true;
        }

        bool writename(const char *value, std::string &result)
        {
            const std::string encoded = sauertext::cube(value);
            char accepted[64] = "";
            if(!sauerprofile::writename(
                   encoded.c_str(), accepted, sizeof(accepted)))
                return false;
            result = sauertext::utf8(accepted);
            return true;
        }

        int savednamecount()
        {
            return sauerprofile::savednamecount();
        }

        bool readsavedname(int index, std::string &result)
        {
            char value[64] = "";
            if(!sauerprofile::readsavedname(index, value, sizeof(value)))
                return false;
            result = sauertext::utf8(value);
            return true;
        }

        bool savename(const char *value, std::string &result)
        {
            const std::string encoded = sauertext::cube(value);
            char accepted[64] = "";
            if(!sauerprofile::savename(
                   encoded.c_str(), accepted, sizeof(accepted)))
                return false;
            result = sauertext::utf8(accepted);
            return true;
        }

        bool deletename(const char *value)
        {
            const std::string encoded = sauertext::cube(value);
            return sauerprofile::deletename(encoded.c_str());
        }

        void renderpreview(
            int character,
            PreviewAnimation animation,
            float yaw,
            float zoom,
            int x,
            int y,
            int width,
            int height)
        {
            sauerprofile::renderpreview(
                character,
                legacyanimation(animation),
                yaw,
                zoom,
                x,
                y,
                width,
                height);
        }
    }
}
