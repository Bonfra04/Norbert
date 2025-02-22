#pragma once

#include <vector>

#include "infra/byte.hpp"
#include "infra/code_point.hpp"
#include "infra/list.hpp"

namespace Norbert::Infra
{
    class string;

    class byte_sequence
    {
    public:
        byte_sequence() = default;
        inline byte_sequence(std::initializer_list<byte> init) : bytes(init) {}
        inline byte_sequence(list<byte> init) : bytes(init.begin(), init.end()) {}
        template <typename InputIt>
        inline byte_sequence(InputIt first, InputIt last) : bytes(first, last) {}
        inline byte_sequence(const char* str) { for(const char* c = str; *c != '\0'; c++) this->bytes.push_back(*c); }

        inline size_t length() const { return this->bytes.size(); }
        byte_sequence byteLowercase() const;
        byte_sequence byteUppercase() const;
        inline bool byteCaseInsensitveMatch(const byte_sequence& B) { return this->byteLowercase() == B.byteLowercase(); }
        bool isPrefix(const byte_sequence& input) const;
        bool isByteLessThan(const byte_sequence& b) const;
        string isomorphicDecode() const;

    public:
        class iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = byte;
            using pointer = byte*;
            using const_pointer = const byte*;
            using reference = byte&;

            iterator(pointer ptr) : m_ptr(ptr) {}
            iterator(const_pointer ptr) : m_ptr(const_cast<pointer>(ptr)) {}

            reference operator*() const { return *m_ptr; }
            pointer operator->() { return m_ptr; }
            iterator& operator++() { m_ptr++; return *this; }
            iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
            friend bool operator==(const iterator& a, const iterator& b) { return a.m_ptr == b.m_ptr; }
            friend bool operator!=(const iterator& a, const iterator& b) { return a.m_ptr != b.m_ptr; }

        private:
            pointer m_ptr;
        };

        iterator begin() { return iterator(&bytes[0]); }
        iterator end() { return iterator(&bytes[0] + bytes.size()); }
        const iterator begin() const { return iterator(&bytes[0]); }
        const iterator end() const { return iterator(&bytes[0] + bytes.size()); }

    public:
        inline bool operator==(const byte_sequence& other) const { return this->bytes == other.bytes; }
        inline byte& operator[](size_t index) { return bytes[index]; }
        inline const byte& operator[](size_t index) const { return bytes[index]; }

    public:
        friend std::ostream& operator<<(std::ostream &strm, const byte_sequence &arg);
        
    private:
        std::vector<byte> bytes;
    };
}
