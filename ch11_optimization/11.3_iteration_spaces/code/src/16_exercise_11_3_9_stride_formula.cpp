// 16_exercise_11_3_9_stride_formula.cpp -- Exercise 11.3.9: the general
// unit-stride formula. for (i=A; i<=B; i+=C)  ==  for (j=0; j<=D; j++) with i =
// E*j + F,  E = C, F = A, D = floor((B-A)/C). Checked over 2 250 (A, B, C)
// triples, including negative starts.
//
//   ./run.sh 16      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include
//   -o ex16 code/src/16_exercise_11_3_9_stride_formula.cpp && ./ex16
#include "intdiv.h"

#include <cstdio>
#include <vector>

int main() {
  bool ok = true;
  long cases = 0;
  for (long a = -7; a <= 7; a++)
    for (long b = a + 1; b <= a + 30; b++)
      for (long c = 2; c <= 6; c++) {
        std::vector<long> orig, rewritten; // the i values each loop produces
        for (long i = a; i <= b; i += c)
          orig.push_back(i);
        // the formula under test
        long d = floorDiv(b - a, c); // D = floor((B - A) / C)
        long e = c;                  // E = C
        long f = a;                  // F = A
        for (long j = 0; j <= d; j++)
          rewritten.push_back(e * j + f);
        ok = ok && (orig == rewritten);
        cases++;
      }
  std::printf("Exercise 11.3.9  D=floor((B-A)/C), E=C, F=A  checked over %ld "
              "(A,B,C) triples: %s\n",
              cases, ok ? "all same" : "MISMATCH");
}
