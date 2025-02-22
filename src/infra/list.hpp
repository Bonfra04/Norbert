#pragma once

#include <vector>
#include <functional>
#include <algorithm>
#include <ostream>
#undef EOF

namespace Norbert::Infra
{
    template <typename T>
    class ordered_set;

    template <typename T>
    class list
    {
    public:
        list() = default;
        inline list(std::initializer_list<T> init) : items(init) {}
        template <typename InputIt>
        inline list(InputIt first, InputIt last) : items(first, last) {}

        inline void append(const T& value) { this->items.push_back(value); }
        inline void extend(const list<T>& other) { this->items.insert(this->items.end(), other.items.begin(), other.items.end()); }
        inline void prepend(const T& value) { this->items.insert(this->items.begin(), value); }

        void replace(const std::function<bool(const T&)>& condition, const T& replacement)
        {
            for (T& item : this->items)
                if (condition(item))
                    item = replacement;
        }
        
        inline void insert(size_t index, const T& value) { this->items.insert(this->items.begin() + index, value); }
        inline void remove(const std::function<bool(const T&)>& condition) { this->items.erase(std::remove_if(this->items.begin(), this->items.end(), condition), this->items.end()); }
        inline void remove(size_t index) { this->items.erase(this->items.begin() + index); }
        inline void empty() { this->items.clear(); }
        inline bool contains(const T& value) const { return std::find(this->items.begin(), this->items.end(), value) != this->items.end(); }
        inline size_t size() const { return this->items.size(); };
        inline bool isEmpty() const { return this->items.empty(); }

        inline ordered_set<size_t> getIndices();

        template <typename Func>
        void forEach(const Func& action)
        {
            for (T& item : this->items)
                if constexpr (std::is_same_v<std::invoke_result_t<Func, decltype(item)>, bool>)
                {
                    if (action(item))
                        break;
                }
                else
                    action(item);
        }

        list<T> clone()
        {
            list<T> result;
            for (const T& item : this->items)
                result.append(item);
            return result;
        }

        inline void sortInAscendingOrder(const std::function<bool(const T&, const T&)>& lessThanAlgo) { std::sort(this->items.begin(), this->items.end(), lessThanAlgo); }
        inline void sortInDescendingOrder(const std::function<bool(const T&, const T&)>& greaterThanAlgo) { std::sort(this->items.begin(), this->items.end(), greaterThanAlgo); }

        inline T& operator[](size_t index) { return this->items[index]; }
        inline const T& operator[](size_t index) const { return this->items[index]; }

        // Iterator definitions
        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;

        // Iterator methods
        inline iterator begin() { return items.begin(); }
        inline const_iterator begin() const { return items.begin(); }
        inline iterator end() { return items.end(); }
        inline const_iterator end() const { return items.end(); }

        friend std::ostream& operator<<(std::ostream &strm, const list<T> &arg)
        {
            strm << "« ";
            for (auto it = arg.items.begin(); it != arg.items.end(); ++it)
            {
                strm << *it;
                if (it + 1 != arg.items.end())
                    strm << ", ";
            }
            return strm << " »";
        }

    protected:
        std::vector<T> items;
    };
}
