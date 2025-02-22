#pragma once

#include "css/token_stream.hpp"
#include "infra/infra.hpp"

#include <variant>

namespace Norbert::CSS
{
    struct component_value;

    struct function
    {
        Infra::string name;
        Infra::list<component_value> value;
    };

    struct simple_block
    {
        Token associated_token;
        Infra::list<component_value> value;
    };

    struct component_value
    {
        std::variant<Token, function, simple_block> value;
    };

    struct declaration
    {
        Infra::string name;
        Infra::list<component_value> value;
        bool important = false;
        std::optional<Infra::string> original_text;
    };

    struct rule;

    struct at_rule
    {
        Infra::string name;
        Infra::list<component_value> prelude;
        Infra::list<declaration> declarations;
        Infra::list<rule> child_rules;
        inline at_rule(const Infra::string& name) : name(name) {};
    };

    struct qualified_rule
    {
        Infra::list<component_value> prelude;
        Infra::list<declaration> declarations;
        Infra::list<rule> child_rules;
    };

    struct rule
    {
        std::variant<at_rule, qualified_rule> value;
        inline rule(const at_rule& value) : value(value) {};
        inline rule(const qualified_rule& value) : value(value) {};
    };

    struct stylesheet
    {
        Infra::list<rule> rules;
    };

    class Parser
    {
    private:
        using InputType = std::variant<Definitions::TokenStream, Infra::list<Token>, Infra::string>;
        static Definitions::TokenStream normalizeIntoTokenStream(InputType input);

    public:
        static Infra::list<rule> parseStylesheetContents(InputType input);

    private:
        inline Parser(const Definitions::TokenStream& tokenStream) : tokenStream(tokenStream) {};
        
        Infra::list<rule> consumeStylesheetContent();
        at_rule consumeAtRule();

    private:
        Definitions::TokenStream tokenStream;
    };
}
