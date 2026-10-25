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

using D2MonStatsTxt = uint8_t;
using D2AiControlStrc = uint8_t;
using D2AiParamStrc = uint8_t;
using D2MonsterInteractStrc = uint8_t;
using D2AiControlStrc = uint8_t;


struct D2UnkMonsterDataStrc
{
	int32_t unk0x00;
	int32_t nLifePercentage;
	int32_t unk0x08;
	uint8_t nCount;
	uint8_t pad0x0D[3];
};

struct D2MonsterDataStrc
{
	D2MonStatsTxt* pMonstatsTxt;
	uint8_t nComponent[16];
	uint16_t wNameSeed;
	uint8_t nTypeFlag;
	uint8_t nLastAnimMode;
	uint32_t dwDurielFlag;
	uint8_t nMonUmod[10];
	uint16_t wBossHcIdx;
	D2AiControlStrc* pAiControl;
	union
	{
		D2AiParamStrc* pAiParam;
		struct Unicode* wszMonName;
	};
	D2MonsterInteractStrc* pMonInteract;
	uint32_t unk0x34;
	uint32_t unk0x38;
	D2UnkMonsterDataStrc unk0x3C;
	uint32_t unk0x4C;
	uint32_t pVision;
	uint32_t dwAiState;
	uint32_t dwTxtLevelNo;
	uint16_t wSummonerFlag;
	uint8_t pad0x5E[2];
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
			int16_t nNPCNo;
			uint16_t unk0x16;
			uint32_t pad0;
			int16_t nMessageIndex;
			uint16_t unk0x1A;
		};
		struct
		{
			int32_t nOldLevel;
			int32_t nNewLevel;
		};
	};
};


struct D2GSPacketSrv5D
{
	uint8_t nHeader;
	uint8_t nQuestId;
	uint8_t nFlags;
	uint8_t nState;
	uint16_t field_4;
};


#pragma pack(push, 1)
struct D2GSPacketSrv26 {
	uint8_t  nHeader;         // 0x00 (0x26)
	uint8_t  nMessageType;    // 0x01
	uint8_t  nLang;           // 0x02
	uint8_t  nUnitType;       // 0x03
	uint32_t dwUnitGUID;      // 0x04
	uint8_t  nMessageColor;   // 0x08
	uint8_t  nNameColor;      // 0x09
	char     szName[61];      // 0x0A (Max 60 chars + null)
	char     szMessage[256];  // 0x47 (Max 255 chars + null)
};

struct D2GSPacketSrv5A		//size of 0x28
{
	uint8_t nHeader;			//0x00
	uint8_t nType;				//0x01
	union
	{
		struct //nType < 0x13
		{
			uint8_t nColor;			//0x02
			uint32_t dwParam;			//0x03
			uint8_t nParam;			//0x07
			union
			{
				char szText[32];	//0x08
				char szTextData[2][16];	//0x08
			};
		};

		struct	//nType 0x13
		{
			uint32_t dwSkill;			//0x02
			uint32_t dwEndFrame;		//0x06
			uint8_t nExtraData2[30];	//0x0A
		};

		struct	//nType 0x14
		{
			uint8_t nGroup;			//0x02
			uint32_t dwGroupEndFrame;	//0x03
			uint8_t nExtraData3[33];	//0x07
		};

		struct //nType 0x16
		{
			int32_t nItemId;			//0x02
			int32_t nPlayerId;			//0x06
			uint8_t nExtraData4[30];	//0x0A
		};

		struct //nType 0x17
		{
			uint8_t nDamageColor;		//0x02
			uint32_t dwDamage;			//0x03
			int32_t nDefenderGUID;		//0x07
			int32_t nDefenderType;		//0x0B
			uint8_t nExtraData5[25];	//0x0F
		};

		struct //nType 0x18
		{
			uint8_t nUnitType;			//0x02
			int32_t nItemGUID;			//0x03
			uint8_t nAction;			//0x07
			int32_t nPosX;				//0x08
			int32_t nPosY;				//0x0C
			uint8_t nExtraData6[24];	//0x10
		};

		struct //nType 0x19
		{
			uint8_t nDefenderUnitType;	//0x02
			int32_t nDefenderUnitGUID;	//0x03
			uint8_t nAttackerUnitType;	//0x07
			int32_t nAttackerUnitGUID;	//0x08
			uint8_t nAttackMessageColor;//0x0C
			uint8_t nExtraData7[27];	//0x0D
		};

		struct //nType 0x1A
		{
			int32_t nFrames;			//0x02
			uint8_t nExtraData8[34];	//0x06
		};

		struct //nType 0x1B
		{
			int32_t nCount;			//0x02
			uint8_t nExtraData9[34];	//0x06
		};
	};
};
#pragma pack(pop)