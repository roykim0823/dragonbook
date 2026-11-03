# The code bundle

Part of [11.3 Iteration Spaces](../README.md) · previous: [Beyond §11.3](beyond_11.3.md)

Everything in this file is about the C++ bundle rather than the book: why the examples are C++, the shared headers in `code/include/` quoted verbatim, how to read the trace a program prints, and the fuzzer that guards the eliminator. The subsection files quote each program in full but only *point* here for the shared code.

## Which language for the examples?

**C++ for the loop nests** — the chapter is about C-style `for` loops, and C++ lets us verify the rewrites directly with `std::set` (the plain-loop checks in the numbered programs, through `report()` in `verify.h`). One pitfall: integer `/` truncates toward zero, so the generated `⌈·⌉`/`⌊·⌋` bounds need floor/ceil division helpers (`floorDiv`/`ceilDiv` in `intdiv.h`, quoted in [the code bundle](code_bundle.md#intdivh)).

**C++ is also fine for the eliminator itself** (`fourier_motzkin.h`, `loop_bounds.h`): every quantity stays an integer if you combine rows as `c₂·lo + c₁·up` and normalise by gcd, so no rational-number type is needed. Python would be shorter, not clearer.

**For a production reference**, look at `isl` (Integer Set Library), used by LLVM Polly and MLIR's `affine` dialect: it implements Algorithm 11.13 with integer tightening and far stronger redundancy elimination, and its string syntax `{ [i,j] : 0 <= i <= 5 and i <= j <= 7 }` is exactly Eq. (11.1).

## The shared headers, in dependency order

The headers are quoted one at a time, in the order they include each other:

```
    intdiv.h  →  poly.h  →  enumerate.h  →  fourier_motzkin.h  →  loop_bounds.h  →  verify.h  →  experiment.h
    floor/ceil   rows, print   brute force    Algorithm 11.11      Algorithm 11.13    run nest,     one whole
    division     normalise     (ground truth) eliminate, implied   bounds, emitter    verdict       experiment
```

Every block is one contiguous stretch of the file's text, verbatim (the two excerpts from inside `eliminate()` are re-indented). Each header says which program uses it first, so this file can be read on its own or opened from the code example that first needs it.

### intdiv.h

First used by program 04, through `normalize()`, and from program 05 on directly by the emitter (`boundExpr`) and the checker (`boundValue`); the table of values is in [§11.3.5](11.3.5_controlling_execution_order.md), under *Integers vs. rationals*.

C++ agrees with `floorDiv` for a positive quotient and with `ceilDiv` for a negative one, which is why code that uses `/` for a loop bound works on the positive test cases and fails on the negative ones. `floorDiv` fixes the one case where truncation is wrong, an inexact division with a negative quotient, and `ceilDiv` is defined through it by `ceil(a / b) = -floor(-a / b)`.

**code/include/intdiv.h** (`floorDiv`, `ceilDiv`)
```cpp
// floor(a / b): the largest integer that is <= the exact quotient.
template <class A, class B> inline std::common_type_t<A, B> floorDiv(A a, B b) {
  using T = std::common_type_t<A, B>;
  T x = a, y = b;
  T q = x / y; // quotient truncated toward zero
  T r = x % y; // remainder; it is 0 exactly when the division is exact
  // Truncating and flooring only differ when the division is inexact AND the
  // exact quotient is negative, i.e. x and y have opposite signs. In C++ the
  // remainder r has the sign of x, so that is the test on r below. In that
  // case the truncated quotient is one too large.
  bool inexact = (r != 0);
  bool negativeQuotient = (r < 0) != (y < 0);
  if (inexact && negativeQuotient)
    return q - 1;
  return q;
}

// ceil(a / b): the smallest integer that is >= the exact quotient.
// Uses the identity  ceil(a / b) == -floor(-a / b).
template <class A, class B> inline std::common_type_t<A, B> ceilDiv(A a, B b) {
  using T = std::common_type_t<A, B>;
  T x = a;
  return -floorDiv(-x, b);
}
```

### poly.h

First used by program 02 ([§11.3.3](11.3.3_matrix_formulation.md), code example 02), which builds Fig. 11.10 row by row.

The two types every program is built from. A `Constraint` is one row: the coefficient vector `a` (one entry per column of `B`) and the constant `c`. A `Polyhedron` is the list of variable names, which fixes what each column means, plus its rows. `addInequality` appends one row from `{variable, coefficient}` pairs and a constant, so a program can write each inequality on its own line; `column` turns a name into its column index and throws on a misspelt name, so a typo cannot silently become a wrong row.

**code/include/poly.h** (`Constraint`, and `Polyhedron` with `column()` and `addInequality()`)
```cpp
struct Constraint {
  std::vector<int64_t> a; // one coefficient per variable (a row of B)
  int64_t c = 0;          // the constant (the matching entry of b)
};

struct Polyhedron {
  std::vector<std::string> vars; // column names: vars[m] names column m
  std::vector<Constraint> rows;  // the inequalities, all of the form ... >= 0

  // The column index of a variable name. A misspelt name throws instead of
  // silently producing a wrong row.
  int column(const std::string &name) const {
    for (size_t m = 0; m < vars.size(); ++m)
      if (vars[m] == name)
        return (int)m;
    throw std::invalid_argument("Polyhedron: no variable named '" + name + "'");
  }

  // Append the row  sum(coef * var) + c >= 0  and return *this so calls chain.
  // Example:  addInequality({{"i", -1}, {"j", 1}}, 0)  appends  -i + j >= 0.
  // Every call builds a FRESH row: nothing carries over from one call to the
  // next, so a chain of four calls appends four independent rows.
  Polyhedron &
  addInequality(std::initializer_list<std::pair<const char *, int64_t>> terms,
                int64_t c) {
    Constraint row; // new local row for this call, every coefficient 0
    row.a.assign(vars.size(), 0);
    row.c = c;
    // += (not =) only matters when one call names the same variable twice:
    // {{"i", 1}, {"i", 2}} gives 3*i rather than silently keeping the last.
    for (const auto &term : terms)
      row.a[column(term.first)] += term.second;
    rows.push_back(row); // copy it into the polyhedron; `row` dies here
    return *this;
  }
};
```

The printing part. Program 02 is the first to call it (`printRows`). `printRows` writes the title and the variable names, then one row per line through `rowStr`, which is `exprStr` plus `" >= 0"`. `exprStr` renders a coefficient vector and a constant as text the way one would write it by hand: a coefficient of ±1 prints as `i` or `-i` rather than `1*i`, a zero coefficient is skipped, and a row with no variable terms prints just its constant (that is how `5 >= 0` appears in the traces of §11.3.5). `signStr` only decides whether a term gets `-`, ` + ` or ` - ` in front, which depends on whether it is the first term.

**code/include/poly.h** (printing: `signStr`, `exprStr`, `rowStr`, `printRows`)
```cpp
// ---------- printing ---------------------------------------------------------
// The sign written in front of a term: the first term gets "-" or nothing,
// every later term gets " - " or " + ".
inline std::string signStr(bool firstTerm, int64_t value) {
  if (firstTerm)
    return value < 0 ? "-" : "";
  return value < 0 ? " - " : " + ";
}

// Render  a[0]*x_0 + ... + c  as text such as "2*i - j + 5". Columns with a
// zero coefficient are left out (boundExpr relies on that: it zeroes the loop
// variable's column before printing the rest of the row).
inline std::string exprStr(const Polyhedron &poly,
                           const std::vector<int64_t> &a, int64_t c) {
  std::string out;
  for (size_t m = 0; m < a.size(); ++m) {
    if (a[m] == 0)
      continue; // nothing to print for this column
    out += signStr(out.empty(), a[m]);
    if (std::llabs(a[m]) != 1)
      out += std::to_string(std::llabs(a[m])) + "*"; // "2*i", but "i" not "1*i"
    out += poly.vars[m];
  }
  if (out.empty())
    return std::to_string(c); // no variable terms: print the constant alone
  if (c != 0)
    out += signStr(false, c) + std::to_string(std::llabs(c));
  return out;
}

// One row as "2*i - j + 5 >= 0".
inline std::string rowStr(const Polyhedron &poly, const Constraint &row) {
  return exprStr(poly, row.a, row.c) + " >= 0";
}

// Print a title, the variable names, then every row on its own line.
inline void printRows(const Polyhedron &poly, const std::string &title) {
  std::cout << title << "  (vars: ";
  for (const std::string &name : poly.vars)
    std::cout << name << " ";
  std::cout << ")\n";
  for (const Constraint &row : poly.rows)
    std::cout << "    " << rowStr(poly, row) << "\n";
}
```

`normalize()` is Algorithm 11.11's step 2 strengthened for integers: divide the row by the gcd of its coefficients and floor the constant (the reasoning is in [§11.3.5](11.3.5_controlling_execution_order.md), *Integers vs. rationals*). `combine()` calls it on every new row.

**code/include/poly.h** (`normalize()`)
```cpp
// ---------- normalisation (Alg. 11.11 step 2, strengthened for integers) ----
// Divide every coefficient by their common factor g and FLOOR the constant:
//     g*(a'.x) + c >= 0   and   x integer    ==>    a'.x + floor(c / g) >= 0
// The book's step 2 only divides out the common factor (rational reasoning).
// Flooring the constant is the standard tightening for integer points and is
// what turns e.g.  -2i - 2j + 99 >= 0  into  -i - j + 49 >= 0.
inline Constraint normalize(Constraint row) {
  // g = gcd of all |coefficients|  (gcd(0, v) == v, so starting at 0 is fine)
  int64_t g = 0;
  for (int64_t coef : row.a)
    g = std::gcd(g, std::llabs(coef));
  if (g > 1) {
    for (int64_t &coef : row.a)
      coef /= g;
    row.c = floorDiv(row.c, g);
  }
  return row;
}
```

The pairing loop of `eliminate()` needs one small helper: `isConstant` says whether a row has no variable term left, so that it is just a number against zero. That is the row the book's step 3 looks at: `5 >= 0` is dropped, `-3 >= 0` would mean an empty polyhedron.

**code/include/poly.h** (`isConstant()`)
```cpp
// A row whose coefficients are all 0 is just "c >= 0": always true when
// c >= 0, impossible when c < 0. Such rows appear when FM eliminates the last
// variable that a pair of bounds had in common.
inline bool isConstant(const Constraint &row) {
  for (int64_t coef : row.a)
    if (coef != 0)
      return false;
  return true;
}
```

### enumerate.h

First used by program 02 ([§11.3.3](11.3.3_matrix_formulation.md), code example 02). `enumerateBox` is the ground truth that every later verification rests on; it needs only poly.h, which is why it has a header of its own and why programs 02, 03, 04 and 08 can use it without the elimination and loop-bound code. `evaluate` computes `a·x + c` for one row at the point, and `contains` rejects the point at the first row that comes out negative. `nextPointInBox` moves to the next point of the box like a car's odometer: the last coordinate ticks up, and whenever a coordinate passes `hi` it wraps back to `lo` and the coordinate before it ticks up instead; it returns `false` once every coordinate has wrapped, which means every point has been visited. `enumerateBox` starts at `(lo, lo, …)` and simply tests one point per tick; the same `lo` and `hi` apply to every column, so the box is always a cube, and figure 03 in [§11.3.4](11.3.4_symbolic_constants.md) shows one call, the 1681 candidates and the 231 points kept. The cost is `(hi − lo + 1)^d` membership tests, which is fine for the small boxes used here (the largest, the exercise nest Fig. 11.17(c) of §11.3.7, is about ten million points) and is exactly why the box should be generous but not absurd.

**code/include/enumerate.h** (`Point`, `evaluate`, `contains`, `nextPointInBox`, `enumerateBox`: brute force)
```cpp
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
```

### fourier_motzkin.h

First used by program 04 ([§11.3.5](11.3.5_controlling_execution_order.md), code example 04). `eliminate()` starts by sorting the rows into three piles by the sign of the coefficient of the variable to eliminate, which is Fact 1 of §11.3.3 in code:

**code/include/fourier_motzkin.h** (inside `eliminate()`)
```cpp
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
```

The two pieces that produce the trace of program 04. `combine()` is book step 1 for one pair, the row sum `c₂·lo + c₁·up` (with `normalize()` as book step 2); the pairing loop in `eliminate()` calls it for every (lower, upper) pair, prints the trace line, and drops a trivially true constant row while keeping a false one so that step 3 can be detected by the caller:

**code/include/fourier_motzkin.h** (`combine()`: one pair)
```cpp
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
```

The pairing loop that calls it sits inside `eliminate()`, after the three-pile sort quoted above (re-indented here):

**code/include/fourier_motzkin.h** (inside `eliminate()`: the pairing loop)
```cpp
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
```

`infeasible()` runs the elimination to the very end, variable after variable, and reports whether a false constant is left; that is the book's step 3 asked about a whole polyhedron. It is one-sided on purpose: a false constant proves emptiness, while its absence only proves a rational point exists.

**code/include/fourier_motzkin.h** (`infeasible()`)
```cpp
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
```

The test itself is `implied()` in `fourier_motzkin.h`: add the negation of the bound to the context and run Fourier–Motzkin to the end; the bound is implied exactly when a false constant appears. Over the integers the negation of `a·x + c ≥ 0` is `−a·x − c − 1 ≥ 0`.

**code/include/fourier_motzkin.h** (`implied()`)
```cpp
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
```

### loop_bounds.h

First used by program 05 ([§11.3.5](11.3.5_controlling_execution_order.md), code example 05). `computeBounds()` is Algorithm 11.13 itself and produces everything in program 05's trace above the `generated code:` line. The book's Fig. 11.15 maps onto it line by line:

| Fig. 11.15 (book page 798) | code in `computeBounds()` |
|---|---|
| `S_n = S` | `Polyhedron projected = poly;` |
| `for (i = n; i >= 1; i--)`: `L_vi`, `U_vi` = all lower / upper bounds on `v_i` in `S_i` | the first loop, `i` from `n − 1` down to `0`: every row of `projected` with a positive coefficient of the loop variable goes to `bounds.lower[i]`, negative to `bounds.upper[i]` |
| `S_{i−1}` = Algorithm 11.11 applied to eliminate `v_i` from `S_i` | `projected = eliminate(projected, v, verbose);` and the `S_i` printouts of the trace |
| (not in Fig. 11.15) | a false constant in `S_0` sets `bounds.empty` and returns: the nest runs zero times |
| `S' = ∅` | `Polyhedron outerBounds;` |
| `for (i = 1; i <= n; i++)`: remove any bounds in `L_vi`, `U_vi` implied by `S'` | the second loop: `dropImpliedBounds()` once for the lower and once for the upper bounds; each call asks `implied(context, bound)` per candidate and prints `redundant:` for a dropped one |
| add the remaining constraints on `v_i` to `S'` | the two `outerBounds.rows.push_back` loops at the end of the second loop |

**code/include/loop_bounds.h** (`computeBounds()`: Pass 1 and Pass 2 of Fig. 11.15)
```cpp
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
```

`dropImpliedBounds()` is Pass 2 for one loop and one direction. It differs from Fig. 11.15 in what counts as context (see *Where the tool goes beyond Fig. 11.15* in [§11.3.5](11.3.5_controlling_execution_order.md)): not only `S'` but also the other direction's bounds, the candidates not yet decided, and the candidates already kept, never a candidate already dropped.

**code/include/loop_bounds.h** (`dropImpliedBounds()`: Pass 2 for one loop, one direction)
```cpp
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
```

The rest of `loop_bounds.h` is the emitter, which turns the result of `computeBounds` into the `generated code:` lines of the trace. `computeBounds` returns a `Bounds`: for each loop in the given order, the rows that bound that loop's variable from below and from above. `boundExpr` solves one such row for the loop variable, which is Fact 1 of §11.3.3 in code plus the rounding of *Integers vs. rationals* in §11.3.5: move the other terms across, flip the sign for an upper bound, and wrap in `ceil_div` or `floor_div` when the coefficient is not ±1. `joinMinMax` nests several bounds into `max(a, max(b, c))` or `min(...)`, `loopHeader` puts the two together into the text of one `for (...)` line, and `emit` prints one such line per loop plus the body.

**code/include/loop_bounds.h** (`Bounds`: the result of Algorithm 11.13)
```cpp
// The result of Algorithm 11.13. Loop i (0 = outermost) runs the variable
// order[i] from max(lower[i]) to min(upper[i]); each entry of lower[i] /
// upper[i] is a row of the polyhedron that bounds that variable, still in
// row form (boundExpr turns a row into the C text).
struct Bounds {
  std::vector<int> order;                            // column of each loop
  std::vector<std::vector<Constraint>> lower, upper; // bound rows per loop
  bool empty = false; // the polyhedron has no points: the nest runs 0 times
};
```

`computeBounds` and `dropImpliedBounds`, which fill a `Bounds`, are the two quotes above; the emitter follows them in the file.

**code/include/loop_bounds.h** (the emitter: `boundExpr`, `boundExprs`, `joinMinMax`, `loopHeader`, `printBounds`, `emit`)
```cpp
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
    std::string text = exprStr(poly, rest, restConstant);
    if (coef == 1)
      return text;
    return "ceil_div(" + text + ", " + std::to_string(coef) + ")";
  }
  // upper bound: v <= R / (-coef), rounded down
  std::string text = exprStr(poly, rest, restConstant);
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
```

### verify.h

First used by program 05 ([§11.3.5](11.3.5_controlling_execution_order.md), code example 05). This header runs a generated nest and checks it against the points `enumerateBox` found; it is the half of the checker that needs the `Bounds` type of loop_bounds.h. `runNest` is how a generated nest is executed without compiling it. `boundValue` is `boundExpr` evaluated instead of printed: one bound row, at the current values of the outer variables, with the same `ceilDiv`/`floorDiv` rounding the emitted code would use. `runLevel` runs one loop level: take the maximum of the lower-bound values and the minimum of the upper-bound values, and for every value in between recurse one loop inward; when the innermost level is reached the point is recorded. `pointStr` formats one point as `[1,2]`. `compare` is the verdict line in every trace, and `report` is the one-line comparison the plain-loop checks print.

**code/include/verify.h** (`boundValue`, `runLevel`, `runNest`, `pointStr`, `compare`, `report`: running and checking a generated nest)
```cpp
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
```

### experiment.h

First used by program 05 ([§11.3.5](11.3.5_controlling_execution_order.md), code example 05). `experiment` is the driver that every reorder and change-of-axes program from here on calls; `reorder` is the same with the identity substitution filled in, which is what the pure-reorder programs 05 and 10 call. It does, in order: substitute new axes if asked (`changeAxes`, below), run `computeBounds` in the requested order, print the bounds and the generated nest (`printGeneratedNest`, which also stops early when the polyhedron is empty), then **verify**: `runNest` executes the generated nest exactly as the emitted code would, `mapBack` converts each visited point to the original variables, `enumerateBox` produces the truth, and `compare` checks that the two point sets are equal and that no point was visited twice. The two numbers before the body are the brute-force box, chosen as in program 02.

`experiment` does not take a list of symbolic constants (§11.3.4): a program adds the constant as the first entry of `vars`, puts it first in the `order` argument, and ignores the first loop the emitter prints. The bounds `computeBounds` finds for that "loop" are the conditions under which the nest is non-empty (the `n ≥ 0` of Example 11.9), not a loop to run.

**code/include/experiment.h** (`printGeneratedNest()`, `experiment()` and `reorder()`: the driver)
```cpp
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

// A pure reorder: the same variables, looped in a different order. This is
// experiment() with the identity substitution (x = y), which is what programs
// 05 and 10 do.
inline void reorder(const std::string &title, const Polyhedron &poly,
                    const std::vector<std::string> &order, int64_t lo,
                    int64_t hi, const std::string &body, bool verbose) {
  size_t d = poly.vars.size();
  Matrix identity(d, std::vector<int64_t>(d, 0));
  for (size_t k = 0; k < d; ++k)
    identity[k][k] = 1;
  experiment(title, poly, poly.vars, identity, std::vector<int64_t>(d, 0),
             order, lo, hi, body, verbose);
}
```

First used by program 06 ([§11.3.6](11.3.6_changing_axes.md), code example 06). `changeAxes` is the one matrix multiply of §11.3.6 in code: with `x_old = transform · y_new + offset`, each old row `a·x + c ≥ 0` becomes `(a·transform)·y + (a·offset + c) ≥ 0`, and the new row is normalised. `mapBack` is the same substitution applied to a point, used to translate the points the generated nest visits back into `(i, j)` for the comparison.

**code/include/experiment.h** (`changeAxes()` and `mapBack()`)
```cpp
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
```

First used by program 15 ([§11.3.7](11.3.7_exercises.md), Exercise 11.3.8(b)). `experimentEquality` is the variant of the driver for a change of axes that is not unimodular. Instead of substituting, it keeps the old variables, adds the new axis variable as a first column, and pins it down with the defining equation written as **two** rows (`m − i + 2j ≥ 0` and `−m + i − 2j ≥ 0`). Algorithm 11.13 then does the rest, and the determined variable comes out with a `ceil_div`/`floor_div` pair of bounds. When comparing with brute force the extra first coordinate is dropped.

**code/include/experiment.h** (`experimentEquality()`)
```cpp
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
```

## How to read the program output

The part of program 10's output for Exercise 11.3.4(a), with annotations added at the right:

**./run.sh 10** (excerpt, annotated)
```
==== (a) reversed order j,i ====
  eliminating i: 1 lower bound(s), 3 upper bound(s) -> at most 3 new rows     ← Pass 1, Alg 11.11
    (i - 1 >= 0)  x  (-i + 29 >= 0)  =>  28 >= 0   [always true, dropped]
    (i - 1 >= 0)  x  (-i + j - 2 >= 0)  =>  j - 3 >= 0
    (i - 1 >= 0)  x  (-i - j + 39 >= 0)  =>  -j + 38 >= 0
  S_1 (after projecting away i)  (vars: i j )                                  ← the shadow on j
    j - 3 >= 0
    -j + 38 >= 0
  eliminating j: 1 lower bound(s), 1 upper bound(s) -> at most 1 new rows
    (j - 3 >= 0)  x  (-j + 38 >= 0)  =>  35 >= 0   [always true, dropped]
  S_0 (after projecting away j)  (vars: i j )                                  ← empty = all constants OK
  redundant: -i + 29 >= 0                                                      ← Pass 2 pruning
  bounds (Algorithm 11.13):
    L_j:  3        U_j:  38
    L_i:  1        U_i:  j - 2  -j + 39
  generated code:
for (j = 3; j <= 38; j++)
  for (i = 1; i <= min(j - 2, -j + 39); i++)
    X[i][j] = 0;
  verify (a) reversed order j,i: 342 iterations, IDENTICAL point set to brute force (no duplicates)
  first iterations in generated order: [1,3] [1,4] [2,4] [1,5] [2,5] [3,5] [1,6] [2,6] [3,6] [4,6] ...
```

To experiment, copy any numbered program (say `10_exercise_11_3_4_reverse_order.cpp`) to a new number and edit its `main()`: build a `Polyhedron` with `.addInequality({{"i", 1}, {"j", -1}}, 5)` for `i − j + 5 ≥ 0`, then call `experiment(title, poly, newVars, transform, offset, order, lo, hi, body, verbose)`. `transform` and `offset` express the old variables in terms of the new ones (`x_old = transform·y_new + offset`); pass the identity and a zero vector for a pure reordering. For a change of axes that is *not* unimodular, use `experimentEquality(title, poly, "m", {{"i", 1}, {"j", -2}}, order, lo, hi, body, verbose)` instead: it keeps the old variables, adds `m = i − 2j` as an equality, and lets the determined variable's loop run once or not at all.

## The fuzzer

### Code example 90 — `./run.sh 90`

If you change the eliminator or the bound generator, run the fuzzer (`code/src/90_fm_fuzz.cpp`) afterwards. It compares the generated nest with brute force on 3 000 random polyhedra in random loop orders and must end with `0 mismatches, 0 bound losses`.

The fuzzer first replays two regression cases for the pruning pass (a duplicated bound, and two bounds that imply each other), then generates 3 000 random polyhedra: a box `[−3, 6]^d` in two or three variables plus one to three random rows with small coefficients, so brute force stays cheap and empty or degenerate shapes are common. For each it picks a random loop order, runs Algorithm 11.13, and compares the generated nest with brute force. A nest that lost a bound (an empty lower or upper list) is reported separately, because `runNest` cannot execute it.

**code/src/90_fm_fuzz.cpp**
```cpp
#include "verify.h"

#include <random>

// Two cases that once broke the pruning pass (see the comment in
// dropImpliedBounds): a bound must survive when its only "proof" is a copy or
// an equivalent of itself.
static int regressions() {
  int bad = 0;
  Polyhedron poly; // duplicate upper bound: both used to be dropped
  poly.vars = {"i"};
  poly.addInequality({{"i", 1}}, 0)
      .addInequality({{"i", -1}}, 5)
      .addInequality({{"i", -1}}, 5);
  Bounds bounds = computeBounds(poly, {"i"});
  if (bounds.upper[0].empty()) {
    ++bad;
    std::cout << "REGRESSION: duplicate bound lost\n";
  }
  Polyhedron poly2; // j <= i and 2j <= 2i+1 are equivalent over Z
  poly2.vars = {"i", "j"};
  poly2.addInequality({{"i", 1}}, 0)
      .addInequality({{"i", -1}}, 5)
      .addInequality({{"j", 1}}, 0)
      .addInequality({{"j", -1}, {"i", 1}}, 0)
      .addInequality({{"j", -2}, {"i", 2}}, 1);
  Bounds bounds2 = computeBounds(poly2, {"i", "j"});
  if (bounds2.upper[1].empty()) {
    ++bad;
    std::cout << "REGRESSION: mutually-implying bounds lost\n";
  }
  return bad;
}

// A row with the given coefficients and constant (the variables have no names
// here, so addInequality cannot be used).
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
```

**./run.sh 90**
```
fuzz: 2895 non-empty nests checked, 105 empty skipped, 0 mismatches, 0 bound losses
```
