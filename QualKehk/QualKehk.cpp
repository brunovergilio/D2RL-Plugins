#include "Common/D2Functions.h"


constexpr auto kTOMLFile =
R"toml(# Qual-Kehk
# Enables custom rune loot tables for Qual-Kehk's quest, for each difficulty.

[qualkehk]
# Master switch - set to false to prevent hooks from being installed.
enabled = true

# runes represents an array of item codes. These are not runes the code rolls from - they're
# the exact rewards Qual-Kehk will give per difficulty. Maximum reward count = 40.
# Item code are at most 4 characters long. Anything over that is ignored.

[normal]
runes = ["r07", "r08", "r09"]

[nightmare]
runes = ["r16", "r17", "r18"]

[hell]
runes = ["r21", "r22", "r23"])toml"sv;


static constexpr D2RL::PluginFlags PatchingSampleFlags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks;


UNITS_GetPlayerDataFn UNITS_GetPlayerData{};
QUESTS_StateDebugFn QUESTS_StateDebug{};
SUNIT_IterateUnitsOfTypeFn SUNIT_IterateUnitsOfType{};
ACT5Q2_UnitIterate_UpdateQuestStateFlagsFn ACT5Q2_UnitIterate_UpdateQuestStateFlags{};
QUESTS_NPCActivateSpeechesFn QUESTS_NPCActivateSpeeches{};
QUESTRECORD_GetQuestStateFn QUESTRECORD_GetQuestState{};
QUESTRECORD_SetQuestStateFn QUESTRECORD_SetQuestState{};
QUESTS_UnitIterateFn QUESTS_UnitIterate{};
ACT5Q2_UnitIterate_StatusCyclerExFn ACT5Q2_UnitIterate_StatusCyclerEx{};
QUESTS_CreateItemFn QUESTS_CreateItem{};
QUESTRECORD_ClearQuestStateFn QUESTRECORD_ClearQuestState{};
QUESTRECORD_ResetIntermediateStateFlagsFn QUESTRECORD_ResetIntermediateStateFlags{};
QUESTS_AddPlayerGUIDFn QUESTS_AddPlayerGUID{};
SUNIT_GetClientFromPlayerFn SUNIT_GetClientFromPlayer{};
D2GAME_PACKETS_SendPacketFn D2GAME_PACKETS_SendPacket{};
ACT5Q2_Callback11_ScrollMessageFn ACT5Q2_Callback11_ScrollMessageOriginal{};


constexpr size_t kMaxItems = 40;


// Forward declarations
void __fastcall ACT5Q2_Callback11_ScrollMessageHook(D2QuestDataStrc* pQuestData, D2QuestArgStrc* pQuestArg);


// Main plugin class
class QualKehk
{
public:
	bool Install(const D2RL::PluginContext* pContext) noexcept
	{
		if (!LoadConfig(pContext))
		{
			return false;
		}

		if (!m_Enabled)
		{
			pContext->LogInfo("Config settings loaded, but plugin is disabled - no hooks installed.");
			return true;
		}

		if (!D2_GET_FUNCTION(pContext, UNITS_GetPlayerData) ||
			!D2_GET_FUNCTION(pContext, QUESTS_StateDebug) ||
			!D2_GET_FUNCTION(pContext, SUNIT_IterateUnitsOfType) ||
			!D2_GET_FUNCTION(pContext, ACT5Q2_UnitIterate_UpdateQuestStateFlags) ||
			!D2_GET_FUNCTION(pContext, QUESTS_NPCActivateSpeeches) ||
			!D2_GET_FUNCTION(pContext, QUESTRECORD_GetQuestState) ||
			!D2_GET_FUNCTION(pContext, QUESTRECORD_SetQuestState) ||
			!D2_GET_FUNCTION(pContext, QUESTS_UnitIterate) ||
			!D2_GET_FUNCTION(pContext, ACT5Q2_UnitIterate_StatusCyclerEx) ||
			!D2_GET_FUNCTION(pContext, QUESTS_CreateItem) ||
			!D2_GET_FUNCTION(pContext, QUESTRECORD_ClearQuestState) ||
			!D2_GET_FUNCTION(pContext, QUESTRECORD_ResetIntermediateStateFlags) ||
			!D2_GET_FUNCTION(pContext, QUESTS_AddPlayerGUID) ||
			!D2_GET_FUNCTION(pContext, SUNIT_GetClientFromPlayer) ||
			!D2_GET_FUNCTION(pContext, D2GAME_PACKETS_SendPacket) ||
			!D2_HOOK_FUNCTION(pContext, ACT5Q2_Callback11_ScrollMessage))
		{
			pContext->LogError("Failure in loading / hooking one or more function pointers.");
			return false;
		}

		pContext->LogInfo("Plugin enabled - hooks installed.");
		return true;
	}

	bool IsEnabled() const { return m_Enabled; }
	auto& GetItemDropCodes(uint8_t difficulty) const { return m_ItemDropCodes[difficulty]; }

private:
	bool LoadConfig(const D2RL::PluginContext* pContext)
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

		if (table.failed())
		{
			pContext->LogInfo("Couldn't read Qual-Kehk config file, loading default settings.");
			table = toml::parse(kTOMLFile);
			// This should be an assert
			if (table.failed())
			{
				pContext->LogError("Couldn't read default Qual-Kehk config file.");
				return false;
			}
		}

		auto config = table["qualkehk"];
		m_Enabled = config["enabled"].value_or(false);

		auto fillValues = [this, pContext, &table](const char* pTable, uint32_t index) noexcept -> bool
			{
				// Item codes are at most 4 characters long
				union StrToCode
				{
					uint32_t itemCode;
					char codeStr[4];
				} strToCode;
				uint32_t currChar = 0;

				auto lootTable = table[pTable];
				auto& dropCodes = m_ItemDropCodes[index];

				auto runes = lootTable["runes"].as_array();
				if (runes && runes->size() > 0)
				{
					dropCodes.reserve(std::min(runes->size(), kMaxItems));
					for (auto it = runes->begin(); it != runes->end() && dropCodes.size() < kMaxItems; it++)
					{
						strToCode.itemCode = '    ';
						currChar = 0;
						auto sv = it->value_or(""sv);
						if (sv.length() > 4)
						{
							// Invalid item code
							continue;
						}

						for (auto s : sv)
						{
							strToCode.codeStr[currChar++] = s;
						}

						dropCodes.push_back(strToCode.itemCode);
					}
				}

				if (dropCodes.size() == 0)
				{
					pContext->LogError("No items set or invalid drop table.");
					return false;
				}

				return true;
			};

		return fillValues("normal", 0) && fillValues("nightmare", 1) && fillValues("hell", 2);
	}

private:
	std::vector<uint32_t> m_ItemDropCodes[3];
	bool m_Enabled{};
};


QualKehk g_QualKehk;


void __fastcall ACT5Q2_Callback11_ScrollMessageHook(D2QuestDataStrc* pQuestData, D2QuestArgStrc* pQuestArg)
{
	D2Act5Quest2Strc* pQuestDataEx = &pQuestData->tA5Q2;
	if (pQuestArg->nNPCNo != 0x203)
	{
		return;
	}

	auto playerData = reinterpret_cast<uint64_t>(UNITS_GetPlayerData(pQuestArg->pPlayer));
	auto pQuestFlags = reinterpret_cast<D2BitBufferStrc*>(*reinterpret_cast<uint64_t*>(playerData + 0x40 + pQuestData->pGame->nDifficulty * 8ull));

	uint32_t questFilter = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(pQuestData) + 0x2f0);

	if (pQuestArg->nMessageIndex == 20096)
	{
		pQuestDataEx->bQualKehkActivated = 1;
		QUESTS_StateDebug(pQuestData, 2, __FILE__, __LINE__);
		SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, nullptr, ACT5Q2_UnitIterate_UpdateQuestStateFlags);
		QUESTS_NPCActivateSpeeches(pQuestArg->pGame, pQuestArg->pPlayer, pQuestArg->pTarget);
	}
	else if (pQuestArg->nMessageIndex == 20110)
	{
		if (QUESTRECORD_GetQuestState(pQuestFlags, questFilter, 1) == 1)
		{
			if (QUESTRECORD_GetQuestState(pQuestFlags, questFilter, 13))
			{
				if (pQuestData->fState != 5)
				{
					QUESTS_StateDebug(pQuestData, 5, __FILE__, __LINE__);
					auto fn = (*reinterpret_cast<QUESTSEQFILTER*>(reinterpret_cast<uint64_t>(pQuestData) + 0x308))(pQuestData);

					pQuestData->dwFlags &= 0xFFFFFF00;
					QUESTS_UnitIterate(pQuestData, 13, nullptr, ACT5Q2_UnitIterate_StatusCyclerEx, false);
				}
				*reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(pQuestData) + 0x280) = 0;
			}

			auto& items = g_QualKehk.GetItemDropCodes(pQuestData->pGame->nDifficulty);
			bool bRewardGranted = false;
			for (int32_t i = 0; i < items.size(); ++i)
			{
				if (QUESTS_CreateItem(pQuestArg->pGame, pQuestArg->pPlayer, items[i], 0, 2, 1))
				{
					bRewardGranted = true;
				}
			}

			if (bRewardGranted)
			{
				QUESTRECORD_SetQuestState(pQuestFlags, questFilter, 0);
				QUESTRECORD_ClearQuestState(pQuestFlags, questFilter, 1);
				QUESTRECORD_ResetIntermediateStateFlags(pQuestFlags, questFilter);

				auto pPlayerGUIDs = reinterpret_cast<D2QuestGUIDStrc*>(reinterpret_cast<uint64_t>(pQuestData) + 0x1e8);
				QUESTS_AddPlayerGUID(pPlayerGUIDs, (pQuestArg->pPlayer ? pQuestArg->pPlayer->dwUnitId : -1));

				D2ClientStrc* pClient = SUNIT_GetClientFromPlayer(pQuestArg->pPlayer, __FILE__, __LINE__);

				uint32_t packet5D = 0x0002205d;
				D2GAME_PACKETS_SendPacket(pClient, &packet5D);
			}

			QUESTS_NPCActivateSpeeches(pQuestArg->pGame, pQuestArg->pPlayer, pQuestArg->pTarget);
		}
	}
}



static constexpr D2RL::PluginInfo PatchingSampleInfo
{
	.infoSize = D2RL::PluginInfoSize,
	.abiVersion = D2RL_PLUGIN_ABI_VERSION,
	.id = "qualkehk",
	.name = "Qual-Kehk Plugin",
	.version = "0.0.8",
	.author = "ArchVile",
	.description = "Allows control over how many runes Qual-Kehk's quest gives, as well as which ones, per difficulty.",
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

	if (!g_QualKehk.Install(pContext))
	{
		pContext->LogError("Qual-Kehk plugin failed.");
		return false;
	}

	if (g_QualKehk.IsEnabled())
	{
		pContext->LogInfo("Qual-Kehk hook installed.");
	}
	else
	{
		pContext->LogInfo("Qual-Kehk hook not installed.");
	}

	return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {}