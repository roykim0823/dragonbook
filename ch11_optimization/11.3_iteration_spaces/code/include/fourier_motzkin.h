// fourier_motzkin.h -- Algorithm 11.11: Fourier-Motzkin elimination
// (projecting one variable away), and the two questions built on it that the
// pruning pass of Algorithm 11.13 asks: "is this polyhedron empty?" and "is
// this row implied by these others?".
#ifndef DRAGONBOOK_CH11_11_3_FOURIER_MOTZKIN_H
#define DRAGONBOOK_CH11_11_3_FOURIER_MOTZKIN_H

#include "poly.h"

// Combine one lower bound and one upper bound on x_m into a row without x_m.
//     lo:   c1*x_m + Lrest >= 0   (c1 > 0)   i.e.   x_m >= -Lrest / c1
//     up:  -c2*x_m + Urest >= 0   (c2 > 0)   i.e.   x_m <=  Urest / c2
// Multiply lo by c2 and up by c1 and add them: the x_m terms cancel
// (c2*c1 - c1*c2 = 0) and what is left is
//     c2*Lrest + c1*Urest >= 0,       the book's   c2*L <= c1*U.
inline Constraint combine(const Constraint &lo, const Constraint &up, int m) {
  int64_t c1 = lo.a[m];
  int64_t c2 = -up.a[m];
  Constraint sum;
  sum.a.resize(lo.a.size());
  for (size_t k = 0; k < sum.a.size(); ++k)
    sum.a[k] = c2 * lo.a[k] + c1 * up.a[k]; // column m becomes 0
  sum.c = c2 * lo.c + c1 * up.c;
  // book step 2: divide out the common factor (normalize also floors the
  // constant, the integer tightening explained in poly.h)
  return normalize(sum);
}

// Project variable column m away (Algorithm 11.11). The result is
//     (the rows of poly that do not mention x_m)
//   + (one combined row for every (lower bound, upper bound) pair).
// The eliminated column stays in the table but is 0 in every surviving row.
// With verbose = true every pair tried is printed; the README traces are this.
inline Polyhedron eliminate(const Polyhedron &poly, int m,
                            bool verbose = false) {
  Polyhedron result;
  result.vars = poly.vars;

  // Sort the rows into three piles by the sign of the x_m coefficient: the
  // lower and upper bounds together are the book's C, the third pile is S - C
  // and passes straight through to S' (part of book step 4).
  std::vector<Constraint> lower, upper;
  for (const Constraint &row : poly.rows) {
    if (row.a[m] > 0)
      lower.push_back(row);
    else if (row.a[m] < 0)
      upper.push_back(row);
    else
      result.rows.push_back(row); // does not mention x_m: keep as is
  }
  if (verbose) {
    std::cout << "  eliminating " << poly.vars[m] << ": " << lower.size()
              << " lower bound(s), " << upper.size()
              << " upper bound(s) -> at most " << lower.size() * upper.size()
              << " new rows\n";
  }

  // Book step 1: every lower bound paired with every upper bound gives a new
  // row (combine() also does step 2 on it).
  for (const Constraint &lo : lower) {
    for (const Constraint &up : upper) {
      Constraint combined = combine(lo, up, m);
      if (verbose) {
        std::cout << "    (" << rowStr(poly, lo) << ")  x  ("
                  << rowStr(poly, up) << ")  =>  " << rowStr(poly, combined);
        if (isConstant(combined))
          std::cout << (combined.c >= 0 ? "   [always true, dropped]"
                                        : "   [INFEASIBLE]");
        std::cout << "\n";
      }
      // A row like "5 >= 0" carries no information: drop it. A row like
      // "-3 >= 0" is kept so that callers can see the polyhedron is empty:
      // book step 3 (unsatisfiable row => S is empty) is left to them,
      // infeasible() below and computeBounds() in loop_bounds.h.
      if (isConstant(combined) && combined.c >= 0)
        continue;
      result.rows.push_back(combined); // book step 4: S' = (S - C) + new rows
    }
  }
  return result;
}

// Eliminate every variable. If a false constant such as "-3 >= 0" remains, the
// polyhedron has no rational point and therefore no integer point either.
// (The converse is not guaranteed: rational feasibility does not imply that an
// integer point exists. For bound pruning only the "infeasible" answer is
// used, so this one-sided test is enough.)
inline bool infeasible(Polyhedron poly) {
  for (size_t m = 0; m < poly.vars.size(); ++m)
    poly = eliminate(poly, (int)m);
  for (const Constraint &row : poly.rows)
    if (isConstant(row) && row.c < 0)
      return true;
  return false;
}

// Is `row` implied by the rows of `poly`?
// It is exactly when  poly AND NOT(row)  has no solution. Over the integers
//     NOT(a.x + c >= 0)   is   a.x + c <= -1,   i.e.   -a.x - c - 1 >= 0.
inline bool implied(const Polyhedron &poly, const Constraint &row) {
  Constraint negated;
  negated.a = row.a;
  for (int64_t &coef : negated.a)
    coef = -coef;
  negated.c = -row.c - 1;

  Polyhedron test = poly;
  test.rows.push_back(negated);
  return infeasible(test);
}

#endif // DRAGONBOOK_CH11_11_3_FOURIER_MOTZKIN_H
