#pragma once

#include "encoding/encodings.hpp"

namespace Norbert::Encoding
{
    class UTF16LE_encoding : public encoding {
    public:
        inline UTF16LE_encoding() : encoding("UTF-16LE", {
            "csunicode",
            "iso-10646-ucs-2",
            "ucs-2",
            "unicode",
            "unicodefeff",
            "utf-16",
            "utf-16le"
        }) {}
    
    public:
        inline std::unique_ptr<decoder> createDecoder() const override { return nullptr; }
    };

    extern std::shared_ptr<UTF16LE_encoding> UTF16LE;
}
