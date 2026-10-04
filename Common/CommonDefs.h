#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <array>

#define D2_FUNCTION_DEF(name, rva, prototype, ...) \
using name##Fn = prototype; \
inline constexpr uintptr_t k##name##RVA = rva; \
inline constexpr std::initializer_list<uint8_t> k##name##ExpectedBytes = { __VA_ARGS__ }; \
inline constexpr const char* kp##name##Str = #name;