#pragma once

#include "ioqueue.hpp"
#include "encodings.hpp"

namespace Norbert::Encoding
{
    std::shared_ptr<io_queue<Infra::code_point>> decodeIoQueue(io_queue<Infra::byte>& ioQueue, std::shared_ptr<encoding> encoding, std::shared_ptr<io_queue<Infra::code_point>> output = nullptr);
    std::shared_ptr<encoding> BOMSniff(io_queue<Infra::byte>& ioQueue);
}
