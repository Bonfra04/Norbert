#include "encoding/legacy_hooks.hpp"
#include "encoding/utf8.hpp"
#include "encoding/utf16be.hpp"
#include "encoding/utf16le.hpp"
using namespace Norbert::Infra;

namespace Norbert::Encoding
{
    std::shared_ptr<io_queue<code_point>> decodeIoQueue(io_queue<byte>& ioQueue, std::shared_ptr<encoding> encoding, std::shared_ptr<io_queue<code_point>> output)
    {
        std::shared_ptr<Encoding::encoding> BOMEncoding = BOMSniff(ioQueue);
        if (BOMEncoding != nullptr)
        {
            encoding = BOMEncoding;
            if (BOMEncoding == UTF8)
                ioQueue.read(3);
            else
                ioQueue.read(2);
        }

        std::shared_ptr<decoder> decoder = encoding->createDecoder();

        if (output == nullptr)
            output = std::make_shared<io_queue<code_point>>();
        decoder->processQueue(ioQueue, *output, decoder::error_mode::replacement);
        return output;
    }

    std::shared_ptr<encoding> BOMSniff(io_queue<byte>& ioQueue)
    {
        byte_sequence BOM = ioQueue.peek(3);
        
        if(byte_sequence{ 0xEF, 0xBB, 0xBF }.isPrefix(BOM))
            return UTF8;
        if(byte_sequence{ 0xFE, 0xFF }.isPrefix(BOM))
            return UTF16BE;
        if(byte_sequence{ 0xFF, 0xFE }.isPrefix(BOM))
            return UTF16LE;

        return nullptr;
    }
}
