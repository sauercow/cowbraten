#include "editor_api.h"
#include "sauer_editor_bridge.h"

namespace cowui
{
    namespace editor
    {
        bool readstate(State &result)
        {
            sauereditor::State source;
            if(!sauereditor::readstate(source)) return false;
            result.editing = source.editing;
            result.all_faces = source.all_faces;
            result.outline = source.outline;
            result.wireframe = source.wireframe;
            result.entity_snap = source.entity_snap;
            result.entity_editing = source.entity_editing;
            result.fullbright = source.fullbright;
            result.show_materials = source.show_materials;
            result.map_name = source.map_name ? source.map_name : "";
            result.sunlight = source.sunlight;
            result.skylight = source.skylight;
            result.ambient = source.ambient;
            result.sunlight_pitch = source.sunlight_pitch;
            result.sunlight_yaw = source.sunlight_yaw;
            result.fog_distance = source.fog_distance;
            result.fog_colour = source.fog_colour;
            return true;
        }
        bool runaction(int id) { return sauereditor::runaction(id); }
        bool settoggle(int id, bool enabled) { return sauereditor::settoggle(id, enabled); }
        bool setlighting(int id, int value) { return sauereditor::setlighting(id, value); }
        bool createentity(int id) { return sauereditor::createentity(id); }
        bool createlight(int radius, int red, int green, int blue) { return sauereditor::createlight(radius, red, green, blue); }
        bool createtrigger(int model, int behavior, int tag) { return sauereditor::createtrigger(model, behavior, tag); }
        bool applymaterial(int id) { return sauereditor::applymaterial(id); }
        bool applypostfx(int id) { return sauereditor::applypostfx(id); }

        static void addmapmodel(int id, const char *name, void *data)
        {
            MapModel row;
            row.id = id;
            row.name = name ? name : "";
            static_cast<std::vector<MapModel> *>(data)->push_back(row);
        }
        void readmapmodels(std::vector<MapModel> &result)
        {
            result.clear();
            sauereditor::visitmapmodels(addmapmodel, &result);
        }
        bool createmapmodel(int id) { return sauereditor::createmapmodel(id); }
        bool savemap(const char *name) { return sauereditor::savemap(name); }
        bool loadmap(const char *name) { return sauereditor::loadmap(name); }
        bool newmap(int size) { return sauereditor::newmap(size); }
        bool readselection(Selection &result)
        {
            sauereditor::Selection source;
            if(!sauereditor::readselection(source)) return false;
            result.available = source.available;
            result.type = source.type;
            for(int i = 0; i < 5; ++i) result.attributes[i] = source.attributes[i];
            return true;
        }
        bool applyselection(const int attributes[5]) { return sauereditor::applyselection(attributes); }
    }
}
