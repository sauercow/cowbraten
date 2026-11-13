#ifndef COWBRATEN_UI_RML_CONSOLE_TEXT_H
#define COWBRATEN_UI_RML_CONSOLE_TEXT_H

#include <string>
#include <vector>

namespace cowui
{
    namespace consoletext
    {
        inline void appendescaped(std::string &target, char value)
        {
            switch(value)
            {
                case '&': target += "&amp;"; break;
                case '<': target += "&lt;"; break;
                case '>': target += "&gt;"; break;
                case '\"': target += "&quot;"; break;
                case '\'': target += "&#39;"; break;
                default: target.push_back(value); break;
            }
        }

        inline bool supportedcolor(char value)
        {
            return value >= '0' && value <= '9';
        }

        inline std::string markup(const char *text)
        {
            if(!text) return std::string();

            std::string result, segment;
            std::vector<char> colors(1, '7');
            const auto flush = [&]()
            {
                if(segment.empty()) return;
                result += "<span class=\"cube-color-";
                result.push_back(colors.back());
                result += "\">";
                result += segment;
                result += "</span>";
                segment.clear();
            };

            for(const char *cursor = text; *cursor; ++cursor)
            {
                if(*cursor == '\f' && cursor[1])
                {
                    flush();
                    const char code = *++cursor;
                    if(code == 's') colors.push_back(colors.back());
                    else if(code == 'r')
                    {
                        if(colors.size() > 1) colors.pop_back();
                    }
                    else if(supportedcolor(code)) colors.back() = code;
                    continue;
                }
                appendescaped(segment, *cursor);
            }
            flush();
            return result;
        }
    }
}

#endif
