// enumerate.h -- the ground truth. Nothing here is from the book; it is the
// brute force that every generated nest is CHECKED against: enumerateBox lists
// the integer points of a polyhedron by trying every point of a box. It needs
// only the data model (poly.h), so the programs of 11.3.3 and 11.3.4 can use it
// before any elimination or loop-bound code exists.
#ifndef DRAGONBOOK_CH11_11_3_ENUMERATE_H
#define DRAGONBOOK_CH11_11_3_ENUMERATE_H

#include "poly.h"

#include <set>
#include <vector>

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
  // point x is inside if it satisfies every row
  for (const Constraint &row : poly.rows)
    if (evaluate(row, x) < 0)
      return false;
  return true;
}

// Advance x to the next point of the box [lo, hi]^d, counting like a car's
// odometer: the last coordinate ticks up; when it passes hi it wraps back to
// lo and the coordinate before it ticks up instead. Returns false when x was
// the last point (every coordinate was at hi).
// Example, d = 2, lo = 0, hi = 2, starting from x = (0, 0): successive calls
// turn x into
//     (0,1)  (0,2)  (1,0)  (1,1)  (1,2)  (2,0)  (2,1)  (2,2)  then false
// The step (0,2) -> (1,0) is the wrap: x[1] is at hi, so it goes back to lo
// and x[0] ticks up. After the last point, x[1] and x[0] both wrap and
// nothing is left to tick, so the function returns false with x = (0, 0).
// The order is that of the nested loop  for x[0] in lo..hi, for x[1] in lo..hi.
inline bool nextPointInBox(Point &x, int64_t lo, int64_t hi) {
  for (int d = (int)x.size() - 1; d >= 0; --d) {
    if (x[d] < hi) {
      ++x[d]; // this coordinate ticks up, no wrap
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

#endif // DRAGONBOOK_CH11_11_3_ENUMERATE_H
