#include <semver.hpp>
#include <doctest.h>
#include <array>
#include <iomanip>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include "test_utils.hpp"

using namespace semver;

namespace {

template <typename Stream, typename Value, typename = void>
inline constexpr bool is_stream_insertable_v = false;

template <typename Stream, typename Value>
inline constexpr bool is_stream_insertable_v<Stream, Value, std::void_t<decltype(std::declval<Stream&>() << std::declval<const Value&>())>> = true;

} // namespace

TEST_CASE("to string") {
  version version;
  CHECK(version.to_string() == "0.1.0");

  for (auto str : valid_versions) {
    REQUIRE(parse(str, version));
    CHECK(version.to_string() == str);
  }
}

TEST_CASE("operator<<") {
  std::ostringstream os;

  SUBCASE("basic version") {
    version v;
    REQUIRE(parse("1.2.3", v));
    os << v;
    REQUIRE(os.str() == "1.2.3");
  }

  SUBCASE("with prerelease and build metadata") {
    version v;
    REQUIRE(parse("1.0.0-alpha.1+build.42", v));
    os << v;
    REQUIRE(os.str() == "1.0.0-alpha.1+build.42");
  }

  SUBCASE("uint8_t components are rendered as numbers") {
    const version<std::uint8_t> v{std::uint8_t{255}, std::uint8_t{2}, std::uint8_t{3}};
    os << v;
    REQUIRE(os.str() == "255.2.3");
  }

  SUBCASE("numeric flags do not alter the canonical version") {
    const version<> v{10, 11, 12, "rc.1", "build"};
    os << std::hex << std::showbase << v;
    REQUIRE(os.str() == "10.11.12-rc.1+build");
  }

  SUBCASE("width applies to the complete version") {
    const version<> v{10, 11, 12, "rc.1", "build"};
    os << std::setfill('_') << std::setw(24) << v;
    REQUIRE(os.str() == "_____10.11.12-rc.1+build");
  }

  SUBCASE("insertion is limited to narrow streams") {
    static_assert(is_stream_insertable_v<std::ostringstream, version<>>);
    static_assert(!is_stream_insertable_v<std::wostringstream, version<>>);
  }
}

TEST_CASE("to_string with various integer widths") {
  SUBCASE("uint8_t boundary value") {
    semver::version<uint8_t> v;
    REQUIRE(semver::parse("255.0.0", v));
    REQUIRE(v.to_string() == "255.0.0");
  }

  SUBCASE("uint16_t boundary value") {
    semver::version<uint16_t> v;
    REQUIRE(semver::parse("65535.0.0", v));
    REQUIRE(v.to_string() == "65535.0.0");
  }

  SUBCASE("uint32_t boundary value") {
    semver::version<uint32_t> v;
    REQUIRE(semver::parse("4294967295.0.0", v));
    REQUIRE(v.to_string() == "4294967295.0.0");
  }

  SUBCASE("uint64_t large patch value") {
    semver::version<uint64_t> v;
    REQUIRE(semver::parse("999999999999.1.0", v));
    REQUIRE(v.to_string() == "999999999999.1.0");
  }

  SUBCASE("complex prerelease and build metadata preserved exactly") {
    semver::version<> v;
    REQUIRE(semver::parse("1.2.3----RC-SNAPSHOT.12.9.1--.12+788", v));
    REQUIRE(v.to_string() == "1.2.3----RC-SNAPSHOT.12.9.1--.12+788");
  }
}

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
TEST_CASE("std::formatter") {
  semver::version<> v;
  REQUIRE(semver::parse("1.2.3-alpha.1+build.42", v));

  SUBCASE("std::format basic") {
    REQUIRE(std::format("{}", v) == "1.2.3-alpha.1+build.42");
  }

  SUBCASE("std::format_to") {
    std::string out;
    std::format_to(std::back_inserter(out), "{}", v);
    REQUIRE(out == "1.2.3-alpha.1+build.42");
  }

  SUBCASE("default version") {
    semver::version<> def;
    REQUIRE(std::format("{}", def) == "0.1.0");
  }

  SUBCASE("standard string format specs") {
    REQUIRE(std::format("{:>30}", v) == std::format("{:>30}", v.to_string()));
    REQUIRE(std::format("{:*<30}", v) == std::format("{:*<30}", v.to_string()));
    REQUIRE(std::format("{:.5}", v) == "1.2.3");
  }

  SUBCASE("non-string format spec throws") {
    CHECK_THROWS_AS((void)std::vformat("{:d}", std::make_format_args(v)), std::format_error);
  }
}
#endif

TEST_CASE("to_chars zero-allocation serialization") {
  static_assert(!std::is_same_v<semver::to_chars_result, semver::from_chars_result>);
  static_assert(std::is_same_v<decltype(semver::to_chars_result::ptr), char*>);
  static_assert(std::is_same_v<decltype(semver::from_chars_result::ptr), const char*>);

  SUBCASE("basic version") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    char buf[32];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "1.2.3");
  }

  SUBCASE("version with prerelease") {
    version<> v;
    REQUIRE(parse("1.0.0-alpha.1", v));
    char buf[32];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "1.0.0-alpha.1");
  }

  SUBCASE("version with build metadata") {
    version<> v;
    REQUIRE(parse("2.3.4+build.42", v));
    char buf[32];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "2.3.4+build.42");
  }

  SUBCASE("version with prerelease and build metadata") {
    version<> v;
    REQUIRE(parse("1.0.0-alpha.1+build.42", v));
    char buf[64];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "1.0.0-alpha.1+build.42");
  }

  SUBCASE("buffer exactly the right size succeeds") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    char buf[5];
    const auto r = semver::to_chars(buf, buf + 5, v);
    REQUIRE(r);
    REQUIRE(r.ptr == buf + 5);
  }

  SUBCASE("buffer one byte too small fails") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    char buf[4];
    const auto r = semver::to_chars(buf, buf + 4, v);
    REQUIRE_FALSE(r);
    REQUIRE(r.ec == std::errc::value_too_large);
    REQUIRE(r.ptr == buf + 4);
  }

  SUBCASE("empty buffer fails") {
    version<> v;
    REQUIRE(parse("0.0.0", v));
    char buf[1];
    const auto r = semver::to_chars(buf, buf, v);
    REQUIRE_FALSE(r);
    REQUIRE(r.ec == std::errc::value_too_large);
  }

  SUBCASE("round-trip: to_chars output matches to_string") {
    constexpr std::array<std::string_view, 6> cases = {{
      "0.0.0", "1.2.3", "1.0.0-alpha.1+build", "10.20.30",
      "999999999.999999999.999999999", "1.2.3----RC-SNAPSHOT.12.9.1--.12+788"
    }};
    for (auto sv : cases) {
      version<> v;
      REQUIRE(parse(sv, v));
      char buf[256];
      const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
      REQUIRE(r);
      REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == v.to_string());
    }
  }

  SUBCASE("uint8_t version serializes correctly") {
    version<uint8_t> v;
    REQUIRE(parse("255.0.1", v));
    char buf[16];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "255.0.1");
  }

  SUBCASE("ptr advances by exactly the written length") {
    version<> v;
    REQUIRE(parse("10.200.3000", v));
    char buf[32] = {};
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    const auto written = static_cast<std::size_t>(r.ptr - buf);
    REQUIRE(written == std::string_view("10.200.3000").size());
  }

  SUBCASE("null pointer ranges return value_too_large") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    char buf[32];

    const auto both_null = semver::to_chars(nullptr, nullptr, v);
    REQUIRE_FALSE(both_null);
    REQUIRE(both_null.ec == std::errc::value_too_large);
    REQUIRE(both_null.ptr == nullptr);

    const auto null_first = semver::to_chars(nullptr, buf, v);
    REQUIRE_FALSE(null_first);
    REQUIRE(null_first.ec == std::errc::value_too_large);
    REQUIRE(null_first.ptr == buf);

    const auto null_last = semver::to_chars(buf, nullptr, v);
    REQUIRE_FALSE(null_last);
    REQUIRE(null_last.ec == std::errc::value_too_large);
    REQUIRE(null_last.ptr == nullptr);
  }

  SUBCASE("reversed pointers (last < first) returns value_too_large") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    char buf[32];
    const auto r = semver::to_chars(buf + 10, buf, v);
    REQUIRE_FALSE(r);
    REQUIRE(r.ec == std::errc::value_too_large);
  }
}
