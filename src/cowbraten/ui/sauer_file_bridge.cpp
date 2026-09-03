#include "engine.h"

#include "sauer_file_bridge.h"

namespace cowui
{
    namespace sauerfile
    {
        void *open(const char *path)
        {
            if(!path || !path[0])
                return nullptr;

            return openfile(path, "rb");
        }

        void close(void *file)
        {
            if(!file)
                return;

            delete static_cast<stream *>(file);
        }

        std::size_t read(
            void *buffer,
            std::size_t size,
            void *file)
        {
            if(!file || !buffer || size == 0)
                return 0;

            return static_cast<stream *>(file)->read(
                buffer,
                size
            );
        }

        bool seek(
            void *file,
            long offset,
            int origin)
        {
            if(!file)
                return false;

            return static_cast<stream *>(file)->seek(
                static_cast<stream::offset>(offset),
                origin
            );
        }

        std::size_t tell(void *file)
        {
            if(!file)
                return 0;

            const stream::offset position =
                static_cast<stream *>(file)->tell();

            return position >= 0
                ? static_cast<std::size_t>(position)
                : 0;
        }

        std::size_t length(void *file)
        {
            if(!file)
                return 0;

            const stream::offset file_size =
                static_cast<stream *>(file)->size();

            return file_size >= 0
                ? static_cast<std::size_t>(file_size)
                : 0;
        }
    }
}
