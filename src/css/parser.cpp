#include "css/parser.hpp"

#include "encoding/encoding.hpp"
#include "css/input_stream.hpp"

using namespace Norbert::Infra;
using namespace Norbert::Encoding;
using namespace Norbert::CSS::Definitions;

#include <iostream>

namespace Norbert::CSS
{
    Definitions::TokenStream Parser::normalizeIntoTokenStream(InputType input)
    {
        if(std::holds_alternative<TokenStream>(input))
            return std::get<TokenStream>(input);
        else if(std::holds_alternative<list<Token>>(input))
            return TokenStream(std::get<list<Token>>(input));
        else if(std::holds_alternative<string>(input))
        {
            io_queue<code_point> ioQueue(std::get<string>(input));
            std::shared_ptr<io_queue<code_point>> input_stream = filterCodePoints(ioQueue);
            return Tokenizer(*input_stream).tokenize();
        }
        else
            throw std::invalid_argument("Invalid input type");
    }

    list<rule> Parser::parseStylesheetContents(InputType input)
    {
        TokenStream tokenStream = Parser::normalizeIntoTokenStream(input);        
        return Parser(tokenStream).consumeStylesheetContent();
    }

    list<rule> Parser::consumeStylesheetContent()
    {
        list<rule> rules;

        return this->tokenStream.process<list<rule>>([this, &rules](const Token& token)
        {
            switch(token.type)
            {
                case TokenType::Whitespace:
                {
                    this->tokenStream.discardToken();
                    break;
                }
                case TokenType::EOF:
                {
                    return rules;
                }
                case TokenType::CDO:
                case TokenType::CDC:
                {
                    this->tokenStream.discardToken();
                    break;
                }
                case TokenType::AtKeyword:
                {
                    rules.append(this->consumeAtRule());
                }
                default:
                {
                    std::cout << "Token: " << int(token.type) << std::endl;
                    this->tokenStream.consumeToken();
                }
            }
        });
    }

    at_rule Parser::consumeAtRule()
    {
        Token token = this->tokenStream.consumeToken();
        at_rule rule(std::get<string>(token.value));
        return rule;

        // this->tokenStream.process([this, &rule](const Token& token)
        // {
        //     switch(token.type)
        //     {
        //         case TokenType::Semicolon:
        //         case TokenType::EOF:
        //         {
        //             this->tokenStream.discardToken();
        //         }
        //     }
        // });
    }
}
