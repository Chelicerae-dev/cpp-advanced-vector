#pragma once
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>
template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept {
        Swap(other);
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        Swap(rhs);
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    //Конструктор по умолчанию.
    //Инициализирует вектор нулевого размера и вместимости. Не выбрасывает исключений.
    //Алгоритмическая сложность: O(1).
    Vector() noexcept = default;

    //Конструктор, который создаёт вектор заданного размера.
    //Вместимость созданного вектора равна его размеру, а элементы проинициализированы значением по умолчанию для типа T.
    //Алгоритмическая сложность: O(размер вектора).
    explicit Vector(size_t size);

    //Копирующий конструктор. Создаёт копию элементов исходного вектора.
    //Имеет вместимость, равную размеру исходного вектора, то есть выделяет память без запаса.
    //Алгоритмическая сложность: O(размер исходного вектора).
    explicit Vector(const Vector& other);

    Vector(Vector&& other) noexcept;   

    Vector& operator=(const Vector& rhs);
    Vector& operator=(Vector&& rhs) noexcept;

    void Swap(Vector& other) noexcept;

    //Деструктор. Разрушает содержащиеся в векторе элементы и освобождает занимаемую ими память.
    //Алгоритмическая сложность: O(размер вектора).
    ~Vector();

    //Метод void Reserve(size_t capacity). Резервирует достаточно места, чтобы вместить количество элементов, равное capacity.
    //Если новая вместимость не превышает текущую, метод не делает ничего.
    //Алгоритмическая сложность: O(размер вектора).
    void Reserve(size_t capacity);

    void Resize(size_t new_size);
    void PushBack(const T& value);
    void PushBack(T&& value);
    void PopBack() /* noexcept */;

    using iterator = T*;
    using const_iterator = const T*;

    template <typename... Args>
    T& EmplaceBack(Args&&... args);



    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_ + size_;
    }
    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator end() const noexcept {
        return data_ + size_;
    }
    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept {
        return data_ + size_;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }
    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        size_t position = pos - cbegin();
        if(data_.Capacity() > size_) {
            if(position == size_) {
                new (begin() + position) T(std::forward<Args>(args)...);
            } else {
                new (end()) T(std::forward<T>(*(end() - 1)));
                std::move_backward(begin() + position, end() - 1, end());
                data_[position] = T(std::forward<Args>(args)...);
            }
        } else {
            RawMemory<T> temp((size_ == 0) ? 1 : 2 * size_);
            new (temp + position) T(std::forward<Args>(args)...);
            UninitializedCopyMove(begin(), position, temp.GetAddress());
            UninitializedCopyMove(begin() + position, cend() - pos, temp + position + 1);
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(temp);
        }
        ++size_;
        return begin() + position;
    }

    iterator Erase(const_iterator pos) {
        std::move(begin() + (pos - cbegin() + 1), end(), begin() + (pos - cbegin()));
        size_--;
        std::destroy_at(end());
        return data_ + (pos - cbegin());
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    template <typename It, typename NoThrowIt>
    void UninitializedCopyMove(It from, size_t size, NoThrowIt to) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(from, size, to);
        } else {
            std::uninitialized_copy_n(from, size, to);
        }
    }

    void ReallocatePush(T value) {
        if (size_ == data_.Capacity())
        {

            RawMemory<T> tmp_memory((size_ == 0) ? 1 : 2 * size_);
            new (tmp_memory.GetAddress() + size_) T(value);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
            {
                std::uninitialized_move_n(data_.GetAddress(), size_, tmp_memory.GetAddress());
            }
            else
            {
                std::uninitialized_copy_n(data_.GetAddress(), size_, tmp_memory.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(tmp_memory);
        }
        else
        {
            new (data_.GetAddress() + size_) T(value);
        }
        ++size_;
    }

};

template <typename T>
Vector<T>::Vector(size_t size)
    : data_(size)
    , size_(size) {
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}

template <typename T>
Vector<T>::Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_) {
    std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
}

template <typename T>
Vector<T>::Vector(Vector&& other) noexcept {
    Swap(other);
    other.~Vector<T>();
}

template <typename T>
Vector<T>::~Vector() {
   std::destroy_n(data_.GetAddress(), size_);
}

template <typename T>
void Vector<T>::Reserve(size_t new_capacity) {
    if (new_capacity > data_.Capacity()) {
        RawMemory<T> new_data(new_capacity);
        UninitializedCopyMove(data_.GetAddress(), size_, new_data.GetAddress());
        std::destroy_n(data_.GetAddress(), data_.Capacity());
        data_.Swap(new_data);
    }
}

template <typename T>
Vector<T>& Vector<T>::operator=(const Vector& rhs) {
    if(this != &rhs) {
        if(data_.Capacity() >= rhs.size_) {
            if(size_ > rhs.size_) {
                for(size_t i = 0; i < rhs.size_; ++i) {
                    data_[i] = rhs.data_[i];
                }
                std::destroy_n(data_ + rhs.size_, size_ - rhs.size_);
            } else {
                for(size_t i = 0; i < size_; ++i) {
                    data_[i] = rhs.data_[i];
                }
                std::uninitialized_copy_n(rhs.data_ + size_, rhs.size_ - size_, data_ + size_);
            }
            size_ = rhs.size_;
        } else {
            Vector<T> temp(rhs);
            Swap(temp);
        }
    }
    return *this;
}

template <typename T>
Vector<T>& Vector<T>::operator=(Vector&& rhs) noexcept {
    if(this != &rhs) {
        Swap(rhs);
    }
    return *this;
}

template <typename T>
void Vector<T>::Swap(Vector& other) noexcept {
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
}

template <typename T>
void Vector<T>::Resize(size_t new_size) {
    if(size_ > new_size) {
        std::destroy_n(data_ + new_size, size_ - new_size);
        size_ = new_size;
    } else if(new_size > size_) {
        if(data_.Capacity() < new_size) {
            Reserve(new_size);
        }
        std::uninitialized_default_construct_n(data_ + size_, new_size - size_);
        size_ = new_size;
    }
}

template <typename T>
void Vector<T>::PushBack(const T& value) {
    if (size_ == data_.Capacity()) {
        RawMemory<T> temp((size_ == 0) ? 1 : 2 * size_);
        new (temp + size_) T(value);
        UninitializedCopyMove(data_.GetAddress(), size_, temp.GetAddress());
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(temp);
    } else {
        new (data_ + size_) T(value);
    }
    ++size_;
}

//Я не знаю почему не работает адекватно единый вариант с std::forward и передачей по значению - появляются проблемы с перемещением лишним или копированием
template <typename T>
void Vector<T>::PushBack(T&& value) {
    if (size_ == data_.Capacity()) {
        RawMemory<T> temp((size_ == 0) ? 1 : 2 * size_);
        new (temp + size_) T(std::move(value));
        UninitializedCopyMove(data_.GetAddress(), size_, temp.GetAddress());
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(temp);
    } else {
        new (data_ + size_) T(std::move(value));
    }
    ++size_;
}

template <typename T>
void Vector<T>::PopBack() /* noexcept */ {
    std::destroy(data_ + size_ - 1, data_ + size_);
    size_--;
}

template <typename T>
template <typename... Args>
T& Vector<T>::EmplaceBack(Args&&... args) {
    if (size_ == data_.Capacity()) {
        RawMemory<T> temp((size_ == 0) ? 1 : 2 * size_);
        new (temp.GetAddress() + size_) T(std::forward<Args>(args)...);
        UninitializedCopyMove(data_.GetAddress(), size_, temp.GetAddress());
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(temp);
    } else {
        new (data_ + size_) T(std::forward<Args>(args)...);
    }
    ++size_;
    return *(data_ + size_ - 1);
}
