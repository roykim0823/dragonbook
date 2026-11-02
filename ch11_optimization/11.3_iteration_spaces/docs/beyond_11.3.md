# Beyond §11.3 — how it connects to §11.4 (affine array indexes)

Part of [11.3 Iteration Spaces](../README.md) · previous: [§11.3.7](11.3.7_exercises.md) · next: [the code bundle](code_bundle.md)

§11.3 describes the **domain**: which index vectors `x` exist. §11.4 describes a **map** from that domain into memory: which array element iteration `x` touches. An *affine array access* is the four-tuple

```
      ⟨ F, f, B, b ⟩         x is a valid iteration   ⇔   B x + b ≥ 0        (§11.3)
                             iteration x touches element   F x + f           (§11.4)
```

```
          iteration space (§11.3)                        data space (§11.4)
        ┌──────────────────────┐                      ┌──────────────────────┐
        │  i ▲                 │    x  ↦  F x + f     │   array Z            │
        │  5 │          ● ● ●  │  ──────────────────► │   ┌───┬───┬───┐      │
        │  4 │        ● ● ● ●  │                      │   │   │   │   │      │
        │  3 │      ● ● ● ● ●  │   e.g. Z[j][i]:      │   ├───┼───┼───┤      │
        │  2 │    ● ● ● ● ● ●  │   F = [0 1]  f = [0] │   │   │   │   │      │
        │  1 │  ● ● ● ● ● ● ●  │       [1 0]      [0] │   └───┴───┴───┘      │
        │  0 │● ● ● ● ● ● ● ●  │                      │                      │
        │    └────────────► j  │                      │                      │
        └──────────────────────┘                      └──────────────────────┘
```

Everything later in the chapter is a combination of the two matrices:

| Question | Combine how |
|---|---|
| Which array elements are touched at all? | the image of the polyhedron under `F` — a projection, done with Fourier–Motzkin (§11.4, §11.5) |
| Do iterations `x` and `x'` touch the same element? | solve `F x + f = F x' + f'` subject to `B x + b ≥ 0`, `B x' + b ≥ 0` — equalities + §11.3's inequalities (§11.6, data dependence) |
| Which direction in the iteration space reuses data? | the null space of `F`; then choose axes (§11.3.6) so that direction is the innermost loop (§11.5, §11.7) |

Keep the two spaces distinct in your head. In Fig. 11.10 the access `Z[j][i]` makes the data touched look like the iteration space in transposed form, but `Z[2*i][i+j]` would map the same trapezoid to a quite different footprint.
