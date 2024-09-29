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
#include "semver.hpp"

int main() {
  semver::version version;
  bool result = semver::parse("1.2.3-alpha.1+dev", version);
  std::cout << (result ? "true" : "false") << std::endl; // true

  std::cout << version.major << std::endl; // 1
  std::cout << version.minor << std::endl; // 2
  std::cout << version.patch << std::endl; // 3
  std::cout << version.prerelease_tag << std::endl; // alpha.1
  std::cout << version.build_metadata << std::endl; // dev

  std::cout << semver::major<int>("1.2.3-alpha.1+dev").value() << std::endl; // 1
  std::cout << semver::minor<int>("1.2.3-alpha.1+dev").value() << std::endl; // 2
  std::cout << semver::patch<int>("1.2.3-alpha.1+dev").value() << std::endl; // 3
  std::cout << semver::prerelease_tag("1.2.3-alpha.1+dev") << std::endl; // alpha.1
  std::cout << semver::build_metadata("1.2.3-alpha.1+dev") << std::endl; // dev

  assert(semver::valid("1.2.3"));
  assert(!semver::valid("a.b.c"));

  assert(semver::less("1.0.0", "1.0.1"));
  assert(semver::greater("2.0.0", "1.0.0"));
  assert(semver::equal("1.2.3", "1.2.3"));

  return 0;
}
