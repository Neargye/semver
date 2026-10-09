// probe: H5 README compatibility claim (GCC >= 7, Clang >= 6): the whole public API instantiated with narrow, mixed and default components
// where: gcc-old clang-old
// std: c++17 c++20
// expect: build-fail "error"
// meaning: H5 predicts that compilers older than the CI matrix (GCC 9-12, Clang 11-15 from the ubuntu-22.04 repositories)
//   reject some construct; build-fail = README claim false for that compiler/standard (key line names the construct);
//   build-ok = claim holds for that compiler/standard ("NOT held" is the good outcome here).
//   Jobs gcc-old / clang-old are not in the standard job list: run run.py once per old compiler with --job gcc-old or clang-old.
#include <semver.hpp>
#include <cstdint>
#include <functional>
#include <sstream>
#include <vector>

template <typename... I>
int use() {
  using V = semver::version<I...>;
  V v;
  (void)semver::parse("1.2.3-a.1+b", v);
  const auto t = semver::try_parse<I...>("1.2.3");
  const auto c = semver::coerce<I...>("v1.2-rc.1");
  const auto cl = semver::clean<I...>(" =v1.2.3 ");
  char buf[64];
  (void)semver::to_chars(buf, buf + sizeof buf, v);
  std::ostringstream os;
  os << v;
  semver::range_set<I...> rs;
  (void)semver::parse("^1.2 || >=2.0.0-0 <3 !=2.1.0", rs);
  const auto mv = semver::min_version(rs);
  const bool x = semver::intersects(rs, rs) && rs.contains(v) && semver::satisfies(v, "1.x");
  const auto n = semver::inc(v, semver::version_change::prerelease);
  const auto d = semver::diff(v, v.bump_major());
  std::vector<V> vs{v, v.bump_minor(), v.bump_patch()};
  (void)semver::max_satisfying(vs.begin(), vs.end(), rs);
  (void)semver::min_satisfying(vs.begin(), vs.end(), rs);
  (void)std::hash<V>{}(v);
  V w = v.without_prerelease().without_build_metadata();
  swap(v, w);
  return x + !!t + !!c + !!cl + !!mv + !!n + static_cast<int>(d) + semver::compare(v, V{}) + semver::compare_with_build(v, w) +
         (v < w) + (v == w) + (v != w) + (v >= w);
}

int main() {
  // Every instantiation above must compile; the return value is irrelevant for the prediction.
  return use<std::uint8_t>() + use<std::uint16_t, std::uint32_t, std::uint64_t>() + use<>() +
         use<unsigned char, unsigned short, unsigned long long>();
}
