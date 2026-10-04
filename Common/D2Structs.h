#pragma once


#include "CommonDefs.h"


// Structures mapped to D2R versions - WIP


struct D2SeedStrc
{
	union
	{
		struct
		{
			uint32_t nLowSeed;
			uint32_t nHighSeed;
		};
		uint64_t lSeed;
	};
};


struct D2GameStrc
{
	uint8_t pad0[0x68];
	uint8_t difficulty;
	uint8_t pad00;
	uint8_t expansion;
	uint8_t pad1[0x104];
	uint32_t gameFrame;
	uint8_t pad2[0x2064];
	void* pQuestControl; // D2QuestInfoStrc
};


struct D2UnitStrc
{
	uint32_t unitType;
	uint8_t pad0[32];
	D2SeedStrc seed;
};



struct D2QuestDataStrc
{
	int32_t nQuestNo;
	/*D2GameStrc*/void* pGame;
	uint8_t nActNo;
	bool bNotIntro;
	bool bActive;
	uint8_t fLastState;
	uint8_t fState;
	char nInitNo;
	uint16_t dw0E;
	int32_t nSeqId;
	uint32_t dwFlags;
	//void* pQuestDataEx;
	//D2QuestGUIDStrc tPlayerGUIDs;
	//QUESTCALLBACK pfCallback[15];
	///*D2NPCMessageTableStrc*/void* pNPCMessages;
	//int32_t nQuestFilter;
	//QUESTSTATUS pfStatusFilter;
	//QUESTACTIVE pfActiveFilter;
	//QUESTSEQFILTER pfSeqFilter;
	//void* pPrev;
};


// Seems to derive from the base struct now?
struct D2Act4Quest3Strc : D2QuestDataStrc
{
	short nHellforgeObjectMode;
	uint8_t bCainActivated;
	uint8_t bRewardDropsPending;
	uint8_t bSoulstoneSmashed;
	uint8_t pad0x05[3];
	int nGemDropTier;
	int nHits;
	int nHellforgeHammersInGame;
	int nPlayersInAct;
	int unused0x18;
	uint8_t bSoulstoneAcquired;
	uint8_t pad0x1D[3];
};