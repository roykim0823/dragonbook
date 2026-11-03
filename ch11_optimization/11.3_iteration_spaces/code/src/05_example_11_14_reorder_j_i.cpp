// 05_example_11_14_reorder_j_i.cpp -- Example 11.14: Algorithm 11.13 on
// Fig. 11.10 with the order (j, i), i.e. Example 11.10's loop. Shows both
// passes, emits the nest, verifies it against brute force, and re-checks the
// book's hand-written answer  for (j=0;j<=7;j++) for (i=0;i<=min(5,j);i++)  as
// a plain loop.
//
//   ./run.sh 5      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o
//   ex05 code/src/05_example_11_14_reorder_j_i.cpp && ./ex05
#include "experiment.h"

int main() {
  Polyhedron poly; // Fig. 11.10:  for (i=0;i<=5;i++) for (j=i;j<=7;j++)
  poly.vars = {"i", "j"};
  poly.addInequality({{"i", 1}}, 0)            //  i     >= 0     i >= 0
      .addInequality({{"i", -1}}, 5)           // -i + 5 >= 0     i <= 5
      .addInequality({{"i", -1}, {"j", 1}}, 0) // -i + j >= 0     j >= i
      .addInequality({{"j", -1}}, 7);          // -j + 7 >= 0     j <= 7
  // reorder(title, poly, loopOrder, boxLo, boxHi, body, verbose)
  reorder("Example 11.14: order j,i (sweep vertically)", poly, {"j", "i"}, -2,
          12, "Z[j][i] = 0;", true);

  std::set<IJ> ref; // the points the original nest visits
  for (long i = 0; i <= 5; i++)
    for (long j = i; j <= 7; j++)
      ref.insert({i, j});
  std::set<IJ> got; // the book's answer, as a plain loop
  long n = 0;       // its iteration count (catches a point visited twice)
  for (long j = 0; j <= 7; j++)
    for (long i = 0; i <= std::min(5L, j); i++) {
      got.insert({i, j});
      n++;
    }
  std::cout << "\nplain-loop check of the book's answer:\n";
  report("Example 11.14  for j / for i<=min(5,j)", ref, got, n);
}
