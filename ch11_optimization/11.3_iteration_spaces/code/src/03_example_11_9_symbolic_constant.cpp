// 03_example_11_9_symbolic_constant.cpp -- Example 11.9: a symbolic constant n
// is just one more column of B. The polyhedron of  for (i = 0; i <= n; i++)
// is built over (n, i) exactly like program 02 built Fig. 11.10; its integer
// points are all the (n, i) pairs, and a concrete n picks out one slice. For
// n = -10 (the book's guard n >= 0 is false) the slice is empty and the loop
// runs 0 times; for n = 10 both count 11 points.
//
//   ./run.sh 3      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o
//   ex03 code/src/03_example_11_9_symbolic_constant.cpp && ./ex03
#include "enumerate.h"

int main() {
  Polyhedron poly; // Example 11.9:  for (i = 0; i <= n; i++)
  poly.vars = {"n", "i"};
  poly.addInequality({{"i", 1}}, 0)             //  i     >= 0     i >= 0
      .addInequality({{"n", 1}, {"i", -1}}, 0); //  n - i >= 0     i <= n
  printRows(poly, "Example 11.9: symbolic constant n is just another column");

  // Brute force over (n, i). enumerateBox uses the same lo..hi for every
  // column, so the box is the square [-20, 20]^2: n from -20 to 20 and i from
  // -20 to 20, 41 * 41 = 1681 candidate points, from the corner (-20, -20) to
  // (20, 20). A candidate is kept only if both rows hold, so what comes back
  // is the triangle 0 <= i <= n <= 20 (231 points), for every n at once. As in
  // program 02 the box only has to CONTAIN the points asked about below, the
  // n values -10 and 10 with i = 0..10; a bigger box would keep more points
  // with n > 20 but change neither slice. Figure 03 in the guide draws it.
  std::set<Point> points = enumerateBox(poly, -20, 20);

  for (long value : {-10L, 10L}) {
    std::cout << "\n---- n = " << value << " ----\n";
    // The guard the book derives (Sec. 11.3.4): the nest runs at all only if
    // n >= 0. For a single loop the `for` itself enforces it.
    std::cout << "  guard n >= 0: " << (value >= 0 ? "true" : "FALSE") << "\n";

    long byLoop = 0; // count the iterations by running the loop with this n
    for (long i = 0; i <= value; i++)
      byLoop++;

    long byPoly = 0; // count the points of the slice n == value
    for (const Point &p : points)
      if (p[0] == value) // p = (n, i): column 0 is n
        byPoly++;

    std::cout << "  for (i = 0; i <= n; i++) with n = " << value << ": "
              << byLoop << " iterations\n";
    std::cout << "  integer points with n = " << value
              << " (brute force): " << byPoly << "  "
              << (byPoly == byLoop ? "== loop" : "MISMATCH") << "\n";
  }
}
