#ifndef COWBRATEN_UI_RML_SETTINGS_H
#define COWBRATEN_UI_RML_SETTINGS_H

#include <RmlUi/Core/DataModelHandle.h>

namespace Rml
{
    class Context;
    class Event;
}

namespace cowui
{
    class RmlActionRouter;

    class RmlSettings final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);

        void activate();

    private:
        bool refresh();
        static void changefovaction(Rml::Event &event);

        Rml::DataModelHandle model;
        int fov = 100;
        int fov_minimum = 10;
        int fov_maximum = 150;

        static RmlSettings *instance;
    };
}

#endif
