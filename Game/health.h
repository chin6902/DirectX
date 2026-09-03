/*============================================================================
Contents   :  [health.h]

Author     : Chin Qing You
LastUpdate : 2026/08/15
-----------------------------------------------------------------------------

============================================================================*/
#ifndef HEALTH_H
#define HEALTH_H

#include <algorithm>
#include <cmath>

struct Health
{
	float current = 1.0f;
	float max = 1.0f;

	void Reset(float max_hp)
	{
		max = max_hp;
		current = max_hp;
	}

	bool Damage(float amount)
	{
		if (current <= 0.0f) { return false; }      

		current = std::max(0.0f, current - amount);
		return current <= 0.0f;
	}

	void Heal(float amount)
	{
		if (current <= 0.0f) { return; }            
		current = std::min(max, current + amount);
	}

	void SetMax(float max_hp, bool grant_difference = false)
	{
		if (grant_difference && max_hp > max) { current += (max_hp - max); }
		max = max_hp;
		current = std::min(current, max);
	}

	bool  IsDead()   const { return current <= 0.0f; }
	float Fraction() const { return (max > 0.0f) ? current / max : 0.0f; }   
	int   Display()  const { return static_cast<int>(ceilf(current)); }     
};

#endif