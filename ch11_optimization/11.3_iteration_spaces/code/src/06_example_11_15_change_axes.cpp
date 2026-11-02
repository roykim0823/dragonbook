// 06_example_11_15_change_axes.cpp -- Example 11.15: change of axes k = j - i
// (diagonals), order (k, j); Sec. 11.3.6. Old (i, j) in terms of new (k, j):  i
// = j - k,  j = j,  i.e. T = [[-1, 1], [0, 1]], t = 0.
//
//   ./run.sh 6      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o
//   ex06 code/src/06_example_11_15_change_axes.cpp && ./ex06
#include "experiment.h"

int main() {
  Polyhedron poly; // Fig. 11.10:  for (i=0;i<=5;i++) for (j=i;j<=7;j++)
  poly.vars = {"i", "j"};
  poly.ge0({{"i", 1}}, 0)            //  i     >= 0     i >= 0
      .ge0({{"i", -1}}, 5)           // -i + 5 >= 0     i <= 5
      .ge0({{"i", -1}, {"j", 1}}, 0) // -i + j >= 0     j >= i
      .ge0({{"j", -1}}, 7);          // -j + 7 >= 0     j <= 7
  const Matrix transform{
      {-1, 1}, {0, 1}}; // (i, j) = transform * (k, j) + offset:  i = j - k
  const std::vector<int64_t> offset{0, 0};
  // experiment(title, poly, newVars, transform, offset, loopOrder, boxLo,
  // boxHi,
  //            body, verbose): newVars == poly.vars means a pure reorder
  experiment("Example 11.15: k = j - i, order k,j", poly, {"k", "j"}, transform,
             offset, {"k", "j"}, -2, 12, "Z[j][j-k] = 0;", false);

  std::set<IJ> ref; // the points the original nest visits
  for (long i = 0; i <= 5; i++)
    for (long j = i; j <= 7; j++)
      ref.insert({i, j});
  std::set<IJ> got; // the book's answer, as a plain loop
  long n = 0;       // its iteration count (catches a point visited twice)
  for (long k = 0; k <= 7; k++)
    for (long j = k; j <= std::min(5 + k, 7L); j++) {
      got.insert({j - k, j});
      n++;
    }
  std::cout << "\nplain-loop check of the book's answer:\n";
  report("Example 11.15  for k / for j (i = j-k)", ref, got, n);
}
