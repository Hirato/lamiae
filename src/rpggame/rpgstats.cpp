#include "rpggame.h"

int stats::getmaxhp() const
{
	int amnt = bonushealth + deltahealth + (getattr(STAT_ENDURANCE) * 6 + getattr(STAT_STRENGTH) * 3) * 25 / 40.0f;
	return amnt;
}

float stats::gethpregen() const
{
	return max<float>(0, bonushregen + deltahregen + getattr(STAT_ENDURANCE) * 0.0025 + getattr(STAT_STRENGTH) * 0.00125);
}

int stats::getmaxmp() const
{
	int amnt = bonusmana + deltamana + (getattr(STAT_INTELLIGENCE) * 3 + getattr(STAT_WISDOM) * 6) * 25 / 40.0f;
	return max(0, amnt);
}

float stats::getmpregen() const
{
	return max<float>(0, bonusmregen + deltamregen + getattr(STAT_WISDOM)  * 0.05 + getattr(STAT_INTELLIGENCE) * 0.025);
}

int stats::getmaxcarry() const
{
	int amnt = bonuscarry + deltacarry + getattr(STAT_STRENGTH) * 5;
	return max(0, amnt);
}

int stats::getthreshold(int n) const
{
	if(n == ATTACK_NONE) return 0;
	return bonusthresh[n] + deltathresh[n];
}

int stats::getresistance(int n) const
{
	float amnt = 0;
	switch(n)
	{
		case ATTACK_NONE: return 0;
		case ATTACK_FIRE:
		case ATTACK_WATER:
			amnt += getattr(STAT_AGILITY) / 10.0f; break;
		case ATTACK_AIR:
		case ATTACK_EARTH:
			amnt += getattr(STAT_STRENGTH) / 10.0f; break;
		case ATTACK_ARCANE:
			amnt += (getattr(STAT_WISDOM) + getattr(STAT_INTELLIGENCE)) / 20.0f; break;
		case ATTACK_MIND:
			amnt += getattr(STAT_WISDOM) / 10.f; break;
		case ATTACK_HOLY:
		case ATTACK_DARKNESS:
			amnt += (getattr(STAT_WISDOM) + getattr(STAT_ENDURANCE)) / 20.f; break;
		case ATTACK_SLASH:
			amnt += getattr(STAT_AGILITY) / 20.0f; break;
		case ATTACK_BLUNT:
			amnt += getattr(STAT_STRENGTH) / 20.0f; break;
		case ATTACK_PIERCE:
			amnt += getattr(STAT_ENDURANCE) / 20.0f; break;
	}
	amnt += bonusresist[n] + deltaresist[n] + getattr(STAT_LUCK) / 15.0f;
	return min<int>(95, amnt);
}

void stats::skillpotency(int n, float &amnt, float &extra)
{
	amnt = .25;
	switch(n)
	{
		case -1: amnt = 1; extra = rnd(50) / 100.0f; return; //special NO MODIFIERs case
		case SKILL_ARMOUR:
			amnt += (getattr(STAT_STRENGTH)      + getattr(STAT_ENDURANCE) * 2       + getskill(SKILL_ARMOUR) * 5) / 100.0f;
			break;
		case SKILL_DIPLOMACY:
			amnt += (getattr(STAT_CHARISMA) * 3                                      + getskill(SKILL_DIPLOMACY) * 5) / 100.0f;
			break;
		case SKILL_MAGIC:
			amnt += (getattr(STAT_WISDOM) * 1.75 + getattr(STAT_INTELLIGENCE) * 1.25 + getskill(SKILL_MAGIC) * 5) / 100.0f;
			break;
		case SKILL_MARKSMAN:
			amnt += (getattr(STAT_STRENGTH) * .5 + getattr(STAT_AGILITY) * 2.5       + getskill(SKILL_MARKSMAN) * 5) / 100.0f;
			break;
		case SKILL_MELEE:
			amnt += (getattr(STAT_AGILITY) * .5  + getattr(STAT_STRENGTH) * 2.5      + getskill(SKILL_MELEE) * 5) / 100.0f;
			break;
		case SKILL_STEALTH:
			amnt += (getattr(STAT_LUCK) * 1      + getattr(STAT_AGILITY) * 2         + getskill(SKILL_STEALTH) * 5) / 100.0f;
			break;
	}
	amnt = log(1 + amnt);

	extra = amnt * (50 + rnd(101)) / 100.0f;
	if(rnd(1000) < critchance())
		extra = extra * (1.10 + (rnd(191) / 100.0f)) + (rnd(150) / 100.0f) * (rnd(150) / 100.0f);
	extra -= amnt;
}

void stats::setspeeds(float &maxspeed, float &jumpvel) const
{
	float mul = clamp(2 - parent->getweight() / getmaxcarry(), 0.01f, 1.f) * (.2f + parent->getscale() * 0.8f);
	maxspeed = (40 + bonusmovespeed + deltamovespeed + getattr(STAT_AGILITY) / 4.f) * mul;
	jumpvel = (80 + bonusjumpvel + deltajumpvel + getattr(STAT_AGILITY) / 2.f) * mul;
}

void stats::resetdeltas()
{
	loopi(STAT_MAX)
		deltaattrs[i] = 0;
	loopi(SKILL_MAX)
		deltaskills[i] = 0;
	loopi(ATTACK_MAX)
	{
		deltathresh[i] = 0;
		deltaresist[i] = 0;
	}
	deltahealth = deltamana = deltamovespeed = deltajumpvel = deltacarry = deltacrit = 0;
	deltahregen = deltamregen = 0;
}
