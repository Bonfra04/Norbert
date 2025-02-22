#include "css/input_stream.hpp"
using namespace Norbert::Infra;
using namespace Norbert::Encoding;

namespace Norbert::CSS::Definitions
{
    std::shared_ptr<io_queue<code_point>> decode(io_queue<byte>& byteStream)
    {
        std::shared_ptr<encoding> fallback = determineFallbackEncoding(std::nullopt, byteStream);
        return decodeIoQueue(byteStream, fallback);
    }

    std::shared_ptr<encoding> determineFallbackEncoding(std::optional<string> label, io_queue<byte>& byteStream)
    {
        if(label.has_value())
        {
            std::shared_ptr<encoding> encoding = getEncoding(label.value());
            if(encoding != nullptr)
                return encoding;
        }
        
        byte_sequence peeked = byteStream.peek(1024);
        byte_sequence prefix = "@charset \"";
       
        if(prefix.isPrefix(peeked))
        {
            list<code_point> charset;
            size_t i;
            for(i = prefix.length(); i < 1022; i++)
            {
                byte p = peeked[i];
                if (p == '"' || p >= 0x80)
                    break;
                charset.append(code_point(peeked[i]));
            }

            if(peeked[i] == '"' && peeked[i + 1] == ';')
            {
                std::shared_ptr<encoding> encoding = getEncoding(charset);
                if (encoding == UTF16BE || encoding == UTF16LE)
                    encoding = UTF8;

                if (encoding != nullptr)
                {
                    byteStream.read(prefix.length() + charset.size() + 2);
                    return encoding;
                }
            }
        }

        // TODO: Otherwise, if an environment encoding is provided by the referring document, return it.
        
        return UTF8;
    }

    std::shared_ptr<io_queue<code_point>> filterCodePoints(io_queue<code_point>& decodedStream)
    {
        std::shared_ptr<io_queue<code_point>> filtered = std::make_shared<io_queue<code_point>>();
        
        bool lastWasCR = false;
        while(true)
        {
            io_queue<code_point>::ItemType cp = decodedStream.read();
            if (std::holds_alternative<EndOfQueueType>(cp))
            {
                filtered->push(EndOfQueue);
                break;
            }
            code_point codepoint = std::get<code_point>(cp);

            if(lastWasCR && codepoint == 0x000A)
            {
                lastWasCR = false;
                continue;
            }
            lastWasCR = codepoint == 0x000D;

            if(codepoint == 0x000D || codepoint == 0x000C)
                codepoint = 0x000A;
            
            if(codepoint == 0x0000 || codepoint.isSurrogate())
            codepoint = 0xFFFD;

            filtered->push(codepoint);
        }

        return filtered;
    }
}
