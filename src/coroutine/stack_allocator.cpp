#include "stack_allocator.h"

#include <sys/mman.h>

#include <cassert>

namespace coro {

StackAllocator::StackAllocator(size_t frame_num) {
  size_t m_size = frame_num * FRAME_SIZE;
  void* m_ptr = mmap(0, m_size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(m_ptr != nullptr);
  char* tmp_ptr = (char*)m_ptr;

  _page_size = StackAllocator::sysPageSize();
  _frame_num = frame_num;
  for (size_t i = 0; i < frame_num; ++i) {
    FrameHeader* f_hdr = (FrameHeader*)tmp_ptr;
    f_hdr->next_free = nullptr;

    int rc = mprotect((void*)(tmp_ptr + _page_size), _page_size, PROT_NONE);
    assert(rc == 0);

    tmp_ptr += FRAME_SIZE;
  }
  _addr = m_ptr;
}

StackAllocator::~StackAllocator() {
  int rc = munmap(_addr, _frame_num * FRAME_SIZE);
  assert(rc == 0);
}

StackAllocator::CO_STACK_ALLOC_STATUS StackAllocator::allocFrame(void** ptr) {
  *ptr = nullptr;
  FrameHeader *old_hdr, *new_hdr;
  while (true) {
    old_hdr = _first_free;
    if (old_hdr == nullptr) break;
    if (_first_free.compare_exchange_weak(old_hdr, old_hdr->next_free)) {
      old_hdr->next_free = nullptr;
      *ptr = old_hdr;
      return CO_STACK_SUCCESS;
    }
  }

  size_t cur_idx;
  if (_cur_alloc_range > _frame_num) {
    return CO_STACK_FAIL_WITH_NO_FREE;
  }
  cur_idx = _cur_alloc_range.fetch_add(1);
  if (cur_idx >= _frame_num) return CO_STACK_FAIL_WITH_NO_FREE;

  *ptr = (char*)_addr + (FRAME_SIZE * cur_idx);
  return CO_STACK_SUCCESS;
}

void StackAllocator::deallocFrame(void* ptr) {
  FrameHeader* old_hdr = _first_free;
  FrameHeader* new_hdr = (FrameHeader*)ptr;
  while (true) {
    new_hdr->next_free = old_hdr;
    if (_first_free.compare_exchange_weak(old_hdr, new_hdr)) return;
  }
}

StackGuard StackAllocator::allocSafe() {
  StackGuard guard(this);
  if (allocFrame(&(guard._frame)) == CO_STACK_FAIL_WITH_NO_FREE) {
    guard._alloc = nullptr;
    guard._frame = nullptr;
  }
  return guard;
}

}  // namespace coro