#pragma once

// Standalone policy: no Sauer or RmlUi dependencies. This deliberately reuses
// the existing frag-message duration and optional 255-ms legacy fade period.
namespace cowui
{
    namespace fragfeedback
    {
        struct Frame
        {
            bool visible;
            float opacity;
        };

        inline Frame frame(int now, int started, int duration, bool fade, bool active)
        {
            if(!active || started < 0) return {false, 0.0f};
            const int elapsed = now - started;
            if(elapsed < 0) return {false, 0.0f};
            const int hold = duration > 0 ? duration : 0;
            if(elapsed > hold + (fade ? 255 : 0)) return {false, 0.0f};
            if(!fade || elapsed <= hold) return {true, 1.0f};
            const float opacity = 1.0f - float(elapsed - hold) / 255.0f;
            return {true, opacity > 0 ? opacity : 0};
        }
    }
}
