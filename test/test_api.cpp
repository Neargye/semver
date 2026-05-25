// Tests for node-semver-inspired APIs: compare, rcompare, compare_with_build,
// coerce (leading-zero stripping), inc (numeric increment), max_satisfying,
// min_satisfying, clean.

#include <semver.hpp>
#include <doctest.h>
#include <array>
#include <string>

using namespace semver;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static version<> V(std::string_view s) { return from_string(s); }

// ---------------------------------------------------------------------------
// compare / rcompare
// ---------------------------------------------------------------------------

TEST_CASE("compare returns -1, 0, or 1") {
  SUBCASE("less than") {
    REQUIRE(compare(V("1.0.0"), V("2.0.0")) == -1);
    REQUIRE(compare(V("1.0.0-alpha"), V("1.0.0")) == -1);
    REQUIRE(compare(V("0.9.0"), V("1.0.0")) == -1);
  }

  SUBCASE("equal") {
    REQUIRE(compare(V("1.0.0"), V("1.0.0")) == 0);
    REQUIRE(compare(V("1.2.3-alpha"), V("1.2.3-alpha")) == 0);
  }

  SUBCASE("greater than") {
    REQUIRE(compare(V("2.0.0"), V("1.0.0")) == 1);
    REQUIRE(compare(V("1.0.0"), V("1.0.0-alpha")) == 1);
  }

  SUBCASE("prerelease ordering") {
    REQUIRE(compare(V("1.0.0-alpha"), V("1.0.0-alpha.1")) == -1);
    REQUIRE(compare(V("1.0.0-alpha.1"), V("1.0.0-alpha.beta")) == -1);
    REQUIRE(compare(V("1.0.0-alpha.beta"), V("1.0.0-beta")) == -1);
  }
}

TEST_CASE("rcompare is the reverse of compare") {
  REQUIRE(rcompare(V("1.0.0"), V("2.0.0")) == 1);
  REQUIRE(rcompare(V("2.0.0"), V("1.0.0")) == -1);
  REQUIRE(rcompare(V("1.0.0"), V("1.0.0")) == 0);

  SUBCASE("rcompare sorts descending") {
    std::array<version<>, 4> vs = {V("1.0.0"), V("3.0.0"), V("2.0.0"), V("1.5.0")};
    std::sort(vs.begin(), vs.end(), [](const auto& a, const auto& b) {
      return rcompare(a, b) < 0;
    });
    REQUIRE(vs[0].to_string() == "3.0.0");
    REQUIRE(vs[1].to_string() == "2.0.0");
    REQUIRE(vs[2].to_string() == "1.5.0");
    REQUIRE(vs[3].to_string() == "1.0.0");
  }
}

// ---------------------------------------------------------------------------
// compare_with_build
// ---------------------------------------------------------------------------

TEST_CASE("compare_with_build includes build metadata") {
  SUBCASE("same base, different build metadata") {
    REQUIRE(compare_with_build(V("1.0.0+build.1"), V("1.0.0+build.2")) == -1);
    REQUIRE(compare_with_build(V("1.0.0+build.2"), V("1.0.0+build.1")) ==  1);
    REQUIRE(compare_with_build(V("1.0.0+build.1"), V("1.0.0+build.1")) ==  0);
  }

  SUBCASE("different versions override build metadata ordering") {
    REQUIRE(compare_with_build(V("2.0.0+zzz"), V("1.0.0+aaa")) == 1);
    REQUIRE(compare_with_build(V("1.0.0+zzz"), V("2.0.0+aaa")) == -1);
  }

  SUBCASE("no build metadata equals empty build metadata") {
    REQUIRE(compare_with_build(V("1.0.0"), V("1.0.0")) == 0);
    REQUIRE(compare_with_build(V("1.0.0+build"), V("1.0.0")) == 1);
    REQUIRE(compare_with_build(V("1.0.0"), V("1.0.0+build")) == -1);
  }
}

// ---------------------------------------------------------------------------
// coerce — tolerant parsing with leading-zero stripping
// ---------------------------------------------------------------------------

TEST_CASE("coerce strips leading zeros and fills missing components") {
  SUBCASE("leading zero on major") {
    const auto v = coerce("01.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("leading zero on minor") {
    const auto v = coerce("1.02.3");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("leading zero on patch") {
    const auto v = coerce("1.2.03");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("v prefix with leading zero") {
    const auto v = coerce("v01.2");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.0");
  }

  SUBCASE("single component") {
    const auto v = coerce("001");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.0");
  }

  SUBCASE("two components") {
    const auto v = coerce("1.02");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.0");
  }

  SUBCASE("normal version unchanged") {
    const auto v = coerce("1.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("non-numeric returns nullopt") {
    REQUIRE_FALSE(coerce("abc").has_value());
  }

  SUBCASE("prerelease suffix preserved") {
    const auto v = coerce("01.02.03-alpha.1");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3-alpha.1");
  }
}

// ---------------------------------------------------------------------------
// inc — bump version by diff type
// ---------------------------------------------------------------------------

TEST_CASE("inc with prerelease diff increments numeric identifier") {
  SUBCASE("last identifier is numeric: increments it") {
    const auto v = inc(V("1.0.0-alpha.9"), version_diff::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.0-alpha.10");
  }

  SUBCASE("bare numeric identifier: increments it") {
    const auto v = inc(V("1.0.0-9"), version_diff::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.0-10");
  }

  SUBCASE("last identifier is alphanumeric: appends .0") {
    const auto v = inc(V("1.0.0-alpha"), version_diff::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.0-alpha.0");
  }

  SUBCASE("no prerelease: bumps patch and adds -0") {
    const auto v = inc(V("1.0.0"), version_diff::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.1-0");
  }

  SUBCASE("none returns nullopt") {
    REQUIRE_FALSE(inc(V("1.0.0"), version_diff::none).has_value());
  }

  SUBCASE("premajor bumps major and adds -0") {
    const auto v = inc(V("1.2.3"), version_diff::premajor);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "2.0.0-0");
  }

  SUBCASE("preminor bumps minor and adds -0") {
    const auto v = inc(V("1.2.3"), version_diff::preminor);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.3.0-0");
  }
}

// ---------------------------------------------------------------------------
// max_satisfying / min_satisfying
// ---------------------------------------------------------------------------

TEST_CASE("max_satisfying returns highest matching version") {
  std::array<version<>, 4> vs = {V("1.0.0"), V("1.2.3"), V("2.0.0"), V("0.9.0")};
  range_set<> rs;
  REQUIRE(parse(">=1.0.0 <2.0.0", rs));

  SUBCASE("returns highest in range") {
    const auto it = max_satisfying(vs.begin(), vs.end(), rs);
    REQUIRE(it != vs.end());
    REQUIRE(it->to_string() == "1.2.3");
  }

  SUBCASE("returns last when no match") {
    range_set<> rs2;
    REQUIRE(parse(">=3.0.0", rs2));
    REQUIRE(max_satisfying(vs.begin(), vs.end(), rs2) == vs.end());
  }

  SUBCASE("single element satisfying range") {
    range_set<> rs3;
    REQUIRE(parse("=0.9.0", rs3));
    const auto it = max_satisfying(vs.begin(), vs.end(), rs3);
    REQUIRE(it != vs.end());
    REQUIRE(it->to_string() == "0.9.0");
  }

  SUBCASE("empty range returns last") {
    REQUIRE(max_satisfying(vs.begin(), vs.begin(), rs) == vs.begin());
  }
}

TEST_CASE("min_satisfying returns lowest matching version") {
  std::array<version<>, 4> vs = {V("1.0.0"), V("1.2.3"), V("2.0.0"), V("0.9.0")};
  range_set<> rs;
  REQUIRE(parse(">=1.0.0 <2.0.0", rs));

  SUBCASE("returns lowest in range") {
    const auto it = min_satisfying(vs.begin(), vs.end(), rs);
    REQUIRE(it != vs.end());
    REQUIRE(it->to_string() == "1.0.0");
  }

  SUBCASE("returns last when no match") {
    range_set<> rs2;
    REQUIRE(parse(">=3.0.0", rs2));
    REQUIRE(min_satisfying(vs.begin(), vs.end(), rs2) == vs.end());
  }
}

TEST_CASE("max_satisfying and min_satisfying with prerelease option") {
  std::array<version<>, 3> vs = {V("1.0.0-alpha"), V("1.0.0"), V("2.0.0-beta")};
  range_set<> rs;
  REQUIRE(parse(">=1.0.0-alpha <=2.0.0", rs));

  SUBCASE("exclude_prerelease skips pre-releases") {
    const auto mx = max_satisfying(vs.begin(), vs.end(), rs,
                                   version_compare_option::exclude_prerelease);
    REQUIRE(mx != vs.end());
    REQUIRE(mx->to_string() == "1.0.0");
  }

  SUBCASE("include_prerelease includes pre-releases") {
    const auto mx = max_satisfying(vs.begin(), vs.end(), rs,
                                   version_compare_option::include_prerelease);
    REQUIRE(mx != vs.end());
    REQUIRE(mx->to_string() == "2.0.0-beta");
  }
}

// ---------------------------------------------------------------------------
// clean
// ---------------------------------------------------------------------------

TEST_CASE("clean strips prefix/suffix whitespace, = and v") {
  SUBCASE("leading spaces and = and v") {
    const auto v = clean("  =v1.2.3  ");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("v prefix only") {
    const auto v = clean("v1.2.3-alpha");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3-alpha");
  }

  SUBCASE("bare version passes through") {
    const auto v = clean("1.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("range operator returns nullopt") {
    REQUIRE_FALSE(clean("~1.2.3").has_value());
    REQUIRE_FALSE(clean(">=1.2.3").has_value());
  }

  SUBCASE("incomplete version returns nullopt") {
    REQUIRE_FALSE(clean("1.2").has_value());
    REQUIRE_FALSE(clean("1").has_value());
  }

  SUBCASE("build metadata preserved") {
    const auto v = clean("v1.2.3+build.42");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3+build.42");
  }

  SUBCASE("prerelease and build metadata preserved") {
    const auto v = clean("1.2.3-alpha.1+build");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3-alpha.1+build");
  }
}

// ---------------------------------------------------------------------------
// coerce: = prefix stripping (P2-4 fix — node-semver alignment)
// node-semver coerce() accepts "=1.2.3", "= v1.2.3" etc. We now match that.
// ---------------------------------------------------------------------------
TEST_CASE("coerce strips = prefix (P2-4 fix)") {
  SUBCASE("bare = prefix") {
    const auto v = coerce("=1.2.3");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3");
  }

  SUBCASE("= with space before version number") {
    const auto v = coerce("= 1.2.3");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3");
  }

  SUBCASE("= followed by v prefix") {
    const auto v = coerce("=v1.2.3");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3");
  }

  SUBCASE("= with space then v") {
    const auto v = coerce("= v1.2.3");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3");
  }

  SUBCASE("leading whitespace before =") {
    const auto v = coerce("  =1.2.3");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3");
  }

  SUBCASE("= combined with leading-zero stripping") {
    const auto v = coerce("=01.02.03");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3");
  }

  SUBCASE("= with missing minor/patch fills with 0") {
    const auto v1 = coerce("=1");
    REQUIRE(v1.has_value());
    CHECK(v1->to_string() == "1.0.0");

    const auto v2 = coerce("=1.2");
    REQUIRE(v2.has_value());
    CHECK(v2->to_string() == "1.2.0");
  }

  SUBCASE("= with prerelease suffix") {
    const auto v = coerce("=1.2.3-alpha.1");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3-alpha.1");
  }

  SUBCASE("== is not stripped: only one = removed, second = is not a digit/v") {
    CHECK_FALSE(coerce("==1.2.3").has_value());
  }

  SUBCASE("bare = with no number returns nullopt") {
    CHECK_FALSE(coerce("=").has_value());
    CHECK_FALSE(coerce("=abc").has_value());
  }
}
