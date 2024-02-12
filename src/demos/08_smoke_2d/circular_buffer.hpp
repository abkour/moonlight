#pragma once
#include <memory>

namespace moonlight
{

template<typename T>
class CircularBuffer
{
public:

    CircularBuffer()
        : m_data(nullptr)
        , m_cursor(0)
        , m_capacity(0)
    {}

    CircularBuffer(std::size_t initial_size)
    {
        m_cursor = 0;
        m_capacity = initial_size;

        m_data = new T[initial_size];
    }

    CircularBuffer(const CircularBuffer& other)
    {
        m_cursor = other.m_cursor;
        m_capacity = other.m_capacity;

        m_data = new T[m_capacity];
        std::memcpy(m_data, other.m_data, sizeof(T) * m_capacity);
    }

    CircularBuffer(CircularBuffer&& other)
    {
        m_cursor = other.m_cursor;
        m_capacity = other.m_capacity;

        m_data = other.m_data;
        other.m_data = nullptr;
    }

    ~CircularBuffer()
    {
        if (m_data != nullptr)
        {
            delete m_data;
        }
    }

    CircularBuffer& operator=(const CircularBuffer& other)
    {
        m_cursor = other.m_cursor;
        m_capacity = other.m_capacity;

        m_data = new T[m_capacity];
        std::memcpy(m_data, other.m_data, sizeof(T) * m_capacity);

        return *this;
    }

    CircularBuffer& operator=(CircularBuffer&& other)
    {
        m_cursor = other.m_cursor;
        m_capacity = other.m_capacity;

        m_data = other.m_data;
        other.m_data = nullptr;

        return *this;
    }

    T& operator[](const std::size_t index)
    {
        return m_data[index];
    }

    T operator[](const std::size_t index) const
    {
        return m_data[index];
    }

    T* get_address() { return m_data; }

    void push(T element)
    {
        if (m_cursor < m_capacity)
        {
            m_data[m_cursor] = element;
            ++m_cursor;
        }
        else
        {
            m_data[0] = element;
            m_cursor = 0;
        }
    }

    void resize(std::size_t new_size)
    {
        if (new_size <= m_capacity)
            return;

        T* temp = m_data;
        
        m_data = new T[new_size];

        std::memcpy(m_data, temp, sizeof(T) * m_capacity);
        m_capacity = new_size;

        delete temp;
    }

private:
    
    T* m_data;
    std::size_t m_cursor;
    std::size_t m_capacity;
};

}