// 13_exercise_11_3_7_diagonal_axis.cpp -- Exercise 11.3.7: new outermost axis
// along the main diagonal, d = j - i, inner loop j; i = j - d. Old (i, j) from
// new (d, j): T = [[-1, 1], [0, 1]] (unimodular). For (c), k is kept as the
// third variable.
//
//   ./run.sh 13      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include
//   -o ex13 code/src/13_exercise_11_3_7_diagonal_axis.cpp && ./ex13
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
  // (i, j[, k]) from (d, j[, k]):  i = j - d
  const Matrix transform2{{-1, 1}, {0, 1}},
      transform3{{-1, 1, 0}, {0, 1, 0}, {0, 0, 1}};
  const std::vector<int64_t> zero2{0, 0}, zero3{0, 0, 0};
  // experiment(title, poly, newVars, transform, offset, loopOrder, boxLo,
  // boxHi,
  //            body, verbose): newVars == poly.vars means a pure reorder
  experiment("(a) d = j - i", nestA, {"d", "j"}, transform2, zero2, {"d", "j"},
             loA, hiA, "X[j-d][j] = 0;", false);
  experiment("(b) d = j - i", nestB, {"d", "j"}, transform2, zero2, {"d", "j"},
             loB, hiB, "X[j-d][j] = 0;", false);
  experiment("(c) d = j - i", nestC, {"d", "j", "k"}, transform3, zero3,
             {"d", "j", "k"}, loC, hiC, "X[j-d][j][k] = 0;", false);

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
    for (long d = 2; d <= 37; d++)
      for (long j = d + 1; j <= floorDiv(d + 39, 2); j++) {
        s.insert({j - d, j});
        n++;
      }
    report("(a) for d=2..37 / for j=d+1..floor((d+39)/2)", refA, s, n);
  }
  {
    std::set<IJ> s;
    long n = 0;
    for (long d = 0; d <= 9; d++)
      for (long j = d + 10; j <= d + 1000; j++) {
        s.insert({j - d, j});
        n++;
      }
    report("(b) for d=0..9 / for j=d+10..d+1000", refB, s, n);
  }
  {
    std::set<IJK> s;
    long n = 0;
    for (long d = -49; d <= 49; d++)
      for (long j = std::max(d, 0L); j <= floorDiv(d + 49, 2); j++)
        for (long k = 2 * j - d; k <= d - 2 * j + 99; k++) {
          s.insert({j - d, j, k});
          n++;
        }
    report("(c) for d / for j / for k", refC, s, n);
  }
}
