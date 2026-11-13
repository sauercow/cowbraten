#ifndef COWBRATEN_UI_EDITOR_API_H
#define COWBRATEN_UI_EDITOR_API_H

#include <string>
#include <vector>

namespace cowui
{
    namespace editor
    {
        struct State
        {
            bool editing = false, all_faces = false, outline = false;
            bool wireframe = false, entity_snap = false;
            bool entity_editing = false, fullbright = false;
            bool show_materials = false;
            std::string map_name;
            int sunlight = 0, skylight = 0, ambient = 0;
            int sunlight_pitch = 0, sunlight_yaw = 0;
            int fog_distance = 0, fog_colour = 0;
        };
        struct Selection
        {
            bool available = false;
            std::string type;
            int attributes[5] = { 0, 0, 0, 0, 0 };
        };
        struct MapModel { int id = 0; std::string name; };

        bool readstate(State &result);
        bool runaction(int id);
        bool settoggle(int id, bool enabled);
        bool setlighting(int id, int value);
        bool createentity(int id);
        bool createlight(int radius, int red, int green, int blue);
        bool createtrigger(int model, int behavior, int tag);
        bool applymaterial(int id);
        bool applypostfx(int id);
        void readmapmodels(std::vector<MapModel> &result);
        bool createmapmodel(int id);
        bool savemap(const char *name);
        bool loadmap(const char *name);
        bool newmap(int size);
        bool readselection(Selection &result);
        bool applyselection(const int attributes[5]);
    }
}

#endif
