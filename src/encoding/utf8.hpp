#pragma once

#include "encoding/encodings.hpp"

namespace Norbert::Encoding
{
    class UTF8_decoder : public decoder {
    public:
        virtual DecoderResult handler(io_queue<Infra::byte>& ioQueue, const io_queue<Infra::byte>::ItemType& item);
    
    private:
        size_t bytesNeeded = 0;
        size_t bytesSeen = 0;
        Infra::code_point codepoint;
        Infra::code_point lowerBoundary;
        Infra::code_point upperBoundary;
    };

    class UTF8_encoding : public encoding
    {
    public:
        inline UTF8_encoding() : encoding("UTF-8", {
            "unicode-1-1-utf-8",
            "unicode11utf8",
            "unicode20utf8",
            "utf-8",
            "utf8",
            "x-unicode20utf8"
        }) {}

    public:
        inline std::unique_ptr<decoder> createDecoder() const override { return std::make_unique<UTF8_decoder>(); }
    };

    extern std::shared_ptr<UTF8_encoding> UTF8;
}
