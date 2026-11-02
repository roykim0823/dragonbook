// loop_bounds.h -- Algorithm 11.13: the loop bounds for a given loop order
// (Fig. 11.15), and turning them into C-style `for` loops with max()/min()
// and ceil_div/floor_div.
#ifndef DRAGONBOOK_CH11_11_3_LOOP_BOUNDS_H
#define DRAGONBOOK_CH11_11_3_LOOP_BOUNDS_H

#include "fourier_motzkin.h"

// The result of Algorithm 11.13. Loop i (0 = outermost) runs the variable
// order[i] from max(lower[i]) to min(upper[i]); each entry of lower[i] /
// upper[i] is a row of the polyhedron that bounds that variable, still in
// row form (boundExpr turns a row into the C text).
struct Bounds {
  std::vector<int> order;                            // column of each loop
  std::vector<std::vector<Constraint>> lower, upper; // bound rows per loop
  bool empty = false; // the polyhedron has no points: the nest runs 0 times
};

// Pass 2 of Algorithm 11.13 for one loop and one direction. `candidates` are
// the lower (or upper) bound rows of the loop, `otherDirection` the rows of
// the opposite direction, and `outerBounds` the bounds already kept for the
// outer loops (the book's S'). Returns the candidates that are NOT implied by
// the rest and therefore have to stay in the generated max() / min().
inline std::vector<Constraint>
dropImpliedBounds(const Polyhedron &poly, const Polyhedron &outerBounds,
                  const std::vector<Constraint> &candidates,
                  const std::vector<Constraint> &otherDirection, bool verbose) {
  std::vector<Constraint> kept;
  for (size_t j = 0; j < candidates.size(); ++j) {
    // The context that may imply candidates[j]:
    //   S'  +  the other direction's bounds  +  the candidates not decided yet
    //   (q > j)  +  the candidates already kept.
    // Candidates already DROPPED are left out on purpose: FM often produces
    // two rows that imply each other, and using a dropped one as context
    // would remove both and leave the loop with no bound at all.
    Polyhedron context = outerBounds;
    for (size_t q = j + 1; q < candidates.size(); ++q)
      context.rows.push_back(candidates[q]);
    for (const Constraint &row : otherDirection)
      context.rows.push_back(row);
    for (const Constraint &row : kept)
      context.rows.push_back(row);

    if (implied(context, candidates[j])) {
      if (verbose)
        std::cout << "  redundant: " << rowStr(poly, candidates[j]) << "\n";
    } else {
      kept.push_back(candidates[j]);
    }
  }
  return kept;
}

// Algorithm 11.13: bounds for the loops in the order given by name
// (orderNames[0] is the outermost loop).
inline Bounds computeBounds(const Polyhedron &poly,
                            const std::vector<std::string> &orderNames,
                            bool verbose = false) {
  Bounds bounds;
  for (const std::string &name : orderNames)
    bounds.order.push_back(poly.column(name));
  int n = (int)bounds.order.size();
  bounds.lower.resize(n);
  bounds.upper.resize(n);

  // Pass 1 (Fig. 11.15, first loop): innermost -> outermost. At loop i every
  // outer variable already has a value, so EVERY row that still mentions the
  // loop variable is a legal bound for it: record those rows, then project
  // the variable away so the remaining rows only talk about outer variables.
  Polyhedron projected = poly;
  for (int i = n - 1; i >= 0; --i) {
    int v = bounds.order[i];
    for (const Constraint &row : projected.rows) {
      if (row.a[v] > 0)
        bounds.lower[i].push_back(row);
      else if (row.a[v] < 0)
        bounds.upper[i].push_back(row);
    }
    projected = eliminate(projected, v, verbose);
    if (verbose)
      printRows(projected, "  S_" + std::to_string(i) +
                               " (after projecting away " + poly.vars[v] + ")");
  }

  // After the last projection only constants are left (the book's S_0). A
  // false one, e.g. "-3 >= 0", means some loop has max(lower) > min(upper)
  // whatever the outer values are: the nest runs zero times. Pruning would be
  // meaningless then (an empty set implies everything), so stop here.
  for (const Constraint &row : projected.rows) {
    if (isConstant(row) && row.c < 0) {
      bounds.empty = true;
      return bounds;
    }
  }

  // Pass 2 (Fig. 11.15, second loop): outermost -> innermost, drop every bound
  // that is implied by S' (the outer bounds already kept) together with the
  // other bounds of the same variable. (The book uses only S'; see the guide.)
  Polyhedron outerBounds; // S'
  outerBounds.vars = poly.vars;
  for (int i = 0; i < n; ++i) {
    bounds.lower[i] = dropImpliedBounds(poly, outerBounds, bounds.lower[i],
                                        bounds.upper[i], verbose);
    bounds.upper[i] = dropImpliedBounds(poly, outerBounds, bounds.upper[i],
                                        bounds.lower[i], verbose);
    // the bounds kept for this loop become context for the inner loops
    for (const Constraint &row : bounds.lower[i])
      outerBounds.rows.push_back(row);
    for (const Constraint &row : bounds.upper[i])
      outerBounds.rows.push_back(row);
  }
  return bounds;
}

// ---------- code emission ----------------------------------------------------
// Solve one bound row for the loop variable v and print the bound as C text.
//     lower bound    c1*v + R >= 0  (c1 > 0)   ==>   v >= ceil(-R / c1)
//     upper bound   -c2*v + R >= 0  (c2 > 0)   ==>   v <= floor(R / c2)
// where R is the rest of the row: the other variables and the constant.
// With a coefficient of +-1 no division is needed and no ceil_div/floor_div is
// printed.
inline std::string boundExpr(const Polyhedron &poly, const Constraint &row,
                             int v) {
  int64_t coef = row.a[v];
  // R: the row with the v term removed
  std::vector<int64_t> rest = row.a;
  rest[v] = 0;
  int64_t restConstant = row.c;

  if (coef > 0) {
    // lower bound: v >= -R / coef, rounded up
    for (int64_t &x : rest)
      x = -x;
    restConstant = -restConstant;
    std::string text = exprStr(poly, rest, restConstant, v);
    if (coef == 1)
      return text;
    return "ceil_div(" + text + ", " + std::to_string(coef) + ")";
  }
  // upper bound: v <= R / (-coef), rounded down
  std::string text = exprStr(poly, rest, restConstant, v);
  if (coef == -1)
    return text;
  return "floor_div(" + text + ", " + std::to_string(-coef) + ")";
}

// The C text of every row in `rows`, each solved for loop variable v.
inline std::vector<std::string>
boundExprs(const Polyhedron &poly, const std::vector<Constraint> &rows, int v) {
  std::vector<std::string> out;
  for (const Constraint &row : rows)
    out.push_back(boundExpr(poly, row, v));
  return out;
}

// Nest several bounds as "max(a, max(b, c))" (f = "max") or "min(...)". A
// single bound needs no wrapper; no bound at all cannot be turned into a loop.
inline std::string joinMinMax(const std::vector<std::string> &exprs,
                              const std::string &f) {
  if (exprs.empty())
    return "<unbounded>";
  std::string nested = exprs.back();
  for (int i = (int)exprs.size() - 2; i >= 0; --i)
    nested = f + "(" + exprs[i] + ", " + nested + ")";
  return nested;
}

// "for (j = max(0, i); j <= min(7, i + 3); j++)" for loop number i of bounds.
inline std::string loopHeader(const Polyhedron &poly, const Bounds &bounds,
                              size_t i) {
  int v = bounds.order[i];
  const std::string &name = poly.vars[v];
  std::string lo = joinMinMax(boundExprs(poly, bounds.lower[i], v), "max");
  std::string hi = joinMinMax(boundExprs(poly, bounds.upper[i], v), "min");
  return "for (" + name + " = " + lo + "; " + name + " <= " + hi + "; " + name +
         "++)";
}

// Print the L / U lists of Fig. 11.15 for every loop.
inline void printBounds(const Polyhedron &poly, const Bounds &bounds) {
  std::cout << "  bounds (Algorithm 11.13):\n";
  for (size_t i = 0; i < bounds.order.size(); ++i) {
    int v = bounds.order[i];
    std::cout << "    L_" << poly.vars[v] << ":";
    for (const std::string &text : boundExprs(poly, bounds.lower[i], v))
      std::cout << "  " << text;
    std::cout << "\n";
    std::cout << "    U_" << poly.vars[v] << ":";
    for (const std::string &text : boundExprs(poly, bounds.upper[i], v))
      std::cout << "  " << text;
    std::cout << "\n";
  }
}

// Print the generated nest: one `for` per loop, indented, then the body.
inline void emit(const Polyhedron &poly, const Bounds &bounds,
                 const std::string &body) {
  for (size_t i = 0; i < bounds.order.size(); ++i)
    std::cout << std::string(2 * i, ' ') << loopHeader(poly, bounds, i) << "\n";
  std::cout << std::string(2 * bounds.order.size(), ' ') << body << "\n";
}

#endif // DRAGONBOOK_CH11_11_3_LOOP_BOUNDS_H
