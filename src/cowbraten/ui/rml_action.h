#ifndef COWBRATEN_UI_RML_ACTION_H
#define COWBRATEN_UI_RML_ACTION_H

#include <RmlUi/Core/EventListenerInstancer.h>

#include <vector>

namespace Rml
{
    class Event;
}

namespace cowui
{
    typedef void (*RmlActionHandler)(Rml::Event &event);

    class RmlActionRouter final : public Rml::EventListenerInstancer
    {
    public:
        bool registeraction(
            const char *name,
            RmlActionHandler handler
        );

        void clear();
        void install();

        Rml::EventListener *InstanceEventListener(
            const Rml::String &value,
            Rml::Element *element) override;

    private:
        struct Action
        {
            Rml::String name;
            RmlActionHandler handler;
        };

        RmlActionHandler find(
            const Rml::String &name) const;

        std::vector<Action> actions;
    };
}

#endif
