#pragma once

#include <array>
#include <list>
#include <queue>
#include <sys/socket.h>

constexpr auto pool_size = 100;
constexpr auto buffer_size = 2048;

struct memory_pool {

    enum buffer_state
    {
        buffer_free = 0,
        buffer_used,
        buffer_populated,
    };

    struct buf {
        std::array<char, buffer_size> data;
        uint32_t byte_used;
        char * pointer(){
            return data.data();
        }
        size_t size(){
            return data.size();
        }
    };
    
    std::array<std::pair<buffer_state, buf>, pool_size> pool;
    typedef std::pair<buf*, sockaddr> buf_with_addr_t;
    std::queue<buf_with_addr_t> order_queue;

    memory_pool() = default;
    buf* get() {
        for (auto it = pool.begin(); it != pool.end(); ++it) {
            if (it->first == buffer_free) {
                it->first = buffer_used;
                return &(it->second);
            }
        }
        return nullptr;
    }

    void commit(buf* _buf, sockaddr& addr){
        order_queue.push({_buf, addr});
    }
    
    buf_with_addr_t pop() {
        if (order_queue.empty())
            return buf_with_addr_t{nullptr,{}};
        auto foo = order_queue.front();
        order_queue.pop();
        return foo;
    }

    void free(buf* _buf) {
        for (auto it = pool.begin(); it != pool.end(); ++it){
            if ((it->first == buffer_used || it->first == buffer_populated) && &(it->second) == _buf) {
                it->first = buffer_free;
            }
        }
    }
};