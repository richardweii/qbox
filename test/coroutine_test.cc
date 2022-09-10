#include "coroutine/coroutine.h"

#include "coroutine/co_scheduler.h"
#include "gtest/gtest.h"
#include "logging.h"

using namespace std;
using namespace coro;
TEST(Coroutine, Basic) {
  CoScheduler sched;
  bool succ = sched.init();
  ASSERT_TRUE(succ);
  sched.start(true);

  Coroutine coro1(&sched);
  Coroutine coro2(&sched);
  Coroutine coro3(&sched);

  coro1.run([&coro1]() {
    LOG_INFO("now in coroutine %ld, step 1", coro1.id());
    this_coroutine::yield();
    LOG_INFO("now in coroutine %ld, step 2", coro1.id());
    this_coroutine::co_wait();
    LOG_INFO("now in coroutine %ld, step 3", coro1.id());
  });

  coro2.run([&coro2]() {
    LOG_INFO("now in coroutine %ld, step 1", coro2.id());
    this_coroutine::yield();
    LOG_INFO("now in coroutine %ld, step 2", coro2.id());
    this_coroutine::yield();
    LOG_INFO("now in coroutine %ld, step 3", coro2.id());
  });

  coro3.run([&coro3, &coro1]() {
    LOG_INFO("now in coroutine %ld, step 1", coro3.id());
    this_coroutine::yield();
    LOG_INFO("now in coroutine %ld, step 2", coro3.id());
    this_coroutine::yield();
    LOG_INFO("now in coroutine %ld, step 3", coro3.id());
    LOG_INFO("now in coroutine %ld, wake up coro1", coro3.id());
    coro1.wakeUpOnce();
  });

  this_thread::sleep_for(std::chrono::milliseconds(100));
  sched.stop();
  sched.join();
}

TEST(Coroutine, Generic) {
  CoScheduler sched;
  bool succ = sched.init();
  ASSERT_TRUE(succ);
  sched.start(true);

  Coroutine coro1(&sched);
  Coroutine coro2(&sched);
  Coroutine coro3(&sched);

  coro1.run([](Coroutine *coro) {
    LOG_INFO("now in coroutine %ld, step 1", coro->id());
    this_coroutine::yield();
    LOG_INFO("now in coroutine %ld, step 2", coro->id());
    this_coroutine::co_wait();
    LOG_INFO("now in coroutine %ld, step 3", coro->id());
  }, &coro1);

  coro2.run([](Coroutine *coro) {
    LOG_INFO("now in coroutine %ld, step 1", coro->id());
    this_coroutine::yield();
    LOG_INFO("now in coroutine %ld, step 2", coro->id());
    this_coroutine::co_wait();
    LOG_INFO("now in coroutine %ld, step 3", coro->id());
  }, &coro2);

  coro3.run([](Coroutine *coro, Coroutine *a, Coroutine *b) {
    LOG_INFO("now in coroutine %ld, step 1", coro->id());
    this_coroutine::yield();
    LOG_INFO("now in coroutine %ld, step 2", coro->id());
    this_coroutine::yield();
    LOG_INFO("now in coroutine %ld, step 3", coro->id());

    LOG_INFO("now in coroutine %ld, wake up other coro %ld", coro->id(), a->id());
    a->wakeUpOnce();
    LOG_INFO("now in coroutine %ld, wake up other coro %ld", coro->id(), b->id());
    b->wakeUpOnce();
  }, &coro3, &coro1, &coro2);

  this_thread::sleep_for(std::chrono::milliseconds(100));
  sched.stop();
  sched.join();
}