#ifndef COWBRATEN_UI_RML_SETTINGS_H
#define COWBRATEN_UI_RML_SETTINGS_H

#include "keybindings_api.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <vector>

namespace Rml
{
    class Context;
    class Event;
}

namespace cowui
{
    namespace settings
    {
        enum class PresentationMode;
        struct Gamma;
        struct CrosshairChoice;
        struct SoundpackChoice;
        struct AnisotropyState;
        struct BilinearFilteringState;
        struct MipmapBlendingState;
        struct ShaderDetailState;
        struct ShadowState;
        struct ShadowQualityState;
        struct WaterSurfaceEffectsState;
        struct WaterEnvironmentReflectionsState;
        struct WaterRenderResolutionState;
        struct WaterfallEffectsState;
        struct GameFeedback;
        struct GameRadar;
        struct GameModels;
        struct Ammobar;
        struct GameClock;
        struct GlassReflectionsState;
        struct ModelReflectionsState;
        struct BumpModelsState;
        struct EnvironmentMapResolutionState;
        struct DynamicLightsState;
        struct SoftParticlesState;
        struct SoftParticleQualityState;
        struct DecalsState;
        struct DecalPersistenceState;
        struct FixTJointsState;
        struct GrassDistanceState;
        struct MotionBlurIntensityState;
        struct GlarePresetState;
        struct GlareIntensityState;
        struct GlareResolutionState;
        struct TextureQualityState;
        struct TextureReductionState;
        struct TextureCompressionState;
    }

    class RmlActionRouter;

    class RmlSettings final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);

        void activate();
        void videochanged();
        void applypending();
        bool capturingkey() const { return key_capture_active; }
        void cancelkeycapture();
        void acceptkeycandidate(const char *name);

    private:
        struct Resolution
        {
            Rml::String label;
            int width = 0;
            int height = 0;
            bool selected = false;
            bool supported = false;

            bool operator==(const Resolution &other) const
            {
                return label == other.label &&
                    width == other.width &&
                    height == other.height &&
                    selected == other.selected &&
                    supported == other.supported;
            }
        };

        struct CrosshairChoice
        {
            Rml::String label;
            Rml::String resource;
            int id = -1;
            bool selected = false;

            bool operator==(const CrosshairChoice &other) const
            {
                return label == other.label &&
                    resource == other.resource &&
                    id == other.id &&
                    selected == other.selected;
            }
        };

        struct SoundpackChoice
        {
            Rml::String id;
            Rml::String label;
            bool selected = false;

            bool operator==(const SoundpackChoice &other) const
            {
                return id == other.id &&
                    label == other.label &&
                    selected == other.selected;
            }
        };

        struct KeyBindingRow
        {
            int action_id = -1;
            Rml::String label;
            std::vector<Rml::String> keys;
            bool bound = false;

            bool operator==(const KeyBindingRow &other) const
            {
                return action_id == other.action_id && label == other.label &&
                    keys == other.keys &&
                    bound == other.bound;
            }
        };

        enum class Category
        {
            Game,
            Hud,
            Graphics,
            Audio,
            MouseKeyboard,
            Crosshair,
            Chat
        };

        bool refresh();
        bool refreshgamefeedback();
        void presentgamefeedback(const settings::GameFeedback &state,
            bool available);
        void selectblood(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void selecthitsound(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void selectoutlinecapturemeters(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectshowscoreboardatdeath(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        bool refreshgameradar();
        void presentgameradar(const settings::GameRadar &state,
            bool available);
        void selectnumberedcapturebases(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectshowteammatesonradar(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        bool refreshgamemodels();
        void presentgamemodels(const settings::GameModels &state,
            bool available);
        void selectragdolldeaths(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectkeepragdollafterrespawn(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selecthidedeadplayers(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectfullbrightplayermodels(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectfullbrightintensity(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectforceplayermodels(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectalwaysuseteamskins(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        bool refreshammobar();
        void presentammobar(const settings::Ammobar &state, bool available);
        void finishammobarwrite(bool accepted, settings::Ammobar &state);
        void selectammobar(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void selectammobarhorizontal(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectammobarflip(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void selectammobarhideempty(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectammobaralign(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        bool refreshvideo();
        void refreshshaderdetail();
        void presentshaderdetail(const settings::ShaderDetailState &state);
        void selectshaderdetail(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void applypendingshaderdetail();
        bool refreshgraphicsquality();
        void refreshshadows();
        void presentshadows(const settings::ShadowState &state,
            bool available);
        void selectshadowmaps(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void selectblobshadows(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void refreshshadowquality();
        void presentshadowquality(
            const settings::ShadowQualityState &state,
            bool available);
        void selectshadowquality(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void refreshwatersurfaceeffects();
        void presentwatersurfaceeffects(
            const settings::WaterSurfaceEffectsState &state,
            bool available);
        void selectwaterrefraction(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectwaterreflections(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectwatercaustics(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectliquidanimation(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshwaterenvironmentreflections();
        void presentwaterenvironmentreflections(
            const settings::WaterEnvironmentReflectionsState &state,
            bool available);
        void selectwaterenvironmentreflections(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshwaterrenderresolution();
        void presentwaterrenderresolution(
            const settings::WaterRenderResolutionState &state,
            bool available);
        void selectwaterrenderresolution(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshwaterfalleffects();
        void presentwaterfalleffects(
            const settings::WaterfallEffectsState &state,
            bool available);
        void selectwaterfallrefraction(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void selectwaterfallreflections(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshglassreflections();
        void presentglassreflections(
            const settings::GlassReflectionsState &state,
            bool available);
        void selectglassreflections(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshmodelreflections();
        void presentmodelreflections(
            const settings::ModelReflectionsState &state,
            bool available);
        void selectmodelreflections(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshbumpmodels();
        void presentbumpmodels(
            const settings::BumpModelsState &state,
            bool available);
        void selectbumpmodels(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshenvironmentmapresolution();
        void presentenvironmentmapresolution(
            const settings::EnvironmentMapResolutionState &state,
            bool available);
        void selectenvironmentmapresolution(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshdynamiclights();
        void presentdynamiclights(
            const settings::DynamicLightsState &state,
            bool available);
        void selectdynamiclights(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshsoftparticles();
        void presentsoftparticles(
            const settings::SoftParticlesState &state,
            bool available);
        void selectsoftparticles(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshsoftparticlequality();
        void presentsoftparticlequality(
            const settings::SoftParticleQualityState &state,
            bool available);
        void selectsoftparticlequality(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshdecals();
        void presentdecals(const settings::DecalsState &state,
            bool available);
        void selectdecals(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void refreshdecalpersistence();
        void presentdecalpersistence(
            const settings::DecalPersistenceState &state,
            bool available);
        void selectdecalpersistence(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshfixtjoints();
        void presentfixtjoints(const settings::FixTJointsState &state,
            bool available);
        void selectfixtjoints(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void refreshanisotropy();
        void refreshbilinearfiltering();
        void presentbilinearfiltering(
            const settings::BilinearFilteringState &state,
            bool available);
        void selectbilinearfiltering(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshmipmapblending();
        void presentmipmapblending(const settings::MipmapBlendingState &state);
        void selectmipmapblending(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void selectglare(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void refreshgrassdistance();
        void presentgrassdistance(const settings::GrassDistanceState &state,
            bool available);
        void selectgrassdistance(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void refreshmotionblurintensity();
        void presentmotionblurintensity(
            const settings::MotionBlurIntensityState &state,
            bool available);
        void selectmotionblurintensity(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void refreshglarepreset();
        void presentglarepreset(const settings::GlarePresetState &state,
            bool available);
        void selectglarepreset(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void refreshglareintensity();
        void presentglareintensity(
            const settings::GlareIntensityState &state,
            bool available);
        void refreshglareresolution();
        void presentglareresolution(
            const settings::GlareResolutionState &state,
            bool available);
        void selectglareresolution(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void refreshtexturequality();
        void presenttexturequality(
            const settings::TextureQualityState &state,
            bool available);
        void selecttexturequality(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void refreshtexturereduction();
        void presenttexturereduction(
            const settings::TextureReductionState &state,
            bool available);
        void refreshtexturecompression();
        void presenttexturecompression(
            const settings::TextureCompressionState &state,
            bool available);
        void selecttexturecompression(Rml::DataModelHandle,
            Rml::Event &event, const Rml::VariantList &arguments);
        void presentanisotropy(const settings::AnisotropyState &state);
        void selectanisotropy(Rml::DataModelHandle, Rml::Event &event,
                              const Rml::VariantList &arguments);
        void refreshgamma();
        void presentgamma(const settings::Gamma &state, bool available);
        bool refreshaudio();
        bool refreshmouseinput(bool update_edit_buffer = false);
        bool refreshcrosshair(bool update_edit_buffer = false);
        bool refreshhud(bool update_edit_buffer = false);
        bool refreshgameclock();
        void presentgameclock(const settings::GameClock &state,
            bool available);
        void selectgameclock(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void selectgameclockalign(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void commitgameclockscale();
        void commitgameclockx();
        void commitgameclocky();
        bool refreshchat();
        bool refreshkeybindings();
        bool refreshkeyconflict();
        void clearkeyremoval();
        void requestkeyremoval(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void decidekeyremoval(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void decidekeycandidate(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void clearkeyconflict();
        static const char *keybindinglabel(keybindings::Action action);
        void startkeycapture(Rml::DataModelHandle, Rml::Event &event,
            const Rml::VariantList &arguments);
        void commitsensitivity();
        void commitcrosshairsize();
        void commitstreakscale();
        void commitammobarscale();
        void commitammobarlabelsize();
        void commitammobarx();
        void commitammobary();
        void setcategory(Category category);
        void setpresentation(
            settings::PresentationMode presentation,
            Rml::Event &event
        );
        void selectresolution(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        void selectantialiasing(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        void selectdepthbuffer(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        void selectcrosshair(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        void selectsoundpack(
            Rml::DataModelHandle,
            Rml::Event &event,
            const Rml::VariantList &arguments
        );
        static void appendcrosshairchoice(
            const settings::CrosshairChoice &choice,
            void *data
        );
        static void appendsoundpackchoice(
            const settings::SoundpackChoice &choice,
            void *data
        );
        static void changefovaction(Rml::Event &event);
        static void changedeadpushaction(Rml::Event &event);
        static void gamecategoryaction(Rml::Event &event);
        static void hudcategoryaction(Rml::Event &event);
        static void graphicscategoryaction(Rml::Event &event);
        static void audiocategoryaction(Rml::Event &event);
        static void mousekeyboardcategoryaction(Rml::Event &event);
        static void crosshaircategoryaction(Rml::Event &event);
        static void chatcategoryaction(Rml::Event &event);
        static void windowedaction(Rml::Event &event);
        static void exclusivefullscreenaction(Rml::Event &event);
        static void desktopfullscreenaction(Rml::Event &event);
        static void vsyncaction(Rml::Event &event);
        static void changegammaaction(Rml::Event &event);
        static void grassaction(Rml::Event &event);
        static void motionbluraction(Rml::Event &event);
        static void changeglareintensityaction(Rml::Event &event);
        static void changefpslimitaction(Rml::Event &event);
        static void changetexturereductionaction(Rml::Event &event);
        static void changesoundvolumeaction(Rml::Event &event);
        static void changemusicvolumeaction(Rml::Event &event);
        static void decreasesensitivityaction(Rml::Event &event);
        static void increasesensitivityaction(Rml::Event &event);
        static void focussensitivityaction(Rml::Event &event);
        static void blursensitivityaction(Rml::Event &event);
        static void commitsensitivityaction(Rml::Event &event);
        static void invertedmouseaction(Rml::Event &event);
        static void decreasecrosshairsizeaction(Rml::Event &event);
        static void increasecrosshairsizeaction(Rml::Event &event);
        static void focuscrosshairsizeaction(Rml::Event &event);
        static void blurcrosshairsizeaction(Rml::Event &event);
        static void commitcrosshairsizeaction(Rml::Event &event);
        static void crosshaireffectsaction(Rml::Event &event);
        static void crosshairhealthcolorsaction(Rml::Event &event);
        static void teamcrosshairaction(Rml::Event &event);
        static void hitcrosshairaction(Rml::Event &event);
        static void crosshairreloadfadeaction(Rml::Event &event);
        static void crosshairreloadrotationaction(Rml::Event &event);
        static void showfpsaction(Rml::Event &event);
        static void wallclockaction(Rml::Event &event);
        static void wallclock24action(Rml::Event &event);
        static void wallclocksecondsaction(Rml::Event &event);
        static void damagescreenaction(Rml::Event &event);
        static void damagecompassaction(Rml::Event &event);
        static void hudgunsaction(Rml::Event &event);
        static void hudgunswayaction(Rml::Event &event);
        static void muzzleflashaction(Rml::Event &event);
        static void muzzlelightaction(Rml::Event &event);
        static void fragmessagesenabledaction(Rml::Event &event);
        static void fragmessagesinstaaction(Rml::Event &event);
        static void decreasestreakscaleaction(Rml::Event &event);
        static void increasestreakscaleaction(Rml::Event &event);
        static void focusstreakscaleaction(Rml::Event &event);
        static void blurstreakscaleaction(Rml::Event &event);
        static void commitstreakscaleaction(Rml::Event &event);
        static void changeammobariconspacingaction(Rml::Event &event);
        static void changeammobarlabeloffsetaction(Rml::Event &event);
        static void decreaseammobarscaleaction(Rml::Event &event);
        static void increaseammobarscaleaction(Rml::Event &event);
        static void focusammobarscaleaction(Rml::Event &event);
        static void blurammobarscaleaction(Rml::Event &event);
        static void commitammobarscaleaction(Rml::Event &event);
        static void decreaseammobarlabelsizeaction(Rml::Event &event);
        static void increaseammobarlabelsizeaction(Rml::Event &event);
        static void focusammobarlabelsizeaction(Rml::Event &event);
        static void blurammobarlabelsizeaction(Rml::Event &event);
        static void commitammobarlabelsizeaction(Rml::Event &event);
        static void decreaseammobarxaction(Rml::Event &event);
        static void increaseammobarxaction(Rml::Event &event);
        static void focusammobarxaction(Rml::Event &event);
        static void blurammobarxaction(Rml::Event &event);
        static void commitammobarxaction(Rml::Event &event);
        static void decreaseammobaryaction(Rml::Event &event);
        static void increaseammobaryaction(Rml::Event &event);
        static void focusammobaryaction(Rml::Event &event);
        static void blurammobaryaction(Rml::Event &event);
        static void commitammobaryaction(Rml::Event &event);
        static void changegameclockalphaaction(Rml::Event &event);
        static void decreasegameclockscaleaction(Rml::Event &event);
        static void increasegameclockscaleaction(Rml::Event &event);
        static void focusgameclockscaleaction(Rml::Event &event);
        static void blurgameclockscaleaction(Rml::Event &event);
        static void commitgameclockscaleaction(Rml::Event &event);
        static void decreasegameclockxaction(Rml::Event &event);
        static void increasegameclockxaction(Rml::Event &event);
        static void focusgameclockxaction(Rml::Event &event);
        static void blurgameclockxaction(Rml::Event &event);
        static void commitgameclockxaction(Rml::Event &event);
        static void decreasegameclockyaction(Rml::Event &event);
        static void increasegameclockyaction(Rml::Event &event);
        static void focusgameclockyaction(Rml::Event &event);
        static void blurgameclockyaction(Rml::Event &event);
        static void commitgameclockyaction(Rml::Event &event);
        static void publicchatintopfeedaction(Rml::Event &event);
        static void teamchatintopfeedaction(Rml::Event &event);
        static void publicchatinbottomboxaction(Rml::Event &event);
        static void teamchatinbottomboxaction(Rml::Event &event);
        static void teamcoloredchatnamesaction(Rml::Event &event);

        Rml::DataModelHandle model;
        std::vector<Resolution> resolutions;
        std::vector<CrosshairChoice> crosshair_choices;
        std::vector<SoundpackChoice> soundpack_choices;
        std::vector<KeyBindingRow> keybindings_movement;
        std::vector<KeyBindingRow> keybindings_combat;
        std::vector<KeyBindingRow> keybindings_interface;
        bool key_capture_active = false;
        keybindings::Action key_capture_action = keybindings::Action::MoveForward;
        int key_capture_id = -1;
        Rml::String key_candidate;
        Rml::String key_capture_status;
        Rml::String key_binding_status;
        bool key_candidate_pending = false;
        bool key_removal_active = false;
        keybindings::Action key_removal_action = keybindings::Action::MoveForward;
        Rml::String key_removal_key;
        Rml::String key_removal_prompt;
        keybindings::Conflict key_conflict;
        Rml::String key_conflict_status;
        bool key_conflict_available = false;
        bool key_conflict_same_action = false;
        bool key_conflict_present = false;

        int fov = 100;
        int fov_minimum = 10;
        int fov_maximum = 150;
        bool game_feedback_available = false;
        bool blood = true;
        bool hit_sound = false;
        bool outline_capture_meters = false;
        bool show_scoreboard_at_death = true;
        Rml::String game_feedback_status;
        bool game_radar_available = false;
        bool numbered_capture_bases = false;
        bool show_teammates_on_radar = true;
        Rml::String game_radar_status;
        bool game_models_available = false;
        bool ragdoll_deaths = true;
        bool keep_ragdoll_after_respawn = true;
        int deadpush = 2;
        int deadpush_minimum = 1;
        int deadpush_maximum = 20;
        bool hide_dead_players = false;
        bool fullbright_player_models = true;
        int fullbright_intensity_selected = 60;
        bool force_matching_player_models = false;
        bool always_use_team_skins = false;
        Rml::String game_models_status;
        bool ammobar_available = false;
        bool ammobar_visible = false;
        bool ammobar_horizontal = false;
        bool ammobar_flip = false;
        float ammobar_scale = 0.5f;
        float ammobar_scale_minimum = 0.1f;
        float ammobar_scale_maximum = 1.0f;
        Rml::String ammobar_scale_input;
        bool ammobar_scale_invalid = false;
        bool ammobar_scale_editing = false;
        int ammobar_icon_spacing = 20;
        int ammobar_icon_spacing_minimum = 0;
        int ammobar_icon_spacing_maximum = 500;
        bool ammobar_icon_spacing_armed = false;
        float ammobar_label_size = 1.5f;
        float ammobar_label_size_minimum = 0.5f;
        float ammobar_label_size_maximum = 2.0f;
        Rml::String ammobar_label_size_input;
        bool ammobar_label_size_invalid = false;
        bool ammobar_label_size_editing = false;
        int ammobar_label_offset = 20;
        int ammobar_label_offset_minimum = 0;
        int ammobar_label_offset_maximum = 500;
        bool ammobar_label_offset_armed = false;
        bool ammobar_align_first = false;
        bool ammobar_align_center = true;
        bool ammobar_align_last = false;
        float ammobar_x = 0.025f;
        float ammobar_x_minimum = 0.0f;
        float ammobar_x_maximum = 1.0f;
        Rml::String ammobar_x_input;
        bool ammobar_x_invalid = false;
        bool ammobar_x_editing = false;
        float ammobar_y = 0.5f;
        float ammobar_y_minimum = 0.0f;
        float ammobar_y_maximum = 1.0f;
        Rml::String ammobar_y_input;
        bool ammobar_y_invalid = false;
        bool ammobar_y_editing = false;
        bool ammobar_hide_empty = true;
        Rml::String ammobar_status;

        Rml::String resolution_label;
        Rml::String render_resolution_label;
        Rml::String video_status;
        Rml::String presentation_label;
        Rml::String vsync_status;
        int gamma = 100;
        int gamma_minimum = 30;
        int gamma_maximum = 300;
        Rml::String gamma_label;
        Rml::String gamma_status;
        bool shadow_state_available = false;
        bool shadow_maps = false;
        bool shadow_maps_available = false;
        bool blob_shadows = false;
        Rml::String shadow_status;
        bool shadow_quality_available = false;
        int shadow_quality_selected = -1;
        Rml::String shadow_quality_label;
        Rml::String shadow_quality_status;
        bool water_surface_state_available = false;
        bool water_refraction = false;
        bool water_refraction_available = false;
        bool water_reflections = false;
        bool water_reflections_available = false;
        bool water_caustics = false;
        bool liquid_surface_animation = false;
        Rml::String water_surface_status;
        bool water_environment_reflections = false;
        bool water_environment_reflections_available = false;
        Rml::String water_environment_reflections_status;
        bool water_render_resolution_available = false;
        int water_render_resolution_selected = -1;
        Rml::String water_render_resolution_label;
        Rml::String water_render_resolution_dimension;
        Rml::String water_render_resolution_status;
        bool waterfall_state_available = false;
        bool waterfall_refraction = false;
        bool waterfall_refraction_available = false;
        bool waterfall_reflections = false;
        Rml::String waterfall_status;
        bool glass_reflections_available = false;
        bool glass_reflections = false;
        Rml::String glass_reflections_status;
        bool model_reflections_available = false;
        bool model_reflections = false;
        Rml::String model_reflections_status;
        bool bump_models_available = false;
        bool bump_models = false;
        Rml::String bump_models_status;
        bool environment_map_resolution_available = false;
        int environment_map_resolution_selected = -1;
        Rml::String environment_map_resolution_label;
        Rml::String environment_map_resolution_dimension;
        Rml::String environment_map_resolution_status;
        bool dynamic_lights_available = false;
        int dynamic_lights_selected = -1;
        Rml::String dynamic_lights_label;
        Rml::String dynamic_lights_status;
        bool soft_particles_available = false;
        bool soft_particles = false;
        Rml::String soft_particles_status;
        bool soft_particle_quality_available = false;
        int soft_particle_quality_selected = -1;
        Rml::String soft_particle_quality_label;
        Rml::String soft_particle_quality_status;
        bool decals_available = false;
        bool decals = false;
        Rml::String decals_status;
        bool decal_persistence_available = false;
        int decal_persistence_selected = -1;
        Rml::String decal_persistence_label;
        Rml::String decal_persistence_status;
        bool fix_t_joints_available = false;
        bool fix_t_joints = false;
        Rml::String fix_t_joints_status;
        bool grass = false;
        bool grass_distance_available = false;
        int grass_distance_selected = -1;
        Rml::String grass_distance_label;
        Rml::String grass_distance_status;
        bool motion_blur = false;
        bool motion_blur_available = false;
        bool motion_blur_intensity_available = false;
        int motion_blur_intensity_selected = -1;
        Rml::String motion_blur_intensity_label;
        Rml::String motion_blur_intensity_status;
        bool glare = false;
        bool glare_preset_available = false;
        int glare_preset_selected = -1;
        Rml::String glare_preset_label;
        Rml::String glare_preset_status;
        bool glare_intensity_available = false;
        float glare_intensity_slider = 1.0f;
        Rml::String glare_intensity_label;
        Rml::String glare_intensity_status;
        bool glare_resolution_available = false;
        int glare_resolution_selected = -1;
        Rml::String glare_resolution_label;
        int glare_resolution_requested_width = 0;
        Rml::String glare_resolution_status;
        bool texture_quality_available = false;
        int texture_quality_selected = -1;
        Rml::String texture_quality_label;
        Rml::String texture_quality_status;
        int texture_reduction = 0;
        int texture_reduction_minimum = 0;
        int texture_reduction_maximum = 12;
        bool texture_reduction_available = false;
        Rml::String texture_reduction_label;
        Rml::String texture_reduction_status;
        int texture_compression_selected = -1;
        bool texture_compression_available = false;
        bool texture_compression_supported = false;
        Rml::String texture_compression_label;
        Rml::String texture_compression_status;
        Rml::String graphics_quality_status;
        bool anisotropy_enabled = false;
        bool bilinear_filtering = false;
        bool bilinear_filtering_available = false;
        Rml::String bilinear_filtering_status;
        bool mipmap_blending = false;
        bool mipmap_blending_available = false;
        Rml::String mipmap_blending_status;
        int anisotropy_maximum = 0;
        int anisotropy_selected = -1;
        Rml::String anisotropy_label;
        Rml::String anisotropy_status;
        Rml::String fps_limit_label;
        bool presentation_windowed = true;
        bool presentation_exclusive = false;
        bool presentation_desktop = false;
        bool render_resolution_visible = false;
        bool resolution_enabled = true;
        bool vsync = false;
        bool vsync_adaptive = false;
        int fps_limit = 200;
        int fps_limit_minimum = 0;
        int fps_limit_maximum = 1000;
        int antialiasing = 0;
        int antialiasing_requested = -1;
        int pending_antialiasing = -1;
        int depth_buffer = 0;
        int depth_buffer_requested = -1;
        int pending_depth_buffer = -1;
        int pending_shader_detail = -1;
        int shader_detail_selected = -1;
        bool shader_detail_available = false;
        Rml::String shader_detail_label;
        Rml::String shader_detail_status;
        Rml::String antialiasing_label;
        Rml::String antialiasing_status;
        bool antialiasing_off = true;
        bool antialiasing_2x = false;
        bool antialiasing_4x = false;
        bool antialiasing_8x = false;
        bool antialiasing_16x = false;
        Rml::String depth_buffer_label;
        Rml::String depth_buffer_status;
        bool depth_buffer_default = true;
        bool depth_buffer_16 = false;
        bool depth_buffer_24 = false;
        bool depth_buffer_32 = false;
        int sound_volume = 255;
        int sound_volume_minimum = 0;
        int sound_volume_maximum = 255;
        int music_volume = 128;
        int music_volume_minimum = 0;
        int music_volume_maximum = 255;
        Rml::String sound_volume_label;
        Rml::String music_volume_label;
        Rml::String audio_status;
        Rml::String active_soundpack;
        Rml::String active_soundpack_label;
        Rml::String soundpack_status;
        float mouse_sensitivity = 3.0f;
        float mouse_sensitivity_minimum = 0.001f;
        float mouse_sensitivity_maximum = 1000.0f;
        Rml::String mouse_sensitivity_label;
        Rml::String mouse_sensitivity_input;
        Rml::String mouse_sensitivity_status;
        bool mouse_sensitivity_invalid = false;
        bool mouse_sensitivity_editing = false;
        bool inverted_mouse = false;
        Rml::String mouse_input_status;
        int crosshair_selection = -1;
        Rml::String crosshair_selection_label;
        Rml::String crosshair_selection_resource;
        int crosshair_size = 30;
        int crosshair_size_minimum = 0;
        int crosshair_size_maximum = 50;
        Rml::String crosshair_size_input;
        Rml::String crosshair_status;
        bool crosshair_size_invalid = false;
        bool crosshair_size_editing = false;
        bool crosshair_effects = true;
        bool crosshair_health_colors = true;
        bool team_crosshair = true;
        bool hit_crosshair = true;
        bool crosshair_reload_fade = true;
        bool crosshair_reload_rotation = true;
        bool show_fps = true;
        bool wall_clock = false;
        bool wall_clock_24_hour = false;
        bool wall_clock_seconds = false;
        bool damage_screen = true;
        bool damage_compass = true;
        bool hud_guns = true;
        bool hud_gun_sway = true;
        bool muzzle_flash = true;
        bool muzzle_light = true;
        bool frag_messages_enabled = true;
        bool frag_messages_in_insta = true;
        float streak_milestone_scale = 1.25f;
        float streak_milestone_scale_minimum = 1.0f;
        float streak_milestone_scale_maximum = 2.0f;
        Rml::String streak_milestone_scale_input;
        Rml::String hud_status;
        bool streak_milestone_scale_invalid = false;
        bool streak_milestone_scale_editing = false;
        bool game_clock_available = false;
        bool game_clock = false;
        float game_clock_scale = 1.0f;
        float game_clock_scale_minimum = 0.5f;
        float game_clock_scale_maximum = 2.0f;
        Rml::String game_clock_scale_input;
        bool game_clock_scale_invalid = false;
        bool game_clock_scale_editing = false;
        int game_clock_alpha = 255;
        int game_clock_alpha_minimum = 0;
        int game_clock_alpha_maximum = 255;
        int game_clock_align = 0;
        float game_clock_x = 0.50f;
        float game_clock_x_minimum = 0.0f;
        float game_clock_x_maximum = 1.0f;
        Rml::String game_clock_x_input;
        bool game_clock_x_invalid = false;
        bool game_clock_x_editing = false;
        float game_clock_y = 0.03f;
        float game_clock_y_minimum = 0.0f;
        float game_clock_y_maximum = 1.0f;
        Rml::String game_clock_y_input;
        bool game_clock_y_invalid = false;
        bool game_clock_y_editing = false;
        Rml::String game_clock_status;
        bool public_chat_in_top_feed = true;
        bool team_chat_in_top_feed = true;
        bool public_chat_in_bottom_box = false;
        bool team_chat_in_bottom_box = false;
        bool team_colored_chat_names = true;
        bool category_game = true;
        bool category_hud = false;
        bool category_graphics = false;
        bool category_audio = false;
        bool category_mouse_keyboard = false;
        bool category_crosshair = false;
        bool category_chat = false;

        static RmlSettings *instance;
    };
}

#endif
