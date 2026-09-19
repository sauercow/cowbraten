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
            FragMessagesMode frag_messages;
            float streak_milestone_scale;
            float streak_milestone_scale_minimum;
            float streak_milestone_scale_maximum;
        };

        struct Chat
        {
            bool public_messages_in_top_feed;
            bool team_messages_in_top_feed;
            bool public_messages_in_bottom_box;
            bool team_messages_in_bottom_box;
            bool team_colored_names;
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

        bool readvideo(Video &result);
        bool writepresentation(PresentationMode mode);
        bool writeresolution(int width, int height);
        bool visitresolutions(ResolutionVisitor visitor, void *data);

        bool readvsync(Vsync &result);
        bool writevsync(bool enabled);

        bool readfpslimit(FpsLimit &result);
        bool writefpslimit(int value);

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
        bool writefragmessages(FragMessagesMode mode);
        bool writestreakmilestonescale(float value);

        bool readchat(Chat &result);
        bool writepublicchatintopfeed(bool enabled);
        bool writeteamchatintopfeed(bool enabled);
        bool writepublicchatinbottombox(bool enabled);
        bool writeteamchatinbottombox(bool enabled);
        bool writeteamcoloredchatnames(bool enabled);
    }
}

#endif
