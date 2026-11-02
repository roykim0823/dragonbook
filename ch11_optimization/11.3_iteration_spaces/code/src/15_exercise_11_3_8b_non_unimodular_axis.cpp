// 15_exercise_11_3_8b_non_unimodular_axis.cpp -- Exercise 11.3.8(b): replace j
// by m = i - 2j, outermost loop along m. As stated (keep i): the map (i,j) ->
// (m,i) has determinant 2, not unimodular, so j = (i-m)/2 is not always an
// integer. We keep j and add m = i - 2j as an equality; j's loop then runs 0 or
// 1 times. Variant (keep j inner): i = m + 2j is unimodular and every loop is a
// plain for. Both are re-checked as plain loops; the literal form uses the
// guard if ((i-m)%2==0).
//
//   ./run.sh 15      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include
//   -o ex15 code/src/15_exercise_11_3_8b_non_unimodular_axis.cpp && ./ex15
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
  std::cout << "########## Exercise 11.3.8(b) as stated: replace j by m = i - "
               "2j, keep i ##########\n";
  // experimentEquality(title, poly, newVar, definition, loopOrder, boxLo,
  //                    boxHi, body, verbose): adds newVar = definition as an
  //                    equality instead of substituting
  experimentEquality("(a) m = i - 2j, order m,i (j determined)", nestA, "m",
                     {{"i", 1}, {"j", -2}}, {"m", "i", "j"}, loA, hiA,
                     "X[i][j] = 0;", false);
  experimentEquality("(b) m = i - 2j, order m,i (j determined)", nestB, "m",
                     {{"i", 1}, {"j", -2}}, {"m", "i", "j"}, loB, hiB,
                     "X[i][j] = 0;", false);
  experimentEquality("(c) m = i - 2j, order m,i,k (j determined)", nestC, "m",
                     {{"i", 1}, {"j", -2}}, {"m", "i", "j", "k"}, loC, hiC,
                     "X[i][j][k] = 0;", false);

  std::cout << "\n########## Exercise 11.3.8(b) variant: outer axis m = i - "
               "2j, keep j inner ##########\n";
  // (i, j[, k]) from (m, j[, k]):  i = m + 2j  (determinant 1)
  const Matrix transform2{{1, 2}, {0, 1}},
      transform3{{1, 2, 0}, {0, 1, 0}, {0, 0, 1}};
  const std::vector<int64_t> zero2{0, 0}, zero3{0, 0, 0};
  // experiment(title, poly, newVars, transform, offset, loopOrder, boxLo,
  // boxHi,
  //            body, verbose): newVars == poly.vars means a pure reorder
  experiment("(a) m = i - 2j", nestA, {"m", "j"}, transform2, zero2, {"m", "j"},
             loA, hiA, "X[m+2*j][j] = 0;", false);
  experiment("(b) m = i - 2j", nestB, {"m", "j"}, transform2, zero2, {"m", "j"},
             loB, hiB, "X[m+2*j][j] = 0;", false);
  experiment("(c) m = i - 2j", nestC, {"m", "j", "k"}, transform3, zero3,
             {"m", "j", "k"}, loC, hiC, "X[m+2*j][j][k] = 0;", false);

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
    for (long m = -75; m <= -5; m++)
      for (long j = ceilDiv(1 - m, 2);
           j <= std::min(-m - 2, floorDiv(39 - m, 3)); j++) {
        s.insert({m + 2 * j, j});
        n++;
      }
    report("(a) variant: for m=-75..-5 / for j=...", refA, s, n);
    s.clear();
    n = 0;
    for (long m = -75; m <= -5; m++)
      for (long i = 1; i <= std::min(-m - 4, floorDiv(m + 78, 3)); i++)
        if ((i - m) % 2 == 0) {
          s.insert({i, (i - m) / 2});
          n++;
        }
    report("(a) literal: for m / for i / if ((i-m)%2==0)", refA, s, n);
  }
  {
    std::set<IJ> s;
    long n = 0;
    for (long m = -1018; m <= -10; m++)
      for (long j = std::max(ceilDiv(10 - m, 2), -m - 9);
           j <= std::min(floorDiv(1000 - m, 2), -m); j++) {
        s.insert({m + 2 * j, j});
        n++;
      }
    report("(b) variant: for m=-1018..-10 / for j=...", refB, s, n);
    s.clear();
    n = 0;
    for (long m = -1018; m <= -10; m++)
      for (long i = std::max(10L, -m - 18); i <= std::min(1000L, -m); i++)
        if ((i - m) % 2 == 0) {
          s.insert({i, (i - m) / 2});
          n++;
        }
    report("(b) literal: for m / for i / if ((i-m)%2==0)", refB, s, n);
  }
  {
    std::set<IJK> s;
    long n = 0;
    for (long m = -98; m <= 49; m++)
      for (long j = std::max(ceilDiv(-m, 2), 0L); j <= floorDiv(49 - m, 3); j++)
        for (long k = m + 3 * j; k <= 99 - m - 3 * j; k++) {
          s.insert({m + 2 * j, j, k});
          n++;
        }
    report("(c) variant: for m / for j / for k", refC, s, n);
    s.clear();
    n = 0;
    for (long m = -98; m <= 49; m++)
      for (long i = std::max(0L, m); i <= floorDiv(m + 98, 3); i++)
        if ((i - m) % 2 == 0) {
          long j = (i - m) / 2;
          for (long k = i + j; k <= 99 - i - j; k++) {
            s.insert({i, j, k});
            n++;
          }
        }
    report("(c) literal: for m / for i / if / for k", refC, s, n);
  }
}
