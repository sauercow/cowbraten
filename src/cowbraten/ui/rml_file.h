#ifndef COWBRATEN_UI_RML_FILE_H
#define COWBRATEN_UI_RML_FILE_H

#include <RmlUi/Core/FileInterface.h>

namespace cowui
{
    class RmlFileInterface final : public Rml::FileInterface
    {
    public:
        Rml::FileHandle Open(
            const Rml::String &path) override;

        void Close(
            Rml::FileHandle file) override;

        size_t Read(
            void *buffer,
            size_t size,
            Rml::FileHandle file) override;

        bool Seek(
            Rml::FileHandle file,
            long offset,
            int origin) override;

        size_t Tell(
            Rml::FileHandle file) override;

        size_t Length(
            Rml::FileHandle file) override;
    };
}

#endif
