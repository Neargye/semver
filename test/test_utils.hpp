#pragma once

#include <catch.hpp>

#define STATIC_CHECK_OP_AND_REVERSE(v1, op, v2) \
static_assert(v1 op v2);                      \
static_assert(v2 op v1);

#define STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
static_assert(v1 op v2);                            \
static_assert(!(v2 op v1));

#define CHECK_OP_AND_REVERSE(v1, op, v2) \
REQUIRE(v1 op v2);                     \
REQUIRE(v2 op v1);

#define CHECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
REQUIRE(v1 op v2);                           \
REQUIRE_FALSE(v2 op v1);

#define CHECK_FALSE_OP_AND_REVERSE(v1, op, v2) \
REQUIRE_FALSE(v1 op v2);                     \
REQUIRE_FALSE(v2 op v1);