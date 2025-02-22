#pragma once

#include <optional>
#include <memory>

#include "infra/infra.hpp"
#include "encoding/ioqueue.hpp"

namespace Norbert::Encoding
{
    class decoder
    {
    public:
        enum class error_mode { replacement, fatal, html };
        enum class DecoderResultType { Finished, Items, Error, Continue };
        struct DecoderResult {
            DecoderResultType type;
            std::optional<Infra::list<Infra::code_point>> items = std::nullopt;

            DecoderResult(DecoderResultType type) : type(type) {}
            DecoderResult(DecoderResultType type, const Infra::list<Infra::code_point>& items) : type(type), items(items) {}
        };

    public:
        decoder() = default;

        DecoderResult processQueue(io_queue<Infra::byte>& input, io_queue<Infra::code_point>& output, error_mode mode);  
        DecoderResult processItem(const io_queue<Infra::byte>::ItemType& item, io_queue<Infra::byte>& input, io_queue<Infra::code_point>& output, error_mode mode);
    
        virtual DecoderResult handler(io_queue<Infra::byte>& ioQueue, const io_queue<Infra::byte>::ItemType& item) = 0;
    };

    class encoding
    {
    public:
        encoding(const Infra::string& name, const Infra::list<Infra::string>& labels) : name(name), labels(labels) {}
        virtual std::unique_ptr<decoder> createDecoder() const = 0;

    public:
        const Infra::string name;
        const Infra::list<Infra::string> labels;
    };

    std::shared_ptr<encoding> getEncoding(const Infra::string& label);
}
