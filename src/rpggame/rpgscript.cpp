#include "rpggame.h"

using namespace game;

/*
	Reference is a purely script class, so we define it here so we can have some debug stuff
*/

#define temporary(x, i) ((x).getequip(i) || (x).getveffect(i) || (x).getaeffect(i))

rpgent *reference::getent(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type >= T_CHAR && list[i].type <= T_TRIGGER) return (rpgent *) list[i].ptr;
	return NULL;
}
rpgchar *reference::getchar(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_CHAR) return (rpgchar *) list[i].ptr;
	return NULL;
}
rpgitem *reference::getitem(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_ITEM) return (rpgitem *) list[i].ptr;
	return NULL;
}
rpgobstacle *reference::getobstacle(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_OBSTACLE) return (rpgobstacle *) list[i].ptr;
	return NULL;
}
rpgcontainer *reference::getcontainer(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_CONTAINER) return (rpgcontainer *) list[i].ptr;
	return NULL;
}
rpgplatform *reference::getplatform(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_PLATFORM) return (rpgplatform *) list[i].ptr;
	return NULL;
}
rpgtrigger *reference::gettrigger(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_TRIGGER) return (rpgtrigger *) list[i].ptr;
	return NULL;
}
item *reference::getinv(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_INV) return (item *) list[i].ptr;
	if(list[i].type == T_ITEM) return (rpgitem *) list[i].ptr;
	if(list[i].type == T_EQUIP) return ((equipment *) list[i].ptr)->it;

	return NULL;
}
equipment *reference::getequip(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_EQUIP) return (equipment *) list[i].ptr;
	return NULL;
}
mapinfo *reference::getmap(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_MAP) return (mapinfo *) list[i].ptr;
	return NULL;
}
victimeffect *reference::getveffect(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_VEFFECT) return (victimeffect *) list[i].ptr;
	return NULL;
}
areaeffect *reference::getaeffect(int i) const
{
	if(!list.inrange(i)) return NULL;
	if(list[i].type == T_AEFFECT) return (areaeffect *) list[i].ptr;
	return NULL;
}

void reference::pushref(rpgchar *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_CHAR));
	if(DEBUG_VSCRIPT) DEBUGF("pushed rpgchar type %p onto reference %s", d, name);
}
void reference::pushref(rpgitem *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_ITEM));
	if(DEBUG_VSCRIPT) DEBUGF("pushed rpgitem type %p onto reference %s", d, name);
}
void reference::pushref(rpgobstacle *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_OBSTACLE));
	if(DEBUG_VSCRIPT) DEBUGF("pushed obstacle type %p onto reference %s", d, name);
}
void reference::pushref(rpgcontainer *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_CONTAINER));
	if(DEBUG_VSCRIPT) DEBUGF("pushed container type %p onto reference %s", d, name);
}
void reference::pushref(rpgplatform *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_PLATFORM));
	if(DEBUG_VSCRIPT) DEBUGF("pushed platform type %p onto reference %s", d, name);
}
void reference::pushref(rpgtrigger *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_TRIGGER));
	if(DEBUG_VSCRIPT) DEBUGF("pushed trigger type %p onto reference %s", d, name);
}
void reference::pushref(rpgent *d, bool force)
{
	if(!d || !canset(force)) return;

	if(d) switch(d->type())
	{
		case ENT_CHAR: pushref((rpgchar *) d, force); return;
		case ENT_ITEM: pushref((rpgitem *) d, force); return;
		case ENT_OBSTACLE: pushref((rpgobstacle *) d, force); return;
		case ENT_CONTAINER: pushref((rpgcontainer *) d, force); return;
		case ENT_PLATFORM: pushref((rpgplatform *) d, force); return;
		case ENT_TRIGGER: pushref((rpgtrigger *) d, force); return;
	}
	ERRORF("reference::pushref(rpgent *d), if you see this, an unknown entity was encountered (%i), report this as a bug!", d->type());
}
void reference::pushref(item *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_INV));
	if(DEBUG_VSCRIPT) DEBUGF("pushed inventory item type %p onto reference %s", d, name);
}
void reference::pushref(vector<item *> *d, bool force)
{
	if(!canset(force)) return;

	loopv(*d)
		pushref((*d)[i], force);
}
void reference::pushref(equipment *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_EQUIP));
	if(DEBUG_VSCRIPT) DEBUGF("pushed equip type %p onto reference %s", d, name);
}
void reference::pushref(mapinfo *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_MAP));
	if(DEBUG_VSCRIPT) DEBUGF("pushed mapinfo type %p onto reference %s", d, name);
}
void reference::pushref(victimeffect *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_VEFFECT));
	if(DEBUG_VSCRIPT) DEBUGF("pushed victim effect type %p onto reference %s", d, name);
}
void reference::pushref(areaeffect *d, bool force)
{
	if(!canset(force)) return;

	list.add(ref(d, T_AEFFECT));
	if(DEBUG_VSCRIPT) DEBUGF("pushed area effect type %p onto reference %s", d, name);
}
void reference::pushref(reference *d, bool force)
{
	if(!canset(force)) return;

	loopv(d->list)
		list.add(d->list[i]);
}
void reference::setnull(bool force) { if(!canset(force)) return; list.setsize(0); if(DEBUG_VSCRIPT) DEBUGF("reference %s set to null", name);}

reference::~reference()
{
	if(name && DEBUG_VSCRIPT) DEBUGF("freeing reference %s", name);
}

/*
	Some signal stuff
*/

void item::getsignal(const char *sig, bool prop, rpgent *sender, int use)
{
	if(DEBUG_VSCRIPT)
		DEBUGF("item %p received signal %s with sender %p and use %i; propagating: %s", this, sig, sender, use, prop ? "yes" : "no");

	signal *listen = getscript(use)->listeners.access(sig);
	if(!listen) listen = getscript()->listeners.access(sig);

	if(listen) loopv(listen->code)
		rpgscript::doitemscript(this, sender, listen->code[i]);
}

void rpgent::getsignal(const char *sig, bool prop, rpgent *sender)
{
	if(DEBUG_VSCRIPT)
		DEBUGF("entity %p received signal %s with sender %p; propagating: %s", this, sig, sender, prop ? "yes" : "no");

	signal *listen = getscript()->listeners.access(sig);
	if(listen) loopv(listen->code)
		rpgscript::doentscript(this, sender, listen->code[i]);

	if(prop && HASINVENTORY(type()))
	{
		hashtable<const char *, vector<item *> > *inventory = NULL;
		if(type() == ENT_CHAR) inventory = &((rpgchar *) this)->inventory;
		else inventory = &((rpgcontainer *) this)->inventory;

		enumerate(*inventory, vector<item *>, inv,
			loopvj(inv)
			{
				if(inv[j]->quantity > 0)
					inv[j]->getsignal(sig, prop, this, -1);
				if(type() == ENT_CHAR)
				{
					vector<equipment*> &equips = ((rpgchar *) this)->equipped;
					loopvk(equips) if(inv[j] == equips[k]->it)
					{
						inv[j]->getsignal(sig, prop, this, equips[k]->use);
					}
				}
			}
		)
	}
}

void mapinfo::getsignal(const char *sig, bool prop, rpgent *sender)
{
	if(DEBUG_VSCRIPT)
		DEBUGF("map %p received signal %s with sender %p; propagating: %s", this, sig, sender, prop ? "yes" : "no");

	signal *listen = script->listeners.access(sig);
	if(listen) loopv(listen->code)
		rpgscript::domapscript(this, sender, listen->code[i]);

	if(prop) loopv(objs)
	{
		objs[i]->getsignal(sig, prop, sender);
	}
}

bool delayscript::update()
{
	if((remaining -= curtime) > 0 || camera::cutscene)
		return true;

	rpgscript::stack.add(&refs);
	rpgexecute(script);
	rpgscript::stack.pop();

	return false;
}

/*
	start the actual script stuff!
*/

namespace rpgscript
{
	/**
		HOW IT WORKS

		The system is stack based, whenever a script command is executed, the stack is pushed, the script executed then the stack popped.
		The system defines several references, player, hover and curmap; these are special references that should only be modified by the code here.
		This system must be 100% type safe and relatively ignorant of them.
	*/

	vector<rpgent *> obits; //r_destroy
	vector<hashnameset<reference> *> stack;
	vector<localinst *> locals;
	vector<delayscript *> delaystack;
	reference *player = NULL;
	reference *hover = NULL;
	reference *map = NULL;
	reference *talker = NULL;
	reference *looter = NULL;
	reference *trader = NULL;
	reference *config = NULL; //dummy for creation and initialisation.

	void pushstack()
	{
		if(DEBUG_VSCRIPT) DEBUGF("pushing new stack...");
		int size = stack.length() ? 64 : 1024;
		stack.add(new hashnameset<reference>(size));
		if(DEBUG_VSCRIPT) DEBUGF("new depth is %i", stack.length());
	}

	void popstack()
	{
		if(DEBUG_VSCRIPT) DEBUGF("popping stack...");
		if(stack.length() > 1)
		{
			delete stack.pop();
			if(DEBUG_VSCRIPT) DEBUGF("new depth is %i", stack.length());
		}
		else
			ERRORF("Lamiae just tried to pop the reference stack, but it is too shallow (stack.length <= 1)");
	}

	ICOMMAND(r_stack, "e", (uint *body),
		if(!stack.length()) return; //assume no game in progress
		pushstack();
		rpgexecute(body);
		popstack();
	)

	reference *searchstack(const char *name, bool create)
	{
		if(!stack.length())
		{
			ERRORF("no stack");
			return NULL;
		}

		const char *sep = strchr(name, ':');
		static string lookup;
		copystring(lookup, name);
		if(sep) lookup[sep - name] = '\0';

		loopvrev(stack)
		{
			reference *ref = stack[i]->access(lookup);
			if(ref)
			{
				if(DEBUG_VSCRIPT)
					DEBUGF("reference \"%s\" found, returning %p", name, ref);
				return ref;
			}
		}
		if(create)
		{
			if(DEBUG_VSCRIPT) DEBUGF("reference \"%s\" not found, creating", lookup);
			const char *refname = queryhashpool(lookup);
			reference *ref = &(*stack[0])[refname];
			ref->name = refname;
			return ref;

		}
		if(DEBUG_VSCRIPT) DEBUGF("reference \"%s\" not found", lookup);
		return NULL;
	}

	template<typename T>
	reference *registerref(const char *name, T ref)
	{
		if(DEBUG_VSCRIPT) DEBUGF("registering reference %p to name %s", ref, name);
		reference *r = searchstack(name, true);
		if(r) r->setref(ref);
		return r;
	}
	template<>
	inline reference *registerref(const char *name, int)
	{
		return registerref(name, (void *) 0);
	}

	template<typename T>
	reference *registertemp(const char *name, T ref)
	{
		if(DEBUG_VSCRIPT) DEBUGF("registering end-of-stack reference %p to name %s", ref, name);
		reference *r = NULL;
		if(stack.length())
		{
			r = &(*stack.last())[name];
			if(!r->name) r->name = queryhashpool(name);

			r->setref(ref);
		}
		return r;
	}
	template<>
	inline reference *registertemp(const char *name, int)
	{
		return registertemp(name, (void *) 0);
	}

	void copystack(hashnameset<reference> &dst)
	{
		loopvj(stack)
		{
			enumerate(*stack[j], reference, ref,
				reference *newref = &dst[ref.name];
				*newref = ref;
				loopvk(newref->list)
				{
					if(temporary(*newref, k)) newref->list.remove(k--);
				}
			)
		}
	}

	ICOMMAND(r_sleep, "is", (int *rem, const char *scr),
		delayscript *ds = delaystack.add(new delayscript());
		ds->remaining = *rem;
		ds->script = newstring(scr);
		copystack(ds->refs);
	)

	void clean()
	{
		stack.deletecontents();

		while(locals.length() && !locals.last()) locals.pop();
		if(locals.length())
		{
			dumplocals();
			ERRORF("The locals stack wasn't fully freed! REPORT A BUG!");
			locals.deletecontents();
		}

		obits.setsize(0);
		player = map = talker = looter = trader = hover = NULL;
	}

	void init()
	{
		pushstack();
		player = registerref("player", player1);
		hover = searchstack("hover", true);
		looter = searchstack("looter", true);
		trader = searchstack("trader", true);
		talker = searchstack("talker", true);
		map = registerref("curmap", curmap);
		config = registerref("config", NULL);

		player->immutable = hover->immutable = looter->immutable = trader->immutable = talker-> immutable = map->immutable = config->immutable = true;
	}

	void replacerefs(void *orig, void *next)
	{
		loopvj(stack)
		{
			enumerate(*stack[j], reference, ref,
				loopvk(ref.list) if(ref.list[k].ptr == orig)
				{
					if(next) ref.list[k].ptr = next;
					else ref.list.remove(k--);
				}
			)
		}
		loopvj(delaystack)
		{
			enumerate(delaystack[i]->refs, reference, ref,
				loopvk(ref.list) if(ref.list[k].ptr == orig)
				{
					if(next) ref.list[k].ptr = next;
					else ref.list.remove(k--);
				}
			)
		}
	}

	void removereferences(rpgent *ptr, bool references)
	{
		obits.removeobj(ptr);
		enumerate(mapdata, mapinfo, map,
			map.objs.removeobj(ptr);
			loopvj(map.loadactions)
			{
				if(map.loadactions[j]->type() != ACTION_TELEPORT || ((action_teleport *) map.loadactions[j])->ent != ptr)
					continue;
				delete map.loadactions.remove(j--);
			}
			loopvj(map.projs)
			{
				//projectiles depend on character instance of items
				if(map.projs[j]->owner == ptr)
					delete map.projs.remove(j--);
			}
		);
		if(references)
		{
			if(HASINVENTORY(ptr->type()))
			{
				//NOTE: T_EQUIP references are cleared on the temporaries clearing pass
				hashtable<const char *, vector<item *> > *inventory = NULL;
				if(ptr->type() == ENT_CHAR) inventory = &((rpgchar *) ptr)->inventory;
				else inventory = &((rpgcontainer *) ptr)->inventory;

				enumerate(*inventory, vector<item *>, stack,
					loopvk(stack) removeminorrefs(stack[k]);
				)
			}
			removeminorrefs(ptr);
		}
	}

	void changemap()
	{
		map->setref(curmap, true);
		player->setref(player1, true);
		talker->setnull(true);
		looter->setnull(true);
		trader->setnull(true);
		hover->setnull(true);
	}

	void update()
	{
		obits.removeobj(game::player1);

		while(obits.length())
		{
			rpgent *ent = obits.pop();
			removereferences(ent);
			delete ent;
		}
		//clear volatile references
		loopvj(stack)
		{
			enumerate(*stack[j], reference, ref,
				loopvk(ref.list) if(temporary(ref, k))
					ref.list.remove(k--);
			);
		}

		loopv(delaystack)
		{
			if(!delaystack[i]->update())
				delete delaystack.remove(i--);
		}

		vec dir = vec(worldpos).sub(player1->o);
		float maxdist = max(player1->eyeheight, player1->radius) * 1.1;
		if(dir.magnitude() > maxdist)
		{
			dir.normalize().mul(maxdist);
		}
		dir.add(player1->o);

		float dist;
		rpgent *h = intersectclosest(player1->o, dir, player1, dist, 1);

		hover->setref(h, true);
	}

	void doitemscript(item *invokee, rpgent *invoker, uint *code)
	{
		if(!invokee || !code) return;
		if(DEBUG_VSCRIPT)
			DEBUGF("conditions met, executing code with invokee %p and invoker %p", invokee, invoker);

		pushstack();

		registertemp("self", invokee)->immutable = true;
		registertemp("actor", invoker)->immutable = true;

		rpgexecute(code);
		popstack();
	}

	void doentscript(rpgent *invokee, rpgent *invoker, uint *code)
	{
		if(!invokee || !code) return;
		if(DEBUG_VSCRIPT)
			DEBUGF("conditions met, executing code with invokee %p and invoker %p", invokee, invoker);

		pushstack();

		registertemp("self", invokee)->immutable = true;
		registertemp("actor", invoker)->immutable = true;

		rpgexecute(code);
		popstack();
	}

	void domapscript(mapinfo *invokee, rpgent *invoker, uint *code)
	{
		if(!invokee || !code) return;
		if(DEBUG_VSCRIPT)
			DEBUGF("conditions met, executing code with invokee %p and invoker %p", invokee, invoker);

		pushstack();

		registertemp("self", invokee)->immutable = true;
		registertemp("actor", invoker)->immutable = true;

		rpgexecute(code);
		popstack();
	}

	void parseref(const char *str, int &idx)
	{
		idx = 0;
		const char *sep = strchr(str, ':');
		if(sep)
			idx = parseint(++sep);
	}

	#define getreference(var, name, cond, fail, fun) \
		if(!*var) \
		{ \
			ERRORF(#fun "; requires a reference to be specified"); \
			fail; return; \
		} \
		int name ## idx; \
		parseref(var, name ## idx); \
		if(DEBUG_VSCRIPT) \
		{ \
			const char *sep = strchr(var, ':'); \
			DEBUGF("searching for reference " #name ", via reference %s%s", var, sep ? "" : ":0"); \
		} \
		reference *name = searchstack(var); \
		if(!name || !(cond)) \
		{ \
			ERRORF(#fun "; invalid reference \"%s\" or of incompatible type", var); \
			fail; return; \
		}


	// Utility Functions

	ICOMMAND(worlduse, "", (),
		if(hover && hover->getent(0) && hover->getent(0) != player1)
		{
			if(DEBUG_SCRIPT)
				DEBUGF("Player interacted with %p", hover);

			hover->getent(0)->getsignal("interact", false, player1);
		}
	)

	ICOMMAND(r_cansee, "ss", (const char *looker, const char *lookee),
		getreference(looker, ent, ent->getchar(entidx), intret(0), r_cansee)
		getreference(lookee, obj, obj->getent(objidx), intret(0), r_cansee)

		intret(ent->getchar(entidx)->cansee(obj->getent(objidx)));
	)

	ICOMMAND(r_preparemap, "ssi", (const char *m, const char *scr, int *f),
		if(*m)
		{
			mapinfo *tmp = accessmap(m);
			if(!tmp) return;

			mapscript *s = mapscripts.access(scr);
			if(!s)
			{
				if(DEBUG_SCRIPT)
					DEBUGF("Prepared mapinfo for %s with script %s and flags %i", m, scr, *f);

				tmp->script = s;
			}
			else
				ERRORF("Cannot assign script %s to map %s; flags %i still applied", scr, m, *f);
			tmp->flags = *f;
		}
	)

	ICOMMAND(r_reftype, "s", (const char *ref),
		int idx;
		parseref(ref, idx);
		reference *lookup = searchstack(ref);

		if(lookup && lookup->list.inrange(idx)) intret(lookup->list[idx].type);
		else intret(reference::T_INVALID);
	)

	ICOMMAND(r_clearref, "V", (tagval *v, int n),
		loopi(n)
		{
			const char *ref = v[i].getstr();
			loopvrev(stack)
			{
				reference *r = searchstack(ref, false);
				if(r->canset()) loopvrev(stack)
				{
					if(stack[i]->remove(r->name)) return;
				}
			}
		}
	)

	ICOMMAND(r_ref, "V", (tagval *v, int n),
		if(!n) {ERRORF("r_registerref; requires the reference to be named"); return;}

		loopi(n) searchstack(v[i].getstr(), true);
	)

	ICOMMAND(r_local, "V", (tagval *v, int n),
		if(!n) {ERRORF("r_registertemp; requires the reference to be named"); return;}

		loopi(n) registertemp(v[i].getstr(), NULL);
	)

	ICOMMAND(r_setref, "ss", (const char *ref, const char *alias),
		if(!*ref) {ERRORF("r_setref; requires reference to be named"); return;}

		reference *r = searchstack(ref, true);
		if(!r || !r->canset()) return; //if this fails, then no stack

		reference *a = *alias ? searchstack(alias) : NULL;

		const char *refsep = strchr(ref, ':');
		const char *alsep = strchr(alias, ':');
		int refidx = refsep ? parseint(++refsep) : 0;
		int alidx = alsep ? parseint(++alsep) : 0;

		if(refsep)
		{
			if(!a)
			{
				if(DEBUG_VSCRIPT)
					DEBUGF("no valid alias, nulling %s:%i, if exists", ref, refidx);
				if(r->list.inrange(refidx))
					r->list[refidx] = reference::ref();
				return;
			}
			while(r->list.length() < refidx)
				r->list.add(reference::ref());
			if(!r->list.inrange(refidx))
			{
				ERRORF("Invalid reference index %i", refidx);
				return;
			}

			if(alsep)
			{
				if(!a->list.inrange(alidx))
				{
					ERRORF("%s has no ref at position %i", alias, alidx);
					r->list[refidx] = reference::ref();
				}
				else
				{
					if(DEBUG_VSCRIPT)
						DEBUGF("setting %s;%i to %s:%i", ref, refidx, alias, alidx);
					r->list[refidx] = a->list[alidx];
				}
			}
			else
			{
				if(DEBUG_VSCRIPT)
					DEBUGF("replacing %s:%i, with contents of %s", ref, refidx, alias);
				r->list.remove(refidx);
				r->list.insert(refidx, a->list.getbuf(), a->list.length());
			}

		}
		else
		{
			r->setnull();
			if(!a) return;

			if(alsep)
			{
				if(a->list.inrange(alidx))
					r->list.add(a->list[alidx]);
				else
					ERRORF("%s has no reference at position %i", alias, alidx);
			}
			else
				r->pushref(a);
		}
	)

	ICOMMAND(r_swapref, "ss", (const char *f, const char *s),
		if(!*f || !*s) {ERRORF("r_swapref; requires two valid references to swap"); return;}
		reference *a = searchstack(f);
		reference *b = searchstack(s);
		if(!a || !b) {ERRORF("r_swapref; either %s or %s is an invalid reference", f, s); return;}
		if(!a->canset() || b->canset()) return;
		swap(a->list, b->list);
	)

	ICOMMAND(r_refexists, "V", (tagval *v, int n),
		if(!n) {intret(0); return;}
		loopi(n) if(!searchstack(v[i].getstr(), false))
		{
			intret(0);
			return;
		}
		intret(1);
	)

	ICOMMAND(r_matchref, "ss", (const char *f, const char *s),
		if(!*f || !*s) {ERRORF("r_matchref; requires two valid references to compare"); intret(0); return;}
		reference *a = searchstack(f);
		reference *b = searchstack(s);
		if(!a || !b) {ERRORF("r_matchref; either %s or %s is an invalid reference", f, s); intret(0); return;}

		const char *asep = strchr(f, ':');
		const char *bsep = strchr(s, ':');
		int aidx = asep ? parseint(++asep) : 0;
		int bidx = bsep ? parseint(++bsep) : 0;

		if(asep && bsep)
		{
			reference::ref *ra = NULL;
			reference::ref *rb = NULL;
			if(a->list.inrange(aidx))
				ra = &a->list[aidx];
			if(b->list.inrange(bidx))
				rb = &b->list[bidx];

			if(ra && rb)
				intret(ra->ptr == rb->ptr);
			else if (ra || rb)
				intret(0);
			else
				intret(1);
		}
		else if(asep)
		{
			if(!a->list.inrange(aidx) && !b->list.length())
				intret(1);
			else if(a->list.inrange(aidx) && b->list.length() == 1)
				intret(a->list[aidx].ptr == b->list[0].ptr);
			else
				intret(0);
		}
		else if(bsep)
		{
			if(!b->list.inrange(bidx) && !a->list.length())
				intret(1);
			else if(b->list.inrange(bidx) && a->list.length() == 1)
				intret(b->list[bidx].ptr == a->list[0].ptr);
			else
				intret(0);
		}
		else
		{
			loopv(a->list)
			{
				if(!b->list.inrange(i) || a->list[i].ptr != b->list[i].ptr)
				{
					intret(0);
					return;
				}
			}
			intret(a->list.length() == b->list.length());
		}
	)

	ICOMMAND(r_ref_len, "s", (const char *ref),
		reference *r = searchstack(ref, false);
		if(r) intret(r->list.length());
		else intret(0);
	)

	ICOMMAND(r_ref_push, "ss", (const char *ref, const char *add),
		reference *r = searchstack(ref, true);
		reference *a = searchstack(add, false);



		if(a && r->canset())
		{
			const char *addsep = strchr(add, ':');
			if(addsep)
			{
				int idx = parseint(++addsep);
				if(a->list.inrange(idx))
					r->list.add(a->list[idx]);
				else
					ERRORF("nothing at %s; cannot append onto %s", add, ref);
			}
			else
				r->pushref(a);
		}
	)

	ICOMMAND(r_ref_sub, "ss", (const char *r, const char *c),
		reference *ref = searchstack(r, false);
		if(!ref || !ref->canset()) return;

		reference *children = searchstack(c, false);
		if(!children) return;
		const char *csep = strchr(c, ':');
		int cidx = csep ? parseint(++csep) : 0;
		if(!children->list.inrange(cidx)) return;

		if(ref == children)
		{
			if(!csep) ref->list.setsize(0);
			else ref->list.remove(cidx);
		}
		else if(csep)
		{
			loopv(ref->list)
			{
				if(ref->list[i].ptr == children->list[cidx].ptr)
					ref->list.remove(i--);
			}
		}
		else loopvj(children->list)
		{
			loopv(ref->list)
			{
				if(ref->list[i].ptr == children->list[j].ptr)
					ref->list.remove(i--);
			}
		}
	)

	ICOMMAND(r_ref_remove, "sii", (const char *ref, int *i, int *n),
		reference *r = searchstack(ref);
		if(!r || !r->canset()) return;

		*n = max(1, *n);
		int in = clamp(0, r->list.length(), *i);
		int num = max(1, *n + in - r->list.length());
		if(in != *i || *n != num) WARNINGF("r_ref_remove, arguments clamped fo %s: %i %i --> %i %i", ref, *i, *n, in, num);
		r->list.remove(in, num);
	)

	ICOMMAND(r_global_get, "s", (const char *n),
		rpgvar *var = variables.access(n);
		if(!var)
		{
			ERRORF("no global named \"%s\", returning \"\"", n);
			result("");
			return;
		}

		if(DEBUG_VSCRIPT)
			DEBUGF("global %s found, returning...\n\t%s", n, var->value);

		result(var->value);
	)

	bool setglobal(const char *n, const char *v, bool dup)
	{
		rpgvar *var = variables.access(n);

		if(!var)
		{
			if(DEBUG_VSCRIPT)
				DEBUGF("global variable \"%s\" does not exist, creating and setting to \"%s\"", n, v);

			var = &variables[n];
			var->name = queryhashpool(n);
			var->value = !dup ? v : newstring(v);
			return false;
		}

		if(DEBUG_VSCRIPT)
			DEBUGF("global variable \"%s\" exists, setting to \"%s\" from \"%s\"", n, v, var->value);

		delete[] var->value;
		var->value = !dup ? v : newstring(v);

		return true;
	}

	ICOMMAND(r_global_new, "ss", (const char *n, const char *v),
		if(setglobal(n, v))
			WARNINGF("r_global_new, variable \"%s\" already exists!", n);
	)

	ICOMMAND(r_global_set, "ss", (const char *n, const char *v),
		if(!setglobal(n, v))
			WARNINGF("r_global_set, variable \"%s\" doesn't exist, creating anyway.", n);
	)

	bool comparelocals(int a, int b)
	{
		if (a == b) return true;
		if (!locals.inrange(a) || !locals.inrange(b)) return false;

		hashnameset<rpgvar> &seta = locals[a]->variables,
			&setb = locals[b]->variables;

		if(seta.length() != setb.length()) return false;
		//ASSERT(seta->size == setb->size);

		//we walk both tables at once as they are of the same size.
		loopi(seta.size)
		{
			hashnameset<rpgvar>::chain *chaina = seta.chains[i], *chainb = setb.chains[i];
			if(!chaina != !chainb) return false;
			while(chaina)
			{
				while(chainb)
				{
					rpgvar &vara = chaina->elem, &varb = chainb->elem;
					if(vara.name == varb.name && !strcmp(vara.value, varb.value)) break;
					chainb = chainb->next;
				}
				if(!chainb) return false;
				chainb = setb.chains[i];
				chaina = chaina->next;
			}
		}

		return true;
	}

	int alloclocal(bool track)
	{
		localinst *l = NULL;
		int idx = -1;

		loopv(locals) if(locals[i] == NULL)
		{
			idx = i;
			l = locals[i] = new localinst();
			break;
		}
		if(!l)
		{
			idx = locals.length();
			l = locals.add(new localinst());
		}

		if(track) l->refs++;

		return idx;
	}

	void dumplocals()
	{
		loopvj(locals)
		{
			localinst *l = locals[j];
			conoutf("locals[%i] = %p", j, l);

			if(!l) continue;

			conoutf("referenced by %i objects", l->refs);
			enumerate(l->variables, rpgvar, var,
				conoutf("\t%s = %s", escapestring(var.name), escapestring(var.value));
			)
		}
	}
	COMMAND(dumplocals, "");

	bool keeplocal(int i)
	{
		if(i < 0) return true;

		if(!locals.inrange(i) || !locals[i])
		{
			dumplocals();
			ERRORF("Can't increse reference counter for local variable stack %i! \f3REPORT A BUG!", i);
			return false;
		}

		locals[i]->refs++;
		return true;
	}

	bool freelocal(int i)
	{
		if(i < 0) return true;

		if(!locals.inrange(i) || !locals[i])
		{
			dumplocals();
			ERRORF("Can't decrease reference counter for local variable stack %i! \f3REPORT A BUG!", i);
			return false;
		}

		if((--locals[i]->refs) <= 0)
		{
			if(locals[i]->refs < 0)
				ERRORF("locals stack[%i] decremented to %i! \f3REPORT A BUG!", i, locals[i]->refs);

			delete locals[i];
			locals[i] = NULL;
		}
		return true;
	}

	void cleanlocals()
	{
		loopv(locals)
		{
			if(locals[i] && locals[i]->refs <= 0)
			{
				if(locals[i]->refs < 0)
				ERRORF("locals stack[%i] erased with negative refcount! \f3REPORT A BUG!", i);

				delete locals[i];
				locals[i] = NULL;
			}
		}
	}

	int copylocal(int od)
	{
		int id = alloclocal();
		if(!locals.inrange(od) || !locals[od])
		{
			ERRORF("copylocal called with a bad instance id: %i\f3REPORT A BUG!", od);
			return id;
		}

		localinst &dst = *locals[id],
			&src = *locals[od];

		enumerate(src.variables, rpgvar, var,
			rpgvar &newvar = dst.variables[var.name];
			newvar.name = var.name;
			newvar.value = newstring(var.value);
		)

		return id;
	}

	ICOMMAND(r_local_get, "ss", (const char *ref, const char *name),
		getreference(ref, gen, gen->getinv(genidx) || gen->getent(genidx) || gen->getmap(genidx), result(""), r_local_get)

		int li = -1;
		if(gen->getinv(genidx))
			li = gen->getinv(genidx)->locals;
		else if(gen->getent(genidx))
			li = gen->getent(genidx)->locals;
		else if(gen->getmap(genidx))
			li = gen->getmap(genidx)->locals;

		if(li >= 0)
		{
			rpgvar *var = locals[li]->variables.access(name);
			if(var) result(var->value);
		}
	)

	ICOMMAND(r_local_set, "sss", (const char *ref, const char *name, const char *val),
		getreference(ref, gen, gen->getinv(genidx) || gen->getent(genidx) || gen->getmap(genidx), , r_local_get)

		int *li = NULL;

		if(gen->getinv(genidx))
			li = &gen->getinv(genidx)->locals;
		else if(gen->getent(genidx))
			li = &gen->getent(genidx)->locals;
		else if(gen->getmap(genidx))
			li = &gen->getmap(genidx)->locals;

		//critical error really
		if(!li) return;

		if(*li == -1) *li = alloclocal();
		else if(locals[*li]->refs >= 2)
		{
			freelocal(*li);
			*li = copylocal(*li);
		}

		rpgvar &var = locals[*li]->variables[name];
		if(!var.name) var.name = queryhashpool(name);
		delete[] var.value;
		var.value = newstring(val);
	)

	ICOMMAND(r_cat_get, "i", (int *i),
		if(categories.inrange(*i))
		{
			result(categories[*i]);
		}
		else
		{
			ERRORF("category[%i] does not exist", *i);
			result("");
		}
	)

	// Script Commands

	void sendsignal(const char *sig, const char *send, const char *rec, int prop)
	{
		if(!sig) return;
		reference *sender = NULL, *receiver = NULL;
		int sendidx, recidx;
		parseref(send, sendidx);
		parseref(rec, recidx);

		if(*send) sender = searchstack(send);
		if(*rec) receiver = searchstack(rec);

		if(DEBUG_VSCRIPT) DEBUGF("r_signal called with sig: %s - send: %s - rec: %s - prop: %i", sig, send, rec, prop);

		if(*rec && !receiver)
		{
			ERRORF("receiver does not exist");
			return;
		}
		else if(!receiver)
		{
			if(DEBUG_VSCRIPT) DEBUGF("no receiver, sending signal to everything");
			enumerate(game::mapdata, mapinfo, map,
				map.getsignal(sig, true, sender ? sender->getent(sendidx) : NULL);
			)
			camera::getsignal(sig);
		}
		else if(receiver->getmap(recidx))
		{
			if(DEBUG_VSCRIPT) DEBUGF("receiver is map, sending signal");
			receiver->getmap(recidx)->getsignal(sig, prop, sender ? sender->getent(sendidx) : NULL);
			return;
		}
		else  if(receiver->getent(recidx))
		{
			if(DEBUG_VSCRIPT) DEBUGF("receiver is ent, sending signal");
			receiver->getent(recidx)->getsignal(sig, prop, sender ? sender->getent(sendidx) : NULL);
			return;
		}
		else if(receiver->getinv(0))
		{
			if(DEBUG_VSCRIPT) DEBUGF("receiver is item, sending signal");
			receiver->getinv(recidx)->getsignal(sig, prop, sender ? sender->getent(sendidx) : NULL);
			return;
		}

	}
	COMMANDN(r_signal, sendsignal, "sssi");

	ICOMMAND(r_loop_maps, "se", (const char *ref, uint *body),
		if(!*ref) {ERRORF("r_loop_maps; requires reference to alias maps to"); return;}

		pushstack();
		reference *cur = registertemp(ref, NULL);
		cur->immutable = true;

		enumerate(mapdata, mapinfo, map,
			cur->setref(&map, true);
			rpgexecute(body);
		)

		popstack();
	)

	ICOMMAND(r_loop_ents, "sse", (const char *mapref, const char *ref, uint *body),
		if(!*ref) {ERRORF("r_loop_ents; requires map reference and a reference to alias ents to"); return;}
		getreference(mapref, map, map->getmap(mapidx), , r_loop_ents)

		pushstack();
		reference *cur = registertemp(ref, NULL);
		cur->immutable = true;

		loopv(map->getmap(mapidx)->objs)
		{
			cur->setref(map->getmap(mapidx)->objs[i], true);
			rpgexecute(body);
		}

		popstack();
	)

	ICOMMAND(r_setref_ents, "ss", (const char *ref, const char *mapref),
		if(!*ref) return;
		getreference(mapref, map, map->getmap(mapidx), , r_setref_ents)

		reference *entstack = registerref(ref, NULL);
		if(!entstack->canset()) return;

		entstack->setnull();
		loopv(map->getmap(mapidx)->objs)
			entstack->pushref(map->getmap(mapidx)->objs[i]);
	)

	ICOMMAND(r_loop_aeffects, "ssse", (const char *mapref, const char *vic, const char *ref, uint *body),
		if(!*ref) {ERRORF("r_loop_aeffects; requires map reference and a reference to alias ents to"); return;}
		getreference(mapref, map, map->getmap(mapidx), , r_loop_aeffects)

		rpgent *victim = NULL;
		if(*vic)
		{
			if(DEBUG_VSCRIPT) DEBUGF("r_loop_aeffects, entity reference provided, may do bounds checking");
			int vidx;
			parseref(vic, vidx);
			reference *v = searchstack(vic, false);
			if(v) victim = v->getent(vidx);
		}

		pushstack();
		reference *cur = registertemp(ref, NULL);
		cur->immutable = true;

		loopv(map->getmap(mapidx)->aeffects)
		{
			areaeffect *ae = map->getmap(mapidx)->aeffects[i];
			if(victim && victim->o.dist(ae->o) > ae->radius && victim->feetpos().dist(ae->o) > ae->radius) continue;

			cur->setref(ae, true);
			rpgexecute(body);
		}

		popstack();
	)

	ICOMMAND(r_loop_inv, "sse", (const char *entref, const char *ref, uint *body),
		if(!*ref) {ERRORF("r_loop_inv, requires ent reference and reference to alias items to"); return;}
		getreference(entref, ent, ent->getchar(entidx) || ent->getcontainer(entidx), , r_loop_inv)

		pushstack();
		reference *it = registertemp(ref, NULL);
		it->immutable = true;

		if(ent->getchar(entidx)) enumerate(ent->getchar(entidx)->inventory, vector<item *>, stack,
			it->setref(&stack, true);
			rpgexecute(body);
		)
		else enumerate(ent->getcontainer(entidx)->inventory, vector<item *>, stack,
			it->setref(&stack, true);
			rpgexecute(body);
		)

		popstack();
	)

	ICOMMAND(r_setref_invstack, "sss", (const char *ref, const char *srcref, const char *base),
		if(!*ref) {ERRORF("r_setref_invstack; requires ent reference and a reference name"); return;}
		getreference(srcref, src, src->getchar(srcidx) || src->getcontainer(srcidx), , r_setref_invstack)

		reference *invstack = registerref(ref, NULL);
		if(!invstack->canset()) return;

		base = queryhashpool(base);

		if(src->getchar(srcidx))
			invstack->setref(&src->getchar(srcidx)->inventory.access(base, vector<item *>()));
		else
			invstack->setref(&src->getcontainer(srcidx)->inventory.access(base, vector<item *>()));
	)

	ICOMMAND(r_setref_inv, "sss", (const char *ref, const char *srcref, const char *base),
		if(!*ref) {ERRORF("r_setref_inv; requires ent reference and a reference name"); return;}
		getreference(srcref, src, src->getchar(srcidx) || src->getcontainer(srcidx), , r_setref_inv)

		reference *inv = registerref(ref, NULL);
		if(!inv->canset()) return;

		base = queryhashpool(base);

		if(src->getchar(srcidx))
		{
			vector <item *> &stack = src->getchar(srcidx)->inventory.access(base, vector<item *>());
			if(!stack.length()) { stack.add(new item())->init(base); stack[0]->quantity = 0; }
			inv->setref(stack[0]);
		}
		else if(src->getcontainer(srcidx))
		{
			vector <item *> &stack = src->getcontainer(srcidx)->inventory.access(base, vector<item *>());
			if(!stack.length()) { stack.add(new item())->init(base); stack[0]->quantity = 0; }
			inv->setref(stack[0]);
		}
	)

	ICOMMAND(r_setref_equip, "ssi", (const char *ref, const char *entref, int *filter),
		getreference(entref, ent, ent->getchar(entidx), , r_setref_equip)

		rpgchar *d = ent->getchar(entidx);
		reference *equip = searchstack(ref, true);
		if(!equip->canset()) return;

		equip->setnull();
		loopv(d->equipped)
		{
			use_armour *ua = (use_armour *) d->equipped[i]->it->uses[d->equipped[i]->use];
			if((!*filter && !ua->slots) || (ua->slots & *filter))
				equip->pushref(d->equipped[i]);
		}
	)

	ICOMMAND(r_loop_veffects, "sse", (const char *entref, const char *ref, uint *body),
		if(!*ref) {ERRORF("r_loop_veffects; requires ent reference and a reference name"); return;}
		getreference(entref, ent, ent->getent(entidx), , r_loop_veffects)

		pushstack();
		reference *effect = registertemp(ref, NULL);
		effect->immutable = true;

		vector<victimeffect *> &seffects = ent->getent(entidx)->seffects;
		loopv(seffects)
		{
			effect->setref(seffects[i], true);
			rpgexecute(body);
		}

		popstack();
	)

	// Highly generic stuff

	ICOMMAND(r_get_attr, "sii", (const char *ref, int *attr, int *u),
		if(*attr < 0 || *attr >= STAT_MAX) {ERRORF("r_get_attr; attribute %i out of range", *attr); intret(0); return;}
		getreference(ref, obj, obj->getchar(objidx) || obj->getinv(objidx), intret(0), r_get_attr)
		if(obj->getchar(objidx))
			intret(obj->getchar(objidx)->base.getattr(*attr));
		else
		{
			if(obj->getequip(objidx)) *u = obj->getequip(objidx)->use;

			item *it = obj->getinv(objidx);
			use_armour *ua = NULL;
			if(it->uses.inrange(*u))
				ua = (use_armour *) it->uses[*u];

			if(ua && ua->type >= USE_CONSUME)
				intret(ua->reqs.attrs[*attr]);
			else
				intret(0);
		}
	)

	ICOMMAND(r_get_skill, "sii", (const char *ref, int *skill, int *u),
		if(*skill < 0 || *skill >= SKILL_MAX) {ERRORF("r_get_skill; skill %i out of range", *skill); intret(0); return;}
		getreference(ref, obj, obj->getchar(objidx) || obj->getinv(objidx) || obj->getequip(objidx), intret(0), r_get_skill)
		if(obj->getchar(objidx))
			intret(obj->getchar(objidx)->base.getskill(*skill));
		else
		{
			if(obj->getequip(objidx)) *u = obj->getequip(objidx)->use;

			item *it = obj->getinv(objidx);
			use_armour *ua = NULL;
			if(it->uses.inrange(*u))
				ua = (use_armour *) it->uses[*u];

			if(ua && ua->type >= USE_CONSUME)
				intret(ua->reqs.skills[*skill]);
			else
				intret(0);
		}
	)

	ICOMMAND(r_get_weight, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx) || ent->getcontainer(entidx) || ent->getinv(entidx) || ent->getequip(entidx), floatret(0), r_get_weight)

		if(ent->getent(entidx))
			floatret(ent->getent(entidx)->getweight());
		else if(ent->getequip(entidx))
			floatret(ent->getinv(entidx)->weight);
		else
			floatret(ent->getinv(entidx)->quantity * ent->getinv(entidx)->weight);
	)

	ICOMMAND(r_get_name, "si", (const char *ref, int *u),
		getreference(ref, obj, obj->getent(objidx) || obj->getinv(objidx) || obj->getequip(objidx), result(""), r_get_name)
		if(obj->getinv(objidx))
		{
			item *it = obj->getinv(objidx);
			if(obj->getequip(objidx)) *u = obj->getequip(objidx)->use;

			if(it->uses.inrange(*u))
				result(it->uses[*u]->name ? it->uses[*u]->name : "");
			 else
				result(it->name ? it->name : "");
		}
		else
			result(obj->getent(objidx)->getname() ? obj->getent(objidx)->getname() : "");
	)

	ICOMMAND(r_get_type, "s", (const char *ref),
		getreference(ref, ent, ent->getent(entidx), intret(0), r_get_type)
		intret(ent->getent(entidx)->type());
	)

	// Item Commands

	ICOMMAND(r_get_base, "s", (const char *ref),
		getreference(ref, item, item->getinv(itemidx), intret(-1), r_get_base)
		result(item->getinv(itemidx)->base);
	) //TODO save bases for everything and return here.

	ICOMMAND(r_get_use, "s", (const char *ref),
		getreference(ref, equip, equip->getequip(equipidx), intret(-1), r_get_use)
		intret(equip->getequip(equipidx)->use);
	)

	// Entity Commands

	ICOMMAND(r_get_state, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(CS_DEAD), r_get_state)

		intret(ent->getent(entidx)->state);
	)

	ICOMMAND(r_get_pos, "s", (const char *ref),
		getreference(ref, ent, ent->getent(entidx), result("0 0 0"), r_get_pos)
		static string s; formatstring(s, "%f %f %f", ent->getent(entidx)->o.x, ent->getent(entidx)->o.y, ent->getent(entidx)->o.z);
		result(s);
	)

	ICOMMAND(r_add_status, "ssfi", (const char *ref, const char *st, float *mul, int *flags),
		if(!statuses.access(st)) return;
		getreference(ref, victim, victim->getent(victimidx), , r_add_effect)
		statusgroup *sg = statuses.access(st);

		inflict inf(sg, ATTACK_NONE, 1); //resistances aren't applied
		victim->getent(victimidx)->seffects.add(new victimeffect(NULL, &inf, *flags, *mul));
	)

	ICOMMAND(r_kill, "ss", (const char *ref, const char *killer), //I was framed!
		getreference(ref, victim, victim->getchar(victimidx), , r_kill)
		int nastyidx;
		parseref(killer, nastyidx);
		reference *nasty = *killer ? searchstack(killer) : NULL;

		if(DEBUG_SCRIPT) DEBUGF("r_kill; killing \"%s\" with \"%s\" as the killer", ref, killer);
		victim->getent(victimidx)->die(nasty ? nasty->getent(nastyidx) : NULL);
	)

	ICOMMAND(r_resurrect, "s", (const char *ref),
		getreference(ref, target, target->getchar(targetidx), , r_resurrect)

		if(DEBUG_SCRIPT) DEBUGF("r_resurrect; trying to revive \"%s\"", ref);
		target->getchar(targetidx)->revive(false);
	)

	ICOMMAND(r_pickup, "ss", (const char *finder, const char *keepsake), //finders keepers :P
		getreference(finder, actor, actor->getchar(actoridx), , r_pickup)
		getreference(keepsake, item, item->getitem(itemidx), , r_pickup)

		if(DEBUG_SCRIPT) DEBUGF("r_pickup; reference \"%s\" trying to pickup \"%s\"", finder, keepsake);
		actor->getchar(actoridx)->pickup(item->getitem(itemidx));
		if(!item->getitem(itemidx)->quantity)
		{
			if(DEBUG_SCRIPT) DEBUGF("r_pickup; keepsake has no more items left, queueing for obit");
			 obits.add(item->getent(itemidx));
		}
	)

	ICOMMAND(r_additem, "ssie", (const char *ref, const char *base, int *q, uint *body),
		getreference(ref, ent, ent->getchar(entidx) || ent->getcontainer(entidx), , r_additem)

		if(DEBUG_SCRIPT) DEBUGF("added %i instances of item \"%s\" to reference \"%s\"", *q, base, ref);
		item *it = ent->getent(entidx)->additem(base, max(0, *q));
		doitemscript(it, ent->getchar(entidx), body);
	)

	ICOMMAND(r_get_amount, "ss", (const char *ref, const char *base),
		getreference(ref, ent, ent->getchar(entidx) || ent->getcontainer(entidx) || ent->getinv(entidx), intret(0), r_get_amount)

		if(ent->getinv(entidx))
			intret(ent->getinv(entidx)->quantity);
		else
			intret(ent->getent(entidx)->getitemcount(base));
	)

	ICOMMAND(r_drop, "ssi", (const char *ref, const char *itref, int *q),
		getreference(ref, ent, ent->getchar(entidx) || ent->getcontainer(entidx), intret(0), r_drop)
		getreference(itref, it, it->getinv(itidx), intret(0), r_drop)

		int dropped = ent->getent(entidx)->drop(it->getinv(itidx), max(0, *q), true);
		if(DEBUG_SCRIPT) DEBUGF("dropping %i of reference \"%s\" from reference \"%s\" - %i dropped", *q, itref, ref, dropped);
		intret(dropped);
	)

	ICOMMAND(r_remove, "ssi", (const char *ref, const char *itref, int *q),
		getreference(ref, ent, ent->getchar(entidx) || ent->getcontainer(entidx), intret(0), r_remove)
		getreference(itref, it, it->getinv(itidx), intret(0), r_remove)

		int dropped = ent->getent(entidx)->drop(it->getinv(itidx), max(0, *q), false);
		if(DEBUG_SCRIPT) DEBUGF("removing %i of reference \"%s\" from reference \"%s\" - %i removing", *q, itref, ref, dropped);
		intret(dropped);
	)

	ICOMMAND(r_canequip, "ssi", (const char *ref, const char *itref, int *u),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_canequip)
		getreference(itref, it, it->getinv(itidx), intret(0), r_canequip)

		if(it->getequip(itidx)) *u = it->getequip(itidx)->use;

		if(it->getinv(itidx)->uses.inrange(*u))
		{
			use_armour *ua = (use_armour *) it->getinv(itidx)->uses[*u];
			intret(ua->reqs.meet(ent->getchar(entidx)->base));
		}
		else intret(0);
	)

	ICOMMAND(r_equip, "ssi", (const char *ref, const char *itref, int *u),
		getreference(ref, ent, ent->getchar(entidx), , r_equip)
		getreference(itref, it, it->getinv(itidx), , r_equip)

		ent->getent(entidx)->equip(it->getinv(itidx), *u);
	)

	ICOMMAND(r_dequip, "stiN", (const char *ref, tagval *which, int *slot, int *numargs),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_dequip)
		if(*numargs > 2)
		{
			const char *base = which->getstr();
			if(which->type == VAL_NULL || !base[0]) base = NULL;
			intret(ent->getchar(entidx)->dequip(base, *slot));
		}
		else //2 arg version
		{
			if(DEBUG_SCRIPT) DEBUGF("attempting 2-arg dequip on reference %s for (equip?) reference %s", ref, which->getstr());
			getreference(which->getstr(), eq, eq->getequip(eqidx), intret(0), r_dequip)

			intret(ent->getchar(entidx)->dequip(eq->getequip(eqidx)->it->base, eq->getequip(eqidx)->use));
		}
	)

	ICOMMAND(r_use, "ssiN", (const char *ref, const char *itref, int *use, int *numargs),
		getreference(ref, ent, ent->getchar(entidx), , r_use)
		getreference(itref, it, it->getinv(itidx) || it->getequip(itidx), , r_use);

		item *item = NULL;
		int u = -1;

		if(it->getequip(itidx))
		{
			item = it->getequip(itidx)->it;
			u = it->getequip(itidx)->use;
		}
		else if(it->getinv(itidx))
			item = it->getinv(itidx);

		if(*numargs >= 3)
			u = *use;

		ent->getchar(entidx)->useitem(item, NULL, u);
	)

	ICOMMAND(r_transfer, "sssi", (const char *fromref, const char *toref, const char *itref, int *q),
		getreference(fromref, from, from->getchar(fromidx) || from->getcontainer(fromidx), , r_transfer)
		getreference(toref, to, to->getchar(toidx) || to->getcontainer(toidx), , r_transfer)
		*q = max(1, *q);

		//Just save us the effort and further complications.
		if(from == to)
		{
			WARNINGF("from and to reference is identical");
			return;
		}

		const char *itsep = strchr(itref, ':');
		int itidx = itsep ? parseint(++itsep) : 0;
		reference *it = searchstack(itref, false);
		if(!it)
		{
			ERRORF("r_transfer: reference %s does not exist", itref);
			return;
		}

		for(; itidx < it->list.length(); itidx++)
		{
			if(!it->getinv(itidx))
			{
				WARNINGF("%s at index %i is not an item!", itref, itidx);
				if(itsep) break;
				continue;
			}
			if(DEBUG_SCRIPT)
				 DEBUGF("attempting to move %i units of item %s (index %i) (%p) to entity %s (%p) from %s (%p)", *q, itref, itidx, it->getinv(itidx), toref, to->getent(toidx), fromref, from->getent(fromidx));

			vector<item *> *stack = NULL;
			if(from->getchar(fromidx)) stack = from->getchar(fromidx)->inventory.access(it->getinv(itidx)->base);
			else stack = from->getcontainer(fromidx)->inventory.access(it->getinv(itidx)->base);

			if(stack->find(it->getinv(itidx)) == -1)
			{
				WARNINGF("item %p does not exist in the inventory of %p", it->getinv(itidx), from->getent(toidx));
				if(itsep) break;
				continue;
			}

			int orig = it->getinv(itidx)->quantity;
			it->getinv(itidx)->quantity = min(*q, it->getinv(itidx)->quantity);
			to->getent(toidx)->additem(it->getinv(itidx));
			it->getinv(itidx)->quantity = max(orig - *q, 0);

			if(DEBUG_SCRIPT)
				 DEBUGF("transferred up to %i instances of %i from item %p to %p", *q, orig, it->getinv(itidx), to->getent(toidx));

			if(itsep) break;
		}
	)

	ICOMMAND(r_trigger, "s", (const char *ref),
		getreference(ref, obj, obj->gettrigger(objidx), , r_trigger)

		obj->gettrigger(objidx)->lasttrigger = lastmillis;
		obj->gettrigger(objidx)->flags ^= rpgtrigger::F_TRIGGERED;

		if(DEBUG_SCRIPT) DEBUGF("triggered reference \"%s\", triggered flag is now %i", ref, obj->gettrigger(0)->flags & rpgtrigger::F_TRIGGERED);
	)

	ICOMMAND(r_destroy, "s", (const char *victim),
		getreference(victim, obit, obit->getent(obitidx), , r_destroy)
		if(obit->getent(obitidx) == trader->getent(0)) return;

		if(DEBUG_SCRIPT) DEBUGF("queueing reference %s for destruction", victim);
		obits.add(obit->getent(obitidx));
	)

	// Entity - stats

	ICOMMAND(r_get_mana, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_mana)
		intret(ent->getchar(entidx)->mana);
	)

	ICOMMAND(r_get_maxmana, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_maxmana)
		intret(ent->getchar(entidx)->base.getmaxmp());
	)

	ICOMMAND(r_get_manaregen, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_manaregen)
		floatret(ent->getchar(entidx)->base.getmpregen())
	)

	ICOMMAND(r_get_health, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_health)
		intret(ent->getchar(entidx)->health);
	)

	ICOMMAND(r_get_maxhealth, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_maxhealth)
		intret(ent->getchar(entidx)->base.getmaxhp());
	)

	ICOMMAND(r_get_healthregen, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_healthregen)
		floatret(ent->getchar(entidx)->base.gethpregen())
	)

	ICOMMAND(r_get_resist, "si", (const char *ref, int *elem),
		if(*elem < 0 || *elem >= ATTACK_MAX) {ERRORF("r_get_resist; element %i out of range", *elem); intret(0); return;}
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_resist)
		intret(ent->getchar(entidx)->base.getresistance(*elem));
	)

	ICOMMAND(r_get_thresh, "si", (const char *ref, int *elem),
		if(*elem < 0 || *elem >= ATTACK_MAX) {ERRORF("r_get_thres; element %i out of range", *elem); intret(0); return;}
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_thresh)
		intret(ent->getchar(entidx)->base.getthreshold(*elem));
	)

	ICOMMAND(r_get_neededexp, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_neededexp)
		intret(stats::neededexp(ent->getchar(entidx)->base.level));
	)

	ICOMMAND(r_get_maxweight, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_maxweight)
		intret(ent->getchar(entidx)->base.getmaxcarry());
	)

	ICOMMAND(r_get_maxspeed, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_maxspeed)
		float spd; float jmp;
		ent->getchar(entidx)->base.setspeeds(spd, jmp);
		floatret(spd);
	)

	ICOMMAND(r_get_jumpvel, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_jumpvel)
		float spd; float jmp;
		ent->getchar(entidx)->base.setspeeds(spd, jmp);
		floatret(jmp);
	)

	ICOMMAND(r_invest_attr, "si", (const char *ref, int *attr),
		if(*attr < 0 || *attr >= STAT_MAX) return;
		getreference(ref, ent, ent->getchar(entidx), , r_invest_attr)
		if(ent->getchar(entidx)->base.statpoints <= 0 || ent->getchar(entidx)->base.baseattrs[*attr] >= 100) return;

		ent->getchar(entidx)->base.statpoints--;
		ent->getchar(entidx)->base.baseattrs[*attr]++;
	)

	ICOMMAND(r_invest_skill, "si", (const char *ref, int *skill),
		if(*skill < 0 || *skill >= SKILL_MAX) return;
		getreference(ref, ent, ent->getchar(entidx), , r_invest_skill)
		if(ent->getchar(entidx)->base.skillpoints <= 0 || ent->getchar(entidx)->base.baseskills[*skill] >= 100) return;

		ent->getchar(entidx)->base.skillpoints--;
		ent->getchar(entidx)->base.baseskills[*skill]++;
	)

	ICOMMAND(r_givexp, "si", (const char *ref, int *n),
		getreference(ref, ent, ent->getchar(entidx), , r_givexp)
		ent->getchar(entidx)->givexp(*n);
	)

	ICOMMAND(r_get_lastaction, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx) || ent->gettrigger(entidx), intret(lastmillis), r_get_lastaction)

		if(ent->getchar(entidx)) intret(ent->getchar(entidx)->lastaction);
		else if(ent->gettrigger(entidx)) intret(ent->gettrigger(entidx)->lasttrigger);
	)

	ICOMMAND(r_get_charge, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_charge)
		intret(ent->getchar(entidx)->charge);
	)

	ICOMMAND(r_get_primary, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_primary)
		intret(ent->getchar(entidx)->primary);
	)

	ICOMMAND(r_get_secondary, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), intret(0), r_get_secondary)
		intret(ent->getchar(entidx)->secondary);
	)

	ICOMMAND(r_check_recipe, "si", (const char *id, int *num),
		recipe *r = recipes.access(id);
		if(!r)
		{
			ERRORF("r_check_recipe; recipe %s out of range", id);
			intret(0); return;
		}
		intret(r->checkreqs(game::player1, max(1, *num)));
	)

	ICOMMAND(r_use_recipe, "si", (const char *id, int *num),
		recipe *r = recipes.access(id);
		if(!r)
		{
			ERRORF("r_use_recipe; recipe %s out of range", id);
			return;
		}
		r->craft(game::player1, max(1, *num));
	)

	ICOMMAND(r_learn_recipe, "s", (const char *id),
		recipe *r = recipes.access(id);
		if(!r)
		{
			ERRORF("r_learn_recipe; recipe %s out of range", id);
			return;
		}
		r->flags |= recipe::KNOWN;
	)

	//dialogue
	//see also r_script_say in rpgconfig.cpp
	ICOMMAND(r_chat, "ss", (const char *s, const char *node),
		getreference(s, speaker, speaker->getent(speakeridx), , r_chat)
		if(speaker != talker && talker->list.length()) talker->setnull(true);

		script *scr = speaker->getent(speakeridx)->getscript();

		talker->setref(speaker->getent(speakeridx), true);

		if(scr->curnode) scr->curnode->close();
		scr->curnode = NULL;

		if(node[0])
		{
			scr->curnode = scr->chat.access(node);
			if(!scr->curnode)
				ERRORF("no such dialogue node: %s", node);
		}
		if(scr->curnode)
		{
			scr->curnode->close();
			scr->curnode->open();

			if(!scr->curnode->choices.length())
			{
				//if(DEBUGF print something
				//there are no destinations so just print the text and close...
				game::hudline("%s: %s", talker->getent(0)->getname(), scr->curnode->str);
				scr->curnode->close();
				scr->curnode = NULL;
			}
		}

		if(!scr->curnode) talker->setnull(true);
	)

	ICOMMAND(r_response, "sss", (const char *t, const char *n, const char *s),
		if(!talker->getent(0)) { ERRORF("r_response; no conversation in progress?"); return; }

		script *scr = talker->getent(0)->getscript();
		if(scr->curnode)
		{
			if(DEBUG_SCRIPT) DEBUGF("added response for scr->chat[%s]: \"%s\" \"%s\" \"%s\"", scr->curnode->node, t, n, s);
			 scr->curnode->choices.add(new response(t, n, s));
		}
		else
			ERRORF("currently in a non-existant dialogue");
	)

	ICOMMAND(r_trade, "s", (const char *str),
		getreference(str, cont, cont->getchar(contidx) || cont->getcontainer(contidx), , r_trade)
		if(cont->getchar(contidx) && !cont->getchar(contidx)->merchant) return;
		if(cont->getcontainer(contidx) && !cont->getcontainer(contidx)->merchant) return;
		trader->setref(cont->getent(contidx), true);
	)

	ICOMMAND(r_loot, "s", (const char *str),
		getreference(str, cont, cont->getchar(contidx) || cont->getcontainer(contidx), , r_loot)
		looter->setref(cont->getent(contidx), true);
		rpgexecute("showloot");
	)

	// World Commands

	//ICOMMAND(r_resetmap, "s", (const char *ref), - resets contents status

	ICOMMAND(r_on_map, "ss", (const char *mapref, const char *entref),
		getreference(mapref, map, map->getmap(mapidx), intret(0), r_on_map)
		getreference(entref, ent, ent->getent(entidx), intret(0), r_on_map)

		intret(map->getmap(mapidx)->objs.find(ent->getent(entidx)) >= 0);
	)

	#define SPAWNCOMMAND(type, con) \
		ICOMMAND(r_spawn_ ## type, "sisii", (const char *ref, int *attra, const char *attrb, int *attrc, int *attrd), \
			getreference(ref, map, map->getmap(mapidx), , r_spawn_ ## type); \
			 \
			action_spawn *spawn = new action_spawn con; \
			if(map->getmap(mapidx) == curmap) \
			{ \
				if(DEBUG_SCRIPT) DEBUGF("conditions met, spawning (" #type ") %i %s %i %i", *attra, attrb, *attrc, *attrd); \
				spawn->exec(); \
				delete spawn; \
			} \
			else \
			{ \
				if(DEBUG_SCRIPT) DEBUGF("conditions met but on another map queueing (" #type ") %i %s %i %i", *attra, attrb, *attrc, *attrd); \
				map->getmap(mapidx)->loadactions.add(spawn); \
			} \
		)
	//tag, index, number, quantity
	SPAWNCOMMAND(item, (*attra, ENT_ITEM, attrb, *attrc, max(1, *attrd)))
	//tag, index, number
	SPAWNCOMMAND(critter, (*attra, ENT_CHAR, attrb, *attrc, 1))
	//tag, index, number
	SPAWNCOMMAND(obstacle, (*attra, ENT_OBSTACLE, attrb, *attrc, 1))
	//tag, index, number
	SPAWNCOMMAND(container, (*attra, ENT_CONTAINER, attrb, *attrc, 1))
	//tag, index, number
	SPAWNCOMMAND(platform, (*attra, ENT_PLATFORM, attrb, *attrc, 1))
	//tag, index, number
	SPAWNCOMMAND(trigger, (*attra, ENT_TRIGGER, attrb, *attrc, 1))

	#undef SPAWNCOMMAND

	ICOMMAND(r_teleport, "sis", (const char *ref, int *d, const char *m),
		getreference(ref, ent, ent->getent(entidx), , r_teleport)

		mapinfo *destmap = *m ? accessmap(m) : curmap;

		if(ent->getent(entidx) == player1 && curmap != destmap)
		{
			if(DEBUG_SCRIPT) DEBUGF("teleporting player to map %s", m);
			destmap->loadactions.add(new action_teleport(ent->getent(0), *d));
			load_world(m);
			return;
		}
		if(ent->getent(entidx) != player1)
		{
			if(DEBUG_SCRIPT) DEBUGF("moving creature to destination map");
			removereferences(ent->getent(entidx), false);
			destmap->objs.add(ent->getent(entidx));

			if(destmap != curmap)
			{
				if(DEBUG_SCRIPT) DEBUGF("different map, queueing teleport on map %s", m);
				destmap->loadactions.add(new action_teleport(ent->getent(entidx), *d));
				return;
			}
		}

		if(DEBUG_SCRIPT) DEBUGF("teleporting creature to destination %d", *d);
		entities::teleport(ent->getent(entidx), *d);
	)

	//commands for status effects

	ICOMMAND(r_get_status_owner, "ss", (const char *ref, const char *set),
		if(!*set) return;
		getreference(ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), , r_get_status_owner)
		reference *owner = searchstack(set, true);
		if(eff->getveffect(effidx)) owner->setref(eff->getveffect(effidx)->owner);
		else owner->setref(eff->getaeffect(effidx)->owner);
	)

	ICOMMAND(r_get_status_num, "s", (const char *ref),
		getreference(ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), intret(0), r_get_status_num)
		if(eff->getveffect(effidx)) intret(eff->getveffect(effidx)->effects.length());
		else intret(eff->getaeffect(effidx)->effects.length());
	)

	ICOMMAND(r_get_status, "si", (const char *ref, int *idx),
		getreference(ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), result("-1 0 0 0 0"), r_get_status_num)

		vector<status *> *seffects;
		if(eff->getveffect(effidx)) seffects = &eff->getveffect(effidx)->effects;
		else seffects = &eff->getaeffect(effidx)->effects;

		if(!seffects->inrange(*idx)) return;

		status *s = (*seffects)[*idx];

		//should we return mode specific details? ala rpgconfig.cpp::r_status_get_effect
		defformatstring(str, "%i %i %i %i %f", s->type, s->strength, s->duration, s->remain, s->variance);
		result(str);
	)

	ICOMMAND(r_get_status_duration, "si", (const char *ref, int *idx),
		getreference(ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), intret(0), r_get_status_duration)
		vector<status *> &effects = eff->getveffect(effidx) ? eff->getveffect(effidx)->effects : eff->getaeffect(effidx)->effects;
		int val = 0;

		if(*idx >= 0)
		{
			if(!effects.inrange(*idx))
			{
				val = 0;
				ERRORF("r_get_status_duration, invalid index");
			}
			else
				val = effects[*idx]->duration;
		}
		else loopv(effects)
		{
			val = max(val, effects[i]->duration);
		}

		if(DEBUG_VSCRIPT) DEBUGF("r_get_status_duration; returning %i for \"%s\" %i", val, ref, *idx);
		intret(val);
	)

	ICOMMAND(r_get_status_remain, "si", (const char *ref, int *idx),
		getreference(ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), intret(0), r_get_status_remain)
		vector<status *> &effects = eff->getveffect(effidx) ? eff->getveffect(effidx)->effects : eff->getaeffect(effidx)->effects;
		int val = 0;

		if(*idx >= 0)
		{
			if(!effects.inrange(*idx))
			{
				val = 0;
				ERRORF("r_get_status_remain, invalid index");
			}
			else
				val = effects[*idx]->remain;
		}
		else loopv(effects)
		{
			val = max(val, effects[i]->remain);
		}

		if(DEBUG_VSCRIPT) DEBUGF("r_get_status_remain; returning %i for \"%s\" %i", val, ref, *idx);
		intret(val);
	)

	ICOMMAND(r_sound, "ss", (const char *p, const char *r),
		if(*r)
		{
			getreference(r, ent, ent->getent(entidx), , r_sound)
			playsoundname(p, &ent->getent(entidx)->o);
		}
		else
			playsoundname(p);
	)

	ICOMMAND(r_journal_record, "ssb", (const char *bucket, const char *entry, int *status),
		journal *journ = &game::journals[bucket];
		if(!journ->name)
			journ->name = game::queryhashpool(bucket);


		game::hudline("Journal updated: %s", bucket);
		journ->entries.add(newstring(entry));
		if(*status >= 0)
			journ->status = clamp<>(*status, int(JRN_ACCEPTED), int(JRN_MAX - 1));
	)

	ICOMMAND(r_journal_getstatus, "s", (const char *bucket),
		journal *journ = game::journals.access(bucket);
		intret(journ ? journ->status : JRN_RUMOUR);
	)

	static inline bool sortjournal(journal *a, journal *b)
	{
		//we want the status of the lowest value at the top.
		if(a->status < b->status) return true;
		if(a->status == b->status) return strcmp(a->name, b->name) > 0;
		return false;
	}

	ICOMMAND(r_journal_loop_buckets, "re", (ident *id, const uint *body),
		loopstart(id, stack);

		vector<journal *> buckets;

		enumerate(game::journals, journal, bucket,
			buckets.add(&bucket);
		)
		buckets.sort(sortjournal);

		loopv(buckets)
		{
			loopiter(id, stack, buckets[i]->name);
			rpgexecute(body);
		}

		loopend(id, stack);
	)

	ICOMMAND(r_journal_loop_entries, "sre", (const char *bucket, ident *id, const uint *body),
		loopstart(id, stack);

		journal *journ = game::journals.access(bucket);
		if(!journ) return;

		loopv(journ->entries)
		{
			loopiter(id, stack, journ->entries[i]);
			rpgexecute(body);
		}

		loopend(id, stack);
	)
}
