#!/usr/bin/env python3
"""Render every figure in figures/ from the real point sets.

    python3 -m venv venv && venv/bin/pip install matplotlib numpy
    venv/bin/python figures/make_figures.py          # writes figures/NN_name.png

Axis convention (same as the book's Fig. 11.11): the OUTER loop index is the vertical
axis and the INNER loop index is the horizontal axis. So for Fig. 11.10 the j axis is
horizontal and the i axis is vertical. The ASCII sketches in README.md follow the same rule.

Every point set below is produced by literally running the loop nest in Python, so the
figures cannot drift from the programs in code/. Point counts are asserted against the
counts that programs 02 and 08 in code/ verify (33, 342, 9 910, 44 200).
"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))

# fixed palette, validated with the dataviz checker (light surface)
BLUE, RED, GREEN, ORANGE, PURPLE = "#1f4e9a", "#d64545", "#2e7d32", "#b8690a", "#8e4bbf"
FILL = "#dbe7f8"
GRAY = "#9a9a9a"

plt.rcParams.update({"font.size": 11, "axes.grid": True, "grid.alpha": 0.3, "grid.linewidth": 0.6,
                     "axes.spines.top": False, "axes.spines.right": False})


def save(fig, name):
    path = os.path.join(HERE, name)
    fig.savefig(path, dpi=110, bbox_inches="tight")
    plt.close(fig)
    print("wrote", path)


# ----------------------------------------------------------------------------- point sets
def fig_11_10():
    """for (i=0;i<=5;i++) for (j=i;j<=7;j++)  -- in visiting order."""
    return [(i, j) for i in range(0, 6) for j in range(i, 8)]


def ex_a():
    """Fig. 11.17(a): for (i=1;i<30;i++) for (j=i+2;j<40-i;j++)"""
    return [(i, j) for i in range(1, 30) for j in range(i + 2, 40 - i)]


def ex_b(imax=1000):
    """Fig. 11.17(b): for (i=10;i<=1000;i++) for (j=i;j<i+10;j++)"""
    return [(i, j) for i in range(10, imax + 1) for j in range(i, i + 10)]


def ex_c():
    """Fig. 11.17(c): for i<100, j<100+i, k=i+j..99-i-j"""
    return [(i, j, k) for i in range(100) for j in range(100 + i) for k in range(i + j, 100 - i - j)]


P = fig_11_10()
assert len(P) == 33
assert len(ex_a()) == 342 and len(ex_b()) == 9910 and len(ex_c()) == 44200

# the trapezoid of Fig. 11.11 in (j, i) plotting coordinates: x = j, y = i
TRAP = [(0, 0), (7, 0), (7, 5), (5, 5)]


def axes_ij(ax, xlab="j (inner loop)", ylab="i (outer loop)"):
    ax.set_xlabel(xlab)
    ax.set_ylabel(ylab)
    ax.set_aspect("equal")


# ----------------------------------------------------------------------------- 01
def fig01():
    fig, ax = plt.subplots(figsize=(7.2, 5.2))
    ax.add_patch(Polygon(TRAP, closed=True, facecolor=FILL, edgecolor="none", label="polyhedron  Bx + b ≥ 0"))
    xs = [j for i, j in P]; ys = [i for i, j in P]
    ax.scatter(xs, ys, s=55, color=BLUE, zorder=3, label="iterations (integer points)")
    t = np.array([-1, 8.5])
    ax.plot(t, t, "--", color=RED, lw=2, label="j = i   (j ≥ i)")
    ax.axhline(0, ls="--", color=GREEN, lw=2, label="i = 0")
    ax.axhline(5, ls="--", color=GREEN, lw=2, label="i = 5")
    ax.axvline(7, ls="--", color=ORANGE, lw=2, label="j = 7")
    ax.set_xlim(-1, 8.5); ax.set_ylim(-1, 6.5)
    ax.set_xticks(range(0, 9)); ax.set_yticks(range(0, 7))
    axes_ij(ax)
    ax.set_title("Fig. 11.11: the iteration space of the loop nest in Fig. 11.10")
    ax.legend(loc="upper left", fontsize=9, framealpha=0.95)
    save(fig, "01_iteration_space.png")


# ----------------------------------------------------------------------------- 02, 03, 08 sweeps
def sweep(order, title, name, xy):
    """order: points in visiting order; xy(p) -> plot coordinates (x, y) of point p."""
    fig, ax = plt.subplots(figsize=(7.2, 5.2))
    ax.add_patch(Polygon(TRAP, closed=True, facecolor=FILL, edgecolor="none"))
    pts = [xy(p) for p in order]
    xs = [x for x, y in pts]; ys = [y for x, y in pts]
    ax.plot(xs, ys, "-", color=GRAY, lw=1.2, zorder=2)
    ax.scatter(xs, ys, c=range(len(pts)), cmap="viridis", s=55, zorder=3)
    for n, (x, y) in enumerate(pts):
        ax.annotate(str(n), (x, y), xytext=(4, 4), textcoords="offset points", fontsize=7.5)
    ax.set_xlim(-1, 8.5); ax.set_ylim(-1, 6.5)
    ax.set_xticks(range(0, 9)); ax.set_yticks(range(0, 7))
    axes_ij(ax, "j", "i")
    ax.set_title(title)
    save(fig, name)


def fig02():
    sweep(P, "Row by row: i outer, j inner (Fig. 11.12) — numbers = visiting order",
          "02_sweep_original.png", lambda p: (p[1], p[0]))


def fig03():
    order = [(i, j) for j in range(0, 8) for i in range(0, min(5, j) + 1)]
    assert sorted(order) == sorted(P)
    sweep(order, "Column by column: j outer, i inner (Example 11.14)",
          "03_sweep_column.png", lambda p: (p[1], p[0]))


def fig08():
    order = [(j - k, j) for k in range(0, 8) for j in range(k, min(k + 5, 7) + 1)]
    assert sorted(order) == sorted(P)
    sweep(order, "Diagonal by diagonal: k = j − i outer, j inner (Example 11.15)",
          "08_sweep_diagonal.png", lambda p: (p[1], p[0]))


# ----------------------------------------------------------------------------- 04 projection
def fig04():
    """Left: the question the outer loop asks, one column (fixed j) at a time: is the
    strip of i values  max(lower) <= i <= min(upper)  non-empty?  Right: the j values
    that pass are the projection (shadow) of the polyhedron on the j axis, and they are
    exactly the bounds of the outer j loop."""
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(12.5, 5.6), gridspec_kw={"width_ratios": [1.15, 1]})
    for ax in (a1, a2):
        ax.add_patch(Polygon(TRAP, closed=True, facecolor=FILL, edgecolor="none"))
        ax.scatter([j for i, j in P], [i for i, j in P], s=40, color=BLUE, zorder=3)
        ax.set_xlim(-2, 9); ax.set_ylim(-2.4, 6.8)
        ax.set_xticks(range(-1, 9)); ax.set_yticks(range(0, 7))
        axes_ij(ax, "j", "i")

    # left: for every integer j from -1 to 8, the strip  lo = 0 <= i <= hi = min(5, j)
    for j in range(-1, 9):
        lo, hi = 0, min(5, j)
        if j > 7:                       # the row j <= 7 is false: this column is outside
            a1.plot([j, j], [-0.6, 5.6], color=RED, lw=2.2, ls=(0, (3, 2)))
            a1.text(j, 6.0, "j ≤ 7\nfails", ha="center", va="bottom", fontsize=8.5, color=RED)
        elif lo > hi:                   # lower bound above upper bound: empty strip
            a1.plot([j, j], [-0.6, 5.6], color=RED, lw=2.2, ls=(0, (3, 2)))
            a1.text(j, 6.0, "0 ≤ i ≤ %d\nempty" % hi, ha="center", va="bottom", fontsize=8.5, color=RED)
        else:                           # non-empty strip: the inner i loop runs hi-lo+1 times
            a1.plot([j, j], [lo, hi], color=GREEN, lw=4, solid_capstyle="round", zorder=2)
            a1.text(j, hi + 0.45, "0..%d" % hi, ha="center", va="bottom", fontsize=8.5, color=GREEN)
    a1.set_title("For each j: is the strip  0 ≤ i ≤ min(5, j)  non-empty?\n(green = yes, the inner i loop would run; red = no)", fontsize=10.5)

    # right: the j values that passed, dropped onto the j axis = the shadow
    for j in range(0, 8):
        imax = max(i for i, jj in P if jj == j)
        a2.annotate("", xy=(j, -1.2), xytext=(j, imax), arrowprops=dict(arrowstyle="->", color=GRAY, lw=0.9))
    a2.scatter(range(0, 8), [-1.2] * 8, marker="s", s=80, color=RED, zorder=4)
    a2.scatter([-1, 8], [-1.2, -1.2], marker="x", s=80, color=RED, zorder=4)
    a2.text(3.5, -2.1, "shadow on the j axis:  0 ≤ j ≤ 7   =   for (j = 0; j <= 7; j++)", ha="center", fontsize=10, color=RED)
    a2.set_title("The j values that passed = the projection of the polyhedron on j\n(Fourier–Motzkin derives  0 ≤ j  and  j ≤ 7  without trying each j)", fontsize=10.5)
    save(fig, "04_projection_shadow.png")


# ----------------------------------------------------------------------------- 05 FM lower/upper
def fig05():
    fig, ax = plt.subplots(figsize=(8.2, 4.4))
    j = np.linspace(-1, 9, 400)
    lo = np.zeros_like(j)
    up = np.minimum(5, j)
    ax.fill_between(j, lo, up, where=up >= lo, color=FILL, label="feasible i for this j")
    ax.plot(j, lo, color=GREEN, lw=2.5, label="lower bound  i ≥ 0")
    ax.plot(j, up, color=RED, lw=2.5, label="upper bounds  i ≤ 5,  i ≤ j   (min)")
    ax.axvline(0, color="black", ls=":", lw=1.5)
    ax.text(0.2, 6.2, "j ≥ 0 appears when\nlower ≤ upper:  0 ≤ j", fontsize=10)
    ax.set_xlim(-1.5, 9.5); ax.set_ylim(-2, 8)
    ax.set_xlabel("j (the variable we KEEP)")
    ax.set_ylabel("i (the variable we ELIMINATE)")
    ax.set_title("Eliminating i: an i exists  ⇔  every lower bound ≤ every upper bound")
    ax.legend(loc="upper right", fontsize=9, framealpha=0.95)
    save(fig, "05_fm_lower_upper.png")


# ----------------------------------------------------------------------------- 06 Pass 2
def fig06():
    """Pass 2 of Algorithm 11.13 asks, for every candidate bound: is there any value of the
    outer variables for which it is the binding one?  Example 11.14: both upper bounds of i
    touch the shape somewhere, both are kept."""
    fig, a1 = plt.subplots(figsize=(6.2, 5.4))

    # Fig. 11.10 in the order j (outer, vertical) / i (inner, horizontal)
    a1.add_patch(Polygon([(0, 0), (0, 7), (5, 7), (5, 5)], closed=True, facecolor=FILL, edgecolor="none"))
    a1.scatter([i for i, j in P], [j for i, j in P], s=40, color=BLUE, zorder=3)
    y = np.array([-0.5, 8.0])
    a1.plot([5, 5], y, color=RED, lw=2.2, label="i ≤ 5   binds for j ≥ 5")
    a1.plot(y, y, color=ORANGE, lw=2.2, label="i ≤ j   binds for j ≤ 5")
    a1.plot([0, 0], y, color=GREEN, lw=2.2, label="i ≥ 0")
    a1.set_xlim(-1, 8); a1.set_ylim(-0.5, 8)
    a1.set_xticks(range(0, 8)); a1.set_yticks(range(0, 8))
    axes_ij(a1, "i (inner)", "j (outer)")
    a1.set_title("Example 11.14: every bound of i is binding\nfor some j  ->  all kept", fontsize=10.5)
    a1.legend(loc="lower right", fontsize=8.5, framealpha=0.95)
    save(fig, "06_pass2_all_kept.png")


# ----------------------------------------------------------------------------- 09-11 exercises
def fig09():
    pts = ex_a()
    fig, ax = plt.subplots(figsize=(7.6, 6.2))
    ax.scatter([j for i, j in pts], [i for i, j in pts], s=14, color=BLUE, zorder=3)
    i = np.array([0, 40])
    ax.plot(i + 2, i, "--", color=RED, lw=1.8, label="j = i + 2")
    ax.plot(39 - i, i, "--", color=ORANGE, lw=1.8, label="j = 39 − i")
    ax.axhline(1, ls="--", color=GREEN, lw=1.8, label="i = 1")
    ax.axhline(29, ls=":", color=GREEN, lw=1.8, label="i = 29 (never reached)")
    ax.axhline(18, ls="--", color=PURPLE, lw=1.8, label="i = 18 (true max)")
    ax.set_xlim(0, 41); ax.set_ylim(-1, 32)
    axes_ij(ax, "j", "i")
    ax.set_title(f"Fig. 11.17(a): {len(pts)} iterations, a triangle")
    ax.legend(loc="upper right", fontsize=9, framealpha=0.95)
    save(fig, "09_ex_a.png")


def fig10():
    pts = ex_b(39)
    fig, ax = plt.subplots(figsize=(7.6, 6.2))
    ax.scatter([j for i, j in pts], [i for i, j in pts], s=14, color=BLUE, zorder=3)
    i = np.array([8, 42])
    ax.plot(i, i, "--", color=RED, lw=1.8, label="j = i")
    ax.plot(i + 9, i, "--", color=ORANGE, lw=1.8, label="j = i + 9")
    ax.axhline(10, ls="--", color=GREEN, lw=1.8, label="i = 10")
    ax.set_xlim(5, 52); ax.set_ylim(5, 42)
    axes_ij(ax, "j", "i")
    ax.set_title("Fig. 11.17(b): a diagonal band of width 10 (zoom, i ≤ 39)")
    ax.legend(loc="lower right", fontsize=9, framealpha=0.95)
    save(fig, "10_ex_b.png")


def fig11():
    pts = ex_c()[::7]
    fig = plt.figure(figsize=(8.2, 6.6))
    ax = fig.add_subplot(111, projection="3d")
    ax.scatter([p[0] for p in pts], [p[1] for p in pts], [p[2] for p in pts],
               c=[p[2] for p in pts], cmap="viridis", s=5)
    ax.set_xlabel("i"); ax.set_ylabel("j"); ax.set_zlabel("k")
    ax.set_title("Fig. 11.17(c): 44,200 iterations (every 7th shown)\nk between planes k = i + j and k = 99 − i − j; needs i + j ≤ 49")
    save(fig, "11_ex_c.png")


# ----------------------------------------------------------------------------- 10 anti-diagonals
def fig12():
    """Exercise 11.3.4(a): Fig. 11.17(a) in the order j, i.  The upper bound i <= 29 never
    touches the shape (the inner loop never gets past i = 18), so Pass 2 drops it."""
    fig, a2 = plt.subplots(figsize=(8.0, 5.4))
    A = ex_a()
    imax = max(i for i, j in A); assert imax == 18
    a2.add_patch(Polygon([(1, 3), (1, 38), (18.5, 20.5)], closed=True, facecolor=FILL, edgecolor="none"))
    a2.scatter([i for i, j in A], [j for i, j in A], s=12, color=BLUE, zorder=3)
    y = np.array([0, 41.0])
    a2.plot(y - 2, y, color=ORANGE, lw=2.2, label="i ≤ j − 2    binds for j ≤ 20")
    a2.plot(39 - y, y, color=PURPLE, lw=2.2, label="i ≤ 39 − j   binds for j ≥ 21")
    a2.plot([29, 29], y, color=RED, lw=2.2, ls=(0, (4, 2)), label="i ≤ 29   never binds  ->  dropped")
    a2.plot([1, 1], y, color=GREEN, lw=2.2, label="i ≥ 1")
    a2.annotate("", xy=(29, 20.5), xytext=(18.5, 20.5), arrowprops=dict(arrowstyle="<->", color=RED, lw=1.2))
    a2.text(30, 16.2, "largest i is 18;\n29 is never reached", ha="center", fontsize=9, color=RED)
    a2.set_xlim(-2, 36); a2.set_ylim(0, 41)
    axes_ij(a2, "i (inner)", "j (outer)")
    a2.set_aspect("auto")
    a2.set_title("Ex. 11.3.4(a), order j, i:  i ≤ 29 is implied by the other two\nupper bounds (2i ≤ 37)  ->  Pass 2 drops it", fontsize=10.5)
    a2.legend(loc="upper right", fontsize=8.5, framealpha=0.95)
    save(fig, "12_pass2_redundant_bound.png")


def fig13():
    pts = ex_a()
    fig, ax = plt.subplots(figsize=(7.6, 7.0))
    ax.scatter([j for i, j in pts], [i for i, j in pts], s=12, color="#c4c4c4", zorder=2)
    cmap = plt.get_cmap("tab10")
    for n, s in enumerate(range(4, 40, 5)):
        d = sorted((j, i) for i, j in pts if i + j == s)
        ax.plot([j for j, i in d], [i for j, i in d], "-o", ms=4, lw=1.6, color=cmap(n), label=f"s = i + j = {s}")
    ax.set_yticks(range(0, 19, 2))
    axes_ij(ax, "j", "i")
    ax.set_title("Ex. 11.3.8(a): new outer axis s = i + j\nthe outer loop walks the anti-diagonals")
    ax.legend(loc="upper right", fontsize=8.5, framealpha=0.95)
    save(fig, "13_ex_a_antidiagonal.png")


# ----------------------------------------------------------------------------- 07 change of axes
def fig07():
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(12.5, 5.2))
    a1.add_patch(Polygon(TRAP, closed=True, facecolor=FILL, edgecolor="none"))
    a1.scatter([j for i, j in P], [i for i, j in P], s=45, color=BLUE, zorder=3)
    for k in range(0, 8):
        t = np.array([-0.5, 7.5])            # line j - i = k  ->  i = j - k
        a1.plot(t, t - k, color=GRAY, lw=0.8, zorder=1)
    a1.set_xlim(-0.5, 8); a1.set_ylim(-0.5, 6)
    a1.set_xticks(range(0, 8)); a1.set_yticks(range(0, 6))
    axes_ij(a1, "j", "i")
    a1.set_title("old axes (i, j); gray lines: j − i = k")

    newpts = [(j - i, j) for i, j in P]      # (k, j)
    kj = sorted(set(newpts)); assert len(kj) == 33
    poly = [(0, 0), (7, 7), (7, 2), (5, 0)]  # (j, k) vertices of the sheared shape
    a2.add_patch(Polygon(poly, closed=True, facecolor="#f8e3cf", edgecolor="none"))
    a2.scatter([j for k, j in kj], [k for k, j in kj], s=45, color="#c75000", zorder=3)
    a2.set_xlim(-0.5, 8); a2.set_ylim(-0.5, 8)
    a2.set_xticks(range(0, 8)); a2.set_yticks(range(0, 8))
    axes_ij(a2, "j (inner)", "k = j − i (outer)")
    a2.set_title("new axes (k = j − i, j): same points, a sheared shape")
    save(fig, "07_change_axes.png")


if __name__ == "__main__":
    for f in (fig01, fig02, fig03, fig04, fig05, fig06, fig07, fig08, fig09, fig10, fig11, fig12, fig13):
        f()
