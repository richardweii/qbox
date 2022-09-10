#ifndef _CORO_CONTEXT_H_
#define _CORO_CONTEXT_H_
#include "coroutine/fcontext.h"
#include "stack_allocator.h"

namespace coro {

class StackContext {
 public:
  StackContext() : _fctx(), _guard(), _sp(nullptr), _size(0) {}

  static int createContext(StackContext* ctx);
  static void destroy(StackContext* ctx);

  inline char* sp() { return _sp; }
  inline size_t size() { return _size; }

  inline fcontext_t fctx() { return _fctx; }
  inline void setFctx(fcontext_t fctx) { _fctx = fctx; }

 private:
  fcontext_t _fctx;
  StackGuard _guard;
  char* _sp;
  size_t _size;
};

inline int StackContext::createContext(StackContext* ctx) {
  ctx->_guard = StackAllocator::instance()->allocSafe();
  if (!ctx->_guard.isValid()) return -1;
  ctx->_sp = (char*)(ctx->_guard.getFrameAddr()) + StackAllocator::FRAME_SIZE;
  ctx->_size = StackAllocator::FRAME_SIZE;
  return 0;
}

inline void StackContext::destroy(StackContext* ctx) {
  if (ctx->_guard.isValid()) {
    ctx->_guard.~StackGuard();
  }
  ctx->_sp = nullptr;
  ctx->_size = 0;
}

}  // namespace coro

#endif  // _CORO_CONTEXT_H_