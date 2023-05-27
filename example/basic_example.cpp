// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2021 Daniil Goncharov <neargye@gmail.com>.
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

#include <semver.hpp>

#include <iostream>

using namespace semver;

int main() {
  constexpr version v_default;
  static_assert(v_default == version(0, 1, 0, prerelease::none, std::nullopt));
  std::cout << v_default << std::endl; // 0.1.0

  constexpr version v1{1, 4, 3};
  constexpr version v2{"1.2.4-alpha.10"};
  std::cout << v1 << std::endl; // 1.4.3
  std::cout << v2 << std::endl; // 1.2.4-alpha.10
  static_assert(v1 != v2);
  static_assert(!(v1 == v2));
  static_assert(v1 > v2);
  static_assert(v1 >= v2);
  static_assert(!(v1 < v2));
  static_assert(!(v1 <= v2));

  version v_s;
  v_s.from_string("1.2.3-rc.1");
  std::string s1 = v_s.to_string();
  std::cout << s1 << std::endl; // 1.2.3-rc.1
  v_s.prerelease_number = std::nullopt;
  std::string s2 = v_s.to_string();
  std::cout << s2 << std::endl; // 1.2.3-rc

  constexpr version vo = "2.0.0-rc.3"_version;
  std::cout << vo << std::endl; // 2.0.0-rc.3

  return 0;
}
