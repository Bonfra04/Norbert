#pragma once

#include "infra/list.hpp"

namespace Norbert::Infra
{
    template <typename T>
    class ordered_set : public list<T>
    {
    public:
        static ordered_set<size_t> inclusiveRange(size_t n, size_t m) {
            ordered_set<size_t> result;
            for (size_t i = n; i <= m; i++)
                result.append(i);
            return result;
        } 

        static ordered_set<size_t> exclusiveRange(size_t n, size_t m) {
            ordered_set<size_t> result;
            for (size_t i = n; i < m; i++)
                result.append(i);
            return result;
        }

    public:
        ordered_set() = default;
        inline ordered_set(std::initializer_list<T> init_list) : ordered_set(list<T>(init_list)) {}
        template <typename InputIt>
        inline ordered_set(InputIt first, InputIt last) : ordered_set(list<T>(first, last)) {}

        ordered_set(const list<T>& list)
            : list<T>({})
        {
            list.forEach([this](const T& item) {
                this->append(item);
            });
        }

        inline void append(const T& value)
        {
            if (!this->contains(value))
                list<T>::append(value);
        }

        inline void extend(const list<T>& other)
        {
            other.forEach([this](const T& item) {
                this->append(item);
            });
        }

        inline void prepend(const T& value)
        {
            if (!this->contains(value))
                list<T>::prepend(value);
        }

        void replace(const T& item, const T& replacement)
        {
            if (this->contains(item) || this->contains(replacement))
            {
                ordered_set<T> new_set;
                bool replaced = false;
                this->forEach([&](const T& current) {
                    if ((current == item || current == replacement) && !replaced)
                    {
                        new_set.append(replacement);
                        replaced = true;
                    }
                    else if (current != item && current != replacement)
                        new_set.append(current);
                });
                this->empty();
                this->extend(new_set);
            }
        }

        bool isSuperset(const ordered_set<T>& other) const
        {
            for (const T& item : other)
                if (!this->contains(item))
                    return false;
            return true;
        }

        inline bool isSubset(const ordered_set<T>& other) const
        {
            return other.isSuperset(*this);
        }

        inline bool operator==(const ordered_set<T>& other) const
        {
            return this->isSubset(other) && this.isSuperset(other);
        }

        ordered_set intersectionWith(const ordered_set<T>& other) const
        {
            ordered_set<T> result;
            this->forEach([&](const T& item) {
                if (other.contains(item))
                    result.append(item);
            });
            return result;
        }

        ordered_set unionWith(const ordered_set<T>& other) const
        {
            ordered_set<T> result = *this;
            for (const T& item : other)
                result.append(item);
            return result;
        }

        ordered_set differenceWith(const ordered_set<T>& other) const
        {
            ordered_set<T> result;
            this->forEach([&](const T& item) {
                if (!other.contains(item))
                    result.append(item);
            });
            return result;
        }
    };

    template <typename T>
    inline ordered_set<size_t> list<T>::getIndices() { return ordered_set<size_t>::exclusiveRange(0, this->size()); }
}
