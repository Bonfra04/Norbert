#include "encoding/encodings.hpp"
#include "encoding/utf8.hpp"
#include "encoding/utf16be.hpp"
#include "encoding/utf16le.hpp"
using namespace Norbert::Infra;

namespace Norbert::Encoding
{
    decoder::DecoderResult decoder::processQueue(io_queue<byte>& input, io_queue<code_point>& output, error_mode mode)
    {
        while (true)
        {
            DecoderResult result = this->processItem(input.read(), input, output, mode);
            if (result.type != DecoderResultType::Continue)
                return result;
        }
    }

    decoder::DecoderResult decoder::processItem(const io_queue<byte>::ItemType& item, io_queue<byte>& input, io_queue<code_point>& output, error_mode mode)
    {
        if (mode == error_mode::html)
            throw std::invalid_argument("HTML mode is not valid for decoders");
    
        DecoderResult result = this->handler(input, item);

        if(result.type == DecoderResultType::Finished)
        {
            output.push(EndOfQueue);
            return result;
        }
        else if(result.type == DecoderResultType::Items)
        {
            for (const code_point& item : result.items.value())
            {
                if (item.isSurrogate())
                    throw std::runtime_error("Result contains surrogate code points");
                output.push(item);
            }
        }
        else if(result.type == DecoderResultType::Error)
        {
            switch(mode)
            {
                case error_mode::replacement:
                {
                    output.push(0xFFFD);
                    break;
                }
                case error_mode::html:
                {
                    code_point codepoint = result.items.value()[0];
                    output.push(0x26);
                    output.push(0x23);

                    for (char digit : std::to_string(codepoint))
                        output.push(digit);

                    output.push(0x3B);
                    break;
                }
                case error_mode::fatal:
                {
                    return result;
                }
            }
        }

        return DecoderResult { DecoderResultType::Continue };
    }

    std::shared_ptr<encoding> getEncoding(const string& label)
    {
        const string normalized_label = label.stripLeadingAndTrailingASCIIWhitespace().ASCIILowercase();

        if (UTF8->labels.contains(normalized_label))
            return UTF8;

        if (UTF16BE->labels.contains(normalized_label))
            return UTF16BE;

        if (UTF16LE->labels.contains(normalized_label))
            return UTF16LE;

        return nullptr;
    }
}
