// 14_exercise_11_3_8a_antidiagonal_axis.cpp -- Exercise 11.3.8(a): new
// outermost axis s = i + j (anti-diagonals), inner loop j; i = s - j. Old (i,
// j) from new (s, j): T = [[1, -1], [0, 1]] (unimodular).
//
//   ./run.sh 14      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include
//   -o ex14 code/src/14_exercise_11_3_8a_antidiagonal_axis.cpp && ./ex14
#include "experiment.h"

int main() {
  // The three loop nests of Fig. 11.17 as polyhedra ('<' bounds written as
  // '<=' by subtracting 1).
  Polyhedron nestA; // (a) for (i=1;i<30;i++) for (j=i+2;j<40-i;j++)
  nestA.vars = {"i", "j"};
  nestA.ge0({{"i", 1}}, -1)
      .ge0({{"i", -1}}, 29)             //  1 <= i <= 29
      .ge0({{"j", 1}, {"i", -1}}, -2)   //  j - i - 2 >= 0   j >= i + 2
      .ge0({{"j", -1}, {"i", -1}}, 39); // -j - i + 39 >= 0  j <= 39 - i
  Polyhedron nestB; // (b) for (i=10;i<=1000;i++) for (j=i;j<i+10;j++)
  nestB.vars = {"i", "j"};
  nestB.ge0({{"i", 1}}, -10)
      .ge0({{"i", -1}}, 1000)         // 10 <= i <= 1000
      .ge0({{"j", 1}, {"i", -1}}, 0)  //  j - i >= 0      j >= i
      .ge0({{"j", -1}, {"i", 1}}, 9); // -j + i + 9 >= 0  j <= i + 9
  Polyhedron nestC; // (c) for (i=0;i<100;i++) for (j=0;j<100+i;j++)
                    //     for (k=i+j;k<100-i-j;k++)
  nestC.vars = {"i", "j", "k"};
  nestC.ge0({{"i", 1}}, 0)
      .ge0({{"i", -1}}, 99) // 0 <= i <= 99
      .ge0({{"j", 1}}, 0)
      .ge0({{"j", -1}, {"i", 1}}, 99)              // 0 <= j <= 99 + i
      .ge0({{"k", 1}, {"i", -1}, {"j", -1}}, 0)    // k >= i + j
      .ge0({{"k", -1}, {"i", -1}, {"j", -1}}, 99); // k <= 99 - i - j
  // brute-force boxes that contain each polyhedron (see program 02: any
  // containing box gives the same point set)
  const int64_t loA = -5, hiA = 60, loB = 0, hiB = 1020, loC = -5, hiC = 210;
  // (i, j[, k]) from (s, j[, k]):  i = s - j
  const Matrix transform2{{1, -1}, {0, 1}},
      transform3{{1, -1, 0}, {0, 1, 0}, {0, 0, 1}};
  const std::vector<int64_t> zero2{0, 0}, zero3{0, 0, 0};
  // experiment(title, poly, newVars, transform, offset, loopOrder, boxLo,
  // boxHi,
  //            body, verbose): newVars == poly.vars means a pure reorder
  experiment("(a) s = i + j", nestA, {"s", "j"}, transform2, zero2, {"s", "j"},
             loA, hiA, "X[s-j][j] = 0;", false);
  experiment("(b) s = i + j", nestB, {"s", "j"}, transform2, zero2, {"s", "j"},
             loB, hiB, "X[s-j][j] = 0;", false);
  experiment("(c) s = i + j", nestC, {"s", "j", "k"}, transform3, zero3,
             {"s", "j", "k"}, loC, hiC, "X[s-j][j][k] = 0;", false);

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
    for (long t = 4; t <= 39; t++)
      for (long j = ceilDiv(t + 2, 2); j <= t - 1; j++) {
        s.insert({t - j, j});
        n++;
      }
    report("(a) for s=4..39 / for j=ceil((s+2)/2)..s-1", refA, s, n);
  }
  {
    std::set<IJ> s;
    long n = 0;
    for (long t = 20; t <= 2009; t++)
      for (long j = std::max(t - 1000, ceilDiv(t, 2));
           j <= std::min(t - 10, floorDiv(t + 9, 2)); j++) {
        s.insert({t - j, j});
        n++;
      }
    report("(b) for s=20..2009 / for j=...", refB, s, n);
  }
  {
    std::set<IJK> s;
    long n = 0;
    for (long t = 0; t <= 49; t++)
      for (long j = 0; j <= t; j++)
        for (long k = t; k <= 99 - t; k++) {
          s.insert({t - j, j, k});
          n++;
        }
    report("(c) for s=0..49 / for j=0..s / for k=s..99-s", refC, s, n);
  }
}
