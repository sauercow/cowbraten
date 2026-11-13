#include "engine.h"
#include "game.h"
#include "sauer_profile_bridge.h"

namespace cowui
{
    namespace sauerprofile
    {
        namespace
        {
            static const char *name_slots[] =
            {
                "profile_name_1",
                "profile_name_2",
                "profile_name_3",
                "profile_name_4",
                "profile_name_5"
            };

            static const Character characters[] =
            {
                { 0, "Mr. Fixit", "A resourceful combat engineer built for every arena.", "?cowbraten-resource:packages/icons/mrfixit.jpg", true },
                { 1, "Iron Snout", "Fast, compact, and always ready for close-range trouble.", "?cowbraten-resource:packages/icons/snoutx10k.jpg", true },
                { 2, "Ogro", "Heavy armor and an unmistakable old-school silhouette.", "?cowbraten-resource:packages/icons/ogro.jpg", true },
                { 3, "Inky", "A bright arena veteran with a compact tactical profile.", "?cowbraten-resource:packages/icons/inky.jpg", true },
                { 4, "Captain Cannon", "A seasoned fighter prepared for sustained combat.", "?cowbraten-resource:packages/icons/captaincannon.jpg", true }
            };

            static PreviewAnimation last_animation = PreviewAnimation::Idle;
            static int animation_started = 0;

            static bool readslot(int slot, char *result, int result_size)
            {
                if(!result || result_size <= 0 || slot < 0 ||
                   slot >= int(sizeof(name_slots) / sizeof(name_slots[0])))
                    return false;

                ident *id = getident(name_slots[slot]);
                if(!id || id->type != ID_SVAR || !id->storage.s || !*id->storage.s)
                    return false;

                copystring(result, *id->storage.s, result_size);
                return result[0] != '\0';
            }

            static bool writeslot(int slot, const char *value)
            {
                if(slot < 0 || slot >= int(sizeof(name_slots) / sizeof(name_slots[0])))
                    return false;

                ident *id = getident(name_slots[slot]);
                if(!id || id->type != ID_SVAR || !id->storage.s)
                    return false;

                setsvarchecked(id, value ? value : "");
                return true;
            }

            static bool readintident(const char *name, int &result)
            {
                ident *id = getident(name);
                if(!id || id->type != ID_VAR || !id->storage.i)
                    return false;

                result = *id->storage.i;
                return true;
            }

            static bool writeintident(const char *name, int value)
            {
                ident *id = getident(name);
                if(!id || id->type != ID_VAR || !id->storage.i)
                    return false;

                if(*id->storage.i != value)
                    setvarchecked(id, value);
                return true;
            }
        }

        int charactercount()
        {
            return int(sizeof(characters) / sizeof(characters[0]));
        }

        bool readcharacter(int index, Character &result)
        {
            if(index < 0 || index >= charactercount())
                return false;

            result = characters[index];
            return true;
        }

        bool readselection(int &result)
        {
            ident *id = getident("playermodel");
            if(!id || id->type != ID_VAR || !id->storage.i)
                return false;

            result = clamp(*id->storage.i, 0, charactercount() - 1);
            return true;
        }

        bool writeselection(int value, int &result)
        {
            ident *id = getident("playermodel");
            if(!id || id->type != ID_VAR || !id->storage.i ||
               value < 0 || value >= charactercount())
            {
                return false;
            }

            if(*id->storage.i != value)
                setvarchecked(id, value);

            return readselection(result);
        }

        bool readmodelspoofing(ModelSpoofing &result)
        {
            int enabled = 0, model = 0;
            if(!readintident("modelspoofing", enabled) ||
               !readintident("spoofplayermodel", model))
            {
                return false;
            }

            result.enabled = enabled != 0;
            result.model = clamp(model, 0, charactercount() - 1);
            return true;
        }

        bool writemodelspoofingenabled(bool value, ModelSpoofing &result)
        {
            if(!writeintident("modelspoofing", value ? 1 : 0) ||
               !readmodelspoofing(result))
            {
                return false;
            }

            writecfg();
            return true;
        }

        bool writemodelspoofingselection(int value, ModelSpoofing &result)
        {
            if(value < 0 || value >= charactercount() ||
               !writeintident("spoofplayermodel", value) ||
               !readmodelspoofing(result))
            {
                return false;
            }

            writecfg();
            return true;
        }

        bool readname(char *result, int result_size)
        {
            if(!game::player1 || !result || result_size <= 0)
                return false;

            copystring(result, game::player1->name, result_size);
            return true;
        }

        bool writename(const char *value, char *result, int result_size)
        {
            if(!game::player1 || !value || !result || result_size <= 0)
                return false;

            game::switchname(value);
            return readname(result, result_size);
        }

        int savednamecount()
        {
            int count = 0;
            char value[MAXNAMELEN + 1] = "";
            for(int slot = 0; slot < int(sizeof(name_slots) / sizeof(name_slots[0])); ++slot)
                if(readslot(slot, value, sizeof(value))) ++count;
            return count;
        }

        bool readsavedname(int index, char *result, int result_size)
        {
            if(index < 0 || !result || result_size <= 0)
                return false;

            int visible = 0;
            for(int slot = 0; slot < int(sizeof(name_slots) / sizeof(name_slots[0])); ++slot)
            {
                char value[MAXNAMELEN + 1] = "";
                if(!readslot(slot, value, sizeof(value)))
                    continue;
                if(visible++ == index)
                {
                    copystring(result, value, result_size);
                    return true;
                }
            }
            return false;
        }

        bool savename(const char *value, char *result, int result_size)
        {
            if(!writename(value, result, result_size) || !result[0])
                return false;

            const int slot_count = int(sizeof(name_slots) / sizeof(name_slots[0]));
            int first_empty = -1;
            for(int slot = 0; slot < slot_count; ++slot)
            {
                char existing[MAXNAMELEN + 1] = "";
                if(readslot(slot, existing, sizeof(existing)))
                {
                    if(!strcmp(existing, result))
                    {
                        writecfg();
                        return true;
                    }
                }
                else if(first_empty < 0)
                    first_empty = slot;
            }

            if(first_empty >= 0)
            {
                if(!writeslot(first_empty, result))
                    return false;
                writecfg();
                return true;
            }

            for(int slot = 0; slot < slot_count - 1; ++slot)
            {
                char next[MAXNAMELEN + 1] = "";
                if(!readslot(slot + 1, next, sizeof(next)) || !writeslot(slot, next))
                    return false;
            }
            if(!writeslot(slot_count - 1, result))
                return false;
            writecfg();
            return true;
        }

        bool deletename(const char *value)
        {
            if(!value || !value[0])
                return false;

            const int slot_count = int(sizeof(name_slots) / sizeof(name_slots[0]));
            char retained[sizeof(name_slots) / sizeof(name_slots[0])][MAXNAMELEN + 1] = {};
            int retained_count = 0;
            bool removed = false;

            for(int slot = 0; slot < slot_count; ++slot)
            {
                char existing[MAXNAMELEN + 1] = "";
                if(!readslot(slot, existing, sizeof(existing)))
                    continue;
                if(!removed && !strcmp(existing, value))
                {
                    removed = true;
                    continue;
                }
                copystring(
                    retained[retained_count++],
                    existing,
                    sizeof(retained[0]));
            }

            if(!removed)
                return false;

            for(int slot = 0; slot < slot_count; ++slot)
                if(!writeslot(
                       slot,
                       slot < retained_count ? retained[slot] : ""))
                    return false;

            writecfg();
            return true;
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
            if(character < 0 || character >= charactercount() ||
               width <= 1 || height <= 1)
            {
                return;
            }

            if(animation != last_animation)
            {
                last_animation = animation;
                animation_started = lastmillis;
            }

            modelpreview::start(x, y, width, height, true);
            game::renderplayerpreview(
                character,
                0,
                GUN_RIFLE,
                int(animation),
                animation_started,
                yaw,
                clamp(zoom, 0.70f, 1.45f));
            modelpreview::end();

            hudshader->set();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
        }
    }
}
