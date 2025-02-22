#pragma once

#include "infra/infra.hpp"
#include "css/tokenizer.hpp"

// #include <any>

namespace Norbert::CSS::Definitions
{
    class TokenStream
    {
    public:
        inline TokenStream(const Infra::list<Token>& tokens) : tokens(tokens) {};

        const Norbert::CSS::Token& nextToken() const;
        inline bool empty() const { return this->nextToken().type == CSS::TokenType::EOF; }
        const Norbert::CSS::Token& consumeToken();
        inline void discardToken() { if(!this->empty()) this->index++; }
        inline void mark() { this->markedIndexes.push(this->index); }
        inline void restoreMark() { this->index = this->markedIndexes.pop().value(); }
        inline void discardMark() { this->markedIndexes.pop(); }
        void discardWhitespace();

        template <typename T>
        T process(std::function<std::optional<T>(const Token&)> callback)
        {
            while(true)
            {
                std::optional<T> result = callback(this->nextToken());
                if(!result.has_value())
                    return result.value();
            }
        }

    private:
        Infra::list<Token> tokens;
        size_t index = 0;
        Infra::stack<size_t> markedIndexes;
    };
}

