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

#include <сassert>
#include <iostream>
#include "semver.hpp"

int main() {
  constexpr std::string_view raw_version = "1.2.3";
  semver::version version;
  const auto [ptr, ec] = semver::parse(raw_version, version);
  if (ec == std::errc{}) {
    assert(ptr == raw_version.end());
    std::cout << version.major() << std::endl; // 1
    std::cout << version.minor() << std::endl; // 2
    std::cout << version.patch() << std::endl; // 3
  }

  semver::version version2;
  if (semver::parse("1.2.3-alpha0.1+build", version2)) {
    std::cout << version2.major() << std::endl; // 1
    std::cout << version2.minor() << std::endl; // 2
    std::cout << version2.patch() << std::endl; // 3
    std::cout << version2.prerelease_tag() << std::endl; // "alpha0.1"
    std::cout << version2.build_metadata() << std::endl; // "build"
  }

  assert(version > version2);
  assert(version2 < version);

  // use 64 bit integer for numbers
  semver::version<int64_t, int64_t, int64_t> version3;
  if (semver::parse("0.0.999999999999", version3)) {
    std::cout << version3.major() << std::endl; // 0
    std::cout << version3.minor() << std::endl; // 0
    std::cout << version3.patch() << std::endl; // 999999999999
  }

  const bool result = semver::valid("0.0.1-beta");
  std::cout << std::boolalpha << result << std::endl; // true
}
