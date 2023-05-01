#pragma once
 
#include <iostream>
#include <utility>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <cstdlib>
 
#include "array_ptr.h"
 
struct ReserveProxyObj {
    std::size_t value;
};
 
ReserveProxyObj Reserve(std::size_t capacity_to_reserve) {
    return {capacity_to_reserve};
}
 
 
template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;
 
    SimpleVector() noexcept = default;
    
    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        : SimpleVector(size, Type{})
    {
    }
 
    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : vector_(size)
        , size_(size)
        , capacity_(size)
    {
        std::fill(vector_.Get(), vector_.Get() + size, value);
    }
 
    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : vector_(init.size())
        , size_(init.size())
        , capacity_(init.size())
    {
        std::copy(init.begin(), init.end(), vector_.Get());
    }
    
    // Конструктор с резервированием
    explicit SimpleVector(ReserveProxyObj capacity)
            : vector_(capacity.value)
            , size_(0)
            , capacity_(capacity.value)
    {
    }
    
    // Конструктор копирования
    SimpleVector(const SimpleVector& other)
        : vector_(other.GetSize())
        , size_(other.GetSize())
        , capacity_(other.GetSize())
    {
        std::copy(other.begin(), other.end(), vector_.Get());
    }
    
    SimpleVector(SimpleVector&& other) noexcept
        : vector_(std::move(other.vector_))
        , size_(std::exchange(other.size_, 0))
        , capacity_(std::exchange(other.capacity_, 0))
    {
    }
    
    // Оператор присваивания
    SimpleVector& operator=(const SimpleVector& rhs) {
        if (*this != rhs) {
            auto tmp = rhs;
            swap(tmp);
        }
        return *this;
    }
      
    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        if (*this != rhs) {
            vector_.swap(rhs.vector_);
            size_ = std::exchange(rhs.size_, 0);
            capacity_ = std::exchange(rhs.capacity_, 0);
        }
        return *this;
    }
    
    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }
 
    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }
 
    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }
 
    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return vector_[index];
    }
 
    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return vector_[index];
    }
 
    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("out_of_range");
        }
        return vector_[index];
    }
 
    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("out_of_range");
        }
        return vector_[index];
    }
 
    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
        return;
    }
 
    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size <= capacity_) {
            if (new_size < size_) {
                size_ = new_size;
                return;
            }
            std::generate(begin() + size_, begin() + new_size, [](){ return Type{}; });
            size_ = new_size;
            return;
        }
        ArrayPtr<Type> tmp(new_size);
        std::move(vector_.Get(), vector_.Get() + size_, tmp.Get());
        std::generate(tmp.Get() + size_, tmp.Get() + new_size, [](){ return Type{}; });
        // я думала-думала, но не догадалась, как без цикла мувать :`-(
        // коллеги подсказали использовать generate
        vector_.swap(tmp);
        size_ = new_size;
        capacity_ = std::max(new_size, capacity_ * 2);
        return;
    }
    
    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
        return;
    }
    
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        Insert(cend(), item);
        return;
    }
    
    void PushBack(Type&& item) {
        Insert(cend(), std::move(item));
        return;
    }
    
    // использует copy
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= cbegin() && pos <= cend());
        size_t index = (pos == end() ? size_ : pos - begin());
        if (size_ == capacity_) {
            if (capacity_ == 0) {
                ArrayPtr<Type> tmp(1);
                vector_.swap(tmp);
                vector_[0] = value;
                size_ = 1;
                capacity_ = 1;
                return begin();
            }
            ArrayPtr<Type> tmp(capacity_ *2);
            std::copy(begin(), vector_.Get() + index, tmp.Get());
            tmp[index] = value;
            std::copy(vector_.Get() + index, end(), tmp.Get() + index + 1);
            vector_.swap(tmp);
            ++size_;
            capacity_ *= 2;
        }
        else {
            std::copy_backward(vector_.Get() + index, vector_.Get() + size_, vector_.Get() + size_ + 1);
            vector_[index] = value;
            ++size_;
        }
        return Iterator(vector_.Get() + index);
    }
    
    // использует move
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= cbegin() && pos <= cend());
        size_t index = (pos == end() ? size_ : pos - begin());
        if (size_ == capacity_) {
            if (capacity_ == 0) {
                ArrayPtr<Type> tmp(1);
                vector_.swap(tmp);
                vector_[0] = std::move(value);
                size_ = 1;
                capacity_ = 1;
                return begin();
            }
            ArrayPtr<Type> tmp(capacity_ * 2);
            std::move(begin(), vector_.Get() + index, tmp.Get());
            tmp[index] = std::move(value);
            std::move(vector_.Get() + index, end(), tmp.Get() + index + 1);
            vector_.swap(tmp);
            ++size_;
            capacity_ *= 2;
        }
        else {
            std::move_backward(vector_.Get() + index, vector_.Get() + size_, vector_.Get() + size_ + 1);
            vector_[index] = std::move(value);
            ++size_;
        }
        return Iterator(vector_.Get() + index);
    }
    
    // Удаляет элемент вектора в указанной позиции
    // Возвращает итератор на элемент, следующий за удалённым
    Iterator Erase(ConstIterator pos) {
        assert(!IsEmpty());
        size_t index = pos - begin();
        std::move(vector_.Get() + index + 1, end(), vector_.Get() + index);
        --size_;
        return Iterator(vector_.Get() + index);
    }
    
    // Задает ёмкость вектора
    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }
        ArrayPtr<Type> tmp(new_capacity);
        //std::move()
        //std::fill(tmp.Get(), tmp.Get() + size_, Type{});
        vector_.swap(tmp);
        capacity_ = new_capacity;
    }
    
    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        vector_.swap(other.vector_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }
 
    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return Iterator(vector_.Get());
    }
 
    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return Iterator(vector_.Get() + size_);
    }
 
    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return ConstIterator(vector_.Get());
    }
 
    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return ConstIterator(vector_.Get() + size_);
    }
 
    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return begin();
    }
 
    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return end();
    }
    
private:
    ArrayPtr<Type> vector_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};
 
template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if(lhs.GetSize() == rhs.GetSize()) {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    return false;
}
 
template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}
 
template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
 
template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}
 
template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}
 
template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
