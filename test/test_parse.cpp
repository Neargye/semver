#include <semver.hpp>
#include <doctest.h>
#include <array>
#include <ostream>
#include <optional>
#include <system_error>

using namespace semver;

TEST_CASE("parse") {
  SUBCASE("simple") {
    struct version {
      int major, minor, patch;
    };

    constexpr std::array<std::pair<std::string_view, version>, 8> versions = {{
      {"0.0.0", {0,0,0}},
      {"0.0.1", {0,0,1}},
      {"0.1.0", {0,1,0}},
      {"0.1.1", {0,1,1}},
      {"1.0.0", {1,0,0}},
      {"1.0.1", {1,0,1}},
      {"1.1.0", {1,1,0}},
      {"1.1.1", {1,1,1}}
    }};

    for (const auto& [version, expected]: versions) {
      semver::version result;
      REQUIRE(parse(version, result));
      REQUIRE(result.major() == expected.major);
      REQUIRE(result.minor() == expected.minor);
      REQUIRE(result.patch() == expected.patch);
    }
  }

  SUBCASE("negative") {
    constexpr std::array<std::string_view, 3> versions = {{
      {"0.0.-1"},
      {"0.-1.0"},
      {"-1.0.0"}
    }};

    for (auto version: versions) {
      semver::version result;
      REQUIRE_FALSE(parse(version, result));
    }
  }

  SUBCASE("leading zero") {
    constexpr std::array<std::string_view, 3> versions = {{
      {"0.0.01"},
      {"0.01.0"},
      {"01.0.0"}
    }};

    for (auto version: versions) {
      semver::version result;
      REQUIRE_FALSE(parse(version, result));
    }
  }

  SUBCASE("incomplete") {
    constexpr std::array<std::string_view, 7> versions = {{
      "",
      "1.",
      "1.*",
      "1.0",
      "1.0.",
      "1.0.*",
      "*"
    }};

    for (auto version: versions) {
      semver::version result;
      REQUIRE_FALSE(parse(version, result));
    }
  }

  SUBCASE("overflow") {
    constexpr std::string_view v = "0.0.256";

    semver::version<std::uint8_t, std::uint8_t, std::uint8_t> result;
    REQUIRE_FALSE(parse(v, result));

    semver::version<std::uint16_t, std::uint16_t, std::uint16_t> result2;
    REQUIRE(parse(v, result2));
    REQUIRE(result2.major() == 0);
    REQUIRE(result2.minor() == 0);
    REQUIRE(result2.patch() == 256);

    constexpr std::string_view v2 = "0.4294967296.0";
    semver::version<std::uint32_t, std::uint32_t, std::uint32_t> result3;
    REQUIRE_FALSE(parse(v2, result3));

    semver::version<std::uint64_t, std::uint64_t, std::uint64_t> result4;
    REQUIRE(parse(v2, result4));
    REQUIRE(result4.major() == 0);
    REQUIRE(result4.minor() == 4294967296);
    REQUIRE(result4.patch() == 0);
  }

  SUBCASE("prerelease") {
    struct version {
      int major, minor, patch;
      std::string_view prerelease_tag;
    };

    constexpr std::array<std::pair<std::string_view, version>, 4> versions = {{
      {"0.0.1-alpha.128", {0, 0, 1, "alpha.128"}},
      {"1.2.3-alpha.beta.rc-45.42", {1, 2, 3, "alpha.beta.rc-45.42"}},
      {"0.0.1-alpha-beta", {0, 0, 1, "alpha-beta"}},
      {"1.0.1-alpha.5-114-ga2f3905", {1, 0, 1, "alpha.5-114-ga2f3905"}}
    }};

    for (const auto& [version, expected]: versions) {
      semver::version result;
      REQUIRE(parse(version, result));
      REQUIRE(result.major() == expected.major);
      REQUIRE(result.minor() == expected.minor);
      REQUIRE(result.patch() == expected.patch);
      REQUIRE(result.prerelease_tag() == expected.prerelease_tag);
    }
  }

  SUBCASE("build-metadata") {
    struct version {
      int major, minor, patch;
      std::string_view build_metadata;
    };

    constexpr std::array<std::pair<std::string_view, version>, 3> versions = {{
      {"0.0.1+123", {0, 0, 1, "123"}},
      {"1.2.3+sha.42089", {1, 2, 3, "sha.42089"}},
      {"0.0.1+001-meta-info", {0, 0, 1, "001-meta-info"}}
    }};

    for (const auto& [version, expected]: versions) {
      semver::version result;
      REQUIRE(parse(version, result));
      REQUIRE(result.major() == expected.major);
      REQUIRE(result.minor() == expected.minor);
      REQUIRE(result.patch() == expected.patch);
      REQUIRE(result.prerelease_tag().empty());
      REQUIRE(result.build_metadata() == expected.build_metadata);
    }
  }

  SUBCASE("prerelease + build-metadata") {
    struct version {
      int major, minor, patch;
      std::string_view prerelease_tag, build_metadata;
    };

    constexpr std::array<std::pair<std::string_view, version>, 3> versions = {{
      {"0.0.1-alpha.128+123", {0, 0, 1, "alpha.128", "123"}},
      {"1.2.3-alpha.beta.rc-45.42+sha.42089", {1, 2, 3, "alpha.beta.rc-45.42", "sha.42089"}},
      {"0.0.1-alpha-beta+001-meta-info", {0, 0, 1, "alpha-beta", "001-meta-info"}}
    }};

    for (const auto& [version, expected]: versions) {
      semver::version result;
      REQUIRE(parse(version, result));
      REQUIRE(result.major() == expected.major);
      REQUIRE(result.minor() == expected.minor);
      REQUIRE(result.patch() == expected.patch);
      REQUIRE(result.prerelease_tag() == expected.prerelease_tag);
      REQUIRE(result.build_metadata() == expected.build_metadata);
    }
  }

  SUBCASE("alphanumeric prerelease with embedded digits") {
    // Regression: identifiers like "PullRequest0434" (letter-then-digit) must
    // NOT be rejected as leading-zero numeric identifiers (spec §9).
    semver::version v;
    REQUIRE(parse("0.16.411-PullRequest0434.25+45cf6d2", v));
    REQUIRE(v.major() == 0);
    REQUIRE(v.minor() == 16);
    REQUIRE(v.patch() == 411);
    REQUIRE(v.prerelease_tag() == "PullRequest0434.25");
    REQUIRE(v.build_metadata() == "45cf6d2");
  }
}

TEST_CASE("construct") {
  SUBCASE("version(major, minor, patch) constructor") {
    const semver::version v{1u, 2u, 3u};
    REQUIRE(v.major() == 1);
    REQUIRE(v.minor() == 2);
    REQUIRE(v.patch() == 3);
    REQUIRE(v.prerelease_tag().empty());
    REQUIRE(v.build_metadata().empty());

    semver::version parsed;
    REQUIRE(semver::parse("1.2.3", parsed));
    REQUIRE(v == parsed);
  }

  SUBCASE("single-type template version<uint32_t>") {
    semver::version<uint32_t> v{1u, 0u, 0u};
    REQUIRE(v.major() == 1u);
    REQUIRE(v.minor() == 0u);
    REQUIRE(v.patch() == 0u);

    semver::version<uint32_t> parsed;
    REQUIRE(semver::parse("1.0.0", parsed));
    REQUIRE(v == parsed);
  }
}

TEST_CASE("parse number overflow") {
  // Regression: a 20+-digit component must NOT silently wrap around.
  // Before the fix, uint64_t overflow produced a valid-looking but wrong value.
  semver::version<> v;

  SUBCASE("20-digit major wraps uint64_t — must be rejected") {
    // 18446744073709551616 == 2^64, overflows uint64_t to 0
    auto result = semver::parse("18446744073709551616.0.0", v);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::result_out_of_range);
  }

  SUBCASE("all-nines 20-digit component") {
    auto result = semver::parse("99999999999999999999.0.0", v);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::result_out_of_range);
  }

  SUBCASE("uint64_t max is still accepted for uint64_t version") {
    // 18446744073709551615 == UINT64_MAX — valid for uint64_t components
    semver::version<uint64_t> big;
    REQUIRE(semver::parse("18446744073709551615.0.0", big));
    REQUIRE(big.major() == std::numeric_limits<uint64_t>::max());
  }
}

TEST_CASE("from_chars_result contract") {
  SUBCASE("ptr points past last consumed byte on success") {
    semver::version v;
    constexpr std::string_view sv = "1.2.3-alpha.1+build";
    const auto [ptr, ec] = semver::parse(sv, v);
    REQUIRE(ec == std::errc{});
    REQUIRE(ptr == sv.data() + sv.size());
  }

  SUBCASE("trailing garbage fails parse") {
    semver::version v;
    REQUIRE_FALSE(semver::parse("1.2.3garbage", v));
    REQUIRE_FALSE(semver::parse("1.2.3 ",        v)); // trailing space
    REQUIRE_FALSE(semver::parse("1.2.3.4",        v)); // extra component
  }

  SUBCASE("errc::invalid_argument for structurally malformed input") {
    semver::version v;
    auto result = semver::parse("not-a-version", v);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::invalid_argument);
  }

  SUBCASE("errc::result_out_of_range for typed component overflow") {
    semver::version<std::uint8_t> v;
    auto result = semver::parse("256.0.0", v); // uint8_t max is 255
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::result_out_of_range);
  }

  SUBCASE("ptr points to first invalid byte on malformed input") {
    semver::version v;
    constexpr std::string_view bad = "1.2.x";
    const auto [ptr, ec] = semver::parse(bad, v);
    REQUIRE(ec == std::errc::invalid_argument);
    REQUIRE(ptr == bad.data() + 4);
  }

  SUBCASE("failed parse resets output object") {
    semver::version v;
    REQUIRE(semver::parse("2.3.4-alpha+meta", v));
    REQUIRE_FALSE(semver::parse("broken", v));
    REQUIRE(v.major() == 0);
    REQUIRE(v.minor() == 1);
    REQUIRE(v.patch() == 0);
    REQUIRE(v.prerelease_tag().empty());
    REQUIRE(v.build_metadata().empty());
  }

  SUBCASE("lexer failure preserves the previous output object") {
    semver::version v;
    REQUIRE(semver::parse("2.3.4-alpha+meta", v));

    const auto result = semver::parse("1.2.3\t", v);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::invalid_argument);
    REQUIRE(v.major() == 2);
    REQUIRE(v.minor() == 3);
    REQUIRE(v.patch() == 4);
    REQUIRE(v.prerelease_tag() == "alpha");
    REQUIRE(v.build_metadata() == "meta");
  }

  SUBCASE("trailing garbage preserves parsed prefix like from_chars") {
    semver::version v;
    const auto result = semver::parse("2.3.4tail", v);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::invalid_argument);
    REQUIRE(v.major() == 2);
    REQUIRE(v.minor() == 3);
    REQUIRE(v.patch() == 4);
    REQUIRE(v.prerelease_tag().empty());
    REQUIRE(v.build_metadata().empty());
  }
}

TEST_CASE("parse malformed identifiers") {
  semver::version v;

  SUBCASE("reject empty prerelease identifiers") {
    REQUIRE_FALSE(semver::parse("1.0.0-.", v));
    REQUIRE_FALSE(semver::parse("1.0.0-alpha..1", v));
  }

  SUBCASE("reject empty build metadata identifiers") {
    REQUIRE_FALSE(semver::parse("1.0.0+.", v));
    REQUIRE_FALSE(semver::parse("1.0.0+meta..1", v));
  }

  SUBCASE("reject numeric prerelease leading zero but allow alphanumeric") {
    REQUIRE_FALSE(semver::parse("1.0.0-01", v));
    REQUIRE(semver::parse("1.0.0-01a", v));
  }
}

TEST_CASE("prerelease precedence \u2014 semver spec \u00a711") {
  // \u00a711.4: numeric-only identifiers compared as integers; alphanumeric compared
  // lexically; numeric always has lower precedence than alphanumeric;
  // more fields > fewer fields when the prefix is equal.

  SUBCASE("numeric identifiers compared as integers, not strings") {
    // \"9\" > \"10\" lexicographically, but 9 < 10 as integers
    semver::version v9, v10;
    REQUIRE(semver::parse("1.0.0-9",  v9));
    REQUIRE(semver::parse("1.0.0-10", v10));
    REQUIRE(v9 < v10);
  }

  SUBCASE("numeric identifier always less than alphanumeric (\u00a711.4.1)") {
    semver::version vnum, valpha;
    REQUIRE(semver::parse("1.0.0-1",     vnum));
    REQUIRE(semver::parse("1.0.0-alpha", valpha));
    REQUIRE(vnum < valpha);
  }

  SUBCASE("fewer fields < more fields when prefix equal (\u00a711.4.4)") {
    semver::version vfew, vmore;
    REQUIRE(semver::parse("1.0.0-alpha",   vfew));
    REQUIRE(semver::parse("1.0.0-alpha.1", vmore));
    REQUIRE(vfew < vmore);
  }

  SUBCASE("all-numeric prerelease fields valid and ordered numerically") {
    semver::version v0, v1, v10;
    REQUIRE(semver::parse("1.0.0-0",  v0));
    REQUIRE(semver::parse("1.0.0-1",  v1));
    REQUIRE(semver::parse("1.0.0-10", v10));
    REQUIRE(v0 < v1);
    REQUIRE(v1 < v10);
  }

  SUBCASE("alphanumeric identifiers compared lexically") {
    semver::version va, vb, vz;
    REQUIRE(semver::parse("1.0.0-alpha", va));
    REQUIRE(semver::parse("1.0.0-beta",  vb));
    REQUIRE(semver::parse("1.0.0-zeta",  vz));
    REQUIRE(va < vb);
    REQUIRE(vb < vz);
  }

  SUBCASE("spec example chain: alpha < alpha.1 < alpha.beta < \u2026 < 1.0.0") {
    // The ordering example from https://semver.org/#spec-item-11
    constexpr std::array<std::string_view, 8> chain = {{
      "1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta",
      "1.0.0-beta",  "1.0.0-beta.2",  "1.0.0-beta.11",
      "1.0.0-rc.1",  "1.0.0"
    }};
    for (std::size_t i = 1; i < chain.size(); ++i) {
      semver::version lo, hi;
      REQUIRE(semver::parse(chain[i - 1], lo));
      REQUIRE(semver::parse(chain[i],     hi));
      REQUIRE(lo < hi);
      REQUIRE(hi > lo);
    }
  }
}

TEST_CASE("from_chars — partial parse semantics") {
  SUBCASE("exact version string consumes entire input") {
    const char buf[] = "1.2.3";
    version<> v;
    const auto r = semver::from_chars(buf, buf + 5, v);
    REQUIRE(r);
    REQUIRE(v.major() == 1);
    REQUIRE(v.minor() == 2);
    REQUIRE(v.patch() == 3);
    REQUIRE(r.ptr == buf + 5);
  }

  SUBCASE("stops at space — ptr points to first trailing char") {
    const char buf[] = "1.2.3 extra";
    version<> v;
    const auto r = semver::from_chars(buf, buf + sizeof(buf) - 1, v);
    REQUIRE(r);
    REQUIRE(r.ptr == buf + 5);
    REQUIRE(*r.ptr == ' ');
    REQUIRE(v.major() == 1);
    REQUIRE(v.minor() == 2);
    REQUIRE(v.patch() == 3);
  }

  SUBCASE("stops at newline — ptr points to newline") {
    const char buf[] = "1.2.3\nmore";
    version<> v;
    const auto r = semver::from_chars(buf, buf + sizeof(buf) - 1, v);
    REQUIRE(r);
    REQUIRE(r.ptr == buf + 5);
    REQUIRE(*r.ptr == '\n');
  }

  SUBCASE("stops at comma") {
    const char buf[] = "2.0.0,other";
    version<> v;
    const auto r = semver::from_chars(buf, buf + sizeof(buf) - 1, v);
    REQUIRE(r);
    REQUIRE(*r.ptr == ',');
    REQUIRE(v.major() == 2);
    REQUIRE(v.minor() == 0);
    REQUIRE(v.patch() == 0);
  }

  SUBCASE("stops at @") {
    const char buf[] = "0.1.0@tag";
    version<> v;
    const auto r = semver::from_chars(buf, buf + sizeof(buf) - 1, v);
    REQUIRE(r);
    REQUIRE(*r.ptr == '@');
  }

  SUBCASE("stops at semicolon") {
    const char buf[] = "3.4.5;6.7.8";
    version<> v;
    const auto r = semver::from_chars(buf, buf + sizeof(buf) - 1, v);
    REQUIRE(r);
    REQUIRE(v.major() == 3);
    REQUIRE(v.minor() == 4);
    REQUIRE(v.patch() == 5);
    REQUIRE(*r.ptr == ';');
  }

  SUBCASE("prerelease tag parsed, stops at comma") {
    const char buf[] = "1.2.3-alpha.1,next";
    version<> v;
    const auto r = semver::from_chars(buf, buf + sizeof(buf) - 1, v);
    REQUIRE(r);
    REQUIRE(v.prerelease_tag() == "alpha.1");
    REQUIRE(*r.ptr == ',');
  }

  SUBCASE("build metadata parsed, stops at newline") {
    const char buf[] = "1.2.3+build.42\n";
    version<> v;
    const auto r = semver::from_chars(buf, buf + sizeof(buf) - 1, v);
    REQUIRE(r);
    REQUIRE(v.build_metadata() == "build.42");
    REQUIRE(*r.ptr == '\n');
  }

  SUBCASE("full prerelease+build, stops at tab") {
    const char buf[] = "1.0.0-rc.1+sha.abc\t";
    version<> v;
    const auto r = semver::from_chars(buf, buf + sizeof(buf) - 1, v);
    REQUIRE(r);
    REQUIRE(v.prerelease_tag() == "rc.1");
    REQUIRE(v.build_metadata() == "sha.abc");
    REQUIRE(*r.ptr == '\t');
  }

  SUBCASE("zero-length range returns failure") {
    const char buf[] = "";
    version<> v;
    REQUIRE_FALSE(semver::from_chars(buf, buf, v));
  }

  SUBCASE("invalid input (no dots) returns failure") {
    const char buf[] = "garbage";
    version<> v;
    REQUIRE_FALSE(semver::from_chars(buf, buf + sizeof(buf) - 1, v));
  }

  SUBCASE("uint8_t overflow returns result_out_of_range") {
    const char buf[] = "256.0.0";
    version<uint8_t> v;
    const auto r = semver::from_chars(buf, buf + sizeof(buf) - 1, v);
    REQUIRE_FALSE(r);
    REQUIRE(r.ec == std::errc::result_out_of_range);
  }

  SUBCASE("leading zero in component returns failure") {
    const char buf[] = "01.0.0 rest";
    version<> v;
    REQUIRE_FALSE(semver::from_chars(buf, buf + sizeof(buf) - 1, v));
  }

  SUBCASE("result is same as parse() for clean version string") {
    const char buf[] = "5.6.7-pre.1+meta";
    version<> vfc, vp;
    const auto rfc = semver::from_chars(buf, buf + sizeof(buf) - 1, vfc);
    REQUIRE(rfc);
    REQUIRE(semver::parse(std::string_view{buf, sizeof(buf) - 1}, vp));
    REQUIRE(vfc == vp);
  }
}

TEST_CASE("try_parse") {
  SUBCASE("valid string returns engaged optional with correct values") {
    const auto v = semver::try_parse("1.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->major() == 1);
    REQUIRE(v->minor() == 2);
    REQUIRE(v->patch() == 3);
  }

  SUBCASE("invalid strings return nullopt") {
    REQUIRE_FALSE(semver::try_parse("not-valid").has_value());
    REQUIRE_FALSE(semver::try_parse("").has_value());
    REQUIRE_FALSE(semver::try_parse("1.2").has_value());
    REQUIRE_FALSE(semver::try_parse("1.2.3garbage").has_value());
    REQUIRE_FALSE(semver::try_parse("01.0.0").has_value());
  }

  SUBCASE("prerelease and build metadata are preserved") {
    const auto v = semver::try_parse("1.0.0-rc.1+build.42");
    REQUIRE(v.has_value());
    REQUIRE(v->prerelease_tag() == "rc.1");
    REQUIRE(v->build_metadata() == "build.42");
  }

  SUBCASE("type parameter is forwarded") {
    const auto v8 = semver::try_parse<uint8_t>("255.255.255");
    REQUIRE(v8.has_value());
    REQUIRE(v8->patch() == 255);
    REQUIRE_FALSE(semver::try_parse<uint8_t>("256.0.0").has_value());
  }

  SUBCASE("try_parse result equals parse() result") {
    const auto opt = semver::try_parse("2.3.4-beta+meta");
    version<> parsed;
    REQUIRE(parse("2.3.4-beta+meta", parsed));
    REQUIRE(opt.has_value());
    REQUIRE(*opt == parsed);
  }
}

TEST_CASE("from_string") {
  SUBCASE("valid string returns version with correct values") {
    const auto v = semver::from_string("1.2.3-alpha+build");
    REQUIRE(v.major() == 1);
    REQUIRE(v.minor() == 2);
    REQUIRE(v.patch() == 3);
    REQUIRE(v.prerelease_tag() == "alpha");
    REQUIRE(v.build_metadata() == "build");
  }

  SUBCASE("invalid strings throw std::system_error") {
    REQUIRE_THROWS_AS((void)semver::from_string("not-a-version"), std::system_error);
    REQUIRE_THROWS_AS((void)semver::from_string(""),              std::system_error);
    REQUIRE_THROWS_AS((void)semver::from_string("1.2.3garbage"),  std::system_error);
  }

  SUBCASE("error code is invalid_argument for malformed input") {
    bool caught = false;
    try {
      (void)semver::from_string("garbage");
    } catch (const std::system_error& e) {
      caught = true;
      REQUIRE(e.code() == std::make_error_code(std::errc::invalid_argument));
    }
    REQUIRE(caught);
  }

  SUBCASE("error code is result_out_of_range on overflow") {
    bool caught = false;
    try {
      (void)semver::from_string<uint8_t>("256.0.0");
    } catch (const std::system_error& e) {
      caught = true;
      REQUIRE(e.code() == std::make_error_code(std::errc::result_out_of_range));
    }
    REQUIRE(caught);
  }

  SUBCASE("from_string result equals parse() result") {
    const auto v = semver::from_string("3.4.5-rc.2+sha.abc");
    version<> parsed;
    REQUIRE(parse("3.4.5-rc.2+sha.abc", parsed));
    REQUIRE(v == parsed);
  }
}

TEST_CASE("coerce") {
  SUBCASE("full version is unchanged") {
    const auto v = semver::coerce("1.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->major() == 1);
    REQUIRE(v->minor() == 2);
    REQUIRE(v->patch() == 3);
  }

  SUBCASE("missing patch filled with 0") {
    const auto v = semver::coerce("1.2");
    REQUIRE(v.has_value());
    REQUIRE(v->major() == 1);
    REQUIRE(v->minor() == 2);
    REQUIRE(v->patch() == 0);
  }

  SUBCASE("missing minor and patch filled with 0") {
    const auto v = semver::coerce("1");
    REQUIRE(v.has_value());
    REQUIRE(v->major() == 1);
    REQUIRE(v->minor() == 0);
    REQUIRE(v->patch() == 0);
  }

  SUBCASE("v prefix is stripped") {
    const auto v = semver::coerce("v1.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->major() == 1);
    REQUIRE(v->minor() == 2);
    REQUIRE(v->patch() == 3);
  }

  SUBCASE("V prefix is stripped") {
    const auto v = semver::coerce("V2.0");
    REQUIRE(v.has_value());
    REQUIRE(v->major() == 2);
    REQUIRE(v->minor() == 0);
    REQUIRE(v->patch() == 0);
  }

  SUBCASE("pre-release tag is preserved") {
    const auto v = semver::coerce("1.2.3-beta.1");
    REQUIRE(v.has_value());
    REQUIRE(v->prerelease_tag() == "beta.1");
  }

  SUBCASE("build metadata is preserved") {
    const auto v = semver::coerce("1.2.3+build.42");
    REQUIRE(v.has_value());
    REQUIRE(v->build_metadata() == "build.42");
  }

  SUBCASE("no leading digits returns nullopt") {
    REQUIRE_FALSE(semver::coerce("not-a-version").has_value());
  }

  SUBCASE("empty string returns nullopt") {
    REQUIRE_FALSE(semver::coerce("").has_value());
  }
}

TEST_CASE("clean") {
  SUBCASE("plain version passes through") {
    const auto v = semver::clean("1.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->major() == 1);
    REQUIRE(v->minor() == 2);
    REQUIRE(v->patch() == 3);
  }

  SUBCASE("leading = stripped") {
    const auto v = semver::clean("=1.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->major() == 1);
  }

  SUBCASE("leading =v stripped") {
    const auto v = semver::clean("=v1.2.3");
    REQUIRE(v.has_value());
    REQUIRE(v->major() == 1);
    REQUIRE(v->minor() == 2);
    REQUIRE(v->patch() == 3);
  }

  SUBCASE("leading v stripped") {
    const auto v = semver::clean("v1.2.3-alpha");
    REQUIRE(v.has_value());
    REQUIRE(v->prerelease_tag() == "alpha");
  }

  SUBCASE("surrounding whitespace stripped") {
    const auto v = semver::clean("  1.2.3  ");
    REQUIRE(v.has_value());
    REQUIRE(v->patch() == 3);
  }

  SUBCASE("incomplete version returns nullopt") {
    REQUIRE_FALSE(semver::clean("1.2").has_value());
  }

  SUBCASE("tilde range returns nullopt") {
    REQUIRE_FALSE(semver::clean("~1.2.3").has_value());
  }

  SUBCASE("double equals returns nullopt") {
    REQUIRE_FALSE(semver::clean("==1.2.3").has_value());
  }
}


