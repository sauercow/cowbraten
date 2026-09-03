#include "rml_action.h"

#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Factory.h>

#include <cstdio>

namespace cowui
{
    class RmlActionListener final : public Rml::EventListener
    {
    public:
        explicit RmlActionListener(RmlActionHandler handler) :
            handler(handler)
        {
        }

        void ProcessEvent(Rml::Event &event) override
        {
            handler(event);
        }

        void OnDetach(Rml::Element *) override
        {
            delete this;
        }

    private:
        RmlActionHandler handler;
    };

    bool RmlActionRouter::registeraction(
        const char *name,
        RmlActionHandler handler)
    {
        if(!name || !name[0] || !handler || find(name))
            return false;

        Action action;
        action.name = name;
        action.handler = handler;
        actions.push_back(action);
        return true;
    }

    void RmlActionRouter::clear()
    {
        actions.clear();
    }

    void RmlActionRouter::install()
    {
        Rml::Factory::RegisterEventListenerInstancer(this);
    }

    Rml::EventListener *RmlActionRouter::InstanceEventListener(
        const Rml::String &value,
        Rml::Element *)
    {
        RmlActionHandler handler = find(value);

        if(!handler)
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: rejected unknown action '%s'\n",
                value.c_str()
            );
            return nullptr;
        }

        return new RmlActionListener(handler);
    }

    RmlActionHandler RmlActionRouter::find(
        const Rml::String &name) const
    {
        for(std::vector<Action>::const_iterator action = actions.begin();
            action != actions.end();
            ++action)
        {
            if(action->name == name)
                return action->handler;
        }

        return nullptr;
    }
}
