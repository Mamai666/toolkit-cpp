#ifndef STDDEQUEEXT_H
#define STDDEQUEEXT_H

#include <cstring>
#include <inttypes.h>
#include <iostream>
#include <string>
#include <mutex>
#include <boost/circular_buffer.hpp>

// Функция для копирования данных типа uint8_t*
static uint8_t* copy_uint8_t(const uint8_t* src, size_t size) {
    uint8_t* dest = new uint8_t[size];
    std::memcpy(dest, src, size);
    return dest;
}

template<typename T>
class StdDequeExt
{
public:
    StdDequeExt(uint16_t maxCapacity) : m_deq(maxCapacity)
    {
    }

    ~StdDequeExt() {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        m_deq.clear();
    }

    /**
     * @brief pushBack - положить елемент в конец очереди
     * @param el - элемент
     */
    void pushBack(T el) {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        m_deq.push_back(el);
    }


//    void pushBackU8Ptr(uint8_t* ptrEl, size_t sizeEl) {
//        std::lock_guard<std::mutex> lgMtx(m_mtx);
//        m_deq.push_back(new uint8_t[sizeEl]);
//        size_t idx = m_deq.size() -1;
//        memcpy(&m_deq[idx][])
//    }

    /**
     * @brief getFront - взять 1ый элемент (слева)
     * @return - копия элемента
     */
    T getFront() {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        T el = m_deq.front();
        return el;
    }

    /**
     * @brief getBack - взять последний элемент (справа)
     * @return - копия элемента
     */
    T getBack() {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        T el = m_deq.back();
        return el;
    }

    /**
     * @brief takeFront - взять 1ый элемент с его удалением
     * @return - копия элемента
     */
    T takeFront() { // Взять элемент из начала с его удалением
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        T el = m_deq.front();
        if(m_deq.size() > 0)
            m_deq.pop_front();
        return el;
    }

    /**
     * @brief takeBack - взять последний элемент с его удалением
     * @return - копия элемента
     */
    T takeBack() { // Взять элемент из конца с его удалением
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        T el = m_deq.back();
        if(m_deq.size() > 0)
            m_deq.pop_back();
        return el;
    }

    /**
     * @brief removeFront - удалить 1ый элемент
     */
    void removeFront() {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        if(m_deq.size() > 0)
            m_deq.pop_front();
    }

    /**
     * @brief removeBack - удалить последний элемент
     */
    void removeBack() {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        if(m_deq.size() > 0)
            m_deq.pop_back();
    }

    /**
     * @brief removeAt - удалить элемент по индексу
     * @param i - нужный индекс
     */
    void removeAt(const size_t i) {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        if(m_deq.size() > 0)
            m_deq.erase(m_deq.begin()+i);
    }

    /**
     * @brief getAt - получить элемент по индексу
     * @param i - нужный индекс
     * @return
     */
    T getAt(const size_t i) {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        T el = m_deq.at(i);
        return el;
    }

    /**
     * @brief size - узнать текущий заполненный размер очереди
     * @return
     */
    size_t size() const
    {
        return m_deq.size();
    }

    /**
     * @brief getCapacity - узнать ёмкость (макс.возможный размер) очереди
     * @return
     */
    uint16_t getCapacity() const
    {
        return m_deq.capacity();
    }

    /**
     * @brief setCapacity - задать новую ёмкость очереди
     * @param newCapacity - новая емкость
     */
    void setCapacity(uint16_t newCapacity)
    {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        m_deq.set_capacity(newCapacity);
    }

private:
    boost::circular_buffer<T> m_deq;
    std::mutex m_mtx;
};


#endif // STDDEQUEEXT_H
