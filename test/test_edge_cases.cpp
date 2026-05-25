// Edge-case and regression tests covering issues found in the production readiness review.
// P0-1: is_leading_zero loop bound
// P0-2: validate_prerelease_tag + constructor assert contract
// P0-3: uint_write_backward rename (verified via correct output)
// P1-2: hyphen range with multiple spaces after '-'
// P1-3: inc() direct construction (no string round-trip)
// P1-4: parse_partial skip_tag (no allocation for discarded build metadata)
// P1-5: operator<< (no SFINAE regressions)
// P2-2: satisfies() range_set overload
// P2-3: constexpr library version constants

#include <semver.hpp>
#include <doctest.h>
#include <string>
#include <sstream>
#include <array>

using namespace semver;

// ---------------------------------------------------------------------------
// P0-2: validate_prerelease_tag — indirectly via parse and compare
// The 4-arg constructor now asserts that the tag is valid.
// We test that correctly-formatted tags compare as the spec requires.
// ---------------------------------------------------------------------------
TEST_CASE("prerelease tag comparison correctness (P0-2)") {
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

// ---------------------------------------------------------------------------
// P0-3: uint_write_backward (renamed from detail::to_chars) — verify output
// ---------------------------------------------------------------------------
TEST_CASE("to_chars serialization correctness (P0-3 regression)") {
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

// ---------------------------------------------------------------------------
// P0-1: is_leading_zero boundary — parse prerelease with exactly the
// right number of tokens (validates the loop exits before going OOB).
// We test both valid single-char "0" and invalid "00".
// ---------------------------------------------------------------------------
TEST_CASE("leading zero detection boundary (P0-1)") {
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

// ---------------------------------------------------------------------------
// P1-2: Hyphen range with multiple spaces around '-'
// ---------------------------------------------------------------------------
TEST_CASE("hyphen range with multiple spaces (P1-2)") {
  SUBCASE("two spaces after hyphen") {
    range_set<> rs;
    REQUIRE(parse("1.2.3 -  2.3.4", rs));
    CHECK(rs.contains(*try_parse("1.2.3"), include_prerelease));
    CHECK(rs.contains(*try_parse("2.0.0"), include_prerelease));
    CHECK(rs.contains(*try_parse("2.3.4"), include_prerelease));
    CHECK_FALSE(rs.contains(*try_parse("1.2.2"), include_prerelease));
    CHECK_FALSE(rs.contains(*try_parse("2.3.5"), include_prerelease));
  }

  SUBCASE("three spaces after hyphen") {
    range_set<> rs;
    REQUIRE(parse("1.0.0 -   2.0.0", rs));
    CHECK(rs.contains(*try_parse("1.5.0"), include_prerelease));
    CHECK_FALSE(rs.contains(*try_parse("2.0.1"), include_prerelease));
  }

  SUBCASE("single space still works (regression)") {
    range_set<> rs;
    REQUIRE(parse("1.0.0 - 2.0.0", rs));
    CHECK(rs.contains(*try_parse("1.5.0"), include_prerelease));
  }

  SUBCASE("no space after hyphen is NOT a hyphen range") {
    // "1.0.0 -2.0.0" is ambiguous/invalid — not treated as hyphen range
    range_set<> rs;
    const auto result = parse("1.0.0 -2.0.0", rs);
    // Either fails to parse, or parses differently — just verify it doesn't
    // silently match as a hyphen range.
    if (result) {
      // If it parsed, it should NOT match 1.5.0 as a hyphen range would
      CHECK_FALSE(rs.contains(*try_parse("1.5.0"), include_prerelease));
    }
  }

  SUBCASE("hyphen range with wildcard RHS and multiple spaces") {
    range_set<> rs;
    REQUIRE(parse("1.2.3 -  2.x", rs));
    CHECK(rs.contains(*try_parse("1.2.3"), include_prerelease));
    CHECK(rs.contains(*try_parse("2.5.0"), include_prerelease));
    CHECK_FALSE(rs.contains(*try_parse("3.0.0"), include_prerelease));
  }
}

// ---------------------------------------------------------------------------
// P1-3: inc() direct construction — no string round-trip
// ---------------------------------------------------------------------------
TEST_CASE("inc() direct construction correctness (P1-3)") {
  SUBCASE("premajor with explicit pre tag") {
    const auto v = inc(from_string("1.2.3"), version_diff::premajor, "beta");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "2.0.0-beta");
  }

  SUBCASE("preminor with explicit pre tag") {
    const auto v = inc(from_string("1.2.3"), version_diff::preminor, "rc.1");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.3.0-rc.1");
  }

  SUBCASE("prepatch with explicit pre tag") {
    const auto v = inc(from_string("1.2.3"), version_diff::prepatch, "alpha");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.2.4-alpha");
  }

  SUBCASE("prerelease with explicit pre tag replaces existing") {
    const auto v = inc(from_string("1.0.0-alpha.1"), version_diff::prerelease, "beta");
    REQUIRE(v.has_value());
    CHECK(v->to_string() == "1.0.0-beta");
  }

  SUBCASE("inc with invalid user-provided pre tag returns nullopt") {
    // Leading zero in numeric identifier → validate_prerelease_tag fails → nullopt
    CHECK_FALSE(inc(from_string("1.0.0"), version_diff::premajor, "01").has_value());
    // Empty dot-separated identifier
    CHECK_FALSE(inc(from_string("1.0.0"), version_diff::premajor, "alpha.").has_value());
    // Invalid character
    CHECK_FALSE(inc(from_string("1.0.0"), version_diff::premajor, "alpha!").has_value());
  }

  SUBCASE("inc prerelease increments last numeric identifier") {
    CHECK(inc(from_string("1.0.0-alpha.1"), version_diff::prerelease)->to_string()
          == "1.0.0-alpha.2");
    CHECK(inc(from_string("1.0.0-9"), version_diff::prerelease)->to_string()
          == "1.0.0-10");
    CHECK(inc(from_string("1.0.0-alpha"), version_diff::prerelease)->to_string()
          == "1.0.0-alpha.0");
  }

  SUBCASE("inc default pre is '0'") {
    CHECK(inc(from_string("1.0.0"), version_diff::premajor)->to_string()   == "2.0.0-0");
    CHECK(inc(from_string("1.0.0"), version_diff::preminor)->to_string()   == "1.1.0-0");
    CHECK(inc(from_string("1.0.0"), version_diff::prepatch)->to_string()   == "1.0.1-0");
    CHECK(inc(from_string("1.0.0"), version_diff::prerelease)->to_string() == "1.0.1-0");
  }
}

// ---------------------------------------------------------------------------
// P1-4: parse_partial skips build metadata without allocating
// (verified via successful parse of range strings with build metadata)
// ---------------------------------------------------------------------------
TEST_CASE("range parse with build metadata in range boundary (P1-4)") {
  SUBCASE("caret range: build metadata on lower bound is discarded") {
    // ^1.2.3+build parses and the '+build' is stripped for range matching
    range_set<> rs;
    REQUIRE(parse("^1.2.3+build.42", rs));
    CHECK(rs.contains(*try_parse("1.5.0"), include_prerelease));
    CHECK_FALSE(rs.contains(*try_parse("2.0.0"), include_prerelease));
  }

  SUBCASE("tilde range: build metadata ignored") {
    range_set<> rs;
    REQUIRE(parse("~1.2.3+xyz", rs));
    CHECK(rs.contains(*try_parse("1.2.9"), include_prerelease));
    CHECK_FALSE(rs.contains(*try_parse("1.3.0"), include_prerelease));
  }

  SUBCASE("hyphen range: build metadata on upper bound is discarded") {
    range_set<> rs;
    REQUIRE(parse("1.2.3 - 2.0.0+meta", rs));
    CHECK(rs.contains(*try_parse("1.5.0"), include_prerelease));
    CHECK_FALSE(rs.contains(*try_parse("2.0.1"), include_prerelease));
  }
}

// ---------------------------------------------------------------------------
// P1-5: operator<< — no SFINAE regressions
// ---------------------------------------------------------------------------
TEST_CASE("operator<< writes correct output (P1-5)") {
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

// ---------------------------------------------------------------------------
// P2-2: satisfies() overload taking pre-parsed range_set
// ---------------------------------------------------------------------------
TEST_CASE("satisfies() range_set overload (P2-2)") {
  range_set<> rs;
  REQUIRE(parse("^1.2.0", rs));

  const auto v_in   = *try_parse("1.5.0");
  const auto v_out  = *try_parse("2.0.0");
  const auto v_pre  = *try_parse("1.5.0-alpha");

  SUBCASE("matches version in range") {
    CHECK(satisfies(v_in, rs));
  }

  SUBCASE("rejects version out of range") {
    CHECK_FALSE(satisfies(v_out, rs));
  }

  SUBCASE("exclude_prerelease default blocks prereleases") {
    CHECK_FALSE(satisfies(v_pre, rs));
  }

  SUBCASE("include_prerelease includes prerelease when range bounds are satisfied") {
    // With include_prerelease, the "explicit comparator with same M.m.p" filter
    // is bypassed entirely. 1.5.0-alpha satisfies ^1.2.0 because
    //   1.5.0-alpha >= 1.2.0   (true)
    //   1.5.0-alpha < 2.0.0-0  (true, since 1.5 < 2.0)
    CHECK(satisfies(v_pre, rs, include_prerelease));

    // But a prerelease above the upper bound is still excluded.
    const auto v_above = *try_parse("2.0.0-alpha");
    // 2.0.0-alpha < 2.0.0-0? No: 2.0.0-alpha > 2.0.0-0 (alpha > 0 alphanumeric vs numeric)
    CHECK_FALSE(satisfies(v_above, rs, include_prerelease));
  }

  SUBCASE("string overload and range_set overload agree") {
    for (const auto& s : {"1.2.0", "1.5.0", "2.0.0", "1.2.0-0"}) {
      const auto v = try_parse(s);
      REQUIRE(v.has_value());
      CHECK(satisfies(*v, rs) == satisfies(*v, "^1.2.0"));
    }
  }
}

// ---------------------------------------------------------------------------
// P2-3: constexpr library version integer constants
// ---------------------------------------------------------------------------
TEST_CASE("constexpr library version constants (P2-3)") {
  static_assert(library_version_major == SEMVER_VERSION_MAJOR, "major mismatch");
  static_assert(library_version_minor == SEMVER_VERSION_MINOR, "minor mismatch");
  static_assert(library_version_patch == SEMVER_VERSION_PATCH, "patch mismatch");

  CHECK(library_version_major == SEMVER_VERSION_MAJOR);
  CHECK(library_version_minor == SEMVER_VERSION_MINOR);
  CHECK(library_version_patch == SEMVER_VERSION_PATCH);

  // Must match the runtime library_version object.
  CHECK(library_version.major() == library_version_major);
  CHECK(library_version.minor() == library_version_minor);
  CHECK(library_version.patch() == library_version_patch);
}

// ---------------------------------------------------------------------------
// Additional edge cases: validate_prerelease_tag rules
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Regression: operator== / operator<=> (via <=> under C++20) correctness
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// P0-2 fix: swap(version&, version&) is now SEMVER_CONSTEXPR
// ---------------------------------------------------------------------------
TEST_CASE("swap(version&, version&) correctness (P0-2 fix)") {
  SUBCASE("exchanges all fields including prerelease and build metadata") {
    version<> a = *try_parse("1.2.3-alpha.1+build.1");
    version<> b = *try_parse("4.5.6-beta.2+build.2");
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
    version<> a = *try_parse("1.2.3-alpha");
    const version<> a_orig = a;
    version<> b = *try_parse("9.9.9");
    const version<> b_orig = b;
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

// ---------------------------------------------------------------------------
// P0-1 fix: to_chars uses range-for instead of std::copy — now truly
// constexpr in C++17/20 for versions with prerelease and/or build metadata.
// ---------------------------------------------------------------------------
TEST_CASE("to_chars: range-for covers all tag paths (P0-1 fix)") {
  // Helper: parse s, serialize via to_chars, compare with original.
  const auto round_trip = [](std::string_view s) -> bool {
    const auto v = try_parse(std::string{s});
    if (!v) return false;
    char buf[128] = {};
    const auto r = to_chars(buf, buf + sizeof(buf), *v);
    return r && std::string_view{buf, static_cast<std::size_t>(r.ptr - buf)} == s;
  };

  SUBCASE("prerelease only — first range-for loop") {
    CHECK(round_trip("1.2.3-alpha"));
    CHECK(round_trip("1.2.3-alpha.1"));
    CHECK(round_trip("1.2.3-0.3.7"));
    CHECK(round_trip("1.0.0-rc.1"));
    CHECK(round_trip("1.0.0-x.7.z.92"));
  }

  SUBCASE("build metadata only — second range-for loop") {
    CHECK(round_trip("1.2.3+build.42"));
    CHECK(round_trip("1.0.0+20130313144700"));
    CHECK(round_trip("1.0.0+exp.sha.5114f85"));
  }

  SUBCASE("both prerelease and build metadata — both loops") {
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
  // C++20 + constexpr std::string: to_chars must be evaluable at compile time
  // for versions with prerelease — validates the range-for replacement.
  static_assert([]() constexpr {
    const version<> v{1, 2, 3, "rc.1"};
    std::array<char, 32> buf = {};
    const auto r = to_chars(buf.data(), buf.data() + buf.size(), v);
    return r.ec == std::errc{}
        && buf[0] == '1' && buf[1] == '.' && buf[2] == '2' && buf[3] == '.'
        && buf[4] == '3' && buf[5] == '-' && buf[6] == 'r' && buf[7] == 'c';
  }(), "to_chars with prerelease must be constexpr when SEMVER_HAS_CONSTEXPR is set");
#endif
}

// ---------------------------------------------------------------------------
// P0-3 fix: operator<< has enable_if SFINAE guard in C++17 —
// constrains overload to types derived from std::ios_base.
// ---------------------------------------------------------------------------
TEST_CASE("operator<< routes correctly to stream types (P0-3 fix)") {
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

// ---------------------------------------------------------------------------
// P3-1: SEMVER_MAX_INPUT_LENGTH reduced to 512 — boundary behaviour.
// The existing test_validation.cpp tests use the macro dynamically so they
// automatically cover the new limit. These subcases add targeted checks.
// ---------------------------------------------------------------------------
TEST_CASE("SEMVER_MAX_INPUT_LENGTH=512 boundary (P3-1)") {
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
    auto r = parse(s, v);
    CHECK(!r);
    CHECK(r.ec == std::errc::value_too_large);
  }
}
