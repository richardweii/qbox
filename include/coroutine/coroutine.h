#pragma once
#include <atomic>
#include <functional>
#include <list>

#include "concurrent_queue.h"
#include "fcontext.h"

using coro_id_t = int;
using CoroutineTask = std::function<void()>;

// idle -> runnable -> waiting -> runnable -> idle
enum class CoroutineState {
  IDLE,
  RUNNABLE,
  WAITING,
};

class StackContext {
  static constexpr int kCoroutineStackSize = 2 << 20;  // 2MB
 public:
  StackContext() : _fctx(), base_(new char[kCoroutineStackSize]), _size(kCoroutineStackSize) {}
  ~StackContext() { delete[] base_; }
  inline char *sp() { return base_ + kCoroutineStackSize; }  // 栈地址是从高到低的，所以从边界开始
  inline size_t size() { return _size; }

  inline fcontext_t fctx() { return _fctx; }
  inline void setFctx(fcontext_t fctx) { _fctx = fctx; }

 private:
  fcontext_t _fctx;
  char *base_ = nullptr;  // 协程在堆上的位置
  size_t _size;
};

class Scheduler;
class Coroutine {
 public:
  friend class Scheduler;
  Coroutine(coro_id_t id, Scheduler *sched) : coro_id_(id), sched_(sched) {
    ctx_.setFctx(make_fcontext(ctx_.sp(), ctx_.size(), &Coroutine::entrance));
  }
  void yield();
  void co_wait(int events = 1);
  void wakeup_once();
  coro_id_t id() const { return coro_id_; }

  void *user_ctx() const { return user_ctx_; }
  void set_user_ctx(void *ctx) { user_ctx_ = ctx; }
  void clear_user_ctx() { user_ctx_ = nullptr; }

 protected:
  virtual void routine();

 private:
  void switch_coro(Coroutine *target);
  void resume();
  static void entrance(transfer_t from);

  coro_id_t coro_id_;
  Scheduler *sched_;
  StackContext ctx_;
  CoroutineTask task_;
  std::atomic_int waiting_events_{};
  std::list<Coroutine *>::iterator iter{};
  CoroutineState state_{CoroutineState::IDLE};
  //---------- for user ------------
  void *user_ctx_ = nullptr;
};