// semver example
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// Copyright (c) 2018 Daniil Goncharov <neargye@gmail.com>.
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

#include <iostream>

#include <semver.hpp>

using namespace semver;

int main() {
  constexpr version v_default;
  static_assert(v_default == version(0, 1, 0, prerelease::none, 0), "");
  std::cout << v_default << std::endl; // 0.1.0

  constexpr version v1(1, 4, 3);
  constexpr version v2(255, 255, 255, prerelease::alpha, 255);
  std::cout << v1 << std::endl; // 1.4.3
  std::cout << v2 << std::endl; // 1.2.4-alpha.10

  auto s = v2.to_string();
  std::cout << s.size() << std::endl;
  std::cout << s.capacity() << std::endl;
  s.shrink_to_fit();
  std::cout << s.size() << std::endl;
  std::cout << s.capacity() << std::endl;

  version vs("1.2.3-alpha");
  version v(0,0,0);
  v.from_string("1.2.3-alpha.100");
  std::cout << v.to_string() << std::endl;

  return 0;
}
