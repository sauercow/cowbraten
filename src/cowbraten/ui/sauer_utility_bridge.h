#ifndef COWBRATEN_UI_SAUER_UTILITY_BRIDGE_H
#define COWBRATEN_UI_SAUER_UTILITY_BRIDGE_H

namespace cowui
{
    namespace sauerutility
    {
        typedef void (*TextVisitor)(const char *text, void *data);
        bool loadtext(const char *filename, TextVisitor visitor, void *data);
        bool savetext(const char *filename, const char *text);
        bool executecommands(const char *text);
        bool readclipboard(TextVisitor visitor, void *data);
        bool writeclipboard(const char *text);
    }
}

#endif
