#include "rte_ring.h"

#include <iostream>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

struct Foo {
  int a;
  char *b;
  Foo(Foo &&other) {
    std::cout << "Foo(Foo &&other)" << std::endl;
    b = other.b;
    a = other.a;
    other.b = nullptr;
  };

  Foo(const Foo &other) {
    std::cout << "Foo(const Foo &other)" << std::endl;
    a = other.a;
    b = new char[10];
    // memecp
  };

  Foo &operator=(Foo &&other) {
    std::cout << "Foo &operator=(Foo &&other)" << std::endl;

    b = other.b;
    a = other.a;
    other.b = nullptr;
    return *this;
  };

  Foo &operator=(const Foo &other) {
    std::cout << "Foo &operator=(const Foo &other)" << std::endl;
    b = new char[10];
    a = other.a;
    return *this;
  };

  Foo() {
    a = 1;
    b = new char[10];
  }
};

TEST(RteRing, basic) {
  RteRing<Foo> ring(16);
  std::vector<Foo> foos;
  for (int i = 0; i < 16; i++) {
    foos.emplace_back();
    foos.back().a = 1;
  }

  for (int i = 0; i < 15; i++) {
    EXPECT_EQ(ring.enqueue(&foos[i]), RteRing<Foo>::RTE_RING_OK);
  }
  EXPECT_EQ(ring.enqueue(&foos[15]), RteRing<Foo>::RTX_RING_FULL);

  Foo *foos_ptr[10];
  auto ret = ring.dequeue(10, foos_ptr);
  EXPECT_EQ(ret, RteRing<Foo>::RTE_RING_OK);

  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(foos_ptr[i]->a, foos[i].a);
    EXPECT_EQ(foos_ptr[i]->b, foos[i].b);
  }
}

struct Noob {
  int a;
  int b;
  int c;
  Noob() = default;
  Noob(int a, int b, int c) : a(a), b(b), c(c){};
};

TEST(RteRing, Perf) {
  const int threads_num = 16;
  const int iteration = 100000;
  RteRing<Noob> queue(4096);

  std::vector<Noob> noobs;
  noobs.reserve(iteration * threads_num);
  for (int i = 0; i < iteration * threads_num; i++) {
    noobs.emplace_back(i, i * 2, i * 3);
  }

  auto time_start = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;
  for (int i = 0; i < threads_num; i++) {
    threads.emplace_back([i, &queue, &noobs]() -> void {
      for (int j = 0; j < iteration;) {
        auto rc = queue.enqueue((&(noobs)[i * iteration + j]));
        if (rc == RteRing<Noob>::RTE_RING_OK) {
          j++;
        }
      }
    });
  }

  for (int i = 0; i < threads_num; i++) {
    threads.emplace_back([&queue]() -> void {
      for (int j = 0; j < iteration;) {
        Noob *noob = nullptr;
        auto rc = queue.dequeue(&noob);
        if (rc == RteRing<Noob>::RTE_RING_OK) {
          // EXPECT_NE(noob, nullptr);
          // EXPECT_EQ(noob->c, noob->a + noob->b);
          j++;
        }
      }
    });
  }

  for (auto &th : threads) {
    th.join();
  }

  auto time_end = std::chrono::high_resolution_clock::now();
  auto time_delta = time_end - time_start;
  auto count = std::chrono::duration_cast<std::chrono::microseconds>(time_delta).count();
  std::cout << "Total time:" << count * 1.0 / 1000 / 1000 << "s" << std::endl;
  std::cout << "Avg time:" << count * 1.0 / iteration / 32 << "us" << std::endl;
}