#include "engine.h"
#include "sauer_utility_bridge.h"

#include <SDL.h>

namespace cowui
{
    namespace sauerutility
    {
        static bool validfilename(const char *filename)
        {
            if(!filename || !filename[0] || filename[0] == '/' ||
               strstr(filename, "..") || strchr(filename, ':')) return false;
            loopi(strlen(filename))
            {
                const uchar c = filename[i];
                if(c < 32 || c == '\\') return false;
            }
            return true;
        }

        bool loadtext(const char *filename, TextVisitor visitor, void *data)
        {
            if(!validfilename(filename) || !visitor) return false;
            size_t size = 0;
            char *text = loadfile(filename, &size, true, false);
            if(!text) return false;
            visitor(text, data);
            delete[] text;
            return true;
        }

        bool savetext(const char *filename, const char *text)
        {
            if(!validfilename(filename) || !text) return false;
            stream *file = openutf8file(filename, "w");
            if(!file) return false;
            file->write(text, strlen(text));
            delete file;
            return true;
        }

        bool executecommands(const char *text)
        {
            if(!text || !text[0]) return false;
            execute(text);
            return true;
        }

        bool readclipboard(TextVisitor visitor, void *data)
        {
            if(!visitor) return false;
            char *text = SDL_GetClipboardText();
            if(!text) return false;
            visitor(text, data);
            SDL_free(text);
            return true;
        }

        bool writeclipboard(const char *text)
        {
            return text && SDL_SetClipboardText(text) == 0;
        }
    }
}
