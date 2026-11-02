// experiment.h -- one complete "reorder / change axes" experiment, as the
// numbered programs run it: substitute new axes (Sec. 11.3.6) if asked,
// compute the bounds (Algorithm 11.13), print the generated nest, run it, and
// verify the visited points against brute force on the ORIGINAL polyhedron.
#ifndef DRAGONBOOK_CH11_11_3_EXPERIMENT_H
#define DRAGONBOOK_CH11_11_3_EXPERIMENT_H

#include "verify.h"

// transform[r][c] is row r, column c of the matrix T below.
using Matrix = std::vector<std::vector<int64_t>>;

// Affine change of axes  x = T*y + t   (x: old variables, y: new variables).
// Substituting into an old row gives a row over the new variables:
//     a.x + c >= 0   ==>   a.(T*y + t) + c >= 0   ==>   (a T).y + (a.t + c) >=
//     0
inline Polyhedron changeAxes(const Polyhedron &poly,
                             const std::vector<std::string> &newVars,
                             const Matrix &transform,
                             const std::vector<int64_t> &offset) {
  Polyhedron newPoly;
  newPoly.vars = newVars;
  for (const Constraint &row : poly.rows) {
    Constraint newRow;
    newRow.a.assign(newVars.size(), 0);
    newRow.c = row.c;
    for (size_t r = 0; r < row.a.size(); ++r) {
      for (size_t c = 0; c < newVars.size(); ++c)
        newRow.a[c] += row.a[r] * transform[r][c]; // (a T)_c
      newRow.c += row.a[r] * offset[r];            // a.t
    }
    newPoly.rows.push_back(normalize(newRow));
  }
  return newPoly;
}

// The same substitution applied to one point:  x = T*y + t.
inline Point mapBack(const Point &y, const Matrix &transform,
                     const std::vector<int64_t> &offset) {
  Point x(transform.size(), 0);
  for (size_t r = 0; r < transform.size(); ++r) {
    x[r] = offset[r];
    for (size_t c = 0; c < y.size(); ++c)
      x[r] += transform[r][c] * y[c];
  }
  return x;
}

// Print the generated nest. Returns false if the polyhedron turned out empty:
// then there is nothing to run or verify.
inline bool printGeneratedNest(const Polyhedron &newPoly, const Bounds &bounds,
                               const std::string &body) {
  std::cout << "  generated code:\n";
  emit(newPoly, bounds, body);
  if (bounds.empty) {
    std::cout << "  ** polyhedron is EMPTY: the generated nest runs 0 "
                 "iterations; pruning skipped **\n";
    return false;
  }
  return true;
}

// Reorder the loops of `poly` (newVars == poly.vars, transform = identity) or
// change its axes (x = transform * y + offset), then loop in `order`
// (outermost first). lo..hi is the brute-force box for the ORIGINAL
// variables, chosen as in program 02. `body` is only printed.
inline void experiment(const std::string &title, const Polyhedron &poly,
                       const std::vector<std::string> &newVars,
                       const Matrix &transform,
                       const std::vector<int64_t> &offset,
                       const std::vector<std::string> &order, int64_t lo,
                       int64_t hi, const std::string &body, bool verbose) {
  std::cout << "\n==== " << title << " ====\n";

  // 1. substitute the new axes (a pure reorder keeps the rows as they are)
  bool pureReorder = (newVars == poly.vars);
  Polyhedron newPoly =
      pureReorder ? poly : changeAxes(poly, newVars, transform, offset);
  if (!pureReorder)
    printRows(newPoly, "  constraints after substitution");

  // 2. Algorithm 11.13 in the requested loop order
  Bounds bounds = computeBounds(newPoly, order, verbose);
  printBounds(newPoly, bounds);
  if (!printGeneratedNest(newPoly, bounds, body))
    return;

  // 3. run the generated nest, translate every visited point back to the
  //    original variables (for a pure reorder the identity matrix is a no-op)
  std::vector<Point> visited;
  for (const Point &y : runNest(newPoly, bounds))
    visited.push_back(mapBack(y, transform, offset));

  // 4. the verdict: same point set as brute force on the original polyhedron
  compare(title, enumerateBox(poly, lo, hi), visited);
}

// A change of axes that is NOT unimodular. Instead of substituting, keep the
// old variables, add the new axis variable `newVar = sum coeff*old` as a first
// column pinned down by an EQUALITY (written as two >= rows), and let
// Algorithm 11.13 handle the rest. The old variable that the equality
// determines gets a ceil/floor pair as bounds, i.e. an inner loop that runs
// once or not at all; that is where the "stride" of a non-unimodular map shows
// up. The loop body stays in the old variables.
inline void experimentEquality(
    const std::string &title, const Polyhedron &poly, const char *newVar,
    std::initializer_list<std::pair<const char *, int64_t>> definition,
    const std::vector<std::string> &order, int64_t lo, int64_t hi,
    const std::string &body, bool verbose) {
  std::cout << "\n==== " << title << " ====\n";

  // 1. a copy of the polyhedron with newVar as an extra first column
  Polyhedron newPoly = poly;
  newPoly.vars.insert(newPoly.vars.begin(), newVar);
  for (Constraint &row : newPoly.rows)
    row.a.insert(row.a.begin(), 0); // existing rows do not mention newVar

  // 2. the equality  newVar - sum(coeff*old) = 0  as two inequalities:
  //    newVar - sum(coeff*old) >= 0   and   -(newVar - sum(coeff*old)) >= 0
  Constraint equality;
  equality.a.assign(newPoly.vars.size(), 0);
  equality.a[0] = 1;
  for (const auto &term : definition)
    equality.a[newPoly.column(term.first)] -= term.second;
  Constraint negatedEquality = equality;
  for (int64_t &coef : negatedEquality.a)
    coef = -coef;
  newPoly.rows.push_back(equality);
  newPoly.rows.push_back(negatedEquality);
  printRows(newPoly, "  constraints with the equality " + std::string(newVar) +
                         " = ... added as two rows");

  // 3. Algorithm 11.13 in the requested loop order
  Bounds bounds = computeBounds(newPoly, order, verbose);
  if (!printGeneratedNest(newPoly, bounds, body))
    return;

  // 4. run the nest; dropping the first coordinate (newVar) gives back the
  //    original variables
  std::vector<Point> visited;
  for (const Point &y : runNest(newPoly, bounds))
    visited.push_back(Point(y.begin() + 1, y.end()));

  // 5. the verdict
  compare(title, enumerateBox(poly, lo, hi), visited);
}

#endif // DRAGONBOOK_CH11_11_3_EXPERIMENT_H
