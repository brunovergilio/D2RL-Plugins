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
	uint8_t nDifficulty;
	uint8_t pad00;
	uint8_t expansion;
	uint8_t pad1[0x104];
	uint32_t gameFrame;
	uint8_t pad2[0x2064];
	void* pQuestControl; // D2QuestInfoStrc
};


struct D2UnitStrc
{
	uint32_t dwUnitType;
	int32_t nClassId;
	uint32_t dwUnitId;
	uint8_t pad0[24];
	D2SeedStrc seed;
};

struct D2QuestGUIDStrc
{
	uint32_t nPlayerGUIDs[32];
	uint16_t nPlayerCount;
	uint8_t pad0x82[2];
};

struct D2CoordStrc
{
	int nX;
	int nY;
};

struct D2Act4Quest3Strc
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

struct D2Act5Quest2Strc
{
	D2QuestGUIDStrc tPlayerGUIDs;
	uint8_t bQualKehkActivated;
	uint8_t unused0x85;
	uint8_t bWussiesSpawned[3];
	uint8_t pad0x89[3];
	D2CoordStrc pWussieCoords[3];
	int nSpawnedWussies;
	int nKilledWussies;
	int nFreedWussies;
	int unk0xB0[15];
	int nPortalGUIDs[3];
	uint8_t bPortalSpawned[3];
	uint8_t pad0xFB;
	int nPortalUpdateInvocations[3];
	uint8_t unk0x108[3];
	uint8_t bChangeToSpecialObjectMode[3];
	uint8_t unk0x10E[3];
	uint8_t pad0x111[3];
	int unk0x114[3];
	int nFreedWussieUnitGUIDs[15];
	int nWussiesInRangeToDoor;
};



struct D2QuestDataStrc;
struct D2QuestArgStrc;
using D2BitBufferStrc = void;
using D2PlayerDataStrc = void;
using D2ClientStrc = void;
using D2NPCMessageTableStrc = void;

using QUESTINIT = void(__fastcall*)(D2QuestDataStrc*);
using QUESTCALLBACK = void(__fastcall*)(D2QuestDataStrc*, D2QuestArgStrc*);
using QUESTSTATUS = bool(__fastcall*)(D2QuestDataStrc*, D2UnitStrc*, D2BitBufferStrc*, D2BitBufferStrc*, uint8_t*);
using QUESTUPDATE = bool(__fastcall*)(D2GameStrc*, D2QuestDataStrc*);
using QUESTACTIVE = bool(__fastcall*)(D2QuestDataStrc*, int32_t, D2UnitStrc*, D2BitBufferStrc*, D2UnitStrc*);
using QUESTSEQ = int32_t(__fastcall*)(D2QuestDataStrc*, D2UnitStrc*, D2BitBufferStrc*, D2BitBufferStrc*, uint8_t*);
using QUESTSEQFILTER = bool(__fastcall*)(D2QuestDataStrc*);


struct D2QuestDataStrc
{
	int32_t nQuestNo;
	D2GameStrc* pGame;
	uint8_t nActNo;
	bool bNotIntro;
	bool bActive;
	uint8_t fLastState;
	uint8_t fState;
	char nInitNo;
	uint16_t dw0E;
	int32_t nSeqId;
	uint32_t dwFlags;
	union
	{
		uint8_t pad[456];
		D2Act4Quest3Strc tA4Q3;
		D2Act5Quest2Strc tA5Q2;
	};
	D2QuestGUIDStrc tPlayerGUIDs;
	QUESTCALLBACK pfCallback[15];
	D2NPCMessageTableStrc* pNPCMessages;
	int32_t nQuestFilter;
	QUESTSTATUS pfStatusFilter;
	QUESTACTIVE pfActiveFilter;
	QUESTSEQFILTER pfSeqFilter;
	void* pPrev;
};


struct D2QuestArgStrc
{
	D2GameStrc* pGame;
	int32_t nEvent;
	D2UnitStrc* pTarget;
	D2UnitStrc* pPlayer;
	uint32_t unk0x10;
	union
	{
		struct
		{
			/*D2TextHeaderStrc*/void* pTextControl;
			uint32_t dw18;
		};
		struct
		{
			int32_t nNPCNo;
			uint32_t unk0x16;
			int32_t nMessageIndex;
			uint32_t unk0x1A;
		};
		struct
		{
			int32_t nOldLevel;
			int32_t nNewLevel;
		};
	};
};