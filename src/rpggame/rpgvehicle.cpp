#include "rpggame.h"

void rpgvehicle::update()
{
	move = strafe = 0;
	if(passengers.length())
	{
		move = passengers[0]->move;
		yaw += passengers[0]->strafe * curtime / 30.f;
	}

	//FIXME do proper vehicle physics.
	moveplayer(this, 3, true);

	loopv(passengers)
	{
		passengers[i]->o = o;
		passengers[i]->yaw = yaw;
	}
}

void rpgvehicle::render()
{
	int sec = move ? (move > 0 ? ANIM_FORWARD : ANIM_BACKWARD)| ANIM_LOOP : 0;
	sec <<= ANIM_SECONDARY;

	rendermodel(temp.mdl, ANIM_MAPMODEL| ANIM_LOOP | sec, vec(o).sub(vec(0, 0, eyeheight)), yaw + 90, pitch, roll, MDL_CULL_DIST|MDL_CULL_OCCLUDED, NULL, NULL, 0, 0, temp.alpha);
}

void rpgvehicle::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
{
	loopv(weapon->effects)
	{
		if(!game::statuses.inrange(weapon->effects[i]->status)) continue;
		seffects.add(new victimeffect(attacker, weapon->effects[i], weapon->chargeflags, mul));
	}

	if(ammo) loopv(ammo->effects)
	{
		if(!game::statuses.inrange(ammo->effects[i]->status)) continue;
		seffects.add(new victimeffect(attacker, ammo->effects[i], weapon->chargeflags, mul));
	}

	getsignal("hit", false, attacker);
}

void rpgvehicle::init(int base)
{
	game::loadingrpgvehicle = this;
	rpgscript::config->setref(this, true);

	defformatstring(file)("%s/%i.cfg", game::datapath("vehicles"), base);
	execfile(file);

	rpgscript::config->setnull(true);
	game::loadingrpgvehicle = NULL;
}
