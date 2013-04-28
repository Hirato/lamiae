#include "rpggame.h"
void rpgobstacle::update()
{
	if(!(flags&F_STATIONARY))
	{
		//throttle updates
		if(!timeinair && vel.magnitude() < 1 && rnd(clamp((10000 + lastupdate - lastmillis) / 10, 1, 1000)))
			return;

		lastupdate = lastmillis;
		moveplayer(this, 2, false);
		entities::touchents(this);
	}
}

void rpgobstacle::render()
{
	rendermodel(temp.mdl, ANIM_MAPMODEL|ANIM_LOOP, vec(o).sub(vec(0, 0, eyeheight)), yaw, pitch, roll, MDL_CULL_DIST|MDL_CULL_OCCLUDED, NULL, NULL, 0, 0, 1, temp.alpha);
}

void rpgobstacle::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
{
	loopv(weapon->effects)
	{
		seffects.add(new victimeffect(attacker, weapon->effects[i], weapon->chargeflags, mul));
	}

	if(ammo) loopv(ammo->effects)
	{
		seffects.add(new victimeffect(attacker, ammo->effects[i], weapon->chargeflags, mul));
	}

	vel.add(dir.mul(weapon->kickback + (ammo ? ammo->kickback : 0)));

	getsignal("hit", false, attacker);
}

void rpgobstacle::init(const char *base)
{
	game::loadingrpgobstacle = this;
	rpgscript::config->setref(this, true);

	defformatstring(file)("%s/%s.cfg", game::datapath("obstacles"), base);
	execfile(file);

	game::loadingrpgobstacle = NULL;
	rpgscript::config->setnull(true);
}

bool rpgobstacle::validate()
{
	if(!script)
	{
		ERRORF("Obstacle %p uses invalid script - trying fallback: null", this);
		script = DEFAULTSCR;

		if(!script) return false;
	}

	return true;
}
