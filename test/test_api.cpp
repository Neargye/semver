#include <semver.hpp>
#include <doctest.h>
#include <array>
#include <string>

using namespace semver;

static version<> V(std::string_view s) { return from_string(s); }

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
    REQUIRE(compare(V("1.0.0-a"), V("1.0.0-z")) == -1);
    REQUIRE(compare(V("1.0.0-z"), V("1.0.0-a")) == 1);
  }
}

TEST_CASE("compare_with_build includes build metadata") {
  SUBCASE("same base, different build metadata") {
    REQUIRE(compare_with_build(V("1.0.0+build.1"), V("1.0.0+build.2")) == -1);
    REQUIRE(compare_with_build(V("1.0.0+build.2"), V("1.0.0+build.1")) ==  1);
    REQUIRE(compare_with_build(V("1.0.0+build.1"), V("1.0.0+build.1")) ==  0);
    REQUIRE(compare_with_build(V("1.0.0+build.10"), V("1.0.0+build.2")) == -1);
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

  SUBCASE("incomplete components are ignored") {
    const auto major = coerce("1.");
    const auto minor = coerce("1.2.");
    REQUIRE(major.has_value());
    REQUIRE(minor.has_value());
    REQUIRE(major->to_string() == "1.0.0");
    REQUIRE(minor->to_string() == "1.2.0");
  }

  SUBCASE("normal version unchanged") {
    const auto v = coerce("1.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("extra component is ignored after patch") {
    const auto v = coerce("1.2.3.4");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("trailing text is ignored after patch") {
    const auto v = coerce("1.2.3garbage");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("non-numeric returns nullopt") {
    REQUIRE_FALSE(coerce("abc").has_value());
  }

  SUBCASE("component overflow returns nullopt") {
    REQUIRE_FALSE(coerce("18446744073709551616.0.0").has_value());
    REQUIRE_FALSE(coerce("1.18446744073709551616.0").has_value());
    REQUIRE_FALSE(coerce("1.2.18446744073709551616").has_value());
    REQUIRE_FALSE(coerce<std::uint8_t>("256.0.0").has_value());
    REQUIRE_FALSE(coerce<std::uint8_t>("1.256.0").has_value());
    REQUIRE_FALSE(coerce<std::uint8_t>("1.2.256").has_value());
  }

  SUBCASE("prerelease suffix preserved") {
    const auto v = coerce("01.02.03-alpha.1");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3-alpha.1");
  }

  SUBCASE("valid shorthand qualifiers at the raw input limit are preserved") {
    std::string prerelease{"1-"};
    prerelease.append(max_input_length - prerelease.size(), 'a');
    REQUIRE(prerelease.size() == max_input_length);
    const auto prerelease_version = coerce(prerelease);
    REQUIRE(prerelease_version.has_value());
    REQUIRE(prerelease_version->prerelease_tag().size() == max_input_length - 2);

    std::string build{"1+"};
    build.append(max_input_length - build.size(), 'b');
    REQUIRE(build.size() == max_input_length);
    const auto build_version = coerce(build);
    REQUIRE(build_version.has_value());
    REQUIRE(build_version->build_metadata().size() == max_input_length - 2);
  }
}

TEST_CASE("inc with prerelease change increments numeric identifier") {
  SUBCASE("last identifier is numeric: increments it") {
    const auto v = inc(V("1.0.0-alpha.9"), version_change::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.0-alpha.10");
  }

  SUBCASE("bare numeric identifier: increments it") {
    const auto v = inc(V("1.0.0-9"), version_change::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.0-10");
  }

  SUBCASE("last identifier is alphanumeric: appends .0") {
    const auto v = inc(V("1.0.0-alpha"), version_change::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.0-alpha.0");
  }

  SUBCASE("numeric identifier larger than uint64_t increments exactly") {
    const auto v = inc(V("1.0.0-18446744073709551616"), version_change::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.0-18446744073709551617");
  }

  SUBCASE("arbitrarily long all-nines identifier grows by one digit") {
    const auto v = inc(V("1.0.0-alpha.999999999999999999999999999999"), version_change::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.0-alpha.1000000000000000000000000000000");
  }

  SUBCASE("no prerelease: bumps patch and adds -0") {
    const auto v = inc(V("1.0.0"), version_change::prerelease);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.0.1-0");
  }

  SUBCASE("none returns nullopt") {
    REQUIRE_FALSE(inc(V("1.0.0"), version_change::none).has_value());
  }

  SUBCASE("unknown change returns nullopt") {
    REQUIRE_FALSE(inc(V("1.0.0"), static_cast<version_change>(255)).has_value());
  }

  SUBCASE("premajor bumps major and adds -0") {
    const auto v = inc(V("1.2.3"), version_change::premajor);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "2.0.0-0");
  }

  SUBCASE("preminor bumps minor and adds -0") {
    const auto v = inc(V("1.2.3"), version_change::preminor);
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.3.0-0");
  }
}

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

TEST_CASE("max_satisfying and min_satisfying with prerelease policy") {
  std::array<version<>, 3> vs = {V("1.0.0-alpha"), V("1.0.0"), V("2.0.0-beta")};
  range_set<> rs;
  REQUIRE(parse(">=1.0.0-alpha <=2.0.0", rs));

  SUBCASE("exclude policy skips pre-releases") {
    const auto mx = max_satisfying(vs.begin(), vs.end(), rs, prerelease_policy::exclude);
    REQUIRE(mx != vs.end());
    REQUIRE(mx->to_string() == "1.0.0");
  }

  SUBCASE("include policy includes pre-releases") {
    const auto mx = max_satisfying(vs.begin(), vs.end(), rs, prerelease_policy::include);
    REQUIRE(mx != vs.end());
    REQUIRE(mx->to_string() == "2.0.0-beta");
  }

  SUBCASE("min_satisfying forwards the prerelease policy") {
    range_set<> any;
    REQUIRE(parse("*", any));

    const auto excluded = min_satisfying(vs.begin(), vs.end(), any);
    REQUIRE(excluded != vs.end());
    REQUIRE(excluded->to_string() == "1.0.0");

    const auto included = min_satisfying(vs.begin(), vs.end(), any, prerelease_policy::include);
    REQUIRE(included != vs.end());
    REQUIRE(included->to_string() == "1.0.0-alpha");
  }
}

TEST_CASE("clean strips prefix/suffix whitespace, = and v") {
  SUBCASE("leading spaces and = and v") {
    const auto v = clean("  =v1.2.3  ");
    REQUIRE(v.has_value());
    REQUIRE(v->to_string() == "1.2.3");
  }

  SUBCASE("spaces between wrappers") {
    const auto v = clean("  = v1.2.3  ");
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

  SUBCASE("input over configured maximum is rejected before trimming") {
    std::string input(SEMVER_MAX_INPUT_LENGTH + 1, ' ');
    input.replace(input.size() - 5, 5, "1.2.3");
    REQUIRE_FALSE(clean(input).has_value());
  }
}

TEST_CASE("coerce strips = prefix") {
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

TEST_CASE("inc() returns nullopt on integer overflow") {
  SUBCASE("major overflow with uint8_t") {
    const version<uint8_t> v{uint8_t{255}, uint8_t{0}, uint8_t{0}};
    CHECK_FALSE(inc(v, version_change::major).has_value());
    CHECK_FALSE(inc(v, version_change::premajor).has_value());
  }

  SUBCASE("minor overflow with uint8_t") {
    const version<uint8_t> v{uint8_t{1}, uint8_t{255}, uint8_t{0}};
    CHECK_FALSE(inc(v, version_change::minor).has_value());
    CHECK_FALSE(inc(v, version_change::preminor).has_value());
  }

  SUBCASE("patch overflow with uint8_t") {
    const version<uint8_t> v{uint8_t{1}, uint8_t{2}, uint8_t{255}};
    CHECK_FALSE(inc(v, version_change::patch).has_value());
    CHECK_FALSE(inc(v, version_change::prepatch).has_value());
  }

  SUBCASE("patch overflow for prerelease bump without existing tag") {
    const version<uint8_t> v{uint8_t{1}, uint8_t{2}, uint8_t{255}};
    CHECK_FALSE(inc(v, version_change::prerelease).has_value());
  }

  SUBCASE("prerelease with existing tag does not bump patch") {
    const version<uint8_t> v{uint8_t{1}, uint8_t{2}, uint8_t{255}, "alpha.1"};
    const auto r = inc(v, version_change::prerelease);
    REQUIRE(r.has_value());
    CHECK(r->to_string() == "1.2.255-alpha.2");
  }

  SUBCASE("uint64_t max major returns nullopt") {
    const version<uint64_t> v{std::numeric_limits<uint64_t>::max(), uint64_t{0}, uint64_t{0}};
    CHECK_FALSE(inc(v, version_change::major).has_value());
    CHECK_FALSE(inc(v, version_change::premajor).has_value());
  }
}

TEST_CASE("coerce() falls back to M.m.p on invalid suffix") {
  SUBCASE("purely-numeric leading-zero prerelease identifier falls back") {
    const auto v = coerce("1.0.0-01");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.0.0");
  }

  SUBCASE("multi-digit leading-zero numeric prerelease falls back") {
    const auto v = coerce("1.2.3-0123");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3");
  }

  SUBCASE("trailing dot in prerelease falls back") {
    const auto v = coerce("1.2.3-alpha.");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3");
  }

  SUBCASE("double dot in prerelease falls back") {
    const auto v = coerce("1.2.3-alpha..1");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.3");
  }

  SUBCASE("empty explicit qualifiers fall back") {
    for (const auto input : {"1.2.3-", "1.2.3+", "1.2.3-alpha+"}) {
      const auto v = coerce(input);
      REQUIRE(v.has_value());
      REQUIRE(v->to_string() == "1.2.3");
    }
  }

  SUBCASE("valid prerelease is preserved") {
    const auto v = coerce("1.2.3-alpha.1");
    REQUIRE(v.has_value());
    CHECK(v->prerelease_tag() == "alpha.1");
  }

  SUBCASE("valid build metadata is preserved") {
    const auto v = coerce("1.2.3+build.42");
    REQUIRE(v.has_value());
    CHECK(v->build_metadata() == "build.42");
  }
}
