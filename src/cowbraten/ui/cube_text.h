#ifndef COWBRATEN_UI_CUBE_TEXT_H
#define COWBRATEN_UI_CUBE_TEXT_H

#include "sauer_text_bridge.h"

#include <string>

namespace cowui
{
    inline std::string plain_cube_text(const std::string &text)
    {
        const std::string utf8 = sauertext::utf8(text.c_str(), text.size());
        std::string result;
        result.reserve(utf8.size());
        for(std::size_t i = 0; i < utf8.size(); ++i)
        {
            if(utf8[i] == '\f')
            {
                if(i + 1 < utf8.size()) ++i;
                continue;
            }
            result.push_back(utf8[i]);
        }
        return result;
    }
}

#endif
