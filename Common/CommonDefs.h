#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include "Third Party/D2RLPlugin/api.h"
#define TOML_EXCEPTIONS 0
#include "Third Party/toml.hpp"
#include <string_view>
#include <filesystem>
using namespace std::literals;


#define D2_FUNCTION_DEF(name, rva, prototype, ...) \
using name##Fn = prototype; \
inline constexpr uintptr_t k##name##RVA = rva; \
inline constexpr std::initializer_list<uint8_t> k##name##ExpectedBytes = { __VA_ARGS__ }; \
inline constexpr const char* kp##name##Str = #name;


template<typename Fn>
bool GetFn(const D2RL::PluginContext* pContext, uintptr_t rva, const void* pExpected, uint32_t expectedSize, const char* pFunctionName, Fn& fn)
{
	if (!pContext->CheckExpectedBytes(rva, pExpected, expectedSize))
	{
		char msg[128];
		snprintf(msg, 128 - 1, "Invalid entry point: %s.", pFunctionName);
		pContext->LogError(msg);

		fn = reinterpret_cast<Fn>(nullptr);
		return false;
	}

	fn = reinterpret_cast<Fn>(pContext->exeBase + rva);
	return true;
}

template<typename Fn>
bool HookFn(const D2RL::PluginContext* pContext, uintptr_t rva, const void* pExpected, uint32_t expectedSize, const char* pFunctionName, Fn targetFn, Fn& originalFn)
{
	targetFn = reinterpret_cast<Fn>(nullptr);
	originalFn = reinterpret_cast<Fn>(nullptr);

	if (!pContext->CheckExpectedBytes(rva, pExpected, expectedSize))
	{
		char msg[128];
		snprintf(msg, 128 - 1, "Invalid entry point: %s.", pFunctionName);
		pContext->LogError(msg);

		return false;
	}

	if (!pContext->InstallInlineHook(rva, pExpected, expectedSize, targetFn, &originalFn))
	{
		char msg[128];
		snprintf(msg, 128 - 1, "Failed creating hook for entry point: %s.", pFunctionName);
		pContext->LogError(msg);

		return false;
	}

	return true;
}

#define D2_GET_FUNCTION(ctx, name) GetFn<name##Fn>(ctx, k##name##RVA, k##name##ExpectedBytes.data(), static_cast<uint32_t>(k##name##ExpectedBytes.size()), kp##name##Str, name)
#define D2_HOOK_FUNCTION(ctx, name) HookFn<name##Fn>(ctx, k##name##RVA, k##name##ExpectedBytes.data(), static_cast<uint32_t>(k##name##ExpectedBytes.size()), kp##name##Str, \
name##Hook, name##Original)