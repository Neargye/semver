#include <semver.hpp>
#include <doctest.h>
#include <ostream>
#include <string>
#include "test_utils.hpp"

using namespace semver;

TEST_CASE("validation") {
#ifdef SEMVER_CONSTEXPR_SUPPORT
  SUBCASE("constexpr valid") {
    constexpr std::string_view v1 = "0.0.1";
    static_assert(valid(v1));

    constexpr std::string_view v2 = "1.2.3-rc.4";
    static_assert(valid(v2));

    constexpr std::string_view v3 = "1.1.2-prerelease+meta";
    static_assert(valid(v3));
  }

  SUBCASE("constexpr invalid") {
    constexpr std::string_view v1 = "";
    static_assert(!valid(v1));

    constexpr std::string_view v2 = "1.01.*";
    static_assert(!valid(v2));

    constexpr std::string_view v3 = "1.1.2-prerelease_meta";
    static_assert(!valid(v3));
  }
#endif

  SUBCASE("runtime valid") {
    for (auto version: valid_versions) {
      REQUIRE(valid(version));
    }
  }

  SUBCASE("runtime invalid") {
    for (auto version: invalid_versions) {
      REQUIRE_FALSE(valid(version));
    }
  }
}

TEST_CASE("integral type constraint") {
  SUBCASE("various integral widths parse correctly") {
    semver::version<uint8_t>  v8;
    semver::version<uint16_t> v16;
    semver::version<uint32_t> v32;
    semver::version<uint64_t> v64;
    semver::version<int64_t>  vi64;
    REQUIRE(semver::parse("1.2.3", v8));
    REQUIRE(semver::parse("1.2.3", v16));
    REQUIRE(semver::parse("1.2.3", v32));
    REQUIRE(semver::parse("1.2.3", v64));
    REQUIRE(semver::parse("1.2.3", vi64));
    REQUIRE(v8.major() == 1);
    REQUIRE(v64.patch() == 3);
  }
}

TEST_CASE("valid with type-parameterized integer range") {
  SUBCASE("int8_t: rejects values outside its range") {
    REQUIRE(semver::valid<int8_t>("127.0.0"));
    REQUIRE_FALSE(semver::valid<int8_t>("128.0.0"));
    REQUIRE_FALSE(semver::valid<int8_t>("200.0.0"));
  }

  SUBCASE("uint8_t: rejects values > 255") {
    REQUIRE(semver::valid<uint8_t>("255.0.0"));
    REQUIRE_FALSE(semver::valid<uint8_t>("256.0.0"));
  }

  SUBCASE("uint64_t: accepts UINT64_MAX, rejects UINT64_MAX + 1") {
    REQUIRE(semver::valid<uint64_t>("18446744073709551615.0.0"));
    REQUIRE_FALSE(semver::valid<uint64_t>("18446744073709551616.0.0"));
  }
}

TEST_CASE("SEMVER_MAX_INPUT_LENGTH") {
  SUBCASE("input at limit is accepted") {
    // Build a valid version string padded with build metadata to reach the limit
    std::string input = "1.0.0+";
    input.append(SEMVER_MAX_INPUT_LENGTH - input.size(), 'a');
    REQUIRE(input.size() == SEMVER_MAX_INPUT_LENGTH);
    semver::version<> v;
    REQUIRE(semver::parse(input, v));
    REQUIRE(v.major() == 1);
  }

  SUBCASE("input exceeding limit is rejected") {
    std::string input = "1.0.0+";
    input.append(SEMVER_MAX_INPUT_LENGTH - input.size() + 1, 'b');
    REQUIRE(input.size() == SEMVER_MAX_INPUT_LENGTH + 1);
    semver::version<> v;
    auto result = semver::parse(input, v);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::value_too_large);
  }

  SUBCASE("range_set parse also respects the limit") {
    std::string input = ">=1.0.0+";
    input.append(SEMVER_MAX_INPUT_LENGTH - input.size() + 1, 'c');
    semver::range_set<> rs;
    auto result = semver::parse(input, rs);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::value_too_large);
  }
}

TEST_CASE("from_chars_result pointer semantics") {
  SUBCASE("successful parse: ptr points past last consumed char") {
    const char input[] = "1.2.3";
    semver::version<> v;
    auto result = semver::parse(std::string_view{input, 5}, v);
    REQUIRE(result);
    // ptr points one past the last consumed character (end of input)
    REQUIRE(result.ptr == input + 5);
  }

  SUBCASE("partial input: ptr shows where parsing ended") {
    const char input[] = "1.2.3-alpha";
    semver::version<> v;
    // Parse the full string successfully
    auto result = semver::parse(std::string_view{input, 11}, v);
    REQUIRE(result);
    REQUIRE(result.ptr == input + 11);
  }

  SUBCASE("failed parse: ptr points at the offending character") {
    semver::version<> v;
    const char input[] = "1.2.x";
    auto result = semver::parse(std::string_view{input, 5}, v);
    REQUIRE_FALSE(result);
    REQUIRE(*result.ptr == 'x');
  }

  SUBCASE("bool conversion: true on success, false on failure") {
    semver::version<> v;
    REQUIRE(static_cast<bool>(semver::parse("1.0.0", v)));
    REQUIRE_FALSE(static_cast<bool>(semver::parse("garbage", v)));
  }
}

TEST_CASE("default constructor produces 0.1.0") {
  semver::version<> v;
  REQUIRE(v.major() == 0);
  REQUIRE(v.minor() == 1);
  REQUIRE(v.patch() == 0);
  REQUIRE(v.prerelease_tag().empty());
  REQUIRE(v.build_metadata().empty());
  REQUIRE(v.to_string() == "0.1.0");
}

TEST_CASE("re-parsing into same object resets state") {
  semver::version<> v;
  REQUIRE(semver::parse("1.2.3-alpha+build", v));
  REQUIRE(v.prerelease_tag() == "alpha");
  REQUIRE(v.build_metadata() == "build");

  // Parse a simple version — prerelease and build must be cleared
  REQUIRE(semver::parse("4.5.6", v));
  REQUIRE(v.major() == 4);
  REQUIRE(v.minor() == 5);
  REQUIRE(v.patch() == 6);
  REQUIRE(v.prerelease_tag().empty());
  REQUIRE(v.build_metadata().empty());
}

#ifdef SEMVER_CONSTEXPR_SUPPORT
TEST_CASE("constexpr parse and accessors") {
  // Full compile-time parse → accessor chain
  static_assert([] {
    semver::version<> v;
    (void)semver::parse("10.20.30", v);
    return v.major() == 10 && v.minor() == 20 && v.patch() == 30;
  }());

  // Prerelease tag preserved at compile time
  static_assert([] {
    semver::version<> v;
    (void)semver::parse("1.0.0-alpha.beta.1", v);
    return v.prerelease_tag() == "alpha.beta.1";
  }());

  // Build metadata preserved at compile time
  static_assert([] {
    semver::version<> v;
    (void)semver::parse("1.0.0+sha.42089", v);
    return v.build_metadata() == "sha.42089";
  }());
}

TEST_CASE("constexpr comparison operators") {
  // Semver spec §11 precedence: 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta
  //   < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0
  static_assert([] {
    semver::version<> a, b;
    (void)semver::parse("1.0.0-alpha", a);
    (void)semver::parse("1.0.0-alpha.1", b);
    return a < b;
  }());

  static_assert([] {
    semver::version<> a, b;
    (void)semver::parse("1.0.0-alpha.1", a);
    (void)semver::parse("1.0.0-alpha.beta", b);
    return a < b;
  }());

  static_assert([] {
    semver::version<> a, b;
    (void)semver::parse("1.0.0-beta.2", a);
    (void)semver::parse("1.0.0-beta.11", b);
    return a < b; // numeric: 2 < 11 (not lexicographic)
  }());

  static_assert([] {
    semver::version<> a, b;
    (void)semver::parse("1.0.0-rc.1", a);
    (void)semver::parse("1.0.0", b);
    return a < b; // prerelease < release
  }());

  // Equality: build metadata ignored
  static_assert([] {
    semver::version<> a, b;
    (void)semver::parse("1.0.0+build.1", a);
    (void)semver::parse("1.0.0+build.999", b);
    return a == b;
  }());

  // Major/minor/patch ordering
  static_assert([] {
    semver::version<> a, b;
    (void)semver::parse("2.0.0", a);
    (void)semver::parse("1.999.999", b);
    return a > b;
  }());

  static_assert([] {
    semver::version<> a, b;
    (void)semver::parse("1.1.0", a);
    (void)semver::parse("1.0.999", b);
    return a > b;
  }());
}

TEST_CASE("constexpr to_string round-trip") {
  static_assert([] {
    semver::version<> v;
    (void)semver::parse("1.2.3-alpha+build", v);
    return v.to_string() == "1.2.3-alpha+build";
  }());

  static_assert([] {
    semver::version<> v;
    (void)semver::parse("0.0.0", v);
    return v.to_string() == "0.0.0";
  }());

  static_assert([] {
    semver::version<> v;
    (void)semver::parse("999999999.999999999.999999999", v);
    return v.to_string() == "999999999.999999999.999999999";
  }());
}

TEST_CASE("constexpr valid/invalid detection") {
  // Verify valid() itself works at compile time with non-trivial inputs
  static_assert(semver::valid("1.0.0-alpha-a.b-c-somethinglong+build.1-aef.1-its-okay"));
  static_assert(semver::valid("1.2.3----RC-SNAPSHOT.12.9.1--.12+788"));
  static_assert(!semver::valid("1.2.3-0123")); // leading zero in numeric prerelease
  static_assert(!semver::valid("01.1.1"));     // leading zero in major
  static_assert(!semver::valid("9.8.7+meta+meta")); // double +
}

TEST_CASE("constexpr range_set") {
  static_assert([] {
    semver::range_set<> rs;
    (void)semver::parse(">=1.0.0 <2.0.0", rs);
    semver::version<> v;
    (void)semver::parse("1.5.0", v);
    return rs.contains(v);
  }());

  static_assert([] {
    semver::range_set<> rs;
    (void)semver::parse(">=1.0.0 <2.0.0", rs);
    semver::version<> v;
    (void)semver::parse("2.0.0", v);
    return !rs.contains(v);
  }());

  // Union (||) at compile time
  static_assert([] {
    semver::range_set<> rs;
    (void)semver::parse(">=1.0.0 <2.0.0 || >=3.0.0 <4.0.0", rs);
    semver::version<> v1, v2, v3;
    (void)semver::parse("1.5.0", v1);
    (void)semver::parse("3.5.0", v2);
    (void)semver::parse("2.5.0", v3);
    return rs.contains(v1) && rs.contains(v2) && !rs.contains(v3);
  }());

  // Prerelease in range at compile time
  static_assert([] {
    semver::range_set<> rs;
    (void)semver::parse(">1.0.0-alpha.1", rs);
    semver::version<> v;
    (void)semver::parse("1.0.0-alpha.2", v);
    return rs.contains(v);
  }());
}

TEST_CASE("constexpr version constructor") {
  static_assert([] {
    semver::version<> v{1, 2, 3};
    return v.major() == 1 && v.minor() == 2 && v.patch() == 3
        && v.prerelease_tag().empty() && v.build_metadata().empty();
  }());

  static_assert([] {
    semver::version<> def;
    return def.major() == 0 && def.minor() == 1 && def.patch() == 0;
  }());
}
#endif

#if __cpp_consteval >= 201811L && defined(SEMVER_FULL_CONSTEXPR) && SEMVER_FULL_CONSTEXPR && !defined(_MSC_VER)
TEST_CASE("consteval _semver literal") {
  using namespace semver::literals;

  static_assert("1.2.3"_semver.major() == 1);
  static_assert("1.2.3"_semver.minor() == 2);
  static_assert("1.2.3"_semver.patch() == 3);
  static_assert("0.1.0"_semver == semver::version<>{0, 1, 0});
  static_assert("1.0.0-alpha"_semver < "1.0.0-beta"_semver);
  static_assert("1.0.0-alpha.1"_semver < "1.0.0-alpha.2"_semver);

  // Complex prerelease preserved
  static_assert("1.2.3----RC-SNAPSHOT.12.9.1--.12"_semver.patch() == 3);

  // Literal produces same result as runtime parse
  SUBCASE("literal matches runtime parse") {
    // `consteval version<>` with non-empty strings cannot appear in runtime
    // REQUIRE or be stored as `constexpr auto` — std::string SSO has a
    // self-referential pointer (_M_p → _M_local_buf) that is not a valid
    // constant expression outside a transient evaluation context.
    static_assert([] {
      const auto v = "1.0.0-alpha+build"_semver;
      return v.prerelease_tag() == "alpha" && v.build_metadata() == "build";
    }());
  }
}
#endif
