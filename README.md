[![Github releases](https://img.shields.io/github/release/Neargye/semver.svg)](https://github.com/Neargye/semver/releases)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver)
[![Conan package](https://img.shields.io/badge/Conan-package-blueviolet)](https://conan.io/center/recipes/neargye-semver)
[![License](https://img.shields.io/github/license/Neargye/semver.svg)](LICENSE)

C++ library compare and manipulate versions are available as extensions to the `<major>.<minor>.<patch>-<prerelease_tag>+<build_metadata>` format complying with [Semantic Versioning 2.0.0](https://semver.org)

## [Features & Examples](example/)

* Parse

  ```cpp
  semver::version v1;
  if (semver::parse("1.4.3", v1)) {
    const int patch = v1.patch(); // 3
    // do something...
  }

  semver::version v2;
  if (semver::parse("1.2.4-alpha.10")) {
    const std::string prerelease_tag = v2.prerelease_tag() // alpha.10
    // do something...
  }
  ```

* Сomparison

  ```cpp
  assert(v1 != v2);
  assert(v1 > v2);
  assert(v1 >= v2);
  assert(v2 < v1);
  assert(v2 <= v1);
  ```

* Validate

  ```cpp
  const bool result = semver::valid("1.2.3-alpha+build");
  assert(result);
  ```

* Range matching

  ```cpp
  semver::range_set range;
  if (semver::parse(">=1.0.0 <2.0.0 || >3.2.1", range)) {
    semver::version version;
    if (semver::parse("1.2.3", version)) {
      assert(range.contains(version));
    }
  }
  ```

Check the *examples* folder to see more various usage examples

## Default-constructed version

A default-constructed `semver::version{}` represents `0.1.0`, not `0.0.0`.
This is intentional and follows the [Semantic Versioning 2.0.0 FAQ](https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase):

> The simplest thing to do is start your initial development release at **0.1.0** and then increment the minor version for each subsequent release.

## Integration

You should add required file [semver.hpp](include/semver.hpp).

If you are using [vcpkg](https://github.com/Microsoft/vcpkg/) on your project for external dependencies, then you can use the [neargye-semver](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver).

If you are using [Conan](https://www.conan.io/) to manage your dependencies, merely add `neargye-semver/x.y.z` to your conan's requires, where `x.y.z` is the release version you want to use.


Alternatively, you can use something like [CPM](https://github.com/TheLartians/CPM) which is based on CMake's `Fetch_Content` module.

```cmake
CPMAddPackage(
    NAME semver
    GITHUB_REPOSITORY Neargye/semver
    GIT_TAG x.y.z # Where `x.y.z` is the release version you want to use.
)
```

## Compiler compatibility

* Clang/LLVM >= 5
* MSVC++ >= 14.11 / Visual Studio >= 2017
* Xcode >= 10
* GCC >= 7

## Licensed under the [MIT License](LICENSE)
