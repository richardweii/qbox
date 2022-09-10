#ifndef _CORO_SCHEDULER_H_
#define _CORO_SCHEDULER_H_

#include <condition_variable>
#include <mutex>

#include "coroutine.h"
#include "logging.h"

namespace coro {

class CoScheduler : public Coroutine {
 public:
  friend class Coroutine;
  CoScheduler() : Coroutine(nullptr){};
  bool init();
  void start(bool new_thread = false) {
    if (new_thread) {
      _this_thread = new std::thread([this]() {
        _instance = this;
        instance()->routine();
      });
    } else {
      _instance = this;
      instance()->routine();
    }
  }

  void stop() { 
    _stop = true;
    _cv.notify_one();
  }

  void join() {
    if (_this_thread == nullptr) {
      LOG_ERROR("cannot join.");
      return;
    }
    _this_thread->join();
    delete _this_thread;
  }

  void wakeUp(Coroutine *coro) { coro->wakeUpOnce(); }

  static CoScheduler *instance() { return _instance; }
  Coroutine *active() { return _active; }

 private:
  void routine() override;
  static thread_local CoScheduler *_instance;
  void polling();
  // thread-safe
  void activate(Coroutine *coro);

  void enqueue(Coroutine **head, Coroutine **tail, Coroutine *coro);
  Coroutine *dequeue(Coroutine **head, Coroutine **tail);

  void removeFromQueue(Coroutine **head, Coroutine **tail, Coroutine *coro);

  Coroutine *_waiting_list_head = nullptr;
  Coroutine *_waiting_list_tail = nullptr;

  Coroutine *_runnable_list_head = nullptr;
  Coroutine *_runnable_list_tail = nullptr;

  Coroutine *_active = nullptr;
  volatile bool _stop = false;
  std::thread *_this_thread = nullptr;
  std::mutex _mutex;
  std::condition_variable _cv;
};

}  // namespace coro
#endif  // _CORO_SCHEDULER_H_