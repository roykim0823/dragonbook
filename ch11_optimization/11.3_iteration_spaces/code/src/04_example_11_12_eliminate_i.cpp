// 04_example_11_12_eliminate_i.cpp -- Example 11.12: Fourier-Motzkin
// elimination (Algorithm 11.11) of i from Fig. 11.10, step by step. One lower
// bound (0 <= i) times two upper bounds (i <= 5, i <= j) gives two new rows;
// one is trivial. Then the book's remark after the algorithm is checked by
// brute force: for every j near the shape, is there an integer i, and does the
// projection S' admit that j exactly when there is?
//
//   ./run.sh 4      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o
//   ex04 code/src/04_example_11_12_eliminate_i.cpp && ./ex04
#include "verify.h"

int main() {
  Polyhedron poly; // Fig. 11.10:  for (i=0;i<=5;i++) for (j=i;j<=7;j++)
  poly.vars = {"i", "j"};
  poly.ge0({{"i", 1}}, 0)            //  i     >= 0     i >= 0
      .ge0({{"i", -1}}, 5)           // -i + 5 >= 0     i <= 5
      .ge0({{"i", -1}, {"j", 1}}, 0) // -i + j >= 0     j >= i
      .ge0({{"j", -1}}, 7);          // -j + 7 >= 0     j <= 7
  std::cout << "Example 11.12: project away i (verbose)\n";
  Polyhedron shadow = eliminate(poly, poly.column("i"), true);
  printRows(shadow, "  result");

  // Brute force: every integer point of S in a box around the shape.
  std::set<Point> points = enumerateBox(poly, -2, 12);
  std::cout << "\nper-j check: which j have an integer i in S, and does S' "
               "admit exactly those?\n";
  bool allOk = true;
  for (int64_t j = -1; j <= 8; ++j) {
    std::vector<int64_t> is; // the i values of S on this j (set order = sorted)
    for (const Point &p : points)
      if (p[1] == j)
        is.push_back(p[0]);
    bool exists = !is.empty();
    // The i column of every row of S' is 0, so any i (here 0) can stand in.
    bool admitted = contains(shadow, Point{0, j});
    std::cout << "  j = " << std::setw(2) << j << ":  ";
    if (exists)
      std::cout << "i in " << is.front() << ".." << is.back() << "  ("
                << is.size() << (is.size() == 1 ? " value) " : " values)");
    else
      std::cout << "no integer i         ";
    std::cout << "   S' admits j: " << (admitted ? "yes" : "no ") << "   "
              << (admitted == exists ? "OK" : "MISMATCH") << "\n";
    allOk = allOk && (admitted == exists);
  }
  std::cout << "  verify: S' admits exactly the j that have some i: "
            << (allOk ? "IDENTICAL" : "MISMATCH") << "\n";
}
