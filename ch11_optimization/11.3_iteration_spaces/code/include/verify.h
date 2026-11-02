// verify.h -- the ground truth. Nothing here is from the book; it is what lets
// every generated nest in the README be CHECKED rather than trusted:
//   enumerateBox  lists the integer points of a polyhedron by brute force,
//   runNest       executes a generated loop nest the way the emitted C would,
//   compare       checks that the two point sets are the same (no point missed,
//                 no point visited twice),
//   report        the one-line check used for hand-written loops.
#ifndef DRAGONBOOK_CH11_11_3_VERIFY_H
#define DRAGONBOOK_CH11_11_3_VERIFY_H

#include "loop_bounds.h"

#include <iomanip>
#include <limits>
#include <set>
#include <tuple>

// One integer point: x[m] is the value of variable column m.
using Point = std::vector<int64_t>;

// The value of  a.x + c  for one row at point x.
inline int64_t evaluate(const Constraint &row, const Point &x) {
  int64_t sum = row.c;
  for (size_t m = 0; m < x.size(); ++m)
    sum += row.a[m] * x[m];
  return sum;
}

// Is x inside the polyhedron? Every row has to come out >= 0.
inline bool contains(const Polyhedron &poly, const Point &x) {
  for (const Constraint &row : poly.rows)  // point x is inside if it satisfies every row
    if (evaluate(row, x) < 0)
      return false;
  return true;
}

// Advance x to the next point of the box [lo, hi]^d, counting like a car's
// odometer: the last coordinate ticks up; when it passes hi it wraps back to
// lo and the coordinate before it ticks up instead. Returns false when x was
// the last point (every coordinate was at hi).
inline bool nextPointInBox(Point &x, int64_t lo, int64_t hi) {
  for (int d = (int)x.size() - 1; d >= 0; --d) {
    if (x[d] < hi) {
      ++x[d];
      return true;
    }
    x[d] = lo; // this coordinate wraps; carry into the one before it
  }
  return false;
}

// Brute force: every integer point of the box [lo, hi]^d that satisfies all
// rows. The box is the caller's guess of a region that CONTAINS the
// polyhedron; any box that does gives the same answer, a box that is too
// small undercounts. (Program 02 shows how to check the guess: a bigger box
// must find nothing new.) Cost: (hi - lo + 1)^d membership tests.
inline std::set<Point> enumerateBox(const Polyhedron &poly, int64_t lo,
                                    int64_t hi) {
  std::set<Point> inside;
  Point x(poly.vars.size(), lo); // start at the corner (lo, lo, ..., lo)
  do {
    if (contains(poly, x))
      inside.insert(x);
  } while (nextPointInBox(x, lo, hi));
  return inside;
}

// The numeric value of one bound row for loop variable v, given the values of
// the outer variables already stored in x (x[v] itself is ignored). This is
// boundExpr evaluated instead of printed, with the same ceil / floor rounding
// as the emitted ceil_div / floor_div.
inline int64_t boundValue(const Constraint &row, int v, const Point &x) {
  // rest = the row with the v term removed, evaluated at x
  int64_t rest = row.c;
  for (size_t m = 0; m < x.size(); ++m)
    if ((int)m != v)
      rest += row.a[m] * x[m];
  int64_t coef = row.a[v];
  if (coef > 0)
    return ceilDiv(-rest, coef); // coef*v + rest >= 0  ==>  v >= -rest/coef
  return floorDiv(rest, -coef);  // coef*v + rest >= 0  ==>  v <= rest/(-coef)
}

// Run loop level i (0 = outermost) of the generated nest and recurse inward.
// Every point reached by the innermost body is appended to `visited`, in the
// order the generated code would visit it.
inline void runLevel(const Bounds &bounds, size_t i, Point &x,
                     std::vector<Point> &visited) {
  if (i == bounds.order.size()) {
    visited.push_back(x); // all loop variables set: this is one iteration
    return;
  }
  int v = bounds.order[i];
  // for (v = max(lower bounds); v <= min(upper bounds); v++)
  int64_t lo = std::numeric_limits<int64_t>::min();
  int64_t hi = std::numeric_limits<int64_t>::max();
  for (const Constraint &row : bounds.lower[i])
    lo = std::max(lo, boundValue(row, v, x));
  for (const Constraint &row : bounds.upper[i])
    hi = std::min(hi, boundValue(row, v, x));
  for (int64_t value = lo; value <= hi; ++value) {
    x[v] = value;
    runLevel(bounds, i + 1, x, visited);
  }
}

// Run the whole generated nest and return the points it visits, in order.
inline std::vector<Point> runNest(const Polyhedron &poly,
                                  const Bounds &bounds) {
  // A loop without a lower or without an upper bound would run forever.
  for (size_t i = 0; i < bounds.order.size(); ++i)
    if (bounds.lower[i].empty() || bounds.upper[i].empty())
      throw std::logic_error("runNest: loop over " +
                             poly.vars[bounds.order[i]] +
                             " has no lower or no upper bound");
  std::vector<Point> visited;
  Point x(poly.vars.size(), 0);
  runLevel(bounds, 0, x, visited);
  return visited;
}

// "[1,2,3]"
inline std::string pointStr(const Point &p) {
  std::string s = "[";
  for (size_t i = 0; i < p.size(); ++i)
    s += (i ? "," : "") + std::to_string(p[i]);
  return s + "]";
}

// Generated nest vs brute force. `truth` is the point set from enumerateBox,
// `visited` the points the generated nest produced. They must be the same set
// AND have the same count, so that no point was visited twice. The visiting
// order may differ, that is the whole point of reordering loops.
inline void compare(const std::string &label, const std::set<Point> &truth,
                    const std::vector<Point> &visited) {
  std::set<Point> visitedSet(visited.begin(), visited.end());
  bool sameSet = (visitedSet == truth);
  bool noDuplicates = (visitedSet.size() == visited.size());
  bool ok = sameSet && noDuplicates;
  std::cout << "  verify " << label << ": " << visited.size() << " iterations, "
            << (ok ? "IDENTICAL point set to brute force (no duplicates)"
                   : "MISMATCH!")
            << "\n";
  std::cout << "  first iterations in generated order: ";
  for (size_t i = 0; i < std::min<size_t>(visited.size(), 10); ++i)
    std::cout << pointStr(visited[i]) << " ";
  std::cout << (visited.size() > 10 ? "...\n" : "\n");
}

// Hand-written rewrite vs original loop, both run as plain C++ loops that
// collect (i, j[, k]) tuples. `ref` is the set the original nest visits, `got`
// the set the rewrite visits, `nGot` how many iterations the rewrite ran (so a
// rewrite that visits a point twice is caught too).
using IJ = std::pair<long, long>;
using IJK = std::tuple<long, long, long>;
template <class S>
inline void report(const char *what, const S &ref, const S &got, long nGot) {
  bool ok = (ref == got) && (long)got.size() == nGot;
  std::cout << "  " << std::left << std::setw(52) << what << " " << std::right
            << std::setw(6) << got.size() << " pts  "
            << (ok ? "== original" : "MISMATCH") << "\n";
}

#endif // DRAGONBOOK_CH11_11_3_VERIFY_H
