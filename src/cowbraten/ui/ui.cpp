#include "ui.h"
#include "rml_action.h"
#include "rml_render.h"
#include "rml_file.h"
#include "rml_server_browser.h"
#include "rml_settings.h"
#include "sauer_input_bridge.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/TextInputContext.h>
#include <RmlUi/Core/TextInputHandler.h>

#include <SDL.h>

#include <cstdio>

namespace cowui
{
    static RmlRenderInterface render_interface;
    static RmlFileInterface file_interface;
    static RmlActionRouter action_router;
    static RmlServerBrowser server_browser;
    static RmlSettings settings_model;
    static Rml::Context *context = nullptr;

    struct Screen
    {
        ScreenId id;
        const char *path;
        Rml::ElementDocument *document;
    };

    static Screen screens[] =
    {
        {
            ScreenId::Test,
            "data/cowbraten/ui/default/screens/test.rml",
            nullptr
        },
        {
            ScreenId::Main,
            "data/cowbraten/ui/default/screens/main.rml",
            nullptr
        },
        {
            ScreenId::Servers,
            "data/cowbraten/ui/default/screens/servers.rml",
            nullptr
        },
        {
            ScreenId::Settings,
            "data/cowbraten/ui/default/screens/settings.rml",
            nullptr
        }
    };

    static Screen *active_screen = nullptr;

    static bool initialized = false;
    static bool console_suspended = false;

    static Screen *findscreen(ScreenId id)
    {
        for(Screen &screen : screens)
            if(screen.id == id)
                return &screen;

        return nullptr;
    }

    static void hidedocument(Screen &screen)
    {
        if(!screen.document)
            return;

        // Release focused controls before another screen or gameplay receives
        // keyboard input.
        screen.document->Focus();
        screen.document->Hide();
    }

    static void syncconsoleownership()
    {
        const bool should_suspend = active_screen &&
            sauerinput::consoleactive();

        if(should_suspend == console_suspended)
            return;

        console_suspended = should_suspend;

        if(!active_screen || !active_screen->document)
            return;

        if(console_suspended)
        {
            hidedocument(*active_screen);
        }
        else
        {
            if(active_screen->id == ScreenId::Settings)
                settings_model.activate();

            active_screen->document->Show();
        }
    }

    static void toggletestselection(Rml::Event &event)
    {
        Rml::Element *element = event.GetCurrentElement();

        element->SetClass(
            "is-selected",
            !element->IsClassSet("is-selected")
        );

        std::fprintf(
            stderr,
            "Cowbraten UI: test selection toggled\n"
        );
    }

    static void logtestwheel(Rml::Event &event)
    {
        event.StopPropagation();

        std::fprintf(
            stderr,
            "Cowbraten UI: test control received mouse wheel\n"
        );
    }

    static void closeui(Rml::Event &event)
    {
        event.StopPropagation();
        close();
    }

    static void showmainmenu(Rml::Event &event)
    {
        event.StopPropagation();
        show(ScreenId::Main);
    }

    static void showserverbrowser(Rml::Event &event)
    {
        event.StopPropagation();
        show(ScreenId::Servers);
    }

    static void showsettingsscreen(Rml::Event &event)
    {
        event.StopPropagation();
        show(ScreenId::Settings);
    }

    class RmlTextInputHandler final : public Rml::TextInputHandler
    {
    public:
        void OnActivate(Rml::TextInputContext *input_context) override
        {
            active_context = input_context;
            sauerinput::textinput(true);
            sauerinput::keyrepeat(true);
        }

        void OnDeactivate(Rml::TextInputContext *input_context) override
        {
            if(active_context == input_context)
                deactivate();
        }

        void OnDestroy(Rml::TextInputContext *input_context) override
        {
            if(active_context == input_context)
                deactivate();
        }

        bool active() const
        {
            return active_context != nullptr;
        }

        void deactivate()
        {
            active_context = nullptr;
            sauerinput::keyrepeat(false);
            sauerinput::textinput(false);
        }

    private:
        Rml::TextInputContext *active_context = nullptr;
    };

    static RmlTextInputHandler text_input_handler;

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

    static int mousebutton(int button)
    {
        switch(button)
        {
            case SDL_BUTTON_LEFT: return 0;
            case SDL_BUTTON_RIGHT: return 1;
            case SDL_BUTTON_MIDDLE: return 2;
            default: return 3;
        }
    }

    static bool finishinput(bool consumed)
    {
        if(active_screen && active_screen->id == ScreenId::Settings)
            settings_model.applypending();
        return consumed;
    }

    static Rml::Input::KeyIdentifier keyidentifier(SDL_Keycode key)
    {
        switch(key)
        {
            case SDLK_UNKNOWN: return Rml::Input::KI_UNKNOWN;
            case SDLK_ESCAPE: return Rml::Input::KI_ESCAPE;
            case SDLK_SPACE: return Rml::Input::KI_SPACE;
            case SDLK_0: return Rml::Input::KI_0;
            case SDLK_1: return Rml::Input::KI_1;
            case SDLK_2: return Rml::Input::KI_2;
            case SDLK_3: return Rml::Input::KI_3;
            case SDLK_4: return Rml::Input::KI_4;
            case SDLK_5: return Rml::Input::KI_5;
            case SDLK_6: return Rml::Input::KI_6;
            case SDLK_7: return Rml::Input::KI_7;
            case SDLK_8: return Rml::Input::KI_8;
            case SDLK_9: return Rml::Input::KI_9;
            case SDLK_a: return Rml::Input::KI_A;
            case SDLK_b: return Rml::Input::KI_B;
            case SDLK_c: return Rml::Input::KI_C;
            case SDLK_d: return Rml::Input::KI_D;
            case SDLK_e: return Rml::Input::KI_E;
            case SDLK_f: return Rml::Input::KI_F;
            case SDLK_g: return Rml::Input::KI_G;
            case SDLK_h: return Rml::Input::KI_H;
            case SDLK_i: return Rml::Input::KI_I;
            case SDLK_j: return Rml::Input::KI_J;
            case SDLK_k: return Rml::Input::KI_K;
            case SDLK_l: return Rml::Input::KI_L;
            case SDLK_m: return Rml::Input::KI_M;
            case SDLK_n: return Rml::Input::KI_N;
            case SDLK_o: return Rml::Input::KI_O;
            case SDLK_p: return Rml::Input::KI_P;
            case SDLK_q: return Rml::Input::KI_Q;
            case SDLK_r: return Rml::Input::KI_R;
            case SDLK_s: return Rml::Input::KI_S;
            case SDLK_t: return Rml::Input::KI_T;
            case SDLK_u: return Rml::Input::KI_U;
            case SDLK_v: return Rml::Input::KI_V;
            case SDLK_w: return Rml::Input::KI_W;
            case SDLK_x: return Rml::Input::KI_X;
            case SDLK_y: return Rml::Input::KI_Y;
            case SDLK_z: return Rml::Input::KI_Z;
            case SDLK_SEMICOLON: return Rml::Input::KI_OEM_1;
            case SDLK_PLUS: return Rml::Input::KI_OEM_PLUS;
            case SDLK_COMMA: return Rml::Input::KI_OEM_COMMA;
            case SDLK_MINUS: return Rml::Input::KI_OEM_MINUS;
            case SDLK_PERIOD: return Rml::Input::KI_OEM_PERIOD;
            case SDLK_SLASH: return Rml::Input::KI_OEM_2;
            case SDLK_BACKQUOTE: return Rml::Input::KI_OEM_3;
            case SDLK_LEFTBRACKET: return Rml::Input::KI_OEM_4;
            case SDLK_BACKSLASH: return Rml::Input::KI_OEM_5;
            case SDLK_RIGHTBRACKET: return Rml::Input::KI_OEM_6;
            case SDLK_QUOTEDBL: return Rml::Input::KI_OEM_7;
            case SDLK_KP_0: return Rml::Input::KI_NUMPAD0;
            case SDLK_KP_1: return Rml::Input::KI_NUMPAD1;
            case SDLK_KP_2: return Rml::Input::KI_NUMPAD2;
            case SDLK_KP_3: return Rml::Input::KI_NUMPAD3;
            case SDLK_KP_4: return Rml::Input::KI_NUMPAD4;
            case SDLK_KP_5: return Rml::Input::KI_NUMPAD5;
            case SDLK_KP_6: return Rml::Input::KI_NUMPAD6;
            case SDLK_KP_7: return Rml::Input::KI_NUMPAD7;
            case SDLK_KP_8: return Rml::Input::KI_NUMPAD8;
            case SDLK_KP_9: return Rml::Input::KI_NUMPAD9;
            case SDLK_KP_ENTER: return Rml::Input::KI_NUMPADENTER;
            case SDLK_KP_MULTIPLY: return Rml::Input::KI_MULTIPLY;
            case SDLK_KP_PLUS: return Rml::Input::KI_ADD;
            case SDLK_KP_MINUS: return Rml::Input::KI_SUBTRACT;
            case SDLK_KP_PERIOD: return Rml::Input::KI_DECIMAL;
            case SDLK_KP_DIVIDE: return Rml::Input::KI_DIVIDE;
            case SDLK_KP_EQUALS: return Rml::Input::KI_OEM_NEC_EQUAL;
            case SDLK_BACKSPACE: return Rml::Input::KI_BACK;
            case SDLK_TAB: return Rml::Input::KI_TAB;
            case SDLK_CLEAR: return Rml::Input::KI_CLEAR;
            case SDLK_RETURN: return Rml::Input::KI_RETURN;
            case SDLK_PAUSE: return Rml::Input::KI_PAUSE;
            case SDLK_CAPSLOCK: return Rml::Input::KI_CAPITAL;
            case SDLK_PAGEUP: return Rml::Input::KI_PRIOR;
            case SDLK_PAGEDOWN: return Rml::Input::KI_NEXT;
            case SDLK_END: return Rml::Input::KI_END;
            case SDLK_HOME: return Rml::Input::KI_HOME;
            case SDLK_LEFT: return Rml::Input::KI_LEFT;
            case SDLK_UP: return Rml::Input::KI_UP;
            case SDLK_RIGHT: return Rml::Input::KI_RIGHT;
            case SDLK_DOWN: return Rml::Input::KI_DOWN;
            case SDLK_INSERT: return Rml::Input::KI_INSERT;
            case SDLK_DELETE: return Rml::Input::KI_DELETE;
            case SDLK_HELP: return Rml::Input::KI_HELP;
            case SDLK_F1: return Rml::Input::KI_F1;
            case SDLK_F2: return Rml::Input::KI_F2;
            case SDLK_F3: return Rml::Input::KI_F3;
            case SDLK_F4: return Rml::Input::KI_F4;
            case SDLK_F5: return Rml::Input::KI_F5;
            case SDLK_F6: return Rml::Input::KI_F6;
            case SDLK_F7: return Rml::Input::KI_F7;
            case SDLK_F8: return Rml::Input::KI_F8;
            case SDLK_F9: return Rml::Input::KI_F9;
            case SDLK_F10: return Rml::Input::KI_F10;
            case SDLK_F11: return Rml::Input::KI_F11;
            case SDLK_F12: return Rml::Input::KI_F12;
            case SDLK_F13: return Rml::Input::KI_F13;
            case SDLK_F14: return Rml::Input::KI_F14;
            case SDLK_F15: return Rml::Input::KI_F15;
            case SDLK_NUMLOCKCLEAR: return Rml::Input::KI_NUMLOCK;
            case SDLK_SCROLLLOCK: return Rml::Input::KI_SCROLL;
            case SDLK_LSHIFT: return Rml::Input::KI_LSHIFT;
            case SDLK_RSHIFT: return Rml::Input::KI_RSHIFT;
            case SDLK_LCTRL: return Rml::Input::KI_LCONTROL;
            case SDLK_RCTRL: return Rml::Input::KI_RCONTROL;
            case SDLK_LALT: return Rml::Input::KI_LMENU;
            case SDLK_RALT: return Rml::Input::KI_RMENU;
            case SDLK_LGUI: return Rml::Input::KI_LMETA;
            case SDLK_RGUI: return Rml::Input::KI_RMETA;
            default: return Rml::Input::KI_UNKNOWN;
        }
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

        Rml::SetTextInputHandler(
            &text_input_handler
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

        action_router.clear();

        if(!action_router.registeraction(
               "test.toggle-selection",
               toggletestselection) ||
           !action_router.registeraction(
               "test.log-wheel",
               logtestwheel) ||
           !action_router.registeraction(
               "navigation.close",
               closeui) ||
           !action_router.registeraction(
               "navigation.main",
               showmainmenu) ||
           !action_router.registeraction(
               "navigation.servers",
               showserverbrowser) ||
           !action_router.registeraction(
               "navigation.settings",
               showsettingsscreen) ||
           !server_browser.init(context, action_router) ||
           !settings_model.init(context, action_router))
        {
            settings_model.shutdown(context);
            server_browser.shutdown(context);
            action_router.clear();
            Rml::Shutdown();
            context = nullptr;
            render_interface.shutdowngl();
            return false;
        }

        action_router.install();

        initialized = true;
        return true;
    }

    void shutdown()
    {
        if(active_screen)
            close();

        for(Screen &screen : screens)
            screen.document = nullptr;

        if(initialized)
        {
            settings_model.shutdown(context);
            server_browser.shutdown(context);
            Rml::Shutdown();
            context = nullptr;
            initialized = false;
        }
        else
        {
            context = nullptr;
        }

        text_input_handler.deactivate();
        action_router.clear();

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

        if(active_screen && active_screen->id == ScreenId::Settings)
            settings_model.videochanged();
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

            if(active_screen && active_screen->id == ScreenId::Settings)
                settings_model.videochanged();
        }
    }

    bool processevent(const SDL_Event &event)
    {
        if(!initialized || !context || !active_screen)
            return false;

        syncconsoleownership();

        if(console_suspended)
            return false;

        if(event.type == SDL_KEYDOWN &&
           !event.key.repeat &&
           sauerinput::openconsolefromkey(event.key.keysym.sym))
        {
            syncconsoleownership();
            return true;
        }

        switch(event.type)
        {
            case SDL_MOUSEMOTION:
                return !context->ProcessMouseMove(
                    event.motion.x,
                    event.motion.y,
                    keymodifierstate()
                );

            case SDL_MOUSEBUTTONDOWN:
                return !context->ProcessMouseButtonDown(
                    mousebutton(event.button.button),
                    keymodifierstate()
                );

            case SDL_MOUSEBUTTONUP:
                return finishinput(
                    !context->ProcessMouseButtonUp(
                        mousebutton(event.button.button),
                        keymodifierstate()
                    )
                );

            case SDL_MOUSEWHEEL:
                return !context->ProcessMouseWheel(
                    Rml::Vector2f(
                        float(event.wheel.x),
                        float(-event.wheel.y)
                    ),
                    keymodifierstate()
                );

            case SDL_KEYDOWN:
            {
                if(event.key.keysym.sym == SDLK_ESCAPE)
                    return true;

                const bool capture = text_input_handler.active();
                bool propagate = context->ProcessKeyDown(
                    keyidentifier(event.key.keysym.sym),
                    keymodifierstate()
                );

                if(event.key.keysym.sym == SDLK_RETURN ||
                   event.key.keysym.sym == SDLK_KP_ENTER)
                {
                    propagate = context->ProcessTextInput('\n') &&
                        propagate;
                }

                return finishinput(capture || !propagate);
            }

            case SDL_KEYUP:
            {
                if(event.key.keysym.sym == SDLK_ESCAPE)
                {
                    close();
                    return true;
                }

                const bool capture = text_input_handler.active();
                const bool propagate = context->ProcessKeyUp(
                    keyidentifier(event.key.keysym.sym),
                    keymodifierstate()
                );

                return finishinput(capture || !propagate);
            }

            case SDL_TEXTINPUT:
            {
                const bool capture = text_input_handler.active();
                const bool propagate = context->ProcessTextInput(
                    Rml::String(event.text.text)
                );

                return capture || !propagate;
            }
        }

        return false;
    }

    void update()
    {
        if(initialized &&
           context &&
           active_screen)
        {
            syncconsoleownership();

            if(console_suspended)
                return;

            if(active_screen->id == ScreenId::Servers)
                server_browser.update();

            context->Update();
        }
    }

    void render()
    {
        if(!initialized ||
           !context ||
           !active_screen ||
           console_suspended ||
           !render_interface.ready())
        {
            return;
        }

        render_interface.beginframe();
        context->Render();
        render_interface.endframe();
    }

    bool show(ScreenId id)
    {
        if(!initialized || !context)
            return false;

        Screen *screen = findscreen(id);
        if(!screen)
            return false;

        if(active_screen == screen)
        {
            if(id == ScreenId::Settings)
                settings_model.activate();

            return true;
        }

        if(id == ScreenId::Servers)
            server_browser.activate();
        else if(id == ScreenId::Settings)
            settings_model.activate();

        if(!screen->document)
        {
            screen->document = context->LoadDocument(
                screen->path
            );

            if(!screen->document)
            {
                std::fprintf(
                    stderr,
                    "Cowbraten UI: failed to load screen '%s'\n",
                    screen->path
                );
                return false;
            }
        }

        const bool opening = active_screen == nullptr;
        if(active_screen)
            hidedocument(*active_screen);

        screen->document->Show();
        active_screen = screen;
        console_suspended = false;
        syncconsoleownership();

        if(opening)
            sauerinput::beginmenu();

        return true;
    }

    void showtest()
    {
        show(ScreenId::Test);
    }

    void showmain()
    {
        show(ScreenId::Main);
    }

    void showservers()
    {
        show(ScreenId::Servers);
    }

    void showsettings()
    {
        show(ScreenId::Settings);
    }

    void close()
    {
        if(!active_screen)
            return;

        hidedocument(*active_screen);
        active_screen = nullptr;
        console_suspended = false;

        sauerinput::endmenu();
    }

    bool visible()
    {
        return active_screen != nullptr;
    }
}
