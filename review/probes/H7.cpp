// probe: H7 std::formatter<semver::version> (const format calling formatter<string_view>::format) on current libraries
// where: gcc-latest clang-libcxx macos-appleclang msvc-x64 msvc-x86 clangcl
// std: c++20 c++23
// expect: run-ok
// meaning: H7 predicts a failure only on libraries released before LWG 3636 (VS 2019, early libc++), which the CI images
//   do not have; run-ok = formatter works with this library (or __cpp_lib_format is not defined, printed as "no format");
//   build-fail = formatter broken on a current library (H7 wider than assumed); run-fail = wrong output.
#include <semver.hpp>
#include <cstdio>
#include <string>
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
#  include <format>
#endif

int main() {
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
  const std::string s = std::format("[{:>14}][{}]", semver::version<>{1, 2, 3, "rc.1"}, semver::version<>{4, 5, 6});
  std::printf("%s\n", s.c_str());
  return s == "[    1.2.3-rc.1][4.5.6]" ? 0 : 1;
#else
  std::printf("no format\n");
  return 0;
#endif
}
