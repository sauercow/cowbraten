#ifndef COWBRATEN_UI_RML_DEMO_PLAYER_H
#define COWBRATEN_UI_RML_DEMO_PLAYER_H

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

namespace Rml
{
    class Context;
    class ElementDocument;
    class Event;
}

namespace cowui
{
    class RmlActionRouter;

    class RmlDemoPlayer final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);
        void update(bool menu_active);
        bool visible() const { return document_visible; }
        bool receivesinput() const;

    private:
        bool loaddocument();
        void setvisible(bool value);
        void sync();

        static void togglepause(Rml::Event &);
        static void rewind(Rml::Event &);
        static void forward(Rml::Event &);
        static void stepback(Rml::Event &);
        static void stepforward(Rml::Event &);
        static void cyclespeed(Rml::Event &);
        static void togglexray(Rml::Event &);
        static void togglehitbox(Rml::Event &);
        static void seek(Rml::Event &);
        static void stop(Rml::Event &);
        static void openlibrary(Rml::Event &);

        Rml::Context *context = nullptr;
        Rml::ElementDocument *document = nullptr;
        Rml::DataModelHandle model;
        Rml::String map_name = "Unknown map";
        Rml::String mode_name = "Unknown mode";
        Rml::String camera_name = "Free camera";
        Rml::String position_text = "0:00";
        Rml::String duration_text = "0:00";
        Rml::String speed_text = "1.00x";
        Rml::String progress_width = "0%";
        Rml::String cursor_left = "50%";
        Rml::String cursor_top = "50%";
        Rml::String cursor_size = "20px";
        int position = 0;
        int duration = 1;
        bool active = false;
        bool interactive = false;
        bool paused = false;
        bool xray_enabled = false;
        bool hitbox_enabled = false;
        bool document_visible = false;
        bool load_attempted = false;
        static RmlDemoPlayer *instance;
    };
}

#endif
