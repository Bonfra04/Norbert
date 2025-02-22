#pragma once

#include <cstdint>
#include <variant>
#include <mutex>
#include <condition_variable>

#include "infra/infra.hpp"

namespace Norbert::Encoding
{    
    static constexpr struct EndOfQueueType {
        bool operator==(const EndOfQueueType&) const { return true; }
    } EndOfQueue;
    
    template <typename T>
    class io_queue : public Infra::list<std::variant<T, EndOfQueueType>>
    {
        static_assert(std::is_same_v<T, Infra::byte> || std::is_same_v<T,Infra::code_point>, "T must be either byte or code_point");
    
    public:
        using ItemType = std::variant<T, EndOfQueueType>;
    
    public:
        io_queue() = default;
        io_queue(std::initializer_list<ItemType> initList)
        {
            for (const auto& item : initList)
                this->push(item);
        }

        ItemType read()
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->cv.wait(lock, [this] { return this->size() >= 1; });

            if(std::holds_alternative<EndOfQueueType>(this->items[0]))
                return EndOfQueue;
            ItemType result = this->items[0];
            this->remove(0);
            return result;
        }

        Infra::list<T> read(size_t number)
        {
            using namespace Infra;
            
            list<T> readItems;
            for(size_t i = 0; i < number; i++)
            {
                ItemType item = this->read();
                if(std::holds_alternative<EndOfQueueType>(item))
                    break;
                readItems.append(std::get<T>(item));
            }
            return readItems;
        }

        Infra::list<T> peek(size_t number)
        {
            using namespace Infra;

            std::unique_lock<std::mutex> lock(this->mutex);
            this->cv.wait(lock, [this, number] { return this->size() >= number || this->contains(EndOfQueue); });

            list<T> prefix;
            ordered_set<size_t>::exclusiveRange(0, number).forEach([&](size_t i) {
                if(std::holds_alternative<EndOfQueueType>(this->items[i]))
                    return true;
                prefix.append(std::get<T>(this->items[i]));
                return false;
            });
            return prefix;
        }

        void push(const ItemType& item)
        {
            std::lock_guard<std::mutex> lock(this->mutex);
            
            if(this->size() > 0 && std::holds_alternative<EndOfQueueType>(this->items[this->size() - 1]))
                if (std::holds_alternative<EndOfQueueType>(item))
                    return;
                else
                    this->insert(this->size() - 1, item);
            else
                this->append(item);

            this->cv.notify_all();
        }

        void push(const Infra::list<ItemType>& items)
        {
            std::lock_guard<std::mutex> lock(this->mutex);
            items.forEach([&](const ItemType& item) { this->push(item); });
        }

        void restore(const ItemType& item)
        {
            if (std::holds_alternative<EndOfQueueType>(item))
                return;
            std::lock_guard<std::mutex> lock(this->mutex);
            this->prepend(item);
        }

        void restore(const Infra::list<ItemType>& items)
        {
            std::lock_guard<std::mutex> lock(this->mutex);
            this->items.insert(this->items.begin(), items.items.begin(), items.items.end());
        }

        template <typename U>
        U convertTo()
        {
            using namespace Infra;
            static_assert(std::is_same_v<U, list<T>> || std::is_same_v<U, byte_sequence> || std::is_same_v<U, string>, "U must be either list<T>, byte_sequence, or string");
            list<T> items;
            while(true)
            {
                ItemType item = this->read();
                if(std::holds_alternative<EndOfQueueType>(item))
                    break;
                items.append(std::get<T>(item));
            }
            U result = items;
            return result;
        }

        template <typename U>
        io_queue(const U input)
        {
            using namespace Infra;
            static_assert(std::is_same_v<U, list<T>> || std::is_same_v<U, byte_sequence> || std::is_same_v<U, string>, "U must be either list<T>, byte_sequence, or string");
            io_queue<T> result;
            for(const T& item : input)
                this->push(item);
            this->push(EndOfQueue);
        }

    private:
        std::mutex mutex;
        std::condition_variable cv;
    };
}
