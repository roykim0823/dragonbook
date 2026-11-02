# 11.3 Iteration Spaces

A compiler looks at a loop nest such as

```c
for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
        Z[i][j] = X[j][i];
```

and asks two questions:

1. **Parallelism** — which iterations can run at the same time without changing the result?
2. **Locality** — in which order should the iterations run so that the memory system (caches) is used well?

Both questions are about *rearranging* the iterations. But a compiler cannot "try" orders on a loop whose bounds are symbolic (`n`) or that has millions of iterations. It needs a **compact, exact, mathematical description** of "all the iterations", that it can reason about without running the loop. §11.3 supplies that description. §11.4 then describes *what memory each iteration touches*, and the rest of the chapter combines the two.

## Vocabulary you need (the book assumes you know these)

| Term | Plain-language meaning | Example |
|---|---|---|
| **affine expression** | linear terms plus a constant; no products of variables | `2i + 3j − 1` is affine; `i·j`, `i²`, `n·j` are not |
| **half-space** | one side of a straight line (2-D) / flat plane (3-D) / hyperplane (n-D) | `j ≥ i` is everything on one side of the line `j = i` |
| **convex polyhedron** | intersection of finitely many half-spaces; "convex" = no dents: the segment between any two inside points stays inside | a triangle, a trapezoid, a tetrahedron |
| **integer point / lattice point** | a point whose coordinates are all integers | `(2, 5)` yes, `(2.5, 5)` no |
| **iteration space** | the set of integer points inside the polyhedron given by the loop bounds | the dots in the picture of the next section |
| **lexicographic order** | dictionary order on vectors: compare first coordinate, break ties with the second, … | `(1,7) < (2,0)` because `1 < 2` |
| **projection** | the shadow a shape casts when you look along one axis | the shadow of the trapezoid on the `j` axis is the segment `0..7` |
| **symbolic constant** | a value unknown at compile time but fixed during the whole nest | `n` in `for (i=0; i<=n; i++)` |

## Three ideas in one picture

§11.3 rests on three ideas, and the section is much easier to follow if you hold them together from the start: a loop nest **is** a set of points; that set **is** a convex shape; and that shape **is** a system of linear inequalities. The book introduces them one at a time (§11.3.1, §11.3.2, §11.3.3). Here they are side by side for the loop nest the book uses throughout the section, Fig. 11.10:

**The Three Views**
```
       loop nest (Fig. 11.10)            geometry (Fig. 11.11)              algebra (Eq. 11.1)
                                      ┌──────────────────────────┐
                                      │  i ▲                     │
                                      │  5 ├          ● ● ●      │     ┌──────────────────────────┐
   ┌────────────────────────────┐     │  4 ├        ● ● ● ●      │     │  { (i,j) ∈ Z² |          │
   │ for (i = 0; i <= 5; i++)   │     │  3 ├      ● ● ● ● ●      │     │      i ≥ 0,  i ≤ 5,      │
   │   for (j = i; j <= 7; j++) │  =  │  2 ├    ● ● ● ● ● ●      │  =  │      j ≥ i,  j ≤ 7 }     │
   │     Z[j,i] = 0;            │     │  1 ├  ● ● ● ● ● ● ●      │     │                          │
   └────────────────────────────┘     │  0 ├● ● ● ● ● ● ● ●      │     │  = { x | B x + b ≥ 0 }   │
                                      │    └───────────────► j   │     └──────────────────────────┘
                                      │     0 1 2 3 4 5 6 7      │
                                      └──────────────────────────┘
           "the program"                 "the iteration space"              "the matrix form"
```

Read it as: `i` is the outer loop and runs from 0 to 5; for each `i`, the inner loop `j` starts at `i` (not at 0) and runs to 7. The inner bound depending on the outer index is what makes the shape above a trapezoid rather than a rectangle. The book writes the array element as `Z[j,i]`; in C that is `Z[j][i]`, and the rest of this guide uses the C form.

(The axes follow the book's Fig. 11.11: `j` horizontal, `i` vertical, so the outer loop index is the vertical axis and each row of dots is one trip of the outer loop. Row `i` holds `8 − i` points and columns `j = 5, 6, 7` each hold six: the shape is a trapezoid, not a triangle. Compare with the plot of Fig. 11.11 in §11.3.1.)

* Every iteration is a **point** with integer coordinates `(i, j)`.
* The set of all iterations is a **polyhedron** (a shape with flat sides) — here a trapezoid.
* The polyhedron is described by **linear inequalities**, which can be packed into a matrix `B` and a vector `b`.

Once the loop is a polyhedron, "reorder the loop" becomes "sweep the same set of points along a different axis", and "generate the new loop" becomes "compute the new bounds of that sweep". The algorithm that computes new bounds is **Fourier–Motzkin elimination**.

## Contents

The guide continues in `docs/`, one file per book subsection, in book order. Read them top to bottom; each starts with links back here and to the previous and next file.

- [§11.3.1 Constructing Iteration Spaces from Loop Nests](docs/11.3.1_constructing_iteration_spaces.md) — a loop nest as a set of integer points; Examples 11.5 and 11.6
- [§11.3.2 Execution Order for Loop Nests](docs/11.3.2_execution_order.md) — lexicographic order and why reordering is the whole game
- [§11.3.3 Matrix Formulation of Inequalities](docs/11.3.3_matrix_formulation.md) — the rows `B·x + b ≥ 0` of Eq. 11.1, the data model, and brute force; Example 11.8
- [§11.3.4 Incorporating Symbolic Constants](docs/11.3.4_symbolic_constants.md) — a symbolic bound such as `n` is just another column; Example 11.9
- [§11.3.5 Controlling the Order of Execution](docs/11.3.5_controlling_execution_order.md) — projection, Fourier–Motzkin (Algorithm 11.11), loop-bound generation (Algorithm 11.13), integers vs. rationals; Examples 11.10 to 11.14
- [§11.3.6 Changing Axes](docs/11.3.6_changing_axes.md) — sweeping along a new axis with a unimodular change of variables; Example 11.15
- [§11.3.7 Exercises for Section 11.3](docs/11.3.7_exercises.md) — Exercises 11.3.1 to 11.3.11, each derived by a program and verified
- [Beyond §11.3 — how it connects to §11.4 (affine array indexes)](docs/beyond_11.3.md) — how iteration spaces feed the affine array accesses of §11.4
- [The code bundle](docs/code_bundle.md) — why C++; every function of `code/include/*.h` quoted verbatim and explained, from `intdiv.h` to `experiment.h` (the subsections link here instead of quoting header code); an annotated trace; the fuzzer (code example 90)

## Bundle contents and how to build

This folder is a top-down guide to Dragon Book (2nd ed.) §11.3 with runnable C++ and worked exercises.

| Path | What it is |
|---|---|
| `README.md` | this file: the introduction, the contents list, and how to build |
| `docs/*.md` | the guide itself, one file per book subsection (`11.3.1_…` to `11.3.7_…`), then the §11.4 outlook |
| `docs/code_bundle.md` | everything about the code: why C++, the shared headers quoted and explained in dependency order (the subsections point here for header code), how to read a trace, the fuzzer |
| `code/src/NN_*.cpp` | one program per book item, numbered in book order: `01` Example 11.5 … `06` Example 11.15, `07` Exercise 11.3.1 … `16` Exercise 11.3.9. Each derives the answer with the shared headers, verifies it against brute force, and re-checks the hand-written loop |
| `code/src/90_fm_fuzz.cpp` | stress test of the eliminator and bound generator: 3 000 random polyhedra, generated nest vs. brute force, plus pruning regressions |
| `code/include/poly.h` | the data model: `Constraint` (one row), `Polyhedron` (Eq. 11.1), row normalisation, printing |
| `code/include/fourier_motzkin.h` | Algorithm 11.11: `eliminate()`, plus `infeasible()` / `implied()` built on it |
| `code/include/loop_bounds.h` | Algorithm 11.13: `computeBounds()` (pass 1 inline, pass 2 in `dropImpliedBounds()`), and the emitter that prints `for` loops |
| `code/include/verify.h` | `enumerateBox` (brute force), `runNest` (run a generated nest), `compare` (the verdict), and `report` for plain-loop checks |
| `code/include/experiment.h` | one complete reorder / change-of-axes experiment, used by programs 05–15 |
| `code/include/intdiv.h` | `floor_div`/`ceil_div`, shared by everything |
| `figures/*.png` | plots of the actual iteration spaces (rendered from the real point sets) |
| `figures/make_figures.py` | the matplotlib script that renders them; re-run it after changing a point set |
| `run.sh` | `./run.sh` builds and runs every numbered program and prints its output, `./run.sh 5` just number 05; exits non-zero on any `MISMATCH` |
| `code/CMakeLists.txt` | the same as a CMake project, one `ctest` per numbered program |

Build and run everything: `./run.sh`
or `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.
One program by hand: `g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o ex05 code/src/05_example_11_14_reorder_j_i.cpp && ./ex05`.
