#include "bitmap.h"
#include "test.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>

void TestBasic() {
  Bitmap *bitmap = NewBitmap(157);
  EXPECT(!bitmap->Full(), "shouldn't full");
  for (int i = 0; i < 157; i++) {
    EXPECT(bitmap->SetFirstFreePos(), "set free %d failed", i);
  }
  EXPECT(bitmap->Full(), "should full");
  bitmap->Clear(2);
  EXPECT(!bitmap->Full(), "shouldn't full");
  EXPECT(bitmap->FirstFreePos() == 2, "first free should be %d", 2);
  EXPECT(bitmap->Set(2), "set %d failed", 2);
}

int main(int argc, char **argv) {
  TestBasic();
  Bitmap *bitmap = NewBitmap(819);
  int num = std::atoi(argv[1]);
  {
    auto time_start = std::chrono::high_resolution_clock::now();
    for (int j = 0; j < num; j++) {
      for (int i = 0; i < 819; i++) {
        bitmap->SetFirstFreePos();
      }
      for (int i = 0; i < 819; i++) {
        bitmap->Clear(i);
      }
    }

    auto time_end = std::chrono::high_resolution_clock::now();
    auto time_delta = time_end - time_start;
    auto count =
        std::chrono::duration_cast<std::chrono::microseconds>(time_delta)
            .count();
    std::cout << "Total time:" << count * 1.0 / 1000 / 1000 << "s" << std::endl;
  }
  DeleteBitmap(bitmap);
}