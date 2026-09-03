#ifndef COWBRATEN_UI_SAUER_FILE_BRIDGE_H
#define COWBRATEN_UI_SAUER_FILE_BRIDGE_H

#include <cstddef>

namespace cowui
{
    namespace sauerfile
    {
        void *open(const char *path);
        void close(void *file);

        std::size_t read(
            void *buffer,
            std::size_t size,
            void *file
        );

        bool seek(
            void *file,
            long offset,
            int origin
        );

        std::size_t tell(void *file);
        std::size_t length(void *file);
    }
}

#endif
