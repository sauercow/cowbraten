#ifndef COWBRATEN_UI_SETTINGS_API_H
#define COWBRATEN_UI_SETTINGS_API_H

namespace cowui
{
    namespace settings
    {
        enum class PresentationMode
        {
            Windowed,
            ExclusiveFullscreen,
            DesktopFullscreen
        };

        enum class Theme
        {
            Default,
            ModernMetallic
        };

        enum class InterfaceFont
        {
            Legacy,
            Lato
        };

        struct InterfaceAppearance
        {
            int ui_scale;
            int ui_scale_minimum;
            int ui_scale_maximum;
            int text_scale;
            int text_scale_minimum;
            int text_scale_maximum;
            InterfaceFont font;
        };

        struct Fov
        {
            int value;
            int minimum;
            int maximum;
        };

        struct GameFeedback
        {
            bool blood;
            bool hit_sound;
            bool outline_capture_meters;
            bool show_scoreboard_at_death;
        };

        // Native Settings Game-tab trailcolors document. Packed HVARP
        // storage; RGB sliders use the native 0..255 channel range.
        struct TrailColors
        {
            int shotgun;
            int chaingun;
            int rocketlauncher;
            int rifle;
            int grenadelauncher;
            int pistol;
            int channel_minimum;
            int channel_maximum;
        };

        struct GameRadar
        {
            bool numbered_capture_bases;
            bool show_teammates_on_radar;
        };

        // Effective idents can diverge from the native Settings writes:
        // ragdollmillis 0/10000, hidedead 0/1, fullbrightmodels
        // 0/60/100/150/200. Opening Settings must not coerce them.
        struct GameModels
        {
            bool ragdoll_deaths;
            bool keep_ragdoll_after_respawn;
            int ragdoll_millis;
            int deadpush;
            int deadpush_minimum;
            int deadpush_maximum;
            bool hide_dead_players;
            int hidedead;
            bool fullbright_player_models;
            int fullbrightmodels;
            bool force_matching_player_models;
            bool always_use_team_skins;
        };

        struct Video
        {
            int width;
            int height;
            int drawable_width;
            int drawable_height;
            PresentationMode presentation;
        };

        struct Vsync
        {
            bool enabled;
            bool adaptive;
        };

        struct FpsLimit
        {
            int value;
            int minimum;
            int maximum;
        };

        // requested is the persistent ident; effective is the last brightness
        // SDL reports for the game window, which can lag or fail independently.
        struct Gamma
        {
            int requested;
            int effective;
            int minimum;
            int maximum;
        };

        struct Antialiasing
        {
            int requested_samples;
            int actual_samples;
        };

        struct DepthBuffer
        {
            int requested_bits;
            int actual_bits;
        };

        // Grass reflects the native switch; motion blur also reflects its
        // renderer availability gate, not per-frame visibility.
        struct GraphicsQuality
        {
            bool grass;
            bool motion_blur;
            bool motion_blur_available;
            bool glare;
        };

        struct ShadowState
        {
            bool shadow_maps;
            bool blob_shadows;
            bool shadow_maps_available;
        };

        enum class ShadowQuality
        {
            Medium,
            High,
            Custom
        };
        struct ShadowQualityState
        {
            ShadowQuality quality;
            int map_size;
            int blur;
        };

        struct WaterSurfaceEffectsState
        {
            bool refraction;
            bool reflections;
            bool caustics;
            bool liquid_animation;
            bool render_targets_available;
        };

        struct WaterEnvironmentReflectionsState
        {
            bool enabled;
        };

        enum class WaterRenderResolution
        {
            Minimum,
            Low,
            Medium,
            High,
            VeryHigh,
            Ultra
        };
        struct WaterRenderResolutionState
        {
            WaterRenderResolution level;
            int configured_exponent;
            int requested_dimension;
        };

        struct WaterfallEffectsState
        {
            bool refraction;
            bool reflections;
            bool render_targets_available;
        };

        struct GlassReflectionsState
        {
            bool enabled;
        };

        struct ModelReflectionsState
        {
            bool enabled;
        };

        struct BumpModelsState
        {
            bool enabled;
        };

        enum class EnvironmentMapResolution
        {
            Minimum,
            VeryLow,
            Low,
            Medium,
            High,
            VeryHigh,
            Ultra
        };
        struct EnvironmentMapResolutionState
        {
            EnvironmentMapResolution level;
            int configured_exponent;
            int requested_dimension;
        };

        enum class DynamicLights
        {
            Off,
            Medium,
            High,
            Custom
        };
        struct DynamicLightsState
        {
            DynamicLights setting;
            int max_lights;
        };

        struct SoftParticlesState
        {
            bool enabled;
        };

        enum class SoftParticleQuality
        {
            Low,
            Medium,
            High,
            Custom
        };
        struct SoftParticleQualityState
        {
            SoftParticleQuality quality;
            int depth_size;
            int blur;
        };

        struct DecalsState
        {
            bool enabled;
        };

        struct FixTJointsState
        {
            bool enabled;
        };

        enum class DecalPersistence
        {
            Short,
            Long,
            Custom
        };
        struct DecalPersistenceState
        {
            DecalPersistence persistence;
            int fade_start_milliseconds;
            int triangle_budget_per_type;
        };

        enum class GrassDistance
        {
            Quick,
            Moderate,
            Slow,
            Custom
        };
        struct GrassDistanceState
        {
            GrassDistance distance;
            int value;
        };

        enum class MotionBlurIntensity { Subtle, Moderate, Intense, Custom };
        struct MotionBlurIntensityState
        {
            MotionBlurIntensity intensity;
            float scale;
        };

        enum class GlarePreset { Subtle, Glowy, Soft, Intense, Custom };
        struct GlarePresetState
        {
            GlarePreset preset;
            float scale;
            int blur_radius;
        };

        struct GlareIntensityState
        {
            float configured_scale;
        };

        enum class GlareResolution
        {
            Minimum,
            Low,
            Medium,
            High,
            Ultra
        };
        struct GlareResolutionState
        {
            GlareResolution level;
            int configured_exponent;
            int requested_width;
        };

        enum class TextureQuality
        {
            Low,
            Medium,
            Full,
            Custom
        };
        struct TextureQualityState
        {
            TextureQuality quality;
            int texture_size_limit;
        };

        struct TextureReductionState
        {
            int levels;
            int minimum;
            int maximum;
        };

        enum class TextureCompressionPreset
        {
            Off,
            AllEligible,
            AtLeast256,
            AtLeast512,
            AtLeast1024,
            Above1024,
            AtLeast2048,
            AtLeast4096,
            Custom
        };
        struct TextureCompressionState
        {
            TextureCompressionPreset preset;
            int threshold;
            int minimum;
            int maximum;
            bool supported;
        };

        enum class ShaderDetailLevel { Performance = 0, Low = 1, Medium = 2, Full = 3 };
        struct ShaderDetailState
        {
            int requested;
            bool ready;
            // Resident, historically forced base selections; not a global effective level.
            int inspected, alternatives, unresolved;
        };
        bool readshaderdetail(ShaderDetailState &result);
        // Native write acceptance, independent of shader selection health.
        bool writeshaderdetail(ShaderDetailLevel level, ShaderDetailState &result);

        struct BilinearFilteringState
        {
            bool enabled;
        };
        bool readbilinearfiltering(BilinearFilteringState &result);
        bool writebilinearfiltering(bool enabled,
            BilinearFilteringState &result);

        enum class MipmapBlendingStatus { Unavailable, NoTextures, Verified, Mismatch, Mixed };
        struct MipmapBlendingState
        {
            MipmapBlendingStatus status;
            bool requested;
            int textures;
        };
        bool readmipmapblending(MipmapBlendingState &result);
        // Acceptance of the native write; verification is reported in status.
        bool writemipmapblending(bool enabled, MipmapBlendingState &result);

        enum class AnisotropyLevel { Off = 0, X2 = 2, X4 = 4, X8 = 8, X16 = 16 };
        enum class AnisotropyStatus { Unavailable, Unsupported, NoTextures, Verified, Mismatch, Mixed };
        struct AnisotropyState
        {
            AnisotropyStatus status;
            int requested;
            int maximum;
            int capped;
            int textures;
            float actual; // Only a uniform loaded value when Verified/Mismatch.
        };

        // Volume and soundpack apply immediately. soundchans/soundfreq/
        // soundbufferlen store the ident (requested) immediately and queue
        // CHANGE_SOUND; the mixer (effective) updates on resetsound().
        // Mix_QuerySpec does not report chunk size, so buffer length has
        // no mixer readback. mumble applies through its VARFP callback.
        struct Audio
        {
            int sound_volume;
            int sound_volume_minimum;
            int sound_volume_maximum;
            int music_volume;
            int music_volume_minimum;
            int music_volume_maximum;
            const char *active_soundpack;
            bool backend_active;
            int sound_channels;
            int sound_channels_minimum;
            int sound_channels_maximum;
            int effective_sound_channels;
            int sound_frequency;
            int effective_sound_frequency;
            int sound_buffer_length;
            int sound_buffer_length_minimum;
            int sound_buffer_length_maximum;
            bool mumble;
        };

        struct SoundpackChoice
        {
            const char *id;
            const char *display_name;
        };

        struct MouseInput
        {
            float sensitivity;
            float sensitivity_minimum;
            float sensitivity_maximum;
            bool inverted;
        };

        struct Crosshair
        {
            int selection;
            const char *resource;
            int size;
            int size_minimum;
            int size_maximum;
            bool effects;
            bool health_colors;
            bool teammates;
            bool hit_marker;
            bool reload_fade;
            bool reload_rotation;
        };

        struct CrosshairChoice
        {
            int id;
            const char *label;
            const char *resource;
        };

        enum class FragMessagesMode
        {
            Off,
            AllModes,
            OutsideInsta
        };

        struct Hud
        {
            bool show_fps;
            bool wall_clock;
            bool wall_clock_24_hour;
            bool wall_clock_seconds;
            bool damage_screen;
            bool damage_compass;
            bool hud_guns;
            bool hud_gun_sway;
            bool muzzle_flash;
            bool muzzle_light;
            FragMessagesMode frag_messages;
            int max_frag_messages;
            int max_frag_messages_minimum;
            int max_frag_messages_maximum;
            float frag_message_duration;
            float frag_message_duration_minimum;
            float frag_message_duration_maximum;
            bool frag_message_self_frags;
            bool frag_message_other_frags;
            bool frag_message_team_kills;
            float frag_message_scale;
            float frag_message_scale_minimum;
            float frag_message_scale_maximum;
            bool frag_message_fade;
            float frag_message_x;
            float frag_message_x_minimum;
            float frag_message_x_maximum;
            float frag_message_y;
            float frag_message_y_minimum;
            float frag_message_y_maximum;
            int frag_message_stack_dir;
            float streak_milestone_scale;
            float streak_milestone_scale_minimum;
            float streak_milestone_scale_maximum;
        };

        struct Ammobar
        {
            bool visible;
            bool horizontal;
            bool flip;
            float scale;
            float scale_minimum;
            float scale_maximum;
            int icon_spacing;
            int icon_spacing_minimum;
            int icon_spacing_maximum;
            float label_size;
            float label_size_minimum;
            float label_size_maximum;
            int label_offset;
            int label_offset_minimum;
            int label_offset_maximum;
            int align;
            float x;
            float x_minimum;
            float x_maximum;
            float y;
            float y_minimum;
            float y_maximum;
            bool hide_empty;
        };

        // Match overlay clock. Align is the native ident; radios select
        // only on exact -1/0/1. Colour idents stay unexposed.
        struct GameClock
        {
            bool enabled;
            float scale;
            float scale_minimum;
            float scale_maximum;
            int alpha;
            int alpha_minimum;
            int alpha_maximum;
            int align;
            float x;
            float x_minimum;
            float x_maximum;
            float y;
            float y_minimum;
            float y_maximum;
        };

        // Native Settings HUD tab master plus the hudscore adjust document.
        // Colour HVARPs are not native Settings GUI controls.
        struct HudScore
        {
            bool enabled;
            float scale;
            float scale_minimum;
            float scale_maximum;
            int alpha;
            int alpha_minimum;
            int alpha_maximum;
            int spacing;
            int spacing_minimum;
            int spacing_maximum;
            int align;
            float x;
            float x_minimum;
            float x_maximum;
            float y;
            float y_minimum;
            float y_maximum;
        };

        // Native scoreboard column checkboxes. hidefrags is inverted:
        // UI "frags" on means hidefrags==0. showping and showdamage may
        // hold non-canonical values; opening Settings must not coerce them.
        struct ScoreboardColumns
        {
            bool show_flags;
            bool show_frags;
            bool show_deaths;
            bool show_suicides;
            bool show_kpd;
            bool show_accuracy;
            bool show_damage;
            int showdamage;
            bool show_damage_dealt;
            bool show_damage_net;
            bool show_damage_received;
            bool show_ping;
            int showping;
            bool show_pj;
            bool show_clientnum;
        };

        struct Chat
        {
            enum
            {
                AutoIgnoreSlotCount = 20,
                AutoIgnoreNameSize = 16
            };

            bool public_messages_in_top_feed;
            bool team_messages_in_top_feed;
            bool public_messages_in_bottom_box;
            bool team_messages_in_bottom_box;
            bool team_colored_names;
            bool auto_ignore_enabled;
            int auto_ignore_slot_count;
            char auto_ignore_names[AutoIgnoreSlotCount][AutoIgnoreNameSize];
        };

        struct AutoexecChoice
        {
            const char *id;
            const char *display_name;
            bool selected;
        };

        typedef void (*AutoexecChoiceVisitor)(
            const AutoexecChoice &choice,
            void *data
        );

        struct DisplayPresetChoice
        {
            const char *id;
            const char *display_name;
        };

        typedef void (*DisplayPresetChoiceVisitor)(
            const DisplayPresetChoice &choice,
            void *data
        );

        // Native Settings console tab minus Chat-owned bits.
        // Top-left `confilter` omits CON_CHAT / CON_TEAMCHAT; Chat owns
        // those two bits plus `miniconfilter` and `teamcolorchat`.
        // `fullconfilter` includes every native checkbox bit.
        struct Console
        {
            bool important_information_in_top_feed;
            bool warnings_in_top_feed;
            bool errors_in_top_feed;
            bool script_messages_in_top_feed;
            bool game_events_in_top_feed;
            bool self_frags_in_top_feed;
            bool other_player_frags_in_top_feed;
            bool team_kills_in_top_feed;
            bool important_information_in_full_console;
            bool warnings_in_full_console;
            bool errors_in_full_console;
            bool script_messages_in_full_console;
            bool public_messages_in_full_console;
            bool team_messages_in_full_console;
            bool game_events_in_full_console;
            bool self_frags_in_full_console;
            bool other_player_frags_in_full_console;
            bool team_kills_in_full_console;
            int size;
            int size_minimum;
            int size_maximum;
        };

        struct Resolution
        {
            int width;
            int height;
        };

        typedef void (*ResolutionVisitor)(
            const Resolution &resolution,
            void *data
        );

        typedef void (*CrosshairChoiceVisitor)(
            const CrosshairChoice &choice,
            void *data
        );

        typedef void (*SoundpackChoiceVisitor)(
            const SoundpackChoice &choice,
            void *data
        );

        bool readfov(Fov &result);
        bool writefov(int value);

        bool readgamefeedback(GameFeedback &result);
        bool writeblood(bool enabled, GameFeedback &result);
        bool writehitsound(bool enabled, GameFeedback &result);
        bool writeoutlinemeters(bool enabled, GameFeedback &result);
        bool writedeathscore(bool enabled, GameFeedback &result);

        bool readtrailcolors(TrailColors &result);
        bool writetrailcolorshotgun(int packed, TrailColors &result);
        bool writetrailcolorchaingun(int packed, TrailColors &result);
        bool writetrailcolorrocketlauncher(int packed, TrailColors &result);
        bool writetrailcolorrifle(int packed, TrailColors &result);
        bool writetrailcolorgrenadelauncher(int packed, TrailColors &result);
        bool writetrailcolorpistol(int packed, TrailColors &result);

        bool readgameradar(GameRadar &result);
        bool writebasenumbers(bool enabled, GameRadar &result);
        bool writeradarteammates(bool enabled, GameRadar &result);

        bool readgamemodels(GameModels &result);
        bool writeragdoll(bool enabled, GameModels &result);
        bool writeragdollmillis(bool enabled, GameModels &result);
        bool writedeadpush(int value, GameModels &result);
        bool writehidedead(bool enabled, GameModels &result);
        bool writefullbrightmodels(bool enabled, GameModels &result);
        bool writefullbrightintensity(int value, GameModels &result);
        bool writeforceplayermodels(bool enabled, GameModels &result);
        bool writeteamskins(bool enabled, GameModels &result);

        bool readvideo(Video &result);
        bool writepresentation(PresentationMode mode);
        bool writeresolution(int width, int height);
        bool visitresolutions(ResolutionVisitor visitor, void *data);

        bool readvsync(Vsync &result);
        bool writevsync(bool enabled);

        bool readfpslimit(FpsLimit &result);
        bool writefpslimit(int value);

        bool readgamma(Gamma &result);
        bool writegamma(int value, Gamma &result);

        bool readantialiasing(Antialiasing &result);
        bool writeantialiasing(int samples);
        bool readdepthbits(DepthBuffer &result);
        bool writedepthbits(int bits);
        void resetgraphics();

        bool readgraphicsquality(GraphicsQuality &result);
        bool writegrass(bool enabled);
        bool writemotionblur(bool enabled);
        bool writeglare(bool enabled);

        bool readshadows(ShadowState &result);
        bool writeshadowmaps(bool enabled, ShadowState &result);
        bool writeblobshadows(bool enabled, ShadowState &result);

        bool readshadowquality(ShadowQualityState &result);
        // Custom is output-only and is rejected as a write request.
        bool writeshadowquality(ShadowQuality quality,
            ShadowQualityState &result);

        bool readwatersurfaceeffects(WaterSurfaceEffectsState &result);
        bool writewaterrefraction(bool enabled,
            WaterSurfaceEffectsState &result);
        bool writewaterreflections(bool enabled,
            WaterSurfaceEffectsState &result);
        bool writecaustics(bool enabled, WaterSurfaceEffectsState &result);
        bool writeliquidanimation(bool enabled,
            WaterSurfaceEffectsState &result);

        bool readwaterenvironmentreflections(
            WaterEnvironmentReflectionsState &result);
        bool writewaterenvironmentreflections(bool enabled,
            WaterEnvironmentReflectionsState &result);

        bool readwaterrenderresolution(
            WaterRenderResolutionState &result);
        bool writewaterrenderresolution(WaterRenderResolution level,
            WaterRenderResolutionState &result);

        bool readwaterfalleffects(WaterfallEffectsState &result);
        bool writewaterfallrefraction(bool enabled,
            WaterfallEffectsState &result);
        bool writewaterfallreflections(bool enabled,
            WaterfallEffectsState &result);

        bool readglassreflections(GlassReflectionsState &result);
        bool writeglassreflections(bool enabled,
            GlassReflectionsState &result);

        bool readmodelreflections(ModelReflectionsState &result);
        bool writemodelreflections(bool enabled,
            ModelReflectionsState &result);

        bool readbumpmodels(BumpModelsState &result);
        bool writebumpmodels(bool enabled, BumpModelsState &result);

        bool readenvironmentmapresolution(
            EnvironmentMapResolutionState &result);
        bool writeenvironmentmapresolution(EnvironmentMapResolution level,
            EnvironmentMapResolutionState &result);

        bool readdynamiclights(DynamicLightsState &result);
        // Custom is output-only and is rejected as a write request.
        bool writedynamiclights(DynamicLights setting,
            DynamicLightsState &result);

        bool readsoftparticles(SoftParticlesState &result);
        bool writesoftparticles(bool enabled, SoftParticlesState &result);

        bool readsoftparticlequality(SoftParticleQualityState &result);
        // Custom is output-only and is rejected as a write request.
        bool writesoftparticlequality(SoftParticleQuality quality,
            SoftParticleQualityState &result);

        bool readdecals(DecalsState &result);
        bool writedecals(bool enabled, DecalsState &result);

        bool readfixtjoints(FixTJointsState &result);
        bool writefixtjoints(bool enabled, FixTJointsState &result);

        bool readdecalpersistence(DecalPersistenceState &result);
        // Custom is output-only and is rejected as a write request.
        bool writedecalpersistence(DecalPersistence persistence,
            DecalPersistenceState &result);

        bool readgrassdistance(GrassDistanceState &result);
        // Custom is output-only and is rejected as a write request.
        bool writegrassdistance(GrassDistance distance,
            GrassDistanceState &result);

        bool readmotionblurintensity(MotionBlurIntensityState &result);
        // Custom is output-only and is rejected as a write request.
        bool writemotionblurintensity(MotionBlurIntensity intensity,
            MotionBlurIntensityState &result);

        bool readglarepreset(GlarePresetState &result);
        bool writeglarepreset(GlarePreset preset, GlarePresetState &result);

        bool readglareintensity(GlareIntensityState &result);
        bool writeglareintensity(float configured_scale,
            GlareIntensityState &result);

        bool readglareresolution(GlareResolutionState &result);
        bool writeglareresolution(GlareResolution level,
            GlareResolutionState &result);

        bool readtexturequality(TextureQualityState &result);
        // Custom is output-only and is rejected as a write request.
        bool writetexturequality(TextureQuality quality,
            TextureQualityState &result);

        bool readtexturereduction(TextureReductionState &result);
        bool writetexturereduction(int levels,
            TextureReductionState &result);

        bool readtexturecompression(TextureCompressionState &result);
        bool writetexturecompression(int threshold,
            TextureCompressionState &result);

        bool readanisotropy(AnisotropyState &result);
        // Returns the callback's verification result without a second scan.
        bool writeanisotropy(AnisotropyLevel level, AnisotropyState &result);

        bool readaudio(Audio &result);
        bool visitsoundpacks(SoundpackChoiceVisitor visitor, void *data);
        bool writesoundvolume(int value);
        bool writemusicvolume(int value);
        bool writesoundpack(const char *id);
        bool writesoundchannels(int value, Audio &result);
        // Native Settings list is 11025/22050/44100 only. Other ident
        // values are shown on read and are not coerced.
        bool writesoundfrequency(int value, Audio &result);
        bool writesoundbufferlength(int value, Audio &result);
        bool writemumble(bool enabled, Audio &result);
        bool hassoundchanges();
        void resetsound();

        bool readmouseinput(MouseInput &result);
        bool writesensitivity(float value);
        bool writeinvertedmouse(bool enabled);

        bool readcrosshair(Crosshair &result);
        bool visitcrosshairs(CrosshairChoiceVisitor visitor, void *data);
        bool writecrosshairselection(int selection);
        bool writecrosshairsize(int size);
        bool writecrosshaireffects(bool enabled);
        bool writecrosshairhealthcolors(bool enabled);
        bool writeteamcrosshair(bool enabled);
        bool writehitcrosshair(bool enabled);
        bool writecrosshairreloadfade(bool enabled);
        bool writecrosshairreloadrotation(bool enabled);

        bool readhud(Hud &result);
        bool writeshowfps(bool enabled);
        bool writewallclock(bool enabled);
        bool writewallclock24hour(bool enabled);
        bool writewallclockseconds(bool enabled);
        bool writedamagescreen(bool enabled);
        bool writedamagecompass(bool enabled);
        bool writehudguns(bool enabled);
        bool writehudgunsway(bool enabled);
        bool writemuzzleflash(bool enabled);
        bool writemuzzlelight(bool enabled);
        bool writefragmessages(FragMessagesMode mode);
        bool writemaxfragmessages(int value, Hud &result);
        bool writefragmessageduration(float value, Hud &result);
        bool writefragmessageselffrags(bool enabled, Hud &result);
        bool writefragmessageotherfrags(bool enabled, Hud &result);
        bool writefragmessageteamkills(bool enabled, Hud &result);
        bool writefragmessagescale(float value, Hud &result);
        bool writefragmessagefade(bool enabled, Hud &result);
        bool writefragmessagex(float value, Hud &result);
        bool writefragmessagey(float value, Hud &result);
        bool writefragmessagestackdir(int value, Hud &result);
        bool writestreakmilestonescale(float value);

        bool readammobar(Ammobar &result);
        bool writeammobar(bool enabled, Ammobar &result);
        bool writeammobarhorizontal(bool enabled, Ammobar &result);
        bool writeammobarflip(bool enabled, Ammobar &result);
        bool writeammobarscale(float value, Ammobar &result);
        bool writeammobarsep(int value, Ammobar &result);
        bool writeammobarcountscale(float value, Ammobar &result);
        bool writeammobarcountsep(int value, Ammobar &result);
        bool writeammobaralign(int value, Ammobar &result);
        bool writeammobarx(float value, Ammobar &result);
        bool writeammobary(float value, Ammobar &result);
        bool writeammobarhideempty(bool enabled, Ammobar &result);

        bool readgameclock(GameClock &result);
        bool writegameclock(bool enabled, GameClock &result);
        bool writegameclockscale(float value, GameClock &result);
        bool writegameclockalpha(int value, GameClock &result);
        bool writegameclockalign(int value, GameClock &result);
        bool writegameclockx(float value, GameClock &result);
        bool writegameclocky(float value, GameClock &result);

        bool readhudscore(HudScore &result);
        bool writehudscore(bool enabled, HudScore &result);
        bool writehudscorescale(float value, HudScore &result);
        bool writehudscorealpha(int value, HudScore &result);
        bool writehudscoresep(int value, HudScore &result);
        bool writehudscorealign(int value, HudScore &result);
        bool writehudscorex(float value, HudScore &result);
        bool writehudscorey(float value, HudScore &result);

        bool readscoreboardcolumns(ScoreboardColumns &result);
        bool writeshowflags(bool enabled, ScoreboardColumns &result);
        // UI bool is "show frags column": true writes hidefrags 0, false writes 1.
        bool writehidefrags(bool show_column, ScoreboardColumns &result);
        bool writeshowdeaths(bool enabled, ScoreboardColumns &result);
        bool writeshowsuicides(bool enabled, ScoreboardColumns &result);
        bool writeshowkpd(bool enabled, ScoreboardColumns &result);
        bool writeshowaccuracy(bool enabled, ScoreboardColumns &result);
        bool writeshowdamage(bool enabled, ScoreboardColumns &result);
        bool writeshowdamagemode(int value, ScoreboardColumns &result);
        bool writeshowdamagereceived(bool enabled, ScoreboardColumns &result);
        bool writeshowping(bool enabled, ScoreboardColumns &result);
        bool writeshowpj(bool enabled, ScoreboardColumns &result);
        bool writeshowclientnum(bool enabled, ScoreboardColumns &result);

        bool readchat(Chat &result);
        bool writepublicchatintopfeed(bool enabled);
        bool writeteamchatintopfeed(bool enabled);
        bool writepublicchatinbottombox(bool enabled);
        bool writeteamchatinbottombox(bool enabled);
        bool writeteamcoloredchatnames(bool enabled);
        bool writeautoignoreenabled(bool enabled, Chat &result);
        bool writeautoignorename(int slot, const char *name, Chat &result);
        bool writeautoignoreslotcount(int count, Chat &result);

        bool visitautoexecprofiles(AutoexecChoiceVisitor visitor, void *data);
        bool writeautoexecprofile(const char *id);
        bool runautoexecprofile(const char *id);
        bool visitdisplaypresets(DisplayPresetChoiceVisitor visitor,
            void *data);
        bool rundisplaypreset(const char *id);

        bool readtheme(Theme &result);
        bool writetheme(Theme theme, Theme &result);
        bool readinterfaceappearance(InterfaceAppearance &result);
        bool writeuiscale(int value, InterfaceAppearance &result);
        bool writeuitextscale(int value, InterfaceAppearance &result);
        bool writeinterfacefont(InterfaceFont font,
            InterfaceAppearance &result);
        bool resetinterfaceappearance(InterfaceAppearance &result);

        bool readconsole(Console &result);
        bool writeimportantintopfeed(bool enabled, Console &result);
        bool writewarningsintopfeed(bool enabled, Console &result);
        bool writeerrorsintopfeed(bool enabled, Console &result);
        bool writescriptmessagesintopfeed(bool enabled, Console &result);
        bool writegameeventsintopfeed(bool enabled, Console &result);
        bool writeselffragsintopfeed(bool enabled, Console &result);
        bool writeotherplayerfragsintopfeed(bool enabled, Console &result);
        bool writeteamkillsintopfeed(bool enabled, Console &result);
        bool writeimportantinfullconsole(bool enabled, Console &result);
        bool writewarningsinfullconsole(bool enabled, Console &result);
        bool writeerrorsinfullconsole(bool enabled, Console &result);
        bool writescriptmessagesinfullconsole(bool enabled, Console &result);
        bool writepublicchatinfullconsole(bool enabled, Console &result);
        bool writeteamchatinfullconsole(bool enabled, Console &result);
        bool writegameeventsinfullconsole(bool enabled, Console &result);
        bool writeselffragsinfullconsole(bool enabled, Console &result);
        bool writeotherplayerfragsinfullconsole(bool enabled, Console &result);
        bool writeteamkillsinfullconsole(bool enabled, Console &result);
        bool writeconsolesize(int value, Console &result);
    }
}

#endif
