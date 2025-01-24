// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2020 - 2024 Daniil Goncharov <neargye@gmail.com>.
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

#include <сassert>
#include <iostream>
#include "semver.hpp"

int main() {
  std::cout << std::boolalpha;

  constexpr std::string_view raw_range = ">=1.2.9 <2.0.0";
  semver::range_set range;
  const auto [ptr, ec] = semver::parse(raw_range, range);
  if (ec == std::errc{}) {
    assert(ptr == raw_range.end());

    semver::version version;
    if (semver::parse("1.3.0", version)) {
      const bool result = range.contains(version);
      std::cout << result << std::endl; // true
    }
  }

  semver::range_set range2;
  if (semver::parse(">=1.0.0 <=2.0.0 || >=3.0.0", range2)) {
    semver::version version;
    if (semver::parse("3.5.0", version)) {
      const bool result = range2.contains(version);
      std::cout << result << std::endl; // true
    }
  }

  semver::range_set range3;
  if (semver::parse(">1.2.3-alpha.3", range3)) {
    semver::version version;
    if (semver::parse("1.2.3-alpha.7", version)) {
      const bool result = range3.contains(version);
      std::cout << result << std::endl; // true
    }

    if (semver::parse("3.4.5-alpha.9", version)) {
      // By default, we exclude prerelease tag from comparison.
      bool result = range3.contains(version);
      std::cout << result << std::endl; // false

      // But we can suppress this behavior by passing semver::range::option::include_prerelease.
      // For details see: https://github.com/npm/node-semver#prerelease-tags
      result = range3.contains(version, semver::version_compare_option::include_prerelease);
      std::cout << result << std::endl; // true
    }
  }

  return 0;
}
