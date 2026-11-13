#ifndef COWBRATEN_UI_SAUER_EDITOR_BRIDGE_H
#define COWBRATEN_UI_SAUER_EDITOR_BRIDGE_H

namespace cowui
{
    namespace sauereditor
    {
        struct State
        {
            bool editing;
            bool all_faces;
            bool outline;
            bool wireframe;
            bool entity_snap;
            bool entity_editing;
            bool fullbright;
            bool show_materials;
            const char *map_name;
            int sunlight;
            int skylight;
            int ambient;
            int sunlight_pitch;
            int sunlight_yaw;
            int fog_distance;
            int fog_colour;
        };

        struct Selection
        {
            bool available;
            char type[32];
            int attributes[5];
        };

        typedef void (*MapModelVisitor)(int id, const char *name, void *data);

        bool readstate(State &result);
        bool runaction(int id);
        bool settoggle(int id, bool enabled);
        bool setlighting(int id, int value);
        bool createentity(int id);
        bool createlight(int radius, int red, int green, int blue);
        bool createtrigger(int model, int behavior, int tag);
        bool applymaterial(int id);
        bool applypostfx(int id);
        void visitmapmodels(MapModelVisitor visitor, void *data);
        bool createmapmodel(int id);
        bool savemap(const char *name);
        bool loadmap(const char *name);
        bool newmap(int size);
        bool readselection(Selection &result);
        bool applyselection(const int attributes[5]);
    }
}

#endif
