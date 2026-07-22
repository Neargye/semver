#include <semver.hpp>
#include <cstdint>
#include <sstream>
#include <type_traits>

int main() {
  semver::version direct{1, 2, 3};
  static_assert(std::is_same_v<decltype(direct), semver::version<>>);
  static_assert(std::is_same_v<decltype(direct.major()), std::uint32_t>);

  semver::version wide{std::uint64_t{5'000'000'000}, 2, 3};
  static_assert(std::is_same_v<decltype(wide), semver::version<std::uint64_t, std::uint32_t, std::uint32_t>>);

  const auto qualified = semver::from_string("1.2.3-alpha+build");
  const auto parsed = semver::try_parse("1.2.3");
  const auto next = semver::inc(direct, semver::version_change::patch);

  semver::range_set<> range;
  const auto range_result = semver::parse("^1.2", range);
  const auto parsed_range = semver::try_parse_range("1.x");
  semver::range_set<> compatible;
  const auto compatible_result = semver::parse(">=1 <2 !=1.5.0", compatible);
  const semver::version<std::uint64_t> mixed{1, 9, 0};
  const auto minimum = semver::min_version(range);

  std::ostringstream stream;
  stream << direct;

  const auto ok =
      parsed && next && range_result && parsed_range && compatible_result && direct == *parsed && direct.to_string() == "1.2.3" &&
      next->to_string() == "1.2.4" && semver::diff(direct, *next) == semver::version_change::patch &&
      wide.major() == std::uint64_t{5'000'000'000} &&
      qualified.without_prerelease().to_string() == "1.2.3+build" &&
      qualified.without_build_metadata().to_string() == "1.2.3-alpha" && stream.str() == "1.2.3" &&
      range.contains(direct) && range.contains(mixed) && parsed_range->contains(direct) && minimum && minimum->to_string() == "1.2.0" &&
      semver::intersects(range, compatible);
  return ok ? 0 : 1;
}
