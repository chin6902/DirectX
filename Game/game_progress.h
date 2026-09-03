/*============================================================================
Contents   :  [game_progress.h]

Author     : Chin Qing You
LastUpdate : 2026/08/15
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_PROGRESS_H
#define GAME_PROGRESS_H

#include "element.h"

inline constexpr int PLAYER_LEVEL_MAX = 6;
inline constexpr int ELEMENT_LEVEL_MAX = 3;
inline constexpr int UPGRADE_OFFER_MAX = 3;

enum StatType
{
	STAT_MAX_HP,
	STAT_MOVE_SPEED,
	STAT_PICKUP_RANGE,
	STAT_IFRAME_TIME,       
	STAT_HP_REGEN,         
	STAT_DAMAGE_REDUCTION,  
	STAT_XP_GAIN,         
	STAT_TYPE_COUNT,
};

enum UpgradeKind
{
	UPGRADE_ELEMENT,
	UPGRADE_STAT,
	UPGRADE_ULTIMATE,  
	UPGRADE_ITEM,
};

enum UltimateUpgrade
{
	ULT_UPGRADE_NONE = -1,
	ULT_UPGRADE_FIRE,
	ULT_UPGRADE_ICE,
	ULT_UPGRADE_THUNDER,
};

enum ItemType
{
	ITEM_FIRE_STAFF,       
	ITEM_POTION,           
	ITEM_ELECTRIC_STAFF,  
	ITEM_TYPE_COUNT,
};

struct UpgradeOption
{
	UpgradeKind kind = UPGRADE_STAT;
	ElementType element = ELEMENT_FIRE;   
	StatType    stat = STAT_MAX_HP;  
	ItemType    item = ITEM_FIRE_STAFF;
	const char* label = "";
};

struct ElementMods
{
	float damage_mul = 1.0f;
	float status_time = 1.0f;   
	float status_mag = 1.0f;  
	int   bonus_pierce = 0;
	int   bonus_chains = 0;
	float life_mul = 1.0f;   
	int   bonus_count = 0;      
};

void GameProgress_Initialize();

// --- XP / levelling ---
void GameProgress_AddXP(int amount);        
int  GameProgress_GetXP();
int  GameProgress_GetXPForNextLevel();
int  GameProgress_GetXPForCurrentLevel();  
int  GameProgress_GetLevel();

bool GameProgress_IsLevelPending();
int  GameProgress_GetOfferCount();
const UpgradeOption& GameProgress_GetOffer(int index);
void GameProgress_ChooseOffer(int index);

int   GameProgress_GetSlotCount();          
float GameProgress_GetChargeTimeMul();    
int   GameProgress_GetElementLevel(ElementType e);
const ElementMods& GameProgress_GetElementMods(ElementType e);

float GameProgress_GetStat(StatType s);

UltimateUpgrade GameProgress_GetUltimateUpgrade();
int GameProgress_GetStatRank(StatType s);
int GameProgress_GetStatMaxRank(StatType s);
ItemType GameProgress_GetChosenItem();
float GameProgress_GetPickUpRange();

#endif