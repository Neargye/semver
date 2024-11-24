// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2024 Daniil Goncharov <neargye@gmail.com>.
// Copyright (c) 2020 - 2021 Alexander Gorbunov <naratzul@gmail.com>.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <assert.h>
#include <iostream>
#include <vector>
#include "semver.hpp"

static constexpr semver::version<> collect_numbers() {
  constexpr std::string_view str = "155.2.32-alpha/.23+";
  semver::detail::lexer lexer{ str };
  semver::detail::version_parser parser{ lexer };

  semver::version ver{};
  const auto res = parser.parse(ver);

  return ver;
}

int main() {
  static_assert(collect_numbers().major() == 155);
  static_assert(collect_numbers().minor() == 2);
  static_assert(collect_numbers().patch() == 32);
  static_assert(collect_numbers().prerelease_tag() == "alpha");
  static_assert(collect_numbers().build_metadata() == "");

  static_assert(!semver::valid("1.2.3a"));

  collect_numbers();

  return 0;
}
