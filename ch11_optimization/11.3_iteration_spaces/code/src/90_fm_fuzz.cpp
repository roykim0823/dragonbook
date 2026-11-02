// 90_fm_fuzz.cpp -- stress test for the eliminator (fourier_motzkin.h +
// loop_bounds.h): random small polyhedra, random loop order, generated nest vs.
// brute force.  Also two hand-written regression cases for the pruning pass.
// Expected: "0 mismatches, 0 bound losses".  (Empty polyhedra are skipped; for
// those the nest runs zero times.)
//
//   ./run.sh 90      or      g++ -std=c++17 -O2 -Wall -Wextra -I code/include
//   -o ex90 code/src/90_fm_fuzz.cpp && ./ex90
#include "verify.h"

#include <random>

// Two cases that once broke the pruning pass (see the comment in
// dropImpliedBounds): a bound must survive when its only "proof" is a copy or
// an equivalent of itself.
static int regressions() {
  int bad = 0;
  Polyhedron poly; // duplicate upper bound: both used to be dropped
  poly.vars = {"i"};
  poly.ge0({{"i", 1}}, 0).ge0({{"i", -1}}, 5).ge0({{"i", -1}}, 5);
  Bounds bounds = computeBounds(poly, {"i"});
  if (bounds.upper[0].empty()) {
    ++bad;
    std::cout << "REGRESSION: duplicate bound lost\n";
  }
  Polyhedron poly2; // j <= i and 2j <= 2i+1 are equivalent over Z
  poly2.vars = {"i", "j"};
  poly2.ge0({{"i", 1}}, 0)
      .ge0({{"i", -1}}, 5)
      .ge0({{"j", 1}}, 0)
      .ge0({{"j", -1}, {"i", 1}}, 0)
      .ge0({{"j", -2}, {"i", 2}}, 1);
  Bounds bounds2 = computeBounds(poly2, {"i", "j"});
  if (bounds2.upper[1].empty()) {
    ++bad;
    std::cout << "REGRESSION: mutually-implying bounds lost\n";
  }
  return bad;
}

// A row with the given coefficients and constant (the variables have no names
// here, so ge0 cannot be used).
static Constraint row(std::vector<int64_t> a, int64_t c) {
  Constraint r;
  r.a = std::move(a);
  r.c = c;
  return r;
}

// A random polyhedron in d variables: the box [-3, 6]^d (so that brute force
// is finite) plus 1 to 3 random rows with small coefficients.
static Polyhedron randomPolyhedron(int d, std::mt19937 &rng) {
  Polyhedron poly;
  poly.vars = d == 2 ? std::vector<std::string>{"i", "j"}
                     : std::vector<std::string>{"i", "j", "k"};
  for (int v = 0; v < d; ++v) {
    std::vector<int64_t> unit(d, 0);
    unit[v] = 1;
    poly.rows.push_back(row(unit, 3)); //  x_v + 3 >= 0   i.e.  x_v >= -3
    unit[v] = -1;
    poly.rows.push_back(row(unit, 6)); // -x_v + 6 >= 0   i.e.  x_v <= 6
  }
  int extra = 1 + rng() % 3;
  for (int e = 0; e < extra; ++e) {
    std::vector<int64_t> a(d);
    for (int v = 0; v < d; ++v)
      a[v] = (int)(rng() % 7) - 3;     // coefficients in -3..3
    int64_t c = (int)(rng() % 15) - 5; // constant in -5..9
    Constraint extraRow = row(a, c);
    if (!isConstant(extraRow)) // all-zero coefficients: not a real row
      poly.rows.push_back(extraRow);
  }
  return poly;
}

int main() {
  int bad = regressions();
  std::mt19937 rng(12345); // fixed seed: the run is reproducible
  int lost = 0, empty = 0, tests = 0;
  for (int t = 0; t < 3000; ++t) {
    int d = 2 + (t % 2); // alternate 2 and 3 variables
    Polyhedron poly = randomPolyhedron(d, rng);

    // ground truth, then Algorithm 11.13 in a random loop order
    auto truth = enumerateBox(poly, -3, 6);
    std::vector<std::string> order = poly.vars;
    std::shuffle(order.begin(), order.end(), rng);
    Bounds bounds = computeBounds(poly, order);
    if (truth.empty() || bounds.empty) {
      ++empty;
      continue; // nothing to run
    }

    // every loop must have kept at least one lower and one upper bound
    bool unbounded = false;
    for (size_t i = 0; i < bounds.order.size(); ++i)
      if (bounds.lower[i].empty() || bounds.upper[i].empty())
        unbounded = true;
    if (unbounded) {
      ++lost;
      std::cout << "BOUND LOST (test " << t << ")\n";
      printRows(poly, "P");
      continue;
    }

    // the generated nest must visit exactly the brute-force points, once each
    auto visited = runNest(poly, bounds);
    std::set<Point> visitedSet(visited.begin(), visited.end());
    ++tests;
    if (visitedSet != truth || visitedSet.size() != visited.size()) {
      ++bad;
      std::cout << "MISMATCH (test " << t << ")\n";
      printRows(poly, "P");
      emit(poly, bounds, "...");
    }
  }
  std::cout << "fuzz: " << tests << " non-empty nests checked, " << empty
            << " empty skipped, " << bad << " mismatches, " << lost
            << " bound losses\n";
  return (bad + lost) ? 1 : 0;
}
