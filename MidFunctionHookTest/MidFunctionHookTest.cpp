#include "Common/D2Functions.h"


static constexpr D2RL::PluginFlags PatchingSampleFlags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks;


UNITS_GetPlayerDataFn UNITS_GetPlayerData{};
QUESTRECORD_GetQuestStateFn QUESTRECORD_GetQuestState{};
QUESTRECORD_ClearQuestStateFn QUESTRECORD_ClearQuestState{};
QUESTRECORD_ResetIntermediateStateFlagsFn QUESTRECORD_ResetIntermediateStateFlags{};
QUESTS_GetQuestDataFn QUESTS_GetQuestData{};
ACT4Q3_InitQuestDataFn ACT4Q3_InitQuestData{};
QUESTS_StatusCyclerExFn QUESTS_StatusCyclerEx{};
MONSTERUNIQUE_GetBossHcIdxFn MONSTERUNIQUE_GetBossHcIdx{};
D2GAME_PACKETS_SendPacketFn D2GAME_PACKETS_SendPacket{};
SUNIT_GetClientFromPlayerFn SUNIT_GetClientFromPlayer{};
QUESTS_RefreshStatusFn QUESTS_RefreshStatus{};
QUESTRECORD_SetQuestStateFn QUESTRECORD_SetQuestState{};
GAME_ForEachIngameClientFn GAME_ForEachIngameClient{};
D2GAME_PACKETS_SendPacket0x26_ServerMessageFn D2GAME_PACKETS_SendPacket0x26_ServerMessage{};
D2GAME_PACKETS_SendPacketWithSizeFn D2GAME_PACKETS_SendPacketWithSize{};
UNITS_GetUnitIdFn UNITS_GetUnitId{};
SKILLITEM_pSpell10_BookOfSkillFn SKILLITEM_pSpell10_BookOfSkill{};
QUESTS_SendLogUpdateFn QUESTS_SendLogUpdate{};
STATLIST_AddUnitStatFn STATLIST_AddUnitStat{};

extern "C"
{
	void hkTreasureHook();
	void __fastcall HookHandler(D2GameStrc* pGame, D2UnitStrc* pMonster, D2UnitStrc* pPlayer)
	{
		static bool b = false;
		if (!pPlayer || !pGame || !pMonster)
		{
			return;
		}

		if (MONSTERUNIQUE_GetBossHcIdx(pMonster) != 42)
		{
			return;
		}

		for (auto unit = 0; unit < 3; unit++)
		{
			for (auto i = 0; i < 8; i++)
			{
				D2GSPacketSrv26 pkt26{};
				pkt26.nHeader = 0x26;
				pkt26.nMessageType = i;
				pkt26.nUnitType = unit;
				pkt26.nMessageColor = i;
				//pkt26.dwUnitGUID = UNITS_GetUnitId(pPlayer, __FILE__, __LINE__);
				snprintf(pkt26.szMessage, sizeof(pkt26.szMessage), "Player killed shenk with message %d!", i);
				snprintf(pkt26.szName, sizeof(pkt26.szName), "Tester");
				GAME_ForEachIngameClient(pGame, D2GAME_PACKETS_SendPacket0x26_ServerMessage, &pkt26, nullptr);
				pkt26.szName[0] = 0;
				GAME_ForEachIngameClient(pGame, D2GAME_PACKETS_SendPacket0x26_ServerMessage, &pkt26, nullptr);
			}
		}

		//D2GAME_PACKETS_SendPacket0x26_ServerMessage(SUNIT_GetClientFromPlayer(pPlayer, __FILE__, __LINE__), &pkt);

		//auto pQuestData = QUESTS_GetQuestData(pGame, 27);
		//if (pQuestData)
		//{
		//	auto playerData = reinterpret_cast<uint64_t>(UNITS_GetPlayerData(pPlayer));
		//	auto pQuestFlags = reinterpret_cast<D2BitBufferStrc*>(*reinterpret_cast<uint64_t*>(playerData + 0x40 + pGame->nDifficulty * 8ull));

		//	QUESTRECORD_ResetIntermediateStateFlags(pQuestFlags, 27);
		//	int flags[] = { 0, 1, 12, 13, 14, 15 };
		//	for (auto f : flags)
		//	{
		//		QUESTRECORD_ClearQuestState(pQuestFlags, 27, f);
		//	}
		//	memset(&pQuestData->tA4Q3, 0, sizeof(pQuestData->tA4Q3));
		//	ACT4Q3_InitQuestData(pQuestData);

		//	pQuestData->dwFlags &= 0xFFFFFF00;
		//	pQuestData->fState = 1;
		//	pQuestData->fLastState = 2;

		//	D2ClientStrc* pClient = SUNIT_GetClientFromPlayer(pPlayer, __FILE__, __LINE__);

		//	D2GSPacketSrv5D packet5D{};
		//	packet5D.nState = pQuestData->fLastState;
		//	QUESTS_RefreshStatus(pQuestData, &packet5D.nState, pQuestFlags);

		//	packet5D.nHeader = 0x5D;
		//	packet5D.nQuestId = 27;
		//	packet5D.nFlags = (uint8_t)pQuestData->dwFlags;
		//	packet5D.field_4 = 0;

		//	D2GAME_PACKETS_SendPacket(pClient, reinterpret_cast<uint32_t*>(&packet5D));
		//}
	}

	ptrdiff_t g_TargetCallAddress = 0x397290;
	ptrdiff_t g_JumpBackAddress = 0x44143d;
}


uint8_t __fastcall QUESTS_GiveRadamentRewardFromBookHook(D2GameStrc* pGame, D2UnitStrc* pUnit)
{
	auto playerData = reinterpret_cast<uint64_t>(UNITS_GetPlayerData(pUnit));
	auto pQuestFlags = reinterpret_cast<D2BitBufferStrc*>(*reinterpret_cast<uint64_t*>(playerData + 0x40 + pGame->nDifficulty * 8ull));

	uint32_t numSkills = pGame->nDifficulty + 2;

	QUESTRECORD_ClearQuestState(pQuestFlags, 9, 5);
	STATLIST_AddUnitStat(pUnit, 5, numSkills);
	QUESTS_SendLogUpdate(pUnit, 8);

	return 1;
}


constexpr ptrdiff_t kRVAToPatch = 0x44142b;

constexpr std::initializer_list<uint8_t> kExpectedBytes =
{
	0x0f, 0xb6, 0x8e, 0x06, 0x01, 0x00, 0x00,
	0x44, 0x8b, 0xc7,
	0x0f, 0xb7, 0xd5,
	0xe8, 0x53, 0x5e, 0xf5, 0xff
};


static constexpr D2RL::PluginInfo PatchingSampleInfo
{
	.infoSize = D2RL::PluginInfoSize,
	.abiVersion = D2RL_PLUGIN_ABI_VERSION,
	.id = "hooktest",
	.name = "Hook test Plugin",
	.version = "0.0.1",
	.author = "ArchVile",
	.description = "Testing mid-function hooks.",
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
		!D2_GET_FUNCTION(pContext, STATLIST_AddUnitStat) ||
		!D2_GET_FUNCTION(pContext, QUESTS_SendLogUpdate) ||
		!D2_GET_FUNCTION(pContext, QUESTS_RefreshStatus) ||
		!D2_GET_FUNCTION(pContext, UNITS_GetUnitId) ||
		!D2_GET_FUNCTION(pContext, GAME_ForEachIngameClient) ||
		!D2_GET_FUNCTION(pContext, D2GAME_PACKETS_SendPacket0x26_ServerMessage) ||
		!D2_GET_FUNCTION(pContext, D2GAME_PACKETS_SendPacketWithSize) ||
		!D2_GET_FUNCTION(pContext, SUNIT_GetClientFromPlayer) ||
		!D2_GET_FUNCTION(pContext, D2GAME_PACKETS_SendPacket) ||
		!D2_GET_FUNCTION(pContext, MONSTERUNIQUE_GetBossHcIdx) ||
		!D2_GET_FUNCTION(pContext, QUESTS_StatusCyclerEx) ||
		!D2_GET_FUNCTION(pContext, ACT4Q3_InitQuestData) ||
		!D2_GET_FUNCTION(pContext, QUESTS_GetQuestData) ||
		!D2_GET_FUNCTION(pContext, QUESTRECORD_SetQuestState) ||
		!D2_GET_FUNCTION(pContext, QUESTRECORD_GetQuestState) ||
		!D2_GET_FUNCTION(pContext, QUESTRECORD_ClearQuestState) ||
		!D2_GET_FUNCTION(pContext, QUESTRECORD_ResetIntermediateStateFlags))
	{
		pContext->LogError("Failure in loading / hooking one or more function pointers.");
		return false;
	}


	uint8_t patch[18] = { 0xff, 0x25 };
	*(uint64_t*)&patch[6] = (uint64_t)hkTreasureHook;
	patch[15] = patch[16] = patch[17] = 0x90;

	g_JumpBackAddress += pContext->exeBase;
	g_TargetCallAddress += pContext->exeBase;

	if (!pContext->CheckExpectedBytes(kRVAToPatch, kExpectedBytes.data(), uint32_t(kExpectedBytes.size())))
	{
		return false;
	}
	
	if (!pContext->PatchBytes(kRVAToPatch, kExpectedBytes.data(), uint32_t(kExpectedBytes.size()), patch, 18))
	{
		return false;
	}

	return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {}