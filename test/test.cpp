// semver c++ test
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
#include <sstream>

using namespace semver;

TEST_CASE("constructors") {
  SECTION("default") {
    constexpr Version v;
    static_assert(v.major == 0 &&
                  v.minor == 1 &&
                  v.patch == 0 &&
                  v.pre_release_type == Version::PreReleaseType::kNone &&
                  v.pre_release_version == 0, "");
  }
  SECTION("constructor") {
    constexpr Version v1(1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4);
    static_assert(v1.major == 1 &&
                  v1.minor == 2 &&
                  v1.patch == 3 &&
                  v1.pre_release_type == Version::PreReleaseType::kReleaseCandidate &&
                  v1.pre_release_version == 4, "");

    constexpr Version v2(1, 2, 3, Version::PreReleaseType::kNone, 4);
    static_assert(v2.major == 1 &&
                  v2.minor == 2 &&
                  v2.patch == 3 &&
                  v2.pre_release_type == Version::PreReleaseType::kNone &&
                  v2.pre_release_version == 0, "");
  }
}

TEST_CASE("operators") {
  constexpr Version v1_0_0(1, 0, 0, Version::PreReleaseType::kNone, 0);
  constexpr Version v1_0_0_a(1, 0, 0, Version::PreReleaseType::kAlpha, 0);
  constexpr Version v1_0_0_a_1(1, 0, 0, Version::PreReleaseType::kAlpha, 1);
  constexpr Version v1_0_1_a(1, 0, 1, Version::PreReleaseType::kAlpha, 0);
  constexpr Version v1_1_0_a(1, 1, 0, Version::PreReleaseType::kAlpha, 0);
  constexpr Version v2_0_0(2, 0, 0, Version::PreReleaseType::kNone, 0);

  SECTION("operator =") {
    constexpr Version v = v1_0_0;
    static_assert(v == v1_0_0, "");
  }

  SECTION("operator ==") {
    constexpr Version v = v1_0_0;
    static_assert(v == v1_0_0, "");
  }

  SECTION("operator !=") {
    static_assert(v1_0_0 != v1_0_0_a, "");

    static_assert(v1_0_0 != v1_1_0_a, "");

    static_assert(v1_0_0 != v1_0_1_a, "");

    static_assert(v1_0_0 != v1_0_0_a_1, "");

    static_assert(v1_0_0 != v2_0_0, "");

  }

  SECTION("operator >") {
    static_assert(v2_0_0 > v1_0_0, "");

    static_assert(v1_0_0 > v1_0_0_a, "");

    static_assert(v1_0_0 > v1_0_0_a_1, "");

    static_assert(v1_0_0_a_1 > v1_0_0_a, "");

    static_assert(v1_0_1_a > v1_0_0_a, "");

    static_assert(v1_0_1_a > v1_0_0_a_1, "");

    static_assert(v1_1_0_a > v1_0_0_a, "");

    static_assert(v1_1_0_a > v1_0_0_a_1, "");
  }

  SECTION("operator >=") {
    static_assert(v2_0_0 >= v1_0_0, "");

    static_assert(v1_0_0 >= v1_0_0_a, "");

    static_assert(v1_0_0 >= v1_0_0_a_1, "");

    static_assert(v1_0_0_a_1 >= v1_0_0_a, "");

    static_assert(v1_0_1_a >= v1_0_0_a, "");

    static_assert(v1_0_1_a >= v1_0_0_a_1, "");


    static_assert(v1_1_0_a >= v1_0_0_a, "");

    static_assert(v1_1_0_a >= v1_0_0_a_1, "");

    constexpr Version v = v1_0_0;
    static_assert(v1_0_0 >= v, "");
  }

  SECTION("operator <") {
    static_assert(v1_0_0 < v2_0_0, "");

    static_assert(v1_0_0_a < v1_0_0, "");

    static_assert(v1_0_0_a_1 < v1_0_0, "");

    static_assert(v1_0_0_a < v1_0_0_a_1, "");

    static_assert(v1_0_0_a < v1_0_1_a, "");

    static_assert(v1_0_0_a_1 < v1_0_1_a, "");

    static_assert(v1_0_0_a < v1_1_0_a, "");

    static_assert(v1_0_0_a_1 < v1_1_0_a, "");
  }

  SECTION("operator <=") {
    static_assert(v1_0_0 <= v2_0_0, "");

    static_assert(v1_0_0_a <= v1_0_0, "");

    static_assert(v1_0_0_a_1 <= v1_0_0, "");

    static_assert(v1_0_0_a <= v1_0_0_a_1, "");

    static_assert(v1_0_0_a <= v1_0_1_a, "");

    static_assert(v1_0_0_a_1 <= v1_0_1_a, "");

    static_assert(v1_0_0_a <= v1_1_0_a, "");

    static_assert(v1_0_0_a_1 <= v1_1_0_a, "");

    constexpr Version v = v1_0_0;
    static_assert(v1_0_0 <= v, "");
  }
}


TEST_CASE("from/to string") {
  const std::size_t versions_length = 19;
  std::array<Version, versions_length> versions = {{
      Version{1, 2, 3},
      Version{255, 255, 255},
      Version{0, 0, 0},
      //
      Version{1, 2, 3, Version::PreReleaseType::kNone, 0},
      Version{1, 2, 3, Version::PreReleaseType::kNone, 4},
      Version{255, 255, 255, Version::PreReleaseType::kNone, 255},
      Version{0, 0, 0, Version::PreReleaseType::kNone, 0},
      //
      Version{1, 2, 3, Version::PreReleaseType::kAlpha, 0},
      Version{1, 2, 3, Version::PreReleaseType::kAlpha, 4},
      Version{255, 255, 255, Version::PreReleaseType::kAlpha, 255},
      Version{0, 0, 0, Version::PreReleaseType::kAlpha, 0},
      //
      Version{1, 2, 3, Version::PreReleaseType::kBetha, 0},
      Version{1, 2, 3, Version::PreReleaseType::kBetha, 4},
      Version{255, 255, 255, Version::PreReleaseType::kBetha, 255},
      Version{0, 0, 0, Version::PreReleaseType::kBetha, 0},
      //
      Version{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4},
      Version{255, 255, 255, Version::PreReleaseType::kReleaseCandidate, 255},
      Version{0, 0, 0, Version::PreReleaseType::kReleaseCandidate, 0},
  }};
  std::array<std::string, versions_length> versions_strings = {{
      "1.2.3",
      "255.255.255",
      "0.0.0",
      //
      "1.2.3",
      "1.2.3",
      "255.255.255",
      "0.0.0",
      //
      "1.2.3-alpha",
      "1.2.3-alpha.4",
      "255.255.255-alpha.255",
      "0.0.0-alpha",
      //
      "1.2.3-betha",
      "1.2.3-betha.4",
      "255.255.255-betha.255",
      "0.0.0-betha",
      //
      "1.2.3-rc",
      "1.2.3-rc.4",
      "255.255.255-rc.255",
      "0.0.0-rc",
  }};

  SECTION("from std::string") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      Version v1(versions_strings[i]);
      REQUIRE(versions[i] == v1);

      Version v2;
      REQUIRE(v2.FromString(versions_strings[i]));
      REQUIRE(versions[i] == v2);

      Version v3;
      REQUIRE(FromString(&v3, versions_strings[i]));
      REQUIRE(versions[i] == v3);
    }
  }

  SECTION("from char*") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      Version v1(versions_strings[i].c_str());
      REQUIRE(versions[i] == v1);

      Version v2;
      REQUIRE(v2.FromString(versions_strings[i].c_str()));
      REQUIRE(versions[i] == v2);

      Version v3;
      REQUIRE(FromString(&v3, versions_strings[i].c_str()));
      REQUIRE(versions[i] == v3);
    }
  }

  SECTION("to std::string") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      const std::string s1 = versions[i].ToString();
      REQUIRE(s1 == versions_strings[i]);

      const std::string s2 = ToString(versions[i]);
      REQUIRE(s2 == versions_strings[i]);
    }
  }

  SECTION("to char*") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      char s1[kVersionStringLength];
      const auto size1 = versions[i].ToString(s1);
      REQUIRE(s1 == versions_strings[i]);
      REQUIRE(size1 == versions_strings[i].length());

      char s2[kVersionStringLength];
      const auto size2 = ToString(versions[i], s2);
      REQUIRE(s2 == versions_strings[i]);
      REQUIRE(size2 == versions_strings[i].length());
    }
  }

  SECTION("operator <<") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      std::stringstream os;
      os << versions[i];
      REQUIRE(os.str() == versions_strings[i]);
    }
  }

  SECTION("operator >>") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      std::stringstream is(versions_strings[i]);
      Version vi;
      is >> vi;
      REQUIRE(versions[i] == vi);
    }
  }
}
