#include "rpggame.h"

extern bool reloadtexture(const char *name); //texture.cpp

namespace rpgio
{
	#define GAME_VERSION 37
	#define COMPAT_VERSION 37
	#define GAME_MAGIC "RPGS"

	/**
		SAVING STUFF
			STRINGS - use writestring(stream *, char *)
			VECTORS - write an int corresponding to the number of elements, then write the elements
			POINTERS - convert to reference, if this is not possible don't save it
			VEC - use writevec macro, you need to write all 3 coordinates independantly

		LOADING STUFF
			STRINGS - use readstring(stream *)
			VECTORS - read the next int, and use that as the basis of knowing how many elements to load
			POINTERS - read the reference and convert it back to a pointer of the intended object. If this can't be done reliably don't save it
			VEC - use readvec macro, you need to read all 3 coordinates independantly

		REMINDERS
			HOW MANY - don't forget to indicate how many times you need to repeat the loop; there should be a amcro that helps with this
			ORDER - you must read and write items in the same order
			LITTLE ENDIAN - pullil add getlil for single values and lilswap for arrays and objects are going to be your bestest friends
			POINTERS - they change each run, remember that, for convention, use -1 for players
			ARGUMENTS - as a argument to a function, ORDER IS NOT RESPECTED, use with extreme care
			TIME - the difference of lastmillis is taken and applied on save and load - use countdown timers instead if possible.

		ORDER
			they are to coincide with the order of the item structs, and before the map functions, the order in which crap is stored there
	*/

	#define readvec(v) \
	v.x = f->getlil<float>(); \
	v.y = f->getlil<float>(); \
	v.z = f->getlil<float>(); \
	if(DEBUG_IO) \
		conoutf("\fs\f2DEBUG:\fr Read vec (%f, %f, %f) from file", v.x, v.y, v.z);

	#define writevec(v) \
	f->putlil(v.x); \
	f->putlil(v.y); \
	f->putlil(v.z); \
	if(DEBUG_IO) \
		conoutf("\fs\f2DEBUG:\fr Wrote vec (%f, %f, %f) to file", v.x, v.y, v.z);

	#define CHECKEOF(f, val) \
	if((f).end()) \
	{ \
		abort = true; \
		conoutf(CON_ERROR, "\fs\f3ERROR:\fr Unexpected EoF at " __FILE__ ":%i; aborting - You should report this.", __LINE__); \
		return val; \
	}

	struct saveheader
	{
		char magic[4];
		int sversion; //save
		int gversion; //game
	};

	bool abort = false;
	mapinfo *lastmap = NULL;

	rpgent *entfromnum(int num)
	{
		if(num == -2)
			return NULL;
		if(num == -1)
			return game::player1;
		else if(num >= lastmap->objs.length())
		{
			conoutf("\fs\f6WARNING:\fr invalid entity num (%i), possible corruption", num);
			return NULL;
		}
		return lastmap->objs[num];
	}

	int enttonum(rpgent *ent)
	{
		if(ent == NULL)
			return -2;
		if(ent == game::player1)
			return -1;

		int i = lastmap->objs.find(game::player1);
		if(i != -1)
		{
			int num = lastmap->objs.find(ent);
			if(num > i)
				num--;
			return num;
		}
		else
			return lastmap->objs.find(ent);
	}

	struct reference
	{
		int ind;
		mapinfo *map;
		rpgent *&ref;
		reference(int i, mapinfo *m, rpgent *&r) : ind(i), map(m), ref(r) {}
		~reference() {}
	};
	vector<reference> updates;

	const char *readstring(stream *f)
	{
		int len = f->getlil<int>();
		char *s = NULL;
		if(len)
		{
			s = new char[len];
			f->read(s, len);
			if(DEBUG_IO)
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr Read \"%s\" from file (%i)", s, len);
		}

		return s;
	}

	void writestring(stream *f, const char *s)
	{
		int len = s ? strlen(s) + 1 : 0;
		f->putlil(len);

		if(!len) return;
		f->write(s, len);
		if(DEBUG_IO)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr Wrote \"%s\" to file (%i)", s, len);
	}

	void readfaction(stream *f, faction *fact)
	{
		int num = f->getlil<int>();
		if(DEBUG_IO)
			conoutf(CON_DEBUG, "reading %i relations", num);

		loopi(num)
		{
			CHECKEOF(*f, )

			int val = f->getlil<short>();
			if(!fact->relations.inrange(i))
				fact->relations.add(val);
			else
				fact->relations[i] = val;
		}
	}

	void writefaction(stream *f, faction *fact)
	{
		f->putlil(fact->relations.length());
		if(DEBUG_IO)
			conoutf(CON_DEBUG, "saving %i relations", fact->relations.length());

		loopv(fact->relations)
			f->putlil(fact->relations[i]);
	}

	void readrecipe(stream *f, recipe *rec)
	{
		rec->flags |= f->getlil<int>();
	}

	void writerecipe(stream *f, recipe *rec)
	{
		f->putlil( rec->flags & recipe::SAVE);
	}

	void readmerchant(stream *f, merchant *mer)
	{
		mer->credit = f->getlil<int>();
	}

	void writemerchant(stream *f, merchant *mer)
	{
		f->putlil(mer->credit);
	}

	item *readitem(stream *f, item *it = NULL)
	{
		if(!it) it = new item();

		delete[] it->name;
		delete[] it->icon;
		delete[] it->description;
		delete[] it->mdl;

		it->name = readstring(f);
		it->icon = readstring(f);
		it->description = readstring(f);
		it->mdl = readstring(f);

		it->quantity = f->getlil<int>();
		it->base = f->getlil<int>();

		it->script = f->getlil<int>();
		it->category = f->getlil<int>();
		it->flags = f->getlil<int>();
		it->value = f->getlil<int>();
		it->maxdurability = f->getlil<int>();
		it->weight = f->getlil<float>();
		it->durability = f->getlil<float>();
		it->recovery = f->getlil<float>();

		int uses = f->getlil<int>();
		loopi(uses)
		{
			CHECKEOF(*f,it)
			int type = f->getlil<int>();
			use *u = NULL;
			switch(type)
			{
				case USE_WEAPON:
				{
					if(!u) u = new use_weapon(0);
					use_weapon *wp = (use_weapon *) u;

					wp->range = f->getlil<int>();
					wp->angle = f->getlil<int>();
					wp->lifetime = f->getlil<int>();
					wp->gravity = f->getlil<int>();
					wp->projeffect = f->getlil<int>();
					wp->traileffect = f->getlil<int>();
					wp->deatheffect = f->getlil<int>();
					wp->cost = f->getlil<int>();
					wp->pflags = f->getlil<int>();
					wp->ammo = f->getlil<int>();
					wp->target = f->getlil<int>();
					wp->radius = f->getlil<int>();
					wp->kickback = f->getlil<int>();
					wp->recoil = f->getlil<int>();
					wp->charge = f->getlil<int>();

					wp->basecharge = f->getlil<float>();
					wp->mincharge = f->getlil<float>();
					wp->maxcharge = f->getlil<float>();
					wp->elasticity = f->getlil<float>();
					wp->speed = f->getlil<float>();
				}
				case USE_ARMOUR:
				{
					if(!u) u = new use_armour(0);
					use_armour *ar = (use_armour *) u;

					delete[] ar->vwepmdl;
					delete[] ar->hudmdl;

					ar->vwepmdl = readstring(f);
					ar->hudmdl = readstring(f);

					ar->idlefx = f->getlil<int>();
					ar->slots = f->getlil<int>();
					ar->skill = f->getlil<int>();

					loopj(STAT_MAX) ar->reqs.attrs[j] = f->getlil<short>();
					loopj(SKILL_MAX) ar->reqs.skills[j] = f->getlil<short>();
				}
				case USE_CONSUME:
				{
					if(!u) u = new use(0);

					delete[] u->name;
					delete[] u->description;
					delete[] u->icon;

					u->name = readstring(f);
					u->description = readstring(f);
					u->icon = readstring(f);

					u->script = f->getlil<int>();
					u->cooldown = f->getlil<int>();
					u->chargeflags = f->getlil<int>();

					int efx = f->getlil<int>();
					loopj(efx)
					{
						CHECKEOF(*f, it)
						int s = f->getlil<int>();
						int e = f->getlil<int>();
						float m = f->getlil<float>();
						u->effects.add(new inflict(s, e, m));
					}
					break;
				}
			}
			it->uses.add(u);
		}

		return it;
	}

	void writeitem(stream *f, item *it)
	{
		writestring(f, it->name);
		writestring(f, it->icon);
		writestring(f, it->description);
		writestring(f, it->mdl);

		f->putlil(it->quantity);
		f->putlil(it->base);

		f->putlil(it->script);
		f->putlil(it->category);
		f->putlil(it->flags);
		f->putlil(it->value);
		f->putlil(it->maxdurability);
		f->putlil(it->weight);
		f->putlil(it->durability);
		f->putlil(it->recovery);

		f->putlil(it->uses.length());
		loopv(it->uses)
		{
			f->putlil(it->uses[i]->type);
			switch(it->uses[i]->type)
			{
				case USE_WEAPON:
				{
					use_weapon *wp = (use_weapon *) it->uses[i];

					f->putlil(wp->range);
					f->putlil(wp->angle);
					f->putlil(wp->lifetime);
					f->putlil(wp->gravity);
					f->putlil(wp->projeffect);
					f->putlil(wp->traileffect);
					f->putlil(wp->deatheffect);
					f->putlil(wp->cost);
					f->putlil(wp->pflags);
					f->putlil(wp->ammo);
					f->putlil(wp->target);
					f->putlil(wp->radius);
					f->putlil(wp->kickback);
					f->putlil(wp->recoil);
					f->putlil(wp->charge);

					f->putlil(wp->basecharge);
					f->putlil(wp->mincharge);
					f->putlil(wp->maxcharge);
					f->putlil(wp->elasticity);
					f->putlil(wp->speed);
					//fallthrough
				}
				case USE_ARMOUR:
				{
					use_armour *ar = (use_armour *) it->uses[i];

					writestring(f, ar->vwepmdl);
					writestring(f, ar->hudmdl);

					f->putlil(ar->idlefx);
					f->putlil(ar->slots);
					f->putlil(ar->skill);

					loopj(STAT_MAX) f->putlil<short>(ar->reqs.attrs[j]);
					loopj(SKILL_MAX)  f->putlil<short>(ar->reqs.skills[j]);
					//fallthrough
				}
				case USE_CONSUME:
				{
					use *u = it->uses[i];

					writestring(f, u->name);
					writestring(f, u->description);
					writestring(f, u->icon);

					f->putlil(u->script);
					f->putlil(u->cooldown);
					f->putlil(u->chargeflags);

					f->putlil(u->effects.length());
					loopvj(u->effects)
					{
						f->putlil(u->effects[j]->status);
						f->putlil(u->effects[j]->element);
						f->putlil(u->effects[j]->mul);
					}
					break;
				}
			}
		}
	}

	rpgent *readent(stream *f, rpgent *ent = NULL)
	{
		int type = f->getlil<int>();
		if(ent)
			type = ent->type();

		switch(type)
		{
			case ENT_CHAR:
			{
				if(!ent) ent = new rpgchar();
				rpgchar *loading = (rpgchar *) ent;

				delete[] loading->name;
				delete[] loading->mdl;
				delete[] loading->portrait;

				loading->name = readstring(f);
				loading->mdl = readstring(f);
				loading->portrait = readstring(f);

				#define x(var, type) loading->base.var = f->getlil<type>();

				x(experience, int)
				x(level, int)
				x(statpoints, int)
				x(skillpoints, int)
				loopi(STAT_MAX) x(baseattrs[i], short)
				loopi(SKILL_MAX) x(baseskills[i], short)
				loopi(STAT_MAX) x(deltaattrs[i], short)
				loopi(SKILL_MAX) x(deltaskills[i], short)

				loopi(ATTACK_MAX) x(bonusthresh[i], short)
				loopi(ATTACK_MAX) x(bonusresist[i], short)
				x(bonushealth, int)
				x(bonusmana, int)
				x(bonusmovespeed, int)
				x(bonusjumpvel, int)
				x(bonuscarry, int)
				x(bonuscrit, int)
				x(bonushregen, float)
				x(bonusmregen, float)

				loopi(ATTACK_MAX) x(deltathresh[i], short)
				loopi(ATTACK_MAX) x(deltaresist[i], short)
				x(deltahealth, int)
				x(deltamana, int)
				x(deltamovespeed, int)
				x(deltajumpvel, int)
				x(deltacarry, int)
				x(deltacrit, int)
				x(deltahregen, float)
				x(deltamregen, float)

				#undef x

				loading->script = f->getlil<int>();
				loading->faction = f->getlil<int>();
				loading->merchant = f->getlil<int>();
				loading->health = f->getlil<float>();
				loading->mana = f->getlil<float>();
				loading->lastaction = f->getlil<int>() + lastmillis;

				vector<item *> items;
				int num = f->getlil<int>();
				loopi(num)
				{
					CHECKEOF(*f, ent)
					item *it = items.add(readitem(f));
					loading->inventory.access(it->base, vector<item *>()).add(it);
				}

				int equiplen = f->getlil<int>();
				loopi(equiplen)
				{
					CHECKEOF(*f, ent)
					int idx = f->getlil<int>();
					int use = f->getlil<int>();

					//validate equipment
					items[idx]->quantity++;
					loading->equip(items[idx], use);
				}

				break;
			}
			case ENT_ITEM:
			{
				if(!ent) ent = new rpgitem();
				rpgitem *loading = (rpgitem *) ent;

				readitem(f, loading);

				break;
			}
			case ENT_OBSTACLE:
			{
				if(!ent) ent = new rpgobstacle();
				rpgobstacle *loading = (rpgobstacle *) ent;

				delete loading->mdl;
				loading->mdl = readstring(f);
				loading->weight = f->getlil<int>();
				loading->script = f->getlil<int>();
				loading->flags = f->getlil<int>();

				break;
			}
			case ENT_CONTAINER:
			{
				if(!ent) ent = new rpgcontainer();
				rpgcontainer *loading = (rpgcontainer *) ent;

				delete[] loading->mdl;
				delete[] loading->name;

				loading->mdl = readstring(f);
				loading->name = readstring(f);

				loading->capacity = f->getlil<int>();
				loading->faction = f->getlil<int>();
				loading->merchant = f->getlil<int>();
				loading->script = f->getlil<int>();
				loading->lock = f->getlil<int>();
				loading->magelock = f->getlil<int>();

				int items = f->getlil<int>();
				loopi(items)
				{
					CHECKEOF(*f, ent)
					item *it = readitem(f);
					loading->inventory.access(it->base, vector<item *>()).add(it);
				}

				break;
			}
			case ENT_PLATFORM:
			{
				if(!ent) ent = new rpgplatform();
				rpgplatform *loading = (rpgplatform *) ent;

				delete[] loading->mdl;

				loading->mdl = readstring(f);
				loading->speed = f->getlil<int>();
				loading->script = f->getlil<int>();
				loading->flags = f->getlil<int>();

				int steps = f->getlil<int>();
				loopi(steps)
				{
					CHECKEOF(*f, ent)
					vector<int> &detours = loading->routes.access(f->getlil<int>(), vector<int>());
					int routes = f->getlil<int>();
					loopj(routes) detours.add(f->getlil<int>());
				}


				loading->target = f->getlil<int>();

				break;
			}
			case ENT_TRIGGER:
			{
				if(!ent) ent = new rpgtrigger();
				rpgtrigger *loading = (rpgtrigger *) ent;

				delete[] loading->mdl;
				delete[] loading->name;

				loading->mdl = readstring(f);
				loading->name = readstring(f);
				loading->script = f->getlil<int>();
				loading->flags = f->getlil<int>();
				loading->lasttrigger = f->getlil<int>() + lastmillis;

				break;
			}
			default:
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr unknown entity type %i", type);
				abort = true;
				return NULL;
		}

		int numeffs = f->getlil<int>();
		loopi(numeffs)
		{
			CHECKEOF(*f, ent)
			victimeffect *eff = new victimeffect();
			ent->seffects.add(eff);

			updates.add(reference(f->getlil<int>(), lastmap, eff->owner));
			eff->group = f->getlil<int>();
			eff->elem = f->getlil<int>();
			int numstat = f->getlil<int>();
			loopj(numstat)
			{
				CHECKEOF(*f, ent)
				int type = f->getlil<int>();
				status *st = NULL;
				switch(type)
				{
					case STATUS_POLYMORPH:
					{
						status_polymorph *poly = new status_polymorph();
						st = poly;

						poly->mdl = readstring(f);
						break;
					}
					case STATUS_LIGHT:
					{
						status_light *light = new status_light();
						st = light;

						readvec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						status_script *scr = new status_script();
						st = scr;

						scr->script = readstring(f);
						break;
					}
					case STATUS_SIGNAL:
					{
						status_signal *sig = new status_signal();
						st = sig;

						sig->signal = readstring(f);
						break;
					}
					default:
						st = new status_generic();
						break;
				}
				st->type = type;
				st->duration = f->getlil<int>();
				st->remain = f->getlil<int>();
				st->strength = f->getlil<int>();
				st->variance = f->getlil<float>();

				eff->effects.add(st);
			}
		}

		ent->eyeheight = f->getlil<float>();
		readvec(ent->o);
		ent->newpos = ent->o;
		readvec(ent->vel);
		readvec(ent->falling);

		ent->yaw = f->getlil<float>();
		ent->pitch = f->getlil<float>();
		ent->roll = f->getlil<float>();

		ent->timeinair = f->getlil<int>();

		ent->state = f->getchar();
		ent->editstate = f->getchar();

		ent->lastjump = f->getlil<int>() + lastmillis;

		return ent;
	}

	void writeent(stream *f, rpgent *d)
	{
		f->putlil(d->type());

		switch(d->type())
		{
			case ENT_CHAR:
			{
				rpgchar *saving = (rpgchar *) d;

				writestring(f, saving->name);
				writestring(f, saving->mdl);
				writestring(f, saving->portrait);

				#define x(var) f->putlil(saving->base.var);

				x(experience)
				x(level)
				x(statpoints)
				x(skillpoints)
				loopi(STAT_MAX) x(baseattrs[i])
				loopi(SKILL_MAX) x(baseskills[i])
				loopi(STAT_MAX) x(deltaattrs[i])
				loopi(SKILL_MAX) x(deltaskills[i])

				loopi(ATTACK_MAX) x(bonusthresh[i])
				loopi(ATTACK_MAX) x(bonusresist[i])
				x(bonushealth)
				x(bonusmana)
				x(bonusmovespeed)
				x(bonusjumpvel)
				x(bonuscarry)
				x(bonuscrit)
				x(bonushregen)
				x(bonusmregen)

				loopi(ATTACK_MAX) x(deltathresh[i])
				loopi(ATTACK_MAX) x(deltaresist[i])
				x(deltahealth)
				x(deltamana)
				x(deltamovespeed)
				x(deltajumpvel)
				x(deltacarry)
				x(deltacrit)
				x(deltahregen)
				x(deltamregen)

				#undef x

				f->putlil(saving->script);
				f->putlil(saving->faction);
				f->putlil(saving->merchant);
				f->putlil(saving->health);
				f->putlil(saving->mana);
				f->putlil(saving->lastaction - lastmillis);

				vector<item *> items;
				enumerate(saving->inventory, vector<item *>, stack,
					loopvj(stack) items.add(stack[j]);
				)

				f->putlil(items.length());
				loopv(items) writeitem(f, items[i]);

				f->putlil(saving->equipped.length());
				loopv(saving->equipped)
				{
					f->putlil(items.find(saving->equipped[i]->it));
					f->putlil(saving->equipped[i]->use);
				}

				break;
			}
			case ENT_ITEM:
			{
				rpgitem *saving = (rpgitem *) d;

				writeitem(f, saving);

				break;
			}
			case ENT_OBSTACLE:
			{
				rpgobstacle *saving = (rpgobstacle *) d;

				writestring(f, saving->mdl);
				f->putlil(saving->weight);
				f->putlil(saving->script);
				f->putlil(saving->flags);

				break;
			}
			case ENT_CONTAINER:
			{
				rpgcontainer *saving = (rpgcontainer *) d;

				writestring(f, saving->mdl);
				writestring(f, saving->name);

				f->putlil(saving->capacity);
				f->putlil(saving->faction);
				f->putlil(saving->merchant);
				f->putlil(saving->script);
				f->putlil(saving->lock);
				f->putlil(saving->magelock);

				vector<item *> items;
				enumerate(saving->inventory, vector<item *>, stack,
					loopvj(stack) items.add(stack[j]);
				)

				f->putlil(items.length());
				loopv(items) writeitem(f, items[i]);

				break;
			}
			case ENT_PLATFORM:
			{
				rpgplatform *saving = (rpgplatform *) d;

				writestring(f, saving->mdl);
				f->putlil(saving->speed);
				f->putlil(saving->script);
				f->putlil(saving->flags);

				f->putlil(saving->routes.length());

				enumeratekt(saving->routes, int, stop, vector<int>, routes,
					f->putlil(stop);
					f->putlil(routes.length());
					loopvj(routes)
						f->putlil(routes[j]);
				);

				f->putlil(saving->target);

				break;
			}
			case ENT_TRIGGER:
			{
				rpgtrigger *saving = (rpgtrigger *) d;

				writestring(f, saving->mdl);
				writestring(f, saving->name);
				f->putlil(saving->script);
				f->putlil(saving->flags);
				f->putlil(saving->lasttrigger - lastmillis);

				break;
			}
			default:
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr unsupported ent type %i, aborting", d->type());
				return;
		}

		f->putlil(d->seffects.length());
		loopv(d->seffects)
		{
			f->putlil(enttonum(d->seffects[i]->owner));
			f->putlil(d->seffects[i]->group);
			f->putlil(d->seffects[i]->elem);
			f->putlil(d->seffects[i]->effects.length());

			loopvj(d->seffects[i]->effects)
			{
				status *st = d->seffects[i]->effects[j];
				f->putlil(st->type);
				switch(st->type)
				{
					case STATUS_POLYMORPH:
					{
						status_polymorph *poly = (status_polymorph *) st;

						writestring(f, poly->mdl);
						break;
					}
					case STATUS_LIGHT:
					{
						status_light *light = (status_light *) st;

						writevec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						status_script *scr = (status_script *) st;

						writestring(f, scr->script);
						break;
					}
					case STATUS_SIGNAL:
					{
						status_signal *sig = (status_signal *) st;

						writestring(f, sig->signal);
						break;
					}
				}
				f->putlil(st->duration);
				f->putlil(st->remain);
				f->putlil(st->strength);
				f->putlil(st->variance);
			}
		}

		f->putlil(d->eyeheight);
		writevec(d->o);
		writevec(d->vel);
		writevec(d->falling);

		f->putlil(d->yaw);
		f->putlil(d->pitch);
		f->putlil(d->roll);

		f->putlil(d->timeinair);

		f->putchar(d->state);
		f->putchar(d->editstate);

		f->putlil(d->lastjump - lastmillis);
	}

	mapinfo *readmap(stream *f)
	{
		const char *name = readstring(f);
		mapinfo *loading = game::accessmap(name);
		lastmap = loading;

		loading->name = name;
		loading->script = f->getlil<int>();
		loading->flags = f->getlil<int>();
		loading->loaded = f->getchar();

		int numobjs = f->getlil<int>(),
		numactions = f->getlil<int>(),
		numprojs = f->getlil<int>(),
		numaeffects = f->getlil<int>(),
		numblips = f->getlil<int>();

		loopi(numobjs)
		{
			CHECKEOF(*f, loading)
			loading->objs.add(readent(f));
		}

		loopvrev(updates)
		{
			CHECKEOF(*f, loading)
			if(updates[i].map == lastmap)
				updates[i].ref = entfromnum(updates[i].ind);
		}

		loopi(numactions)
		{
			CHECKEOF(*f, loading)
			action *act = NULL;
			int type = f->getlil<int>();

			switch(type)
			{
				case ACTION_TELEPORT:
				{
					int ent = f->getlil<int>();
					if(!entfromnum(ent))
					{//how'd that happen?
					conoutf("\fs\f6WARNING:\fr loaded teleport loadaction for invalid ent? ignoring");
					f->getlil<int>();
					continue;
					}
					act = new action_teleport(entfromnum(ent), f->getlil<int>());
					break;
				}
				case ACTION_SPAWN:
				{
					int tag = f->getlil<int>(),
						ent = f->getlil<int>(),
						id = f->getlil<int>(),
						amount = f->getlil<int>(),
						qty = f->getlil<int>();
						act = new action_spawn(tag, ent, id, amount, qty);
						break;
				}
				case ACTION_SCRIPT:
				{
					act = new action_script(readstring(f));
					break;
				}
			}

			loading->loadactions.add(act);
		}

		loopk(numprojs)
		{
			CHECKEOF(*f, loading)
			projectile *p = new projectile();

			p->owner = (rpgchar *) entfromnum(f->getlil<int>());
			if(p->owner && p->owner->type() != ENT_CHAR)
				p->owner = NULL;

			int wep = f->getlil<int>();
			int ammo = f->getlil<int>();
			int use = f->getlil<int>();
			int ause = f->getlil<int>();

			if(p->owner)
			{
				enumerate(p->owner->inventory, vector<item *>, stack,
					if(stack.inrange(wep))
						p->item = equipment(stack[wep], use);
					if(stack.inrange(ammo))
						p->ammo = equipment(stack[ammo], ause);

					wep -= stack.length();
					ammo -= stack.length();
				)
			}

			readvec(p->o); readvec(p->dir); readvec(p->emitpos);
			p->lastemit = 0; //should emit immediately
			p->gravity = f->getlil<int>();
			p->deleted = f->getchar();

			p->pflags = f->getlil<int>();
			p->time = f->getlil<int>();
			p->dist = f->getlil<int>();

			p->projfx = f->getlil<int>();
			p->trailfx = f->getlil<int>();
			p->deathfx = f->getlil<int>();
			p->radius = f->getlil<int>();
			p->elasticity = f->getlil<float>();
			p->charge = f->getlil<float>();
			p->chargeflags = f->getlil<int>();

			loading->projs.add(p);
		}

		loopi(numaeffects)
		{
			CHECKEOF(*f, loading)
			areaeffect *aeff = new areaeffect();
			loading->aeffects.add(aeff);

			aeff->owner = entfromnum(f->getlil<int>());
			readvec(aeff->o);
			aeff->lastemit = 0; //should emit immediately
			aeff->group = f->getlil<int>();
			aeff->elem = f->getlil<int>();
			aeff->radius = f->getlil<int>();
			aeff->fx = f->getlil<int>();

			int numstat = f->getlil<int>();
			loopj(numstat)
			{
				CHECKEOF(*f, loading)
				int type = f->getlil<int>();
				status *st = NULL;
				switch(type)
				{
					case STATUS_POLYMORPH:
					{
						status_polymorph *poly = new status_polymorph();
						st = poly;

						poly->mdl = readstring(f);
						break;
					}
					case STATUS_LIGHT:
					{
						status_light *light = new status_light();
						st = light;

						readvec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						status_script *scr = new status_script();
						st = scr;

						scr->script = readstring(f);
						break;
					}
					case STATUS_SIGNAL:
					{
						status_signal *sig = new status_signal();
						st = sig;

						sig->signal = readstring(f);
						break;
					}
					default:
						st = new status_generic();
						break;
				}
				st->type = type;
				st->duration = f->getlil<int>();
				st->remain = f->getlil<int>();
				st->strength = f->getlil<int>();
				st->variance = f->getlil<float>();

				aeff->effects.add(st);
			}
		}

		loopi(numblips)
		{
			///FIXME finalize blip structure and write me
		}

		lastmap = NULL;
		return loading;
	}

	void writemap(stream *f, mapinfo *saving)
	{
		lastmap = saving;

		writestring(f, saving->name);
		f->putlil(saving->script);
		f->putlil(saving->flags);
		f->putchar(saving->loaded);

		f->putlil(saving->objs.length());
		f->putlil(saving->loadactions.length());
		f->putlil(saving->projs.length());
		f->putlil(saving->aeffects.length());
		f->putlil(saving->blips.length());

		loopv(saving->objs)
		{
			writeent(f, saving->objs[i]);
		}

		loopv(saving->loadactions)
		{
			f->putlil(saving->loadactions[i]->type());

			switch(saving->loadactions[i]->type())
			{
				case ACTION_TELEPORT:
				{
					action_teleport *act = (action_teleport *) saving->loadactions[i];
					f->putlil(enttonum(act->ent));
					f->putlil(act->dest);

					break;
				}
				case ACTION_SPAWN:
				{
					action_spawn *spw = (action_spawn *) saving->loadactions[i];
					f->putlil(spw->tag);
					f->putlil(spw->ent);
					f->putlil(spw->id);
					f->putlil(spw->amount);
					f->putlil(spw->qty);

					break;
				}
				case ACTION_SCRIPT:
					writestring(f, ((action_script *) saving->loadactions[i])->script);
					break;
			}
		}

		loopv(saving->projs)
		{
			projectile *p = saving->projs[i];
			f->putlil(enttonum(p->owner));

			int offset = 0;
			int wep = -1;
			int ammo = -1;

			if(p->owner)
			{
				enumerate(p->owner->inventory, vector<item *>, stack,
					if(stack.find(p->item.it) >= 0)
						wep = stack.find(p->item.it) + offset;
					if(stack.find(p->ammo.it) >= 0)
						ammo = stack.find(p->ammo.it) + offset;

					offset += stack.length();
				)
			}

			f->putlil(wep);
			f->putlil(ammo);
			f->putlil(p->item.use);
			f->putlil(p->ammo.use);

			writevec(p->o); writevec(p->dir); writevec(p->emitpos);
			//f->putlil(p->lastemit);
			f->putlil(p->gravity);
			f->putchar(p->deleted);

			f->putlil(p->pflags);
			f->putlil(p->time);
			f->putlil(p->dist);

			f->putlil(p->projfx);
			f->putlil(p->trailfx);
			f->putlil(p->deathfx);
			f->putlil(p->radius);

			f->putlil(p->elasticity);
			f->putlil(p->charge);
			f->putlil(p->chargeflags);
		}

		loopv(saving->aeffects)
		{
			areaeffect *aeff = saving->aeffects[i];

			f->putlil(enttonum(aeff->owner));
			writevec(aeff->o);
			f->putlil(aeff->group);
			f->putlil(aeff->elem);
			f->putlil(aeff->radius);
			f->putlil(aeff->fx);

			f->putlil(aeff->effects.length());
			loopvj(aeff->effects)
			{
				status *st = aeff->effects[i];
				f->putlil(st->type);
				switch(st->type)
				{
					case STATUS_POLYMORPH:
					{
						status_polymorph *poly = (status_polymorph *) st;

						writestring(f, poly->mdl);
						break;
					}
					case STATUS_LIGHT:
					{
						status_light *light = (status_light *) st;

						writevec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						status_script *scr = (status_script *) st;

						writestring(f, scr->script);
						break;
					}
					case STATUS_SIGNAL:
					{
						status_signal *sig = (status_signal *) st;

						writestring(f, sig->signal);
						break;
					}
				}
				f->putlil(st->duration);
				f->putlil(st->remain);
				f->putlil(st->strength);
				f->putlil(st->variance);
			}
		}

		loopv(saving->blips)
		{
			///FIXME finalize blip structure and write me
		}
		lastmap = NULL;
	}

	//don't mind the ::blah, just a namespace collision with rpgio:: when we want it from the global scope
	void writereferences(stream *f, vector<mapinfo *> &maps, hashset< ::reference> &stack)
	{
		f->putlil(stack.length());

		enumerate(stack, ::reference, saving,
			writestring(f, saving.name);

			f->putlil(saving.list.length());
			loopvj(saving.list)
			{
				::reference::ref &sav = saving.list[j];
				char type = sav.type;
				if(sav.ptr == NULL)
				{
					if(DEBUG_IO)
						conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr reference %s:%i of type %i is null, saving as T_INVALID", saving.name, j, type);
					type = ::reference::T_INVALID;
				}
				switch(type)
				{
					case ::reference::T_CHAR:
					case ::reference::T_ITEM:
					case ::reference::T_OBSTACLE:
					case ::reference::T_CONTAINER:
					case ::reference::T_PLATFORM:
					case ::reference::T_TRIGGER:
					{
						int map = -1;
						int ent = -1;
						if(sav.ptr == game::player1) { map = ent = -1; }
						else loopvj(maps)
						{
							ent = maps[j]->objs.find(sav.ptr);
							if(ent >= 0) {map = j; break;}
						}
						if(map < 0 && ent < 0 && sav.ptr != game::player1)
						{
							conoutf(CON_WARN, "\fs\f6WARNING:\fr char/item/object reference \"%s\" points to non-player entity that does not exist");
							f->putchar(::reference::T_INVALID);
							continue;
						}

						if(DEBUG_IO)
							conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr writing reference %s as rpgent reference: %i %i", saving.name, map, ent);
						f->putchar(type);
						f->putlil(map);
						f->putlil(ent);

						break;
					}
					case ::reference::T_MAP:
					{
						f->putchar(type);
						int map = maps.find(sav.ptr);
						f->putlil(map);
						if(DEBUG_IO) conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr writing reference %s as map reference: %i", saving.name, map);
						break;
					}
					case ::reference::T_INV:
					{
						int map = -1;
						int ent = -1;
						int base = saving.getinv(j)->base;
						int offset = -1;

						{
							vector <item *> *stack = game::player1->inventory.access(base);
							if(stack) offset = stack->find(sav.ptr);
						}
						if(offset < 0) loopv(maps)
						{
							loopvj(maps[i]->objs)
							{
								vector<item *> *stack = NULL;
								if(maps[i]->objs[j]->type() == ENT_CHAR)
									stack = ((rpgchar *) maps[i]->objs[j])->inventory.access(base);
								else if(maps[i]->objs[j]->type() == ENT_CONTAINER)
									stack = ((rpgcontainer *) maps[i]->objs[j])->inventory.access(base);

								if(stack) offset = stack->find(sav.ptr);
								if(offset >= 0) { ent = j; break; }
							}
							if(offset >= 0) { map = i; break; }
						}
						if(offset < 0)
						{
							conoutf(CON_WARN, "\fs\f6WARNING:\fr inv reference \"%s:%i\" points to an item that does not exist", saving.name, j);
							f->putchar(::reference::T_INVALID);
							continue;
						}

						if(DEBUG_IO) conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr writing reference \"%s:%i\" as type T_INV with indices: %i %i %i %i", saving.name, j, map, ent, base, offset);

						f->putchar(type);
						f->putlil(map);
						f->putlil(ent);
						f->putlil(base);
						f->putlil(offset);

						break;
					}

					//
					default:
						conoutf(CON_ERROR, "\fs\f3ERROR:\fr unsupported reference type %i for reference %s, saving as T_INVALID", sav.type, saving.name);
					// Temporary reference types below this line...
					case ::reference::T_EQUIP:
					case ::reference::T_VEFFECT:
					case ::reference::T_AEFFECT:
						type = ::reference::T_INVALID;
					case ::reference::T_INVALID:
						if(DEBUG_IO) conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr writing null reference %s", saving.name);
						f->putchar(type);
						break;
				}
			}
		)
	}

	void readreferences(stream *f, vector<mapinfo *> &maps, hashset< ::reference> &stack)
	{
		static ::reference dummy("");
		int num = f->getlil<int>();
		loopi(num)
		{
			CHECKEOF(*f, )
			const char *name = readstring(f);
			::reference *loading = &stack.access(name, dummy);
			if(dummy.name != loading->name)
				delete[] name;
			else
				loading->name = name;

			int len = f->getlil<int>();
			loopj(len)
			{
				char type = f->getchar();
				switch(type)
				{
					case ::reference::T_CHAR:
					case ::reference::T_ITEM:
					case ::reference::T_OBSTACLE:
					case ::reference::T_CONTAINER:
					case ::reference::T_PLATFORM:
					case ::reference::T_TRIGGER:
					{
						int map = f->getlil<int>();
						int ent = f->getlil<int>();

						if(map == -1 && ent == -1)
						{
							if(DEBUG_IO) conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr reading player char reference %s", loading->name);
							loading->pushref(game::player1);
						}
						else if(maps.inrange(map) && maps[map]->objs.inrange(ent))
						{
							if(DEBUG_IO) conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr reading valid rpgent reference %s: %i %i", loading->name, map, ent);
							loading->pushref(maps[map]->objs[ent]);
						}
						else conoutf(CON_WARN, "\fs\f6WARNING:\fr rpgent reference %s: %i %i - indices out of range", loading->name, map, ent);

						break;
					}
					case ::reference::T_MAP:
					{
						int map = f->getlil<int>();
						if(DEBUG_IO) conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr reading map reference %s: %i", loading->name, map);
						if(maps.inrange(map)) loading->pushref(maps[map]);
						break;
					}
					case ::reference::T_INV:
					{
						int map = f->getlil<int>();
						int ent = f->getlil<int>();
						int base = f->getlil<int>();
						int offset = f->getlil<int>();

						if(DEBUG_IO) conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr reading T_INV reference with values %i %i %i %i...", map, ent, base, offset);
						vector <item *> *stack = NULL;

						if(map == -1 && ent == -1)
							stack = game::player1->inventory.access(base);
						else if (maps.inrange(map) && maps[map]->objs.inrange(ent))
						{
							if(maps[map]->objs[ent]->type() == ENT_CHAR)
								stack = ((rpgchar *) maps[map]->objs[ent])->inventory.access(base);
							else if(maps[map]->objs[ent]->type() == ENT_CONTAINER)
								stack = ((rpgcontainer *) maps[map]->objs[ent])->inventory.access(base);
						}

						if(stack && stack->inrange(offset))
						{
							if(DEBUG_IO) conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr Loading T_INV reference to %p successfully", (*stack)[offset]);
							loading->pushref((*stack)[offset]);
						}
						else
							conoutf(CON_WARN, "\fs\f6WARNING:\fr T_INV has out of range values: %i %i %i %i, loading failed", map, ent, base, offset);

						break;
					}

					//Temporary types below this line
					case ::reference::T_EQUIP:
					case ::reference::T_VEFFECT:
					case ::reference::T_AEFFECT:
					case ::reference::T_INVALID:
						if(DEBUG_IO) conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr reading now null reference %s - ignoring", loading->name);
						break;
					default:
						conoutf(CON_ERROR, "\fs\f3ERROR:\fr unsupported reference type %i for reference %s, this will cause issues; aborting", type, loading->name);
						abort = true;
						return;
				}
			}
		}
	}

	void readdelayscript(stream *f, vector<mapinfo *> &maps, delayscript *loading)
	{
		readreferences(f, maps, loading->refs);
		loading->script = readstring(f);
		loading->remaining = f->getlil<int>();
	}

	void writedelayscript(stream *f, vector<mapinfo *> &maps, delayscript *saving)
	{
		writereferences(f, maps, saving->refs);
		writestring(f, saving->script);
		f->putlil(saving->remaining);
	}

	void readjournal(stream *f)
	{
		const char *name = readstring(f);
		journal *journ = game::journals.access(name);

		if(journ)
		{
			conoutf(CON_WARN, "\fs\f6WARNING:\fr additional instance of journal %s exists, merging", name);
			delete[] name;
		}
		else
		{
			journ = &game::journals.access(name, journal());
			journ->name = name;
		}

		int entries = f->getlil<int>();
		loopi(entries)
			journ->entries.add(readstring(f));
	}

	void writejournal(stream *f, journal *saving)
	{
		writestring(f, saving->name);
		f->putlil(saving->entries.length());
		loopv(saving->entries)
			writestring(f, saving->entries[i]);
	}

	void loadgame(const char *name)
	{
		defformatstring(file)("data/rpg/saves/%s.sgz", name);
		stream *f = opengzfile(file, "rb");

		if(!f)
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr unable to read file: %s", file);
			return;
		}

		saveheader hdr;
		f->read(&hdr, sizeof(saveheader));
		lilswap(&hdr.sversion, 2);

		if(hdr.sversion < COMPAT_VERSION || hdr.sversion > GAME_VERSION || strncmp(hdr.magic, GAME_MAGIC, 4))
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr Unsupported version or corrupt save: %i (%i) - %4.4s (%s)", hdr.sversion, GAME_VERSION, hdr.magic, GAME_MAGIC);
			delete f;
			return;
		}
		if(DEBUG_IO)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr supported save: %i  %4.4s", hdr.sversion, hdr.magic);

		const char *data = readstring(f);
		game::newgame(data, true);
		delete[] data;
		abort = false;

		if(game::compatversion > hdr.gversion)
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr saved game is of game version %i, last compatible version is %i; aborting", hdr.gversion, game::compatversion);
			delete f;
			localdisconnect();
			return;
		}

		const char *curmap = readstring(f);
		if(!curmap)
		{
			delete f;
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr no game/map in progress? aborting");
			localdisconnect();
			return;
		}

		lastmap = game::accessmap(curmap);
		readent(f, game::player1);

		int num;
		#define READ(m, b) \
			num = f->getlil<int>(); \
			loopi(num) \
			{ \
				if(abort) break; \
				if(f->end()) \
				{ \
					conoutf(CON_ERROR, "\fs\f3ERROR:\fr unexpected EoF, aborting"); \
					abort = true; break; \
				} \
				if(DEBUG_IO) \
					conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr reading " #m " %i of %i", i + 1, num); \
				b; \
			}

		///TODO redo hotkeys
// 		READ(hotkey,
// 			int b = f->getlil<int>();
// 			int u = f->getlil<int>();
// 			game::hotkeys.add(equipment(b, u));
// 		)

		READ(faction,
			if(!game::factions.inrange(i)) game::factions.add(new faction());
			readfaction(f, game::factions[i]);
		);
		READ(recipe,
			if(!game::recipes.inrange(i)) game::recipes.add(new recipe());
			readrecipe(f, game::recipes[i]);
		);
		READ(merchant,
			if(!game::merchants.inrange(i)) game::merchants.add(new merchant());
			readmerchant(f, game::merchants[i]);
		);
		READ(variable,
			if(!game::variables.inrange(i)) game::variables.add(0);
			game::variables[i] = readstring(f);
		);
		vector<mapinfo *> maps;
		READ(mapinfo, maps.add(readmap(f)));

		READ(reference stack,
			if(!rpgscript::stack.inrange(i)) rpgscript::pushstack();
			readreferences(f, maps, *rpgscript::stack[i]);
		)

		READ(delayscript stack,
			rpgscript::delaystack.add(new delayscript());
			readdelayscript(f, maps, rpgscript::delaystack[i]);
		)

		READ(journal bucket,
			readjournal(f);
		)

		#undef READ

		delete f;
		updates.shrink(0);

		if(abort)
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr aborted - something went seriously wrong");
			localdisconnect();
			delete[] curmap;
			return;
		}

		game::transfer = true;
		game::openworld(curmap);
		delete[] curmap;

		//the game is compatible but is an older version
		//this is to update things to a newer version if such changes are required
		for(int v = hdr.gversion; v < game::gameversion; v++)
		{
			defformatstring(signal)("import %i", v);
			if(DEBUG_IO)
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr the game is outdated, currently version %i - sending \"%s\" to do any needed changes", v, signal);
			enumerate(*game::mapdata, mapinfo, map,
				map.getsignal(signal, true, NULL);
			)
		}
	}
	COMMAND(loadgame, "s");

	void savegame(const char *name)
	{
		if(!game::mapdata || !game::curmap)
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr No game in progress, can't save");
			return;
		}
		else if(!game::cansave())
		{
			conoutf("You may not save at this time");
			return;
		}

		defformatstring(file)("data/rpg/saves/%s.sgz.tmp", name);
		stream *f = opengzfile(path(file), "wb");

		if(!f)
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr failed to create savegame");
			return;
		}

		saveheader hdr;
		hdr.sversion = GAME_VERSION;
		hdr.gversion = game::gameversion;
		memcpy(hdr.magic, GAME_MAGIC, 4);

		lilswap(&hdr.sversion, 2);
		f->write(&hdr, sizeof(saveheader));

		writestring(f, game::data);
		writestring(f, game::curmap->name);
		writeent(f, game::player1);
		game::curmap->objs.removeobj(game::player1);

		#define WRITE(m, v, b) \
			f->putlil(v.length()); \
			loopv(v) \
			{ \
				if(DEBUG_IO) \
					conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr Writing " #m " %i of %i", i + 1, v.length()); \
				b; \
			}

		///TODO redo hotkeys
// 		WRITE(hotkey, game::hotkeys,
// 			f->putlil(game::hotkeys[i].base);
// 			f->putlil(game::hotkeys[i].use);
// 		)

		WRITE(faction, game::factions,
			writefaction(f, game::factions[i]);
		)
		WRITE(recipe, game::recipes,
			writerecipe(f, game::recipes[i]);
		)
		WRITE(merchant, game::merchants,
			writemerchant(f, game::merchants[i]);
		)
		WRITE(variable, game::variables,
			writestring(f, game::variables[i]);
		)

		vector<mapinfo *> maps;
		enumerate(*game::mapdata, mapinfo, map, maps.add(&map););
		WRITE(map, maps, writemap(f, maps[i]));

		WRITE(reference stack, rpgscript::stack,
			writereferences(f, maps, *rpgscript::stack[i]);
		)

		WRITE(delayscript stack, rpgscript::delaystack,
			writedelayscript(f, maps, rpgscript::delaystack[i]);
		)

		vector<journal *> journ;
		enumerate(game::journals, journal, bucket,
			journ.add(&bucket);
		)
		WRITE(journal bucket, journ,
			writejournal(f, journ[i]);
		)

		game::curmap->objs.add(game::player1);
		DELETEP(f);

		string actual, final;
		formatstring(actual)("%s", findfile(file, "wb"));
		copystring(final, actual, strlen(actual) - 3);
		rename(actual, final);

		conoutf("Game saved successfully to data/rpg/%s.sgz", name);

		copystring(file + strlen(file) - 8, ".png", 5);
		scaledscreenshot(file, 2, 256, 256);
		reloadtexture(file);
	}
	COMMAND(savegame, "s");
}