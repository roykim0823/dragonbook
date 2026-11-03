// 02_example_11_8_matrix_form.cpp -- Fig. 11.10 as a polyhedron: Example 11.6
// (the iteration space) and Example 11.8 (B x + b >= 0). Prints the four rows
// of Fig. 11.13 and counts the integer points two ways: by running the loop
// nest and by brute-force enumeration of the polyhedron. Both must give 33.
//
//   ./run.sh 2      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o
//   ex02 code/src/02_example_11_8_matrix_form.cpp && ./ex02
#include "enumerate.h"

int main() {
  Polyhedron poly; // Fig. 11.10:  for (i=0;i<=5;i++) for (j=i;j<=7;j++)
  poly.vars = {"i", "j"};
  // Each call appends one row  coefficients . (i, j) + constant >= 0:
  //    i     >= 0   (i >= 0)   poly.rows[0]: a = [ 1,  0]  c = 0
  //   -i + 5 >= 0   (i <= 5)   poly.rows[1]: a = [-1,  0]  c = 5
  //   -i + j >= 0   (j >= i)   poly.rows[2]: a = [-1,  1]  c = 0
  //   -j + 7 >= 0   (j <= 7)   poly.rows[3]: a = [ 0, -1]  c = 7
  poly.addInequality({{"i", 1}}, 0)
      .addInequality({{"i", -1}}, 5)
      .addInequality({{"i", -1}, {"j", 1}}, 0)
      .addInequality({{"j", -1}}, 7);
  printRows(poly, "Example 11.8: B i + b >= 0 for Fig. 11.10");

  long byLoop = 0; // count the iterations by running the nest
  for (long i = 0; i <= 5; i++)
    for (long j = i; j <= 7; j++)
      byLoop++;

  // enumerateBox tries every integer point of the square [lo, hi]^2 and keeps
  // those that satisfy all four rows, so the box only has to CONTAIN the
  // polyhedron: 0 <= i <= 5 and 0 <= j <= 7 fit inside [-2, 12]^2 with room to
  // spare. A box that is too small would silently undercount, so also check
  // that a much bigger box finds nothing new.
  size_t byPoly = enumerateBox(poly, -2, 12).size();
  size_t byBiggerBox = enumerateBox(poly, -20, 30).size();
  std::cout << "Fig. 11.10 original nest: " << byLoop << " iterations\n";
  std::cout << "integer points of the polyhedron (brute force): " << byPoly
            << "  " << ((long)byPoly == byLoop ? "== loop nest" : "MISMATCH")
            << "\n";
  std::cout << "box check: [-2,12]^2 gives " << byPoly << ", [-20,30]^2 gives "
            << byBiggerBox << "  "
            << (byBiggerBox == byPoly ? "(same: the box is big enough)"
                                      : "MISMATCH")
            << "\n";
}
