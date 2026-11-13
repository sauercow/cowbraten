#ifndef COWBRATEN_UI_CUBE_TEXT_H
#define COWBRATEN_UI_CUBE_TEXT_H

#include <string>

namespace cowui
{
    inline std::string plain_cube_text(const std::string &text)
    {
        std::string result;
        result.reserve(text.size());
        for(std::size_t i = 0; i < text.size(); ++i)
        {
            if(text[i] == '\f')
            {
                if(i + 1 < text.size()) ++i;
                continue;
            }
            result.push_back(text[i]);
        }
        return result;
    }
}

#endif
