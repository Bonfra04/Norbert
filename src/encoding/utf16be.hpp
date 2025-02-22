#pragma once

#include "encoding/encodings.hpp"

namespace Norbert::Encoding
{
    class UTF16BE_encoding : public encoding {
    public:
        inline UTF16BE_encoding() : encoding("UTF-16BE", {
            "unicodefffe",
            "utf-16be"
        }) {}
    
    public:
        inline std::unique_ptr<decoder> createDecoder() const override { return nullptr; }
    };

    extern std::shared_ptr<UTF16BE_encoding> UTF16BE;    
}
