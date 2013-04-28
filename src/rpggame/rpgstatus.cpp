#include "rpggame.h"

victimeffect::victimeffect(rpgent *o, inflict *inf, int chargeflags, float mul) : owner(o)
{
	group = inf->status;
	elem = group->friendly ? ATTACK_NONE : inf->element;
	mul *= inf->mul;

	loopv(group->effects)
	{
		status *st = effects.add(group->effects[i]->dup());
		st->remain = st->duration;

		if(chargeflags & CHARGE_MAG) st->strength *= mul;
		if(chargeflags & CHARGE_DURATION) st->remain *= mul;
	}
}

bool victimeffect::update(rpgent *victim)
{
	int resist = 0;
	int thresh = 0;
	if(victim->type() == ENT_CHAR)
	{
		rpgchar *d = (rpgchar *) victim;
		resist = d->base.getresistance(elem);
		thresh = d->base.getthreshold(elem);
	}

	bool instant = true;
	loopvrev(effects)
	{
		instant = instant && effects[i]->duration == 0;
		effects[i]->update(victim, owner, resist, thresh);

		if(effects[i]->duration >= 0 && (effects[i]->remain -= curtime) <= 0)
		{
			delete effects.remove(i);
		}
	}

	if(group->persisteffect)
		group->persisteffect->drawaura(victim, 1, instant ? effect::DEATH : effect::DEATH_PROLONG, curtime);

	return effects.length();
}

bool areaeffect::update()
{
	vector<int> removing;
	bool instant = true;
	loopvrev(effects)
	{
		instant = instant && effects[i]->duration == 0;
		if(effects[i]->duration >= 0 && (effects[i]->remain -= curtime) <= 0)
			removing.add(i);
	}

	loopvj(game::curmap->objs)
	{
		//checking the head and the feet should be sufficiently accurate for gaming purposes
		rpgent *victim = game::curmap->objs[j];
		const vec min = vec(victim->o.x - victim->radius, victim->o.y - victim->radius, victim->o.z - victim->eyeheight);
		const vec max = vec(victim->o.x + victim->radius, victim->o.y + victim->radius, victim->o.z + victim->aboveeye);

		if(o.dist_to_bb(min, max) <= radius)
		{
			int resist = 0;
			int thresh = 0;
			if(victim->type() == ENT_CHAR)
			{
				rpgchar *d = (rpgchar *) victim;
				resist = d->base.getresistance(elem);
				thresh = d->base.getthreshold(elem);
			}

			loopvrev(effects)
				effects[i]->update(victim, owner, resist, thresh);

			if(group->persisteffect)
				group->persisteffect->drawaura(victim, 1, instant ? effect::DEATH : effect::DEATH_PROLONG, curtime);

		}
	}

	loopv(removing) delete effects.remove(removing[i]);

	return effects.length();
}

extern int projallowflare;

void areaeffect::render()
{
	if(!fx) return;

	switch(fx->particle)
	{
		default:
			if((lastmillis - lastemit) >= emitmillis)
			{
				fx->drawsphere(o, radius, 2, effect::DEATH_PROLONG, lastmillis - lastemit);
				lastemit = lastmillis;
			}
			break;

		case PART_EXPLOSION:
		case PART_EXPLOSION_BLUE:
			fx->drawsphere(o, radius, 1, effect::DEATH_PROLONG, 0);
			break;
	}
	if(projallowflare && fx->flags & (FX_FLARE | FX_FIXEDFLARE))
	{
		vec col = vec(fx->lightcol).mul(256);
		bool fixed = fx->flags & FX_FIXEDFLARE;
		regularlensflare(o, col.x, col.y, col.z, fixed, false, fx->lightradius * (fixed ? 1.f : 7.5f));
	}
}

void areaeffect::dynlight()
{
	if(!fx || !(fx->flags & FX_DYNLIGHT)) return;
	adddynlight(o, fx->lightradius, fx->lightcol);
}

void status_generic::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	//time based multiplier, dwindles on last 20% of duration
	int strength = this->strength < 0 ? min(0, this->strength + thresh) : max(0, this->strength - thresh);
	float mult = (mul + extra * variance) * (100 - resist) / 100.f;
	if(duration > 0)
		mult *= clamp(5.f * remain/duration, 0.f, 1.0f);

	switch(victim->type())
	{
		case ENT_CHAR:
		{
			rpgchar *ent = (rpgchar *) victim;
			//instant applicable types
			switch(type)
			{
				case STATUS_HEALTH:
					if(DEBUG_STATUS)
						DEBUGF("Applying health effect to %p; %i %i; mult %f", victim, strength, duration, mult);
					ent->health += strength * mult * (duration != 0 ? curtime / 1000.f : 1);
					if(strength < 0 && ent->health < 0) ent->die(owner);
					return;

				case STATUS_MANA:
					if(DEBUG_STATUS)
						DEBUGF("Applying mana effect to %p; %i %i; mult %f", victim, strength, duration, mult);
					ent->mana += strength * mult * (duration != 0 ? curtime / 1000.f : 1);
					ent->mana = max<float>(ent->mana, 0);
					return;

				case STATUS_DOOM:
					if(DEBUG_STATUS)
						DEBUGF("Applying doom effect to %p; %i %i; mult %f", victim, strength, duration, mult);
					//either instant death, or to cater to duration == -1 a gradual percentage wise drain of health
					if(duration == 0 || (ent->health -= ent->base.getmaxhp() * strength * mult * curtime / 100000.f) < 0)
						ent->die(owner);
					return;
			}
			if(duration == 0) break;

			static const char *attrs[] = { "strength", "endurance", "agility", "charisma", "intelligence", "wisdom", "luck" };
			static const char *skills[] = { "armour", "diplomacy", "magic", "marksman", "melee", "stealth", "craft" };
			static const char *resists[] = { "fire", "water", "air", "earth", "arcane", "mind", "holy", "darkness", "slash", "blunt", "pierce" };

			//the rest
			switch(type)
			{
				case STATUS_MOVE:
					if(DEBUG_STATUS)
						DEBUGF("Applying movement buff to %p; %i %i; mult %f", victim, strength, duration, mult);
					ent->base.deltamovespeed += strength * mult;
					ent->base.deltajumpvel += strength * mult;
					return;

				case STATUS_CRIT:
					if(DEBUG_STATUS)
						DEBUGF("Applying crit buff to %p; %i %i; mult %f", victim, strength, duration, mult);
					ent->base.deltacrit += strength * mult;
					return;

				case STATUS_WEIGHT:
					if(DEBUG_STATUS)
						DEBUGF("Applying weight capacity buff to %p; %i %i; mult %f", victim, strength, duration, mult);
					ent->base.deltacarry += strength * mult;
					return;

				case STATUS_HREGEN:
					if(DEBUG_STATUS)
						DEBUGF("Applying healthregen buff to %p; %i %i; mult %f", victim, strength, duration, mult);
					ent->base.deltahregen += strength * mult;
					return;

				case STATUS_MREGEN:
					if(DEBUG_STATUS)
						DEBUGF("Applying manaregen buff to %p; %i %i; mult %f", victim, strength, duration, mult);
					ent->base.deltamregen += strength * mult;
					return;

				case STATUS_STRENGTH:
				case STATUS_ENDURANCE:
				case STATUS_AGILITY:
				case STATUS_CHARISMA:
				case STATUS_WISDOM:
				case STATUS_INTELLIGENCE:
				case STATUS_LUCK:
					if(DEBUG_STATUS)
						DEBUGF("Applying %s buff to %p; %i %i; mult %f", attrs[type-STATUS_STRENGTH], victim, strength, duration, mult);
					ent->base.deltaattrs[type - STATUS_STRENGTH] += strength * mult;
					return;

				case STATUS_ARMOUR:
				case STATUS_DIPLOMACY:
				case STATUS_MAGIC:
				case STATUS_MARKSMAN:
				case STATUS_MELEE:
				case STATUS_STEALTH:
				case STATUS_CRAFT:
					if(DEBUG_STATUS)
						DEBUGF("Applying %s buff to %p; %i %i; mult %f", skills[type - STATUS_ARMOUR], victim, strength, duration, mult);
					ent->base.deltaskills[type - STATUS_ARMOUR] += strength * mult;
					return;

				case STATUS_FIRE_T:
				case STATUS_WATER_T:
				case STATUS_AIR_T:
				case STATUS_EARTH_T:
				case STATUS_ARCANE_T:
				case STATUS_MIND_T:
				case STATUS_HOLY_T:
				case STATUS_DARKNESS_T:
				case STATUS_SLASH_T:
				case STATUS_BLUNT_T:
				case STATUS_PIERCE_T:
					if(DEBUG_STATUS)
						DEBUGF("Applying %s threshold buff to %p; %i %i; mult %f", resists[type - STATUS_FIRE_T], victim, strength, duration, mult);
					ent->base.deltathresh[type - STATUS_FIRE_T] += strength * mult;
					return;

				case STATUS_FIRE_R:
				case STATUS_WATER_R:
				case STATUS_AIR_R:
				case STATUS_EARTH_R:
				case STATUS_ARCANE_R:
				case STATUS_MIND_R:
				case STATUS_HOLY_R:
				case STATUS_DARKNESS_R:
				case STATUS_SLASH_R:
				case STATUS_BLUNT_R:
				case STATUS_PIERCE_R:
					if(DEBUG_VSTATUS)
						DEBUGF("Applying %s resistance buff to %p; %i %i; mult %f", resists[type - STATUS_FIRE_R], victim, strength, duration, mult);
					ent->base.deltaresist[type - STATUS_FIRE_R] += strength * mult;
					return;

				case STATUS_STUN:
					///WRITE ME, immobilise; don't allow movement or attacks with somatic components
				case STATUS_SILENCE:
					///WRITE ME, prevent attacks with a vocal component, special speech path for talking to NPCs too?
					return;
			}
		}
		case ENT_CONTAINER:
		{
			rpgcontainer *ent = (rpgcontainer *) victim;
			switch(type)
			{
				case STATUS_LOCK:
					if(ent->lock == 101) return; //special case, no unlocking

					if(-strength * mult > ent->lock) ent->lock = 0; //unlock!
					else if(strength * mult > ent->lock) ent->lock = min<int>(100, strength * mult); //lock!

					return;
				case STATUS_MAGELOCK: //magelock
					if(!duration != 0)
						ent->magelock += strength * mult;
					return;
			}
		}
	}

	//generic types, note that unhandled types are a) specialise or b) not handled by that entiy
	switch(type)
	{
		case STATUS_DISPEL:
			///WRITE ME; select a random status effect every update and weaken it
			return;

		case STATUS_REFLECT:
			///WRITE ME; what do?
			return;

		case STATUS_INVIS:
			if(DEBUG_STATUS)
				DEBUGF("Applying invisibility effect to %p; %i %i; mult %f", victim, strength, duration, mult);
			victim->temp.alpha -= strength * mult / 100.f;
			break;
	}
}

void status_polymorph::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	if(duration == 0 || (duration > 0 && remain <= curtime)) return;

	victim->temp.mdl = mdl;
}

void status_light::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	if(duration == 0 || (duration > 0 && remain <= curtime)) return;

	int strength = this->strength < 0 ? min(0, this->strength + thresh) : max(0, this->strength - thresh);
	float mult = (mul + extra * variance) * (100 - resist) / 100.f;
	if(duration > 0)
		mult *= clamp(5.f * remain/duration, 0.f, 1.0f);

	victim->temp.light.add(vec4(colour, strength * mult));
}

VAR(eff_strength, 1, -1, -1);
VAR(eff_remain, 1, -1, -1);
VAR(eff_mult, 1, -1, -1);
VAR(eff_duration, 1, -1, -1);

void status_signal::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	eff_strength = strength < 0 ? min(0, strength + thresh) : max(0, strength - thresh);
	eff_remain = remain;
	eff_duration = duration;
	eff_mult = (mul + extra * variance) * (100 - resist) / 100.f;

	victim->getsignal(signal, true, owner);
}

void status_script::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	if(!script) return;

	if(!code) code = compilecode(script);
	eff_strength = strength < 0 ? min(0, strength + thresh) : max(0, strength - thresh);
	eff_remain = remain;
	eff_duration = duration;
	eff_mult = (mul + extra * variance) * (100 - resist) / 100.f;

	rpgscript::doentscript(victim, owner, code);
}
