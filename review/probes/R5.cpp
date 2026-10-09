// probe: R5 32-bit targets: 64-bit components, hashing, length limits and range algorithms at run time
// where: gcc-m32 msvc-x86 msvc-x64
// std: c++17 c++20
// expect: run-ok
// meaning: run-ok = no 32-bit specific defect (size_t 32 bit, std::hash 32-bit constant branch, uint64_t components);
//   run-fail = the failing check is printed as "FAIL <n>"; msvc-x64 is the 64-bit control for msvc-x86.
#include <semver.hpp>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <limits>
#include <string>
#include <unordered_set>

static int failures = 0;
static void check(bool ok, int n) {
  if (!ok) {
    std::printf("FAIL %d\n", n);
    ++failures;
  }
}

int main() {
  using V64 = semver::version<std::uint64_t>;
  V64 v;
  check(static_cast<bool>(semver::parse("18446744073709551615.18446744073709551615.18446744073709551615-rc.1+b", v)), 1);
  check(v.major() == (std::numeric_limits<std::uint64_t>::max)(), 2);
  check(v.to_string() == "18446744073709551615.18446744073709551615.18446744073709551615-rc.1+b", 3);
  check(!semver::parse("18446744073709551616.0.0", v) && v.major() == (std::numeric_limits<std::uint64_t>::max)(), 4);
  check(!semver::inc(V64{(std::numeric_limits<std::uint64_t>::max)(), 0u, 0u}, semver::version_change::major).has_value(), 5);

  char buf[80];
  const auto r = semver::to_chars(buf, buf + sizeof buf, v);
  check(r && std::string(buf, r.ptr) == v.to_string(), 6);
  const auto small = semver::to_chars(buf, buf + 10, v);
  check(!small && small.ec == std::errc::value_too_large && small.ptr == buf + 10, 7);

  const std::string over(semver::max_input_length + 1, '1');
  semver::version<> w;
  const auto fr = semver::from_chars(over.data(), over.data() + over.size(), w);
  check(!fr && fr.ec == std::errc::value_too_large, 8);
  check(!semver::coerce(over).has_value(), 9);

  std::unordered_set<semver::version<>> set;
  for (unsigned i = 0; i < 2000; ++i) set.insert(semver::version<>{i % 10, i / 10 % 10, i / 100, i % 3 ? "" : "rc.1"});
  check(set.size() == 2000, 10);
  check(std::hash<semver::version<>>{}(semver::version<>{1, 2, 3, "a", "b1"}) == std::hash<semver::version<>>{}(semver::version<>{1, 2, 3, "a", "b2"}), 11);

  semver::range_set<std::uint64_t> rs;
  check(static_cast<bool>(semver::parse(">=4294967296.0.0 <18446744073709551615.0.0 || ^1.2", rs)), 12);
  check(rs.contains(V64{4294967296u, 0u, 0u}) && !rs.contains(V64{4294967295u, 0u, 0u}), 13);
  const auto mv = semver::min_version(rs);
  check(mv && *mv == V64{1u, 2u, 0u}, 14);
  semver::range_set<std::uint64_t> other;
  check(static_cast<bool>(semver::parse(">=5000000000.1.0-0 <5000000000.1.0", other)) && !semver::intersects(rs, other, semver::prerelease_policy::exclude), 15);
  check(semver::intersects(rs, other, semver::prerelease_policy::include), 16);

  check(semver::compare(semver::version<std::uint8_t, std::uint64_t, std::uint16_t>{1u, 4294967296u, 3u}, V64{1u, 4294967296u, 3u}) == 0, 17);
  std::printf("sizeof(size_t)=%u failures=%d\n", static_cast<unsigned>(sizeof(std::size_t)), failures);
  return failures == 0 ? 0 : 1;
}
