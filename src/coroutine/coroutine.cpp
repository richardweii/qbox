#include "coroutine/coroutine.h"

#include "coroutine/coroutine_pool.h"
#include "coroutine/scheduler.h"
#include "logging.h"

void Coroutine::entrance(transfer_t from) {
  Coroutine *coro = reinterpret_cast<Coroutine *>(from.data);
  Coroutine *sched = coro->sched_;
  sched->ctx_.setFctx(from.fctx);
  coro->routine();
  jump_fcontext(sched->ctx_.fctx(), nullptr);
}

void Coroutine::routine() {
  while (true) {
    state_ = CoroutineState::RUNNABLE;
    // do some work
    task_();
    // exit
    state_ = CoroutineState::IDLE;
    clear_user_ctx();
    switch_coro(sched_);
  }
}

void Coroutine::yield() {
  if (this == sched_) {
    LOG_FATAL("DOT NOT ALLOW YIELD SCHEDULER.");
  }
  state_ = CoroutineState::RUNNABLE;
  switch_coro(sched_);
}

void Coroutine::resume() { switch_coro(this); }

void Coroutine::switch_coro(Coroutine *target) {
  transfer_t t = jump_fcontext(target->ctx_.fctx(), target);
  target->ctx_.setFctx(t.fctx);
}

void Coroutine::co_wait(int events) {
  if (events == 0) return;
  if (waiting_events_.fetch_add(events) + events > 0) {
    state_ = CoroutineState::WAITING;
    switch_coro(sched_);
  }
}

extern thread_local Scheduler *scheduler;

void Coroutine::wakeup_once() {
  if ((--waiting_events_) == 0) {
    LOG_ASSERT(scheduler != nullptr, "invalid scheduler");
    sched_->addWakupCoroutine(this);
  }
}