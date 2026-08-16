/*============================================================================
Contents   :  [game_progress.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/15
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cstdlib>

#include "game_progress.h"

struct LevelReward
{
	bool  grants_slot = false;
	bool  offers_element = false;
	bool  offers_stat = false;
	float charge_time_mul = 1.0f;   
};

static constexpr LevelReward g_LevelPlan[PLAYER_LEVEL_MAX + 1] =
{
	{},                                                                       // 0 
	{},                                                                       // 1
	{.grants_slot = true, .offers_element = true, .charge_time_mul = 0.85f }, // 2
	{.offers_stat = true },                                                   // 3
	{.offers_stat = true },                                                   // 4
	{.grants_slot = true, .offers_element = true, .charge_time_mul = 0.85f }, // 5
	{.offers_stat = true },                                                   // 6
	{.offers_stat = true },                                                   // 7
	{.offers_stat = true },                                                   // 8
	{.offers_stat = true },                                                   // 9
	{.offers_stat = true },                                                   // 10
};

// XP needed to reach each level
static constexpr int g_XPToLevel[PLAYER_LEVEL_MAX + 1] =
{
	0, 0, 20, 50, 90, 140, 210, 300, 410, 550, 720,
};

static constexpr ElementMods g_ElementBonus[ELEMENT_TYPE_COUNT][ELEMENT_LEVEL_MAX + 1] =
{
	// ---------------- FIRE ----------------
	{
		{},                                                             // 0 
		{},                                                             // 1 
		{.damage_mul = 1.1f, .status_mag = 1.5f },                      // 2
		{.damage_mul = 1.3f, .status_time = 1.4f, .status_mag = 2.0f }, // 3
	},
	// ---------------- ICE -----------------
	{
		{},																// 0
		{},																// 1
		{.status_time = 1.5f, .status_mag = 0.85f, .bonus_pierce = 3 }, // 2  
		{.damage_mul = 1.25f, .status_time = 2.0f, .status_mag = 0.7f,  // 3
		  .bonus_pierce = 6 },                                       
	},
	// -------------- THUNDER ---------------
	{
		{},																 // 0
		{},																 // 1
		{.bonus_chains = 1, .life_mul = 1.3f },                          // 2
		{.damage_mul = 1.2f, .bonus_chains = 2, .life_mul = 1.8f },      // 3
	},
};

struct StatInfo
{
	float       base;
	float       step;       
	int         max_rank;   
	const char* label;
};

static constexpr StatInfo g_StatInfo[STAT_TYPE_COUNT] =
{
	//  base    step   cap  label
	{  10.0f,   5.0f,   6,  "Max HP +5"          },  // 10 -> 40
	{ 300.0f,  20.0f,   5,  "Move Speed +20"     },  // 300 -> 400
	{  90.0f,  30.0f,   4,  "Pickup Range +30"   },  // 90 -> 210
	{   1.0f,   0.1f,   4,  "I-Frames +0.1s"     },  // 1.0 -> 1.4 s
	{   0.0f,   0.1f,   5,  "HP Regen +0.1/s"    },  // 0 -> 0.5/s
	{   0.0f,   1.0f,   3,  "Damage Taken -1"    },  // 0 -> 3 flat
	{   1.0f,  0.25f,   4,  "XP Gain +25%"       },  // 1.0 -> 2.0x
};

static constexpr const char* g_ElementLabel[ELEMENT_TYPE_COUNT] =
{
	"Fire +1  damage / burn",
	"Ice +1  control / pierce",
	"Thunder +1  summons / chains",
};

static int   g_Level = 1;
static int   g_XP = 0;
static int   g_SlotCount = 1;
static float g_ChargeTimeMul = 1.0f;

static int   g_ElementLevel[ELEMENT_TYPE_COUNT]{};
static int   g_StatRanks[STAT_TYPE_COUNT]{};

static bool          g_LevelPending = false;
static UpgradeOption g_Offers[UPGRADE_OFFER_MAX];
static int           g_OfferCount = 0;

static bool ElementAvailable(int e)
{
	return g_ElementLevel[e] < ELEMENT_LEVEL_MAX;
}

static bool StatAvailable(int s)
{
	return g_StatRanks[s] < g_StatInfo[s].max_rank;
}

static UpgradeOption MakeElementOption(int e)
{
	return { .kind = UPGRADE_ELEMENT,
			 .element = static_cast<ElementType>(e),
			 .label = g_ElementLabel[e] };
}

static UpgradeOption MakeStatOption(int s)
{
	return { .kind = UPGRADE_STAT,
			 .stat = static_cast<StatType>(s),
			 .label = g_StatInfo[s].label };
}

static void BuildOffers(const LevelReward& reward)
{
	g_OfferCount = 0;

	UpgradeOption pool[static_cast<int>(ELEMENT_TYPE_COUNT) + static_cast<int>(STAT_TYPE_COUNT)];
	int pool_count = 0;

	if (reward.offers_element)
	{
		for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
		{
			if (ElementAvailable(e)) { pool[pool_count++] = MakeElementOption(e); }
		}
	}

	if (reward.offers_stat)
	{
		for (int s = 0; s < STAT_TYPE_COUNT; s++)
		{
			if (StatAvailable(s)) { pool[pool_count++] = MakeStatOption(s); }
		}
	}

	if (!reward.offers_element)
	{
		for (int i = pool_count - 1; i > 0; i--)
		{
			const int j = rand() % (i + 1);
			std::swap(pool[i], pool[j]);
		}
	}

	const int take = std::min(pool_count, UPGRADE_OFFER_MAX);
	for (int i = 0; i < take; i++)
	{
		g_Offers[g_OfferCount++] = pool[i];
	}

	if (reward.offers_element && reward.grants_slot && g_OfferCount > 0)
	{
		bool has_element = false;
		for (int i = 0; i < g_OfferCount; i++)
		{
			if (g_Offers[i].kind == UPGRADE_ELEMENT) { has_element = true; break; }
		}

		if (!has_element)
		{
			for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
			{
				if (ElementAvailable(e)) { g_Offers[0] = MakeElementOption(e); break; }
			}
		}
	}

	if (g_OfferCount == 0)
	{
		int best = 0;
		int best_room = -1;
		for (int s = 0; s < STAT_TYPE_COUNT; s++)
		{
			const int room = g_StatInfo[s].max_rank - g_StatRanks[s];
			if (room > best_room) { best_room = room; best = s; }
		}
		g_Offers[g_OfferCount++] = MakeStatOption(best);
	}
}

void GameProgress_Initialize()
{
	g_Level = 1;
	g_XP = 0;
	g_SlotCount = 1;
	g_ChargeTimeMul = 1.0f;
	g_LevelPending = false;
	g_OfferCount = 0;

	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++) { g_ElementLevel[e] = 1; }
	for (int s = 0; s < STAT_TYPE_COUNT; s++) { g_StatRanks[s] = 0; }
}

// ============================================================================
// XP / levelling
// ============================================================================
void GameProgress_AddXP(int amount)
{
	if (g_Level >= PLAYER_LEVEL_MAX || g_LevelPending) { return; }

	g_XP += std::max(1, static_cast<int>(amount * GameProgress_GetStat(STAT_XP_GAIN)));

	const int next = g_Level + 1;
	if (g_XP < g_XPToLevel[next]) { return; }

	g_Level = next;
	const LevelReward& reward = g_LevelPlan[g_Level];

	if (reward.grants_slot)
	{
		g_SlotCount = std::min(g_SlotCount + 1, 3);
	}
	g_ChargeTimeMul *= reward.charge_time_mul;

	BuildOffers(reward);
	g_LevelPending = true;    
}

int GameProgress_GetXP() { return g_XP; }
int GameProgress_GetLevel() { return g_Level; }

int GameProgress_GetXPForNextLevel()
{
	if (g_Level >= PLAYER_LEVEL_MAX) { return g_XP; }
	return g_XPToLevel[g_Level + 1];
}

int GameProgress_GetXPForCurrentLevel()
{
	return g_XPToLevel[g_Level];
}

bool GameProgress_IsLevelPending() { return g_LevelPending; }
int  GameProgress_GetOfferCount() { return g_OfferCount; }

const UpgradeOption& GameProgress_GetOffer(int index)
{
	static const UpgradeOption empty{};
	if (index < 0 || index >= g_OfferCount) { return empty; }
	return g_Offers[index];
}

void GameProgress_ChooseOffer(int index)
{
	if (!g_LevelPending) { return; }
	if (index < 0 || index >= g_OfferCount) { return; }

	const UpgradeOption& opt = g_Offers[index];

	if (opt.kind == UPGRADE_ELEMENT)
	{
		int& lv = g_ElementLevel[opt.element];
		lv = std::min(lv + 1, ELEMENT_LEVEL_MAX);
	}
	else
	{
		int& rank = g_StatRanks[opt.stat];
		rank = std::min(rank + 1, g_StatInfo[opt.stat].max_rank);
	}

	g_LevelPending = false;
	g_OfferCount = 0;
}

int GameProgress_GetSlotCount() { return g_SlotCount; }

float GameProgress_GetChargeTimeMul()
{
	return g_ChargeTimeMul;
}

int GameProgress_GetElementLevel(ElementType e)
{
	if (e < 0 || e >= ELEMENT_TYPE_COUNT) { return 0; }
	return g_ElementLevel[e];
}

const ElementMods& GameProgress_GetElementMods(ElementType e)
{
	static constexpr ElementMods none{};
	if (e < 0 || e >= ELEMENT_TYPE_COUNT) { return none; }
	return g_ElementBonus[e][g_ElementLevel[e]];
}

float GameProgress_GetStat(StatType s)
{
	if (s < 0 || s >= STAT_TYPE_COUNT) { return 0.0f; }
	return g_StatInfo[s].base + g_StatInfo[s].step * g_StatRanks[s];
}

int GameProgress_GetStatRank(StatType s)
{
	if (s < 0 || s >= STAT_TYPE_COUNT) { return 0; }
	return g_StatRanks[s];
}

int GameProgress_GetStatMaxRank(StatType s)
{
	if (s < 0 || s >= STAT_TYPE_COUNT) { return 0; }
	return g_StatInfo[s].max_rank;
}