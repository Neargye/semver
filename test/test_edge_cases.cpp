#include <semver.hpp>
#include <doctest.h>
#include <string>
#include <sstream>
#include <array>

using namespace semver;

TEST_CASE("prerelease tag comparison correctness") {
  // Parsed versions: the parser guarantees no leading zeros in numeric identifiers.
  // These must compare correctly regardless of string length.
  SUBCASE("numeric identifiers compared by value, not length") {
    // "9" vs "10": 9 < 10, i.e. shorter string is numerically less
    const auto v9  = *try_parse("1.0.0-9");
    const auto v10 = *try_parse("1.0.0-10");
    CHECK(v9 < v10);
    CHECK(compare(v9, v10) == -1);

    // "99" < "100"
    const auto v99  = *try_parse("1.0.0-99");
    const auto v100 = *try_parse("1.0.0-100");
    CHECK(v99 < v100);
  }

  SUBCASE("alphanumeric identifiers compared lexicographically") {
    const auto va = *try_parse("1.0.0-alpha");
    const auto vb = *try_parse("1.0.0-beta");
    CHECK(va < vb);
    CHECK(compare(va, vb) == -1);
  }

  SUBCASE("numeric < alphanumeric (spec \u00a711.4.1)") {
    const auto vnum  = *try_parse("1.0.0-1");
    const auto valph = *try_parse("1.0.0-alpha");
    CHECK(vnum < valph);
    CHECK(compare(vnum, valph) == -1);
  }

  SUBCASE("4-arg constructor with valid tags produces correct comparisons") {
    const version<> v0{1, 0, 0, "0"};
    const version<> v1{1, 0, 0, "1"};
    const version<> va{1, 0, 0, "alpha"};
    const version<> rel{1, 0, 0};

    CHECK(v0 < v1);
    CHECK(v1 < va);  // numeric < alphanumeric
    CHECK(va < rel); // prerelease < release
    CHECK(v0 == version<>{1, 0, 0, "0"});
  }

  SUBCASE("4-arg constructor: empty prerelease equals no-prerelease") {
    const version<> a{1, 2, 3, ""};
    const version<> b{1, 2, 3};
    CHECK(a == b);
    CHECK(compare(a, b) == 0);
  }
}

TEST_CASE("to_chars serialization correctness") {
  SUBCASE("single-digit components") {
    char buf[32];
    const auto v = *try_parse("1.2.3");
    const auto r = to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    CHECK(std::string_view{buf, static_cast<std::size_t>(r.ptr - buf)} == "1.2.3");
  }

  SUBCASE("multi-digit components") {
    char buf[64];
    const auto v = *try_parse("10.200.3000");
    const auto r = to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    CHECK(std::string_view{buf, static_cast<std::size_t>(r.ptr - buf)} == "10.200.3000");
  }

  SUBCASE("with prerelease and build") {
    char buf[64];
    const auto v = *try_parse("1.2.3-alpha.1+build.42");
    const auto r = to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    CHECK(std::string_view{buf, static_cast<std::size_t>(r.ptr - buf)} == "1.2.3-alpha.1+build.42");
  }

  SUBCASE("buffer too small returns value_too_large") {
    char buf[4]; // too small for "1.2.3"
    const auto v = *try_parse("1.2.3");
    const auto r = to_chars(buf, buf + sizeof(buf), v);
    CHECK(!r);
    CHECK(r.ec == std::errc::value_too_large);
  }

  SUBCASE("round-trip: parse → to_chars → parse") {
    constexpr std::array<std::string_view, 5> cases = {{
      "0.0.0", "1.2.3", "10.20.30", "1.0.0-alpha.1+build", "999999999.999999999.999999999"
    }};
    for (const auto s : cases) {
      const auto v = *try_parse(std::string{s});
      CHECK(v.to_string() == s);
    }
  }
}

TEST_CASE("leading zero detection boundary") {
  SUBCASE("'0' alone is valid (single zero numeric identifier)") {
    CHECK(valid("1.0.0-0"));
    CHECK(valid("1.0.0-0.alpha.0"));
  }

  SUBCASE("'00' is invalid (leading zero in numeric identifier)") {
    CHECK_FALSE(valid("1.0.0-00"));
    CHECK_FALSE(valid("1.0.0-00.alpha"));
    CHECK_FALSE(valid("1.0.0-alpha.00"));
    CHECK_FALSE(valid("1.2.3-0123"));
    CHECK_FALSE(valid("1.2.3-0123.0123"));
  }

  SUBCASE("'0alpha' is valid (alphanumeric, leading zero rule does not apply)") {
    CHECK(valid("1.0.0-0alpha"));
    CHECK(valid("1.0.0-0alpha.1"));
  }

  SUBCASE("'01b' is valid (alphanumeric)") {
    CHECK(valid("1.2.3-01b"));
  }

  // Boundary: a long prerelease identifier just under SEMVER_MAX_INPUT_LENGTH.
  // This validates that the is_leading_zero lookahead loop terminates correctly.
  SUBCASE("long purely-numeric identifier with leading zero is rejected") {
    // Build "1.0.0-0" + 100 digits: purely numeric, leading zero → invalid
    std::string s = "1.0.0-0";
    s.append(100, '1');
    CHECK_FALSE(valid(s));
  }

  SUBCASE("long alphanumeric identifier with leading '0' is valid") {
    // Build "1.0.0-0" + "abc": alphanumeric, leading zero rule doesn't apply
    std::string s = "1.0.0-0";
    s.append(100, 'a');
    CHECK(valid(s));
  }
}

TEST_CASE("inc() direct construction correctness") {
  SUBCASE("premajor with explicit pre tag") {
    const auto v = inc(from_string("1.2.3"), version_change::premajor, "beta");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "2.0.0-beta");
  }

  SUBCASE("preminor with explicit pre tag") {
    const auto v = inc(from_string("1.2.3"), version_change::preminor, "rc.1");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.3.0-rc.1");
  }

  SUBCASE("prepatch with explicit pre tag") {
    const auto v = inc(from_string("1.2.3"), version_change::prepatch, "alpha");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.4-alpha");
  }

  SUBCASE("prerelease with explicit pre tag replaces existing") {
    const auto v = inc(from_string("1.0.0-alpha.1"), version_change::prerelease, "beta");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.0.0-beta");
  }

  SUBCASE("inc with invalid user-provided pre tag returns nullopt") {
    // Leading zero in numeric identifier → validate_prerelease_tag fails → nullopt
    CHECK_FALSE(inc(from_string("1.0.0"), version_change::premajor, "01").has_value());
    // Empty dot-separated identifier
    CHECK_FALSE(inc(from_string("1.0.0"), version_change::premajor, "alpha.").has_value());
    // Invalid character
    CHECK_FALSE(inc(from_string("1.0.0"), version_change::premajor, "alpha!").has_value());
  }

  SUBCASE("inc prerelease increments last numeric identifier") {
    CHECK(inc(from_string("1.0.0-alpha.1"), version_change::prerelease)->to_string()
          == "1.0.0-alpha.2");
    CHECK(inc(from_string("1.0.0-9"), version_change::prerelease)->to_string()
          == "1.0.0-10");
    CHECK(inc(from_string("1.0.0-alpha"), version_change::prerelease)->to_string()
          == "1.0.0-alpha.0");
  }

  SUBCASE("inc default pre is '0'") {
    CHECK(inc(from_string("1.0.0"), version_change::premajor)->to_string()   == "2.0.0-0");
    CHECK(inc(from_string("1.0.0"), version_change::preminor)->to_string()   == "1.1.0-0");
    CHECK(inc(from_string("1.0.0"), version_change::prepatch)->to_string()   == "1.0.1-0");
    CHECK(inc(from_string("1.0.0"), version_change::prerelease)->to_string() == "1.0.1-0");
  }
}

TEST_CASE("operator<< writes correct output") {
  SUBCASE("release version") {
    std::ostringstream oss;
    oss << version<>{1, 2, 3};
    CHECK(oss.str() == "1.2.3");
  }

  SUBCASE("prerelease version") {
    std::ostringstream oss;
    oss << *try_parse("1.0.0-alpha.1");
    CHECK(oss.str() == "1.0.0-alpha.1");
  }

  SUBCASE("with build metadata") {
    std::ostringstream oss;
    oss << *try_parse("1.0.0+build.42");
    CHECK(oss.str() == "1.0.0+build.42");
  }

  SUBCASE("full version") {
    std::ostringstream oss;
    oss << *try_parse("1.2.3-rc.1+build.999");
    CHECK(oss.str() == "1.2.3-rc.1+build.999");
  }
}

TEST_CASE("range_set contains agrees with string satisfies") {
  range_set<> rs;
  REQUIRE(parse("^1.2.0", rs));

  const auto v_in   = *try_parse("1.5.0");
  const auto v_out  = *try_parse("2.0.0");
  const auto v_pre  = *try_parse("1.5.0-alpha");

  SUBCASE("matches version in range") {
    CHECK(rs.contains(v_in));
  }

  SUBCASE("rejects version out of range") {
    CHECK_FALSE(rs.contains(v_out));
  }

  SUBCASE("exclude policy blocks prereleases by default") {
    CHECK_FALSE(rs.contains(v_pre));
  }

  SUBCASE("include policy allows prerelease when range bounds are satisfied") {
    // The include policy disables the "explicit comparator with same M.m.p" filter
    // is bypassed entirely. 1.5.0-alpha satisfies ^1.2.0 because
    //   1.5.0-alpha >= 1.2.0   (true)
    //   1.5.0-alpha < 2.0.0-0  (true, since 1.5 < 2.0)
    CHECK(rs.contains(v_pre, prerelease_policy::include));

    // But a prerelease above the upper bound is still excluded.
    const auto v_above = *try_parse("2.0.0-alpha");
    // 2.0.0-alpha < 2.0.0-0? No: 2.0.0-alpha > 2.0.0-0 (alpha > 0 alphanumeric vs numeric)
    CHECK_FALSE(rs.contains(v_above, prerelease_policy::include));
  }

  SUBCASE("string satisfies and pre-parsed contains agree") {
    for (const auto& s : {"1.2.0", "1.5.0", "2.0.0", "1.2.0-0"}) {
      const auto v = try_parse(s);
      REQUIRE(v.has_value());
      CHECK(rs.contains(*v) == satisfies(*v, "^1.2.0"));
    }
  }
}

TEST_CASE("library version") {
  CHECK(library_version.major() == SEMVER_VERSION_MAJOR);
  CHECK(library_version.minor() == SEMVER_VERSION_MINOR);
  CHECK(library_version.patch() == SEMVER_VERSION_PATCH);
}

TEST_CASE("validate_prerelease_tag contract via parse") {
  SUBCASE("empty dot-separated identifiers are invalid") {
    CHECK_FALSE(valid("1.0.0-.1"));       // leading dot
    CHECK_FALSE(valid("1.0.0-1."));       // trailing dot
    CHECK_FALSE(valid("1.0.0-1..2"));     // double dot
  }

  SUBCASE("invalid characters in prerelease are rejected") {
    CHECK_FALSE(valid("1.0.0-alpha!"));
    CHECK_FALSE(valid("1.0.0-alpha@beta"));
    CHECK_FALSE(valid("1.0.0-1.2.3.#"));
  }

  SUBCASE("hyphens inside identifier are valid (spec \u00a79)") {
    CHECK(valid("1.0.0-alpha-beta"));
    CHECK(valid("1.0.0-a-b.c-d.1"));
    CHECK(valid("1.0.0----RC-SNAPSHOT.12.9.1--.12"));
  }

  SUBCASE("purely alphanumeric with digits only and no leading zero is valid") {
    CHECK(valid("1.0.0-1"));
    CHECK(valid("1.0.0-0"));   // single '0' is valid
    CHECK(valid("1.0.0-10"));
    CHECK(valid("1.0.0-999"));
  }
}

TEST_CASE("operator== excludes build metadata (spec §10)") {
  const auto a = *try_parse("1.2.3+build.1");
  const auto b = *try_parse("1.2.3+build.2");
  CHECK(a == b);            // build metadata excluded
  CHECK(!(a != b));
  CHECK(compare(a, b) == 0);
  CHECK(compare_with_build(a, b) != 0);  // compare_with_build includes it
}

TEST_CASE("operator== includes prerelease") {
  const auto a = *try_parse("1.2.3-alpha");
  const auto b = *try_parse("1.2.3");
  CHECK(a != b);
  CHECK(a < b);
  CHECK(compare(a, b) == -1);
}

TEST_CASE("swap(version&, version&) correctness") {
  SUBCASE("exchanges all fields including prerelease and build metadata") {
    auto a = *try_parse("1.2.3-alpha.1+build.1");
    auto b = *try_parse("4.5.6-beta.2+build.2");
    using std::swap;
    swap(a, b);
    CHECK(a.major() == 4);
    CHECK(a.minor() == 5);
    CHECK(a.patch() == 6);
    CHECK(a.prerelease_tag() == "beta.2");
    CHECK(a.build_metadata() == "build.2");
    CHECK(b.major() == 1);
    CHECK(b.minor() == 2);
    CHECK(b.patch() == 3);
    CHECK(b.prerelease_tag() == "alpha.1");
    CHECK(b.build_metadata() == "build.1");
  }

  SUBCASE("swap release versions") {
    version<> a{1, 0, 0};
    version<> b{2, 0, 0};
    using std::swap;
    swap(a, b);
    CHECK(a.major() == 2);
    CHECK(b.major() == 1);
    CHECK(a.prerelease_tag().empty());
    CHECK(b.prerelease_tag().empty());
  }

  SUBCASE("swap is its own inverse (double-swap = identity)") {
    auto a = *try_parse("1.2.3-alpha");
    const auto a_orig = a;
    auto b = *try_parse("9.9.9");
    const auto b_orig = b;
    using std::swap;
    swap(a, b);
    swap(a, b);
    CHECK(a == a_orig);
    CHECK(b == b_orig);
  }

#if SEMVER_HAS_CONSTEXPR
  // Under C++20 with constexpr std::string, swap must be constexpr.
  static_assert([]() constexpr {
    version<> a{1, 0, 0};
    version<> b{2, 0, 0};
    using std::swap;
    swap(a, b);
    return a.major() == 2 && b.major() == 1;
  }(), "swap must be constexpr when SEMVER_HAS_CONSTEXPR is set");
#endif
}

TEST_CASE("to_chars covers all tag paths") {
  // Helper: parse s, serialize via to_chars, compare with original.
  const auto round_trip = [](std::string_view s) -> bool {
    const auto v = try_parse(std::string{s});
    if (!v)
      return false;

    char buf[128] = {};
    const auto r = to_chars(buf, buf + sizeof(buf), *v);
    return r && std::string_view{buf, static_cast<std::size_t>(r.ptr - buf)} == s;
  };

  SUBCASE("prerelease only") {
    CHECK(round_trip("1.2.3-alpha"));
    CHECK(round_trip("1.2.3-alpha.1"));
    CHECK(round_trip("1.2.3-0.3.7"));
    CHECK(round_trip("1.0.0-rc.1"));
    CHECK(round_trip("1.0.0-x.7.z.92"));
  }

  SUBCASE("build metadata only") {
    CHECK(round_trip("1.2.3+build.42"));
    CHECK(round_trip("1.0.0+20130313144700"));
    CHECK(round_trip("1.0.0+exp.sha.5114f85"));
  }

  SUBCASE("both prerelease and build metadata") {
    CHECK(round_trip("1.2.3-beta.1+sha.1234"));
    CHECK(round_trip("1.0.0-alpha.1+build.99"));
    CHECK(round_trip("1.2.3----RC-SNAPSHOT.12.9.1--.12+788"));
  }

  SUBCASE("to_chars and to_string agree") {
    const auto v = *try_parse("2.0.0-rc.3+exp.sha.abc");
    char buf[128] = {};
    const auto r = to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    CHECK(std::string_view{buf, static_cast<std::size_t>(r.ptr - buf)} == v.to_string());
  }

#if SEMVER_HAS_CONSTEXPR
  // C++20 with constexpr std::string must evaluate prerelease serialization at compile time.
  static_assert([]() constexpr {
    const version<> v{1, 2, 3, "rc.1"};
    std::array<char, 32> buf = {};
    const auto r = to_chars(buf.data(), buf.data() + buf.size(), v);
    return r.ec == std::errc{} && buf[0] == '1' && buf[1] == '.' && buf[2] == '2' && buf[3] == '.' && buf[4] == '3' && buf[5] == '-' && buf[6] == 'r' && buf[7] == 'c';
  }(), "to_chars with prerelease must be constexpr when SEMVER_HAS_CONSTEXPR is set");
#endif
}

TEST_CASE("operator<< routes correctly to stream types") {
  SUBCASE("std::ostringstream (ios_base-derived): release version") {
    std::ostringstream oss;
    oss << version<>{2, 1, 0};
    CHECK(oss.str() == "2.1.0");
  }

  SUBCASE("std::ostream base reference") {
    std::ostringstream oss;
    std::ostream& base = oss;
    base << *try_parse("3.0.0-alpha+meta");
    CHECK(oss.str() == "3.0.0-alpha+meta");
  }

  SUBCASE("chained << calls") {
    std::ostringstream oss;
    oss << *try_parse("1.0.0") << " vs " << *try_parse("2.0.0");
    CHECK(oss.str() == "1.0.0 vs 2.0.0");
  }
}

TEST_CASE("SEMVER_MAX_INPUT_LENGTH boundary") {
  static_assert(semver::max_input_length == SEMVER_MAX_INPUT_LENGTH,
      "max_input_length constant must match the macro");

  SUBCASE("input exactly at limit (valid prerelease): accepted") {
    // "1.0.0-" + N alphanumeric chars to reach SEMVER_MAX_INPUT_LENGTH total
    std::string s = "1.0.0-";
    s.append(SEMVER_MAX_INPUT_LENGTH - s.size(), 'a');
    REQUIRE(s.size() == SEMVER_MAX_INPUT_LENGTH);
    CHECK(valid(s));
  }

  SUBCASE("input one byte over limit: rejected with value_too_large") {
    std::string s = "1.0.0-";
    s.append(SEMVER_MAX_INPUT_LENGTH - s.size() + 1, 'a');
    REQUIRE(s.size() == SEMVER_MAX_INPUT_LENGTH + 1);
    version<> v;
    const auto r = parse(s, v);
    CHECK(!r);
    CHECK(r.ec == std::errc::value_too_large);
  }
}
