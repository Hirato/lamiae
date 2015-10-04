#include "rpggame.h"

void rpgtrigger::update()
{
	//hack to make models passthrough. (eg, doors)
	if(flags & F_TRIGGERED)
		state = (lastmillis - lasttrigger < 750) ? CS_ALIVE : CS_DEAD;
	else
		state = (lastmillis - lasttrigger < 750) ? CS_DEAD : CS_ALIVE;
}

void rpgtrigger::render()
{
	if(flags & F_INVIS) return;
	vec4 col(colour, temp.alpha);

	rendermodel(temp.mdl, (flags & F_TRIGGERED) ? ANIM_TRIGGER : (lasttrigger - lastmillis > 1500 ? ANIM_MAPMODEL|ANIM_LOOP : ANIM_TRIGGER|ANIM_REVERSE), vec(o).sub(vec(0, 0, eyeheight)), yaw, pitch, roll, MDL_CULL_DIST|MDL_CULL_OCCLUDED, NULL, NULL, lasttrigger, 1500, getscale(), col);
}

void rpgtrigger::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
{
	loopv(weapon->effects)
	{
		seffects.add(new victimeffect(attacker, weapon->effects[i], weapon->chargeflags, mul));
	}

	if(ammo) loopv(ammo->effects)
	{
		seffects.add(new victimeffect(attacker, ammo->effects[i], weapon->chargeflags, mul));
	}

	getsignal("hit", false, attacker);
}

void rpgtrigger::init(const char *base)
{
	game::loadingrpgtrigger = this;
	rpgscript::config->setref(this, true);

	defformatstring(file, "%s/%s.cfg", game::datapath("triggers"), base);
	rpgexecfile(file);

	rpgscript::config->setnull(true);
	game::loadingrpgtrigger = NULL;
}

bool rpgtrigger::validate()
{
	if(!script)
	{
		ERRORF("Trigger %p uses invalid script - trying fallback: null", this);
		script = DEFAULTSCR;

		if(!script) return false;
	}

	return true;
}

VAR(at_entry, 1, 0, -1);
VAR(at_exit, 1, 0, -1);

void areatrigger::update()
{
	if((flags & (AT_ONFRAME|AT_ONFIXEDPERIOD)) == AT_ONFIXEDPERIOD)
	{
		remaining += curtime;
		if(remaining < period && ((flags & AT_PERIOD_MASK) == AT_ONFIXEDPERIOD))
			return;
	}
	// AT_ONENTRY|AT_ONEXIT|AT_ONFRAME all require evaluation each frame

	static vector<rpgent *> inside;
	mapinfo *curmap = game::curmap;
	inside.setsize(0);

	if(flags & AT_TESTCRITTERS)
	{
		loopv(curmap->objs)
		{
			rpgent *obj = curmap->objs[i];
			if(obj->type() != ENT_CHAR) continue;
			int j = 0;
			for(;j < 3; j++) if(obj->o[j] > top[j] || obj->o[j] < bottom[j]) break;
			if((j == 3) != !!(flags & AT_TESTEXTERNAL)) inside.add(obj); //mutually exclusive scenario

		}
	}
	else //if (flags & AT_TESTPLAYER)
	{
		rpgent *obj = game::player1;
		int j = 0;
		for(;j < 3; j++) if(obj->o[j] > top[j] || obj->o[j] < bottom[j]) break;
		if((j == 3) != !!(flags & AT_TESTEXTERNAL)) inside.add(obj);

	}

	// If these flags are set, then we need to track the new occupants.
	if(flags & (AT_ONEXIT|AT_ONENTRY))
	{
		at_exit = 1;
		loopv(occupants)
		{
			if(inside.find(occupants[i]) < 0)
			{
				rpgent *ent = occupants.removeunordered(i--);
				if(flags & AT_SIGNALMAP) curmap->getsignal(sig, false, ent);
				if(flags & AT_SIGNALCRITTER) ent->getsignal(sig, false, ent);
			}
		}
		at_exit = 0;
		at_entry = 1;
		loopv(inside)
		{
			if(occupants.find(inside[i]) < 0)
			{
				//remove from the queue so we don't send a second signal
				rpgent *ent = inside.removeunordered(i--);
				occupants.add(ent);
				if(flags & AT_SIGNALMAP) curmap->getsignal(sig, false, ent);
				if(flags & AT_SIGNALCRITTER) ent->getsignal(sig, false, ent);
			}
		}
		at_entry = 0;
	}
	if(flags & (AT_ONFRAME|AT_ONFIXEDPERIOD))
	{
		if(!(flags & AT_ONFRAME))
		{
			if (remaining < period) return;
			remaining -= period;
		}
		loopv(inside)
		{
			if(flags & AT_SIGNALMAP) curmap->getsignal(sig, false, inside[i]);
			if(flags & AT_SIGNALCRITTER) inside[i]->getsignal(sig, false, inside[i]);
		}
	}
}
