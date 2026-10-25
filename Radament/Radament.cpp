#include "Common/D2Functions.h"


constexpr auto kTOMLFile =
R"toml(# Radament
# Enables custom skill point rewards after reading Radament's skill book, per difficulty.

[radament]
# Master switch - set to false to prevent hooks from being installed.
enabled = true

# num_skills is the number of skill points given after reading the skill book.

[normal]
num_skills = 1

[nightmare]
num_skills = 2

[hell]
num_skills = 3)toml"sv;


static constexpr D2RL::PluginFlags PatchingSampleFlags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks;


UNITS_GetPlayerDataFn UNITS_GetPlayerData{};
QUESTRECORD_ClearQuestStateFn QUESTRECORD_ClearQuestState{};
STATLIST_AddUnitStatFn STATLIST_AddUnitStat{};
QUESTS_SendLogUpdateFn QUESTS_SendLogUpdate{};
SKILLITEM_pSpell10_BookOfSkillFn SKILLITEM_pSpell10_BookOfSkillOriginal{};


// Forward declarations
int32_t __fastcall SKILLITEM_pSpell10_BookOfSkillHook(D2GameStrc* pGame, D2UnitStrc* pUnit, D2UnitStrc* pItem);


// Main plugin class
class Radament
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
			!D2_GET_FUNCTION(pContext, QUESTRECORD_ClearQuestState) ||
			!D2_GET_FUNCTION(pContext, STATLIST_AddUnitStat) ||
			!D2_GET_FUNCTION(pContext, QUESTS_SendLogUpdate) ||
			!D2_HOOK_FUNCTION(pContext, SKILLITEM_pSpell10_BookOfSkill))
		{
			pContext->LogError("Failure in loading / hooking one or more function pointers.");
			return false;
		}

		pContext->LogInfo("Plugin enabled - hooks installed.");
		return true;
	}

	auto IsEnabled() const { return m_Enabled; }
	auto GetNumSkills(uint8_t difficulty) const { return m_NumSkills[difficulty]; }

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
			pContext->LogInfo("Couldn't read Radament config file, loading default settings.");
			table = toml::parse(kTOMLFile);
			// This should be an assert
			if (table.failed())
			{
				pContext->LogError("Couldn't read default Radament config file.");
				return false;
			}
		}

		auto config = table["radament"];
		m_Enabled = config["enabled"].value_or(false);

		auto fillValues = [this, pContext, &table](const char* pTable, uint32_t index) noexcept
			{
				auto lootTable = table[pTable];

				m_NumSkills[index] = lootTable["num_skills"].value_or(1);
			};

		fillValues("normal", 0);
		fillValues("nightmare", 1);
		fillValues("hell", 2);

		return true;
	}

private:
	int32_t m_NumSkills[3]{};
	bool m_Enabled{};
};


Radament g_Radament;


int32_t __fastcall SKILLITEM_pSpell10_BookOfSkillHook(D2GameStrc* pGame, D2UnitStrc* pUnit, D2UnitStrc* pItem)
{
	auto playerData = reinterpret_cast<uint64_t>(UNITS_GetPlayerData(pUnit));
	auto pQuestFlags = reinterpret_cast<D2BitBufferStrc*>(*reinterpret_cast<uint64_t*>(playerData + 0x40 + pGame->nDifficulty * 8ull));

	QUESTRECORD_ClearQuestState(pQuestFlags, 9, 5);
	STATLIST_AddUnitStat(pUnit, 5, g_Radament.GetNumSkills(pGame->nDifficulty));
	QUESTS_SendLogUpdate(pUnit, 8);

	return 1;
}


static constexpr D2RL::PluginInfo PatchingSampleInfo
{
	.infoSize = D2RL::PluginInfoSize,
	.abiVersion = D2RL_PLUGIN_ABI_VERSION,
	.id = "radament",
	.name = "Radament Plugin",
	.version = "0.1.0",
	.author = "ArchVile",
	.description = "Allows control over how many skill points reading Radament's book awards, per difficulty.",
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

	if (!g_Radament.Install(pContext))
	{
		pContext->LogError("Radament plugin failed.");
		return false;
	}

	if (g_Radament.IsEnabled())
	{
		pContext->LogInfo("Radament hook installed.");
	}
	else
	{
		pContext->LogInfo("Radament hook not installed.");
	}

	return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {}