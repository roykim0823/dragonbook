// 07_exercise_11_3_1_unit_stride.cpp -- Exercise 11.3.1: rewrite three strided
// loops with unit stride (substitute i = c*j + a, round the limit down).
//
//   ./run.sh 7      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o
//   ex07 code/src/07_exercise_11_3_1_unit_stride.cpp && ./ex07
#include <cstdio>
#include <vector>

int main() {
  std::printf("Exercise 11.3.1 (unit-stride rewrites)\n");
  std::vector<long> a, b;
  for (long i = 10; i < 50; i += 7)
    a.push_back(i); // (a)
  for (long j = 0; j <= 5; j++)
    b.push_back(7 * j + 10); // i = 7j+10, 7j+10 <= 49 -> j <= 5
  std::printf("  (a) i=10..49 step 7  vs  j=0..5, i=7j+10     %s\n",
              a == b ? "same" : "MISMATCH");
  a.clear();
  b.clear();
  for (long i = -3; i <= 10; i += 2)
    a.push_back(i); // (b)
  for (long j = 0; j <= 6; j++)
    b.push_back(2 * j - 3); // i = 2j-3, 2j-3 <= 10 -> j <= 6
  std::printf("  (b) i=-3..10 step 2  vs  j=0..6, i=2j-3      %s\n",
              a == b ? "same" : "MISMATCH");
  a.clear();
  b.clear();
  for (long i = 50; i >= 10; i--)
    a.push_back(i); // (c) descending
  for (long j = 0; j <= 40; j++)
    b.push_back(50 - j); // i = 50-j, 50-j >= 10 -> j <= 40
  std::printf("  (c) i=50 down to 10  vs  j=0..40, i=50-j     %s\n",
              a == b ? "same" : "MISMATCH");
}
