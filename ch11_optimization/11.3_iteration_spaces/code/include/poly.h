// poly.h -- the data model of Sec. 11.3: a convex polyhedron
//     { x in Z^d  |  B x + b >= 0 }                              (Eq. 11.1)
// over named variables, plus row normalisation and printing.
//
// A Constraint is ONE row of  B x + b >= 0:
//     a[0]*x_0 + a[1]*x_1 + ... + a[d-1]*x_{d-1} + c  >= 0
// Column m belongs to the variable vars[m] of the Polyhedron the row lives in.
// Reading a row as a bound on one variable x_m (Fact 1 of Sec. 11.3.3):
//     a[m] > 0   the row is a LOWER bound on x_m   (x_m >= something)
//     a[m] < 0   the row is an UPPER bound on x_m  (x_m <= something)
//     a[m] == 0  the row says nothing about x_m
#ifndef DRAGONBOOK_CH11_11_3_POLY_H
#define DRAGONBOOK_CH11_11_3_POLY_H

#include "intdiv.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

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
  // Example:  poly.ge0({{"i", -1}, {"j", 1}}, 0)  appends  -i + j >= 0.
  // Every call builds a FRESH row: nothing carries over from one call to the
  // next, so a chain of four calls appends four independent rows.
  Polyhedron &ge0(std::initializer_list<std::pair<const char *, int64_t>> terms,
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

// A row whose coefficients are all 0 is just "c >= 0": always true when
// c >= 0, impossible when c < 0. Such rows appear when FM eliminates the last
// variable that a pair of bounds had in common.
inline bool isConstant(const Constraint &row) {
  for (int64_t coef : row.a)
    if (coef != 0)
      return false;
  return true;
}

// ---------- printing ---------------------------------------------------------
// The sign written in front of a term: the first term gets "-" or nothing,
// every later term gets " - " or " + ".
inline std::string signStr(bool firstTerm, int64_t value) {
  if (firstTerm)
    return value < 0 ? "-" : "";
  return value < 0 ? " - " : " + ";
}

// Render  a[0]*x_0 + ... + c  as text such as "2*i - j + 5". Column `skip` is
// left out; boundExpr uses that to leave out the loop variable itself.
inline std::string exprStr(const Polyhedron &poly,
                           const std::vector<int64_t> &a, int64_t c,
                           int skip = -1) {
  std::string out;
  for (size_t m = 0; m < a.size(); ++m) {
    if ((int)m == skip || a[m] == 0)
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

#endif // DRAGONBOOK_CH11_11_3_POLY_H
