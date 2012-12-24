#include "rpggame.h"

///base functions
void rpgitem::resetmdl()
{
	temp.mdl = (mdl && mdl[0]) ? mdl : DEFAULTMODEL;
}

void rpgitem::update()
{
	//throttle updates
	if(!timeinair && vel.magnitude() < 1 && rnd(clamp((10000 + lastupdate - lastmillis) / 10, 1, 1000)))
		return;

	lastupdate = lastmillis;
	moveplayer(this, 2, false);
	if(!timeinair && !floor.iszero())
	{
		vec dir = vec(floor).rotate(PI / 2, vec((yaw + 90) * RAD, 0));
		vectoyawpitch(dir, roll, pitch);
	}
	entities::touchents(this);
}

void rpgitem::render()
{
	rendermodel(temp.mdl, ANIM_MAPMODEL|ANIM_LOOP, vec(o).sub(vec(0, 0, eyeheight)), yaw + 90, pitch, roll, MDL_CULL_DIST|MDL_CULL_OCCLUDED, NULL, NULL, 0, 0, temp.alpha);
}

const char *rpgitem::getname() const
{
	if(name && name[0]) return name;
	return "";
}

int rpgitem::getscript()
{
	return script;
}

void item::init(int base)
{
	this->base = base;
	game::loadingitem = this;

	defformatstring(file)("data/rpg/games/%s/items/%i.cfg", game::data, base);
	execfile(file);

	game::loadingitem = NULL;
}

void rpgitem::init(int base)
{
	rpgscript::config->setref(this, true);
	item::init(base);
	rpgscript::config->setnull(true);
}

item *rpgitem::additem(int base, int q)
{
	if(base != base) return NULL;

	quantity += q;
	return this;
}
item *rpgitem::additem(item *it) {
	if(it->compare(this))
	{
		quantity += it->quantity;
		return this;
	}
	return NULL;
}

int rpgitem::getitemcount(int i)
{
	if(i == base)
		return quantity;
	return 0;
}

int rpgitem::getcount(item *it)
{
	if(it == this)
		return quantity;
	return 0;
}

float rpgitem::getweight()
{
	return quantity * weight;
}

void rpgitem::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
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

	vel.add(dir.mul(weapon->kickback + (ammo ? ammo->kickback : 0)));

	rpgent::getsignal("hit", false, attacker);
}
