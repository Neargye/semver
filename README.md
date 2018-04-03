# semver c++11

C++ library compare and manipulate versions are available as extensions to the `<major>.<minor>.<patch>[-<prerel>]` format complying with [Semantic Versioning 2.0.0](semver.org)

Branch | Linux/OSX | Windows
-------|-----------|---------
master |[![Build Status](https://travis-ci.org/Neargye/semver.svg?branch=master)](https://travis-ci.org/Neargye/semver)|[![Build status](https://ci.appveyor.com/api/projects/status/74dwt15di5564tos/branch/master?svg=true)](https://ci.appveyor.com/project/Terik23/semver/branch/master)

## Features

* C++11
* Header-only
* Dependency-free
* constexpr version comparison: <, <=, ==, !=, > >=
* Version FromString/ToString

## Example

```cpp
#include <semver.hpp>
#include <iostream>

int main() {
  constexpr semver::Version v_default;
  static_assert(v_default == semver::Version(0, 1, 0, semver::Version::PreReleaseType::kNone, 0), "");

  std::cout << v_default << std::endl; // 0.1.0

  constexpr semver::Version v1(1, 4, 3);
  std::cout << v1 << std::endl; // 1.4.3
  constexpr semver::Version v2(1, 2, 4, semver::Version::PreReleaseType::kAlpha, 10);
  std::cout << v2 << std::endl; // 1.2.4-alpha.10
  static_assert(v1 > v2, "");

  semver::Version v_s;
  v_s.FromString("1.2.3-rc.1");
  const std::string s1 = v_s.ToString(); // 1.2.3-rc.1
  std::cout << s1 << std::endl;
  v_s.pre_release_version = 0;
  const std::string s2 = v_s.ToString(); // 1.2.3-rc
  std::cout << s2 << std::endl;

  semver::Version vi;
  std::cin >> vi;
  std::cout << vi << std::endl;

  return 0;
}
```

## License MIT