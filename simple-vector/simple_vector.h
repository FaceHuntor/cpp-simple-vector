#pragma once

#include <cassert>
#include <stdexcept>
#include <initializer_list>

#include "array_ptr.h"

class ReserveProxyObj
{
public:
    explicit ReserveProxyObj(size_t size) : size_(size) {}
    [[nodiscard]] size_t GetSize() const {return size_;}
private:
    size_t size_;
};

ReserveProxyObj Reserve(size_t size){
    return ReserveProxyObj(size);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size) : SimpleVector(size, Type()){
    }

    SimpleVector(size_t size, const Type& value) {
        ArrayPtr<Type> new_data(size);
        std::fill(new_data.Get(), new_data.Get() + size, value);
        new_data.swap(data_);
        size_ = capacity_ = size;
    }

    SimpleVector(std::initializer_list<Type> init) {
        Construct(init, init.size());
    }

    explicit SimpleVector(ReserveProxyObj proxy) : SimpleVector() {
        Reserve(proxy.GetSize());
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return !size_;
    }

    Type& operator[](size_t index) noexcept {
        return data_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return data_[index];
    }

    Type& At(size_t index) {
        RangeCheck(index);
        return data_[index];
    }

    const Type& At(size_t index) const {
        RangeCheck(index);
        return data_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if(new_size <= capacity_){
            if(new_size > size_){
                FillDefault(data_.Get() + size_, data_.Get() + new_size);
            }
            size_ = new_size;
            return;
        }
        ArrayPtr<Type> new_data(new_size);
        std::copy(std::make_move_iterator(data_.Get()), std::make_move_iterator(data_.Get() + size_), new_data.Get());
        FillDefault(new_data.Get() + size_, new_data.Get() + new_size);
        new_data.swap(data_);
        size_ = capacity_ = new_size;
    }

    void Reserve(size_t new_capacity){
        if(new_capacity <= capacity_){
            return;
        }
        ArrayPtr<Type> new_data(new_capacity);
        std::copy(std::make_move_iterator(data_.Get()), std::make_move_iterator(data_.Get() + std::min(new_capacity, size_)), new_data.Get());
        new_data.swap(data_);
        capacity_ = new_capacity;
    }

    Iterator begin() noexcept {
        return data_.Get();
    }

    Iterator end() noexcept {
        return data_.Get() + size_;
    }

    ConstIterator begin() const noexcept {
        return data_.Get();
    }

    ConstIterator end() const noexcept {
        return data_.Get() + size_;
    }

    ConstIterator cbegin() const noexcept {
        return begin();
    }

    ConstIterator cend() const noexcept {
        return end();
    }

    SimpleVector(const SimpleVector& other) {
        Construct(other, other.size_);
    }

    SimpleVector(SimpleVector&& other)  noexcept {
        this->swap(other);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        SimpleVector tmp(rhs);
        swap(tmp);
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) = default;

    void PushBack(const Type& item) {
        PushBack(std::move(Type(item)));
    }

    void PushBack(Type&& item) {
        if(size_ == capacity_){
            Recreate(!capacity_ ? 1 : capacity_ * 2, size_, std::move(item));
        } else{
            data_[size_++] = std::move(item);
        }
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        return InsertInt(pos, std::move(Type(value)));
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        return InsertInt(pos, std::move(value));
    }

    void PopBack() noexcept {
        --size_;
    }

    Iterator Erase(ConstIterator pos) {
        auto shift = pos - begin();
        std::copy(std::make_move_iterator(data_.Get() + shift + 1), std::make_move_iterator(data_.Get() + size_), data_.Get() + shift);
        --size_;
        return begin() + shift;
    }

    void swap(SimpleVector& other) noexcept {
        data_.swap(other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

private:
    Iterator InsertInt(ConstIterator pos, Type&& value) {
        auto shift = pos - begin();
        if(size_ == capacity_){
            Recreate(!capacity_ ? 1 : capacity_ * 2, shift, std::move(value));
        } else{
            std::copy_backward(std::make_move_iterator(data_.Get() + shift), std::make_move_iterator(data_.Get() + size_), data_.Get() + size_ + 1);
            data_[shift] = std::move(value);
            ++size_;
        }
        return begin() + shift;
    }

    void RangeCheck(size_t index) const{
        if(index >= size_){
            throw std::out_of_range("");
        }
    }

    void Recreate(size_t new_capacity, size_t pos, Type&& item){
        ArrayPtr<Type> new_data(new_capacity);
        std::copy(std::make_move_iterator(data_.Get()), std::make_move_iterator(data_.Get() + pos), new_data.Get());
        new_data[pos] = std::move(item);
        std::copy(std::make_move_iterator(data_.Get() + pos), std::make_move_iterator(data_.Get() + size_), new_data.Get() + pos + 1);
        new_data.swap(data_);
        capacity_ = new_capacity;
        ++size_;
    }

    template<class Container>
    void Construct(const Container& other, size_t size){
        ArrayPtr<Type> new_data(size);
        std::copy(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()), new_data.Get());
        new_data.swap(data_);
        size_ = capacity_ = size;
    }

    void FillDefault(Iterator begin, Iterator end){
        for(auto it = begin; it != end; ++it){
            *it = std::move(Type());
        }
    }

private:
    ArrayPtr<Type> data_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
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
