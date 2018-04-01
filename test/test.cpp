// semver c++11 test
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

#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include <semver.hpp>
#include <cstring>
#include <string>

using namespace semver;

TEST_CASE("constexpr") {
  SECTION("default") {
    constexpr Version v_default;
    static_assert(v_default.major == 0 &&
                  v_default.minor == 1 &&
                  v_default.patch == 0 &&
                  v_default.pre_release_type == Version::PreReleaseType::kNone &&
                  v_default.pre_release_version == 0, "");
  }

  SECTION("operator ==") {
    constexpr Version v1(1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4);
    constexpr Version v2(1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4);
    static_assert(v1 == v2, "");
  }

  SECTION("operator >") {
    constexpr Version v1(1, 0, 0, Version::PreReleaseType::kAlpha, 0);
    constexpr Version v2(1, 1, 0, Version::PreReleaseType::kAlpha, 0);
    constexpr Version v3(1, 0, 1, Version::PreReleaseType::kAlpha, 0);
    constexpr Version v4(1, 0, 0, Version::PreReleaseType::kAlpha, 1);
    constexpr Version v5(1, 0, 0, Version::PreReleaseType::kNone, 0);
    constexpr Version v6(2, 0, 0, Version::PreReleaseType::kNone, 0);
    static_assert(v2 > v1, "");
    static_assert(v3 > v1, "");
    static_assert(v4 > v1, "");
    static_assert(v5 > v1, "");
    static_assert(v6 > v1, "");
  }
}

TEST_CASE("ToString")
{
  SECTION("std::string")
  {
    Version v(1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4);

    std::string s = v.ToString();
    REQUIRE(std::strcmp(s.c_str(), "1.2.3-rc.4") == 0);
  }

  SECTION("char*")
  {
    Version v(1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4);

    char s1[kVersionStringLength];
    v.ToString(s1);
    REQUIRE(std::strcmp(s1, "1.2.3-rc.4") == 0);

    char s2[11];
    v.ToString(s2);
    REQUIRE(std::strcmp(s2, "1.2.3-rc.4") == 0);
  }
}

TEST_CASE("FromString") {
  constexpr Version v(1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4);

  SECTION("std::string")
  {
    const std::string s("1.2.3-rc.4");

    Version v1(s);
    REQUIRE(v == v1);

    Version v2;
    v2.FromString(s);
    REQUIRE(v == v2);

    Version v3;
    FromString(&v3, s);
    REQUIRE(v == v3);
  }

  SECTION("char*")
  {
    const char s[] = "1.2.3-rc.4";

    Version v1(s);
    REQUIRE(v == v1);

    Version v2;
    v2.FromString(s);
    REQUIRE(v == v2);

    Version v3;
    FromString(&v3, s);
    REQUIRE(v == v3);
  }
}
