#pragma once

#include <optional>

#include "encoding/encoding.hpp"

namespace Norbert::CSS::Definitions
{
    std::shared_ptr<Encoding::encoding> determineFallbackEncoding(std::optional<Infra::string> label, Encoding::io_queue<Infra::byte>& byteStream);
    std::shared_ptr<Encoding::io_queue<Infra::code_point>> decode(Encoding::io_queue<Infra::byte>& byteStream);
    std::shared_ptr<Encoding::io_queue<Infra::code_point>> filterCodePoints(Encoding::io_queue<Infra::code_point>& decodedStream);
}
