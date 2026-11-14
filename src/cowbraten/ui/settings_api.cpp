#include "settings_api.h"

#include "sauer_settings_bridge.h"

#include <cstring>

namespace cowui
{
    namespace settings
    {
        static PresentationMode presentationmode(
            sauersettings::PresentationMode mode)
        {
            switch(mode)
            {
                case sauersettings::PresentationMode::Windowed:
                    return PresentationMode::Windowed;
                case sauersettings::PresentationMode::ExclusiveFullscreen:
                    return PresentationMode::ExclusiveFullscreen;
                case sauersettings::PresentationMode::DesktopFullscreen:
                    return PresentationMode::DesktopFullscreen;
            }
            return PresentationMode::Windowed;
        }

        static sauersettings::PresentationMode presentationmode(
            PresentationMode mode)
        {
            switch(mode)
            {
                case PresentationMode::Windowed:
                    return sauersettings::PresentationMode::Windowed;
                case PresentationMode::ExclusiveFullscreen:
                    return sauersettings::PresentationMode::ExclusiveFullscreen;
                case PresentationMode::DesktopFullscreen:
                    return sauersettings::PresentationMode::DesktopFullscreen;
            }
            return sauersettings::PresentationMode::Windowed;
        }

        static Theme theme(sauersettings::Theme value)
        {
            switch(value)
            {
                case sauersettings::Theme::Default:
                    return Theme::Default;
                case sauersettings::Theme::ModernMetallic:
                    return Theme::ModernMetallic;
            }
            return Theme::Default;
        }

        static sauersettings::Theme theme(Theme value)
        {
            switch(value)
            {
                case Theme::Default:
                    return sauersettings::Theme::Default;
                case Theme::ModernMetallic:
                    return sauersettings::Theme::ModernMetallic;
            }
            return sauersettings::Theme::Default;
        }

        static FragMessagesMode fragmessagesmode(
            sauersettings::FragMessagesMode mode)
        {
            switch(mode)
            {
                case sauersettings::FragMessagesMode::Off:
                    return FragMessagesMode::Off;
                case sauersettings::FragMessagesMode::AllModes:
                    return FragMessagesMode::AllModes;
                case sauersettings::FragMessagesMode::OutsideInsta:
                    return FragMessagesMode::OutsideInsta;
            }
            return FragMessagesMode::Off;
        }

        static sauersettings::FragMessagesMode fragmessagesmode(
            FragMessagesMode mode)
        {
            switch(mode)
            {
                case FragMessagesMode::Off:
                    return sauersettings::FragMessagesMode::Off;
                case FragMessagesMode::AllModes:
                    return sauersettings::FragMessagesMode::AllModes;
                case FragMessagesMode::OutsideInsta:
                    return sauersettings::FragMessagesMode::OutsideInsta;
            }
            return sauersettings::FragMessagesMode::Off;
        }

        bool readfov(Fov &result)
        {
            sauersettings::Fov legacy;
            if(!sauersettings::readfov(legacy))
                return false;

            result.value = legacy.value;
            result.minimum = legacy.minimum;
            result.maximum = legacy.maximum;
            return true;
        }

        bool writefov(int value)
        {
            return sauersettings::writefov(value);
        }

        static void copygamefeedback(
            const sauersettings::GameFeedback &source,
            GameFeedback &result)
        {
            result.blood = source.blood;
            result.hit_sound = source.hit_sound;
            result.outline_capture_meters = source.outline_capture_meters;
            result.show_scoreboard_at_death = source.show_scoreboard_at_death;
        }

        bool readgamefeedback(GameFeedback &result)
        {
            sauersettings::GameFeedback legacy = {};
            const bool read = sauersettings::readgamefeedback(legacy);
            copygamefeedback(legacy, result);
            return read;
        }

        bool writeblood(bool enabled, GameFeedback &result)
        {
            sauersettings::GameFeedback legacy = {};
            const bool accepted = sauersettings::writeblood(enabled, legacy);
            copygamefeedback(legacy, result);
            return accepted;
        }

        bool writehitsound(bool enabled, GameFeedback &result)
        {
            sauersettings::GameFeedback legacy = {};
            const bool accepted = sauersettings::writehitsound(enabled, legacy);
            copygamefeedback(legacy, result);
            return accepted;
        }

        bool writeoutlinemeters(bool enabled, GameFeedback &result)
        {
            sauersettings::GameFeedback legacy = {};
            const bool accepted = sauersettings::writeoutlinemeters(
                enabled, legacy);
            copygamefeedback(legacy, result);
            return accepted;
        }

        bool writedeathscore(bool enabled, GameFeedback &result)
        {
            sauersettings::GameFeedback legacy = {};
            const bool accepted = sauersettings::writedeathscore(
                enabled, legacy);
            copygamefeedback(legacy, result);
            return accepted;
        }

        static void copytrailcolors(
            const sauersettings::TrailColors &source,
            TrailColors &result)
        {
            result.shotgun = source.shotgun;
            result.chaingun = source.chaingun;
            result.rocketlauncher = source.rocketlauncher;
            result.rifle = source.rifle;
            result.grenadelauncher = source.grenadelauncher;
            result.pistol = source.pistol;
            result.channel_minimum = source.channel_minimum;
            result.channel_maximum = source.channel_maximum;
        }

        bool readtrailcolors(TrailColors &result)
        {
            sauersettings::TrailColors legacy = {};
            const bool read = sauersettings::readtrailcolors(legacy);
            copytrailcolors(legacy, result);
            return read;
        }

        bool writetrailcolorshotgun(int packed, TrailColors &result)
        {
            sauersettings::TrailColors legacy = {};
            const bool accepted = sauersettings::writetrailcolorshotgun(
                packed, legacy);
            copytrailcolors(legacy, result);
            return accepted;
        }

        bool writetrailcolorchaingun(int packed, TrailColors &result)
        {
            sauersettings::TrailColors legacy = {};
            const bool accepted = sauersettings::writetrailcolorchaingun(
                packed, legacy);
            copytrailcolors(legacy, result);
            return accepted;
        }

        bool writetrailcolorrocketlauncher(int packed, TrailColors &result)
        {
            sauersettings::TrailColors legacy = {};
            const bool accepted =
                sauersettings::writetrailcolorrocketlauncher(
                    packed, legacy);
            copytrailcolors(legacy, result);
            return accepted;
        }

        bool writetrailcolorrifle(int packed, TrailColors &result)
        {
            sauersettings::TrailColors legacy = {};
            const bool accepted = sauersettings::writetrailcolorrifle(
                packed, legacy);
            copytrailcolors(legacy, result);
            return accepted;
        }

        bool writetrailcolorgrenadelauncher(int packed, TrailColors &result)
        {
            sauersettings::TrailColors legacy = {};
            const bool accepted =
                sauersettings::writetrailcolorgrenadelauncher(
                    packed, legacy);
            copytrailcolors(legacy, result);
            return accepted;
        }

        bool writetrailcolorpistol(int packed, TrailColors &result)
        {
            sauersettings::TrailColors legacy = {};
            const bool accepted = sauersettings::writetrailcolorpistol(
                packed, legacy);
            copytrailcolors(legacy, result);
            return accepted;
        }

        static void copygameradar(
            const sauersettings::GameRadar &source,
            GameRadar &result)
        {
            result.numbered_capture_bases = source.numbered_capture_bases;
            result.show_teammates_on_radar = source.show_teammates_on_radar;
        }

        bool readgameradar(GameRadar &result)
        {
            sauersettings::GameRadar legacy = {};
            const bool read = sauersettings::readgameradar(legacy);
            copygameradar(legacy, result);
            return read;
        }

        bool writebasenumbers(bool enabled, GameRadar &result)
        {
            sauersettings::GameRadar legacy = {};
            const bool accepted = sauersettings::writebasenumbers(
                enabled, legacy);
            copygameradar(legacy, result);
            return accepted;
        }

        bool writeradarteammates(bool enabled, GameRadar &result)
        {
            sauersettings::GameRadar legacy = {};
            const bool accepted = sauersettings::writeradarteammates(
                enabled, legacy);
            copygameradar(legacy, result);
            return accepted;
        }

        static void copygamemodels(
            const sauersettings::GameModels &source,
            GameModels &result)
        {
            result.ragdoll_deaths = source.ragdoll_deaths;
            result.keep_ragdoll_after_respawn =
                source.keep_ragdoll_after_respawn;
            result.ragdoll_millis = source.ragdoll_millis;
            result.deadpush = source.deadpush;
            result.deadpush_minimum = source.deadpush_minimum;
            result.deadpush_maximum = source.deadpush_maximum;
            result.hide_dead_players = source.hide_dead_players;
            result.hidedead = source.hidedead;
            result.fullbright_player_models =
                source.fullbright_player_models;
            result.fullbrightmodels = source.fullbrightmodels;
            result.force_matching_player_models =
                source.force_matching_player_models;
            result.always_use_team_skins = source.always_use_team_skins;
        }

        bool readgamemodels(GameModels &result)
        {
            sauersettings::GameModels legacy = {};
            const bool read = sauersettings::readgamemodels(legacy);
            copygamemodels(legacy, result);
            return read;
        }

        bool writeragdoll(bool enabled, GameModels &result)
        {
            sauersettings::GameModels legacy = {};
            const bool accepted = sauersettings::writeragdoll(
                enabled, legacy);
            copygamemodels(legacy, result);
            return accepted;
        }

        bool writeragdollmillis(bool enabled, GameModels &result)
        {
            sauersettings::GameModels legacy = {};
            const bool accepted = sauersettings::writeragdollmillis(
                enabled, legacy);
            copygamemodels(legacy, result);
            return accepted;
        }

        bool writedeadpush(int value, GameModels &result)
        {
            sauersettings::GameModels legacy = {};
            const bool accepted = sauersettings::writedeadpush(
                value, legacy);
            copygamemodels(legacy, result);
            return accepted;
        }

        bool writehidedead(bool enabled, GameModels &result)
        {
            sauersettings::GameModels legacy = {};
            const bool accepted = sauersettings::writehidedead(
                enabled, legacy);
            copygamemodels(legacy, result);
            return accepted;
        }

        bool writefullbrightmodels(bool enabled, GameModels &result)
        {
            sauersettings::GameModels legacy = {};
            const bool accepted = sauersettings::writefullbrightmodels(
                enabled, legacy);
            copygamemodels(legacy, result);
            return accepted;
        }

        bool writefullbrightintensity(int value, GameModels &result)
        {
            sauersettings::GameModels legacy = {};
            const bool accepted = sauersettings::writefullbrightintensity(
                value, legacy);
            copygamemodels(legacy, result);
            return accepted;
        }

        bool writeforceplayermodels(bool enabled, GameModels &result)
        {
            sauersettings::GameModels legacy = {};
            const bool accepted = sauersettings::writeforceplayermodels(
                enabled, legacy);
            copygamemodels(legacy, result);
            return accepted;
        }

        bool writeteamskins(bool enabled, GameModels &result)
        {
            sauersettings::GameModels legacy = {};
            const bool accepted = sauersettings::writeteamskins(
                enabled, legacy);
            copygamemodels(legacy, result);
            return accepted;
        }

        bool readvideo(Video &result)
        {
            sauersettings::Video legacy;
            if(!sauersettings::readvideo(legacy))
                return false;

            result.width = legacy.width;
            result.height = legacy.height;
            result.drawable_width = legacy.drawable_width;
            result.drawable_height = legacy.drawable_height;
            result.presentation = presentationmode(legacy.presentation);
            return true;
        }

        bool writepresentation(PresentationMode mode)
        {
            return sauersettings::writepresentation(
                presentationmode(mode)
            );
        }

        bool writeresolution(int width, int height)
        {
            return sauersettings::writeresolution(width, height);
        }

        struct ResolutionAdapter
        {
            ResolutionVisitor visitor;
            void *data;
        };

        static void visitresolution(
            const sauersettings::Resolution &legacy,
            void *data)
        {
            ResolutionAdapter &adapter =
                *static_cast<ResolutionAdapter *>(data);
            Resolution resolution;
            resolution.width = legacy.width;
            resolution.height = legacy.height;
            adapter.visitor(resolution, adapter.data);
        }

        bool visitresolutions(ResolutionVisitor visitor, void *data)
        {
            if(!visitor)
                return false;

            ResolutionAdapter adapter;
            adapter.visitor = visitor;
            adapter.data = data;
            return sauersettings::visitresolutions(
                visitresolution,
                &adapter
            );
        }

        bool readvsync(Vsync &result)
        {
            sauersettings::Vsync legacy;
            if(!sauersettings::readvsync(legacy))
                return false;

            result.enabled = legacy.enabled;
            result.adaptive = legacy.adaptive;
            return true;
        }

        bool writevsync(bool enabled)
        {
            return sauersettings::writevsync(enabled);
        }

        bool readfpslimit(FpsLimit &result)
        {
            sauersettings::FpsLimit legacy;
            if(!sauersettings::readfpslimit(legacy))
                return false;

            result.value = legacy.value;
            result.minimum = legacy.minimum;
            result.maximum = legacy.maximum;
            return true;
        }

        bool writefpslimit(int value)
        {
            return sauersettings::writefpslimit(value);
        }

        static void copygamma(const sauersettings::Gamma &source, Gamma &result)
        {
            result.requested = source.requested;
            result.effective = source.effective;
            result.minimum = source.minimum;
            result.maximum = source.maximum;
        }

        bool readgamma(Gamma &result)
        {
            sauersettings::Gamma legacy = {};
            const bool read = sauersettings::readgamma(legacy);
            copygamma(legacy, result);
            return read;
        }

        bool writegamma(int value, Gamma &result)
        {
            sauersettings::Gamma legacy = {};
            const bool accepted = sauersettings::writegamma(value, legacy);
            copygamma(legacy, result);
            return accepted;
        }

        bool readantialiasing(Antialiasing &result)
        {
            sauersettings::Antialiasing legacy;
            if(!sauersettings::readantialiasing(legacy))
                return false;

            result.requested_samples = legacy.requested_samples;
            result.actual_samples = legacy.actual_samples;
            return true;
        }

        bool writeantialiasing(int samples)
        {
            return sauersettings::writeantialiasing(samples);
        }

        bool readdepthbits(DepthBuffer &result)
        {
            sauersettings::DepthBuffer legacy;
            if(!sauersettings::readdepthbits(legacy))
                return false;

            result.requested_bits = legacy.requested_bits;
            result.actual_bits = legacy.actual_bits;
            return true;
        }

        bool writedepthbits(int bits)
        {
            return sauersettings::writedepthbits(bits);
        }

        void resetgraphics()
        {
            sauersettings::resetgraphics();
        }

        bool readgraphicsquality(GraphicsQuality &result)
        {
            sauersettings::GraphicsQuality legacy;
            if(!sauersettings::readgraphicsquality(legacy))
                return false;

            result.grass = legacy.grass;
            result.motion_blur = legacy.motion_blur;
            result.motion_blur_available = legacy.motion_blur_available;
            result.glare = legacy.glare;
            return true;
        }

        bool writegrass(bool enabled)
        {
            return sauersettings::writegrass(enabled);
        }

        bool writemotionblur(bool enabled)
        {
            return sauersettings::writemotionblur(enabled);
        }

        bool writeglare(bool enabled)
        {
            return sauersettings::writeglare(enabled);
        }

        static void copyshadows(
            const sauersettings::ShadowState &source,
            ShadowState &result)
        {
            result.shadow_maps = source.shadow_maps;
            result.blob_shadows = source.blob_shadows;
            result.shadow_maps_available = source.shadow_maps_available;
        }

        bool readshadows(ShadowState &result)
        {
            sauersettings::ShadowState legacy = {};
            const bool read = sauersettings::readshadows(legacy);
            copyshadows(legacy, result);
            return read;
        }

        bool writeshadowmaps(bool enabled, ShadowState &result)
        {
            sauersettings::ShadowState legacy = {};
            const bool accepted = sauersettings::writeshadowmaps(
                enabled, legacy);
            copyshadows(legacy, result);
            return accepted;
        }

        bool writeblobshadows(bool enabled, ShadowState &result)
        {
            sauersettings::ShadowState legacy = {};
            const bool accepted = sauersettings::writeblobshadows(
                enabled, legacy);
            copyshadows(legacy, result);
            return accepted;
        }

        static ShadowQuality shadowquality(
            sauersettings::ShadowQuality quality)
        {
            switch(quality)
            {
                case sauersettings::ShadowQuality::Medium:
                    return ShadowQuality::Medium;
                case sauersettings::ShadowQuality::High:
                    return ShadowQuality::High;
                case sauersettings::ShadowQuality::Custom:
                    return ShadowQuality::Custom;
            }
            return ShadowQuality::Custom;
        }

        static sauersettings::ShadowQuality shadowquality(
            ShadowQuality quality)
        {
            switch(quality)
            {
                case ShadowQuality::Medium:
                    return sauersettings::ShadowQuality::Medium;
                case ShadowQuality::High:
                    return sauersettings::ShadowQuality::High;
                case ShadowQuality::Custom:
                    return sauersettings::ShadowQuality::Custom;
            }
            return sauersettings::ShadowQuality::Custom;
        }

        static void copyshadowquality(
            const sauersettings::ShadowQualityState &source,
            ShadowQualityState &result)
        {
            result.quality = shadowquality(source.quality);
            result.map_size = source.map_size;
            result.blur = source.blur;
        }

        bool readshadowquality(ShadowQualityState &result)
        {
            sauersettings::ShadowQualityState legacy = {};
            const bool read = sauersettings::readshadowquality(legacy);
            copyshadowquality(legacy, result);
            return read;
        }

        bool writeshadowquality(
            ShadowQuality quality,
            ShadowQualityState &result)
        {
            sauersettings::ShadowQualityState legacy = {};
            const bool accepted = sauersettings::writeshadowquality(
                shadowquality(quality), legacy);
            copyshadowquality(legacy, result);
            return accepted;
        }

        static void copywatersurfaceeffects(
            const sauersettings::WaterSurfaceEffectsState &source,
            WaterSurfaceEffectsState &result)
        {
            result.refraction = source.refraction;
            result.reflections = source.reflections;
            result.caustics = source.caustics;
            result.liquid_animation = source.liquid_animation;
            result.render_targets_available =
                source.render_targets_available;
        }

        bool readwatersurfaceeffects(WaterSurfaceEffectsState &result)
        {
            sauersettings::WaterSurfaceEffectsState legacy = {};
            const bool read = sauersettings::readwatersurfaceeffects(legacy);
            copywatersurfaceeffects(legacy, result);
            return read;
        }

        bool writewaterrefraction(
            bool enabled,
            WaterSurfaceEffectsState &result)
        {
            sauersettings::WaterSurfaceEffectsState legacy = {};
            const bool accepted = sauersettings::writewaterrefraction(
                enabled, legacy);
            copywatersurfaceeffects(legacy, result);
            return accepted;
        }

        bool writewaterreflections(
            bool enabled,
            WaterSurfaceEffectsState &result)
        {
            sauersettings::WaterSurfaceEffectsState legacy = {};
            const bool accepted = sauersettings::writewaterreflections(
                enabled, legacy);
            copywatersurfaceeffects(legacy, result);
            return accepted;
        }

        bool writecaustics(bool enabled, WaterSurfaceEffectsState &result)
        {
            sauersettings::WaterSurfaceEffectsState legacy = {};
            const bool accepted = sauersettings::writecaustics(
                enabled, legacy);
            copywatersurfaceeffects(legacy, result);
            return accepted;
        }

        bool writeliquidanimation(
            bool enabled,
            WaterSurfaceEffectsState &result)
        {
            sauersettings::WaterSurfaceEffectsState legacy = {};
            const bool accepted = sauersettings::writeliquidanimation(
                enabled, legacy);
            copywatersurfaceeffects(legacy, result);
            return accepted;
        }

        static void copywaterenvironmentreflections(
            const sauersettings::WaterEnvironmentReflectionsState &source,
            WaterEnvironmentReflectionsState &result)
        {
            result.enabled = source.enabled;
        }

        bool readwaterenvironmentreflections(
            WaterEnvironmentReflectionsState &result)
        {
            sauersettings::WaterEnvironmentReflectionsState legacy = {};
            const bool read =
                sauersettings::readwaterenvironmentreflections(legacy);
            copywaterenvironmentreflections(legacy, result);
            return read;
        }

        bool writewaterenvironmentreflections(
            bool enabled,
            WaterEnvironmentReflectionsState &result)
        {
            sauersettings::WaterEnvironmentReflectionsState legacy = {};
            const bool accepted =
                sauersettings::writewaterenvironmentreflections(
                    enabled, legacy);
            copywaterenvironmentreflections(legacy, result);
            return accepted;
        }

        static WaterRenderResolution waterrenderresolution(
            sauersettings::WaterRenderResolution level)
        {
            switch(level)
            {
                case sauersettings::WaterRenderResolution::Minimum:
                    return WaterRenderResolution::Minimum;
                case sauersettings::WaterRenderResolution::Low:
                    return WaterRenderResolution::Low;
                case sauersettings::WaterRenderResolution::Medium:
                    return WaterRenderResolution::Medium;
                case sauersettings::WaterRenderResolution::High:
                    return WaterRenderResolution::High;
                case sauersettings::WaterRenderResolution::VeryHigh:
                    return WaterRenderResolution::VeryHigh;
                case sauersettings::WaterRenderResolution::Ultra:
                    return WaterRenderResolution::Ultra;
            }
            return WaterRenderResolution::Minimum;
        }

        static bool waterrenderresolution(
            WaterRenderResolution level,
            sauersettings::WaterRenderResolution &legacy)
        {
            switch(level)
            {
                case WaterRenderResolution::Minimum:
                    legacy = sauersettings::WaterRenderResolution::Minimum;
                    return true;
                case WaterRenderResolution::Low:
                    legacy = sauersettings::WaterRenderResolution::Low;
                    return true;
                case WaterRenderResolution::Medium:
                    legacy = sauersettings::WaterRenderResolution::Medium;
                    return true;
                case WaterRenderResolution::High:
                    legacy = sauersettings::WaterRenderResolution::High;
                    return true;
                case WaterRenderResolution::VeryHigh:
                    legacy = sauersettings::WaterRenderResolution::VeryHigh;
                    return true;
                case WaterRenderResolution::Ultra:
                    legacy = sauersettings::WaterRenderResolution::Ultra;
                    return true;
            }
            return false;
        }

        static void copywaterrenderresolution(
            const sauersettings::WaterRenderResolutionState &source,
            WaterRenderResolutionState &result)
        {
            result.level = waterrenderresolution(source.level);
            result.configured_exponent = source.configured_exponent;
            result.requested_dimension = source.requested_dimension;
        }

        bool readwaterrenderresolution(
            WaterRenderResolutionState &result)
        {
            sauersettings::WaterRenderResolutionState legacy = {};
            const bool read =
                sauersettings::readwaterrenderresolution(legacy);
            copywaterrenderresolution(legacy, result);
            return read;
        }

        bool writewaterrenderresolution(
            WaterRenderResolution level,
            WaterRenderResolutionState &result)
        {
            sauersettings::WaterRenderResolution legacy_level;
            if(!waterrenderresolution(level, legacy_level))
            {
                readwaterrenderresolution(result);
                return false;
            }

            sauersettings::WaterRenderResolutionState legacy = {};
            const bool accepted =
                sauersettings::writewaterrenderresolution(
                    legacy_level, legacy);
            copywaterrenderresolution(legacy, result);
            return accepted;
        }

        static void copywaterfalleffects(
            const sauersettings::WaterfallEffectsState &source,
            WaterfallEffectsState &result)
        {
            result.refraction = source.refraction;
            result.reflections = source.reflections;
            result.render_targets_available =
                source.render_targets_available;
        }

        bool readwaterfalleffects(WaterfallEffectsState &result)
        {
            sauersettings::WaterfallEffectsState legacy = {};
            const bool read = sauersettings::readwaterfalleffects(legacy);
            copywaterfalleffects(legacy, result);
            return read;
        }

        bool writewaterfallrefraction(
            bool enabled,
            WaterfallEffectsState &result)
        {
            sauersettings::WaterfallEffectsState legacy = {};
            const bool accepted = sauersettings::writewaterfallrefraction(
                enabled, legacy);
            copywaterfalleffects(legacy, result);
            return accepted;
        }

        bool writewaterfallreflections(
            bool enabled,
            WaterfallEffectsState &result)
        {
            sauersettings::WaterfallEffectsState legacy = {};
            const bool accepted = sauersettings::writewaterfallreflections(
                enabled, legacy);
            copywaterfalleffects(legacy, result);
            return accepted;
        }

        static void copyglassreflections(
            const sauersettings::GlassReflectionsState &source,
            GlassReflectionsState &result)
        {
            result.enabled = source.enabled;
        }

        bool readglassreflections(GlassReflectionsState &result)
        {
            sauersettings::GlassReflectionsState legacy = {};
            const bool read = sauersettings::readglassreflections(legacy);
            copyglassreflections(legacy, result);
            return read;
        }

        bool writeglassreflections(
            bool enabled,
            GlassReflectionsState &result)
        {
            sauersettings::GlassReflectionsState legacy = {};
            const bool accepted = sauersettings::writeglassreflections(
                enabled, legacy);
            copyglassreflections(legacy, result);
            return accepted;
        }

        static void copymodelreflections(
            const sauersettings::ModelReflectionsState &source,
            ModelReflectionsState &result)
        {
            result.enabled = source.enabled;
        }

        bool readmodelreflections(ModelReflectionsState &result)
        {
            sauersettings::ModelReflectionsState legacy = {};
            const bool read = sauersettings::readmodelreflections(legacy);
            copymodelreflections(legacy, result);
            return read;
        }

        bool writemodelreflections(
            bool enabled,
            ModelReflectionsState &result)
        {
            sauersettings::ModelReflectionsState legacy = {};
            const bool accepted = sauersettings::writemodelreflections(
                enabled, legacy);
            copymodelreflections(legacy, result);
            return accepted;
        }

        static void copybumpmodels(
            const sauersettings::BumpModelsState &source,
            BumpModelsState &result)
        {
            result.enabled = source.enabled;
        }

        bool readbumpmodels(BumpModelsState &result)
        {
            sauersettings::BumpModelsState legacy = {};
            const bool read = sauersettings::readbumpmodels(legacy);
            copybumpmodels(legacy, result);
            return read;
        }

        bool writebumpmodels(bool enabled, BumpModelsState &result)
        {
            sauersettings::BumpModelsState legacy = {};
            const bool accepted = sauersettings::writebumpmodels(
                enabled, legacy);
            copybumpmodels(legacy, result);
            return accepted;
        }

        static EnvironmentMapResolution environmentmapresolution(
            sauersettings::EnvironmentMapResolution level)
        {
            switch(level)
            {
                case sauersettings::EnvironmentMapResolution::Minimum:
                    return EnvironmentMapResolution::Minimum;
                case sauersettings::EnvironmentMapResolution::VeryLow:
                    return EnvironmentMapResolution::VeryLow;
                case sauersettings::EnvironmentMapResolution::Low:
                    return EnvironmentMapResolution::Low;
                case sauersettings::EnvironmentMapResolution::Medium:
                    return EnvironmentMapResolution::Medium;
                case sauersettings::EnvironmentMapResolution::High:
                    return EnvironmentMapResolution::High;
                case sauersettings::EnvironmentMapResolution::VeryHigh:
                    return EnvironmentMapResolution::VeryHigh;
                case sauersettings::EnvironmentMapResolution::Ultra:
                    return EnvironmentMapResolution::Ultra;
            }
            return EnvironmentMapResolution::Minimum;
        }

        static bool environmentmapresolution(
            EnvironmentMapResolution level,
            sauersettings::EnvironmentMapResolution &legacy)
        {
            switch(level)
            {
                case EnvironmentMapResolution::Minimum:
                    legacy = sauersettings::EnvironmentMapResolution::Minimum;
                    return true;
                case EnvironmentMapResolution::VeryLow:
                    legacy = sauersettings::EnvironmentMapResolution::VeryLow;
                    return true;
                case EnvironmentMapResolution::Low:
                    legacy = sauersettings::EnvironmentMapResolution::Low;
                    return true;
                case EnvironmentMapResolution::Medium:
                    legacy = sauersettings::EnvironmentMapResolution::Medium;
                    return true;
                case EnvironmentMapResolution::High:
                    legacy = sauersettings::EnvironmentMapResolution::High;
                    return true;
                case EnvironmentMapResolution::VeryHigh:
                    legacy = sauersettings::EnvironmentMapResolution::VeryHigh;
                    return true;
                case EnvironmentMapResolution::Ultra:
                    legacy = sauersettings::EnvironmentMapResolution::Ultra;
                    return true;
            }
            return false;
        }

        static void copyenvironmentmapresolution(
            const sauersettings::EnvironmentMapResolutionState &source,
            EnvironmentMapResolutionState &result)
        {
            result.level = environmentmapresolution(source.level);
            result.configured_exponent = source.configured_exponent;
            result.requested_dimension = source.requested_dimension;
        }

        bool readenvironmentmapresolution(
            EnvironmentMapResolutionState &result)
        {
            sauersettings::EnvironmentMapResolutionState legacy = {};
            const bool read =
                sauersettings::readenvironmentmapresolution(legacy);
            copyenvironmentmapresolution(legacy, result);
            return read;
        }

        bool writeenvironmentmapresolution(
            EnvironmentMapResolution level,
            EnvironmentMapResolutionState &result)
        {
            sauersettings::EnvironmentMapResolution legacy_level;
            if(!environmentmapresolution(level, legacy_level))
            {
                readenvironmentmapresolution(result);
                return false;
            }

            sauersettings::EnvironmentMapResolutionState legacy = {};
            const bool accepted =
                sauersettings::writeenvironmentmapresolution(
                    legacy_level, legacy);
            copyenvironmentmapresolution(legacy, result);
            return accepted;
        }

        static DynamicLights dynamiclights(
            sauersettings::DynamicLights setting)
        {
            switch(setting)
            {
                case sauersettings::DynamicLights::Off:
                    return DynamicLights::Off;
                case sauersettings::DynamicLights::Medium:
                    return DynamicLights::Medium;
                case sauersettings::DynamicLights::High:
                    return DynamicLights::High;
                case sauersettings::DynamicLights::Custom:
                    return DynamicLights::Custom;
            }
            return DynamicLights::Custom;
        }

        static sauersettings::DynamicLights dynamiclights(
            DynamicLights setting)
        {
            switch(setting)
            {
                case DynamicLights::Off:
                    return sauersettings::DynamicLights::Off;
                case DynamicLights::Medium:
                    return sauersettings::DynamicLights::Medium;
                case DynamicLights::High:
                    return sauersettings::DynamicLights::High;
                case DynamicLights::Custom:
                    return sauersettings::DynamicLights::Custom;
            }
            return sauersettings::DynamicLights::Custom;
        }

        static void copydynamiclights(
            const sauersettings::DynamicLightsState &source,
            DynamicLightsState &result)
        {
            result.setting = dynamiclights(source.setting);
            result.max_lights = source.max_lights;
        }

        bool readdynamiclights(DynamicLightsState &result)
        {
            sauersettings::DynamicLightsState legacy = {};
            const bool read = sauersettings::readdynamiclights(legacy);
            copydynamiclights(legacy, result);
            return read;
        }

        bool writedynamiclights(
            DynamicLights setting,
            DynamicLightsState &result)
        {
            sauersettings::DynamicLightsState legacy = {};
            const bool accepted = sauersettings::writedynamiclights(
                dynamiclights(setting), legacy);
            copydynamiclights(legacy, result);
            return accepted;
        }

        static void copysoftparticles(
            const sauersettings::SoftParticlesState &source,
            SoftParticlesState &result)
        {
            result.enabled = source.enabled;
        }

        bool readsoftparticles(SoftParticlesState &result)
        {
            sauersettings::SoftParticlesState legacy = {};
            const bool read = sauersettings::readsoftparticles(legacy);
            copysoftparticles(legacy, result);
            return read;
        }

        bool writesoftparticles(
            bool enabled,
            SoftParticlesState &result)
        {
            sauersettings::SoftParticlesState legacy = {};
            const bool accepted = sauersettings::writesoftparticles(
                enabled, legacy);
            copysoftparticles(legacy, result);
            return accepted;
        }

        static SoftParticleQuality softparticlequality(
            sauersettings::SoftParticleQuality quality)
        {
            switch(quality)
            {
                case sauersettings::SoftParticleQuality::Low:
                    return SoftParticleQuality::Low;
                case sauersettings::SoftParticleQuality::Medium:
                    return SoftParticleQuality::Medium;
                case sauersettings::SoftParticleQuality::High:
                    return SoftParticleQuality::High;
                case sauersettings::SoftParticleQuality::Custom:
                    return SoftParticleQuality::Custom;
            }
            return SoftParticleQuality::Custom;
        }

        static sauersettings::SoftParticleQuality softparticlequality(
            SoftParticleQuality quality)
        {
            switch(quality)
            {
                case SoftParticleQuality::Low:
                    return sauersettings::SoftParticleQuality::Low;
                case SoftParticleQuality::Medium:
                    return sauersettings::SoftParticleQuality::Medium;
                case SoftParticleQuality::High:
                    return sauersettings::SoftParticleQuality::High;
                case SoftParticleQuality::Custom:
                    return sauersettings::SoftParticleQuality::Custom;
            }
            return sauersettings::SoftParticleQuality::Custom;
        }

        static void copysoftparticlequality(
            const sauersettings::SoftParticleQualityState &source,
            SoftParticleQualityState &result)
        {
            result.quality = softparticlequality(source.quality);
            result.depth_size = source.depth_size;
            result.blur = source.blur;
        }

        bool readsoftparticlequality(SoftParticleQualityState &result)
        {
            sauersettings::SoftParticleQualityState legacy = {};
            const bool read = sauersettings::readsoftparticlequality(legacy);
            copysoftparticlequality(legacy, result);
            return read;
        }

        bool writesoftparticlequality(
            SoftParticleQuality quality,
            SoftParticleQualityState &result)
        {
            sauersettings::SoftParticleQualityState legacy = {};
            const bool accepted = sauersettings::writesoftparticlequality(
                softparticlequality(quality), legacy);
            copysoftparticlequality(legacy, result);
            return accepted;
        }

        static void copydecals(
            const sauersettings::DecalsState &source,
            DecalsState &result)
        {
            result.enabled = source.enabled;
        }

        bool readdecals(DecalsState &result)
        {
            sauersettings::DecalsState legacy = {};
            const bool read = sauersettings::readdecals(legacy);
            copydecals(legacy, result);
            return read;
        }

        bool writedecals(bool enabled, DecalsState &result)
        {
            sauersettings::DecalsState legacy = {};
            const bool accepted = sauersettings::writedecals(
                enabled, legacy);
            copydecals(legacy, result);
            return accepted;
        }

        static void copyfixtjoints(
            const sauersettings::FixTJointsState &source,
            FixTJointsState &result)
        {
            result.enabled = source.enabled;
        }

        bool readfixtjoints(FixTJointsState &result)
        {
            sauersettings::FixTJointsState legacy = {};
            const bool read = sauersettings::readfixtjoints(legacy);
            copyfixtjoints(legacy, result);
            return read;
        }

        bool writefixtjoints(bool enabled, FixTJointsState &result)
        {
            sauersettings::FixTJointsState legacy = {};
            const bool accepted = sauersettings::writefixtjoints(
                enabled, legacy);
            copyfixtjoints(legacy, result);
            return accepted;
        }

        static DecalPersistence decalpersistence(
            sauersettings::DecalPersistence persistence)
        {
            switch(persistence)
            {
                case sauersettings::DecalPersistence::Short:
                    return DecalPersistence::Short;
                case sauersettings::DecalPersistence::Long:
                    return DecalPersistence::Long;
                case sauersettings::DecalPersistence::Custom:
                    return DecalPersistence::Custom;
            }
            return DecalPersistence::Custom;
        }

        static sauersettings::DecalPersistence decalpersistence(
            DecalPersistence persistence)
        {
            switch(persistence)
            {
                case DecalPersistence::Short:
                    return sauersettings::DecalPersistence::Short;
                case DecalPersistence::Long:
                    return sauersettings::DecalPersistence::Long;
                case DecalPersistence::Custom:
                    return sauersettings::DecalPersistence::Custom;
            }
            return sauersettings::DecalPersistence::Custom;
        }

        static void copydecalpersistence(
            const sauersettings::DecalPersistenceState &source,
            DecalPersistenceState &result)
        {
            result.persistence = decalpersistence(source.persistence);
            result.fade_start_milliseconds =
                source.fade_start_milliseconds;
            result.triangle_budget_per_type =
                source.triangle_budget_per_type;
        }

        bool readdecalpersistence(DecalPersistenceState &result)
        {
            sauersettings::DecalPersistenceState legacy = {};
            const bool read = sauersettings::readdecalpersistence(legacy);
            copydecalpersistence(legacy, result);
            return read;
        }

        bool writedecalpersistence(
            DecalPersistence persistence,
            DecalPersistenceState &result)
        {
            sauersettings::DecalPersistenceState legacy = {};
            const bool accepted = sauersettings::writedecalpersistence(
                decalpersistence(persistence), legacy);
            copydecalpersistence(legacy, result);
            return accepted;
        }

        static GrassDistance grassdistance(
            sauersettings::GrassDistance distance)
        {
            switch(distance)
            {
                case sauersettings::GrassDistance::Quick:
                    return GrassDistance::Quick;
                case sauersettings::GrassDistance::Moderate:
                    return GrassDistance::Moderate;
                case sauersettings::GrassDistance::Slow:
                    return GrassDistance::Slow;
                case sauersettings::GrassDistance::Custom:
                    return GrassDistance::Custom;
            }
            return GrassDistance::Custom;
        }

        static sauersettings::GrassDistance grassdistance(
            GrassDistance distance)
        {
            switch(distance)
            {
                case GrassDistance::Quick:
                    return sauersettings::GrassDistance::Quick;
                case GrassDistance::Moderate:
                    return sauersettings::GrassDistance::Moderate;
                case GrassDistance::Slow:
                    return sauersettings::GrassDistance::Slow;
                case GrassDistance::Custom:
                    return sauersettings::GrassDistance::Custom;
            }
            return sauersettings::GrassDistance::Custom;
        }

        static void copygrassdistance(
            const sauersettings::GrassDistanceState &source,
            GrassDistanceState &result)
        {
            result.distance = grassdistance(source.distance);
            result.value = source.value;
        }

        bool readgrassdistance(GrassDistanceState &result)
        {
            sauersettings::GrassDistanceState legacy = {};
            const bool read = sauersettings::readgrassdistance(legacy);
            copygrassdistance(legacy, result);
            return read;
        }

        bool writegrassdistance(
            GrassDistance distance,
            GrassDistanceState &result)
        {
            sauersettings::GrassDistanceState legacy = {};
            const bool accepted = sauersettings::writegrassdistance(
                grassdistance(distance), legacy);
            copygrassdistance(legacy, result);
            return accepted;
        }

        static MotionBlurIntensity motionblurintensity(
            sauersettings::MotionBlurIntensity intensity)
        {
            switch(intensity)
            {
                case sauersettings::MotionBlurIntensity::Subtle:
                    return MotionBlurIntensity::Subtle;
                case sauersettings::MotionBlurIntensity::Moderate:
                    return MotionBlurIntensity::Moderate;
                case sauersettings::MotionBlurIntensity::Intense:
                    return MotionBlurIntensity::Intense;
                case sauersettings::MotionBlurIntensity::Custom:
                    return MotionBlurIntensity::Custom;
            }
            return MotionBlurIntensity::Custom;
        }

        static sauersettings::MotionBlurIntensity motionblurintensity(
            MotionBlurIntensity intensity)
        {
            switch(intensity)
            {
                case MotionBlurIntensity::Subtle:
                    return sauersettings::MotionBlurIntensity::Subtle;
                case MotionBlurIntensity::Moderate:
                    return sauersettings::MotionBlurIntensity::Moderate;
                case MotionBlurIntensity::Intense:
                    return sauersettings::MotionBlurIntensity::Intense;
                case MotionBlurIntensity::Custom:
                    return sauersettings::MotionBlurIntensity::Custom;
            }
            return sauersettings::MotionBlurIntensity::Custom;
        }

        static void copymotionblurintensity(
            const sauersettings::MotionBlurIntensityState &source,
            MotionBlurIntensityState &result)
        {
            result.intensity = motionblurintensity(source.intensity);
            result.scale = source.scale;
        }

        bool readmotionblurintensity(MotionBlurIntensityState &result)
        {
            sauersettings::MotionBlurIntensityState legacy = {};
            const bool read = sauersettings::readmotionblurintensity(legacy);
            copymotionblurintensity(legacy, result);
            return read;
        }

        bool writemotionblurintensity(
            MotionBlurIntensity intensity,
            MotionBlurIntensityState &result)
        {
            sauersettings::MotionBlurIntensityState legacy = {};
            const bool accepted = sauersettings::writemotionblurintensity(
                motionblurintensity(intensity), legacy);
            copymotionblurintensity(legacy, result);
            return accepted;
        }

        static GlarePreset glarepreset(sauersettings::GlarePreset preset)
        {
            switch(preset)
            {
                case sauersettings::GlarePreset::Subtle:
                    return GlarePreset::Subtle;
                case sauersettings::GlarePreset::Glowy:
                    return GlarePreset::Glowy;
                case sauersettings::GlarePreset::Soft:
                    return GlarePreset::Soft;
                case sauersettings::GlarePreset::Intense:
                    return GlarePreset::Intense;
                case sauersettings::GlarePreset::Custom:
                    return GlarePreset::Custom;
            }
            return GlarePreset::Custom;
        }

        static sauersettings::GlarePreset glarepreset(GlarePreset preset)
        {
            switch(preset)
            {
                case GlarePreset::Subtle:
                    return sauersettings::GlarePreset::Subtle;
                case GlarePreset::Glowy:
                    return sauersettings::GlarePreset::Glowy;
                case GlarePreset::Soft:
                    return sauersettings::GlarePreset::Soft;
                case GlarePreset::Intense:
                    return sauersettings::GlarePreset::Intense;
                case GlarePreset::Custom:
                    return sauersettings::GlarePreset::Custom;
            }
            return sauersettings::GlarePreset::Custom;
        }

        static void copyglarepreset(
            const sauersettings::GlarePresetState &source,
            GlarePresetState &result)
        {
            result.preset = glarepreset(source.preset);
            result.scale = source.scale;
            result.blur_radius = source.blur_radius;
        }

        bool readglarepreset(GlarePresetState &result)
        {
            sauersettings::GlarePresetState legacy = {};
            const bool read = sauersettings::readglarepreset(legacy);
            copyglarepreset(legacy, result);
            return read;
        }

        bool writeglarepreset(GlarePreset preset, GlarePresetState &result)
        {
            sauersettings::GlarePresetState legacy = {};
            const bool accepted = sauersettings::writeglarepreset(
                glarepreset(preset), legacy);
            copyglarepreset(legacy, result);
            return accepted;
        }

        static void copyglareintensity(
            const sauersettings::GlareIntensityState &source,
            GlareIntensityState &result)
        {
            result.configured_scale = source.configured_scale;
        }

        bool readglareintensity(GlareIntensityState &result)
        {
            sauersettings::GlareIntensityState legacy = {};
            const bool read = sauersettings::readglareintensity(legacy);
            copyglareintensity(legacy, result);
            return read;
        }

        bool writeglareintensity(
            float configured_scale,
            GlareIntensityState &result)
        {
            sauersettings::GlareIntensityState legacy = {};
            const bool accepted = sauersettings::writeglareintensity(
                configured_scale, legacy);
            copyglareintensity(legacy, result);
            return accepted;
        }

        static GlareResolution glareresolution(
            sauersettings::GlareResolution level)
        {
            switch(level)
            {
                case sauersettings::GlareResolution::Minimum:
                    return GlareResolution::Minimum;
                case sauersettings::GlareResolution::Low:
                    return GlareResolution::Low;
                case sauersettings::GlareResolution::Medium:
                    return GlareResolution::Medium;
                case sauersettings::GlareResolution::High:
                    return GlareResolution::High;
                case sauersettings::GlareResolution::Ultra:
                    return GlareResolution::Ultra;
            }
            return GlareResolution::Minimum;
        }

        static bool glareresolution(
            GlareResolution level,
            sauersettings::GlareResolution &legacy)
        {
            switch(level)
            {
                case GlareResolution::Minimum:
                    legacy = sauersettings::GlareResolution::Minimum;
                    return true;
                case GlareResolution::Low:
                    legacy = sauersettings::GlareResolution::Low;
                    return true;
                case GlareResolution::Medium:
                    legacy = sauersettings::GlareResolution::Medium;
                    return true;
                case GlareResolution::High:
                    legacy = sauersettings::GlareResolution::High;
                    return true;
                case GlareResolution::Ultra:
                    legacy = sauersettings::GlareResolution::Ultra;
                    return true;
            }
            return false;
        }

        static void copyglareresolution(
            const sauersettings::GlareResolutionState &source,
            GlareResolutionState &result)
        {
            result.level = glareresolution(source.level);
            result.configured_exponent = source.configured_exponent;
            result.requested_width = source.requested_width;
        }

        bool readglareresolution(GlareResolutionState &result)
        {
            sauersettings::GlareResolutionState legacy = {};
            const bool read = sauersettings::readglareresolution(legacy);
            copyglareresolution(legacy, result);
            return read;
        }

        bool writeglareresolution(
            GlareResolution level,
            GlareResolutionState &result)
        {
            sauersettings::GlareResolution legacy_level;
            if(!glareresolution(level, legacy_level))
            {
                readglareresolution(result);
                return false;
            }

            sauersettings::GlareResolutionState legacy = {};
            const bool accepted = sauersettings::writeglareresolution(
                legacy_level, legacy);
            copyglareresolution(legacy, result);
            return accepted;
        }

        static TextureQuality texturequality(
            sauersettings::TextureQuality quality)
        {
            switch(quality)
            {
                case sauersettings::TextureQuality::Low:
                    return TextureQuality::Low;
                case sauersettings::TextureQuality::Medium:
                    return TextureQuality::Medium;
                case sauersettings::TextureQuality::Full:
                    return TextureQuality::Full;
                case sauersettings::TextureQuality::Custom:
                    return TextureQuality::Custom;
            }
            return TextureQuality::Custom;
        }

        static sauersettings::TextureQuality texturequality(
            TextureQuality quality)
        {
            switch(quality)
            {
                case TextureQuality::Low:
                    return sauersettings::TextureQuality::Low;
                case TextureQuality::Medium:
                    return sauersettings::TextureQuality::Medium;
                case TextureQuality::Full:
                    return sauersettings::TextureQuality::Full;
                case TextureQuality::Custom:
                    return sauersettings::TextureQuality::Custom;
            }
            return sauersettings::TextureQuality::Custom;
        }

        static void copytexturequality(
            const sauersettings::TextureQualityState &source,
            TextureQualityState &result)
        {
            result.quality = texturequality(source.quality);
            result.texture_size_limit = source.texture_size_limit;
        }

        bool readtexturequality(TextureQualityState &result)
        {
            sauersettings::TextureQualityState legacy = {};
            const bool read = sauersettings::readtexturequality(legacy);
            copytexturequality(legacy, result);
            return read;
        }

        bool writetexturequality(
            TextureQuality quality,
            TextureQualityState &result)
        {
            sauersettings::TextureQualityState legacy = {};
            const bool accepted = sauersettings::writetexturequality(
                texturequality(quality), legacy);
            copytexturequality(legacy, result);
            return accepted;
        }

        static void copytexturereduction(
            const sauersettings::TextureReductionState &source,
            TextureReductionState &result)
        {
            result.levels = source.levels;
            result.minimum = source.minimum;
            result.maximum = source.maximum;
        }

        bool readtexturereduction(TextureReductionState &result)
        {
            sauersettings::TextureReductionState legacy = {};
            const bool read = sauersettings::readtexturereduction(legacy);
            copytexturereduction(legacy, result);
            return read;
        }

        bool writetexturereduction(
            int levels,
            TextureReductionState &result)
        {
            sauersettings::TextureReductionState legacy = {};
            const bool accepted = sauersettings::writetexturereduction(
                levels, legacy);
            copytexturereduction(legacy, result);
            return accepted;
        }

        static TextureCompressionPreset texturecompressionpreset(
            sauersettings::TextureCompressionPreset preset)
        {
            switch(preset)
            {
                case sauersettings::TextureCompressionPreset::Off:
                    return TextureCompressionPreset::Off;
                case sauersettings::TextureCompressionPreset::AllEligible:
                    return TextureCompressionPreset::AllEligible;
                case sauersettings::TextureCompressionPreset::AtLeast256:
                    return TextureCompressionPreset::AtLeast256;
                case sauersettings::TextureCompressionPreset::AtLeast512:
                    return TextureCompressionPreset::AtLeast512;
                case sauersettings::TextureCompressionPreset::AtLeast1024:
                    return TextureCompressionPreset::AtLeast1024;
                case sauersettings::TextureCompressionPreset::Above1024:
                    return TextureCompressionPreset::Above1024;
                case sauersettings::TextureCompressionPreset::AtLeast2048:
                    return TextureCompressionPreset::AtLeast2048;
                case sauersettings::TextureCompressionPreset::AtLeast4096:
                    return TextureCompressionPreset::AtLeast4096;
                case sauersettings::TextureCompressionPreset::Custom:
                    return TextureCompressionPreset::Custom;
            }
            return TextureCompressionPreset::Custom;
        }

        static void copytexturecompression(
            const sauersettings::TextureCompressionState &source,
            TextureCompressionState &result)
        {
            result.preset = texturecompressionpreset(source.preset);
            result.threshold = source.threshold;
            result.minimum = source.minimum;
            result.maximum = source.maximum;
            result.supported = source.supported;
        }

        bool readtexturecompression(TextureCompressionState &result)
        {
            sauersettings::TextureCompressionState legacy = {};
            const bool read = sauersettings::readtexturecompression(legacy);
            copytexturecompression(legacy, result);
            return read;
        }

        bool writetexturecompression(
            int threshold,
            TextureCompressionState &result)
        {
            sauersettings::TextureCompressionState legacy = {};
            const bool accepted = sauersettings::writetexturecompression(
                threshold, legacy);
            copytexturecompression(legacy, result);
            return accepted;
        }

        static void copyshaderdetail(const sauersettings::ShaderDetailState &source, ShaderDetailState &result)
        {
            result.requested = source.requested;
            result.ready = source.ready;
            result.inspected = source.inspected;
            result.alternatives = source.alternatives;
            result.unresolved = source.unresolved;
        }

        bool readshaderdetail(ShaderDetailState &result)
        {
            sauersettings::ShaderDetailState legacy = {};
            const bool read = sauersettings::readshaderdetail(legacy);
            copyshaderdetail(legacy, result);
            return read;
        }

        bool writeshaderdetail(ShaderDetailLevel level, ShaderDetailState &result)
        {
            sauersettings::ShaderDetailState legacy = {};
            const bool accepted = sauersettings::writeshaderdetail(
                static_cast<sauersettings::ShaderDetailLevel>(static_cast<int>(level)), legacy);
            copyshaderdetail(legacy, result);
            return accepted;
        }

        static void copybilinearfiltering(
            const sauersettings::BilinearFilteringState &source,
            BilinearFilteringState &result)
        {
            result.enabled = source.enabled;
        }

        bool readbilinearfiltering(BilinearFilteringState &result)
        {
            sauersettings::BilinearFilteringState legacy = {};
            const bool read = sauersettings::readbilinearfiltering(legacy);
            copybilinearfiltering(legacy, result);
            return read;
        }

        bool writebilinearfiltering(
            bool enabled,
            BilinearFilteringState &result)
        {
            sauersettings::BilinearFilteringState legacy = {};
            const bool accepted = sauersettings::writebilinearfiltering(
                enabled, legacy);
            copybilinearfiltering(legacy, result);
            return accepted;
        }

        static void copymipmapblending(const sauersettings::MipmapBlendingState &source, MipmapBlendingState &result)
        {
            switch(source.status)
            {
                case sauersettings::MipmapBlendingStatus::Unavailable: result.status = MipmapBlendingStatus::Unavailable; break;
                case sauersettings::MipmapBlendingStatus::NoTextures: result.status = MipmapBlendingStatus::NoTextures; break;
                case sauersettings::MipmapBlendingStatus::Verified: result.status = MipmapBlendingStatus::Verified; break;
                case sauersettings::MipmapBlendingStatus::Mismatch: result.status = MipmapBlendingStatus::Mismatch; break;
                case sauersettings::MipmapBlendingStatus::Mixed: result.status = MipmapBlendingStatus::Mixed; break;
            }
            result.requested = source.requested;
            result.textures = source.textures;
        }

        bool readmipmapblending(MipmapBlendingState &result)
        {
            sauersettings::MipmapBlendingState legacy = {};
            const bool read = sauersettings::readmipmapblending(legacy);
            copymipmapblending(legacy, result);
            return read;
        }

        bool writemipmapblending(bool enabled, MipmapBlendingState &result)
        {
            sauersettings::MipmapBlendingState legacy = {};
            const bool accepted = sauersettings::writemipmapblending(enabled, legacy);
            copymipmapblending(legacy, result);
            return accepted;
        }

        static void copyanisotropy(const sauersettings::AnisotropyState &source, AnisotropyState &result)
        {
            switch(source.status)
            {
                case sauersettings::AnisotropyStatus::Unavailable: result.status = AnisotropyStatus::Unavailable; break;
                case sauersettings::AnisotropyStatus::Unsupported: result.status = AnisotropyStatus::Unsupported; break;
                case sauersettings::AnisotropyStatus::NoTextures: result.status = AnisotropyStatus::NoTextures; break;
                case sauersettings::AnisotropyStatus::Verified: result.status = AnisotropyStatus::Verified; break;
                case sauersettings::AnisotropyStatus::Mismatch: result.status = AnisotropyStatus::Mismatch; break;
                case sauersettings::AnisotropyStatus::Mixed: result.status = AnisotropyStatus::Mixed; break;
            }
            result.requested = source.requested;
            result.maximum = source.maximum;
            result.capped = source.capped;
            result.textures = source.textures;
            result.actual = source.actual;
        }

        bool readanisotropy(AnisotropyState &result)
        {
            sauersettings::AnisotropyState legacy;
            const bool read = sauersettings::readanisotropy(legacy);
            copyanisotropy(legacy, result);
            return read;
        }

        bool writeanisotropy(AnisotropyLevel level, AnisotropyState &result)
        {
            sauersettings::AnisotropyState legacy;
            const bool accepted = sauersettings::writeanisotropy(
                static_cast<sauersettings::AnisotropyLevel>(static_cast<int>(level)), legacy);
            copyanisotropy(legacy, result);
            return accepted;
        }

        static void copyaudio(
            const sauersettings::Audio &source,
            Audio &result)
        {
            result.sound_volume = source.sound_volume;
            result.sound_volume_minimum = source.sound_volume_minimum;
            result.sound_volume_maximum = source.sound_volume_maximum;
            result.music_volume = source.music_volume;
            result.music_volume_minimum = source.music_volume_minimum;
            result.music_volume_maximum = source.music_volume_maximum;
            result.active_soundpack = source.active_soundpack;
            result.backend_active = source.backend_active;
            result.sound_channels = source.sound_channels;
            result.sound_channels_minimum = source.sound_channels_minimum;
            result.sound_channels_maximum = source.sound_channels_maximum;
            result.effective_sound_channels =
                source.effective_sound_channels;
            result.sound_frequency = source.sound_frequency;
            result.effective_sound_frequency =
                source.effective_sound_frequency;
            result.sound_buffer_length = source.sound_buffer_length;
            result.sound_buffer_length_minimum =
                source.sound_buffer_length_minimum;
            result.sound_buffer_length_maximum =
                source.sound_buffer_length_maximum;
            result.mumble = source.mumble;
        }

        bool readaudio(Audio &result)
        {
            sauersettings::Audio legacy = {};
            if(!sauersettings::readaudio(legacy))
                return false;

            copyaudio(legacy, result);
            return true;
        }

        struct SoundpackChoiceAdapter
        {
            SoundpackChoiceVisitor visitor;
            void *data;
        };

        static void visitsoundpackchoice(
            const sauersettings::SoundpackChoice &legacy,
            void *data)
        {
            SoundpackChoiceAdapter &adapter =
                *static_cast<SoundpackChoiceAdapter *>(data);
            SoundpackChoice choice;
            choice.id = legacy.id;
            choice.display_name = legacy.display_name;
            adapter.visitor(choice, adapter.data);
        }

        bool visitsoundpacks(SoundpackChoiceVisitor visitor, void *data)
        {
            if(!visitor)
                return false;

            SoundpackChoiceAdapter adapter;
            adapter.visitor = visitor;
            adapter.data = data;
            return sauersettings::visitsoundpacks(
                visitsoundpackchoice,
                &adapter
            );
        }

        bool writesoundvolume(int value)
        {
            return sauersettings::writesoundvolume(value);
        }

        bool writemusicvolume(int value)
        {
            return sauersettings::writemusicvolume(value);
        }

        bool writesoundpack(const char *id)
        {
            return sauersettings::writesoundpack(id);
        }

        bool writesoundchannels(int value, Audio &result)
        {
            sauersettings::Audio legacy = {};
            const bool accepted =
                sauersettings::writesoundchannels(value, legacy);
            copyaudio(legacy, result);
            return accepted;
        }

        bool writesoundfrequency(int value, Audio &result)
        {
            sauersettings::Audio legacy = {};
            const bool accepted =
                sauersettings::writesoundfrequency(value, legacy);
            copyaudio(legacy, result);
            return accepted;
        }

        bool writesoundbufferlength(int value, Audio &result)
        {
            sauersettings::Audio legacy = {};
            const bool accepted =
                sauersettings::writesoundbufferlength(value, legacy);
            copyaudio(legacy, result);
            return accepted;
        }

        bool writemumble(bool enabled, Audio &result)
        {
            sauersettings::Audio legacy = {};
            const bool accepted =
                sauersettings::writemumble(enabled, legacy);
            copyaudio(legacy, result);
            return accepted;
        }

        bool hassoundchanges()
        {
            return sauersettings::hassoundchanges();
        }

        void resetsound()
        {
            sauersettings::resetsound();
        }

        bool readmouseinput(MouseInput &result)
        {
            sauersettings::MouseInput legacy;
            if(!sauersettings::readmouseinput(legacy))
                return false;

            result.sensitivity = legacy.sensitivity;
            result.sensitivity_minimum =
                legacy.sensitivity_minimum;
            result.sensitivity_maximum =
                legacy.sensitivity_maximum;
            result.inverted = legacy.inverted;
            return true;
        }

        bool writesensitivity(float value)
        {
            return sauersettings::writesensitivity(value);
        }

        bool writeinvertedmouse(bool enabled)
        {
            return sauersettings::writeinvertedmouse(enabled);
        }

        bool readcrosshair(Crosshair &result)
        {
            sauersettings::Crosshair legacy;
            if(!sauersettings::readcrosshair(legacy))
                return false;

            result.selection = legacy.selection;
            result.resource = legacy.resource;
            result.size = legacy.size;
            result.size_minimum = legacy.size_minimum;
            result.size_maximum = legacy.size_maximum;
            result.effects = legacy.effects;
            result.health_colors = legacy.health_colors;
            result.teammates = legacy.teammates;
            result.hit_marker = legacy.hit_marker;
            result.reload_fade = legacy.reload_fade;
            result.reload_rotation = legacy.reload_rotation;
            return true;
        }

        struct CrosshairChoiceAdapter
        {
            CrosshairChoiceVisitor visitor;
            void *data;
        };

        static void visitcrosshairchoice(
            const sauersettings::CrosshairChoice &legacy,
            void *data)
        {
            CrosshairChoiceAdapter &adapter =
                *static_cast<CrosshairChoiceAdapter *>(data);
            CrosshairChoice choice;
            choice.id = legacy.id;
            choice.label = legacy.label;
            choice.resource = legacy.resource;
            adapter.visitor(choice, adapter.data);
        }

        bool visitcrosshairs(CrosshairChoiceVisitor visitor, void *data)
        {
            if(!visitor)
                return false;

            CrosshairChoiceAdapter adapter;
            adapter.visitor = visitor;
            adapter.data = data;
            return sauersettings::visitcrosshairs(
                visitcrosshairchoice,
                &adapter
            );
        }

        bool writecrosshairselection(int selection)
        {
            return sauersettings::writecrosshairselection(selection);
        }

        bool writecrosshairsize(int size)
        {
            return sauersettings::writecrosshairsize(size);
        }

        bool writecrosshaireffects(bool enabled)
        {
            return sauersettings::writecrosshaireffects(enabled);
        }

        bool writecrosshairhealthcolors(bool enabled)
        {
            return sauersettings::writecrosshairhealthcolors(enabled);
        }

        bool writeteamcrosshair(bool enabled)
        {
            return sauersettings::writeteamcrosshair(enabled);
        }

        bool writehitcrosshair(bool enabled)
        {
            return sauersettings::writehitcrosshair(enabled);
        }

        bool writecrosshairreloadfade(bool enabled)
        {
            return sauersettings::writecrosshairreloadfade(enabled);
        }

        bool writecrosshairreloadrotation(bool enabled)
        {
            return sauersettings::writecrosshairreloadrotation(enabled);
        }

        static void copyhud(
            const sauersettings::Hud &source,
            Hud &result)
        {
            result.show_fps = source.show_fps;
            result.wall_clock = source.wall_clock;
            result.wall_clock_24_hour = source.wall_clock_24_hour;
            result.wall_clock_seconds = source.wall_clock_seconds;
            result.damage_screen = source.damage_screen;
            result.damage_compass = source.damage_compass;
            result.hud_guns = source.hud_guns;
            result.hud_gun_sway = source.hud_gun_sway;
            result.muzzle_flash = source.muzzle_flash;
            result.muzzle_light = source.muzzle_light;
            result.frag_messages = fragmessagesmode(source.frag_messages);
            result.max_frag_messages = source.max_frag_messages;
            result.max_frag_messages_minimum =
                source.max_frag_messages_minimum;
            result.max_frag_messages_maximum =
                source.max_frag_messages_maximum;
            result.frag_message_duration = source.frag_message_duration;
            result.frag_message_duration_minimum =
                source.frag_message_duration_minimum;
            result.frag_message_duration_maximum =
                source.frag_message_duration_maximum;
            result.frag_message_self_frags = source.frag_message_self_frags;
            result.frag_message_other_frags = source.frag_message_other_frags;
            result.frag_message_team_kills = source.frag_message_team_kills;
            result.frag_message_scale = source.frag_message_scale;
            result.frag_message_scale_minimum =
                source.frag_message_scale_minimum;
            result.frag_message_scale_maximum =
                source.frag_message_scale_maximum;
            result.frag_message_fade = source.frag_message_fade;
            result.frag_message_x = source.frag_message_x;
            result.frag_message_x_minimum = source.frag_message_x_minimum;
            result.frag_message_x_maximum = source.frag_message_x_maximum;
            result.frag_message_y = source.frag_message_y;
            result.frag_message_y_minimum = source.frag_message_y_minimum;
            result.frag_message_y_maximum = source.frag_message_y_maximum;
            result.frag_message_stack_dir = source.frag_message_stack_dir;
            result.streak_milestone_scale = source.streak_milestone_scale;
            result.streak_milestone_scale_minimum =
                source.streak_milestone_scale_minimum;
            result.streak_milestone_scale_maximum =
                source.streak_milestone_scale_maximum;
        }

        bool readhud(Hud &result)
        {
            sauersettings::Hud legacy;
            if(!sauersettings::readhud(legacy))
                return false;

            copyhud(legacy, result);
            return true;
        }

        bool writeshowfps(bool enabled)
        {
            return sauersettings::writeshowfps(enabled);
        }

        bool writewallclock(bool enabled)
        {
            return sauersettings::writewallclock(enabled);
        }

        bool writewallclock24hour(bool enabled)
        {
            return sauersettings::writewallclock24hour(enabled);
        }

        bool writewallclockseconds(bool enabled)
        {
            return sauersettings::writewallclockseconds(enabled);
        }

        bool writedamagescreen(bool enabled)
        {
            return sauersettings::writedamagescreen(enabled);
        }

        bool writedamagecompass(bool enabled)
        {
            return sauersettings::writedamagecompass(enabled);
        }

        bool writehudguns(bool enabled)
        {
            return sauersettings::writehudguns(enabled);
        }

        bool writehudgunsway(bool enabled)
        {
            return sauersettings::writehudgunsway(enabled);
        }

        bool writemuzzleflash(bool enabled)
        {
            return sauersettings::writemuzzleflash(enabled);
        }

        bool writemuzzlelight(bool enabled)
        {
            return sauersettings::writemuzzlelight(enabled);
        }

        bool writefragmessages(FragMessagesMode mode)
        {
            return sauersettings::writefragmessages(
                fragmessagesmode(mode)
            );
        }

        bool writestreakmilestonescale(float value)
        {
            return sauersettings::writestreakmilestonescale(value);
        }

        bool writemaxfragmessages(int value, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writemaxfragmessages(
                value, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        bool writefragmessageduration(float value, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writefragmessageduration(
                value, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        bool writefragmessageselffrags(bool enabled, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writefragmessageselffrags(
                enabled, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        bool writefragmessageotherfrags(bool enabled, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writefragmessageotherfrags(
                enabled, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        bool writefragmessageteamkills(bool enabled, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writefragmessageteamkills(
                enabled, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        bool writefragmessagescale(float value, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writefragmessagescale(
                value, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        bool writefragmessagefade(bool enabled, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writefragmessagefade(
                enabled, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        bool writefragmessagex(float value, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writefragmessagex(
                value, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        bool writefragmessagey(float value, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writefragmessagey(
                value, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        bool writefragmessagestackdir(int value, Hud &result)
        {
            sauersettings::Hud legacy = {};
            const bool accepted = sauersettings::writefragmessagestackdir(
                value, legacy);
            copyhud(legacy, result);
            return accepted;
        }

        static void copyammobar(
            const sauersettings::Ammobar &source,
            Ammobar &result)
        {
            result.visible = source.visible;
            result.horizontal = source.horizontal;
            result.flip = source.flip;
            result.scale = source.scale;
            result.scale_minimum = source.scale_minimum;
            result.scale_maximum = source.scale_maximum;
            result.icon_spacing = source.icon_spacing;
            result.icon_spacing_minimum = source.icon_spacing_minimum;
            result.icon_spacing_maximum = source.icon_spacing_maximum;
            result.label_size = source.label_size;
            result.label_size_minimum = source.label_size_minimum;
            result.label_size_maximum = source.label_size_maximum;
            result.label_offset = source.label_offset;
            result.label_offset_minimum = source.label_offset_minimum;
            result.label_offset_maximum = source.label_offset_maximum;
            result.align = source.align;
            result.x = source.x;
            result.x_minimum = source.x_minimum;
            result.x_maximum = source.x_maximum;
            result.y = source.y;
            result.y_minimum = source.y_minimum;
            result.y_maximum = source.y_maximum;
            result.hide_empty = source.hide_empty;
        }

        bool readammobar(Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool read = sauersettings::readammobar(legacy);
            copyammobar(legacy, result);
            return read;
        }

        bool writeammobar(bool enabled, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobar(
                enabled, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobarhorizontal(bool enabled, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobarhorizontal(
                enabled, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobarflip(bool enabled, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobarflip(
                enabled, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobarscale(float value, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobarscale(
                value, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobarsep(int value, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobarsep(
                value, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobarcountscale(float value, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobarcountscale(
                value, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobarcountsep(int value, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobarcountsep(
                value, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobaralign(int value, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobaralign(
                value, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobarx(float value, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobarx(
                value, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobary(float value, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobary(
                value, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        bool writeammobarhideempty(bool enabled, Ammobar &result)
        {
            sauersettings::Ammobar legacy = {};
            const bool accepted = sauersettings::writeammobarhideempty(
                enabled, legacy);
            copyammobar(legacy, result);
            return accepted;
        }

        static void copygameclock(
            const sauersettings::GameClock &source,
            GameClock &result)
        {
            result.enabled = source.enabled;
            result.scale = source.scale;
            result.scale_minimum = source.scale_minimum;
            result.scale_maximum = source.scale_maximum;
            result.alpha = source.alpha;
            result.alpha_minimum = source.alpha_minimum;
            result.alpha_maximum = source.alpha_maximum;
            result.align = source.align;
            result.x = source.x;
            result.x_minimum = source.x_minimum;
            result.x_maximum = source.x_maximum;
            result.y = source.y;
            result.y_minimum = source.y_minimum;
            result.y_maximum = source.y_maximum;
        }

        bool readgameclock(GameClock &result)
        {
            sauersettings::GameClock legacy = {};
            const bool read = sauersettings::readgameclock(legacy);
            copygameclock(legacy, result);
            return read;
        }

        bool writegameclock(bool enabled, GameClock &result)
        {
            sauersettings::GameClock legacy = {};
            const bool accepted = sauersettings::writegameclock(
                enabled, legacy);
            copygameclock(legacy, result);
            return accepted;
        }

        bool writegameclockscale(float value, GameClock &result)
        {
            sauersettings::GameClock legacy = {};
            const bool accepted = sauersettings::writegameclockscale(
                value, legacy);
            copygameclock(legacy, result);
            return accepted;
        }

        bool writegameclockalpha(int value, GameClock &result)
        {
            sauersettings::GameClock legacy = {};
            const bool accepted = sauersettings::writegameclockalpha(
                value, legacy);
            copygameclock(legacy, result);
            return accepted;
        }

        bool writegameclockalign(int value, GameClock &result)
        {
            sauersettings::GameClock legacy = {};
            const bool accepted = sauersettings::writegameclockalign(
                value, legacy);
            copygameclock(legacy, result);
            return accepted;
        }

        bool writegameclockx(float value, GameClock &result)
        {
            sauersettings::GameClock legacy = {};
            const bool accepted = sauersettings::writegameclockx(
                value, legacy);
            copygameclock(legacy, result);
            return accepted;
        }

        bool writegameclocky(float value, GameClock &result)
        {
            sauersettings::GameClock legacy = {};
            const bool accepted = sauersettings::writegameclocky(
                value, legacy);
            copygameclock(legacy, result);
            return accepted;
        }

        static void copyhudscore(
            const sauersettings::HudScore &source,
            HudScore &result)
        {
            result.enabled = source.enabled;
            result.scale = source.scale;
            result.scale_minimum = source.scale_minimum;
            result.scale_maximum = source.scale_maximum;
            result.alpha = source.alpha;
            result.alpha_minimum = source.alpha_minimum;
            result.alpha_maximum = source.alpha_maximum;
            result.spacing = source.spacing;
            result.spacing_minimum = source.spacing_minimum;
            result.spacing_maximum = source.spacing_maximum;
            result.align = source.align;
            result.x = source.x;
            result.x_minimum = source.x_minimum;
            result.x_maximum = source.x_maximum;
            result.y = source.y;
            result.y_minimum = source.y_minimum;
            result.y_maximum = source.y_maximum;
        }

        bool readhudscore(HudScore &result)
        {
            sauersettings::HudScore legacy = {};
            const bool read = sauersettings::readhudscore(legacy);
            copyhudscore(legacy, result);
            return read;
        }

        bool writehudscore(bool enabled, HudScore &result)
        {
            sauersettings::HudScore legacy = {};
            const bool accepted = sauersettings::writehudscore(
                enabled, legacy);
            copyhudscore(legacy, result);
            return accepted;
        }

        bool writehudscorescale(float value, HudScore &result)
        {
            sauersettings::HudScore legacy = {};
            const bool accepted = sauersettings::writehudscorescale(
                value, legacy);
            copyhudscore(legacy, result);
            return accepted;
        }

        bool writehudscorealpha(int value, HudScore &result)
        {
            sauersettings::HudScore legacy = {};
            const bool accepted = sauersettings::writehudscorealpha(
                value, legacy);
            copyhudscore(legacy, result);
            return accepted;
        }

        bool writehudscoresep(int value, HudScore &result)
        {
            sauersettings::HudScore legacy = {};
            const bool accepted = sauersettings::writehudscoresep(
                value, legacy);
            copyhudscore(legacy, result);
            return accepted;
        }

        bool writehudscorealign(int value, HudScore &result)
        {
            sauersettings::HudScore legacy = {};
            const bool accepted = sauersettings::writehudscorealign(
                value, legacy);
            copyhudscore(legacy, result);
            return accepted;
        }

        bool writehudscorex(float value, HudScore &result)
        {
            sauersettings::HudScore legacy = {};
            const bool accepted = sauersettings::writehudscorex(
                value, legacy);
            copyhudscore(legacy, result);
            return accepted;
        }

        bool writehudscorey(float value, HudScore &result)
        {
            sauersettings::HudScore legacy = {};
            const bool accepted = sauersettings::writehudscorey(
                value, legacy);
            copyhudscore(legacy, result);
            return accepted;
        }

        static void copyscoreboardcolumns(
            const sauersettings::ScoreboardColumns &source,
            ScoreboardColumns &result)
        {
            result.show_flags = source.show_flags;
            result.show_frags = source.show_frags;
            result.show_deaths = source.show_deaths;
            result.show_suicides = source.show_suicides;
            result.show_kpd = source.show_kpd;
            result.show_accuracy = source.show_accuracy;
            result.show_damage = source.show_damage;
            result.showdamage = source.showdamage;
            result.show_damage_dealt = source.show_damage_dealt;
            result.show_damage_net = source.show_damage_net;
            result.show_damage_received = source.show_damage_received;
            result.show_ping = source.show_ping;
            result.showping = source.showping;
            result.show_pj = source.show_pj;
            result.show_clientnum = source.show_clientnum;
        }

        bool readscoreboardcolumns(ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool read = sauersettings::readscoreboardcolumns(legacy);
            copyscoreboardcolumns(legacy, result);
            return read;
        }

        bool writeshowflags(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowflags(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writehidefrags(bool show_column, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writehidefrags(
                show_column, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowdeaths(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowdeaths(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowsuicides(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowsuicides(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowkpd(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowkpd(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowaccuracy(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowaccuracy(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowdamage(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowdamage(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowdamagemode(int value, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowdamagemode(
                value, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowdamagereceived(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowdamagereceived(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowping(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowping(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowpj(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowpj(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        bool writeshowclientnum(bool enabled, ScoreboardColumns &result)
        {
            sauersettings::ScoreboardColumns legacy = {};
            const bool accepted = sauersettings::writeshowclientnum(
                enabled, legacy);
            copyscoreboardcolumns(legacy, result);
            return accepted;
        }

        static void copychat(
            const sauersettings::Chat &source,
            Chat &result)
        {
            result.public_messages_in_top_feed =
                source.public_messages_in_top_feed;
            result.team_messages_in_top_feed =
                source.team_messages_in_top_feed;
            result.public_messages_in_bottom_box =
                source.public_messages_in_bottom_box;
            result.team_messages_in_bottom_box =
                source.team_messages_in_bottom_box;
            result.team_colored_names = source.team_colored_names;
            result.auto_ignore_enabled = source.auto_ignore_enabled;
            result.auto_ignore_slot_count = source.auto_ignore_slot_count;
            for(int slot = 0; slot < Chat::AutoIgnoreSlotCount; ++slot)
            {
                std::strncpy(
                    result.auto_ignore_names[slot],
                    source.auto_ignore_names[slot],
                    Chat::AutoIgnoreNameSize - 1
                );
                result.auto_ignore_names[slot][Chat::AutoIgnoreNameSize - 1] = '\0';
            }
        }

        bool readchat(Chat &result)
        {
            sauersettings::Chat legacy = {};
            if(!sauersettings::readchat(legacy))
                return false;

            copychat(legacy, result);
            return true;
        }

        bool writepublicchatintopfeed(bool enabled)
        {
            return sauersettings::writepublicchatintopfeed(enabled);
        }

        bool writeteamchatintopfeed(bool enabled)
        {
            return sauersettings::writeteamchatintopfeed(enabled);
        }

        bool writepublicchatinbottombox(bool enabled)
        {
            return sauersettings::writepublicchatinbottombox(enabled);
        }

        bool writeteamchatinbottombox(bool enabled)
        {
            return sauersettings::writeteamchatinbottombox(enabled);
        }

        bool writeteamcoloredchatnames(bool enabled)
        {
            return sauersettings::writeteamcoloredchatnames(enabled);
        }

        bool writeautoignoreenabled(bool enabled, Chat &result)
        {
            sauersettings::Chat legacy = {};
            const bool accepted =
                sauersettings::writeautoignoreenabled(enabled, legacy);
            if(!accepted)
            {
                readchat(result);
                return false;
            }

            copychat(legacy, result);
            return true;
        }

        bool writeautoignorename(int slot, const char *name, Chat &result)
        {
            sauersettings::Chat legacy = {};
            const bool accepted =
                sauersettings::writeautoignorename(slot, name, legacy);
            if(!accepted)
            {
                readchat(result);
                return false;
            }

            copychat(legacy, result);
            return true;
        }

        bool writeautoignoreslotcount(int count, Chat &result)
        {
            sauersettings::Chat legacy = {};
            const bool accepted =
                sauersettings::writeautoignoreslotcount(count, legacy);
            if(!accepted)
            {
                readchat(result);
                return false;
            }

            copychat(legacy, result);
            return true;
        }

        struct AutoexecChoiceAdapter
        {
            AutoexecChoiceVisitor visitor;
            void *data;
        };

        static void visitautoexecchoice(
            const sauersettings::AutoexecChoice &legacy,
            void *data)
        {
            AutoexecChoiceAdapter &adapter =
                *static_cast<AutoexecChoiceAdapter *>(data);
            AutoexecChoice choice;
            choice.id = legacy.id;
            choice.display_name = legacy.display_name;
            choice.selected = legacy.selected;
            adapter.visitor(choice, adapter.data);
        }

        bool visitautoexecprofiles(
            AutoexecChoiceVisitor visitor,
            void *data)
        {
            if(!visitor)
                return false;
            AutoexecChoiceAdapter adapter = { visitor, data };
            return sauersettings::visitautoexecprofiles(
                visitautoexecchoice, &adapter);
        }

        bool writeautoexecprofile(const char *id)
        {
            return sauersettings::writeautoexecprofile(id);
        }

        bool runautoexecprofile(const char *id)
        {
            return sauersettings::runautoexecprofile(id);
        }

        bool readtheme(Theme &result)
        {
            sauersettings::Theme current;
            if(!sauersettings::readtheme(current))
                return false;
            result = theme(current);
            return true;
        }

        bool writetheme(Theme requested, Theme &result)
        {
            sauersettings::Theme current;
            const bool accepted = sauersettings::writetheme(
                theme(requested), current);
            result = theme(current);
            return accepted;
        }

        static void copyinterfaceappearance(
            const sauersettings::InterfaceAppearance &source,
            InterfaceAppearance &result)
        {
            result.ui_scale = source.ui_scale;
            result.ui_scale_minimum = source.ui_scale_minimum;
            result.ui_scale_maximum = source.ui_scale_maximum;
            result.text_scale = source.text_scale;
            result.text_scale_minimum = source.text_scale_minimum;
            result.text_scale_maximum = source.text_scale_maximum;
            result.font = source.font == sauersettings::InterfaceFont::Legacy
                ? InterfaceFont::Legacy
                : InterfaceFont::Lato;
        }

        bool readinterfaceappearance(InterfaceAppearance &result)
        {
            sauersettings::InterfaceAppearance current;
            const bool read = sauersettings::readinterfaceappearance(current);
            if(read)
                copyinterfaceappearance(current, result);
            return read;
        }

        bool writeuiscale(int value, InterfaceAppearance &result)
        {
            sauersettings::InterfaceAppearance current;
            const bool accepted = sauersettings::writeuiscale(value, current);
            if(accepted || sauersettings::readinterfaceappearance(current))
                copyinterfaceappearance(current, result);
            return accepted;
        }

        bool writeuitextscale(int value, InterfaceAppearance &result)
        {
            sauersettings::InterfaceAppearance current;
            const bool accepted = sauersettings::writeuitextscale(
                value, current);
            if(accepted || sauersettings::readinterfaceappearance(current))
                copyinterfaceappearance(current, result);
            return accepted;
        }

        bool writeinterfacefont(
            InterfaceFont requested, InterfaceAppearance &result)
        {
            sauersettings::InterfaceAppearance current;
            const sauersettings::InterfaceFont font =
                requested == InterfaceFont::Legacy
                    ? sauersettings::InterfaceFont::Legacy
                    : sauersettings::InterfaceFont::Lato;
            const bool accepted =
                sauersettings::writeinterfacefont(font, current);
            if(accepted || sauersettings::readinterfaceappearance(current))
                copyinterfaceappearance(current, result);
            return accepted;
        }

        bool resetinterfaceappearance(InterfaceAppearance &result)
        {
            sauersettings::InterfaceAppearance current;
            const bool accepted =
                sauersettings::resetinterfaceappearance(current);
            if(accepted || sauersettings::readinterfaceappearance(current))
                copyinterfaceappearance(current, result);
            return accepted;
        }

        static void copyconsole(
            const sauersettings::Console &source,
            Console &result)
        {
            result.important_information_in_top_feed =
                source.important_information_in_top_feed;
            result.warnings_in_top_feed = source.warnings_in_top_feed;
            result.errors_in_top_feed = source.errors_in_top_feed;
            result.script_messages_in_top_feed =
                source.script_messages_in_top_feed;
            result.game_events_in_top_feed =
                source.game_events_in_top_feed;
            result.self_frags_in_top_feed =
                source.self_frags_in_top_feed;
            result.other_player_frags_in_top_feed =
                source.other_player_frags_in_top_feed;
            result.team_kills_in_top_feed =
                source.team_kills_in_top_feed;
            result.important_information_in_full_console =
                source.important_information_in_full_console;
            result.warnings_in_full_console =
                source.warnings_in_full_console;
            result.errors_in_full_console =
                source.errors_in_full_console;
            result.script_messages_in_full_console =
                source.script_messages_in_full_console;
            result.public_messages_in_full_console =
                source.public_messages_in_full_console;
            result.team_messages_in_full_console =
                source.team_messages_in_full_console;
            result.game_events_in_full_console =
                source.game_events_in_full_console;
            result.self_frags_in_full_console =
                source.self_frags_in_full_console;
            result.other_player_frags_in_full_console =
                source.other_player_frags_in_full_console;
            result.team_kills_in_full_console =
                source.team_kills_in_full_console;
            result.size = source.size;
            result.size_minimum = source.size_minimum;
            result.size_maximum = source.size_maximum;
        }

        bool readconsole(Console &result)
        {
            sauersettings::Console legacy = {};
            const bool read = sauersettings::readconsole(legacy);
            copyconsole(legacy, result);
            return read;
        }

        bool writeimportantintopfeed(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted = sauersettings::writeimportantintopfeed(
                enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writewarningsintopfeed(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted = sauersettings::writewarningsintopfeed(
                enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeerrorsintopfeed(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted = sauersettings::writeerrorsintopfeed(
                enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writescriptmessagesintopfeed(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writescriptmessagesintopfeed(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writegameeventsintopfeed(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted = sauersettings::writegameeventsintopfeed(
                enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeselffragsintopfeed(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted = sauersettings::writeselffragsintopfeed(
                enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeotherplayerfragsintopfeed(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writeotherplayerfragsintopfeed(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeteamkillsintopfeed(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted = sauersettings::writeteamkillsintopfeed(
                enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeimportantinfullconsole(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writeimportantinfullconsole(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writewarningsinfullconsole(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted = sauersettings::writewarningsinfullconsole(
                enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeerrorsinfullconsole(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted = sauersettings::writeerrorsinfullconsole(
                enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writescriptmessagesinfullconsole(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writescriptmessagesinfullconsole(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writepublicchatinfullconsole(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writepublicchatinfullconsole(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeteamchatinfullconsole(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writeteamchatinfullconsole(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writegameeventsinfullconsole(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writegameeventsinfullconsole(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeselffragsinfullconsole(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writeselffragsinfullconsole(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeotherplayerfragsinfullconsole(bool enabled,
            Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writeotherplayerfragsinfullconsole(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeteamkillsinfullconsole(bool enabled, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted =
                sauersettings::writeteamkillsinfullconsole(
                    enabled, legacy);
            copyconsole(legacy, result);
            return accepted;
        }

        bool writeconsolesize(int value, Console &result)
        {
            sauersettings::Console legacy = {};
            const bool accepted = sauersettings::writeconsolesize(
                value, legacy);
            copyconsole(legacy, result);
            return accepted;
        }
    }
}
