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

#include "semver.hpp"

using namespace semver;

int main() {
    constexpr std::string_view r1 = ">=1.2.7 <1.3.0";
    static_assert(range::satisfies("1.2.7"_version, r1));
    static_assert(range::satisfies("1.2.8"_version, r1));
    static_assert(range::satisfies("1.2.99"_version, r1));
    static_assert(!range::satisfies("1.2.6"_version, r1));
    static_assert(!range::satisfies("1.3.0"_version, r1));
    static_assert(!range::satisfies("1.1.0"_version, r1));

    constexpr std::string_view r2 = "1.2.7 || >=1.2.9 <2.0.0";
    static_assert(range::satisfies(version{1, 2, 7}, r2));
    static_assert(range::satisfies({1, 2, 9}, r2));
    static_assert(!range::satisfies("1.2.8"_version, r2));
    static_assert(!range::satisfies("2.0.0"_version, r2));

    // By default, we exclude prerelease tag from comparison.
    constexpr std::string_view r3 = ">1.2.3-alpha.3";
    static_assert(range::satisfies("1.2.3-alpha.7"_version, r3));
    static_assert(!range::satisfies("3.4.5-alpha.9"_version, r3));

    // But we can suppress this behavior by passing semver::range::option::include_prerelease.
    // For details see: https://github.com/npm/node-semver#prerelease-tags
    static_assert(range::satisfies("3.4.5-alpha.9"_version, r3, range::satisfies_option::include_prerelease));

    return 0;
}
