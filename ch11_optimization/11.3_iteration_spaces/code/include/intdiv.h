// intdiv.h -- integer division that rounds the way the maths does.
//
// C++ `/` truncates toward zero, so (-1) / 2 == 0. Loop bounds need the
// mathematical floor and ceiling instead:
//     floorDiv(-1, 2) == -1     (largest integer <= -0.5)
//     ceilDiv(-1, 2)  ==  0     (smallest integer >= -0.5)
// Both helpers are templated on the two operand types so that `long`, `int64_t`
// and plain int literals can be mixed without casts.
#ifndef DRAGONBOOK_CH11_11_3_INTDIV_H
#define DRAGONBOOK_CH11_11_3_INTDIV_H

#include <type_traits>

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

#endif // DRAGONBOOK_CH11_11_3_INTDIV_H
