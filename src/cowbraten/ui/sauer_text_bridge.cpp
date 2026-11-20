#include "sauer_text_bridge.h"

#include <cstring>
#include <vector>

using uchar = unsigned char;

extern std::size_t encodeutf8(
    uchar *destination,
    std::size_t destination_length,
    const uchar *source,
    std::size_t source_length,
    std::size_t *carry);

extern std::size_t decodeutf8(
    uchar *destination,
    std::size_t destination_length,
    const uchar *source,
    std::size_t source_length,
    std::size_t *carry);

namespace cowui
{
    namespace sauertext
    {
        std::string utf8(const char *text, std::size_t length)
        {
            if(!text || !length) return std::string();

            std::string result;
            result.reserve(length * 2);

            // encodeutf8() deliberately removes Sauer color escapes for file
            // output. RmlUi's console markup still needs them, so convert each
            // text segment while carrying the two-byte escapes through.
            std::size_t segment_begin = 0;
            while(segment_begin < length)
            {
                std::size_t segment_end = segment_begin;
                while(segment_end < length && text[segment_end] != '\f')
                    ++segment_end;

                const std::size_t segment_length =
                    segment_end - segment_begin;
                if(segment_length)
                {
                    // Every character in Sauer's accepted legacy repertoire
                    // needs no more than two UTF-8 bytes.
                    std::vector<uchar> converted(segment_length * 2);
                    const std::size_t converted_length = encodeutf8(
                        converted.data(),
                        converted.size(),
                        reinterpret_cast<const uchar *>(
                            text + segment_begin),
                        segment_length,
                        nullptr
                    );
                    result.append(
                        reinterpret_cast<const char *>(converted.data()),
                        converted_length
                    );
                }

                if(segment_end >= length) break;
                result.push_back(text[segment_end++]);
                if(segment_end < length)
                    result.push_back(text[segment_end++]);
                segment_begin = segment_end;
            }
            return result;
        }

        std::string utf8(const char *text)
        {
            return text ? utf8(text, std::strlen(text)) : std::string();
        }

        std::string cube(const char *text, std::size_t length)
        {
            if(!text || !length) return std::string();

            // A decoded Cube character occupies one byte and can never make
            // the result larger than its UTF-8 source.
            std::vector<uchar> converted(length);
            const std::size_t converted_length = decodeutf8(
                converted.data(),
                converted.size(),
                reinterpret_cast<const uchar *>(text),
                length,
                nullptr
            );
            return std::string(
                reinterpret_cast<const char *>(converted.data()),
                converted_length
            );
        }

        std::string cube(const char *text)
        {
            return text ? cube(text, std::strlen(text)) : std::string();
        }
    }
}
