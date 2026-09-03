#include "rml_file.h"
#include "sauer_file_bridge.h"

namespace cowui
{
    Rml::FileHandle RmlFileInterface::Open(
        const Rml::String &path)
    {
        return reinterpret_cast<Rml::FileHandle>(
            sauerfile::open(path.c_str())
        );
    }

    void RmlFileInterface::Close(
        Rml::FileHandle file)
    {
        sauerfile::close(
            reinterpret_cast<void *>(file)
        );
    }

    size_t RmlFileInterface::Read(
        void *buffer,
        size_t size,
        Rml::FileHandle file)
    {
        return sauerfile::read(
            buffer,
            size,
            reinterpret_cast<void *>(file)
        );
    }

    bool RmlFileInterface::Seek(
        Rml::FileHandle file,
        long offset,
        int origin)
    {
        return sauerfile::seek(
            reinterpret_cast<void *>(file),
            offset,
            origin
        );
    }

    size_t RmlFileInterface::Tell(
        Rml::FileHandle file)
    {
        return sauerfile::tell(
            reinterpret_cast<void *>(file)
        );
    }

    size_t RmlFileInterface::Length(
        Rml::FileHandle file)
    {
        return sauerfile::length(
            reinterpret_cast<void *>(file)
        );
    }
}
