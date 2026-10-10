#include "Common/D2Functions.h"



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


constexpr uint32_t kAct5Q2RuneCodes[] =
{
	' 70r', ' 80r', ' 90r', ' 71r', ' 81r', ' 91r', ' 72r', ' 82r', ' 92r'
};


void __fastcall ACT5Q2_Callback11_ScrollMessageHook(D2QuestDataStrc* pQuestData, D2QuestArgStrc* pQuestArg)
{
	D2Act5Quest2Strc* pQuestDataEx = &pQuestData->tA5Q2;
	if (pQuestArg->nNPCNo != 0x203)
	{
		return;
	}

	auto playerData = reinterpret_cast<uint64_t>(UNITS_GetPlayerData(pQuestArg->pPlayer));
	D2BitBufferStrc* pQuestFlags = reinterpret_cast<D2BitBufferStrc*>(playerData + 0x40 + pQuestData->pGame->nDifficulty * 8);

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

			int32_t nRewards = 0;
			if (QUESTRECORD_GetQuestState(pQuestFlags, 0x24, 6))
			{
				nRewards = 2;
			}
			else if (QUESTRECORD_GetQuestState(pQuestFlags, 0x24, 7))
			{
				nRewards = 1;
			}
			else
			{
				nRewards = 3;
			}

			if (nRewards > 0)
			{
				bool bRewardGranted = false;
				for (int32_t i = 0; i < std::size(kAct5Q2RuneCodes); ++i)
				{
					if (QUESTS_CreateItem(pQuestArg->pGame, pQuestArg->pPlayer, kAct5Q2RuneCodes[i], 0, 2, 1))
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
	.version = "0.0.1",
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
		pContext->LogError("Qual-Kehk plugin failed.");
		return false;
	}

	return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {}