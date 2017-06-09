#include "rpggame.h"

extern bool reloadtexture(const char *name); //texture.cpp

namespace rpgio
{
	#define SAVE_VERSION 49
	#define COMPAT_VERSION 49
	#define SAVE_MAGIC "RPGS"

	/**
		SAVING STUFF
			STRINGS - use writestring(stream *, char *)
			HASHES  - see strings
			VECTORS - write an int corresponding to the number of elements, then write the elements
			POINTERS - convert to reference, if this is not possible don't save it
			VEC - use writevec macro, you need to write all 3 coordinates independantly

		LOADING STUFF
			STRINGS - use readstring(stream *)
			HASHES  - use READHASH macros and if needed, NOTNULL and VERIFYHASH macros
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
		DEBUGF("Read vec (%f, %f, %f) from file", v.x, v.y, v.z);

	#define writevec(v) \
	f->putlil(v.x); \
	f->putlil(v.y); \
	f->putlil(v.z); \
	if(DEBUG_IO) \
		DEBUGF("Wrote vec (%f, %f, %f) to file", v.x, v.y, v.z);

	#define CHECKEOF(f, val) \
	if((f).end()) \
	{ \
		abort = true; \
		ERRORF("Unexpected EoF at " __FILE__ ":%i; aborting - You should report this.", __LINE__); \
		return val; \
	}

	#define NOTNULL(f, val) \
	if(!f) \
	{ \
		abort = true; \
		ERRORF("Encountered unexpected NULL value for " #f " at " __FILE__ ":%i; aborting.", __LINE__); \
		return val; \
	}

	#define READHASH(val) \
	do { \
		const char *_str = readstring(f); \
		if(_str) val = game::queryhashpool(_str); \
		else val = NULL; \
		delete[] _str; \
	} while(0);

	#define READHASHEXTENDED(val, ht) \
	do { \
		const char *_str = readstring(f); \
		if(_str) val = ht.access(_str); \
		else val = NULL; \
		delete[] _str; \
	} while(0);

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
			WARNINGF("invalid entity num (%i), possible corruption", num);
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
			int fsize = f->size() - f->tell();
			if(len < 0 || len > fsize)
			{
				ERRORF("Cannot read %i characters from file, there are %i bytes remaining, aborting", len, fsize);
				abort = true;
				return NULL;
			}

			s = new char[len];
			f->read(s, len);
			if(DEBUG_IO)
				DEBUGF("Read \"%s\" from file (%i)", s, len);
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
			DEBUGF("Wrote \"%s\" to file (%i)", s, len);
	}

	void readfaction(stream *f, faction *fact)
	{
		int num = f->getlil<int>();
		if(DEBUG_IO)
			DEBUGF("reading %i relations", num);

		loopi(num)
		{
			CHECKEOF(*f, )
			const char *fac = readstring(f);
			int val = f->getlil<short>();

			if(fact && game::factions.access(fac))
				fact->setrelation(game::queryhashpool(fac), val);
			else
				WARNINGF("Faction %s does not exist, ignoring relation of %i", fac, val);

			delete[] fac;
		}
	}

	void writefaction(stream *f, faction *fact)
	{
		f->putlil(fact->relations.length());
		if(DEBUG_IO)
			DEBUGF("saving %i relations", fact->relations.length());

		enumeratekt(fact->relations, const char *, fac, short, relation,
			if(DEBUG_IO)
				DEBUGF("writing: factions[%s]->relations[%s] (%i)", fact->key, fac, relation);
			writestring(f, fac);
			f->putlil(relation);
		)
	}

	void readrecipe(stream *f, recipe *rec)
	{
		int flags = f->getlil<int>();
		if(rec) rec->flags |= flags;
	}

	void writerecipe(stream *f, recipe *rec)
	{
		f->putlil( rec->flags & recipe::SAVE);
	}

	void readmerchant(stream *f, merchant *mer)
	{
		int credit = f->getlil<int>();
		if(mer) mer->credit = credit;
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

		NOTNULL(it->mdl, it);
		preloadmodel(it->mdl);

		READHASHEXTENDED(it->script, game::scripts);
		READHASH(it->base);

		NOTNULL(it->script, it)
		NOTNULL(it->base, it);

		readvec(it->colour)
		it->quantity = f->getlil<int>();
		it->category = f->getlil<int>();
		it->flags = f->getlil<int>();
		it->value = f->getlil<int>();
		it->maxdurability = f->getlil<int>();
		it->charges = f->getlil<int>();
		it->scale = f->getlil<float>();
		it->weight = f->getlil<float>();
		it->durability = f->getlil<float>();
		it->recovery = f->getlil<float>();

		rpgscript::keeplocal((it->locals = f->getlil<int>()));

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
					if(!u) u = it->uses.add(new use_weapon(NULL));
					use_weapon *wp = (use_weapon *) u;

					READHASHEXTENDED(wp->projeffect, game::effects)
					READHASHEXTENDED(wp->traileffect, game::effects)
					READHASHEXTENDED(wp->deatheffect, game::effects)
					READHASHEXTENDED(wp->ammo, game::ammotypes)

					NOTNULL(wp->ammo, it);

					wp->range = f->getlil<int>();
					wp->angle = f->getlil<int>();
					wp->lifetime = f->getlil<int>();
					wp->gravity = f->getlil<int>();
					wp->cost = f->getlil<int>();
					wp->pflags = f->getlil<int>();
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
				[[fallthrough]];
				case USE_ARMOUR:
				{
					if(!u) u = it->uses.add(new use_armour(NULL));
					use_armour *ar = (use_armour *) u;

					delete[] ar->vwepmdl;
					delete[] ar->hudmdl;

					ar->vwepmdl = readstring(f);
					ar->hudmdl = readstring(f);

					if(ar->vwepmdl) preloadmodel(ar->vwepmdl);
					if(ar->hudmdl) preloadmodel(ar->hudmdl);
					READHASHEXTENDED(ar->idlefx, game::effects);

					ar->slots = f->getlil<int>();
					ar->skill = f->getlil<int>();

					loopj(STAT_MAX) ar->reqs.attrs[j] = f->getlil<short>();
					loopj(SKILL_MAX) ar->reqs.skills[j] = f->getlil<short>();
				}
				[[fallthrough]];
				case USE_CONSUME:
				{
					if(!u) u = it->uses.add(new use(NULL));

					delete[] u->name;
					delete[] u->description;
					delete[] u->icon;

					u->name = readstring(f);
					u->description = readstring(f);
					u->icon = readstring(f);

					READHASHEXTENDED(u->script, game::scripts);
					NOTNULL(u->script, it);

					u->cooldown = f->getlil<int>();
					u->chargeflags = f->getlil<int>();

					int efx = f->getlil<int>();
					loopj(efx)
					{
						CHECKEOF(*f, it)

						statusgroup *status = NULL;
						READHASHEXTENDED(status, game::statuses);
						NOTNULL(status, it);

						int e = f->getlil<int>();
						float m = f->getlil<float>();
						u->effects.add(new inflict(status, e, m));
					}
					break;
				}
			}
		}

		return it;
	}

	void writeitem(stream *f, item *it)
	{
		writestring(f, it->name);
		writestring(f, it->icon);
		writestring(f, it->description);
		writestring(f, it->mdl);

		writestring(f, it->script->key);
		writestring(f, it->base);

		writevec(it->colour)
		f->putlil(it->quantity);
		f->putlil(it->category);
		f->putlil(it->flags);
		f->putlil(it->value);
		f->putlil(it->maxdurability);
		f->putlil(it->charges);
		f->putlil(it->scale);
		f->putlil(it->weight);
		f->putlil(it->durability);
		f->putlil(it->recovery);

		f->putlil(it->locals);

		f->putlil(it->uses.length());
		loopv(it->uses)
		{
			f->putlil(it->uses[i]->type);
			switch(it->uses[i]->type)
			{
				case USE_WEAPON:
				{
					use_weapon *wp = (use_weapon *) it->uses[i];

					writestring(f, wp->projeffect ? wp->projeffect->key : NULL);
					writestring(f, wp->traileffect ? wp->traileffect->key : NULL);
					writestring(f, wp->deatheffect ? wp->deatheffect->key : NULL);
					writestring(f, wp->ammo->key);

					f->putlil(wp->range);
					f->putlil(wp->angle);
					f->putlil(wp->lifetime);
					f->putlil(wp->gravity);
					f->putlil(wp->cost);
					f->putlil(wp->pflags);
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
				}
				[[fallthrough]];
				case USE_ARMOUR:
				{
					use_armour *ar = (use_armour *) it->uses[i];

					writestring(f, ar->vwepmdl);
					writestring(f, ar->hudmdl);
					writestring(f, ar->idlefx ? ar->idlefx->key : NULL);

					f->putlil(ar->slots);
					f->putlil(ar->skill);

					loopj(STAT_MAX) f->putlil<short>(ar->reqs.attrs[j]);
					loopj(SKILL_MAX)  f->putlil<short>(ar->reqs.skills[j]);
				}
				[[fallthrough]];
				case USE_CONSUME:
				{
					use *u = it->uses[i];

					writestring(f, u->name);
					writestring(f, u->description);
					writestring(f, u->icon);
					writestring(f, u->script->key);

					f->putlil(u->cooldown);
					f->putlil(u->chargeflags);

					f->putlil(u->effects.length());
					loopvj(u->effects)
					{
						writestring(f, u->effects[j]->status->key);
						f->putlil(u->effects[j]->element);
						f->putlil(u->effects[j]->mul);
					}
					break;
				}
			}
		}
	}

	vector<rpgchar *> characters;
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
				characters.add(loading);

				delete[] loading->name;
				delete[] loading->mdl;
				delete[] loading->portrait;

				loading->name = readstring(f);
				loading->mdl = readstring(f);
				loading->portrait = readstring(f);
				readvec(loading->colour);

				NOTNULL(loading->mdl, ent);
				preloadmodel(loading->mdl);

				#define x(var, type) loading->base.var = f->getlil<type>();

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

				READHASHEXTENDED(loading->script, game::scripts)
				READHASHEXTENDED(loading->faction, game::factions)
				READHASHEXTENDED(loading->merchant, game::merchants)

				NOTNULL(loading->script, ent)
				NOTNULL(loading->faction, ent)
				if(loading->merchant) NOTNULL(loading->merchant, ent)

				loading->health = f->getlil<float>();
				loading->mana = f->getlil<float>();
				loading->scale = f->getlil<float>();
				loading->lastaction = f->getlil<int>() + lastmillis;

				vector<item *> items;
				int num = f->getlil<int>();
				loopi(num)
				{
					CHECKEOF(*f, ent)
					item *it = items.add(readitem(f));
					if(abort) {delete it; return ent;}
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
				if(abort) return ent;
				break;
			}
			case ENT_OBSTACLE:
			{
				if(!ent) ent = new rpgobstacle();
				rpgobstacle *loading = (rpgobstacle *) ent;

				delete loading->mdl;

				loading->mdl = readstring(f);
				NOTNULL(loading->mdl, ent);
				preloadmodel(loading->mdl);

				READHASHEXTENDED(loading->script, game::scripts);
				NOTNULL(loading->script, ent);

				readvec(loading->colour);
				loading->weight = f->getlil<int>();
				loading->flags = f->getlil<int>();
				loading->scale = f->getlil<float>();

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

				NOTNULL(loading->mdl, ent);
				preloadmodel(loading->mdl);

				READHASHEXTENDED(loading->faction, game::factions)
				READHASHEXTENDED(loading->merchant, game::merchants)
				READHASHEXTENDED(loading->script, game::scripts)
				if(loading->faction) NOTNULL(loading->faction, ent);
				if(loading->merchant) NOTNULL(loading->merchant, ent);
				NOTNULL(loading->script, ent)

				readvec(loading->colour);
				loading->capacity = f->getlil<int>();
				loading->lock = f->getlil<int>();
				loading->magelock = f->getlil<int>();
				loading->scale = f->getlil<float>();

				int items = f->getlil<int>();
				loopi(items)
				{
					CHECKEOF(*f, ent)
					item *it = readitem(f);
					if(abort) { delete it; return ent; }
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
				NOTNULL(loading->mdl, ent);
				preloadmodel(loading->mdl);

				READHASHEXTENDED(loading->script, game::scripts)
				NOTNULL(loading->script, ent)

				readvec(loading->colour);
				loading->speed = f->getlil<int>();
				loading->flags = f->getlil<int>();
				loading->scale = f->getlil<float>();

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
				NOTNULL(loading->mdl, ent);
				preloadmodel(loading->mdl);

				loading->name = readstring(f);
				READHASHEXTENDED(loading->script, game::scripts)
				NOTNULL(loading->script, ent)

				readvec(loading->colour);
				loading->flags = f->getlil<int>();
				loading->lasttrigger = f->getlil<int>() + lastmillis;
				loading->scale = f->getlil<float>();

				break;
			}
			default:
				ERRORF("unknown entity type %i", type);
				abort = true;
				return NULL;
		}

		rpgscript::keeplocal((ent->locals = f->getlil<int>()));

		int numeffs = f->getlil<int>();
		loopi(numeffs)
		{
			CHECKEOF(*f, ent)
			victimeffect *eff = new victimeffect();
			ent->seffects.add(eff);

			updates.add(reference(f->getlil<int>(), lastmap, eff->owner));

			READHASHEXTENDED(eff->group, game::statuses);
			NOTNULL(eff->group, ent);

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
						st = eff->effects.add(new status_polymorph());
						status_polymorph *poly = (status_polymorph *) st;

						poly->mdl = readstring(f);
						break;
					}
					case STATUS_LIGHT:
					{
						st = eff->effects.add(new status_light());
						status_light *light = (status_light *) st;

						readvec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						st = eff->effects.add(new status_script());
						status_script *scr = (status_script *) st;

						scr->script = readstring(f);
						break;
					}
					case STATUS_SIGNAL:
					{
						st = eff->effects.add(new status_signal());
						status_signal *sig = (status_signal *) st;

						sig->signal = readstring(f);
						break;
					}
					default:
						st = eff->effects.add(new status_generic());
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

		ent->maxheight = f->getlil<float>();
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
				writevec(saving->colour)

				#define x(var) f->putlil(saving->base.var);

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


				writestring(f, saving->script->key);
				writestring(f, saving->faction->key);
				writestring(f, saving->merchant ? saving->merchant->key : NULL);

				f->putlil(saving->health);
				f->putlil(saving->mana);
				f->putlil(saving->scale);
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
				writestring(f, saving->script->key);

				writevec(saving->colour)
				f->putlil(saving->weight);
				f->putlil(saving->flags);
				f->putlil(saving->scale);

				break;
			}
			case ENT_CONTAINER:
			{
				rpgcontainer *saving = (rpgcontainer *) d;

				writestring(f, saving->mdl);
				writestring(f, saving->name);
				writestring(f, saving->faction ? saving->faction->key : NULL);
				writestring(f, saving->merchant ? saving->merchant->key : NULL);
				writestring(f, saving->script->key );

				writevec(saving->colour)
				f->putlil(saving->capacity);
				f->putlil(saving->lock);
				f->putlil(saving->magelock);
				f->putlil(saving->scale);

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
				writestring(f, saving->script->key);

				writevec(saving->colour)
				f->putlil(saving->speed);
				f->putlil(saving->flags);
				f->putlil(saving->scale);

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
				writestring(f, saving->script->key);

				writevec(saving->colour)
				f->putlil(saving->flags);
				f->putlil(saving->lasttrigger - lastmillis);
				f->putlil(saving->scale);

				break;
			}
			default:
				ERRORF("unsupported ent type %i, aborting", d->type());
				return;
		}
		f->putlil(d->locals);

		f->putlil(d->seffects.length());
		loopv(d->seffects)
		{
			f->putlil(enttonum(d->seffects[i]->owner));
			writestring(f, d->seffects[i]->group->key);
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

		f->putlil(d->maxheight);
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
	}

	mapinfo *readmap(stream *f)
	{
		const char *name = readstring(f);
		mapinfo *loading = game::accessmap(name);
		lastmap = loading;

		loading->name = name;
		READHASHEXTENDED(loading->script, game::mapscripts);
		NOTNULL(loading->script, loading);

		loading->flags = f->getlil<int>();
		loading->loaded = f->getchar();

		int numobjs = f->getlil<int>(),
		numactions = f->getlil<int>(),
		numprojs = f->getlil<int>(),
		numaeffects = f->getlil<int>(),
		numblips = f->getlil<int>();
		rpgscript::keeplocal((loading->locals = f->getlil<int>()));

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
						WARNINGF("loaded teleport loadaction for invalid ent? ignoring");
						f->getlil<int>();
						f->getlil<int>();
						continue;
					}
					int d = f->getlil<int>();
					int t = f->getlil<int>();
					act = new action_teleport(entfromnum(ent), d, t);
					break;
				}
				case ACTION_SPAWN:
				{
					const char *id = NULL;
					READHASH(id)
					NOTNULL(id, loading)

					int tag = f->getlil<int>(),
						ent = f->getlil<int>(),
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
			projectile *p = loading->projs.add(new projectile());

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

			READHASHEXTENDED(p->projfx, game::effects)
			READHASHEXTENDED(p->trailfx, game::effects)
			READHASHEXTENDED(p->deathfx, game::effects)

			p->radius = f->getlil<int>();
			p->elasticity = f->getlil<float>();
			p->charge = f->getlil<float>();
			p->chargeflags = f->getlil<int>();
		}

		loopi(numaeffects)
		{
			CHECKEOF(*f, loading)

			areaeffect *aeff = loading->aeffects.add(new areaeffect());

			aeff->owner = entfromnum(f->getlil<int>());
			readvec(aeff->o);
			aeff->lastemit = 0; //should emit immediately
			READHASHEXTENDED(aeff->group, game::statuses)
			NOTNULL(aeff->group, loading);

			READHASHEXTENDED(aeff->fx, game::effects)
			if(aeff->fx) NOTNULL(aeff->fx, loading);

			aeff->elem = f->getlil<int>();
			aeff->radius = f->getlil<int>();

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
						st = aeff->effects.add(poly);

						poly->mdl = readstring(f);
						NOTNULL(poly->mdl, loading);
						preloadmodel(poly->mdl);
						break;
					}
					case STATUS_LIGHT:
					{
						status_light *light = new status_light();
						st = aeff->effects.add(light);

						readvec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						status_script *scr = new status_script();
						st = aeff->effects.add(scr);

						scr->script = readstring(f);
						break;
					}
					case STATUS_SIGNAL:
					{
						status_signal *sig = new status_signal();
						st = aeff->effects.add(sig);

						sig->signal = readstring(f);
						break;
					}
					default:
						st = aeff->effects.add(new status_generic());
						break;
				}
				st->type = type;
				st->duration = f->getlil<int>();
				st->remain = f->getlil<int>();
				st->strength = f->getlil<int>();
				st->variance = f->getlil<float>();
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
		writestring(f, saving->script->key);
		f->putlil(saving->flags);
		f->putchar(saving->loaded);

		f->putlil(saving->objs.length());
		f->putlil(saving->loadactions.length());
		f->putlil(saving->projs.length());
		f->putlil(saving->aeffects.length());
		f->putlil(saving->blips.length());
		f->putlil(saving->locals);

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
					f->putlil(act->etype);

					break;
				}
				case ACTION_SPAWN:
				{
					action_spawn *spw = (action_spawn *) saving->loadactions[i];
					writestring(f, spw->id);
					f->putlil(spw->tag);
					f->putlil(spw->ent);
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

			writestring(f, p->projfx ? p->projfx->key : NULL);
			writestring(f, p->trailfx ? p->trailfx->key : NULL);
			writestring(f, p->deathfx ? p->deathfx->key : NULL);

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
			writestring(f, aeff->group ? aeff->group->key : NULL);
			writestring(f, aeff->fx ? aeff->fx->key : NULL);
			f->putlil(aeff->elem);
			f->putlil(aeff->radius);

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
	void writereferences(stream *f, const vector<mapinfo *> &maps, hashnameset< ::reference> &stack)
	{
		f->putlil(stack.length());

		enumerate(stack, ::reference, saving,
			writestring(f, saving.name);
			f->putchar(saving.immutable);

			f->putlil(saving.list.length());
			loopvj(saving.list)
			{
				::reference::ref &sav = saving.list[j];
				char type = sav.type;
				if(sav.ptr == NULL)
				{
					if(DEBUG_IO)
						DEBUGF("reference %s:%i of type %i is null, saving as T_INVALID", saving.name, j, type);
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
							WARNINGF("char/item/object reference \"%s\" points to non-player entity that does not exist", saving.name);
							f->putchar(::reference::T_INVALID);
							continue;
						}

						if(DEBUG_IO)
							DEBUGF("writing reference %s as rpgent reference: %i %i", saving.name, map, ent);
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
						if(DEBUG_IO) DEBUGF("writing reference %s:%i as map reference: %i", saving.name, j, map);
						break;
					}
					case ::reference::T_INV:
					{
						int map = -1;
						int ent = -1;
						const char *base = saving.getinv(j)->base;
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
							WARNINGF("inv reference \"%s:%i\" points to an item that does not exist", saving.name, j);
							f->putchar(::reference::T_INVALID);
							continue;
						}

						if(DEBUG_IO) DEBUGF("writing reference \"%s:%i\" as type T_INV with indices: %i %i %s %i", saving.name, j, map, ent, base, offset);

						f->putchar(type);
						f->putlil(map);
						f->putlil(ent);
						writestring(f, base);
						f->putlil(offset);

						break;
					}

					//
					default:
						ERRORF("unsupported reference type %i for reference %s:%i, saving as T_INVALID", sav.type, saving.name, j);
					// Temporary reference types below this line...
					[[fallthrough]];
					case ::reference::T_EQUIP:
					case ::reference::T_VEFFECT:
					case ::reference::T_AEFFECT:
						type = ::reference::T_INVALID;
						[[fallthrough]];
					case ::reference::T_INVALID:
						if(DEBUG_IO) DEBUGF("writing null reference %s:%i", saving.name, j);
						f->putchar(type);
						break;
				}
			}
		)
	}

	void readreferences(stream *f, const vector<mapinfo *> &maps, hashnameset< ::reference> &stack)
	{
		int num = f->getlil<int>();
		loopi(num)
		{
			CHECKEOF(*f, )
			const char *name = NULL;
			READHASH(name);
			NOTNULL(name, )

			::reference *loading = stack.access(name);
			if(loading)
			{
				WARNINGF("reference \"%s\" appears to have already been loaded", name);
				loading->dump();
			}
			else
			{
				if(DEBUG_IO) DEBUGF("Creating reference \"%s\"", name);
				loading = &stack[name];
				loading->name = name;
			}
			loading->immutable = f->getchar();

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
							if(DEBUG_IO) DEBUGF("reading player char reference %s:%i", loading->name, j);
							loading->pushref(game::player1, true);
						}
						else if(maps.inrange(map) && maps[map]->objs.inrange(ent))
						{
							if(DEBUG_IO) DEBUGF("reading valid rpgent reference %s:%i -> %i %i", loading->name, j, map, ent);
							loading->pushref(maps[map]->objs[ent], true);
						}
						else WARNINGF("rpgent reference %s:%i -> %i %i - indices out of range", loading->name, j, map, ent);

						break;
					}
					case ::reference::T_MAP:
					{
						int map = f->getlil<int>();
						if(DEBUG_IO) DEBUGF("reading map reference %s:%i -> %i", loading->name, j, map);
						if(maps.inrange(map)) loading->pushref(maps[map], true);
						break;
					}
					case ::reference::T_INV:
					{
						int map = f->getlil<int>();
						int ent = f->getlil<int>();
						const char *base = NULL;
						READHASH(base);
						NOTNULL(base, )
						int offset = f->getlil<int>();

						if(DEBUG_IO) DEBUGF("reading T_INV reference %s:%i with values %i %i %s %i...", loading->name, j, map, ent, base, offset);
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
							if(DEBUG_IO) DEBUGF("Loading T_INV reference to %p successfully", (*stack)[offset]);
							loading->pushref((*stack)[offset], true);
						}
						else
							WARNINGF("T_INV reference %s:%i has out of range values: %i %i %s %i, loading failed", loading->name, j, map, ent, base, offset);

						break;
					}

					//Temporary types below this line
					case ::reference::T_EQUIP:
					case ::reference::T_VEFFECT:
					case ::reference::T_AEFFECT:
						WARNINGF("volatile reference type found for reference %s:%i, assuming invalid", loading->name, j);
						[[fallthrough]];
					case ::reference::T_INVALID:
						if(DEBUG_IO) DEBUGF("reading now null reference %s:%i", loading->name, j);
						loading->pushref(NULL, true);
						break;
					default:
						ERRORF("unsupported reference type %i for reference %s:%i, this will cause issues; aborting", type, loading->name, j);
						abort = true;
						return;
				}
			}
		}
	}

	void writedelayscript(stream *f, const vector<mapinfo *> &maps, delayscript *saving)
	{
		writereferences(f, maps, saving->refs);
		writestring(f, saving->script);
		f->putlil(saving->remaining);
	}

	void readdelayscript(stream *f, const vector<mapinfo *> &maps, delayscript *loading)
	{
		readreferences(f, maps, loading->refs);
		loading->script = readstring(f);
		loading->remaining = f->getlil<int>();
	}

	void writejournal(stream *f, journal *saving)
	{
		writestring(f, saving->name);
		f->putlil(saving->status);
		f->putlil(saving->entries.length());
		loopv(saving->entries)
			writestring(f, saving->entries[i]);
	}

	void readjournal(stream *f)
	{
		const char *name = NULL;
		READHASH(name);
		NOTNULL(name, )

		journal *journ = &game::journals[name];
		if(journ->name)
			WARNINGF("additional instance of journal %s exists, merging", name);

		journ->name = name;
		journ->status = f->getlil<int>();

		int entries = f->getlil<int>();
		loopi(entries)
			journ->entries.add(readstring(f));
	}

	void writelocal(stream *f, localinst *saving)
	{
		f->putchar(saving ? saving->shared : 0);
		int n = 0;
		if(saving) n = saving->variables.length();
		f->putlil(n);

		if(saving) enumerate(saving->variables, rpgvar, var,
			writestring(f, var.name);
			writestring(f, var.value);
		)
	}

	void readlocal(stream *f, int i)
	{
		if(!rpgscript::locals.inrange(i))
			rpgscript::alloclocal(false);

		localinst *loading = rpgscript::locals[i];
		loading->refs = 0;
		loading->shared = f->getchar();

		int n = f->getlil<int>();
		loopj(n)
		{
			CHECKEOF(*f, )
			const char *name = NULL;
			READHASH(name);
			NOTNULL(name, );

			const char *value = readstring(f);

			rpgvar &var = loading->variables[name];
			if(var.value) delete[] var.value;

			var.name = name;
			var.value = value;
		}
	}

	void writetimer(stream *f, vector<mapinfo *> &maps, timer *saving)
	{
		writestring(f, saving->name);
		writestring(f, saving->cond);
		writestring(f, saving->script);

		f->putlil(saving->delay);
		f->putlil(saving->remaining);
	}

	void readtimer(stream *f, vector<mapinfo *> &maps)
	{
		const char *name;
		READHASH(name);
		NOTNULL(name, );

		bool del = false;
		timer *loading;

		if(rpgscript::timers.access(name))
		{
			if(DEBUG_IO) DEBUGF("Timer %s already exists, restoring countdown only", name);
			del = true;
			loading = new timer();
		}
		else loading = &rpgscript::timers[name];

		loading->name = name;

		loading->cond = readstring(f);
		loading->script = readstring(f);

		loading->delay = f->getlil<int>();
		loading->remaining = f->getlil<int>();

		if(del)
		{
			rpgscript::timers[name].remaining = loading->remaining;
			delete loading;
		}
	}

	void loadgame(const char *name)
	{
		defformatstring(file, "%s/%s.sgz", game::datapath("saves"), name);
		stream *f = opengzfile(file, "rb");

		if(!f)
		{
			ERRORF("unable to read file: %s", file);
			return;
		}

		saveheader hdr;
		f->read(&hdr, sizeof(saveheader));
		lilswap(&hdr.sversion, 2);

		if(hdr.sversion < COMPAT_VERSION || hdr.sversion > SAVE_VERSION || strncmp(hdr.magic, SAVE_MAGIC, 4))
		{
			ERRORF("Unsupported version or corrupt save: %i (%i) - %4.4s (%s)", hdr.sversion, SAVE_VERSION, hdr.magic, SAVE_MAGIC);
			delete f;
			return;
		}
		if(DEBUG_IO)
			DEBUGF("supported save: %i  %4.4s", hdr.sversion, hdr.magic);

		vector<mapinfo *> maps;
		const char *data = readstring(f);
		const char *curmap = readstring(f);
		const char *cpmap = readstring(f);
		int cpnum = f->getlil<int>();
		abort = !game::newgame(data, true);
		if(cpmap) game::setcheckpoint(cpmap, cpnum);

		delete[] data;
		delete[] cpmap;

		if(game::compatversion > hdr.gversion)
		{
			ERRORF("saved game is of game version %i, last compatible version is %i; aborting", hdr.gversion, game::compatversion);
			abort = true;
			goto cleanup;
		}
		if(!curmap || abort)
		{
			ERRORF("No map in progress?");
			abort = true; goto cleanup;
		}

		lastmap = game::accessmap(curmap);

		int num;
		#define READ(m, b) \
			num = f->getlil<int>(); \
			loopi(num) \
			{ \
				if(abort) goto cleanup; \
				if(f->end()) \
				{ \
					ERRORF("unexpected EoF, aborting"); \
					abort = true; goto cleanup; \
				} \
				if(DEBUG_IO) \
					DEBUGF("reading " #m " %i of %i", i + 1, num); \
				b; \
			}

		///TODO redo hotkeys
// 		READ(hotkey,
// 			int b = f->getlil<int>();
// 			int u = f->getlil<int>();
// 			game::hotkeys.add(equipment(b, u));
// 		)

		READ(faction,
			const char *key = NULL;
			READHASH(key);
			faction *fac = game::factions.access(key);
			if(!fac) WARNINGF("reading faction %s as a dummy", key);
			readfaction(f, fac);
		);
		READ(recipe,
			const char *key = NULL;
			READHASH(key);
			recipe *r = game::recipes.access(key);
			if(!r) WARNINGF("reading recipe %s as a dummy", key);
			readrecipe(f, r);
		);
		READ(merchant,
			const char *key = NULL;
			READHASH(key);
			merchant *m = game::merchants.access(key);
			if(!m) WARNINGF("reading merchant %s as a dummy", key);
			readmerchant(f, m);
		);
		READ(variable,
			 const char *name = NULL;
			 READHASH(name);
			 const char *v = readstring(f);
			 if(!rpgscript::setglobal(name, v, false))
				 WARNINGF("reloading the game added variable \"%s\"", name);
		)
		READ(locals stack,
			 readlocal(f, i);
		)
		readent(f, game::player1);

		READ(mapinfo, maps.add(readmap(f)));

		READ(reference stack,
			if(!rpgscript::stack.inrange(i)) rpgscript::pushstack();
			readreferences(f, maps, *rpgscript::stack[i]);
		)

		READ(delayscript stack,
			rpgscript::delaystack.add(new delayscript());
			readdelayscript(f, maps, rpgscript::delaystack[i]);
		)

		READ(global timers, readtimer(f, maps); )

		READ(journal bucket,
			readjournal(f);
		)

		#undef READ

		if(!abort) loopv(characters)
			characters[i]->compactinventory(NULL);

	cleanup:
		delete f;
		characters.shrink(0);
		updates.shrink(0);
		rpgscript::cleanlocals();

		if(abort)
		{
			ERRORF("aborted - something went seriously wrong");
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
			defformatstring(signal, "import %i", v);
			if(DEBUG_IO)
				DEBUGF("the game is outdated, currently version %i - sending \"%s\" to do any needed changes", v, signal);
			enumerate(game::mapdata, mapinfo, map,
				map.getsignal(signal, true, NULL);
			)
		}
	}
	COMMAND(loadgame, "s");

	void savegame(const char *name)
	{
		if(!game::connected || !game::curmap)
		{
			ERRORF("No game in progress, can't save");
			return;
		}
		else if(!game::cansave())
		{
			conoutf("You may not save at this time");
			return;
		}

		defformatstring(file, "%s/%s.sgz.tmp", game::datapath("saves"), name);
		stream *f = opengzfile(path(file), "wb");

		if(!f)
		{
			ERRORF("failed to create savegame");
			return;
		}

		saveheader hdr;
		hdr.sversion = SAVE_VERSION;
		hdr.gversion = game::gameversion;
		memcpy(hdr.magic, SAVE_MAGIC, 4);

		lilswap(&hdr.sversion, 2);
		f->write(&hdr, sizeof(saveheader));

		writestring(f, game::data);
		writestring(f, game::curmap->name);
		writestring(f, game::cpmap);
		f->putlil(game::cpnum);

		#define WRITE(m, v, b) \
			f->putlil(v.length()); \
			loopv(v) \
			{ \
				if(DEBUG_IO) \
					DEBUGF("Writing " #m " %i of %i", i + 1, v.length()); \
				b; \
			}

		#define WRITEHT(m, ht, t, b) \
			if(DEBUG_IO) DEBUGF("Writing %i " #m "(s)", (ht).length()); \
			f->putlil((ht).length()); \
			enumerate(ht, t, entry, \
				writestring(f, entry.key); \
				if(DEBUG_IO) \
					DEBUGF("Writing " #m " %s to file...", entry.key); \
				b; \
			)


		///TODO redo hotkeys
// 		WRITE(hotkey, game::hotkeys,
// 			f->putlil(game::hotkeys[i].base);
// 			f->putlil(game::hotkeys[i].use);
// 		)

		WRITEHT(faction, game::factions, faction,
			writefaction(f, &entry);
		)
		WRITEHT(recipe, game::recipes, recipe,
			writerecipe(f, &entry);
		)
		WRITEHT(merchant, game::merchants, merchant,
			writemerchant(f, &entry);
		)

		vector<rpgvar *> vars;
		enumerate(game::variables, rpgvar, var, vars.add(&var));

		WRITE(variable, vars,
			writestring(f, vars[i]->name);
			writestring(f, vars[i]->value);
		)
		WRITE(locals stack, rpgscript::locals,
			  writelocal(f, rpgscript::locals[i]);
		)
		writeent(f, game::player1);
		game::curmap->objs.removeobj(game::player1);

		vector<mapinfo *> maps;
		enumerate(game::mapdata, mapinfo, map, maps.add(&map););
		WRITE(map, maps, writemap(f, maps[i]));

		WRITE(reference stack, rpgscript::stack,
			writereferences(f, maps, *rpgscript::stack[i]);
		)

		WRITE(delayscript stack, rpgscript::delaystack,
			writedelayscript(f, maps, rpgscript::delaystack[i]);
		)

		vector<timer *> timers;
		enumerate(rpgscript::timers, timer, t, timers.add(&t));

		WRITE(global timer, timers,
			writetimer(f, maps, timers[i]);
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

		string final;
		copystring(final, file, strlen(file) - 3);
		backup(file, final);

		conoutf("Game saved successfully to %s", final);

		copystring(file + strlen(file) - 8, ".png", 5);
		scaledscreenshot(file, 2, 256, 256);
		reloadtexture(file);
	}
	COMMAND(savegame, "s");
}
