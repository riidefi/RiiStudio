// Allocator.h: Ultra fast C++11 allocator for STL containers.
//
// https://github.com/moya-lang/Allocator/blob/master/Allocator.h
//
// (Adapted for exceptionless C++)
//
#pragma once

#include <memory>

namespace Moya {

template <class T, std::size_t growSize = 1024> class MemoryPool {
  struct Block {
    Block* next;
  };

  class Buffer {
    static const std::size_t blockSize = sizeof(T) > sizeof(Block)
                                             ? sizeof(T)
                                             : sizeof(Block);
    uint8_t data[blockSize * growSize];

  public:
    Buffer* const next;

    Buffer(Buffer* next) : next(next) {}

    T* getBlock(std::size_t index) {
      return reinterpret_cast<T*>(&data[blockSize * index]);
    }
  };

  Block* firstFreeBlock = nullptr;
  Buffer* firstBuffer = nullptr;
  std::size_t bufferedBlocks = growSize;

public:
  MemoryPool() = default;

  ~MemoryPool() {
    while (firstBuffer) {
      Buffer* buffer = firstBuffer;
      firstBuffer = buffer->next;
      delete buffer;
    }
  }

  T* allocate() {
    if (firstFreeBlock) {
      Block* block = firstFreeBlock;
      firstFreeBlock = block->next;
      return reinterpret_cast<T*>(block);
    }

    if (bufferedBlocks >= growSize) {
      firstBuffer = new Buffer(firstBuffer);
      bufferedBlocks = 0;
    }

    return firstBuffer->getBlock(bufferedBlocks++);
  }

  void deallocate(T* pointer) {
    Block* block = reinterpret_cast<Block*>(pointer);
    block->next = firstFreeBlock;
    firstFreeBlock = block;
  }
};

template <class T, std::size_t growSize = 1024>
class Allocator : private MemoryPool<T, growSize> {
public:
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef T value_type;

  template <class U> struct rebind {
    typedef Allocator<U, growSize> other;
  };

  pointer allocate(size_type n, const void* hint = 0) {
    if (n != 1 || hint)
      std::terminate();

    return MemoryPool<T, growSize>::allocate();
  }

  void deallocate(pointer p, size_type n) {
    MemoryPool<T, growSize>::deallocate(p);
  }

  void construct(pointer p, const_reference val) { new (p) T(val); }

  void destroy(pointer p) { p->~T(); }
};

} // namespace Moya
