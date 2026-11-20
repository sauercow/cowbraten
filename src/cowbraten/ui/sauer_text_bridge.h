#ifndef COWBRATEN_UI_SAUER_TEXT_BRIDGE_H
#define COWBRATEN_UI_SAUER_TEXT_BRIDGE_H

#include <cstddef>
#include <string>

namespace cowui
{
    namespace sauertext
    {
        // Sauer stores console text in its one-byte Cube encoding. RmlUi
        // expects UTF-8, so every engine-owned string must cross this boundary
        // before it is exposed to an RML data model.
        std::string utf8(const char *text);
        std::string utf8(const char *text, std::size_t length);

        // Convert UTF-8 supplied by RmlUi back to Sauer's one-byte Cube
        // encoding before passing it to engine APIs.
        std::string cube(const char *text);
        std::string cube(const char *text, std::size_t length);
    }
}

#endif
