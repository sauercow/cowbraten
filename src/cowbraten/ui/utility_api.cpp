#include "utility_api.h"
#include "sauer_utility_bridge.h"

namespace cowui
{
    namespace utility
    {
        static void copytext(const char *text, void *data)
        {
            *static_cast<std::string *>(data) = text ? text : "";
        }

        bool loadtext(const char *filename, std::string &text)
        {
            text.clear();
            return sauerutility::loadtext(filename, copytext, &text);
        }
        bool savetext(const char *filename, const char *text)
        {
            return sauerutility::savetext(filename, text);
        }
        bool executecommands(const char *text)
        {
            return sauerutility::executecommands(text);
        }
        bool readclipboard(std::string &text)
        {
            text.clear();
            return sauerutility::readclipboard(copytext, &text);
        }
        bool writeclipboard(const char *text)
        {
            return sauerutility::writeclipboard(text);
        }
    }
}
