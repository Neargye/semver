#include <semver.hpp>
#include <sstream>
#include <type_traits>

int main() {
  semver::version direct{1, 2, 3};
  static_assert(std::is_same_v<decltype(direct), semver::version<>>);
  static_assert(std::is_same_v<decltype(direct.major()), std::uint32_t>);

  semver::version prerelease{1, 2, 3, "alpha.1"};
  const auto parsed = semver::try_parse("1.2.3");
  const auto parsed_prerelease = semver::try_parse("1.2.3-alpha+build");

  semver::range_set<> range;
  const auto range_result = semver::parse(">=1.0.0 <2.0.0", range);

  std::ostringstream stream;
  stream << direct;

  const auto ok = parsed && parsed_prerelease && range_result && direct == *parsed && direct.to_string() == "1.2.3" && prerelease.to_string() == "1.2.3-alpha.1" && stream.str() == "1.2.3"
               && !range.contains(*parsed_prerelease) && range.contains(*parsed_prerelease, semver::include_prerelease);
  return ok ? 0 : 1;
}
