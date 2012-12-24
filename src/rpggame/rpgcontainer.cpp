#include "rpggame.h"

void rpgcontainer::update()
{
	resetmdl();
	temp.light = vec4(0, 0, 0, 0);
	temp.alpha = 1;
	magelock = 0; // IT'S MAGIC!
}

void rpgcontainer::resetmdl()
{
	temp.mdl = (mdl && mdl[0]) ? mdl : DEFAULTMODEL;
}

void rpgcontainer::render()
{
	rendermodel(temp.mdl, ANIM_MAPMODEL|ANIM_LOOP, vec(o).sub(vec(0, 0, eyeheight)), yaw + 90, pitch, roll, MDL_CULL_DIST|MDL_CULL_OCCLUDED, NULL, NULL, 0, 0, temp.alpha);
}

item *rpgcontainer::additem(item *it)
{
	vector<item *> &inv = inventory.access(it->base, vector<item *>());

	loopv(inv)
	{
		if(inv[i]->compare(it))
		{
			inv[i]->quantity += it->quantity;
			return inv[i];
		}
	}

	item *newit = inv.add(new item());
	it->transfer(*newit);
	return newit;
}

item *rpgcontainer::additem(int base, int q)
{
	item *it = new item();
	it->init(base);
	it->quantity = q;
	item *ret = additem(it);
	delete it;
	return ret;
}

int rpgcontainer::drop(item *it, int q, bool spawn)
{
	vector<item *> &inv = inventory.access(it->base, vector<item *>());
	int rem = 0;
	if(inv.find(it) == -1) return 0;

	rem = min(q, it->quantity);
	it->quantity -= rem;

	if(rem && spawn)
	{
		rpgitem *drop = new rpgitem();
		it->transfer(*drop);
		game::curmap->objs.add(drop);

		drop->quantity = rem;
		drop->o = vec(yaw * RAD, pitch * RAD).mul(radius * 2).add(o);
		entinmap(drop);
	}

	return rem;
}

int rpgcontainer::drop(int base, int q, bool spawn)
{
	vector<item *> &inv = inventory.access(base, vector<item *>());
	int rem = 0;

	loopv(inv)
		rem += drop(inv[i], min(inv[i]->quantity, q - rem), spawn);

	return rem;
}

int rpgcontainer::getitemcount(int base)
{
	vector<item *> &inv = inventory.access(base, vector<item *>());

	int count = 0;
	loopv(inv) count += inv[i]->quantity;

	return count;
}

int rpgcontainer::getcount(item *it)
{
	vector<item *> &inv = inventory.access(it->base, vector<item *>());

	int count = 0;
	if(inv.find(it) >= 0) count += it->quantity;

	return count;
}

float rpgcontainer::getweight()
{
	float ret = 0;

	enumerate(inventory, vector<item *>, stack,
		loopvj(stack) ret += stack[j]->quantity * stack[j]->weight;
	)

	return ret;
}

void rpgcontainer::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
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

void rpgcontainer::init(int base)
{
	game::loadingrpgcontainer = this;
	rpgscript::config->setref(this, true);

	defformatstring(file)("data/rpg/games/%s/containers/%i.cfg", game::data, base);
	execfile(file);

	game::loadingrpgcontainer = NULL;
	rpgscript::config->setnull(true);
}
