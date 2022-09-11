#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <atomic>
#include <functional>
#include <thread>

#include "fcontext.h"
#include "nocopyable.h"

namespace coro {
using Task = std::function<void()>;

class StackContext;
class CoScheduler;

class Coroutine NOCOPYABLE {
 public:
  enum CoroStatus {
    CO_STATUS_BORN = 0,
    CO_STATUS_RUNNABLE,
    CO_STATUS_RUNNING,
    CO_STATUS_WAITING,
    CO_STATUS_FINISH,
    CO_STATUS_EXIT,
  };

  Coroutine(CoScheduler *scheduler);
  virtual ~Coroutine();

  bool run(Task &&task) {
    _task = std::move(task);
    return _initContext();
  }

  template <typename Callable, typename... Args>
  bool run(Callable &&callable, Args &&...args) {
    _task = std::move(std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...));
    return _initContext();
  }

  void resume();
  void yield();
  void co_wait(int events = 1);
  void wakeUpOnce();
  uint64_t id() const { return _coro_id; }

 protected:
  friend class CoScheduler;
  virtual void routine();
  void switchCoro(Coroutine *coro);
  static void entrance(transfer_t from);
  StackContext *context() { return _ctx; }

  CoScheduler *_scheduler;
  Task _task;
  uint64_t _coro_id;
  CoroStatus _status;
  StackContext *_ctx;

  Coroutine *_prev = nullptr;
  Coroutine *_next = nullptr;
  std::atomic_int _waiting_events{0};

 private:
  bool _initContext();
};

namespace this_coroutine {
void yield();
void co_wait(int events = 1);
}  // namespace this_coroutine
}  // namespace coro

#endif  // _COROUTINE_H_