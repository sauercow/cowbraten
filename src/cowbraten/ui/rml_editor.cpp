#include "rml_editor.h"

#include "rml_action.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <cctype>

namespace cowui
{
    RmlEditor *RmlEditor::instance = nullptr;

    namespace
    {
        Rml::String lowercase(Rml::String value)
        {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return char(std::tolower(c)); });
            return value;
        }
        int red(int color) { return (color >> 16) & 0xFF; }
        int green(int color) { return (color >> 8) & 0xFF; }
        int blue(int color) { return color & 0xFF; }
        int packcolor(int r, int g, int b)
        {
            return (std::max(0, std::min(255, r)) << 16) |
                (std::max(0, std::min(255, g)) << 8) |
                std::max(0, std::min(255, b));
        }
    }

    bool RmlEditor::init(Rml::Context *context, RmlActionRouter &router)
    {
        if(!context || instance) return false;
        Rml::DataModelConstructor constructor = context->CreateDataModel("editor");
        if(!constructor) return false;
        bool valid = true;
        if(auto row = constructor.RegisterStruct<ItemRow>())
            valid = row.RegisterMember("id", &ItemRow::id) &&
                row.RegisterMember("label", &ItemRow::label) &&
                row.RegisterMember("detail", &ItemRow::detail);
        else valid = false;
        if(auto row = constructor.RegisterStruct<ModelRow>())
            valid = valid && row.RegisterMember("id", &ModelRow::id) &&
                row.RegisterMember("name", &ModelRow::name);
        else valid = false;
        if(auto row = constructor.RegisterStruct<ChoiceRow>())
            valid = valid && row.RegisterMember("value", &ChoiceRow::value) &&
                row.RegisterMember("label", &ChoiceRow::label) &&
                row.RegisterMember("selected", &ChoiceRow::selected);
        else valid = false;

        valid = valid &&
            constructor.RegisterArray<std::vector<ItemRow>>() &&
            constructor.RegisterArray<std::vector<ModelRow>>() &&
            constructor.RegisterArray<std::vector<ChoiceRow>>() &&
            constructor.Bind("edit_actions", &edit_actions) &&
            constructor.Bind("brush_actions", &brush_actions) &&
            constructor.Bind("entity_presets", &entity_presets) &&
            constructor.Bind("materials", &materials) &&
            constructor.Bind("postfx", &postfx) &&
            constructor.Bind("mapmodels", &visible_models) &&
            constructor.Bind("trigger_models", &trigger_models) &&
            constructor.Bind("trigger_behaviors", &trigger_behaviors) &&
            constructor.Bind("trigger_tags", &trigger_tags) &&
            constructor.Bind("status", &status) &&
            constructor.Bind("map_name", &map_name) &&
            constructor.Bind("model_filter", &model_filter) &&
            constructor.Bind("selection_type", &selection_type) &&
            constructor.Bind("visible_model_count", &visible_model_count) &&
            constructor.Bind("new_map_size", &new_map_size) &&
            constructor.Bind("sun_r", &sun_r) && constructor.Bind("sun_g", &sun_g) && constructor.Bind("sun_b", &sun_b) &&
            constructor.Bind("sky_r", &sky_r) && constructor.Bind("sky_g", &sky_g) && constructor.Bind("sky_b", &sky_b) &&
            constructor.Bind("ambient_r", &ambient_r) && constructor.Bind("ambient_g", &ambient_g) && constructor.Bind("ambient_b", &ambient_b) &&
            constructor.Bind("fog_r", &fog_r) && constructor.Bind("fog_g", &fog_g) && constructor.Bind("fog_b", &fog_b) &&
            constructor.Bind("sunlight_pitch", &sunlight_pitch) &&
            constructor.Bind("sunlight_yaw", &sunlight_yaw) &&
            constructor.Bind("fog_distance", &fog_distance) &&
            constructor.Bind("light_radius", &light_radius) &&
            constructor.Bind("light_r", &light_r) && constructor.Bind("light_g", &light_g) && constructor.Bind("light_b", &light_b) &&
            constructor.Bind("selection_attr0", &selection_attr0) &&
            constructor.Bind("selection_attr1", &selection_attr1) &&
            constructor.Bind("selection_attr2", &selection_attr2) &&
            constructor.Bind("selection_attr3", &selection_attr3) &&
            constructor.Bind("selection_attr4", &selection_attr4) &&
            constructor.Bind("editing", &editing) &&
            constructor.Bind("all_faces", &all_faces) &&
            constructor.Bind("outline", &outline) &&
            constructor.Bind("wireframe", &wireframe) &&
            constructor.Bind("entity_snap", &entity_snap) &&
            constructor.Bind("entity_editing", &entity_editing) &&
            constructor.Bind("fullbright", &fullbright) &&
            constructor.Bind("show_materials", &show_materials) &&
            constructor.Bind("selection_available", &selection_available) &&
            constructor.Bind("section_overview", &section_overview) &&
            constructor.Bind("section_entities", &section_entities) &&
            constructor.Bind("section_lighting", &section_lighting) &&
            constructor.Bind("section_materials", &section_materials) &&
            constructor.Bind("section_map", &section_map) &&
            constructor.Bind("section_selection", &section_selection) &&
            constructor.BindEventCallback("run_action", &RmlEditor::runaction, this) &&
            constructor.BindEventCallback("set_toggle", &RmlEditor::settoggle, this) &&
            constructor.BindEventCallback("create_entity", &RmlEditor::createentity, this) &&
            constructor.BindEventCallback("apply_material", &RmlEditor::applymaterial, this) &&
            constructor.BindEventCallback("apply_postfx", &RmlEditor::applypostfx, this) &&
            constructor.BindEventCallback("create_mapmodel", &RmlEditor::createmapmodel, this) &&
            constructor.BindEventCallback("set_model_filter", &RmlEditor::setmodelfilter, this) &&
            constructor.BindEventCallback("set_color", &RmlEditor::setcolor, this) &&
            constructor.BindEventCallback("set_numeric", &RmlEditor::setnumeric, this) &&
            constructor.BindEventCallback("set_light_value", &RmlEditor::setlightvalue, this) &&
            constructor.BindEventCallback("select_trigger_model", &RmlEditor::selecttriggermodel, this) &&
            constructor.BindEventCallback("select_trigger_behavior", &RmlEditor::selecttriggerbehavior, this) &&
            constructor.BindEventCallback("select_trigger_tag", &RmlEditor::selecttriggertag, this) &&
            constructor.BindEventCallback("set_map_name", &RmlEditor::setmapname, this) &&
            constructor.BindEventCallback("set_map_size", &RmlEditor::setmapsize, this) &&
            constructor.BindEventCallback("set_selection_attr", &RmlEditor::setselectionattr, this) &&
            router.registeraction("editor.section.overview", overviewsection) &&
            router.registeraction("editor.section.entities", entitiessection) &&
            router.registeraction("editor.section.lighting", lightingsection) &&
            router.registeraction("editor.section.materials", materialssection) &&
            router.registeraction("editor.section.map", mapsection) &&
            router.registeraction("editor.section.selection", selectionsection) &&
            router.registeraction("editor.light.create", createlightaction) &&
            router.registeraction("editor.trigger.create", createtriggeraction) &&
            router.registeraction("editor.map.save", savemapaction) &&
            router.registeraction("editor.map.load", loadmapaction) &&
            router.registeraction("editor.map.new", newmapaction) &&
            router.registeraction("editor.selection.apply", applyselectionaction) &&
            router.registeraction("editor.selection.refresh", refreshselectionaction);
        if(!valid)
        {
            context->RemoveDataModel("editor");
            return false;
        }
        model = constructor.GetModelHandle();
        instance = this;
        return true;
    }

    void RmlEditor::shutdown(Rml::Context *context)
    {
        if(instance == this) instance = nullptr;
        if(context && model) context->RemoveDataModel("editor");
        model = Rml::DataModelHandle();
        edit_actions.clear(); brush_actions.clear(); entity_presets.clear();
        materials.clear(); postfx.clear(); all_models.clear(); visible_models.clear();
        trigger_models.clear(); trigger_behaviors.clear(); trigger_tags.clear();
        catalog_built = false;
    }

    void RmlEditor::buildcatalog()
    {
        if(catalog_built) return;
        const ItemRow edits[] =
        {
            {0, "Toggle edit mode", "E"}, {1, "Undo", "U / Z"},
            {2, "Redo", "I"}, {3, "Copy", "C"}, {4, "Paste", "V"},
            {5, "Delete selection", "Delete"}, {6, "Optimize geometry", "remip"},
            {8, "Quick lighting", "No AA"}, {9, "Full lighting", "8x AA"}
        };
        edit_actions.assign(std::begin(edits), std::end(edits));
        const ItemRow brushes[] =
        {
            {15, "Diamond 1", "Sharp"}, {16, "Diamond 3", "Sharp"},
            {17, "Diamond 5", "Sharp"}, {18, "Square 3", "Hard"},
            {19, "Square 5", "Hard"}, {20, "Square 7", "Hard"},
            {21, "Smooth 3", "Soft"}, {22, "Smooth 5", "Soft"},
            {23, "Smooth 7", "Soft"}, {24, "Paint off", "Blend"},
            {25, "Replace", "Blend"}, {26, "Dig", "Blend"},
            {27, "Fill", "Blend"}, {28, "Inverted dig", "Blend"},
            {29, "Inverted fill", "Blend"}
        };
        brush_actions.assign(std::begin(brushes), std::end(brushes));
        const char *entity_labels[] =
        {
            "Shells", "Bullets", "Rockets", "Rifle rounds", "Grenades", "Cartridges",
            "Health", "Health boost", "Green armour", "Yellow armour", "Quad damage",
            "Player start", "Player start · team 1", "Player start · team 2", "Base",
            "Flag · team 1", "Flag · team 2", "Teleport 1", "Teleport destination 1",
            "Teleport 2", "Teleport destination 2", "Teleport 3", "Teleport destination 3",
            "Teleport 4", "Teleport destination 4", "Environment map", "Jump pad",
            "Sound", "Particles", "Ogro · fireball", "Rhino · chaingun",
            "Ratamahatta · shotgun", "Slith · rifle", "Bauul · rockets",
            "Hellpig · bite", "Knight · iceball", "Goblin · slimeball",
            "Spider · grenade", "Spotlight"
        };
        for(int i = 0; i < int(sizeof(entity_labels) / sizeof(entity_labels[0])); ++i)
            entity_presets.push_back({ i, entity_labels[i], i < 11 ? "Pickup" : (i < 29 ? "World" : "Monster") });
        const char *material_labels[] = { "Air", "Water", "Lava", "Clip", "Glass", "No clip", "Game clip", "Death", "Alpha" };
        for(int i = 0; i < 9; ++i) materials.push_back({ i, material_labels[i], "Selection material" });
        const char *effect_labels[] = { "Off", "Bloom · subtle", "Bloom · bright", "Bloom · intense", "Rotoscope", "Rotoscope + blur 3", "Rotoscope + blur 5", "Sobel", "Invert", "GBR", "Black & white", "Blur 3", "Blur 5" };
        for(int i = 0; i < 13; ++i) postfx.push_back({ i, effect_labels[i], "Post effect" });

        const int trigger_model_values[] = { -1, 23, 24, 25, 26, 27, 28 };
        const char *trigger_model_labels[] = { "Invisible", "Carrot", "Switch 1", "Switch 2", "Door 1", "Door 2", "Door 3" };
        for(int i = 0; i < 7; ++i) trigger_models.push_back({ trigger_model_values[i], trigger_model_labels[i], false });
        const int trigger_behavior_values[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,29 };
        const char *trigger_behavior_labels[] = { "Animation loop", "Once · popback", "Once · popback + rumble", "Once", "Once · rumble", "Multiple · popback", "Multiple · popback + rumble", "Multiple", "Multiple · rumble", "Door · open once", "Door · auto close", "Door · locked", "Vanishing", "Vanishing · rumble", "End of level" };
        for(int i = 0; i < 15; ++i) trigger_behaviors.push_back({ trigger_behavior_values[i], trigger_behavior_labels[i], false });
        for(int i = 0; i <= 8; ++i) trigger_tags.push_back({ i, "Tag " + Rml::ToString(i), false });

        std::vector<editor::MapModel> source_models;
        editor::readmapmodels(source_models);
        for(const editor::MapModel &source : source_models)
            all_models.push_back({ source.id, source.name });
        rebuildmodels();
        syncchoices();
        catalog_built = true;
        model.DirtyVariable("edit_actions"); model.DirtyVariable("brush_actions");
        model.DirtyVariable("entity_presets"); model.DirtyVariable("materials");
        model.DirtyVariable("postfx"); model.DirtyVariable("trigger_models");
        model.DirtyVariable("trigger_behaviors"); model.DirtyVariable("trigger_tags");
    }

    void RmlEditor::refresh()
    {
        editor::State state;
        if(editor::readstate(state))
        {
            editing = state.editing; all_faces = state.all_faces; outline = state.outline;
            wireframe = state.wireframe; entity_snap = state.entity_snap;
            entity_editing = state.entity_editing; fullbright = state.fullbright;
            show_materials = state.show_materials;
            if(map_name.empty()) map_name = state.map_name;
            sun_r = red(state.sunlight); sun_g = green(state.sunlight); sun_b = blue(state.sunlight);
            sky_r = red(state.skylight); sky_g = green(state.skylight); sky_b = blue(state.skylight);
            ambient_r = red(state.ambient); ambient_g = green(state.ambient); ambient_b = blue(state.ambient);
            fog_r = red(state.fog_colour); fog_g = green(state.fog_colour); fog_b = blue(state.fog_colour);
            sunlight_pitch = state.sunlight_pitch; sunlight_yaw = state.sunlight_yaw;
            fog_distance = state.fog_distance;
        }
        editor::Selection selection;
        editor::readselection(selection);
        selection_available = selection.available;
        selection_type = selection.available ? selection.type : "No entity selected";
        selection_attr0 = selection.attributes[0]; selection_attr1 = selection.attributes[1];
        selection_attr2 = selection.attributes[2]; selection_attr3 = selection.attributes[3];
        selection_attr4 = selection.attributes[4];
        const char *variables[] = { "editing", "all_faces", "outline", "wireframe", "entity_snap", "entity_editing", "fullbright", "show_materials", "map_name", "sun_r", "sun_g", "sun_b", "sky_r", "sky_g", "sky_b", "ambient_r", "ambient_g", "ambient_b", "fog_r", "fog_g", "fog_b", "sunlight_pitch", "sunlight_yaw", "fog_distance", "selection_available", "selection_type", "selection_attr0", "selection_attr1", "selection_attr2", "selection_attr3", "selection_attr4" };
        for(const char *variable : variables) model.DirtyVariable(variable);
    }

    void RmlEditor::activate(int requested_section)
    {
        buildcatalog();
        refresh();
        if(requested_section >= 0) setsection(requested_section);
        status = editing ? "Edit mode is active." : "Enable edit mode before modifying the map.";
        model.DirtyVariable("status");
    }

    void RmlEditor::setsection(int value)
    {
        section = std::max(0, std::min(5, value));
        section_overview = section == 0; section_entities = section == 1;
        section_lighting = section == 2; section_materials = section == 3;
        section_map = section == 4; section_selection = section == 5;
        model.DirtyVariable("section_overview"); model.DirtyVariable("section_entities");
        model.DirtyVariable("section_lighting"); model.DirtyVariable("section_materials");
        model.DirtyVariable("section_map"); model.DirtyVariable("section_selection");
    }

    void RmlEditor::rebuildmodels()
    {
        visible_models.clear();
        const Rml::String needle = lowercase(model_filter);
        for(const ModelRow &row : all_models)
            if(needle.empty() || lowercase(row.name).find(needle) != Rml::String::npos)
                visible_models.push_back(row);
        visible_model_count = int(visible_models.size());
        model.DirtyVariable("mapmodels"); model.DirtyVariable("visible_model_count");
    }

    void RmlEditor::syncchoices()
    {
        for(ChoiceRow &row : trigger_models) row.selected = row.value == trigger_model;
        for(ChoiceRow &row : trigger_behaviors) row.selected = row.value == trigger_behavior;
        for(ChoiceRow &row : trigger_tags) row.selected = row.value == trigger_tag;
        model.DirtyVariable("trigger_models"); model.DirtyVariable("trigger_behaviors");
        model.DirtyVariable("trigger_tags");
    }

    void RmlEditor::setstatus(bool ok, const char *success, const char *failure)
    {
        status = ok ? success : failure;
        model.DirtyVariable("status");
    }

    void RmlEditor::runaction(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        const bool ok = args.size() == 1 && editor::runaction(args[0].Get<int>(-1));
        setstatus(ok, "Editor action applied.", "This editor action is unavailable."); refresh(); event.StopPropagation();
    }
    void RmlEditor::settoggle(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        const bool ok = args.size() == 2 && editor::settoggle(args[0].Get<int>(-1), args[1].Get<bool>());
        setstatus(ok, "Editor option updated.", "Editor option unavailable."); refresh(); event.StopPropagation();
    }
    void RmlEditor::createentity(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        const bool ok = args.size() == 1 && editor::createentity(args[0].Get<int>(-1));
        setstatus(ok, "Entity created at the configured drop position.", "Enable edit mode before creating entities."); refresh(); event.StopPropagation();
    }
    void RmlEditor::applymaterial(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        const bool ok = args.size() == 1 && editor::applymaterial(args[0].Get<int>(-1));
        setstatus(ok, "Material applied to the selection.", "Select geometry in edit mode first."); event.StopPropagation();
    }
    void RmlEditor::applypostfx(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        const bool ok = args.size() == 1 && editor::applypostfx(args[0].Get<int>(-1));
        setstatus(ok, "Post-processing preset applied.", "Post-processing preset unavailable."); event.StopPropagation();
    }
    void RmlEditor::createmapmodel(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        const bool ok = args.size() == 1 && editor::createmapmodel(args[0].Get<int>(-1));
        setstatus(ok, "Map model created.", "Enable edit mode or choose an available model."); refresh(); event.StopPropagation();
    }
    void RmlEditor::setmodelfilter(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        if(args.size() == 1) { model_filter = args[0].Get<Rml::String>(); model.DirtyVariable("model_filter"); rebuildmodels(); }
        event.StopPropagation();
    }
    void RmlEditor::setcolor(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        if(args.size() == 3)
        {
            const int kind = args[0].Get<int>(-1), channel = args[1].Get<int>(-1);
            const int value = std::max(0, std::min(255, args[2].Get<int>()));
            int *channels[] = { &sun_r,&sun_g,&sun_b,&sky_r,&sky_g,&sky_b,&ambient_r,&ambient_g,&ambient_b,&fog_r,&fog_g,&fog_b };
            const int index = kind * 3 + channel;
            if(kind >= 0 && kind < 4 && channel >= 0 && channel < 3)
            {
                *channels[index] = value;
                const int base = kind * 3;
                editor::setlighting(kind == 3 ? 6 : kind, packcolor(*channels[base], *channels[base+1], *channels[base+2]));
                const char *names[] = { "sun_r","sun_g","sun_b","sky_r","sky_g","sky_b","ambient_r","ambient_g","ambient_b","fog_r","fog_g","fog_b" };
                model.DirtyVariable(names[index]);
            }
        }
        event.StopPropagation();
    }
    void RmlEditor::setnumeric(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        if(args.size() == 2)
        {
            const int id = args[0].Get<int>(-1), value = args[1].Get<int>();
            if(editor::setlighting(id, value))
            {
                if(id == 3) sunlight_pitch = value;
                else if(id == 4) sunlight_yaw = value;
                else if(id == 5) fog_distance = value;
            }
        }
        event.StopPropagation();
    }
    void RmlEditor::setlightvalue(Rml::DataModelHandle, Rml::Event &event, const Rml::VariantList &args)
    {
        if(args.size() == 2)
        {
            int *values[] = { &light_radius, &light_r, &light_g, &light_b };
            const int id = args[0].Get<int>(-1);
            if(id >= 0 && id < 4)
            {
                *values[id] = args[1].Get<int>();
                const char *names[] = { "light_radius", "light_r", "light_g", "light_b" };
                model.DirtyVariable(names[id]);
            }
        }
        event.StopPropagation();
    }
    void RmlEditor::selecttriggermodel(Rml::DataModelHandle, Rml::Event &e, const Rml::VariantList &a) { if(a.size()==1) { trigger_model=a[0].Get<int>(); syncchoices(); } e.StopPropagation(); }
    void RmlEditor::selecttriggerbehavior(Rml::DataModelHandle, Rml::Event &e, const Rml::VariantList &a) { if(a.size()==1) { trigger_behavior=a[0].Get<int>(); syncchoices(); } e.StopPropagation(); }
    void RmlEditor::selecttriggertag(Rml::DataModelHandle, Rml::Event &e, const Rml::VariantList &a) { if(a.size()==1) { trigger_tag=a[0].Get<int>(); syncchoices(); } e.StopPropagation(); }
    void RmlEditor::setmapname(Rml::DataModelHandle, Rml::Event &e, const Rml::VariantList &a) { if(a.size()==1) { map_name=a[0].Get<Rml::String>(); model.DirtyVariable("map_name"); } e.StopPropagation(); }
    void RmlEditor::setmapsize(Rml::DataModelHandle, Rml::Event &e, const Rml::VariantList &a) { if(a.size()==1) { new_map_size=std::max(10,std::min(16,a[0].Get<int>())); model.DirtyVariable("new_map_size"); } e.StopPropagation(); }
    void RmlEditor::setselectionattr(Rml::DataModelHandle, Rml::Event &e, const Rml::VariantList &a)
    {
        if(a.size()==2)
        {
            int *values[] = { &selection_attr0,&selection_attr1,&selection_attr2,&selection_attr3,&selection_attr4 };
            const int id=a[0].Get<int>(-1); if(id>=0 && id<5) { *values[id]=a[1].Get<int>(); const char *names[]={"selection_attr0","selection_attr1","selection_attr2","selection_attr3","selection_attr4"}; model.DirtyVariable(names[id]); }
        }
        e.StopPropagation();
    }

#define SECTION_ACTION(name, value) void RmlEditor::name(Rml::Event &e) { if(instance) instance->setsection(value); e.StopPropagation(); }
    SECTION_ACTION(overviewsection, 0) SECTION_ACTION(entitiessection, 1)
    SECTION_ACTION(lightingsection, 2) SECTION_ACTION(materialssection, 3)
    SECTION_ACTION(mapsection, 4) SECTION_ACTION(selectionsection, 5)
#undef SECTION_ACTION
    void RmlEditor::createlightaction(Rml::Event &e) { if(instance) instance->setstatus(editor::createlight(instance->light_radius,instance->light_r,instance->light_g,instance->light_b),"Light created.","Enable edit mode before creating a light."); e.StopPropagation(); }
    void RmlEditor::createtriggeraction(Rml::Event &e) { if(instance) instance->setstatus(editor::createtrigger(instance->trigger_model,instance->trigger_behavior,instance->trigger_tag),"Trigger created.","Enable edit mode and choose a valid trigger."); e.StopPropagation(); }
    void RmlEditor::savemapaction(Rml::Event &e) { if(instance) instance->setstatus(editor::savemap(instance->map_name.c_str()),"Map saved.","Use a safe relative map name while editing."); e.StopPropagation(); }
    void RmlEditor::loadmapaction(Rml::Event &e) { if(instance) instance->setstatus(editor::loadmap(instance->map_name.c_str()),"Map load requested.","Use a valid installed map name."); e.StopPropagation(); }
    void RmlEditor::newmapaction(Rml::Event &e) { if(instance) instance->setstatus(editor::newmap(instance->new_map_size),"New map created.","Map size must be between 10 and 16."); e.StopPropagation(); }
    void RmlEditor::applyselectionaction(Rml::Event &e) { if(instance) { const int attrs[]={instance->selection_attr0,instance->selection_attr1,instance->selection_attr2,instance->selection_attr3,instance->selection_attr4}; instance->setstatus(editor::applyselection(attrs),"Selected entity updated.","Select an entity in edit mode first."); instance->refresh(); } e.StopPropagation(); }
    void RmlEditor::refreshselectionaction(Rml::Event &e) { if(instance) instance->refresh(); e.StopPropagation(); }
}
