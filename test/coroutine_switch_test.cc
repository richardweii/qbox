#include <unistd.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "coroutine/coroutine_pool.h"
#include "coroutine/scheduler.h"

int switch_time = 100000;
int coro_num = 32;

int main() {
  CoroutinePool coro_pool(1, coro_num);
  std::atomic_int wg{0};
  auto func = [&wg]() {
    for (int i = 0; i < switch_time; i++) {
      this_coroutine::yield();
    }
    wg.fetch_add(1);
  };

  for (int i = 0; i < coro_num; i++) {
    coro_pool.enqueue(func);
  }

  auto start = std::chrono::system_clock::now();
  coro_pool.start();
  while (wg.load() != coro_num)
    ;
  auto end = std::chrono::system_clock::now();
  long elapse = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  double switch_cost_avg = elapse * 1.0 / (switch_time * coro_num);

  std::cout << "elapse : " << elapse << "us, swtich cost average: " << switch_cost_avg << "us/switch" << std::endl;
  return 0;
}