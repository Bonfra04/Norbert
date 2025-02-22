#include "encoding/utf8.hpp"
using namespace Norbert::Infra;

namespace Norbert::Encoding
{
    std::shared_ptr<UTF8_encoding> UTF8 = std::make_shared<UTF8_encoding>();

    decoder::DecoderResult UTF8_decoder::handler(io_queue<byte>& ioQueue, const io_queue<byte>::ItemType& item)
    {
        if (std::holds_alternative<EndOfQueueType>(item))
            if (this->bytesNeeded != 0)
            {
                this->bytesNeeded = 0;
                return DecoderResultType::Error;
            }
            else
                return DecoderResultType::Finished;

        byte b = std::get<byte>(item);
        if (this->bytesNeeded == 0)
        {
            switch (b)
            {
                case 0x00 ... 0x7F:
                {
                    return { DecoderResultType::Items, { code_point(b) } };
                }
                case 0xC2 ... 0xDF:
                {
                    this->bytesNeeded = 1;
                    this->codepoint = 0x1F;
                    break;
                }
                case 0xE0 ... 0xEF:
                {
                    if (b == 0xE0)
                        this->lowerBoundary = 0xA0;
                    if (b == 0xED)
                        this->upperBoundary = 0x9F;
                    this->bytesNeeded = 2; 
                    this->codepoint = 0x0F;
                    break;
                }
                case 0xF0 ... 0xF4:
                {
                    if (b == 0xF0)
                        this->lowerBoundary = 0x90;
                    if (b == 0xF4)
                        this->upperBoundary = 0x8F;
                    this->bytesNeeded = 3;
                    this->codepoint = 0x07;
                    break;
                }
                default:
                {
                    return DecoderResultType::Error;
                }
            }
            return DecoderResultType::Continue;
        }

        if ((b < this->lowerBoundary) || (b > this->upperBoundary))
        {
            this->codepoint = this->bytesNeeded = this->bytesSeen = 0;
            this->lowerBoundary = 0x80;
            this->upperBoundary = 0xBF;
            ioQueue.restore(b);
            return DecoderResultType::Error;
        }

        this->lowerBoundary = 0x80;
        this->upperBoundary = 0xBF;
        this->codepoint = (this->codepoint << 6) | (b & 0x3F);
        
        this->bytesSeen++;
        if(this->bytesSeen != this->bytesNeeded)
            return DecoderResultType::Continue;

        code_point codepoint = this->codepoint;
        this->codepoint = this->bytesNeeded = this->bytesSeen = 0;

        return { DecoderResultType::Items, { codepoint } };
    }
}
