#pragma once

#include <catch.hpp>

#define STATIC_CHECK_OP_AND_REVERSE(v1, op, v2) \
  static_assert(op(v1, v2));                      \
  static_assert(op(v2, v1));

#define STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
  static_assert(op(v1, v2));                            \
  static_assert(!op(v2, v1));

#define CHECK_OP_AND_REVERSE(v1, op, v2) \
  REQUIRE(op(v1, v2));                     \
  REQUIRE(op(v2, v1));

#define CHECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
  REQUIRE(op(v1, v2));                           \
  REQUIRE_FALSE(op(v2, v1));

#define CHECK_FALSE_OP_AND_REVERSE(v1, op, v2) \
  REQUIRE_FALSE(op(v1, v2));                     \
  REQUIRE_FALSE(op(v2, v1));