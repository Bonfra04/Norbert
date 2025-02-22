#include "css/token_stream.hpp"

namespace Norbert::CSS::Definitions
{
    const Token& TokenStream::nextToken() const
    {
        if(this->tokens.size() > this->index)
            return this->tokens[this->index];
        static const Token eof = TokenType::EOF;
        return eof;
    }

    const Token& TokenStream::consumeToken()
    {
        const Token& token = this->nextToken();
        this->index++;
        return token;
    }

    void TokenStream::discardWhitespace()
    {
        while(this->tokens.size() > this->index && this->nextToken().type == TokenType::Whitespace)
            this->index++;
    }

    // void TokenStream::process(std::function<std::optional<T>(const Token&)> callback)
    // {
    //     while(true)
    //     {
    //         std::any result = callback(this->nextToken());
    //     }
    // }
}
