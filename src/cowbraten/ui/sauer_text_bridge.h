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
    }
}

#endif
