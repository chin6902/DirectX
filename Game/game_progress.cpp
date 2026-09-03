/*============================================================================
Contents   :  [game_progress.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/24
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
	bool  offers_ultimate = false;
	bool  offers_item = false;              
	unsigned int stat_mask = 0;
	float charge_time_mul = 1.0f;
};

static constexpr unsigned int StatBit(StatType s) { return 1u << static_cast<int>(s); }
static constexpr unsigned int EARLY_STATS = StatBit(STAT_MOVE_SPEED) | StatBit(STAT_XP_GAIN) | StatBit(STAT_HP_REGEN);

static constexpr LevelReward g_LevelPlan[PLAYER_LEVEL_MAX + 1] =
{
	{},                                                                        // 0 
	{},                                                                        // 1  start
	{.grants_slot = true, .offers_element = true, .charge_time_mul = 0.85f },  // 2  element
	{.offers_stat = true, .stat_mask = EARLY_STATS },                          // 3  stat
	{.offers_item = true },                                                    // 4  ITEM CHOICE (was stat)
	{.grants_slot = true, .offers_element = true, .charge_time_mul = 0.80f },  // 5  element
	{.offers_ultimate = true },                                                // 6  boost
};

static constexpr int g_XPToLevel[PLAYER_LEVEL_MAX + 1] =
{
	0, 0, 10, 40, 80, 160, 420,
};

static constexpr ElementMods g_ElementBonus[ELEMENT_TYPE_COUNT][ELEMENT_LEVEL_MAX + 1] =
{
	// ---------------- FIRE ----------------
	{
		{},
		{},
		{.damage_mul = 1.35f, .status_mag = 1.1f },                      // 2
		{.damage_mul = 1.75f, .status_time = 1.2f, .status_mag = 1.5f }, // 3
	},
	// ---------------- ICE -----------------
	{
		{},
		{},
		{.status_time = 1.5f, .status_mag = 0.85f, .bonus_pierce = 3 },                     // 2 
		{.damage_mul = 1.25f, .status_time = 2.0f, .status_mag = 0.6f, .bonus_pierce = 6 }, // 3
	},
	// -------------- THUNDER ---------------
	{
		{},
		{},
		{.bonus_chains = 1, .life_mul = 1.3f },                          // 2
		{.damage_mul = 1.2f, .bonus_chains = 2, .life_mul = 1.7f },      // 3
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
	{  20.0f,  10.0f,   0,  "Max HP +10"         },  
	{ 300.0f,  50.0f,   1,  "Move Speed +50"     },  // 300 -> 350
	{ 150.0f,  50.0f,   0,  "Pickup Range +50"   },
	{   1.0f,  0.25f,   0,  "I-Frames +0.25s"    },
	{   0.1f,  0.10f,   1,  "HP Regen +0.1/s"    },  // 0.1 -> 0.2/s
	{   0.0f,  1.00f,   0,  "Damage Taken -1"    },
	{   1.0f,  0.50f,   1,  "XP Gain +50%"       },  // 1.0 -> 1.5x
};

static constexpr const char* g_UltimateLabel[ELEMENT_TYPE_COUNT] =
{
	"ultimate casts twice at 0.75x",
	"statuses deal 2x for 1.5x longer",
	"+1 projectile, instance, child",
};

static constexpr const char* g_ElementLabel[ELEMENT_TYPE_COUNT] =
{
	"Fire +1  damage / burn",
	"Ice +1  control / pierce",
	"Thunder +1  summons / chains",
};

static constexpr const char* g_ItemLabel[ITEM_TYPE_COUNT] =
{
	"Fire Staff +30% dmg 15s",
	"Vials +2 HP drop 10%",
	"Storm Staff -20% charge 15s",
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

static UltimateUpgrade g_UltUpgrade = ULT_UPGRADE_NONE;
static ItemType        g_ChosenItem = ITEM_TYPE_COUNT;   

static bool ElementAvailable(int e)
{
	return g_ElementLevel[e] < ELEMENT_LEVEL_MAX;
}

static bool StatAvailable(int s, unsigned int mask)
{
	if (mask != 0 && (mask & StatBit(static_cast<StatType>(s))) == 0) { return false; }
	return g_StatRanks[s] < g_StatInfo[s].max_rank;
}

static UpgradeOption MakeUltimateOption(int e)
{
	return { .kind = UPGRADE_ULTIMATE,
			 .element = static_cast<ElementType>(e),
			 .label = g_UltimateLabel[e] };
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

static UpgradeOption MakeItemOption(int i)
{
	return { .kind = UPGRADE_ITEM,
			 .item = static_cast<ItemType>(i),
			 .label = g_ItemLabel[i] };
}

static void BuildOffers(const LevelReward& reward)
{
	g_OfferCount = 0;

	static constexpr int POOL_MAX = static_cast<int>(ELEMENT_TYPE_COUNT) + static_cast<int>(STAT_TYPE_COUNT);

	UpgradeOption pool[POOL_MAX];
	int pool_count = 0;

	if (reward.offers_element)
	{
		for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
		{
			if (ElementAvailable(e)) { pool[pool_count++] = MakeElementOption(e); }
		}
	}

	if (reward.offers_ultimate)
	{
		for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
		{
			g_Offers[g_OfferCount++] = MakeUltimateOption(e);
		}
		return;
	}

	if (reward.offers_item)
	{
		for (int i = 0; i < ITEM_TYPE_COUNT; i++)
		{
			g_Offers[g_OfferCount++] = MakeItemOption(i);
		}
		return;
	}

	if (reward.offers_stat)
	{
		for (int s = 0; s < STAT_TYPE_COUNT; s++)
		{
			if (StatAvailable(s, reward.stat_mask)) { pool[pool_count++] = MakeStatOption(s); }
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
	g_UltUpgrade = ULT_UPGRADE_NONE;   
	g_ChosenItem = ITEM_TYPE_COUNT;    
	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++) { g_ElementLevel[e] = 1; }
	for (int s = 0; s < STAT_TYPE_COUNT; s++) { g_StatRanks[s] = 0; }
}

void GameProgress_AddXP(int amount)
{
	if (g_Level >= PLAYER_LEVEL_MAX || g_LevelPending) { return; }

	g_XP += std::max(1, static_cast<int>(amount * GameProgress_GetStat(STAT_XP_GAIN)));

	const int next = g_Level + 1;
	if (g_XP < g_XPToLevel[next]) { return; }

	// --- level up ---
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

	if (opt.kind == UPGRADE_ULTIMATE)
	{
		g_UltUpgrade = static_cast<UltimateUpgrade>(opt.element);
	}
	else if (opt.kind == UPGRADE_ELEMENT)
	{
		int& lv = g_ElementLevel[opt.element];
		lv = std::min(lv + 1, ELEMENT_LEVEL_MAX);
	}
	else if (opt.kind == UPGRADE_ITEM)      
	{
		g_ChosenItem = opt.item;
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

UltimateUpgrade GameProgress_GetUltimateUpgrade() { return g_UltUpgrade; }

ItemType GameProgress_GetChosenItem() { return g_ChosenItem; }   

float GameProgress_GetPickUpRange()
{
	return GameProgress_GetStat(STAT_PICKUP_RANGE);
}

int GameProgress_GetStatMaxRank(StatType s)
{
	if (s < 0 || s >= STAT_TYPE_COUNT) { return 0; }
	return g_StatInfo[s].max_rank;
}