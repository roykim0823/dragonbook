// 01_example_11_5_unit_stride.cpp -- Example 11.5: a stride-3 loop rewritten
// with unit stride. for (i = 2; i <= 100; i += 3)   ==   for (j = 0; j <= 32;
// j++) with i = 3j + 2,   32 = floor((100-2)/3). Checks that both loops produce
// the same sequence of i values.
//
//   ./run.sh 1      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o
//   ex01 code/src/01_example_11_5_unit_stride.cpp && ./ex01
#include <cstdio>
#include <vector>

int main() {
  std::vector<long> a, b; // the i values each loop produces, in order
  for (long i = 2; i <= 100; i += 3)
    a.push_back(i); // the book's loop, stride 3
  for (long j = 0; j <= 32; j++)
    b.push_back(3 * j + 2); // the rewrite: unit stride, i = 3j + 2
  std::printf("Example 11.5 stride 3 -> unit stride: %s\n",
              a == b ? "same sequence" : "MISMATCH");
  std::printf("  i: ");
  for (long i : a)
    std::printf("%ld ", i);
  std::printf("\n");
}
