#include "coroutine/coroutine.h"

#include <atomic>
#include <cassert>

#include "coroutine/co_scheduler.h"
#include "coroutine/fcontext.h"
#include "logging.h"
#include "stack_context.h"

namespace coro {

uint64_t newCoroutineID() {
  thread_local std::atomic_uint64_t id{0};
  return id.fetch_add(1, std::memory_order_relaxed);
}

Coroutine::Coroutine(CoScheduler *scheduler)
    : _scheduler(scheduler), _coro_id(newCoroutineID()), _status(CO_STATUS_BORN), _ctx(new StackContext) {}

Coroutine::~Coroutine() {
  assert(_prev == nullptr && _next == nullptr);
  assert(_status == CO_STATUS_FINISH);
  StackContext::destroy(_ctx);
  _status = CO_STATUS_EXIT;
  delete _ctx;
}

void Coroutine::entrance(transfer_t from) {
  Coroutine *coro = reinterpret_cast<Coroutine *>(from.data);
  Coroutine *sched = coro->_scheduler;
  if (coro != sched) {
    sched->context()->setFctx(from.fctx);
  }
  coro->_status = CO_STATUS_RUNNING;
  coro->routine();
  coro->_status = CO_STATUS_FINISH;
  if (coro != sched) {
    jump_fcontext(sched->context()->fctx(), nullptr);
  }
}

bool Coroutine::_initContext() {
  int rc = StackContext::createContext(_ctx);
  if (rc) {
    LOG_ERROR("Create coroutine stack failed.");
    return false;
  }

  _ctx->setFctx(make_fcontext(_ctx->sp(), _ctx->size(), Coroutine::entrance));

  _status = CO_STATUS_RUNNABLE;
  _scheduler->activate(this);
  return true;
}

void Coroutine::routine() {
  if (_task) {
    _task();
  } else {
    LOG_INFO("NO TASK TO EXECUTE");
  }
}

void Coroutine::yield() {
  _status = CO_STATUS_RUNNABLE;
  transfer_t t = jump_fcontext(_scheduler->context()->fctx(), nullptr);
}

void Coroutine::resume() {
  transfer_t t = jump_fcontext(_ctx->fctx(), this);
  _ctx->setFctx(t.fctx);
}

void Coroutine::co_wait(int events) {
  _waiting_events.store(events);
  _status = CO_STATUS_WAITING;
  transfer_t t = jump_fcontext(_scheduler->context()->fctx(), nullptr);
}

void Coroutine::wakeUpOnce() {
  _waiting_events.fetch_sub(1);
  if (_waiting_events == 0) {
    _status = CO_STATUS_RUNNABLE;
    _waiting_events = 0;
    _scheduler->activate(this);
  }
}

}  // namespace coro