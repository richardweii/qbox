#ifndef _STACK_ALLOCATOR_H_
#define _STACK_ALLOCATOR_H_

#include <unistd.h>

#include <atomic>
#include <cstdint>

namespace coro {

class StackGuard;

class StackAllocator {
 public:
  // 单个协程栈2MB
  static constexpr size_t FRAME_SIZE = (2 << 20);

  enum CO_STACK_ALLOC_STATUS {
    CO_STACK_SUCCESS = 0,
    CO_STACK_FAIL_WITH_NO_FREE,
  };

  static inline size_t sysPageSize() { return sysconf(_SC_PAGESIZE); }

  StackAllocator(size_t frame_num = 1024);
  ~StackAllocator();

  StackAllocator(const StackAllocator&) = delete;
  StackAllocator& operator=(const StackAllocator&) = delete;

  static StackAllocator* instance() {
    static StackAllocator allocator;
    return &allocator;
  }

  CO_STACK_ALLOC_STATUS allocFrame(void** ptr);
  void deallocFrame(void* ptr);

  StackGuard allocSafe();

 private:
  struct FrameHeader {
    FrameHeader* next_free;
  };

  size_t _page_size;
  size_t _frame_num;
  void* _addr;
  std::atomic<FrameHeader*> _first_free{0};
  std::atomic_uint64_t _cur_alloc_range{0};
};

class StackGuard {
 public:
  StackGuard() : _frame(nullptr), _alloc(nullptr) {}
  StackGuard(StackAllocator* alloc) : _frame(nullptr), _alloc(alloc) {}
  ~StackGuard() {
    if (isValid()) {
      _alloc->deallocFrame(_frame);
    }
    _frame = nullptr;
    _alloc = nullptr;
  }

  bool isValid() { return _alloc != nullptr && _frame != nullptr; }

  StackGuard(const StackGuard&) = delete;
  StackGuard& operator=(const StackGuard&) = delete;

  StackGuard(StackGuard&& rhs) {
    this->_frame = rhs._frame;
    this->_alloc = rhs._alloc;

    rhs._frame = nullptr;
    rhs._alloc = nullptr;
  }

  StackGuard& operator=(StackGuard&& rhs) {
    this->_frame = rhs._frame;
    this->_alloc = rhs._alloc;

    rhs._frame = nullptr;
    rhs._alloc = nullptr;
    return *this;
  }

  void* getFrameAddr() { return _frame; }

 private:
  friend class StackAllocator;
  void* _frame;
  StackAllocator* _alloc;
};

}  // namespace coro
#endif  // _STACK_ALLOCATOR_H_
