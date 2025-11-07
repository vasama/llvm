//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <expected>

std::expected<int, int> f(int x) {
  if (x < 0)
    return std::unexpected(x);
  return x;
}

std::expected<int, int> g(int x, int y) {
  return f(x)!? + f(y)!?;
}

int h(std::expected<int, int> const& e) {
  return e ? e.value() : e.error() - 10;
}

int main() {
  assert(h(g(+1, +2)) == 3);
  assert(h(g(-1, +2)) == -11);
  assert(h(g(+1, -2)) == -12);
}
