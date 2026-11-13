#include "engine.h"
#include "game.h"
#include "sauer_editor_bridge.h"

#include <cctype>
#include <cstring>

extern void entset(char *what, int *a1, int *a2, int *a3, int *a4, int *a5);

namespace cowui
{
    namespace sauereditor
    {
        namespace
        {
            const char *const action_commands[] =
            {
                "edittoggle", "undo", "redo", "copy", "paste", "delcube",
                "remip", "mapenlarge", "calclight -1", "calclight 1",
                "getsundir", "clearblendmap", "clearblendmapsel",
                "nextblendbrush 1", "rotateblendbrush 1",
                "brush_0", "brush_1", "brush_2", "brush_3", "brush_4",
                "brush_5", "brush_6", "brush_7", "brush_8",
                "blendpaintmode 0", "blendpaintmode 1", "blendpaintmode 2",
                "blendpaintmode 3", "blendpaintmode 4", "blendpaintmode 5"
            };

            const char *const toggle_names[] =
            {
                "allfaces", "outline", "wireframe", "entselsnap",
                "entediting", "fullbright", "showmat"
            };

            const char *const lighting_names[] =
            {
                "sunlight", "skylight", "ambient", "sunlightpitch",
                "sunlightyaw", "fog", "fogcolour"
            };

            const char *const entity_commands[] =
            {
                "newent shells", "newent bullets", "newent rockets",
                "newent riflerounds", "newent grenades", "newent cartridges",
                "newent health", "newent healthboost", "newent greenarmour",
                "newent yellowarmour", "newent quaddamage",
                "newent playerstart", "newent playerstart 1",
                "newent playerstart 2", "newent base", "newent flag 1",
                "newent flag 2", "newent teleport 1", "newent teledest 1",
                "newent teleport 2", "newent teledest 2", "newent teleport 3",
                "newent teledest 3", "newent teleport 4", "newent teledest 4",
                "newent envmap", "newent jumppad", "newent sound",
                "newent particles", "newent monster 0", "newent monster 1",
                "newent monster 2", "newent monster 3", "newent monster 4",
                "newent monster 5", "newent monster 6", "newent monster 7",
                "newent monster 8", "newent spotlight"
            };

            const char *const material_commands[] =
            {
                "editmat air", "editmat water", "editmat lava", "editmat clip",
                "editmat glass", "editmat noclip", "editmat gameclip",
                "editmat death", "editmat alpha"
            };

            const char *const postfx_commands[] =
            {
                "clearpostfx", "bloom 0.3", "bloom 0.55", "bloom 0.8",
                "rotoscope 1", "rotoscope 1 1", "rotoscope 1 2",
                "setpostfx sobel", "setpostfx invert", "setpostfx gbr",
                "setpostfx bw", "setpostfx hblur3; addpostfx vblur3",
                "setpostfx hblur5; addpostfx vblur5"
            };

            ident *intident(const char *name)
            {
                ident *id = getident(name);
                return id && id->type == ID_VAR ? id : nullptr;
            }

            int readint(const char *name, int fallback = 0)
            {
                ident *id = intident(name);
                return id ? *id->storage.i : fallback;
            }

            bool writeint(const char *name, int value)
            {
                ident *id = intident(name);
                if(!id) return false;
                value = clamp(value, id->minval, id->maxval);
                if(*id->storage.i == value) return true;
                setvarchecked(id, value);
                return true;
            }

            bool validmapname(const char *name)
            {
                if(!name || !name[0] || name[0] == '/' || std::strstr(name, ".."))
                    return false;
                for(const unsigned char *c = (const unsigned char *)name; *c; ++c)
                    if(!std::isalnum(*c) && *c != '_' && *c != '-' && *c != '/')
                        return false;
                return true;
            }

            bool validtriggerbehavior(int value)
            {
                return (value >= 0 && value <= 13) || value == 29;
            }

            bool validtriggermodel(int value)
            {
                return value == -1 || (value >= 23 && value <= 28);
            }
        }

        bool readstate(State &result)
        {
            result.editing = editmode;
            result.all_faces = readint("allfaces") != 0;
            result.outline = readint("outline") != 0;
            result.wireframe = readint("wireframe") != 0;
            result.entity_snap = readint("entselsnap") != 0;
            result.entity_editing = readint("entediting") != 0;
            result.fullbright = readint("fullbright") != 0;
            result.show_materials = readint("showmat") != 0;
            result.map_name = game::getclientmap();
            result.sunlight = readint("sunlight");
            result.skylight = readint("skylight");
            result.ambient = readint("ambient");
            result.sunlight_pitch = readint("sunlightpitch");
            result.sunlight_yaw = readint("sunlightyaw");
            result.fog_distance = readint("fog");
            result.fog_colour = readint("fogcolour");
            return true;
        }

        bool runaction(int id)
        {
            if(id < 0 || id >= int(sizeof(action_commands) / sizeof(action_commands[0])))
                return false;
            execute(action_commands[id]);
            return true;
        }

        bool settoggle(int id, bool enabled)
        {
            if(id < 0 || id >= int(sizeof(toggle_names) / sizeof(toggle_names[0])))
                return false;
            return writeint(toggle_names[id], enabled ? 1 : 0);
        }

        bool setlighting(int id, int value)
        {
            if(id < 0 || id >= int(sizeof(lighting_names) / sizeof(lighting_names[0])))
                return false;
            if(id <= 2 || id == 6) value = clamp(value, 0, 0xFFFFFF);
            else if(id == 3) value = clamp(value, -90, 90);
            else if(id == 4) value = clamp(value, 0, 360);
            else if(id == 5) value = clamp(value, 128, 8192);
            return writeint(lighting_names[id], value);
        }

        bool createentity(int id)
        {
            if(!editmode || id < 0 || id >= int(sizeof(entity_commands) / sizeof(entity_commands[0])))
                return false;
            execute(entity_commands[id]);
            return true;
        }

        bool createlight(int radius, int red, int green, int blue)
        {
            if(!editmode) return false;
            defformatstring(command, "newent light %d %d %d %d",
                clamp(radius, 0, 512), clamp(red, 0, 255),
                clamp(green, 0, 255), clamp(blue, 0, 255));
            execute(command);
            return true;
        }

        bool createtrigger(int model, int behavior, int tag)
        {
            if(!editmode || !validtriggermodel(model) ||
               !validtriggerbehavior(behavior) || tag < 0 || tag > 8)
                return false;
            defformatstring(command, "newent mapmodel %d %d %d",
                model, behavior, tag);
            execute(command);
            return true;
        }

        bool applymaterial(int id)
        {
            if(!editmode || id < 0 || id >= int(sizeof(material_commands) / sizeof(material_commands[0])))
                return false;
            execute(material_commands[id]);
            return true;
        }

        bool applypostfx(int id)
        {
            if(id < 0 || id >= int(sizeof(postfx_commands) / sizeof(postfx_commands[0])))
                return false;
            execute(postfx_commands[id]);
            return true;
        }

        void visitmapmodels(MapModelVisitor visitor, void *data)
        {
            if(!visitor) return;
            for(int i = 0; i < 4096; ++i)
            {
                const char *name = mapmodelname(i);
                if(!name) break;
                visitor(i, name, data);
            }
        }

        bool createmapmodel(int id)
        {
            if(!editmode || id < 0 || !mapmodelname(id)) return false;
            defformatstring(command, "newent mapmodel %d", id);
            execute(command);
            return true;
        }

        bool savemap(const char *name)
        {
            if(!editmode || !validmapname(name)) return false;
            defformatstring(command, "savemap %s", name);
            execute(command);
            return true;
        }

        bool loadmap(const char *name)
        {
            if(!validmapname(name)) return false;
            defformatstring(command, "map %s", name);
            execute(command);
            return true;
        }

        bool newmap(int size)
        {
            if(size < 10 || size > 16) return false;
            defformatstring(command, "newmap %d", size);
            execute(command);
            return true;
        }

        bool readselection(Selection &result)
        {
            result.available = false;
            result.type[0] = '\0';
            for(int &attribute : result.attributes) attribute = 0;
            vector<extentity *> &entities_list = entities::getents();
            if(entgroup.empty() || !entities_list.inrange(entgroup.last()))
                return true;
            extentity &entity = *entities_list[entgroup.last()];
            if(entity.type == ET_EMPTY) return true;
            result.available = true;
            copystring(result.type, entities::entname(entity.type), sizeof(result.type));
            result.attributes[0] = entity.attr1;
            result.attributes[1] = entity.attr2;
            result.attributes[2] = entity.attr3;
            result.attributes[3] = entity.attr4;
            result.attributes[4] = entity.attr5;
            return true;
        }

        bool applyselection(const int attributes[5])
        {
            if(!editmode || !attributes) return false;
            Selection selection;
            if(!readselection(selection) || !selection.available) return false;
            string type;
            copystring(type, selection.type);
            int values[5];
            loopi(5) values[i] = attributes[i];
            entset(type, &values[0], &values[1], &values[2], &values[3], &values[4]);
            return true;
        }
    }
}
