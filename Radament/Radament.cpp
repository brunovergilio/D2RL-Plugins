#include "Common/D2Functions.h"


// This plugin was supposed to patch the new SKILLITEM_pSpell10_BookOfSkill() function, which is very simple; however, for some reason,
// the code wouldn't work, and instead of adding more skill points to what was existing, it would only change the UI briefly, before
// a single assignment would revert back to the original skill points available. Therefore, I had to place a mid-function hook, and
// handle the addition of skill points through assembly. Not ideal, but for now I'll stick to this.


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


constexpr D2RL::PluginFlags PatchingSampleFlags = D2RL::PluginFlags::Server | D2RL::PluginFlags::NativeHooks;

constexpr ptrdiff_t kRVAToPatch = 0x580782;

constexpr std::initializer_list<uint8_t> kExpectedBytes =
{
	0x45, 0x33, 0xc9,
	0x48, 0x8b, 0xcf,
	0x41, 0x8d, 0x51, 0x05,
	0x45, 0x8d, 0x41, 0x01,
	0xe8, 0x5b, 0x2d, 0xd7, 0xff
};


extern "C"
{
	// ASM hook
	void RadamentASMHook();

	// Where to go back to once the hook finishes
	ptrdiff_t g_JumpBackAddress = 0x580795;

	// This will store the global function address - initially set to the RVA
	ptrdiff_t g_STATLIST_AddUnitStatAddress = kSTATLIST_AddUnitStatRVA;

	// This will store Radament::m_SkillPoints
	int32_t* g_pSkillPoints = nullptr;
}


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

		uint8_t patch[19] = { 0xff, 0x25 };
		*(uint64_t*)&patch[6] = (uint64_t)RadamentASMHook;
		for (auto i = 14; i < 19; i++)
		{
			patch[i] = 0x90;
		}

		if (!pContext->CheckExpectedBytes(kRVAToPatch, kExpectedBytes.data(), uint32_t(kExpectedBytes.size())))
		{
			return false;
		}

		if (!pContext->PatchBytes(kRVAToPatch, kExpectedBytes.data(), uint32_t(kExpectedBytes.size()), patch, sizeof(patch)))
		{
			return false;
		}

		// Append the base exe to the jump back address
		g_JumpBackAddress += pContext->exeBase;

		// Append the base exe address to the variable accessed in asm
		g_STATLIST_AddUnitStatAddress += pContext->exeBase;
		
		// Assign the skill point value array to the variable accessed in asm
		g_pSkillPoints = m_SkillPoints;

		pContext->LogInfo("Plugin enabled - hooks installed.");
		return true;
	}

	auto IsEnabled() const { return m_Enabled; }
	auto GetNumSkills(uint8_t difficulty) const { return m_SkillPoints[difficulty]; }

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

				m_SkillPoints[index] = lootTable["num_skills"].value_or(1);
			};

		fillValues("normal", 0);
		fillValues("nightmare", 1);
		fillValues("hell", 2);

		return true;
	}

private:
	int32_t m_SkillPoints[3]{};
	bool m_Enabled{};
};


Radament g_Radament;


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