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
		vec dir = vec(floor).rotate(PI / 2, vec(yaw * RAD, 0));
		vectoyawpitch(dir, roll, pitch);
	}
	entities::touchents(this);
}

void rpgitem::render()
{
	rendermodel(temp.mdl, ANIM_MAPMODEL|ANIM_LOOP, vec(o).sub(vec(0, 0, eyeheight)), yaw, pitch, roll, MDL_CULL_DIST|MDL_CULL_OCCLUDED, NULL, NULL, 0, 0, 1, temp.alpha);
}

const char *rpgitem::getname() const
{
	if(name && name[0]) return name;
	return "";
}

void item::init(const char *base, bool world)
{
	if(world) rpgscript::config->setref( (rpgitem *) this, true);
	else rpgscript::config->setref(this, true);

	this->base = game::queryhashpool(base);
	game::loadingitem = this;

	defformatstring(file)("%s/%s.cfg", game::datapath("items"), base);
	execfile(file);

	game::loadingitem = NULL;
	game::loadinguse = NULL;

	rpgscript::config->setnull(true);
}

void rpgitem::init(const char *base)
{
	item::init(base, true);
}

bool item::validate()
{
	if(!game::scripts.access(script))
	{
		ERRORF("Item %p uses invalid script: %s - trying fallback", this, script);
		script = DEFAULTSCR;

		if(!game::scripts.access(script)) return false;
	}

	loopv(uses)
	{
		if(!game::scripts.access(uses[i]->script))
		{
			ERRORF("Item[%p]->uses[%i] uses invalid script: %s - trying fallback", this, i, uses[i]->script);
			uses[i]->script = DEFAULTSCR;

			if(!game::scripts.access(uses[i]->script)) return false;
		}
	}

	return true;
}

bool rpgitem::validate()
{
	return item::validate();
}

item *rpgitem::additem(const char *base, int q)
{
	base = game::hashpool->find(base, NULL);
	if(this->base != base) return NULL;

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

int rpgitem::getitemcount(const char *base)
{
	base = game::hashpool->find(base, NULL);

	return this->base == base ? quantity : 0;
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
		seffects.add(new victimeffect(attacker, weapon->effects[i], weapon->chargeflags, mul));
	}

	if(ammo) loopv(ammo->effects)
	{
		seffects.add(new victimeffect(attacker, ammo->effects[i], weapon->chargeflags, mul));
	}

	vel.add(dir.mul(weapon->kickback + (ammo ? ammo->kickback : 0)));

	rpgent::getsignal("hit", false, attacker);
}
