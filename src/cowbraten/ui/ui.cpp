#include "ui.h"
#include "rml_render.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>

namespace cowui
{
    static RmlRenderInterface render_interface;
    static Rml::Context *context = nullptr;

    static bool initialized = false;
    static bool shown = false;

    static int valid_dimension(int value)
    {
        return value > 0 ? value : 1;
    }

    bool init(int width, int height)
    {
        if(initialized)
            return true;

        width = valid_dimension(width);
        height = valid_dimension(height);

        render_interface.setviewport(
            width,
            height
        );

        if(!render_interface.initgl())
            return false;

        Rml::SetRenderInterface(
            &render_interface
        );

        if(!Rml::Initialise())
        {
            render_interface.shutdowngl();
            return false;
        }

        context = Rml::CreateContext(
            "cowbraten",
            Rml::Vector2i(width, height)
        );

        if(!context)
        {
            Rml::Shutdown();
            render_interface.shutdowngl();
            return false;
        }

        initialized = true;
        return true;
    }

    void shutdown()
    {
        shown = false;

        if(initialized)
        {
            Rml::Shutdown();
            context = nullptr;
            initialized = false;
        }
        else
        {
            context = nullptr;
        }

        render_interface.shutdowngl();
    }

    void resize(int width, int height)
    {
        if(!initialized || !context)
            return;

        width = valid_dimension(width);
        height = valid_dimension(height);

        render_interface.setviewport(
            width,
            height
        );

        context->SetDimensions(
            Rml::Vector2i(width, height)
        );
    }

    void beforeglreset()
    {
        if(initialized)
            render_interface.beforeglreset();
    }

    void afterglreset(int width, int height)
    {
        width = valid_dimension(width);
        height = valid_dimension(height);

        render_interface.setviewport(
            width,
            height
        );

        if(initialized)
            render_interface.afterglreset();

        if(initialized && context)
        {
            context->SetDimensions(
                Rml::Vector2i(width, height)
            );
        }
    }

    bool processevent(const SDL_Event &)
    {
        return false;
    }

    void update()
    {
        if(initialized &&
           context &&
           shown)
        {
            context->Update();
        }
    }

    void render()
    {
        if(!initialized ||
           !context ||
           !shown ||
           !render_interface.ready())
        {
            return;
        }

        render_interface.beginframe();
        context->Render();
        render_interface.endframe();
    }

    void showtest()
    {
        if(initialized && context)
            shown = true;
    }

    void close()
    {
        shown = false;
    }

    bool visible()
    {
        return shown;
    }
}
