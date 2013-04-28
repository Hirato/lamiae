#include "rpggame.h"

VAR(route_tag, 1, 0, -1);

void rpgplatform::update()
{
	if(!entities::ents.inrange(target) || entities::ents[target]->type != PLATFORMROUTE)
	{
		target = -1;
		float closest = 1e16f;
		loopv(entities::ents)
		{
			extentity &e = *entities::ents[i];
			if(e.type == PLATFORMROUTE && routes.access(e.attr[0]) && e.o.dist(o) < closest)
			{
				target = i;
				closest = e.o.dist(o);
			}
		}
	}

	float delta = speed * curtime / 1000.f;
	while(flags && F_ACTIVE && target >= 0 && delta > 0)
	{
		float dist = min(delta, entities::ents[target]->o.dist(o));
		if(dist > 0) //avoid divide by 0 for normalisation
		{
			delta -= dist;
			vec move = vec(entities::ents[target]->o).sub(o).normalize().mul(dist);

			o.add(move);
			loopv(stack) {stack[i]->o.add(move); stack[i]->resetinterp();}
		}

		if(delta < 0) break;
		route_tag = entities::ents[target]->attr[0];

		vector<int> &detours = *routes.access(entities::ents[target]->attr[0]);
		target = -1;

		// we pick a random one; we test up to n times if we fail to get a destination
		// Ideally this loop will only ever run once
		loopi(detours.length())
		{
			int tag = detours[rnd(detours.length())];
			loopvj(entities::ents)
			{
				extentity &e = *entities::ents[j];
				if(e.type == PLATFORMROUTE && e.attr[0] == tag)
				{
					target = j;
					break;
				}
			}
			if(target >= 0) break;
		}

		getsignal("arrived", true, this);
	}

	stack.setsize(0);
}

void rpgplatform::render()
{
	rendermodel(temp.mdl, ANIM_MAPMODEL|ANIM_LOOP, vec(o).sub(vec(0, 0, eyeheight)), yaw, pitch, roll, MDL_CULL_DIST|MDL_CULL_OCCLUDED, NULL, NULL, 0, 0, 1, temp.alpha);
}

void rpgplatform::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
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

void rpgplatform::init(const char *base)
{
	game::loadingrpgplatform = this;
	rpgscript::config->setref(this, true);

	defformatstring(file)("%s/%s.cfg", game::datapath("platforms"), base);
	execfile(file);

	game::loadingrpgplatform = NULL;
	rpgscript::config->setnull(true);
}

bool rpgplatform::validate()
{
	if(!script)
	{
		ERRORF("Platform %p uses invalid script - trying fallback: null", this);
		script = DEFAULTSCR;

		if(!script) return false;
	}

	return true;
}
