/* Author: Alexander Lopez
 * File: my_queue.hh
 * -----------------
 * This file contains a circular queue. This is helpful for a multithreaded maze solving program
 * because it allows me to reserve space in the underlying array, similar to std::vector, so that
 * threads can simply add to the queue without asking the heap for thread safe resizing with new.
 * The std  queue is non-contiguous and therefore I cannot know how many heap requests may be
 * taking place behind the scenes, slowing parallelism.
 */
#pragma once
#ifndef MY_QUEUE_HH
#define MY_QUEUE_HH
#include <stdexcept>
#include <vector>

template<class Value_type>
class My_queue
{

public:
  My_queue() : elems_( initial_size_ ), allocated_size_( initial_size_ ) {}

  void reserve( size_t capacity )
  {
    elems_ = std::vector<Value_type>( capacity );
    allocated_size_ = capacity;
    logical_size_ = 0;
    front_ = 0;
    back_ = 0;
  }

  void push( const Value_type& elem )
  {
    // Doubling allocations so we can't acheive ULLONG_MAX for our container size. Slightly less.
    if ( logical_size_ == full_queue_ ) {
      throw std::logic_error( "My_queue is at max capacity." );
    }
    if ( logical_size_ == allocated_size_ ) {
      grow();
    }
    elems_[back_++] = elem;
    back_ %= allocated_size_;
    logical_size_++;
  }

  void pop()
  {
    if ( logical_size_ == 0 ) {
      throw std::logic_error( "My_queue is empty." );
    }
    ++front_ %= allocated_size_;
    logical_size_--;
  }

  const Value_type& front() const
  {
    if ( logical_size_ == 0 ) {
      throw std::logic_error( "My_queue is empty." );
    }
    return elems_[front_];
  }

  Value_type& front()
  {
    if ( logical_size_ == 0 ) {
      throw std::logic_error( "My_queue is empty." );
    }
    return elems_[front_];
  }

  size_t size() const { return logical_size_; }

  bool empty() const { return logical_size_ == 0; }

private:
  static constexpr size_t initial_size_ = 8;
  const size_t full_queue_ = 1UL << 63;
  std::vector<Value_type> elems_;
  size_t allocated_size_;
  size_t logical_size_ { 0 };
  size_t front_ { 0 };
  size_t back_ { 0 };
  void grow()
  {
    std::vector<Value_type> new_elems( allocated_size_ * 2 );
    for ( size_t i = 0; i < logical_size_; i++ ) {
      new_elems[i] = elems_[front_];
      ++front_ %= allocated_size_;
    }
    allocated_size_ *= 2;
    front_ = 0;
    back_ = logical_size_;
    elems_ = std::move( new_elems );
  }
};

#endif // My_QUEUE_HH
