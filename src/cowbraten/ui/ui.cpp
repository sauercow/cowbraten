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
        if(initialized) return true;

        width = valid_dimension(width);
        height = valid_dimension(height);

        render_interface.setviewport(width, height);

        Rml::SetRenderInterface(&render_interface);

        if(!Rml::Initialise())
            return false;

        context = Rml::CreateContext(
            "cowbraten",
            Rml::Vector2i(width, height)
        );

        if(!context)
        {
            Rml::Shutdown();
            return false;
        }

        initialized = true;
        return true;
    }

    void shutdown()
    {
        shown = false;

        if(!initialized)
        {
            context = nullptr;
            return;
        }

        context = nullptr;
        Rml::Shutdown();

        initialized = false;
    }

    void resize(int width, int height)
    {
        if(!initialized || !context) return;

        width = valid_dimension(width);
        height = valid_dimension(height);

        render_interface.setviewport(width, height);
        context->SetDimensions(Rml::Vector2i(width, height));
    }

    void beforeglreset()
    {
        // GPU resource invalidation will be implemented with the renderer.
    }

    void afterglreset(int width, int height)
    {
        resize(width, height);
    }

    bool processevent(const SDL_Event &)
    {
        return false;
    }

    void update()
    {
        if(initialized && context && shown)
            context->Update();
    }

    void render()
    {
        if(initialized && context && shown)
            context->Render();
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
