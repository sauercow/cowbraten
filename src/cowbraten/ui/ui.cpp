#include "ui.h"
#include "rml_action.h"
#include "rml_chat.h"
#include "rml_console.h"
#include "rml_render.h"
#include "rml_file.h"
#include "rml_server_browser.h"
#include "rml_demo_manager.h"
#include "rml_demo_player.h"
#include "rml_game_menu.h"
#include "rml_profile.h"
#include "rml_settings.h"
#include "sauer_demo_bridge.h"
#include "sauer_input_bridge.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <RmlUi/Core/TextInputContext.h>
#include <RmlUi/Core/TextInputHandler.h>

#include <SDL.h>

#include <cstdio>
#include <cstring>
#include <array>
#include <string>

extern char *uitheme;

namespace cowui
{
    static RmlRenderInterface render_interface;
    static RmlFileInterface file_interface;
    static RmlActionRouter action_router;
    static RmlChat chat_model;
    static RmlConsole console_model;
    static RmlServerBrowser server_browser;
    static RmlDemoManager demo_manager;
    static RmlDemoPlayer demo_player;
    static RmlGameMenu game_menu_model;
    static RmlProfile profile_model;
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
            ScreenId::GameMenu,
            "data/cowbraten/ui/default/screens/game_menu.rml",
            nullptr
        },
        {
            ScreenId::Servers,
            "data/cowbraten/ui/default/screens/servers.rml",
            nullptr
        },
        {
            ScreenId::Demos,
            "data/cowbraten/ui/default/screens/demos.rml",
            nullptr
        },
        {
            ScreenId::Settings,
            "data/cowbraten/ui/default/screens/settings.rml",
            nullptr
        },
        {
            ScreenId::Profile,
            "data/cowbraten/ui/default/screens/profile.rml",
            nullptr
        }
    };

    static Screen *active_screen = nullptr;
    static bool full_menu_from_game = false;

    static bool initialized = false;
    static bool legacy_console_input = false;
    static bool legacy_console_presentation = false;
    static bool demo_player_input_active = false;
    static std::string requested_theme = "default";
    static Rml::String active_theme;
    static Rml::SharedPtr<Rml::StyleSheetContainer> theme_style_sheet;

    static const char *normalizetheme(const char *name)
    {
        if(name && !std::strcmp(name, "modern-metallic"))
            return "modern-metallic";
        return "default";
    }

    static void activaterequestedtheme()
    {
        if(!context)
            return;

        const char *next = normalizetheme(requested_theme.c_str());
        if(active_theme == next)
            return;

        if(!active_theme.empty() && active_theme != "default")
            context->ActivateTheme(active_theme, false);

        active_theme = next;
        if(active_theme != "default")
            context->ActivateTheme(active_theme, true);
    }

    bool applytheme(Rml::ElementDocument *document)
    {
        if(!document || !theme_style_sheet)
            return false;

        const Rml::StyleSheetContainer *base =
            document->GetStyleSheetContainer();
        if(!base)
            return false;

        document->SetStyleSheetContainer(
            base->CombineStyleSheetContainer(*theme_style_sheet));
        return true;
    }

    struct UiDimensions
    {
        int window_width = 1;
        int window_height = 1;
        int drawable_width = 1;
        int drawable_height = 1;
        int context_width = 1;
        int context_height = 1;
        float density_ratio = 1.0f;
        bool input_mapping_valid = false;
    };

    static UiDimensions ui_dimensions;

    // Presses consumed by capture retain ownership through repeat/release,
    // including when the document closes or the console takes over.
    static std::array<bool, SDL_NUM_SCANCODES> captured_keys{};
    static std::array<bool, 256> captured_buttons{};
    static int captured_key_count = 0;

    static Screen *findscreen(ScreenId id)
    {
        for(Screen &screen : screens)
            if(screen.id == id)
                return &screen;

        return nullptr;
    }

    static void hidedocument(Screen &screen)
    {
        if(screen.id == ScreenId::Settings)
            settings_model.cancelkeycapture();

        if(!screen.document)
            return;

        // Release focused controls before another screen or gameplay receives
        // keyboard input.
        screen.document->Focus();
        screen.document->Hide();
    }

    static void syncconsoleownership()
    {
        const bool should_suspend_presentation = active_screen &&
            sauerinput::fullconsoleactive();
        const bool should_suspend_input = active_screen &&
            (sauerinput::commandlineactive() ||
             should_suspend_presentation);

        if(should_suspend_input == legacy_console_input &&
           should_suspend_presentation == legacy_console_presentation)
            return;

        const bool input_started = should_suspend_input &&
            !legacy_console_input;
        const bool presentation_changed =
            should_suspend_presentation != legacy_console_presentation;

        legacy_console_input = should_suspend_input;
        legacy_console_presentation = should_suspend_presentation;

        if(!active_screen || !active_screen->document)
            return;

        if(input_started && !should_suspend_presentation)
        {
            if(active_screen->id == ScreenId::Settings)
                settings_model.cancelkeycapture();

            // Release any focused RmlUi input while leaving the document
            // visible behind the legacy chat/command line.
            active_screen->document->Focus();
        }

        if(!presentation_changed)
            return;

        if(should_suspend_presentation)
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

    static void backui(Rml::Event &event)
    {
        event.StopPropagation();
        if(full_menu_from_game && sauerinput::canclosemenu())
            show(ScreenId::GameMenu);
        else
            close();
    }

    static void showmainmenu(Rml::Event &event)
    {
        event.StopPropagation();
        showmain();
    }

    static void preservegamemenubackstack()
    {
        if(active_screen && active_screen->id == ScreenId::GameMenu &&
           sauerinput::canclosemenu())
            full_menu_from_game = true;
    }

    static void showserverbrowser(Rml::Event &event)
    {
        event.StopPropagation();
        preservegamemenubackstack();
        show(ScreenId::Servers);
    }

    static void showdemoscreen(Rml::Event &event)
    {
        event.StopPropagation();
        preservegamemenubackstack();
        show(ScreenId::Demos);
    }

    static void showsettingsscreen(Rml::Event &event)
    {
        event.StopPropagation();
        preservegamemenubackstack();
        show(ScreenId::Settings);
    }

    static void showprofilescreen(Rml::Event &event)
    {
        event.StopPropagation();
        preservegamemenubackstack();
        show(ScreenId::Profile);
    }

    static void quitapplication(Rml::Event &event)
    {
        event.StopPropagation();
        sauerinput::quit();
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

    static bool setdimensions(
        int window_width,
        int window_height,
        int drawable_width,
        int drawable_height,
        bool &context_changed)
    {
        const bool valid_window =
            window_width > 0 && window_height > 0;
        const bool valid_drawable =
            drawable_width > 0 && drawable_height > 0;

        window_width = valid_dimension(window_width);
        window_height = valid_dimension(window_height);
        drawable_width = valid_drawable
            ? drawable_width
            : window_width;
        drawable_height = valid_drawable
            ? drawable_height
            : window_height;

        const float horizontal_density =
            float(drawable_width) / float(window_width);
        const float vertical_density =
            float(drawable_height) / float(window_height);
        const float density_ratio = horizontal_density < vertical_density
            ? horizontal_density
            : vertical_density;

        context_changed =
            ui_dimensions.context_width != drawable_width ||
            ui_dimensions.context_height != drawable_height ||
            ui_dimensions.density_ratio != density_ratio;

        const bool dimensions_changed =
            ui_dimensions.window_width != window_width ||
            ui_dimensions.window_height != window_height ||
            ui_dimensions.drawable_width != drawable_width ||
            ui_dimensions.drawable_height != drawable_height ||
            context_changed ||
            ui_dimensions.input_mapping_valid !=
                (valid_window && valid_drawable);

        ui_dimensions.window_width = window_width;
        ui_dimensions.window_height = window_height;
        ui_dimensions.drawable_width = drawable_width;
        ui_dimensions.drawable_height = drawable_height;
        // Context space tracks drawable pixels. Density-independent RCSS
        // lengths preserve logical UI size while retaining sharp HiDPI text.
        ui_dimensions.context_width = drawable_width;
        ui_dimensions.context_height = drawable_height;
        ui_dimensions.density_ratio = density_ratio;
        ui_dimensions.input_mapping_valid =
            valid_window && valid_drawable;

        return dimensions_changed;
    }

    static int contextcoordinate(
        int coordinate,
        int window_extent,
        int context_extent)
    {
        if(!ui_dimensions.input_mapping_valid ||
           window_extent <= 0 || context_extent <= 0 ||
           window_extent == context_extent)
        {
            return coordinate;
        }

        const long long scaled =
            static_cast<long long>(coordinate) * context_extent;
        const long long rounding = window_extent / 2;
        return int(
            scaled >= 0
                ? (scaled + rounding) / window_extent
                : (scaled - rounding) / window_extent
        );
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

    bool init(
        int window_width,
        int window_height,
        int drawable_width,
        int drawable_height)
    {
        if(initialized)
            return true;

        bool context_changed = false;
        setdimensions(
            window_width,
            window_height,
            drawable_width,
            drawable_height,
            context_changed
        );

        render_interface.setviewport(
            ui_dimensions.drawable_width,
            ui_dimensions.drawable_height
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
            Rml::Vector2i(
                ui_dimensions.context_width,
                ui_dimensions.context_height
            )
        );

        if(!context)
        {
            Rml::Shutdown();
            render_interface.shutdowngl();
            return false;
        }

        context->SetDensityIndependentPixelRatio(
            ui_dimensions.density_ratio
        );

        theme_style_sheet = Rml::Factory::InstanceStyleSheetFile(
            "data/cowbraten/ui/themes/modern-metallic/theme.rcss");
        if(!theme_style_sheet)
        {
            theme_style_sheet.reset();
            Rml::Shutdown();
            context = nullptr;
            render_interface.shutdowngl();
            return false;
        }

        requested_theme = uitheme && uitheme[0] ? uitheme : "default";
        activaterequestedtheme();

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
               "navigation.back",
               backui) ||
           !action_router.registeraction(
               "navigation.main",
               showmainmenu) ||
           !action_router.registeraction(
               "navigation.servers",
               showserverbrowser) ||
           !action_router.registeraction(
               "navigation.demos",
               showdemoscreen) ||
           !action_router.registeraction(
               "navigation.settings",
               showsettingsscreen) ||
           !action_router.registeraction(
               "navigation.profile",
               showprofilescreen) ||
           !action_router.registeraction(
               "navigation.exit",
               quitapplication) ||
           !chat_model.init(context) ||
           !console_model.init(context) ||
           !server_browser.init(context, action_router) ||
           !demo_manager.init(context, action_router) ||
           !demo_player.init(context, action_router) ||
           !game_menu_model.init(context, action_router) ||
           !profile_model.init(context, action_router) ||
           !settings_model.init(context, action_router))
        {
            settings_model.shutdown(context);
            profile_model.shutdown(context);
            game_menu_model.shutdown(context);
            demo_player.shutdown(context);
            demo_manager.shutdown(context);
            server_browser.shutdown(context);
            console_model.shutdown(context);
            chat_model.shutdown(context);
            action_router.clear();
            theme_style_sheet.reset();
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
        settings_model.cancelkeycapture();
        captured_keys.fill(false);
        captured_buttons.fill(false);
        captured_key_count = 0;
        demo_player_input_active = false;
        if(active_screen)
            close();

        for(Screen &screen : screens)
            screen.document = nullptr;

        if(initialized)
        {
            settings_model.shutdown(context);
            profile_model.shutdown(context);
            game_menu_model.shutdown(context);
            demo_player.shutdown(context);
            demo_manager.shutdown(context);
            server_browser.shutdown(context);
            console_model.shutdown(context);
            chat_model.shutdown(context);
            theme_style_sheet.reset();
            Rml::Shutdown();
            context = nullptr;
            initialized = false;
            active_theme.clear();
        }
        else
        {
            context = nullptr;
        }

        text_input_handler.deactivate();
        action_router.clear();

        render_interface.shutdowngl();
        ui_dimensions = UiDimensions();
    }

    void resize(
        int window_width,
        int window_height,
        int drawable_width,
        int drawable_height)
    {
        if(!initialized || !context)
            return;

        bool context_changed = false;
        const bool dimensions_changed = setdimensions(
            window_width,
            window_height,
            drawable_width,
            drawable_height,
            context_changed
        );

        render_interface.setviewport(
            ui_dimensions.drawable_width,
            ui_dimensions.drawable_height
        );

        if(context_changed)
        {
            context->SetDimensions(
                Rml::Vector2i(
                    ui_dimensions.context_width,
                    ui_dimensions.context_height
                )
            );
            context->SetDensityIndependentPixelRatio(
                ui_dimensions.density_ratio
            );
        }

        if(dimensions_changed &&
           active_screen && active_screen->id == ScreenId::Settings)
        {
            settings_model.videochanged();
        }
    }

    void beforeglreset()
    {
        if(initialized)
            render_interface.beforeglreset();
    }

    void afterglreset(
        int window_width,
        int window_height,
        int drawable_width,
        int drawable_height)
    {
        bool context_changed = false;
        setdimensions(
            window_width,
            window_height,
            drawable_width,
            drawable_height,
            context_changed
        );

        render_interface.setviewport(
            ui_dimensions.drawable_width,
            ui_dimensions.drawable_height
        );

        if(initialized)
            render_interface.afterglreset();

        if(initialized && context)
        {
            context->SetDimensions(
                Rml::Vector2i(
                    ui_dimensions.context_width,
                    ui_dimensions.context_height
                )
            );
            context->SetDensityIndependentPixelRatio(
                ui_dimensions.density_ratio
            );

            if(active_screen && active_screen->id == ScreenId::Settings)
                settings_model.videochanged();
        }
    }

    bool processevent(const SDL_Event &event)
    {
        if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
        {
            const int scan = event.key.keysym.scancode;
            if(scan >= 0 && scan < SDL_NUM_SCANCODES && captured_keys[scan])
            {
                if(event.type == SDL_KEYUP)
                {
                    captured_keys[scan] = false;
                    --captured_key_count;
                }
                return true;
            }
        }
        else if(event.type == SDL_MOUSEBUTTONUP &&
                captured_buttons[event.button.button])
        {
            captured_buttons[event.button.button] = false;
            return true;
        }
        else if(event.type == SDL_TEXTINPUT && captured_key_count)
            return true;

        if(!initialized || !context)
            return false;

        if(!active_screen && demo_player.receivesinput())
        {
            switch(event.type)
            {
                case SDL_MOUSEMOTION:
                {
                    sauerdemo::move_playback_cursor(event.motion.xrel, event.motion.yrel);
                    float x = 0, y = 0;
                    sauerdemo::playback_cursor(x, y);
                    context->ProcessMouseMove(
                        int(x * ui_dimensions.context_width),
                        int(y * ui_dimensions.context_height),
                        keymodifierstate()
                    );
                    return true;
                }
                case SDL_MOUSEBUTTONDOWN:
                    context->ProcessMouseButtonDown(
                        mousebutton(event.button.button),
                        keymodifierstate()
                    );
                    return true;
                case SDL_MOUSEBUTTONUP:
                    context->ProcessMouseButtonUp(
                        mousebutton(event.button.button),
                        keymodifierstate()
                    );
                    return true;
                case SDL_MOUSEWHEEL:
                    context->ProcessMouseWheel(
                        Rml::Vector2f(float(event.wheel.x), float(-event.wheel.y)),
                        keymodifierstate()
                    );
                    return true;
                default:
                    return false;
            }
        }

        if(!active_screen && console_model.visible() &&
           event.type == SDL_MOUSEWHEEL && event.wheel.y)
        {
            console_model.scroll(event.wheel.y * 3);
            return true;
        }

        if(!active_screen)
            return false;

        syncconsoleownership();

        if(legacy_console_input)
            return false;

        if(settings_model.capturingkey())
        {
            switch(event.type)
            {
                case SDL_KEYDOWN:
                {
                    if(event.key.repeat)
                        return true;
                    const int scan = event.key.keysym.scancode;
                    if(scan >= 0 && scan < SDL_NUM_SCANCODES)
                    {
                        captured_keys[scan] = true;
                        ++captured_key_count;
                    }
                    if(event.key.keysym.sym == SDLK_ESCAPE)
                        settings_model.cancelkeycapture();
                    else
                        settings_model.acceptkeycandidate(
                            sauerinput::capturekeyname(event));
                    return true;
                }
                case SDL_MOUSEBUTTONDOWN:
                    captured_buttons[event.button.button] = true;
                    settings_model.acceptkeycandidate(
                        sauerinput::capturekeyname(event));
                    return true;
                case SDL_MOUSEWHEEL:
                    settings_model.acceptkeycandidate(
                        sauerinput::capturekeyname(event));
                    return true;
                case SDL_TEXTINPUT:
                    return true;
            }
        }

        if(!text_input_handler.active() &&
           event.type == SDL_KEYDOWN &&
           !event.key.repeat &&
           sauerinput::openconsolefromkey(event.key.keysym.sym))
        {
            syncconsoleownership();
            return true;
        }

        switch(event.type)
        {
            case SDL_MOUSEMOTION:
            {
                const int context_x = contextcoordinate(
                    event.motion.x,
                    ui_dimensions.window_width,
                    ui_dimensions.context_width
                );
                const int context_y = contextcoordinate(
                    event.motion.y,
                    ui_dimensions.window_height,
                    ui_dimensions.context_height
                );
                return !context->ProcessMouseMove(
                    context_x,
                    context_y,
                    keymodifierstate()
                );
            }

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
                {
                    if(!event.key.repeat)
                    {
                        const bool can_close = sauerinput::canclosemenu();
                        if(active_screen->id == ScreenId::GameMenu)
                            close();
                        else if(full_menu_from_game && can_close)
                            show(ScreenId::GameMenu);
                        else if(active_screen->id != ScreenId::Profile &&
                           !can_close)
                            show(ScreenId::Profile);
                        else if(can_close)
                            close();
                    }
                    return true;
                }

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
        if(!initialized || !context)
            return;

        if(active_screen)
            syncconsoleownership();

        demo_player.update(active_screen != nullptr);
        chat_model.update(
            active_screen != nullptr,
            sauerinput::fullconsoleactive());
        console_model.update();

        const bool player_input = demo_player.receivesinput();
        if(demo_player_input_active && !player_input)
            context->ProcessMouseLeave();
        demo_player_input_active = player_input;

        if(active_screen && !legacy_console_presentation)
        {
            if(active_screen->id == ScreenId::Servers)
                server_browser.update();
            else if(active_screen->id == ScreenId::Demos)
                demo_manager.update();
            else if(active_screen->id == ScreenId::Profile)
                profile_model.update();
            else if(active_screen->id == ScreenId::GameMenu)
                game_menu_model.update();
        }

        if(!demo_player.visible() &&
           !chat_model.visible() &&
           !console_model.visible() &&
           (!active_screen || legacy_console_presentation))
            return;

        if(player_input)
        {
            float x = 0, y = 0;
            sauerdemo::playback_cursor(x, y);
            context->ProcessMouseMove(
                int(x * ui_dimensions.context_width),
                int(y * ui_dimensions.context_height),
                keymodifierstate()
            );
        }

        // Cached menus (especially Settings) contain thousands of elements.
        // Updating a chat message must not walk every menu opened this session.
        // Show() refreshes a hidden document before it receives focus again.
        context->Update(false);
    }

    void render()
    {
        if(!initialized ||
           !context ||
           (!active_screen &&
            !demo_player.visible() &&
            !chat_model.visible() &&
            !console_model.visible()) ||
           (active_screen && legacy_console_presentation &&
            !console_model.visible()) ||
           !render_interface.ready())
        {
            return;
        }

        render_interface.beginframe();
        context->Render();
        render_interface.endframe();

        if(active_screen &&
           !legacy_console_presentation &&
           active_screen->id == ScreenId::Profile)
        {
            profile_model.renderpreview(
                active_screen->document,
                ui_dimensions.context_width,
                ui_dimensions.context_height);
        }
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
        else if(id == ScreenId::Demos)
            demo_manager.activate();
        else if(id == ScreenId::Settings)
            settings_model.activate();
        else if(id == ScreenId::Profile)
            profile_model.activate();
        else if(id == ScreenId::GameMenu)
            game_menu_model.activate();

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

            if(!applytheme(screen->document))
            {
                std::fprintf(
                    stderr,
                    "Cowbraten UI: failed to apply theme to '%s'\n",
                    screen->path);
                screen->document->Close();
                screen->document = nullptr;
                return false;
            }
        }

        const bool opening = active_screen == nullptr;
        if(active_screen)
            hidedocument(*active_screen);

        screen->document->Show();
        active_screen = screen;
        legacy_console_input = false;
        legacy_console_presentation = false;
        syncconsoleownership();

        if(opening)
            sauerinput::beginmenu();

        return true;
    }

    bool tryroutelegacygui(const char *name)
    {
        if(!name)
            return false;

        ScreenId screen;
        if(!std::strcmp(name, "main"))
            screen = sauerinput::canclosemenu()
                ? ScreenId::GameMenu
                : ScreenId::Profile;
        else if(!std::strcmp(name, "servers"))
            screen = ScreenId::Servers;
        else if(!std::strcmp(name, "demos"))
            screen = ScreenId::Demos;
        else if(!std::strcmp(name, "options"))
            screen = ScreenId::Settings;
        else if(!std::strcmp(name, "playermodel"))
            screen = ScreenId::Profile;
        else
            return false;

        return show(screen);
    }

    void showtest()
    {
        show(ScreenId::Test);
    }

    void showmain()
    {
        full_menu_from_game = false;
        show(sauerinput::canclosemenu()
            ? ScreenId::GameMenu
            : ScreenId::Profile);
    }

    void showfullmenu()
    {
        if(!sauerinput::canclosemenu())
            return;
        full_menu_from_game = true;
        show(ScreenId::Profile);
    }

    void showservers()
    {
        show(ScreenId::Servers);
    }

    void showdemos()
    {
        show(ScreenId::Demos);
    }

    void showdemosrecording()
    {
        if(show(ScreenId::Demos)) demo_manager.showrecording();
    }

    void showdemosplayback()
    {
        if(show(ScreenId::Demos)) demo_manager.showplayback();
    }

    void showdemostrash()
    {
        if(show(ScreenId::Demos)) demo_manager.showtrash();
    }

    void showdemosduels()
    {
        if(show(ScreenId::Demos)) demo_manager.showduels();
    }

    void filterdemos(const char *value)
    {
        if(show(ScreenId::Demos)) demo_manager.setsearch(value);
    }

    void selectfirstdemo()
    {
        if(show(ScreenId::Demos)) demo_manager.selectfirst();
    }

    bool playselecteddemo()
    {
        return demo_manager.playselected();
    }

    void showsettings()
    {
        show(ScreenId::Settings);
    }

    void showprofile()
    {
        show(ScreenId::Profile);
    }

    void settheme(const char *name)
    {
        requested_theme = name && name[0] ? name : "default";
        const char *effective = normalizetheme(requested_theme.c_str());
        if(requested_theme != effective)
        {
            std::fprintf(
                stderr,
                "Cowbraten UI: unknown theme '%s', using default\n",
                requested_theme.c_str());
        }
        activaterequestedtheme();
    }

    void close()
    {
        if(!active_screen)
            return;

        hidedocument(*active_screen);
        active_screen = nullptr;
        full_menu_from_game = false;
        legacy_console_input = false;
        legacy_console_presentation = false;

        sauerinput::endmenu();
    }

    bool modalactive()
    {
        return active_screen != nullptr;
    }

    bool receivesinput()
    {
        return modalactive() && !legacy_console_input;
    }

    bool ownsplaybackcursor()
    {
        return initialized && demo_player.visible() &&
            demo_player.receivesinput();
    }

    void consoleline(int type, const char *text)
    {
        chat_model.pushline(type, text);
        console_model.pushline(type, text);
    }

    void clearconsole()
    {
        chat_model.clear();
        console_model.clear();
    }

    int filterconsole(int filter)
    {
        return chat_model.filterlegacyconsole(filter);
    }

    bool chatinputvisible()
    {
        return initialized && !active_screen && chat_model.inputvisible();
    }

    bool fullconsolevisible()
    {
        return initialized && console_model.visible();
    }

    bool visible()
    {
        return modalactive();
    }
}
