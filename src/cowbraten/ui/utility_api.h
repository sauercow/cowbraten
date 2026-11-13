#ifndef COWBRATEN_UI_UTILITY_API_H
#define COWBRATEN_UI_UTILITY_API_H

#include <string>

namespace cowui
{
    namespace utility
    {
        bool loadtext(const char *filename, std::string &text);
        bool savetext(const char *filename, const char *text);
        bool executecommands(const char *text);
        bool readclipboard(std::string &text);
        bool writeclipboard(const char *text);
    }
}

#endif
