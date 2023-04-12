#pragma once
#include <queue>

namespace moonlight
{

class InputBuffer
{
public:

    InputBuffer(std::size_t buffer_size) 
        : m_queue_size(buffer_size)
    {}

    void push(uint8_t val)
    {
        if (m_queue.size() + 1 <= m_queue_size)
        {
            m_queue.push(val);
        }
    }

    void pop()
    {
        m_queue.pop();
    }

    uint8_t front()
    {
        return m_queue.front();
    }

    std::size_t size() const
    {
        return m_queue.size();
    }

private:

    std::queue<uint8_t> m_queue;
    std::size_t m_queue_size;
};

}