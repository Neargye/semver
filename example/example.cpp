// semver c++11 example
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

#include <semver.hpp>
#include <iostream>

int main() {
  constexpr semver::Version v_default;
  static_assert(v_default == semver::Version(0, 1, 0, semver::Version::PreReleaseType::kNone, 0), "");

  std::cout << v_default.ToString() << std::endl; // 0.1.0

  constexpr semver::Version v1(1, 4, 3);
  std::cout << v1.ToString() << std::endl; // 1.4.3
  constexpr semver::Version v2(1, 2, 4, semver::Version::PreReleaseType::kAlpha, 10);
  std::cout << v2.ToString() << std::endl; // 1.2.4-alpha.10
  static_assert(v1 > v2, "");

  semver::Version v_s;
  v_s.FromString("1.2.3-rc.1");
  const std::string s1 = v_s.ToString(); // 1.2.3-rc.1
  std::cout << s1 << std::endl;
  v_s.pre_release_version = 0;
  const std::string s2 = v_s.ToString(); // 1.2.3-rc
  std::cout << s2 << std::endl;

  return 0;
}
