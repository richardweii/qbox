#include "coroutine/co_scheduler.h"

#include <cassert>

#include "coroutine/coroutine.h"
#include "logging.h"
#include "stack_context.h"

namespace coro {

thread_local CoScheduler *CoScheduler::_instance = nullptr;

namespace this_coroutine {
void yield() { CoScheduler::instance()->active()->yield(); }

void co_wait(int events) { CoScheduler::instance()->active()->co_wait(events); }
}  // namespace this_coroutine

bool CoScheduler::init() {
  int rc = StackContext::createContext(_ctx);
  if (rc) {
    LOG_ERROR("Create coroutine stack failed.");
    return false;
  }
  _ctx->setFctx(make_fcontext(_ctx->sp(), _ctx->size(), Coroutine::entrance));
  return true;
}

void CoScheduler::routine() {
  while (!_stop) {
    polling();
    {
      std::unique_lock<std::mutex> lk(_mutex);
      _cv.wait(lk, [this] { return _runnable_list_head != nullptr || _stop; });

      if (_stop) {
        break;
      }

      _active = dequeue(&_runnable_list_head, &_runnable_list_tail);
      assert(_active != nullptr);
    }

    // now run coroutine...
    _active->resume();
    // switch back

    switch (_active->_status) {
      case Coroutine::CO_STATUS_RUNNABLE:
        enqueue(&_runnable_list_head, &_runnable_list_tail, _active);
        break;

      case Coroutine::CO_STATUS_WAITING:
        enqueue(&_waiting_list_head, &_waiting_list_tail, _active);
        break;

      case Coroutine::CO_STATUS_FINISH:
        break;
      default:
        LOG_ERROR("Not Should Come here");
        break;
    }
  }
  _status = CO_STATUS_FINISH;
}

void CoScheduler::polling() {
  // TODO: do some polling work and wake up coroutine
}

void CoScheduler::activate(Coroutine *coro) {
  // TODO: using concurrent queue
  std::unique_lock<std::mutex> lk(_mutex);
  removeFromQueue(&_waiting_list_head, &_waiting_list_tail, coro);
  enqueue(&_runnable_list_head, &_runnable_list_tail, coro);
  _cv.notify_one();
}

void CoScheduler::enqueue(Coroutine **head, Coroutine **tail, Coroutine *coro) {
  assert(coro);
  if (*head == nullptr) {
    *head = coro;
    *tail = coro;
    coro->_prev = nullptr;
    coro->_next = nullptr;
    return;
  }

  (*tail)->_next = coro;
  coro->_prev = (*tail);
  coro->_next = nullptr;
  (*tail) = coro;
}

void CoScheduler::removeFromQueue(Coroutine **head, Coroutine **tail, Coroutine *coro) {
  assert(coro);
  if (*head == nullptr) {
    // empty list
    return;
  }

  if (coro->_prev == nullptr && coro->_next == nullptr) {
    if (*head != coro) {
      // not in the list
      return;
    }
    *head = nullptr;
    *tail = nullptr;
    return;
  }

  if (coro->_prev) {
    coro->_prev->_next = coro->_next;
    if (coro->_next) {
      coro->_next->_prev = coro->_prev;
    } else {
      // is tail
      (*tail) = coro->_prev;
    }
  } else {
    // is head
    coro->_next->_prev = nullptr;
    (*head) = coro->_next;
  }
  coro->_next = nullptr;
  coro->_prev = nullptr;
}

Coroutine *CoScheduler::dequeue(Coroutine **head, Coroutine **tail) {
  if (*head == nullptr) {
    // empty list
    return nullptr;
  }

  Coroutine *coro = *head;

  // only one
  if (coro->_next == nullptr) {
    *head = nullptr;
    *tail = nullptr;
    return coro;
  }

  coro->_next->_prev = nullptr;
  (*head) = coro->_next;

  coro->_next = nullptr;
  coro->_prev = nullptr;
  return coro;
}

}  // namespace coro