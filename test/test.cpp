// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2026 Daniil Goncharov <neargye@gmail.com>.
// Copyright (c) 2020 - 2025 Alexander Gorbunov <naratzul@gmail.com>.
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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <semver.hpp>
#include <iostream>

TEST_CASE("platform constexpr support") {
  // Always printed — helps verify version thresholds in CI.
  // Run with `ctest -V` or the test binary directly to see output.
  std::cout << "\n=== constexpr support ===\n";
  std::cout << "SEMVER_FULL_CONSTEXPR          = " << SEMVER_FULL_CONSTEXPR << "\n";
#ifdef __cpp_lib_constexpr_string
  std::cout << "__cpp_lib_constexpr_string     = " << __cpp_lib_constexpr_string << "\n";
#else
  std::cout << "__cpp_lib_constexpr_string     = (not defined)\n";
#endif
#ifdef __cpp_lib_constexpr_vector
  std::cout << "__cpp_lib_constexpr_vector     = " << __cpp_lib_constexpr_vector << "\n";
#else
  std::cout << "__cpp_lib_constexpr_vector     = (not defined)\n";
#endif
#ifdef _LIBCPP_VERSION
  std::cout << "_LIBCPP_VERSION                = " << _LIBCPP_VERSION << "\n";
#endif
#ifdef _GLIBCXX_RELEASE
  std::cout << "_GLIBCXX_RELEASE               = " << _GLIBCXX_RELEASE << "\n";
#endif
#ifdef _MSC_VER
  std::cout << "_MSC_VER                       = " << _MSC_VER << "\n";
#endif
  std::cout << "=========================\n";
}
