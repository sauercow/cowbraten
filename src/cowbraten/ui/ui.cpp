#include "ui.h"
#include "rml_render.h"
#include "rml_file.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <SDL.h>

namespace cowui
{
    static RmlRenderInterface render_interface;
    static RmlFileInterface file_interface;
    static Rml::Context *context = nullptr;
    static Rml::ElementDocument *test_document = nullptr;

    static bool initialized = false;
    static bool shown = false;

    static int valid_dimension(int value)
    {
        return value > 0 ? value : 1;
    }

    static int keymodifierstate()
    {
        const SDL_Keymod modifiers = SDL_GetModState();
        int result = 0;

        if(modifiers & KMOD_CTRL)
            result |= Rml::Input::KM_CTRL;
        if(modifiers & KMOD_SHIFT)
            result |= Rml::Input::KM_SHIFT;
        if(modifiers & KMOD_ALT)
            result |= Rml::Input::KM_ALT;
        if(modifiers & KMOD_NUM)
            result |= Rml::Input::KM_NUMLOCK;
        if(modifiers & KMOD_CAPS)
            result |= Rml::Input::KM_CAPSLOCK;

        return result;
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

        Rml::SetFileInterface(
            &file_interface
        );

        if(!Rml::Initialise())
        {
            render_interface.shutdowngl();
            return false;
        }

        if(!Rml::LoadFontFace(
            "data/cowbraten/ui/default/fonts/LatoLatin-Regular.ttf",
            "Lato",
            Rml::Style::FontStyle::Normal,
            Rml::Style::FontWeight::Normal))
        {
            Rml::Shutdown();
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
        test_document = nullptr;

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

    bool processevent(const SDL_Event &event)
    {
        if(!initialized || !context || !shown)
            return false;

        if(event.type == SDL_MOUSEMOTION)
        {
            return !context->ProcessMouseMove(
                event.motion.x,
                event.motion.y,
                keymodifierstate()
            );
        }

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
        if(!initialized || !context)
            return;

        if(!test_document)
        {
            static const char *document_rml = R"RML(
<rml>
<head>
    <style>
        body
        {
            margin: 0;
            width: 100%;
            height: 100%;
            background-color: transparent;
        }

        #panel
        {
            position: absolute;
            left: 64px;
            top: 64px;
            width: 360px;
            height: 180px;

            background-color: rgba(28, 32, 40, 220);

            border-width: 3px;
            border-color: rgb(70, 150, 220);
        }

        #accent
        {
            position: absolute;
            left: 24px;
            top: 24px;
            width: 8px;
            height: 132px;

            background-color: rgb(70, 150, 220);
        }

        #test-button
        {
            position: absolute;
            left: 56px;
            top: 40px;
            width: 240px;
            height: 48px;

            padding: 0;
            background-color: rgba(45, 52, 64, 240);

            border-width: 2px;
            border-color: rgb(95, 105, 120);

            font-family: Lato;
            font-size: 20px;
            color: #ffffff;
        }

        #test-button:hover
        {
            background-color: rgb(70, 150, 220);
            border-color: rgb(150, 210, 255);
        }

        #title
        {
            position: absolute;
            left: 56px;
            top: 124px;

            font-family: Lato;
            font-size: 24px;
            color: #ffffff;
        }
    </style>
</head>

<body>
    <div id="panel">
        <div id="accent"></div>
        <button id="test-button">TEST BUTTON</button>
        <div id="title">COWBRATEN UI</div>
    </div>
</body>
</rml>
)RML";

            test_document = context->LoadDocumentFromMemory(
                document_rml,
                "cowbraten://ui_test"
            );

            if(!test_document)
                return;
        }

        test_document->Show();
        shown = true;
    }

    void close()
    {
        if(test_document)
            test_document->Hide();

        shown = false;
    }

    bool visible()
    {
        return shown;
    }
}
