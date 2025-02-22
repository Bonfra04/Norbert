#pragma once

#include <optional>

#include "infra/list.hpp"

namespace Norbert::Infra
{
    template <typename T>
    class stack : public list<T>
    {
    public:
        inline void push(const T& value) { this->append(value); }
        
        std::optional<T> pop()
        {
            if(!this->isEmpty())
            {
                T value = this->items[this->size() - 1];
                this->remove(this->size() - 1);
                return value;
            }
            return std::nullopt;
        }

        std::optional<T> peek()
        {
            if(!this->isEmpty())
                return this->items[this->size() - 1];
            return std::nullopt;
        }
    };
}
