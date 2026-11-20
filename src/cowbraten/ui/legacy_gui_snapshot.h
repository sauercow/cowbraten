#pragma once

namespace cowui
{
    enum class LegacyGuiControlType
    {
        Text,
        Title,
        Button,
        Separator,
        ListBegin,
        ListEnd,
        Checkbox,
        Radio,
        Bitfield,
        Slider,
        Field,
        KeyField,
        Image,
        Tab,
        Unsupported
    };

    struct LegacyGuiSink
    {
        virtual ~LegacyGuiSink() {}
        virtual void begin(const char *name, const char *title) = 0;
        virtual void control(LegacyGuiControlType type, int activation,
            const char *label, const char *value, const char *display_value,
            const char *icon,
            int minimum, int maximum, bool active) = 0;
        virtual void end() = 0;
    };

    // The engine executes the original declarative newgui body against a
    // recorder. RmlUi receives only evaluated labels, values and opaque
    // activation indices; CubeScript source never crosses into the document.
    bool capturelegacygui(const char *name, LegacyGuiSink &sink);
    bool activatelegacygui(int activation, const char *value = nullptr);
}
