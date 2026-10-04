#include <D2RLPlugin/api.h>
#include "Common/D2Functions.h"
#include "Third Party/toml.hpp"
#include <string_view>
#include <filesystem>
using namespace std::literals;


constexpr auto kTOMLFile =
R"toml(# Hellforge
# Enables custom drop tables, drop counts and drop retry attempts for the hellforge quest, per difficulty.

[hellforge]
# Master switch - set to false to prevent hooks from being installed.
enabled = true

# If too many items are already on the ground, the drop function may fail. This
# determines how many extra attempts should be done before giving up.
max_retries = 5

# There are 2 loot tables for each difficulty, one for gems and one for runes.
# Max pool size is 255 for both runes and gems (independent of each other).

[normal]
gem_count = 3
rune_count = 1
rune_pool = ["r01", "r02", "r03", "r04", "r05", "r06", "r07", "r08", "r09", "r10", "r11"]
gem_pool = ["gpv", "gpr", "gpb", "gpy", "gpg", "gpw", "skz"]

[nightmare]
gem_count = 5
rune_count = 2
rune_pool = ["r12", "r13", "r14", "r15", "r16", "r17", "r18", "r19", "r20", "r21", "r22"]
gem_pool = ["gpv", "gpr", "gpb", "gpy", "gpg", "gpw", "skz"]

[hell]
gem_count = 6
rune_count = 4
rune_pool = ["r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27", "r28", "r29"]
gem_pool = ["gpv", "gpr", "gpb", "gpy", "gpg", "gpw", "skz"])toml"sv;


static constexpr D2RL::PluginFlags PatchingSampleFlags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks;

template<typename Fn>
Fn GetFn(const D2RL::PluginContext* pContext, uintptr_t rva, const void* pExpected, uint32_t expectedSize, const char* pFunctionName)
{
	if (!pContext->CheckExpectedBytes(rva, pExpected, expectedSize))
	{
		char msg[128];
		snprintf(msg, 128 - 1, "Invalid entry point: %s.", pFunctionName);
		pContext->LogError(msg);

		return reinterpret_cast<Fn>(nullptr);
	}

	return reinterpret_cast<Fn>(pContext->exeBase + rva);
}

#define D2_GET_FUNCTION(ctx, name) GetFn<name##Fn>(ctx, k##name##RVA, k##name##ExpectedBytes.data(), static_cast<uint32_t>(k##name##ExpectedBytes.size()), kp##name##Str)

UNITS_ChangeAnimModeFn UNITS_ChangeAnimMode{};
QUESTS_GetGlobalSeedFn QUESTS_GetGlobalSeed{};
SEED_RollLimitedRandomNumberFn SEED_RollLimitedRandomNumber{};
D2GAME_DropItemAtUnitFn D2GAME_DropItemAtUnit{};
EVENT_SetEventFn EVENT_SetEvent{};
ACT4Q3_CreateRewardFn ACT4Q3_CreateRewardOriginal{};
OBJECTS_OperateFunction49_HellForgeSetValuesFn OBJECTS_OperateFunction49_HellForgeSetValuesOriginal{};
QUESTS_GetQuestDataFn QUESTS_GetQuestData{};


constexpr uint8_t kInvalidRoll = std::numeric_limits<uint8_t>::max();
constexpr uint8_t kMaxGems = kInvalidRoll;
constexpr uint8_t kMaxRunes = kMaxGems;


// Forward declarations
void __fastcall OBJECTS_OperateFunction49_HellForgeSetValuesHook(void* pObj);
void __fastcall ACT4Q3_CreateRewardHook(void* pQuestData, void* pUnit);


// Main plugin class
class HellforgePlugin
{
public:
	struct LootTable
	{
		uint16_t m_GemCount;
		uint16_t m_RuneCount;
		std::vector<uint32_t> m_GemPool;
		std::vector<uint32_t> m_RunePool;
	};

	bool Install(const D2RL::PluginContext* pContext) noexcept
	{
		LoadConfig(pContext);

		if (!m_Enabled)
		{
			return true;
		}

		auto base = pContext->exeBase;
		UNITS_ChangeAnimMode = D2_GET_FUNCTION(pContext, UNITS_ChangeAnimMode);
		QUESTS_GetGlobalSeed = D2_GET_FUNCTION(pContext, QUESTS_GetGlobalSeed);
		SEED_RollLimitedRandomNumber = D2_GET_FUNCTION(pContext, SEED_RollLimitedRandomNumber);
		D2GAME_DropItemAtUnit = D2_GET_FUNCTION(pContext, D2GAME_DropItemAtUnit);
		EVENT_SetEvent = D2_GET_FUNCTION(pContext, EVENT_SetEvent);
		QUESTS_GetQuestData = D2_GET_FUNCTION(pContext, QUESTS_GetQuestData);

		bool fn1 = pContext->CheckExpectedBytes(kACT4Q3_CreateRewardRVA, kACT4Q3_CreateRewardExpectedBytes.data(), (uint32_t)kACT4Q3_CreateRewardExpectedBytes.size())
			&& pContext->InstallInlineHook(kACT4Q3_CreateRewardRVA, kACT4Q3_CreateRewardExpectedBytes.data(), (uint32_t)kACT4Q3_CreateRewardExpectedBytes.size(),
				ACT4Q3_CreateRewardHook, &ACT4Q3_CreateRewardOriginal);

		bool fn2 = pContext->CheckExpectedBytes(kOBJECTS_OperateFunction49_HellForgeSetValuesRVA,
			kOBJECTS_OperateFunction49_HellForgeSetValuesExpectedBytes.data(), (uint32_t)kOBJECTS_OperateFunction49_HellForgeSetValuesExpectedBytes.size())
			&& pContext->InstallInlineHook(kOBJECTS_OperateFunction49_HellForgeSetValuesRVA,
				kOBJECTS_OperateFunction49_HellForgeSetValuesExpectedBytes.data(), (uint32_t)kOBJECTS_OperateFunction49_HellForgeSetValuesExpectedBytes.size(),
				OBJECTS_OperateFunction49_HellForgeSetValuesHook, &OBJECTS_OperateFunction49_HellForgeSetValuesOriginal);

		return fn1 && fn2;
	}

	bool IsEnabled() const { return m_Enabled; }

	auto& GetLootTable(uint8_t difficulty) const { return m_LootTables[difficulty]; }
	auto GetMaxRetries() const { return m_MaxRetries; }

private:
	std::filesystem::path GetEXEDirectory()
	{
		std::vector<wchar_t> buffer(MAX_PATH);
		DWORD copied = 0;

		while (true) {
			copied = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

			if (copied == 0)
			{
				return {};
			}

			if (copied < buffer.size())
			{
				buffer.resize(copied);
				break;
			}

			// Double the buffer size and retry
			buffer.resize(buffer.size() * 2);
		}

		std::filesystem::path exePath(buffer.begin(), buffer.end());

		// Return only the parent directory
		return exePath.parent_path();
	}

	void LoadConfig(const D2RL::PluginContext* pContext)
	{
		toml::parse_result table;

		std::vector<char> data(1000, 0);
		uint32_t requiredSize{};
		if (pContext->ReadConfig(data.data(), uint32_t(data.size()), &requiredSize))
		{
			table = toml::parse(data.data());
		}
		else if (requiredSize)
		{
			data.resize(requiredSize + 1, 0);
			if (pContext->ReadConfig(data.data(), uint32_t(data.size()), &requiredSize))
			{
				table = toml::parse(data.data());
			}
		}

		if (table.empty())
		{
			pContext->LogError("Couldn't read Hellforge config file, loading default settings.");
			table = toml::parse(kTOMLFile);
		}

		auto config = table["hellforge"];
		m_Enabled = config["enabled"].value_or(false);
		m_MaxRetries = config["max_retries"].value_or(0);

		auto fillValues = [this, &table](const char* pTable, uint32_t index)
			{
				union StrToCode
				{
					uint32_t itemCode;
					char codeStr[4];
				} strToCode;
				uint32_t currChar = 0;

				auto lootTable = table[pTable];
				
				m_LootTables[index].m_GemCount = lootTable["gem_count"].value_or<uint16_t>(0);
				auto gems = lootTable["gem_pool"].as_array();
				m_LootTables[index].m_GemPool.reserve(std::min(gems->size(), size_t(kMaxGems)));
				for (auto it = gems->begin(); it != gems->end() && m_LootTables[index].m_GemPool.size() <= kMaxGems; it++)
				{
					strToCode.itemCode = '    ';
					currChar = 0;
					auto sv = it->value_or(""sv);
					for (auto s : sv)
					{
						strToCode.codeStr[currChar++] = s;
					}

					m_LootTables[index].m_GemPool.push_back(strToCode.itemCode);
				}

				m_LootTables[index].m_RuneCount = lootTable["rune_count"].value_or<uint16_t>(0);
				auto runes = lootTable["rune_pool"].as_array();
				m_LootTables[index].m_RunePool.reserve(std::min(runes->size(), size_t(kMaxRunes)));
				for (auto it = runes->begin(); it != runes->end() && m_LootTables[index].m_RunePool.size() <= kMaxRunes; it++)
				{
					strToCode.itemCode = '    ';
					currChar = 0;
					auto sv = it->value_or(""sv);
					for (auto s : sv)
					{
						strToCode.codeStr[currChar++] = s;
					}

					m_LootTables[index].m_RunePool.push_back(strToCode.itemCode);
				}
			};

		fillValues("normal", 0);
		fillValues("nightmare", 1);
		fillValues("hell", 2);
	}

private:
	LootTable m_LootTables[3];
	uint32_t m_MaxRetries{};
	bool m_Enabled{};
};


HellforgePlugin g_HellforgePlugin;


// Modified A4Q3 struct to split fields into more options
struct D2Act4Quest3CustomStrc
{
	short nHellforgeObjectMode;
	uint8_t bCainActivated;
	uint8_t bRewardDropsPending;
	uint8_t bSoulstoneSmashed;
	uint8_t nFailedDropAttempts;
	uint8_t nLastUnusedGemRoll;
	uint8_t nLastUnusedRuneRoll;
	short nGemsRemaining;
	short nRunesRemaining;
	int nHits;
	int nHellforgeHammersInGame;
	int nPlayersInAct;
	int unused0x18;
	uint8_t bSoulstoneAcquired;
	uint8_t pad0x1D[3];
};

void __fastcall OBJECTS_OperateFunction49_HellForgeSetValuesHook(void* pObj)
{
	uint8_t* pGame = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(pObj) + 0x8);
	auto pQuestData = QUESTS_GetQuestData(pGame, 0x18);
	if (!pQuestData)
	{
		OBJECTS_OperateFunction49_HellForgeSetValuesOriginal(pObj);
		return;
	}

	D2Act4Quest3CustomStrc& hellforgeQuestData = *reinterpret_cast<D2Act4Quest3CustomStrc*>(reinterpret_cast<uint8_t*>(pQuestData) + 0x20);
	uint8_t expansion = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pGame) + 0x106);
	uint8_t difficulty = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pGame) + 0x104);

	auto& lootTable = g_HellforgePlugin.GetLootTable(difficulty);

	hellforgeQuestData.bSoulstoneSmashed = 1;
	hellforgeQuestData.bRewardDropsPending = 1;
	hellforgeQuestData.nPlayersInAct = 1; // This never seems to be set to anything other than 1
	hellforgeQuestData.nGemsRemaining = int16_t(lootTable.m_GemCount);
	hellforgeQuestData.nRunesRemaining = int16_t(expansion != 1) * int16_t(lootTable.m_RuneCount);
	hellforgeQuestData.nFailedDropAttempts = 0;
	hellforgeQuestData.nLastUnusedGemRoll = kInvalidRoll;
	hellforgeQuestData.nLastUnusedRuneRoll = kInvalidRoll;
}


void __fastcall ACT4Q3_CreateRewardHook(void* pQuestData, void* pUnit)
{
	uint8_t* pGame = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(pQuestData) + 0x8);
	uint32_t& unitDropItemCode = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pUnit) + 0x118);
	uint32_t gameFrame = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pGame) + 0x170);
	uint8_t difficulty = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pGame) + 0x104);

	D2Act4Quest3CustomStrc& hellforgeQuestData = *reinterpret_cast<D2Act4Quest3CustomStrc*>(reinterpret_cast<uint8_t*>(pQuestData) + 0x20);

	auto& lootTable = g_HellforgePlugin.GetLootTable(difficulty);
	auto maxRetries = uint8_t(g_HellforgePlugin.GetMaxRetries());

	if (hellforgeQuestData.bSoulstoneSmashed)
	{
		UNITS_ChangeAnimMode(pUnit, 4);
	}

	if (!hellforgeQuestData.bRewardDropsPending)
	{
		return;
	}

	// If no players are in act, consider it completed
	if (!hellforgeQuestData.nPlayersInAct)
	{
		hellforgeQuestData.bRewardDropsPending = 0;
		return;
	}

	int32_t gemsDropped = 0;
	//int32_t gemGroupIndex = 0;
	D2SeedStrc* pSeed = nullptr;
	uint32_t itemCode = 0;

	if (hellforgeQuestData.nGemsRemaining > 0)
	{
		// So I'm not sure why there's a lot for the gem case, I don't think nPlayersInAct is ever != 1, but I'll keep it for now
		for (auto counter = 0; counter < hellforgeQuestData.nPlayersInAct; counter++)
		{
			// See if we have a previous roll from a failed drop attempt, or get a new one
			uint32_t roll = hellforgeQuestData.nLastUnusedGemRoll;
			if (roll == kInvalidRoll)
			{
				pSeed = QUESTS_GetGlobalSeed(pGame);
				roll = SEED_RollLimitedRandomNumber(pSeed, int32_t(lootTable.m_GemPool.size()));
			}

			itemCode = lootTable.m_GemPool[roll];

			unitDropItemCode = itemCode;
			int32_t itemLevel = 50;
			if (D2GAME_DropItemAtUnit(pGame, pUnit, 2, &itemLevel, 0, -1, 0))
			{
				++gemsDropped;
				hellforgeQuestData.nLastUnusedGemRoll = kInvalidRoll;
			}
			else
			{
				// We may fail at dropping an item, if so, mark it as a failed attempt
				// and save the rolled value
				hellforgeQuestData.nLastUnusedGemRoll = roll;
				if (hellforgeQuestData.nFailedDropAttempts++ == maxRetries)
				{
					hellforgeQuestData.nGemsRemaining = 0;
					break;
				}
			}
		}
	}

	if (hellforgeQuestData.nGemsRemaining)
	{
		if (gemsDropped)
		{
			hellforgeQuestData.nGemsRemaining--;
		}
	}
	// if we're in classic, gemDropData.nRunesRemaining will be 0
	else if (hellforgeQuestData.nRunesRemaining)
	{
		// See if we have a previous roll from a failed drop attempt, or get a new one
		uint32_t roll = hellforgeQuestData.nLastUnusedRuneRoll;
		if (roll == kInvalidRoll)
		{
			pSeed = QUESTS_GetGlobalSeed(pGame);
			roll = SEED_RollLimitedRandomNumber(pSeed, int32_t(lootTable.m_RunePool.size()));
		}

		uint32_t runeCode = lootTable.m_RunePool[roll];
		unitDropItemCode = runeCode;
		int32_t itemLevel = 50;
		if (D2GAME_DropItemAtUnit(pGame, pUnit, 2, &itemLevel, 0, -1, 0))
		{
			hellforgeQuestData.nRunesRemaining--;
			hellforgeQuestData.nLastUnusedRuneRoll = kInvalidRoll;
		}
		else
		{
			// We may fail at dropping an item, if so, mark it as a failed attempt
			// and save the rolled value
			hellforgeQuestData.nLastUnusedRuneRoll = roll;
			if (hellforgeQuestData.nFailedDropAttempts++ == maxRetries)
			{
				hellforgeQuestData.nRunesRemaining = 0;
			}
		}
	}

	if (hellforgeQuestData.nGemsRemaining + hellforgeQuestData.nRunesRemaining == 0)
	{
		hellforgeQuestData.bRewardDropsPending = 0;
	}
	else
	{
		EVENT_SetEvent(pGame, pUnit, 7, gameFrame + 20, 0, 0, 0);
	}
}


static constexpr D2RL::PluginInfo PatchingSampleInfo
{
	.infoSize = D2RL::PluginInfoSize,
	.abiVersion = D2RL_PLUGIN_ABI_VERSION,
	.id = "hellforge",
	.name = "Hellforge Plugin",
	.version = "0.9.0",
	.author = "ArchVile",
	.description = "Allows control over how many gems and runes the hellforge quest gives, as well as which ones.",
	.flags = PatchingSampleFlags,
};



D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo*
{
	return &PatchingSampleInfo;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* pContext) noexcept -> bool
{
	if (!pContext)
	{
		return false;
	}

	if (!g_HellforgePlugin.Install(pContext))
	{
		pContext->LogError("Hellforge plugin failed.");
		return false;
	}

	if (g_HellforgePlugin.IsEnabled())
	{
		pContext->LogInfo("Hellforge hook installed.");
	}
	else
	{
		pContext->LogInfo("Hellforge hook not installed.");
	}

	return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {}