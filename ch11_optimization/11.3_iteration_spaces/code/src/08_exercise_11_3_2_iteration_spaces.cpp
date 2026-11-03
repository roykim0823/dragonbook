// 08_exercise_11_3_2_iteration_spaces.cpp -- Exercise 11.3.2: the iteration
// spaces of Fig. 11.17 (a), (b), (c); their sizes two ways. Counts the points
// by running the original loops and by brute-force enumeration of the
// polyhedra. The README describes the shapes; figures 09-11 plot these point
// sets.
//
//   ./run.sh 8      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o
//   ex08 code/src/08_exercise_11_3_2_iteration_spaces.cpp && ./ex08
#include "enumerate.h"

int main() {
  // The three loop nests of Fig. 11.17 as polyhedra ('<' bounds written as
  // '<=' by subtracting 1).
  Polyhedron nestA; // (a) for (i=1;i<30;i++) for (j=i+2;j<40-i;j++)
  nestA.vars = {"i", "j"};
  nestA.addInequality({{"i", 1}}, -1)
      .addInequality({{"i", -1}}, 29)             //  1 <= i <= 29
      .addInequality({{"j", 1}, {"i", -1}}, -2)   // j >= i + 2
      .addInequality({{"j", -1}, {"i", -1}}, 39); // j <= 39 - i
  Polyhedron nestB; // (b) for (i=10;i<=1000;i++) for (j=i;j<i+10;j++)
  nestB.vars = {"i", "j"};
  nestB.addInequality({{"i", 1}}, -10)
      .addInequality({{"i", -1}}, 1000)         // 10 <= i <= 1000
      .addInequality({{"j", 1}, {"i", -1}}, 0)  //  j - i >= 0      j >= i
      .addInequality({{"j", -1}, {"i", 1}}, 9); // -j + i + 9 >= 0  j <= i + 9
  Polyhedron nestC; // (c) for (i=0;i<100;i++) for (j=0;j<100+i;j++)
                    //     for (k=i+j;k<100-i-j;k++)
  nestC.vars = {"i", "j", "k"};
  nestC.addInequality({{"i", 1}}, 0)
      .addInequality({{"i", -1}}, 99) // 0 <= i <= 99
      .addInequality({{"j", 1}}, 0)
      .addInequality({{"j", -1}, {"i", 1}}, 99)              // 0 <= j <= 99 + i
      .addInequality({{"k", 1}, {"i", -1}, {"j", -1}}, 0)    // k >= i + j
      .addInequality({{"k", -1}, {"i", -1}, {"j", -1}}, 99); // k <= 99 - i - j

  long la = 0, lb = 0, lc = 0; // count by running the original nests
  for (long i = 1; i < 30; i++)
    for (long j = i + 2; j < 40 - i; j++)
      la++;
  for (long i = 10; i <= 1000; i++)
    for (long j = i; j < i + 10; j++)
      lb++;
  for (long i = 0; i < 100; i++)
    for (long j = 0; j < 100 + i; j++)
      for (long k = i + j; k < 100 - i - j; k++)
        lc++;
  size_t pa = enumerateBox(nestA, -5, 60).size();
  size_t pb = enumerateBox(nestB, 0, 1020).size();
  size_t pc = enumerateBox(nestC, -5, 210).size();
  std::cout << "Fig. 11.17(a): for i=1..29 / for j=i+2..39-i          " << la
            << " iterations (i actually only reaches 18)\n";
  std::cout << "Fig. 11.17(b): for i=10..1000 / for j=i..i+9           " << lb
            << " iterations\n";
  std::cout << "Fig. 11.17(c): for i / for j / for k=i+j..99-i-j       " << lc
            << " iterations (i,j really only reach 49)\n";
  std::cout << "polyhedron point counts (brute force): (a) " << pa << "  (b) "
            << pb << "  (c) " << pc << "   "
            << (((long)pa == la && (long)pb == lb && (long)pc == lc)
                    ? "== loop nests"
                    : "MISMATCH")
            << "\n";
}
