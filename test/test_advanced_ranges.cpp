#include <semver.hpp>
#include <doctest.h>
#include <array>
#include <string>

using namespace semver;

static bool in(const char* range_str, const char* ver_str) {
  range_set<> rs;
  if (!parse(range_str, rs)) return false;
  const auto v = try_parse(ver_str);
  return v && rs.contains(*v, include_prerelease);
}

static bool in_def(const char* range_str, const char* ver_str) {
  range_set<> rs;
  if (!parse(range_str, rs)) return false;
  const auto v = try_parse(ver_str);
  return v && rs.contains(*v);
}

TEST_CASE("version 4-arg constructor with prerelease") {
  SUBCASE("serialization round-trip") {
    const version<> v{1, 2, 3, "alpha.1"};
    CHECK(v.major() == 1);
    CHECK(v.minor() == 2);
    CHECK(v.patch() == 3);
    CHECK(v.prerelease_tag() == "alpha.1");
    CHECK(v.build_metadata().empty());
    CHECK(v.to_string() == "1.2.3-alpha.1");
  }

  SUBCASE("prerelease is less than same-M.m.p release") {
    const version<> pre{1, 2, 3, "alpha"};
    const version<> rel{1, 2, 3};
    CHECK(pre < rel);
    CHECK(rel > pre);
    CHECK(compare(pre, rel) == -1);
    CHECK(compare(rel, pre) ==  1);
  }

  SUBCASE("upper-bound sentinel -0 ordering") {
    const version<> sentinel{1, 3, 0, "0"};
    const version<> release{1, 3, 0};
    const version<> pre_alpha{1, 3, 0, "alpha"};
    const version<> pre_1{1, 3, 0, "1"};

    CHECK(sentinel < release);
    CHECK(sentinel < pre_alpha);
    CHECK(sentinel < pre_1);
    CHECK(sentinel == version<>{1, 3, 0, "0"});
  }

  SUBCASE("empty prerelease string equals no-prerelease constructor") {
    const version<> a{1, 0, 0, ""};
    const version<> b{1, 0, 0};
    CHECK(compare(a, b) == 0);
    CHECK(a == b);
  }
}

TEST_CASE("x-ranges") {
  SUBCASE("* matches any release") {
    CHECK(in("*", "0.0.0"));
    CHECK(in("*", "1.2.3"));
    CHECK(in("*", "99.0.0"));
  }
  SUBCASE("x is equivalent to *") {
    CHECK(in("x", "1.2.3"));
    CHECK(in("X", "1.2.3"));
  }
  SUBCASE("M.x  =>  >=M.0.0 <(M+1).0.0-0") {
    CHECK(in("1.x", "1.0.0"));
    CHECK(in("1.x", "1.9.9"));
    CHECK_FALSE(in("1.x", "2.0.0"));
    CHECK_FALSE(in("1.x", "0.9.9"));
    CHECK(in("1.*", "1.5.0"));
    CHECK_FALSE(in("1.*", "2.0.0"));
  }
  SUBCASE("M.m.x  =>  >=M.m.0 <M.(m+1).0-0") {
    CHECK(in("1.2.x", "1.2.0"));
    CHECK(in("1.2.x", "1.2.9"));
    CHECK_FALSE(in("1.2.x", "1.3.0"));
    CHECK_FALSE(in("1.2.x", "1.1.9"));
    CHECK(in("1.2.*", "1.2.5"));
  }
  SUBCASE("bare M.m  treated as M.m.x") {
    CHECK(in("1.2", "1.2.0"));
    CHECK(in("1.2", "1.2.9"));
    CHECK_FALSE(in("1.2", "1.3.0"));
  }
  SUBCASE("bare M  treated as M.x") {
    CHECK(in("1", "1.0.0"));
    CHECK(in("1", "1.99.99"));
    CHECK_FALSE(in("1", "2.0.0"));
    CHECK_FALSE(in("1", "0.9.9"));
  }
}

TEST_CASE("tilde ranges") {
  SUBCASE("~M.m.p  =>  >=M.m.p  <M.(m+1).0-0") {
    CHECK(in("~1.2.3", "1.2.3"));
    CHECK(in("~1.2.3", "1.2.9"));
    CHECK_FALSE(in("~1.2.3", "1.3.0"));
    CHECK_FALSE(in("~1.2.3", "1.2.2"));
    CHECK_FALSE(in("~1.2.3", "2.0.0"));
  }
  SUBCASE("~1.2  =>  >=1.2.0  <1.3.0-0") {
    CHECK(in("~1.2", "1.2.0"));
    CHECK(in("~1.2", "1.2.9"));
    CHECK_FALSE(in("~1.2", "1.3.0"));
    CHECK_FALSE(in("~1.2", "1.1.9"));
  }
  SUBCASE("~1  =>  >=1.0.0  <2.0.0-0") {
    CHECK(in("~1", "1.0.0"));
    CHECK(in("~1", "1.9.9"));
    CHECK_FALSE(in("~1", "2.0.0"));
    CHECK_FALSE(in("~1", "0.9.9"));
  }
  SUBCASE("~0.2.3  =>  >=0.2.3  <0.3.0-0") {
    CHECK(in("~0.2.3", "0.2.3"));
    CHECK(in("~0.2.3", "0.2.9"));
    CHECK_FALSE(in("~0.2.3", "0.3.0"));
    CHECK_FALSE(in("~0.2.3", "0.2.2"));
  }
  SUBCASE("~0  =>  >=0.0.0  <1.0.0-0") {
    CHECK(in("~0", "0.0.0"));
    CHECK(in("~0", "0.9.9"));
    CHECK_FALSE(in("~0", "1.0.0"));
  }

  SUBCASE("tilde with prerelease lower bound  =>  >=M.m.p-pre  <M.(m+1).0-0") {
    // Lower bound carries a prerelease; only versions >= that prerelease match.
    CHECK(in("~1.2.3-beta", "1.2.3-beta"));     // exact lower bound
    CHECK(in("~1.2.3-beta", "1.2.3-beta.1"));  // beta.1 > beta
    CHECK(in("~1.2.3-beta", "1.2.3-rc"));       // rc > beta
    CHECK(in("~1.2.3-beta", "1.2.5"));           // release inside range
    CHECK_FALSE(in("~1.2.3-beta", "1.2.3-alpha")); // alpha < beta
    CHECK_FALSE(in("~1.2.3-beta", "1.3.0"));        // above upper bound
  }
}

TEST_CASE("caret ranges") {
  SUBCASE("^M.m.p (M>0)  =>  >=M.m.p  <(M+1).0.0-0") {
    CHECK(in("^1.2.3", "1.2.3"));
    CHECK(in("^1.2.3", "1.9.9"));
    CHECK(in("^1.2.3", "1.2.4"));
    CHECK_FALSE(in("^1.2.3", "2.0.0"));
    CHECK_FALSE(in("^1.2.3", "1.2.2"));
  }
  SUBCASE("^0.m.p (m>0)  =>  >=0.m.p  <0.(m+1).0-0") {
    CHECK(in("^0.2.3", "0.2.3"));
    CHECK(in("^0.2.3", "0.2.9"));
    CHECK_FALSE(in("^0.2.3", "0.3.0"));
    CHECK_FALSE(in("^0.2.3", "0.2.2"));
    CHECK_FALSE(in("^0.2.3", "1.0.0"));
  }
  SUBCASE("^0.0.p  =>  >=0.0.p  <0.0.(p+1)-0") {
    CHECK(in("^0.0.3", "0.0.3"));
    CHECK_FALSE(in("^0.0.3", "0.0.4"));
    CHECK_FALSE(in("^0.0.3", "0.0.2"));
  }
  SUBCASE("^1.2  =>  >=1.2.0  <2.0.0-0") {
    CHECK(in("^1.2", "1.2.0"));
    CHECK(in("^1.2", "1.9.9"));
    CHECK_FALSE(in("^1.2", "2.0.0"));
  }
  SUBCASE("^0.2  =>  >=0.2.0  <0.3.0-0") {
    CHECK(in("^0.2", "0.2.0"));
    CHECK(in("^0.2", "0.2.9"));
    CHECK_FALSE(in("^0.2", "0.3.0"));
  }
  SUBCASE("^0.0  =>  >=0.0.0  <0.1.0-0") {
    CHECK(in("^0.0", "0.0.0"));
    CHECK(in("^0.0", "0.0.9"));
    CHECK_FALSE(in("^0.0", "0.1.0"));
  }
  SUBCASE("^1  =>  >=1.0.0  <2.0.0-0") {
    CHECK(in("^1", "1.0.0"));
    CHECK(in("^1", "1.9.9"));
    CHECK_FALSE(in("^1", "2.0.0"));
    CHECK_FALSE(in("^1", "0.9.9"));
  }
  SUBCASE("^0  =>  >=0.0.0  <1.0.0-0") {
    CHECK(in("^0", "0.0.0"));
    CHECK(in("^0", "0.9.9"));
    CHECK_FALSE(in("^0", "1.0.0"));
  }

  SUBCASE("caret with prerelease lower bound") {
    // ^0.2.3-beta  =>  >=0.2.3-beta  <0.3.0-0
    CHECK(in("^0.2.3-beta", "0.2.3-beta"));
    CHECK(in("^0.2.3-beta", "0.2.3-rc"));
    CHECK(in("^0.2.3-beta", "0.2.5"));
    CHECK_FALSE(in("^0.2.3-beta", "0.2.3-alpha")); // below lower bound
    CHECK_FALSE(in("^0.2.3-beta", "0.3.0"));        // above upper bound
  }
}

TEST_CASE("hyphen ranges") {
  SUBCASE("M.m.p - M.m.p  =>  >=first  <=last") {
    CHECK(in("1.2.3 - 2.3.4", "1.2.3"));
    CHECK(in("1.2.3 - 2.3.4", "2.0.0"));
    CHECK(in("1.2.3 - 2.3.4", "2.3.4"));
    CHECK_FALSE(in("1.2.3 - 2.3.4", "1.2.2"));
    CHECK_FALSE(in("1.2.3 - 2.3.4", "2.3.5"));
  }
  SUBCASE("M.m - M.m.p  =>  >=M.m.0  <=last") {
    CHECK(in("1.2 - 2.3.4", "1.2.0"));
    CHECK(in("1.2 - 2.3.4", "2.3.4"));
    CHECK_FALSE(in("1.2 - 2.3.4", "1.1.9"));
    CHECK_FALSE(in("1.2 - 2.3.4", "2.3.5"));
  }
  SUBCASE("M.m.p - M.m  =>  >=first  <M.(m+1).0-0") {
    CHECK(in("1.2.3 - 2.3", "1.2.3"));
    CHECK(in("1.2.3 - 2.3", "2.3.9"));
    CHECK_FALSE(in("1.2.3 - 2.3", "2.4.0"));
    CHECK_FALSE(in("1.2.3 - 2.3", "1.2.2"));
  }
  SUBCASE("M.m.p - M  =>  >=first  <(M+1).0.0-0") {
    CHECK(in("1.2.3 - 2", "1.2.3"));
    CHECK(in("1.2.3 - 2", "2.9.9"));
    CHECK_FALSE(in("1.2.3 - 2", "3.0.0"));
    CHECK_FALSE(in("1.2.3 - 2", "1.2.2"));
  }
  SUBCASE("hyphen range with prerelease boundaries") {
    // Full versions with prerelease: >=lo-pre <=hi-pre
    CHECK(in("1.2.3-alpha - 2.0.0-beta", "1.2.3-alpha"));
    CHECK(in("1.2.3-alpha - 2.0.0-beta", "1.5.0"));
    CHECK(in("1.2.3-alpha - 2.0.0-beta", "2.0.0-alpha")); // alpha < beta
    CHECK(in("1.2.3-alpha - 2.0.0-beta", "2.0.0-beta"));  // exact upper bound
    CHECK_FALSE(in("1.2.3-alpha - 2.0.0-beta", "2.0.0-rc")); // rc > beta
    CHECK_FALSE(in("1.2.3-alpha - 2.0.0-beta", "1.2.3-0"));  // 0 < alpha
  }
}

TEST_CASE("combined advanced range syntax") {
  SUBCASE("~1.2 || ^2.0") {
    CHECK(in("~1.2 || ^2.0", "1.2.5"));
    CHECK(in("~1.2 || ^2.0", "2.3.0"));
    CHECK_FALSE(in("~1.2 || ^2.0", "1.3.0"));
    CHECK_FALSE(in("~1.2 || ^2.0", "3.0.0"));
  }
  SUBCASE("tilde combined with explicit operator") {
    CHECK(in("~1.2 >=1.2.5", "1.2.5"));
    CHECK(in("~1.2 >=1.2.5", "1.2.9"));
    CHECK_FALSE(in("~1.2 >=1.2.5", "1.2.4"));
    CHECK_FALSE(in("~1.2 >=1.2.5", "1.3.0"));
  }
  SUBCASE("caret combined with explicit operator") {
    CHECK(in("^1.0 >=1.2.0", "1.2.0"));
    CHECK(in("^1.0 >=1.2.0", "1.9.9"));
    CHECK_FALSE(in("^1.0 >=1.2.0", "1.1.9"));
    CHECK_FALSE(in("^1.0 >=1.2.0", "2.0.0"));
  }
}

TEST_CASE("advanced ranges with exclude_prerelease (default)") {
  SUBCASE("tilde: releases in range included, prereleases excluded") {
    // ~1.2.3 => >=1.2.3 <1.3.0-0
    CHECK(in_def("~1.2.3", "1.2.3"));    // exact lower bound (release)
    CHECK(in_def("~1.2.3", "1.2.9"));    // release inside range
    CHECK_FALSE(in_def("~1.2.3", "1.2.9-alpha"));
    CHECK_FALSE(in_def("~1.2.3", "1.3.0"));        // above upper bound
  }

  SUBCASE("upper-bound sentinel -0 excludes ALL versions >= 1.3.0-0") {
    CHECK_FALSE(in_def("~1.2.3", "1.3.0-0"));
    CHECK_FALSE(in_def("~1.2.3", "1.3.0-1"));
    CHECK_FALSE(in_def("~1.2.3", "1.3.0-alpha"));
    CHECK_FALSE(in_def("~1.2.3", "1.3.0"));
  }

  SUBCASE("tilde with prerelease lower bound: same-M.m.p prereleases included by filter") {
    // ~1.2.3-beta => >=1.2.3-beta <1.3.0-0
    // With exclude_prerelease: a prerelease at 1.2.3 passes the filter because
    // the >=1.2.3-beta comparator explicitly targets 1.2.3 with prerelease.
    CHECK(in_def("~1.2.3-beta", "1.2.3-beta"));      // exact lower bound
    CHECK(in_def("~1.2.3-beta", "1.2.3-beta.1"));    // beta.1 > beta
    CHECK(in_def("~1.2.3-beta", "1.2.3-rc"));        // rc > beta
    CHECK(in_def("~1.2.3-beta", "1.2.5"));           // release inside range
    CHECK_FALSE(in_def("~1.2.3-beta", "1.2.3-alpha"));  // alpha < beta
    CHECK_FALSE(in_def("~1.2.3-beta", "1.2.5-alpha"));  // no comparator targets 1.2.5
    CHECK_FALSE(in_def("~1.2.3-beta", "1.3.0"));        // above upper bound
  }

  SUBCASE("caret: releases included, prereleases excluded") {
    // ^1.2.3 => >=1.2.3 <2.0.0-0
    CHECK(in_def("^1.2.3", "1.9.9"));              // release included
    CHECK_FALSE(in_def("^1.2.3", "1.9.9-alpha")); // prerelease excluded (no comparator at 1.9.9)
    CHECK_FALSE(in_def("^1.2.3", "2.0.0-0"));     // upper bound excluded
  }

  SUBCASE("x-range: releases included, prereleases excluded") {
    // 1.x => >=1.0.0 <2.0.0-0
    CHECK(in_def("1.x", "1.5.0"));              // release included
    CHECK_FALSE(in_def("1.x", "1.5.0-alpha")); // prerelease excluded
    CHECK_FALSE(in_def("1.x", "2.0.0"));       // upper bound excluded
  }

  SUBCASE("* (star): matches all releases") {
    CHECK(in_def("*", "0.0.0"));
    CHECK(in_def("*", "99.9.9"));
    // prereleases excluded because * produces >=0.0.0 with no upper bound,
    // and no comparator has a prerelease tag → prerelease filter blocks them
    CHECK_FALSE(in_def("*", "1.0.0-alpha"));
  }
}

TEST_CASE("satisfies with advanced range syntax") {
  SUBCASE("tilde via satisfies") {
    const auto v = *try_parse("1.2.5");
    CHECK(satisfies(v, "~1.2.3"));
    CHECK_FALSE(satisfies(v, "~1.3.0"));
  }

  SUBCASE("caret via satisfies") {
    const auto v = *try_parse("1.9.0");
    CHECK(satisfies(v, "^1.2.3"));
    CHECK_FALSE(satisfies(v, "^2.0.0"));
  }

  SUBCASE("x-range via satisfies") {
    const auto v = *try_parse("1.5.0");
    CHECK(satisfies(v, "1.x"));
    CHECK(satisfies(v, "1.*"));
    CHECK_FALSE(satisfies(v, "2.x"));
  }

  SUBCASE("hyphen range via satisfies") {
    const auto v = *try_parse("1.5.0");
    CHECK(satisfies(v, "1.0.0 - 2.0.0"));
    CHECK_FALSE(satisfies(v, "2.0.0 - 3.0.0"));
  }
}

TEST_CASE("max_satisfying and min_satisfying with advanced ranges") {
  const std::array<version<>, 5> pool = {
    *try_parse("1.0.0"), *try_parse("1.2.0"), *try_parse("1.3.0"),
    *try_parse("2.0.0"), *try_parse("2.5.0")
  };

  SUBCASE("tilde") {
    range_set<> rs; REQUIRE(parse("~1.2", rs));
    const auto hi = max_satisfying(pool.begin(), pool.end(), rs);
    const auto lo = min_satisfying(pool.begin(), pool.end(), rs);
    REQUIRE(hi != pool.end());
    REQUIRE(lo != pool.end());
    CHECK(hi->to_string() == "1.2.0");
    CHECK(lo->to_string() == "1.2.0");
  }

  SUBCASE("caret") {
    range_set<> rs; REQUIRE(parse("^1.0.0", rs));
    const auto hi = max_satisfying(pool.begin(), pool.end(), rs);
    const auto lo = min_satisfying(pool.begin(), pool.end(), rs);
    REQUIRE(hi != pool.end());
    REQUIRE(lo != pool.end());
    CHECK(hi->to_string() == "1.3.0");
    CHECK(lo->to_string() == "1.0.0");
  }

  SUBCASE("x-range") {
    range_set<> rs; REQUIRE(parse("2.x", rs));
    const auto hi = max_satisfying(pool.begin(), pool.end(), rs);
    const auto lo = min_satisfying(pool.begin(), pool.end(), rs);
    REQUIRE(hi != pool.end());
    REQUIRE(lo != pool.end());
    CHECK(hi->to_string() == "2.5.0");
    CHECK(lo->to_string() == "2.0.0");
  }

  SUBCASE("hyphen range") {
    range_set<> rs; REQUIRE(parse("1.0.0 - 1.3.0", rs));
    const auto hi = max_satisfying(pool.begin(), pool.end(), rs);
    const auto lo = min_satisfying(pool.begin(), pool.end(), rs);
    REQUIRE(hi != pool.end());
    REQUIRE(lo != pool.end());
    CHECK(hi->to_string() == "1.3.0");
    CHECK(lo->to_string() == "1.0.0");
  }

  SUBCASE("no match returns end iterator") {
    range_set<> rs; REQUIRE(parse("~9.0.0", rs));
    CHECK(max_satisfying(pool.begin(), pool.end(), rs) == pool.end());
    CHECK(min_satisfying(pool.begin(), pool.end(), rs) == pool.end());
  }
}
