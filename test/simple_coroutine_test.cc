#include <unistd.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "coroutine/coroutine_pool.h"
#include "coroutine/scheduler.h"
#include "timer.h"

int switch_time = 100000;
int coro_num = 32;

constexpr uint64_t io_latency = 100;  // 100 us per io

struct IOEvent {
  uint64_t start;
  Coroutine *coro;
};

struct UserCtx {
  // something whatever....
};

class IOManager {
 public:
  static bool io_finish(IOEvent ev) { return RdtscTimer::instance().us() - ev.start >= io_latency; }

  int write_async() {
    assert(this_coroutine::is_coro_env());
    IOEvent io = {RdtscTimer::instance().us(), this_coroutine::current()};
    std::cout << "Coro [" << this_coroutine::current()->id() << "] issue IO " << io.start << std::endl;
    add_io(io);
    return 0;
  }

  void add_io(IOEvent ev) { io_set.push(ev); }
  IOEvent next_io_event() {
    auto ev = io_set.front();
    io_set.pop();
    return ev;
  };
  size_t io_size() const { return io_set.size(); }

 private:
  std::queue<IOEvent> io_set;
};

void fake_event_loop(IOManager *io_manger) {
  assert(this_coroutine::is_coro_env());
  // 只遍历一次所有IO
  auto size = io_manger->io_size();
  for (size_t i = 0; i < size; i++) {
    IOEvent ev = io_manger->next_io_event();
    if (io_manger->io_finish(ev)) {
      std::cout << "current time : " << RdtscTimer::instance().us() << std::endl;
      std::cout << "Coro [" << ev.coro->id() << "] finish IO " << ev.start << std::endl;
      ev.coro->wakeup_once();

      UserCtx *ctx = reinterpret_cast<UserCtx *>(ev.coro->user_ctx());
      // can do something with `UserCtx`
    } else {
      io_manger->add_io(ev);  // 放回去
    }
  }
}

int main() {
  IOManager io;
  CoroutinePool coro_pool(1, coro_num);
  coro_pool.regester_polling_func(std::bind(fake_event_loop, &io));
  std::atomic_int wg{0};

  auto simple_write_task = [&wg, &io]() {
    UserCtx *ctx = new UserCtx();
    this_coroutine::current()->set_user_ctx(ctx);
    //
    // do some work before write
    //
    io.write_async();
    this_coroutine::co_wait();

    std::cout << "Coro " << this_coroutine::current()->id() << " finish all io task" << std::endl;
    //
    // do some work after write
    //
    this_coroutine::current()->clear_user_ctx();
    delete ctx;
    wg.fetch_add(1);
  };

  auto multi_write_task = [&wg, &io]() {
    UserCtx *ctx = new UserCtx();
    this_coroutine::current()->set_user_ctx(ctx);
    //
    // do some work before write
    //
    int batch = 10;
    for (int i = 0; i < batch; i++) {
      io.write_async();
    }
    this_coroutine::co_wait(batch);

    std::cout << "Coro " << this_coroutine::current()->id() << " finish all io task" << std::endl;
    //
    // do some work after write
    //
    this_coroutine::current()->clear_user_ctx();
    delete ctx;
    wg.fetch_add(1);
  };

  for (int i = 0; i < 1000; i++) {
    coro_pool.enqueue(simple_write_task);
    coro_pool.enqueue(multi_write_task);
  }

  auto start = std::chrono::system_clock::now();
  coro_pool.start();
  while (wg.load() != 1000 * 2)
    ;
  auto end = std::chrono::system_clock::now();
  long elapse = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  double switch_cost_avg = elapse * 1.0 / (switch_time * coro_num);

  std::cout << "elapse : " << elapse << "us" << std::endl;
  std::cout << "total IO Request : " << 1000 * 11 << std::endl;
  std::cout << "IO latency_avg : " << 1000 * 11 * 1.0 / elapse << " us" << std::endl;
  return 0;
}