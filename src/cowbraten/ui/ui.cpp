#include "ui.h"
#include "rml_action.h"
#include "rml_chat.h"
#include "rml_console.h"
#include "rml_render.h"
#include "rml_file.h"
#include "rml_server_browser.h"
#include "rml_scoreboard.h"
#include "rml_frag_feedback.h"
#include "rml_gameplay_hud.h"
#include "frag_feedback_snapshot.h"
#include "rml_match_score_hud.h"
#include "rml_loading.h"
#include "rml_demo_manager.h"
#include "rml_demo_player.h"
#include "rml_editor.h"
#include "rml_game_menu.h"
#include "rml_game_setup.h"
#include "rml_legacy_gui.h"
#include "rml_profile.h"
#include "rml_settings.h"
#include "rml_system.h"
#include "rml_utilities.h"
#include "sauer_demo_bridge.h"
#include "sauer_input_bridge.h"
#include "sauer_music_bridge.h"

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

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <array>
#include <string>

extern char *uitheme;
extern int uiscale;
extern int uitextscale;
extern char *uifont;
extern int lastmillis;

namespace cowui
{
    static RmlRenderInterface render_interface;
    static RmlFileInterface file_interface;
    static RmlActionRouter action_router;
    static RmlChat chat_model;
    static RmlConsole console_model;
    static RmlServerBrowser server_browser;
    static RmlScoreboard scoreboard_model;
    static RmlFragFeedback frag_feedback_model;
    static bool frag_feedback_attempted = false;
    static bool frag_feedback_ready = false;
    static bool frag_feedback_allowed = false;
    static RmlGameplayHud gameplay_hud_model;
    static bool gameplay_hud_ready = false;
    static RmlMatchScoreHud match_score_hud_model;
    static bool match_score_hud_ready = false;
    static RmlLoading loading_model;
    static bool loading_ready = false;
    static RmlDemoManager demo_manager;
    static RmlDemoPlayer demo_player;
    static RmlGameMenu game_menu_model;
    static RmlGameSetup game_setup_model;
    static RmlSystem system_model;
    static RmlUtilities utilities_model;
    static RmlEditor editor_model;
    static RmlProfile profile_model;
    static RmlSettings settings_model;
    static RmlLegacyGui legacy_gui_model;
    static Rml::Context *context = nullptr;
    static FrameStats frame_stats;
    static bool profiling_enabled = false;

    static double elapsedmicroseconds(Uint64 begin, Uint64 end)
    {
        static const double scale =
            1000000.0 / double(SDL_GetPerformanceFrequency());
        return double(end - begin) * scale;
    }

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
            ScreenId::Games,
            "data/cowbraten/ui/default/screens/games.rml",
            nullptr
        },
        {
            ScreenId::System,
            "data/cowbraten/ui/default/screens/system.rml",
            nullptr
        },
        {
            ScreenId::Utilities,
            "data/cowbraten/ui/default/screens/utilities.rml",
            nullptr
        },
        {
            ScreenId::Editor,
            "data/cowbraten/ui/default/screens/editor.rml",
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
        },
        {
            ScreenId::LegacyGui,
            "data/cowbraten/ui/default/screens/legacy_gui.rml",
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

    void applyinterfacepreferences(Rml::ElementDocument *document)
    {
        if(!document)
            return;
        document->SetClass("ui-text-large", uitextscale >= 115);
        document->SetProperty(
            "font-size",
            Rml::CreateString("%.3fdp", 16.0f * float(uitextscale) / 100.0f)
        );
        const char *font = uifont && !std::strcmp(uifont, "legacy")
            ? "Legacy"
            : "Lato";
        document->SetProperty("--font-interface", font);
        document->SetProperty("font-family", font);
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

    static void showgamescreen(Rml::Event &event)
    {
        event.StopPropagation();
        preservegamemenubackstack();
        show(ScreenId::Games);
    }

    static void showaboutscreen(Rml::Event &event)
    {
        event.StopPropagation();
        preservegamemenubackstack();
        if(show(ScreenId::System)) system_model.activate(1);
    }

    static void showadminscreen(Rml::Event &event)
    {
        event.StopPropagation();
        preservegamemenubackstack();
        if(show(ScreenId::System)) system_model.activate(0);
    }

    static void showutilitiesscreen(Rml::Event &event)
    {
        event.StopPropagation();
        preservegamemenubackstack();
        show(ScreenId::Utilities);
    }

    static void showeditorscreen(Rml::Event &event)
    {
        event.StopPropagation();
        preservegamemenubackstack();
        show(ScreenId::Editor);
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

    static float responsiveuiscale(int window_width, int window_height)
    {
        // Treat 1920x1080 as the desktop design viewport. Physical desktop
        // size does not grow linearly with resolution: QHD is commonly still
        // a 27-inch display, while 4K benefits from a stronger accessibility
        // boost. This is the equivalent of a conservative large-screen media
        // query, independent from SDL's HiDPI pixel density.
        const float width_scale = float(window_width) / 1920.0f;
        const float height_scale = float(window_height) / 1080.0f;
        const float viewport_scale = std::min(width_scale, height_scale);
        if(viewport_scale <= 1.0f)
            return 1.0f;
        if(viewport_scale <= 1.5f)
            return 1.0f + (viewport_scale - 1.0f) * 0.25f;
        return std::min(
            1.5f,
            1.125f + (viewport_scale - 1.5f) * 0.75f
        );
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
        const float pixel_density = horizontal_density < vertical_density
            ? horizontal_density
            : vertical_density;
        const float density_ratio =
            pixel_density *
            responsiveuiscale(window_width, window_height) *
            float(uiscale) / 100.0f;

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

        if(!Rml::LoadFontFace(
            "packages/fonts/default.ttf",
            "Legacy",
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
               "navigation.games",
               showgamescreen) ||
           !action_router.registeraction(
               "navigation.about",
               showaboutscreen) ||
           !action_router.registeraction(
               "navigation.admin",
               showadminscreen) ||
           !action_router.registeraction(
               "navigation.utilities",
               showutilitiesscreen) ||
           !action_router.registeraction(
               "navigation.editor",
               showeditorscreen) ||
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
           !scoreboard_model.init(context, action_router) ||
           !(match_score_hud_ready = match_score_hud_model.init(context)) ||
           !demo_manager.init(context, action_router) ||
           !demo_player.init(context, action_router) ||
           !game_menu_model.init(context, action_router) ||
           !game_setup_model.init(context, action_router) ||
           !system_model.init(context, action_router) ||
           !utilities_model.init(context, action_router) ||
           !editor_model.init(context, action_router) ||
           !profile_model.init(context, action_router) ||
           !settings_model.init(context, action_router) ||
           !legacy_gui_model.init(context))
        {
            legacy_gui_model.shutdown(context);
            settings_model.shutdown(context);
            profile_model.shutdown(context);
            editor_model.shutdown(context);
            utilities_model.shutdown(context);
            system_model.shutdown(context);
            game_setup_model.shutdown(context);
            game_menu_model.shutdown(context);
            demo_player.shutdown(context);
            demo_manager.shutdown(context);
            match_score_hud_model.shutdown(context);
            match_score_hud_ready = false;
            frag_feedback_model.shutdown(context);
            frag_feedback_ready = false;
            frag_feedback_attempted = false;
            frag_feedback_allowed = false;
            gameplay_hud_model.shutdown(context);
            gameplay_hud_ready = false;
            scoreboard_model.shutdown(context);
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

        loading_ready = loading_model.init(context);
        if(loading_ready)
        {
            Rml::ElementDocument *loading_document = loading_model.getdocument();
            if(!applytheme(loading_document))
            {
                loading_model.shutdown();
                loading_ready = false;
            }
            else applyinterfacepreferences(loading_document);
        }

        // Optional module. If its RML is missing, keep vanilla HUD available.
        gameplay_hud_ready = gameplay_hud_model.init(context);
        initialized = true;
        interfacepreferenceschanged();
        return true;
    }

    void shutdown()
    {
        sauermusic::sync(false);
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
            loading_model.shutdown();
            loading_ready = false;
            legacy_gui_model.shutdown(context);
            settings_model.shutdown(context);
            profile_model.shutdown(context);
            editor_model.shutdown(context);
            utilities_model.shutdown(context);
            system_model.shutdown(context);
            game_setup_model.shutdown(context);
            game_menu_model.shutdown(context);
            demo_player.shutdown(context);
            demo_manager.shutdown(context);
            match_score_hud_model.shutdown(context);
            match_score_hud_ready = false;
            frag_feedback_model.shutdown(context);
            frag_feedback_ready = false;
            frag_feedback_attempted = false;
            frag_feedback_allowed = false;
            gameplay_hud_model.shutdown(context);
            gameplay_hud_ready = false;
            scoreboard_model.shutdown(context);
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
            loading_model.shutdown();
            loading_ready = false;
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

    void interfacepreferenceschanged()
    {
        if(!initialized || !context)
            return;

        bool context_changed = false;
        setdimensions(
            ui_dimensions.window_width,
            ui_dimensions.window_height,
            ui_dimensions.drawable_width,
            ui_dimensions.drawable_height,
            context_changed
        );
        if(context_changed)
        {
            context->SetDimensions(Rml::Vector2i(
                ui_dimensions.context_width,
                ui_dimensions.context_height
            ));
            context->SetDensityIndependentPixelRatio(
                ui_dimensions.density_ratio
            );
        }

        const int document_count = context->GetNumDocuments();
        for(int i = 0; i < document_count; ++i)
            applyinterfacepreferences(context->GetDocument(i));
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

    bool renderloadingbackground(const char *caption, const char *mapshot,
        const char *mapname, const char *mapinfo)
    {
        if(!initialized || !context || !loading_ready)
            return false;
        return loading_model.background(context, render_interface,
            caption, mapshot, mapname, mapinfo);
    }

    bool renderloadingprogress(float value, const char *text,
        bool full_background)
    {
        if(!initialized || !context || !loading_ready)
            return false;
        return loading_model.progress(context, render_interface,
            value, text, full_background);
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

        if(!active_screen && scoreboard_model.visible() &&
           event.type == SDL_MOUSEWHEEL && event.wheel.y)
        {
            scoreboard_model.scroll(float(event.wheel.y));
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

    void setprofilingenabled(bool enabled)
    {
        profiling_enabled = enabled;
        render_interface.setprofilingenabled(enabled);
        frame_stats = FrameStats();
        render_interface.resetframestats();
    }

    void beginframestats()
    {
        if(!profiling_enabled) return;
        frame_stats = FrameStats();
        render_interface.resetframestats();
    }

    void update()
    {
        if(!initialized || !context)
            return;

	sauermusic::sync(active_screen != nullptr);

        const Uint64 model_begin = profiling_enabled
            ? SDL_GetPerformanceCounter() : 0;

        if(active_screen)
            syncconsoleownership();

        scoreboard_model.update(active_screen != nullptr);
        if(gameplay_hud_ready) gameplay_hud_model.update(active_screen == nullptr);
        // Optional modern feedback. Failure keeps the original HUD renderer.
        if(!frag_feedback_attempted)
        {
            frag_feedback_attempted = true;
            frag_feedback_ready = frag_feedback_model.init(context);
        }
        if(frag_feedback_ready)
            // Keep death notifications visible even when deathscore automatically
            // opens the scoreboard. The legacy frag HUD is not gated by it.
            frag_feedback_model.update(lastmillis,
                frag_feedback_allowed && active_screen == nullptr);
        if(active_screen && match_score_hud_ready) match_score_hud_model.update(MatchHudInput());
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
            else if(active_screen->id == ScreenId::Games)
                game_setup_model.update();
            else if(active_screen->id == ScreenId::System)
                system_model.update();
            else if(active_screen->id == ScreenId::GameMenu)
                game_menu_model.update();
            else if(active_screen->id == ScreenId::LegacyGui)
                legacy_gui_model.update();
        }

        if(!demo_player.visible() &&
           !scoreboard_model.visible() &&
           !gameplay_hud_model.visible() &&
           !chat_model.visible() &&
           !console_model.visible() &&
           (!active_screen || legacy_console_presentation))
        {
            if(profiling_enabled)
                frame_stats.model_update_us = elapsedmicroseconds(
                    model_begin, SDL_GetPerformanceCounter());
            return;
        }

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
        const Uint64 context_begin = profiling_enabled
            ? SDL_GetPerformanceCounter() : 0;
        if(profiling_enabled)
            frame_stats.model_update_us = elapsedmicroseconds(
                model_begin, context_begin);
        context->Update(false);
        if(profiling_enabled)
            frame_stats.context_update_us = elapsedmicroseconds(
                context_begin, SDL_GetPerformanceCounter());
        if(active_screen && active_screen->id == ScreenId::Settings)
            settings_model.scrollpendinglegacyroute(active_screen->document);
    }

    void render()
    {
        if(!initialized ||
           !context ||
           (!active_screen &&
            !scoreboard_model.visible() &&
            !gameplay_hud_model.visible() &&
            !demo_player.visible() &&
            !chat_model.visible() &&
            !console_model.visible()) ||
           (active_screen && legacy_console_presentation &&
            !console_model.visible()) ||
           !render_interface.ready())
        {
            return;
        }

        const Uint64 render_begin = profiling_enabled
            ? SDL_GetPerformanceCounter() : 0;
        render_interface.beginframe();
        context->Render();
        render_interface.endframe();
        if(profiling_enabled)
            frame_stats.context_render_us = elapsedmicroseconds(
                render_begin, SDL_GetPerformanceCounter());

        if(profiling_enabled)
        {
            const RmlRenderInterface::FrameStats render_stats =
                render_interface.framestats();
            frame_stats.draw_calls = render_stats.draw_calls;
            frame_stats.geometry_compiles = render_stats.geometry_compiles;
            frame_stats.geometry_releases = render_stats.geometry_releases;
            frame_stats.texture_binds = render_stats.texture_binds;
            frame_stats.uniform_updates = render_stats.uniform_updates;
            frame_stats.vertex_array_binds = render_stats.vertex_array_binds;
            frame_stats.scissor_changes = render_stats.scissor_changes;
            frame_stats.live_geometries = render_stats.live_geometries;
            frame_stats.live_textures = render_stats.live_textures;
            frame_stats.geometry_cpu_bytes = render_stats.geometry_cpu_bytes;
            frame_stats.texture_cpu_bytes = render_stats.texture_cpu_bytes;
        }

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

    FrameStats framestats()
    {
        const RmlRenderInterface::FrameStats render_stats =
            render_interface.framestats();
        frame_stats.live_geometries = render_stats.live_geometries;
        frame_stats.live_textures = render_stats.live_textures;
        frame_stats.geometry_cpu_bytes = render_stats.geometry_cpu_bytes;
        frame_stats.texture_cpu_bytes = render_stats.texture_cpu_bytes;
        if(context)
        {
            frame_stats.documents = context->GetNumDocuments();
            frame_stats.visible_documents = 0;
            for(int i = 0; i < frame_stats.documents; ++i)
            {
                Rml::ElementDocument *document = context->GetDocument(i);
                if(document && document->IsVisible())
                    ++frame_stats.visible_documents;
            }
        }
        return frame_stats;
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
            else if(id == ScreenId::Utilities)
                utilities_model.activate();
            else if(id == ScreenId::Editor)
                editor_model.activate();

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
        else if(id == ScreenId::Games)
            game_setup_model.activate();
        else if(id == ScreenId::System)
            system_model.activate();
        else if(id == ScreenId::Utilities)
            utilities_model.activate();
        else if(id == ScreenId::Editor)
            editor_model.activate();
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
            applyinterfacepreferences(screen->document);
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
        else if(!std::strcmp(name, "crosshair") ||
                !std::strcmp(name, "soundpacks") ||
                !std::strcmp(name, "trailcolors") ||
                !std::strcmp(name, "ammobar") ||
                !std::strcmp(name, "gameclock") ||
                !std::strcmp(name, "hudscore") ||
                !std::strcmp(name, "scoreboard") ||
                !std::strcmp(name, "hudfragmessages"))
        {
            if(!show(ScreenId::Settings)) return false;
            return settings_model.activatelegacyroute(name);
        }
        else if(!std::strcmp(name, "master") ||
                !std::strcmp(name, "cowbratenusage"))
        {
            if(!show(ScreenId::System)) return false;
            system_model.activate(0);
            return true;
        }
        else if(!std::strcmp(name, "about"))
        {
            if(!show(ScreenId::System)) return false;
            system_model.activate(1);
            return true;
        }
        else if(!std::strcmp(name, "notepad"))
        {
            if(!show(ScreenId::Utilities)) return false;
            utilities_model.activate(0);
            return true;
        }
        else if(!std::strcmp(name, "scratchpad"))
        {
            if(!show(ScreenId::Utilities)) return false;
            utilities_model.activate(1);
            return true;
        }
        else if(!std::strcmp(name, "pastebuffer"))
        {
            if(!show(ScreenId::Utilities)) return false;
            utilities_model.activate(2);
            return true;
        }
        else if(!std::strcmp(name, "editing") ||
                !std::strcmp(name, "quickedit") ||
                !std::strcmp(name, "brushes"))
        {
            if(!show(ScreenId::Editor)) return false;
            editor_model.activate(0);
            return true;
        }
        else if(!std::strcmp(name, "maplighting"))
        {
            if(!show(ScreenId::Editor)) return false;
            editor_model.activate(2);
            return true;
        }
        else if(!std::strcmp(name, "materials") ||
                !std::strcmp(name, "postfx") ||
                !std::strcmp(name, "seltexinfo"))
        {
            if(!show(ScreenId::Editor)) return false;
            editor_model.activate(3);
            return true;
        }
        else if(!std::strcmp(name, "triggers") ||
                !std::strcmp(name, "trigger2") ||
                !std::strcmp(name, "trigger3") ||
                !std::strcmp(name, "newlight") ||
                !std::strcmp(name, "newteleport") ||
                !std::strcmp(name, "mapmodels") ||
                !std::strcmp(name, "monsters"))
        {
            if(!show(ScreenId::Editor)) return false;
            editor_model.activate(1);
            return true;
        }
        else if(!std::strcmp(name, "light") ||
                !std::strcmp(name, "spotlight") ||
                !std::strcmp(name, "playerstart") ||
                !std::strcmp(name, "teleport") ||
                !std::strcmp(name, "teledest") ||
                !std::strcmp(name, "monster") ||
                !std::strcmp(name, "mapmodel") ||
                !std::strcmp(name, "envmap") ||
                !std::strcmp(name, "jumppad") ||
                !std::strcmp(name, "sound") ||
                !std::strcmp(name, "particles") ||
                !std::strcmp(name, "shells") ||
                !std::strcmp(name, "bullets") ||
                !std::strcmp(name, "rockets") ||
                !std::strcmp(name, "riflerounds") ||
                !std::strcmp(name, "grenades") ||
                !std::strcmp(name, "cartridges") ||
                !std::strcmp(name, "health") ||
                !std::strcmp(name, "healthboost") ||
                !std::strcmp(name, "greenarmour") ||
                !std::strcmp(name, "yellowarmour") ||
                !std::strcmp(name, "quaddamage") ||
                !std::strcmp(name, "carrot") ||
                !std::strcmp(name, "base") ||
                !std::strcmp(name, "respawnpoint") ||
                !std::strcmp(name, "box") ||
                !std::strcmp(name, "barrel") ||
                !std::strcmp(name, "platform") ||
                !std::strcmp(name, "elevator") ||
                !std::strcmp(name, "flag"))
        {
            if(!show(ScreenId::Editor)) return false;
            editor_model.activate(5);
            return true;
        }
        else if(!std::strcmp(name, "gamemode") ||
                !std::strcmp(name, "maps") ||
                !std::strcmp(name, "capturemaps") ||
                !std::strcmp(name, "ctfmaps") ||
                !std::strcmp(name, "custommaps"))
            screen = ScreenId::Games;
        else if(!std::strcmp(name, "botmatch"))
        {
            if(!show(ScreenId::Games)) return false;
            game_setup_model.activate(1);
            return true;
        }
        else if(!std::strcmp(name, "campaign") ||
                !std::strcmp(name, "armyofone") ||
                !std::strcmp(name, "privatestansauer"))
        {
            if(!show(ScreenId::Games)) return false;
            game_setup_model.activate(2);
            return true;
        }
        else
        {
            if(!legacy_gui_model.activate(name)) return false;
            return show(ScreenId::LegacyGui);
        }

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

    void showgames()
    {
        show(ScreenId::Games);
    }

    void showadmin()
    {
        if(show(ScreenId::System)) system_model.activate(0);
    }

    void showabout()
    {
        if(show(ScreenId::System)) system_model.activate(1);
    }

    void showutilities()
    {
        if(show(ScreenId::Utilities)) utilities_model.activate(0);
    }

    void showscratchpad()
    {
        if(show(ScreenId::Utilities)) utilities_model.activate(1);
    }

    void showpastebuffer()
    {
        if(show(ScreenId::Utilities)) utilities_model.activate(2);
    }

    void showeditor()
    {
        show(ScreenId::Editor);
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
	sauermusic::sync(false);
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

    // Called from fpsgame::gameplayhud while keeping the existing toggle vars.
    bool matchhudactive() { return match_score_hud_ready; }

    void submitmatchhud(bool enabled_score, bool enabled_clock, bool score_available,
                        bool clock_available, int left, int right,
                        bool blue_on_left, const char *time)
    {
        if(!match_score_hud_ready) return;
        MatchHudInput input;
        input.score_enabled = enabled_score;
        input.clock_enabled = enabled_clock;
        input.score_available = score_available;
        input.clock_available = clock_available;
        input.left_score = left;
        input.right_score = right;
        input.clock_text = time ? time : "";
        match_score_hud_model.update(input, blue_on_left);
    }

    // Lower status dock bridge: the original OpenGL renderer remains intact.
    bool gameplayhudactive() { return gameplay_hud_ready; }

    void syncgameplayhud(const GameplayHudSnapshot &state)
    {
        if(gameplay_hud_ready) gameplay_hud_model.sync(state);
    }

    // Called from the game HUD. The original frag queue stays intact as fallback.
    bool fragfeedbackactive() { return frag_feedback_ready; }

    void setfragfeedbackallowed(bool value) { frag_feedback_allowed = value; }

    void syncfragfeedback(const FragFeedbackSnapshot &snapshot)
    {
        if(frag_feedback_ready)
            frag_feedback_model.sync(snapshot);
    }

    bool setscoreboardvisible(bool value)
    {
        if(!initialized || !context) return false;
        scoreboard_model.request(value);
        return true;
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
