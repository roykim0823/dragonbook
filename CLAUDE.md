# CLAUDE.md — Dragon Book study bundles

Study notes for *Compilers: Principles, Techniques, and Tools* (2nd ed., Aho, Lam, Sethi, Ullman),
one folder per section, grouped by chapter: `ch11_optimization/11.3_iteration_spaces/` is Chapter 11
§11.3 Iteration Spaces. Future sections (`ch11_optimization/11.4_<topic>/`, …) and chapters
(`ch12_<topic>/`) follow the same rules. This file and `ref/` live once at the repository root and apply to every section folder.

## Who this is for

The reader is a novice to the topic. Explanations must be top-down (why the section exists → the
concepts → the algorithm → worked examples → exercises), define every term the book assumes, and
be concrete: every concept gets a runnable example and, where it helps, a figure.

## Folder layout (keep it)

```
dragonbook/
  CLAUDE.md                   this file (shared by all sections)
  ref/*.pdf                   the book (not to be edited or redistributed)
  ch<NN>_<topic>/<NN>.<M>_<section_topic>/
                              one folder per book section, e.g. ch11_optimization/11.3_iteration_spaces/
    README.md                 entry point: the introduction, vocabulary, "three ideas", a Contents list
                              linking into docs/, and the bundle/build appendix
    docs/<N>.<M>.<K>_<topic>.md
                              the guide itself, one file per book subsection in book order
                              (11.3.1_constructing_iteration_spaces.md … 11.3.7_exercises.md), then
                              beyond_11.3.md.
                              Each starts with a nav line (back to README, previous, next). Figures are
                              linked as ../figures/NN_name.png
    docs/code_bundle.md  everything about the code, book-independent: why C++ (former
                              Appendix A), the shared headers quoted verbatim and explained, one
                              `### <header>.h` section per header in include order (intdiv, poly,
                              fourier_motzkin, loop_bounds, verify, experiment) -- the ONLY place
                              header code is quoted --, how to read a trace and the fuzzer, code
                              example 90 (former Appendix B).
    run.sh                    `./run.sh` builds + runs every numbered program (`./run.sh 5` = just 05),
                              prints the output to stdout, fails on MISMATCH
    code/CMakeLists.txt       the same as a CMake project (one ctest per program); `cmake -S code -B build`
    code/src/NN_<item>.cpp    ONE program per book example / exercise, numbered in book order
                              (01_example_11_5_…, …, 16_exercise_11_3_9_…); 90_fm_fuzz.cpp is the fuzzer
    code/include/*.h          shared code, one concern each, include only what a program uses:
                              poly.h (Constraint/Polyhedron, normalize, print) -> fourier_motzkin.h
                              (Alg. 11.11) -> loop_bounds.h (Alg. 11.13 + emitter) -> verify.h
                              (brute force vs generated nest) -> experiment.h (one full experiment);
                              intdiv.h (floor/ceil division). Each program builds its own
                              polyhedra inline with ge0() so the reader sees the rows.
    figures/NN_name.png       figures, numbered in reading order, rendered from the real data
    figures/make_figures.py   the matplotlib script that renders them (needs a venv with matplotlib)
    build/                    scratch output of run.sh / CMake; never referenced, safe to delete
```

Paths below are relative to the section folder unless they start with `ref/`, which is relative to
the repository root (`../../ref/` from a section folder).

## The book

- PDF: `ref/Compilers - Principles, Techniques, and Tools (2e) - Aho et al. - 2006 [AW].pdf`
- **Book page N is PDF page N + 25.** Chapter 11 starts at book page 769.
- `pdftotext`/poppler are not installed. Extract text with `pypdf` in a venv under the scratchpad
  (`python3 -m venv venv && venv/bin/pip install pypdf fonttools`), run with `python -I`.
  Extracted digits come out as `/7/9/6`; strip the slashes before reading.
- Always cite the book's numbering (Example 11.12, Algorithm 11.13, Fig. 11.15, Eq. 11.1) so the
  reader can cross-check, and verify those numbers against the PDF, not from memory.

## Code conventions

- Language: C++17. One numbered `.cpp` per book example or exercise (`code/src/NN_<item>.cpp`, numbered
  in book order; a new item gets the next free number in reading order), each self-contained apart
  from the shared headers in `code/include/` (no `#define` tricks to reuse a .cpp as a library). Built with
  `g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o <name> code/src/<name>.cpp && ./<name>`
  from the section folder.
  Must compile with no warnings. Put the build line in a comment at the top of each file.
- Keep everything in integers. Use the `floorDiv`/`ceilDiv` helpers of `intdiv.h`; never rely on C++
  `/` for negative numerators. The *generated* C code prints `floor_div`/`ceil_div`; leave those names.
- Style: LLVM formatting (`clang-format -style=LLVM`, 80 columns, 2-space indent) with MLIR naming:
  types `PascalCase`, functions and variables `lowerCamelCase`, `int64_t` (no `ll` alias), `.h`/`.cpp`,
  `#ifndef DRAGONBOOK_CH11_11_3_<FILE>_H` guards, own headers included before system headers.
  Run clang-format after every edit; put long comments on their own line so it does not split statements.
- **Every generated or rewritten loop nest must be verified** against a brute-force enumeration
  of the original (same point set, same count, no duplicates), and the program must print the
  verdict. Never present a loop in the README that the code has not verified.
- Program output is not saved in the repo; the README quotes it under a `**./run.sh N**` label.
  When a program changes, rerun it (`./run.sh <number>`) and paste the new output into the README
  quote, so the two never disagree. `./run.sh` with no argument runs all of them and exits non-zero
  on any `MISMATCH`; `ctest` (via `CMakeLists.txt`) runs the same checks. The programs themselves
  always exit 0.
- New tools should also be fuzz-tested against brute force on small random inputs before being
  trusted (see "Known issues" for why).

## README conventions

- Markdown, with code blocks for every example and `![](../figures/..)` for every figure (docs/ files sit one level below the figures).
- Exercise solutions appear in book order and are numbered exactly as the book numbers them.
  If an exercise is solved out of order for a reason, say so where the reader would look for it.
- **Answer the exercise as the book states it.** If a different reading or a modified problem is
  more instructive, give the literal answer first and the variant second, and say why.
- When the implementation strengthens or deviates from the book's algorithm (e.g. integer
  tightening, extra redundancy pruning), say so explicitly next to the affected output.
- ASCII sketches must match the real shape (row counts, vertices). When in doubt, use the PNG.
- Explain which language is used and why, and name a production reference where one exists
  (e.g. `isl` / LLVM Polly / MLIR affine for Chapter 11).

## Figures

- Rendered with matplotlib from the actual point sets or constraint rows, never drawn by hand.
  All of them come from `figures/make_figures.py` (venv: `python3 -m venv venv &&
  venv/bin/pip install matplotlib numpy`, then `venv/bin/python figures/make_figures.py`).
  Edit the script and re-run it; never touch a PNG by hand.
- **Axis convention = the book's Fig. 11.11: the outer loop index is the vertical axis, the
  inner one is horizontal** (for Fig. 11.10 that is `j` horizontal, `i` vertical). ASCII sketches
  in the README follow the same rule. Decided 2026-10-22 to avoid confusion against the book.
- One idea per figure, title states what to look at, axes labelled with the loop variables.
- Number files `01_`, `02_`, … in the order they appear in the guide (docs/ files read in book order).

## Status (ch11_optimization/11.3_iteration_spaces)

- All review findings from 2026-10-22 are applied: pruning-context fix and empty-polyhedron
  early return in the eliminator (now `code/include/loop_bounds.h`), literal Exercise 11.3.8(b) answer in the programs and
  the README, README wording and sketch fixes.
- `code/src/90_fm_fuzz.cpp` (`./run.sh 90`; 3 000 random nests, random loop order) must report
  `0 mismatches, 0 bound losses` after any change to the eliminator. Last run: clean.
- 2026-10-22: added `run.sh`, `CMakeLists.txt`, `figures/make_figures.py`; all figures re-rendered
  with the book's axis orientation; the introduction quotes Fig. 11.10 verbatim and the ASCII sketch no longer
  repeats the loop nest.
- Guide structure (split 2026-10-26, README.md had reached 3 200 lines): `README.md` holds
  `# 11.3 Iteration Spaces`, the short unnumbered intro (problem statement, vocabulary, "three
  ideas in one picture"), a `## Contents` list and the bundle/build section. Each book subsection
  is one file `docs/11.3.N_<topic>.md` headed `# §11.3.N <book's subsection title>`, in book order,
  no grouping "Parts". `##` headings inside a subsection carry a book item number where one exists
  (`## Example 11.10 — …`, `## Algorithm 11.11 — …`, `## Exercise 11.3.4 — …`) and no number
  otherwise. Cross-references name the heading, never a local number; a reference into another
  docs file links it (`[§11.3.7 Exercises](11.3.7_exercises.md)`). "README" in the rules below
  means the guide as a whole (README.md + docs/*.md).
- Every program appears in the guide under its own `Code example NN — ./run.sh N` heading (one
  level below the enclosing heading), placed at the END of the subsection it belongs to, so the
  book explanation reads uninterrupted and the code part can be skipped. The heading holds: a
  sentence on what the program does, a one-line link to the header section of
  docs/code_bundle.md for any shared helper the program uses for the first time (plus, where
  useful, an output-line-to-function table), the program's code under a `**code/src/NN_….cpp**`
  label, and its output under `**./run.sh N**`. Header code itself is quoted only in
  docs/code_bundle.md, verbatim (the two excerpts from inside eliminate() re-indented), with
  the explanatory lead-in next to it; when a header changes, update that file.
- 2026-10-22: shared code moved from `fm_loopgen.cpp` (old `FM_NO_MAIN` trick) into a shared header (now split under `code/include/`)
  and `code/include/intdiv.h`; all three outputs byte-identical before and after.
- 2026-10-22: programs split into one numbered file per book item (`code/01_…` to `16_…`, fuzzer `90_`),
  output printed to stdout only (no saved output files); verdict counts unchanged (19 IDENTICAL,
  17 plain-loop checks, fuzz clean).
- 2026-10-22 code review: fm.hpp split into poly / fourier_motzkin / loop_bounds / verify /
  experiment headers; bounds() no longer prints (returns Bounds::empty), run() refuses an unbounded
  loop, Poly::idx() throws on an unknown name. Output byte-identical to before the split.
- 2026-10-23 review: nests.hpp removed (polyhedra built inline in each program); names: Polyhedron,
  rows, column(), Bounds::lower/upper, enumerate_box, run_nest, compare, is_constant; no std::function
  in verify.hpp. Output byte-identical.
- 2026-10-23: code reformatted to LLVM style with MLIR naming (clang-format, .h headers, define guards,
  camelCase functions/variables, int64_t). Output byte-identical.
- 2026-10-26: headers and programs rewritten for readability at the user's request (the reader found
  poly.h / verify.h hard to follow): named helpers instead of lambdas and index tricks
  (`signStr`, `evaluate`, `nextPointInBox`, `boundValue`, `combine`, `dropImpliedBounds`,
  `boundExprs`, `loopHeader`, `printGeneratedNest`, fuzzer `row`/`randomPolyhedron`), no
  `std::function`, a comment on every non-obvious line. Public names the README refers to are
  unchanged. All 17 outputs byte-identical to before; README code quotes regenerated from the files.
- 2026-10-26: program 03 simplified at the user's request to a twin of program 02: the two-row
  polyhedron over (n, i), enumerated once with enumerateBox; n = -10 (guard n >= 0 false: empty
  slice, 0 iterations) and n = 10 (11 points) are counted as the slice p[0] == value vs the plain
  loop. It no longer uses loop_bounds.h; the README's loop_bounds.h quote moved to code example 05.
- 2026-10-26: README §11.3.5 gained explanatory material at the user's request: a pair table and
  "what the output of an elimination contains" (with Fig. 11.10 / 11.17(b) / 11.17(c) rows checked
  against programs 04 and 11), the derivation c2*lo + c1*up == c2*L <= c1*U, a book-step-to-code table
  next to combine(), and a new "Projection: the question the outer loop asks" subsection with a
  per-j strip table. Figure 04 (projection) re-rendered as two panels (strips per j, then the shadow).
- 2026-10-26: README gained "Pass 2: the question the pruning pass asks" (per-j binding tables for
  Example 11.14 and Fig. 11.17(a), the implied() trace for i <= 29, both traces checked against the
  eliminator). New figure 06_pass2_redundant_bound.png; figures renumbered into README reading
  order (04 projection, 05 FM, 06 pass 2, 07 change_axes, 08 sweep_diagonal, 09-11 ex_a/b/c,
  12 antidiagonal); program 08's comment
  updated to match.
- 2026-10-26: README §11.3.5 no longer uses Fig. 11.17 unannounced: a blockquote before Algorithm 11.11
  quotes the three exercise nests (book p. 800) as "borrowed", and the Fig. 11.17(b) pair table explains
  that "true" in a constant pair row (990 >= 0) is a statement about the new row, not about the input
  rows (i = 1100 is simply not in S).
  The Fig. 11.17(b) pair walk-through and the Fig. 11.17(c) non-unit-coefficient note were then moved
  out of §11.3.5 into Exercise 11.3.5 (their proper place); §11.3.5 keeps a one-sentence pointer.
- 2026-10-26: README "Example 11.12 — worked by hand and by the program" elaborated at the user's
  request (instead of adding invented cases for Algorithm 11.11): input table with piles and the
  book's C / S - C, the two pairs as c2*L <= c1*U and as row arithmetic, the book's steps 1-4 each
  applied, and the book's remark on reading x_m back from C. Program 04 now also brute-forces that
  per-j check (j = -1..8, IDENTICAL); README quotes regenerated. 20 IDENTICAL verdicts in ./run.sh.
- 2026-10-26: the fourier_motzkin.h quote (combine() + pairing loop) moved from the Algorithm 11.11
  recipe section to code example 04, where the trace it produces is shown; the recipe keeps the
  book-step-to-code table and a pointer.
- 2026-10-26: code example 05 now quotes the Algorithm 11.13 core at its first use: computeBounds()
  (with a Fig. 11.15 line-to-code table), dropImpliedBounds(), and implied() (moved there from the
  Pass 2 prose, which keeps a pointer); the emitter quote is labelled as such.
- 2026-10-26: code example 05's experiment.h / verify.h / emitter quotes rebuilt as contiguous verbatim
  excerpts with labels naming their functions (now include boundExprs, printBounds, pointStr), preceded
  by an output-line-to-function table for the ./run.sh 5 trace; the other verify.h / experiment.h quotes
  (code examples 02, 06, 15) got the same labels. All 14 header quotes checked verbatim against the files
  (the two eliminate() excerpts are verbatim up to indentation).
- 2026-10-26: poly.h / intdiv.h quotes given the same treatment: labels naming the functions, verbatim
  excerpts; code example 02 now also quotes the printing functions (signStr/exprStr/rowStr/printRows)
  with an output-line-to-function table; isConstant() is quoted under code example 04 at its first use;
  the intdiv.h quote is preceded by a table of floorDiv/ceilDiv values vs C++ `/` (values produced by
  the functions themselves).
- 2026-10-26: README.md split at the user's request into README.md (intro + Contents + build) and
  docs/ (one file per book subsection, plus beyond_11.3 and appendices A/B). Content moved verbatim
  except: headings promoted one level, figure links rewritten to ../figures/, a nav line per file,
  and five cross-file references turned into links. The scratchpad refresh script that regenerated
  code quotes targeted README.md and must be pointed at docs/*.md if reused.
- 2026-10-26: all 16 header quotes (and their explanatory lead-ins) moved out of the subsections into
  docs/code_bundle.md, ordered by header and include order, plus infeasible() which had never
  been quoted (17 quotes, all checked verbatim). Subsections keep the output-to-function tables and a
  one-line link per former quote. docs/11.3.5 went from 1010 to 516 lines.
- 2026-10-26 review of all md files after the split and the walkthrough move: links/anchors, figure
  order, 17 program code quotes, 17 output quotes (full or abridged) and 17 header quotes re-checked
  mechanically (all clean); prose fixes: stale "quoted under code example 04/05" pointers now point at
  docs/code_bundle.md, cross-file "above/below" references turned into links, "triangle" ->
  "trapezoid" for Fig. 11.10's shadow, change_axes() -> changeAxes(), dangling colons, trailing ---
  separators removed, loop_bounds.h comment "see README" -> "see the guide".
- 2026-10-27: Appendix A (language) and Appendix B (reading a trace, fuzzer) merged into
  docs/code_bundle.md at the user's suggestion; the two appendix files are gone, README contents
  and nav lines updated.
- 2026-10-27: docs/code_walkthrough.md renamed to docs/code_bundle.md, title "The code bundle"; all
  links and link texts updated ("the code bundle").
- 2026-10-27: the user removed the "borrowed nests" blockquote (the three Fig. 11.17 nests) from §11.3.5;
  the nests are quoted only at the top of docs/11.3.7_exercises.md, and the three remaining Fig. 11.17
  mentions in §11.3.5 (Pass 2 example, integers vs. rationals) now link there.
- 2026-10-27: at the user's request §11.3.5 no longer uses any exercise nest: the Fig. 11.17(a) Pass 2
  walk-through (binding table, implied() trace, figure) moved to Exercise 11.3.4 in docs/11.3.7_exercises.md;
  §11.3.5's Pass 2 section now shows the implied() trace on Example 11.14 itself (bound i <= 5, not implied;
  trace checked against the eliminator) and points at Exercise 11.3.4(a) for a dropped bound; the integer
  tightening example is a generic row with a pointer. Figure 06 is now Example 11.14 alone
  (06_pass2_all_kept.png); the Fig. 11.17(a) panel is 12_pass2_redundant_bound.png under Exercise 11.3.4,
  and the anti-diagonal figure became 13_ex_a_antidiagonal.png.
- No open issues.

## Do not

- Do not edit or move anything under `ref/`.
- Do not commit or publish without being asked (this folder is not a git repository).
- Do not change exercise answers to make verification pass; fix the derivation and re-verify.
