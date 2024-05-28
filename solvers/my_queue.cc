/// Author: Alexander Lopez
/// File: my_queue.hh
/// -----------------
/// This file contains a circular queue. This is helpful for a multithreaded
/// maze solving program because it allows me to reserve space in the underlying
/// array, similar to std::vector, so that threads can simply add to the queue
/// without asking the heap for thread safe resizing with new. The std  queue is
/// non-contiguous and therefore I cannot know how many heap requests may be
/// taking place behind the scenes, slowing parallelism.
module;
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
module labyrinth:my_queue;

template <class Value_type> class My_queue {

  public:
    My_queue() : elems_(initial_size), allocated_size_(initial_size)
    {}

    void
    reserve(size_t capacity)
    {
        elems_ = std::vector<Value_type>(capacity);
        allocated_size_ = capacity;
        logical_size_ = 0;
        front_ = 0;
        back_ = 0;
    }

    void
    push(const Value_type &elem)
    {
        // Doubling allocations so we can't acheive ULLONG_MAX for our container
        // size. Slightly less.
        if (logical_size_ == full_queue)
        {
            std::cerr << "My_queue is at max capacity.\n";
            std::abort();
        }
        if (logical_size_ == allocated_size_)
        {
            grow();
        }
        elems_[back_++] = elem;
        back_ %= allocated_size_;
        logical_size_++;
    }

    void
    pop()
    {
        if (logical_size_ == 0)
        {
            std::cerr << "My_queue is empty.\n";
            std::abort();
        }
        ++front_ %= allocated_size_;
        logical_size_--;
    }

    const Value_type &
    front() const
    {
        if (logical_size_ == 0)
        {
            std::cerr << "My_queue is empty.\n";
            std::abort();
        }
        return elems_[front_];
    }

    Value_type &
    front()
    {
        if (logical_size_ == 0)
        {
            std::cerr << "My_queue is empty.\n";
            std::abort();
        }
        return elems_[front_];
    }

    size_t
    size() const
    {
        return logical_size_;
    }

    bool
    empty() const
    {
        return logical_size_ == 0;
    }

  private:
    static constexpr size_t initial_size = 8;
    static constexpr size_t full_queue = 1UL << 63;
    std::vector<Value_type> elems_;
    size_t allocated_size_;
    size_t logical_size_{0};
    size_t front_{0};
    size_t back_{0};

    void
    grow()
    {
        std::vector<Value_type> new_elems(allocated_size_ * 2);
        const size_t back_front_diff = allocated_size_ - front_;
        const size_t first_chunk = std::min(logical_size_, back_front_diff);
        std::copy_n(&elems_[front_], first_chunk, new_elems.data());
        if (first_chunk < allocated_size_)
        {
            std::copy_n(new_elems.data() + first_chunk,
                        logical_size_ - first_chunk, elems_.data());
        }
        allocated_size_ *= 2;
        front_ = 0;
        back_ = logical_size_;
        elems_ = std::move(new_elems);
    }
};
