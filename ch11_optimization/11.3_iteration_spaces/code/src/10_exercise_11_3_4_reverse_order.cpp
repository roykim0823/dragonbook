// 10_exercise_11_3_4_reverse_order.cpp -- Exercise 11.3.4: reverse the nesting
// order of Fig. 11.17 (a), (b), (c) with Algorithm 11.13. (a) and (c) are also
// run in their original order first, as a sanity check (and to show the
// integer-tightened bounds of (c)). The hand-written answers are re-checked as
// plain loops.
//
//   ./run.sh 10      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include
//   -o ex10 code/src/10_exercise_11_3_4_reverse_order.cpp && ./ex10
#include "experiment.h"

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
  // brute-force boxes that contain each polyhedron (see program 02: any
  // containing box gives the same point set)
  const int64_t loA = -5, hiA = 60, loB = 0, hiB = 1020, loC = -5, hiC = 210;
  // reorder(title, poly, loopOrder, boxLo, boxHi, body, verbose)
  reorder("(a) original order i,j", nestA, {"i", "j"}, loA, hiA, "X[i][j] = 0;",
          false);
  reorder("(a) reversed order j,i", nestA, {"j", "i"}, loA, hiA, "X[i][j] = 0;",
          true);
  reorder("(b) reversed order j,i", nestB, {"j", "i"}, loB, hiB, "X[i][j] = 0;",
          false);
  reorder("(c) original order i,j,k (note the tightened bounds)", nestC,
          {"i", "j", "k"}, loC, hiC, "X[i][j][k] = 0;", false);
  reorder("(c) reversed order k,j,i", nestC, {"k", "j", "i"}, loC, hiC,
          "X[i][j][k] = 0;", true);

  // the points the original nests visit, for the plain-loop checks below
  std::set<IJ> refA, refB;
  std::set<IJK> refC;
  for (long i = 1; i < 30; i++)
    for (long j = i + 2; j < 40 - i; j++)
      refA.insert({i, j});
  for (long i = 10; i <= 1000; i++)
    for (long j = i; j < i + 10; j++)
      refB.insert({i, j});
  for (long i = 0; i < 100; i++)
    for (long j = 0; j < 100 + i; j++)
      for (long k = i + j; k < 100 - i - j; k++)
        refC.insert({i, j, k});
  std::cout << "\nplain-loop checks of the answers:\n";
  // Each block below runs one hand-written answer as a plain loop, collects
  // the points it visits and how often it iterated, and compares with the
  // original nest (report prints one line per answer).
  {
    std::set<IJ> s;
    long n = 0;
    for (long j = 3; j <= 38; j++)
      for (long i = 1; i <= std::min(j - 2, 39 - j); i++) {
        s.insert({i, j});
        n++;
      }
    report("(a) for j=3..38 / for i=1..min(j-2,39-j)", refA, s, n);
  }
  {
    std::set<IJ> s;
    long n = 0;
    for (long j = 10; j <= 1009; j++)
      for (long i = std::max(10L, j - 9); i <= std::min(1000L, j); i++) {
        s.insert({i, j});
        n++;
      }
    report("(b) for j=10..1009 / for i=max(10,j-9)..min(1000,j)", refB, s, n);
  }
  {
    std::set<IJK> s;
    long n = 0;
    for (long k = 0; k <= 99; k++)
      for (long j = 0; j <= std::min(k, 99 - k); j++)
        for (long i = 0; i <= std::min(k - j, 99 - k - j); i++) {
          s.insert({i, j, k});
          n++;
        }
    report("(c) for k / for j / for i", refC, s, n);
  }
}
