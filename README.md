[![Github releases](https://img.shields.io/github/release/Neargye/semver.svg)](https://github.com/Neargye/semver/releases)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver)
[![Conan package](https://img.shields.io/badge/Conan-package-blueviolet)](https://conan.io/center/recipes/neargye-semver)
[![License](https://img.shields.io/github/license/Neargye/semver.svg)](LICENSE)

Header-only C++17 library for [Semantic Versioning 2.0.0](https://semver.org). It provides strict version parsing, comparison, formatting, increments, and a documented range syntax for common version constraints. No dependencies.

```cpp
#include <cassert>
#include <semver.hpp>

int main() {
  const auto version = semver::from_string("1.2.3");
  assert(version.major() == 1);

  const auto range = semver::try_parse_range("^1.2");
  if (!range)
    return 1;

  assert(range->contains(version));
}
```

## Features

- Strict SemVer 2.0.0 parsing and validation.
- Configurable unsigned component types; `version<>` uses `std::uint32_t`.
- Mixed-component comparison and range matching without narrowing.
- Compact `*`/`x`/`X` wildcard, partial, comparator, tilde, caret, whitespace-AND, and `||` range syntax.
- Range matching, intersection checks, and minimum-version queries.
- `from_chars`/`to_chars`, streams, `std::hash`, and optional `std::format` support.
- Compile-time parsing where the compiler and standard library support it.

See the [API reference](doc/reference.md) and [limitations](doc/limitations.md).

## Integration

Copy [`semver.hpp`](include/semver.hpp), or use a package manager:

- **vcpkg:** `neargye-semver`
- **Conan:** `neargye-semver/x.y.z`
- **CPM:** `CPMAddPackage(GITHUB_REPOSITORY Neargye/semver GIT_TAG x.y.z)`
- **CMake package:**

  ```cmake
  find_package(semver CONFIG REQUIRED)
  target_link_libraries(your_target PRIVATE semver::semver)
  ```

More examples are available in [`example/`](example/).

## [MIT License](LICENSE)
