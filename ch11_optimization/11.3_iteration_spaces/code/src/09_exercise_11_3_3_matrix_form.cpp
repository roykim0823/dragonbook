// 09_exercise_11_3_3_matrix_form.cpp -- Exercise 11.3.3: the B x + b >= 0 form
// of Fig. 11.17 (a), (b), (c).
//
//   ./run.sh 9      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o
//   ex09 code/src/09_exercise_11_3_3_matrix_form.cpp && ./ex09
#include "poly.h"

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
  std::cout << "Exercise 11.3.3: rows of B x + b >= 0 ('<' bounds written as "
               "'<=' by subtracting 1)\n";
  printRows(nestA, "(a)");
  printRows(nestB, "(b)");
  printRows(nestC, "(c)");
}
