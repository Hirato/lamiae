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

bool timer::update()
{
	remaining -= curtime;
	if(remaining > 0 || camera::cutscene) return true;

	uint *cond = compilecode(this->cond);
	uint *script = compilecode(this->script);
	while(remaining <= 0)
	{
		remaining += delay;
		if(!executebool(cond)) { freecode(cond); freecode(script); return false; }

		rpgexecute(script);
	}

	freecode(cond);
	freecode(script);
	return true;
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
	hashnameset<timer> timers;
	reference *player = NULL;
	reference *hover = NULL;
	reference *map = NULL;
	reference *talker = NULL;
	reference *looter = NULL;
	reference *trader = NULL;
	reference *config = NULL; //dummy for creation and initialisation.

	VARP(debugreference, 0, 1, 1);

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

	reference *searchstack(const char *name, int &idx, bool &hasidx, bool create)
	{
		if(!stack.length())
		{
			ERRORF("no stack");
			return NULL;
		}

		stringslice lookup(name, 0);
		while(lookup.str[lookup.len] != '\0' && lookup.str[lookup.len] != ':')
			lookup.len++;

		if(lookup.str[lookup.len] != ':')
		{
			idx = 0; hasidx = false;
		}
		else
		{
			hasidx = true;
			idx = parseint(lookup.str + lookup.len + 1);
		}

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
			if(DEBUG_VSCRIPT) DEBUGF("reference \"%.*s\" not found, creating", lookup.len, lookup.str);
			const char *refname = queryhashpool(lookup);
			reference *ref = &(*stack[0])[refname];
			ref->name = refname;
			return ref;

		}
		if(DEBUG_VSCRIPT) DEBUGF("reference \"%.*s\" not found", lookup.len, lookup.str);
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

	ICOMMAND(r_timer, "siss", (const char *name, int *interval, const char *cond, const char *scr),
		if(!stack.length()) return;

		*interval = max(50, *interval);

		if(DEBUG_SCRIPT) DEBUGF("Registering timer[%s] with interval %i", name, *interval);
		name = game::queryhashpool(name);
		timer *t = &timers[name];

		t->name = name;
		t->delay = t->remaining = *interval;
		delete[] t->cond; t->cond = newstring(cond);
		delete[] t->script; t->script = newstring(scr);
	)

	void clean()
	{
		delaystack.deletecontents();
		timers.clear();
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

	static inline void replacerefs(hashnameset<reference> &refs, void *orig, void *next)
	{
		enumerate(refs, reference, ref,
			loopvk(ref.list) if(ref.list[k].ptr == orig)
			{
				if(next) ref.list[k].ptr = next;
				else ref.list.remove(k--);
			}
		)
	}

	void replacerefs(void *orig, void *next)
	{
		loopv(stack) replacerefs(*stack[i], orig, next);
		loopv(delaystack) replacerefs(delaystack[i]->refs, orig, next);
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

		vector<timer *> expiredtimers;
		enumerate(timers, timer, t,
			if(!t.update()) expiredtimers.add(&t);
		)
		loopv(expiredtimers) timers.remove(expiredtimers[i]->name);

		vec dir = vec(worldpos).sub(player1->o);
		float maxdist = max(player1->maxheight, player1->radius) * 1.1;
		if(dir.squaredlen() > maxdist * maxdist)
			dir.rescale(maxdist);
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

	#define getbasicreference(fun, var, name, fail) \
		if(!*var) \
		{ \
			ERRORF(#fun "; requires a reference to be specified"); \
			fail; return; \
		} \
		int name ## idx; bool has ## name ## idx; \
		reference *name = searchstack(var, name ## idx, has ## name ##idx); \
		if(DEBUG_VSCRIPT) \
		{ \
			if(!has ## name ## idx) DEBUGF("Reference %s specifies no index, non-reflist ops will assume :0", var); \
		} \
		if(name) { if(DEBUG_VSCRIPT) DEBUGF("reference %s found", var); }\
		else \
		{ \
			ERRORF(#fun "; invalid reference %s", var); \
			fail; return; \
		}

	#define getreference(fun, var, name, cond, fail) \
		getbasicreference(fun, var, name, fail) \
		if(!(cond)) \
		{ \
			ERRORF(#fun "; reference \"%s\" of incompatible type", var); \
			if(debugreference) name->dump(); \
			fail; return; \
		}

	#define domultiref(fun, ref, cond, body) \
		int ref ## count = has ## ref ## idx ? ref ## idx + 1 : ref->list.length(); \
		for(; ref ## idx < ref ## count;  ref ## idx++) \
		{ \
			if(!(cond)) \
			{ \
				ERRORF(#fun "; reference %s:%i is of an incompatible type", ref->name, ref ## idx); \
				continue; \
			} \
			body; \
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
		getreference(r_cansee, looker, ent, ent->getchar(entidx), intret(0))
		getreference(r_cansee, lookee, obj, obj->getent(objidx), intret(0))

		intret(ent->getchar(entidx)->cansee(obj->getent(objidx)));
	)

	ICOMMAND(r_preparemap, "ssi", (const char *m, const char *scr, int *f),
		if(*m)
		{
			mapinfo *tmp = accessmap(m);
			if(!tmp) return;

			mapscript *s = mapscripts.access(scr);
			if(s)
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
		int refidx; bool hasrefidx;
		reference *lookup = searchstack(ref, refidx, hasrefidx);

		intret((lookup && lookup->list.inrange(refidx)) ? lookup->list[refidx].type : reference::T_INVALID);
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

		int refidx; bool hasrefidx;
		reference *r = searchstack(ref, refidx, hasrefidx, true);
		if(!r || !r->canset()) return; //if this fails, then no stack

		int alidx; bool hasalidx;
		reference *a = *alias ? searchstack(alias, alidx, hasalidx) : NULL;

		if(hasrefidx)
		{
			if(!a)
			{
				if(DEBUG_VSCRIPT)
					DEBUGF("no valid alias, nulling %s:%i, if exists", ref, refidx);
				if(r->list.inrange(refidx))
					r->list[refidx] = reference::ref();
				return;
			}
			while(r->list.length() <= refidx)
				r->list.add(reference::ref());

			if(hasalidx)
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

			if(hasalidx)
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
		if(!a->canset() || !b->canset()) return;
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
		getbasicreference(r_matchref, f, a, intret(0));
		getbasicreference(r_matchref, s, b, intret(0));

		if(hasaidx && hasbidx)
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
		else if(hasaidx)
		{
			if(!a->list.inrange(aidx) && !b->list.length())
				intret(1);
			else if(a->list.inrange(aidx) && b->list.length() == 1)
				intret(a->list[aidx].ptr == b->list[0].ptr);
			else
				intret(0);
		}
		else if(hasbidx)
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

		getbasicreference(r_ref_sub, c, children, );
		if(!children->list.inrange(childrenidx)) return;

		if(ref == children)
		{
			if(!haschildrenidx) ref->list.setsize(0);
			else ref->list.remove(childrenidx);
		}
		else if(haschildrenidx)
		{
			loopv(ref->list)
			{
				if(ref->list[i].ptr == children->list[childrenidx].ptr)
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
		if(in != *i || *n != num) WARNINGF("r_ref_remove, arguments clamped to %s: %i %i --> %i %i", ref, *i, *n, in, num);
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
		getreference(r_local_get, ref, gen, gen->getinv(genidx) || gen->getent(genidx) || gen->getmap(genidx), result(""))

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

	ICOMMAND(r_local_exists, "ss", (const char *ref, const char *name),
		getreference(r_local_exists, ref, gen, gen->getinv(genidx) || gen->getent(genidx) || gen->getmap(genidx), intret(0))

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
			intret(var != NULL);
		}
		intret(0);
	)

	ICOMMAND(r_local_set, "sss", (const char *ref, const char *name, const char *val),
		getbasicreference(r_local_set, ref, gen, )

		domultiref(fun, gen, gen->getinv(genidx) || gen->getent(genidx) || gen->getmap(genidx),
			int *li = NULL;

			if(gen->getinv(genidx))
				li = &gen->getinv(genidx)->locals;
			else if(gen->getent(genidx))
				li = &gen->getent(genidx)->locals;
			else if(gen->getmap(genidx))
				li = &gen->getmap(genidx)->locals;

			if(!li)
			{
				ERRORF("Reference %s:%i cannot hold locals", gen->name, genidx);
				return;
			}

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
		int sendidx, recidx;
		bool hassendidx, hasrecidx;
		reference *sender = searchstack(send, sendidx, hassendidx);
		reference *receiver = searchstack(rec, recidx, hasrecidx);

		rpgent *source = sender ? sender->getent(sendidx) : NULL;

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
				map.getsignal(sig, true, source);
			)
			camera::getsignal(sig);
		}
		else
		{
			int count = hasrecidx ? recidx + 1 : receiver->list.length();
			for(; recidx < count; recidx++)
			{
				if(receiver->getmap(recidx))
				{
					if(DEBUG_VSCRIPT) DEBUGF("receiver:%i is map, sending signal", recidx);
					receiver->getmap(recidx)->getsignal(sig, prop, source);
				}
				else if(receiver->getent(recidx))
				{
					if(DEBUG_VSCRIPT) DEBUGF("receiver:%i is ent, sending signal", recidx);
					receiver->getent(recidx)->getsignal(sig, prop, source);
				}
				else if(receiver->getinv(recidx))
				{
					if(DEBUG_VSCRIPT) DEBUGF("receiver:%i is item, sending signal", recidx);
					receiver->getinv(recidx)->getsignal(sig, prop, source);
				}
			}
		}
	}
	COMMANDN(r_signal, sendsignal, "sssi");

	ICOMMAND(r_loop_ref, "sse", (const char *ref, const char *list, uint *body),
		reference *l = searchstack(list, false);
		if(!l) return;

		pushstack();
		reference *cur = registertemp(ref, NULL);
		loopv(l->list)
		{
			cur->setref(l->list[i], true);
			rpgexecute(body);
		}
		popstack();
	)

	ICOMMAND(r_loop_ref_slice, "ssiie", (const char *ref, const char *list, int *start, int *end, uint *body),
		reference *l = searchstack(list, false);
		if(!l) return;

		if(*start < 0) *start = max(0, l->list.length() + *start);
		if(*end <= 0) *end = max(0, l->list.length() + *end);
		if(*start > *end) swap(*start, *end);

		pushstack();
		reference *cur = registertemp(ref, NULL);
		for(; *start < *end && l->list.inrange(*start); (*start)++)
		{
			cur->setref(l->list[*start], true);
			rpgexecute(body);
		}
		popstack();
	)

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

	ICOMMAND(r_loop_ents, "sse", (const char *ref, const char *mapref, uint *body),
		if(!*ref) {ERRORF("r_loop_ents; requires map reference and a reference to alias ents to"); return;}
		getreference(r_loop_ents, mapref, map, map->getmap(mapidx), )

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
		getreference(r_setref_ents, mapref, map, map->getmap(mapidx), )

		reference *entstack = registerref(ref, NULL);
		if(!entstack->canset()) return;

		entstack->setnull();
		loopv(map->getmap(mapidx)->objs)
			entstack->pushref(map->getmap(mapidx)->objs[i]);
	)

	ICOMMAND(r_loop_aeffects, "ssse", (const char *ref, const char *mapref, const char *vic, uint *body),
		if(!*ref) {ERRORF("r_loop_aeffects; requires map reference and a reference to alias ents to"); return;}
		getreference(r_loop_aeffects, mapref, map, map->getmap(mapidx), )

		rpgent *victim = NULL;
		if(*vic)
		{
			if(DEBUG_VSCRIPT) DEBUGF("r_loop_aeffects, entity reference provided, may do bounds checking");
			int vidx; bool hasvidx;
			reference *v = searchstack(vic, vidx, hasvidx, false);
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

	ICOMMAND(r_loop_inv, "sse", (const char *ref, const char *entref, uint *body),
		if(!*ref) {ERRORF("r_loop_inv, requires ent reference and reference to alias items to"); return;}
		getreference(r_loop_inv, entref, ent, ent->getchar(entidx) || ent->getcontainer(entidx), )

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
		getreference(r_setref_invstack, srcref, src, src->getchar(srcidx) || src->getcontainer(srcidx), )

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
		getreference(r_setref_inv, srcref, src, src->getchar(srcidx) || src->getcontainer(srcidx), )

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
		getreference(r_setref_equip, entref, ent, ent->getchar(entidx), )

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

	ICOMMAND(r_loop_veffects, "sse", (const char *ref, const char *entref, uint *body),
		if(!*ref) {ERRORF("r_loop_veffects; requires ent reference and a reference name"); return;}
		getreference(r_loop_veffects, entref, ent, ent->getent(entidx), )

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
		getreference(r_get_attr, ref, obj, obj->getchar(objidx) || obj->getinv(objidx), intret(0))
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
		getreference(r_get_skill, ref, obj, obj->getchar(objidx) || obj->getinv(objidx) || obj->getequip(objidx), intret(0))
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
		getreference(r_get_weight, ref, ent, ent->getchar(entidx) || ent->getcontainer(entidx) || ent->getinv(entidx) || ent->getequip(entidx), floatret(0))

		if(ent->getent(entidx))
			floatret(ent->getent(entidx)->getweight());
		else if(ent->getequip(entidx))
			floatret(ent->getinv(entidx)->weight);
		else
			floatret(ent->getinv(entidx)->quantity * ent->getinv(entidx)->weight);
	)

	ICOMMAND(r_get_name, "si", (const char *ref, int *u),
		getreference(r_get_name, ref, obj, obj->getent(objidx) || obj->getinv(objidx) || obj->getequip(objidx), result(""))
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
		getreference(r_get_type, ref, ent, ent->getent(entidx), intret(0))
		intret(ent->getent(entidx)->type());
	)

	// Item Commands

	ICOMMAND(r_get_base, "s", (const char *ref),
		getreference(r_get_base, ref, item, item->getinv(itemidx), intret(-1))
		result(item->getinv(itemidx)->base);
	) //TODO save bases for everything and return here.

	ICOMMAND(r_get_use, "s", (const char *ref),
		getreference(r_get_use, ref, equip, equip->getequip(equipidx), intret(-1))
		intret(equip->getequip(equipidx)->use);
	)

	// Entity Commands

	ICOMMAND(r_get_state, "s", (const char *ref),
		getreference(r_get_state, ref, ent, ent->getchar(entidx), intret(CS_DEAD))

		intret(ent->getent(entidx)->state);
	)

	ICOMMAND(r_get_pos, "s", (const char *ref),
		getreference(r_get_pos, ref, ent, ent->getent(entidx), result("0 0 0"))
		static string s; formatstring(s, "%f %f %f", ent->getent(entidx)->o.x, ent->getent(entidx)->o.y, ent->getent(entidx)->o.z);
		result(s);
	)

	ICOMMAND(r_add_status, "ssfi", (const char *ref, const char *st, float *mul, int *flags),
		getbasicreference(r_add_status, ref, victim, )
		statusgroup *sg = statuses.access(st);
		if(!sg) return;
		inflict inf(sg, ATTACK_NONE, 1); //resistances aren't applied

		domultiref(r_add_status, victim, victim->getent(victimidx),
			victim->getent(victimidx)->seffects.add(new victimeffect(NULL, &inf, *flags, *mul));
		)
	)

	ICOMMAND(r_kill, "ss", (const char *ref, const char *killer), //I was framed!
		getbasicreference(r_kill, ref, victim, )
		int nastyidx; bool hasnastyidx;
		reference *nasty = *killer ? searchstack(killer, nastyidx, hasnastyidx) : NULL;

		domultiref(r_kill, victim, victim->getchar(victimidx),
			if(DEBUG_SCRIPT) DEBUGF("r_kill; killing %s:%i and shifting the blame to %s", victim->name, victimidx, killer);
			victim->getchar(victimidx)->die(nasty ? nasty->getent(nastyidx) : NULL);
		)
	)

	ICOMMAND(r_resurrect, "s", (const char *ref),
		getbasicreference(r_resurrect, ref, target, )

		domultiref(r_resurrect, target, target->getchar(targetidx),
			if(DEBUG_SCRIPT) DEBUGF("r_resurrect; trying to revive %s:%i", target->name, targetidx);
			target->getchar(targetidx)->revive(false);
		)
	)

	ICOMMAND(r_pickup, "ss", (const char *finder, const char *keepsake), //finders keepers :P
		getreference(r_pickup, finder, actor, actor->getchar(actoridx), )
		getbasicreference(r_pickup, keepsake, item, )

		domultiref(r_pickup, item, item->getitem(itemidx),
			if(DEBUG_SCRIPT) DEBUGF("r_pickup; reference \"%s\" trying to pickup \"%s\"", finder, keepsake);
			actor->getchar(actoridx)->pickup(item->getitem(itemidx));
			if(!item->getitem(itemidx)->quantity)
			{
				if(DEBUG_SCRIPT) DEBUGF("r_pickup; keepsake has no more items left, queueing for obit");
				obits.add(item->getent(itemidx));
			}
		)
	)

	ICOMMAND(r_additem, "ssie", (const char *ref, const char *base, int *q, uint *body),
		getbasicreference(r_additem, ref, ent, )

		domultiref(r_additem, ent, ent->getchar(entidx) || ent->getcontainer(entidx),
			if(DEBUG_SCRIPT) DEBUGF("added %i instances of item \"%s\" to reference %s:%i", *q, base, ent->name, entidx);
			item *it = ent->getent(entidx)->additem(base, max(0, *q));
			doitemscript(it, ent->getchar(entidx), body);
		)
	)

	ICOMMAND(r_get_amount, "ss", (const char *ref, const char *base),
		getreference(r_get_amount, ref, ent, ent->getchar(entidx) || ent->getcontainer(entidx) || ent->getinv(entidx), intret(0))

		if(ent->getinv(entidx))
			intret(ent->getinv(entidx)->quantity);
		else
			intret(ent->getent(entidx)->getitemcount(base));
	)

	ICOMMAND(r_drop, "ssi", (const char *ref, const char *itref, int *q),
		getreference(r_drop, ref, ent, ent->getchar(entidx) || ent->getcontainer(entidx), intret(0))
		getbasicreference(r_drop, itref, it, intret(0))

		int dropped = 0;
		domultiref(r_drop, it, it->getinv(itidx),
			dropped += ent->getent(entidx)->drop(it->getinv(itidx), max(0, *q), true);
			if(DEBUG_SCRIPT) DEBUGF("dropping %i of reference %s:%i from reference \"%s\" - total %i dropped", *q, it->name, itidx, ref, dropped);
		)
		intret(dropped);
	)

	ICOMMAND(r_remove, "ssi", (const char *ref, const char *itref, int *q),
		getreference(r_remove, ref, ent, ent->getchar(entidx) || ent->getcontainer(entidx), intret(0))
		getbasicreference(r_remove, itref, it, intret(0))

		int dropped = 0;
		domultiref(r_drop, it, it->getinv(itidx),
			dropped += ent->getent(entidx)->drop(it->getinv(itidx), max(0, *q), false);
			if(DEBUG_SCRIPT) DEBUGF("removing %i of reference %s:%i from reference \"%s\" - total %i removed", *q, it->name, itidx, ref, dropped);
		)
		intret(dropped);
	)

	ICOMMAND(r_canequip, "ssi", (const char *ref, const char *itref, int *u),
		getreference(r_canequip, ref, ent, ent->getchar(entidx), intret(0))
		getreference(r_canequip, itref, it, it->getinv(itidx), intret(0))

		if(it->getequip(itidx)) *u = it->getequip(itidx)->use;

		if(it->getinv(itidx)->uses.inrange(*u))
		{
			use_armour *ua = (use_armour *) it->getinv(itidx)->uses[*u];
			intret(ua->reqs.meet(ent->getchar(entidx)->base));
		}
		else intret(0);
	)

	ICOMMAND(r_equip, "ssi", (const char *ref, const char *itref, int *u),
		getreference(r_equip, ref, ent, ent->getchar(entidx), )
		getbasicreference(r_equip, itref, it, )

		domultiref(r_equip, it, it->getinv(itidx),
			ent->getent(entidx)->equip(it->getinv(itidx), *u);
		)
	)

	ICOMMAND(r_dequip, "stiN", (const char *ref, tagval *which, int *slot, int *numargs),
		getreference(r_dequip, ref, ent, ent->getchar(entidx), intret(0))
		if(*numargs > 2)
		{
			const char *base = which->getstr();
			if(which->type == VAL_NULL || !base[0]) base = NULL;
			intret(ent->getchar(entidx)->dequip(base, *slot));
		}
		else //2 arg version
		{
			if(DEBUG_SCRIPT) DEBUGF("attempting 2-arg dequip on reference %s for (equip?) reference %s", ref, which->getstr());
			getreference(r_dequip, which->getstr(), eq, eq->getequip(eqidx), intret(0))

			intret(ent->getchar(entidx)->dequip(eq->getequip(eqidx)->it->base, eq->getequip(eqidx)->use));
		}
	)

	ICOMMAND(r_use, "ssiN", (const char *ref, const char *itref, int *use, int *numargs),
		getreference(r_use, ref, ent, ent->getchar(entidx), )
		getreference(r_use, itref, it, it->getinv(itidx) || it->getequip(itidx), );

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
		getreference(r_transfer, fromref, from, from->getchar(fromidx) || from->getcontainer(fromidx), )
		getreference(r_transfer, toref, to, to->getchar(toidx) || to->getcontainer(toidx), )
		getbasicreference(r_transfer, itref, it, );

		*q = max(1, *q);

		//Just save us the effort and further complications.
		if(from == to)
		{
			WARNINGF("from and to reference targets are identical, ignoring r_transfer request");
			return;
		}

		domultiref(r_transfer, it, it->getinv(itidx),
			if(DEBUG_SCRIPT)
				 DEBUGF("attempting to move %i units of item %s:%i (%p) to entity %s (%p) from %s (%p)", *q, it->name, itidx, it->getinv(itidx), toref, to->getent(toidx), fromref, from->getent(fromidx));

			vector<item *> *stack = NULL;
			if(from->getchar(fromidx)) stack = from->getchar(fromidx)->inventory.access(it->getinv(itidx)->base);
			else stack = from->getcontainer(fromidx)->inventory.access(it->getinv(itidx)->base);

			if(stack->find(it->getinv(itidx)) == -1)
			{
				WARNINGF("item %p does not exist in the inventory of %p", it->getinv(itidx), from->getent(toidx));
				continue;
			}

			int orig = it->getinv(itidx)->quantity;
			it->getinv(itidx)->quantity = min(*q, it->getinv(itidx)->quantity);
			to->getent(toidx)->additem(it->getinv(itidx));
			it->getinv(itidx)->quantity = max(orig - *q, 0);

			if(DEBUG_SCRIPT)
				 DEBUGF("transferred up to %i instances of %i from item %p to %p", *q, orig, it->getinv(itidx), to->getent(toidx));
		)
	)

	ICOMMAND(r_trigger, "s", (const char *ref),
		getbasicreference(r_trigger, ref, obj, )

		domultiref(r_trigger, obj, obj->gettrigger(objidx),
			obj->gettrigger(objidx)->lasttrigger = lastmillis;
			obj->gettrigger(objidx)->flags ^= rpgtrigger::F_TRIGGERED;
			if(DEBUG_SCRIPT) DEBUGF("triggered reference \"%s\", triggered flag is now %i", ref, obj->gettrigger(0)->flags & rpgtrigger::F_TRIGGERED);
		)
	)

	ICOMMAND(r_destroy, "s", (const char *victim),
		getbasicreference(r_destroy, victim, obit, )

		domultiref(r_destroy, obit, obit->getent(obitidx) && obit->getent(obitidx) != trader->getent(0),
			if(DEBUG_SCRIPT) DEBUGF("queueing reference %s:%i (%p) for destruction", obit->name, obitidx, obit->getent(obitidx));

			obits.add(obit->getent(obitidx));
		)
	)

	// Entity - stats

	ICOMMAND(r_get_mana, "s", (const char *ref),
		getreference(r_get_mana, ref, ent, ent->getchar(entidx), intret(0))
		intret(ent->getchar(entidx)->mana);
	)

	ICOMMAND(r_get_maxmana, "s", (const char *ref),
		getreference(r_get_maxmana, ref, ent, ent->getchar(entidx), intret(0))
		intret(ent->getchar(entidx)->base.getmaxmp());
	)

	ICOMMAND(r_get_manaregen, "s", (const char *ref),
		getreference(r_get_manaregen, ref, ent, ent->getchar(entidx), intret(0))
		floatret(ent->getchar(entidx)->base.getmpregen())
	)

	ICOMMAND(r_get_health, "s", (const char *ref),
		getreference(r_get_health, ref, ent, ent->getchar(entidx), intret(0))
		intret(ent->getchar(entidx)->health);
	)

	ICOMMAND(r_get_maxhealth, "s", (const char *ref),
		getreference(r_get_maxhealth, ref, ent, ent->getchar(entidx), intret(0))
		intret(ent->getchar(entidx)->base.getmaxhp());
	)

	ICOMMAND(r_get_healthregen, "s", (const char *ref),
		getreference(r_get_healthregen, ref, ent, ent->getchar(entidx), intret(0))
		floatret(ent->getchar(entidx)->base.gethpregen())
	)

	ICOMMAND(r_get_resist, "si", (const char *ref, int *elem),
		if(*elem < 0 || *elem >= ATTACK_MAX) {ERRORF("r_get_resist; element %i out of range", *elem); intret(0); return;}
		getreference(r_get_resist, ref, ent, ent->getchar(entidx), intret(0))
		intret(ent->getchar(entidx)->base.getresistance(*elem));
	)

	ICOMMAND(r_get_thresh, "si", (const char *ref, int *elem),
		if(*elem < 0 || *elem >= ATTACK_MAX) {ERRORF("r_get_thres; element %i out of range", *elem); intret(0); return;}
		getreference(r_get_thresh, ref, ent, ent->getchar(entidx), intret(0))
		intret(ent->getchar(entidx)->base.getthreshold(*elem));
	)

	ICOMMAND(r_get_maxweight, "s", (const char *ref),
		getreference(r_get_maxweight, ref, ent, ent->getchar(entidx), intret(0))
		intret(ent->getchar(entidx)->base.getmaxcarry());
	)

	ICOMMAND(r_get_maxspeed, "s", (const char *ref),
		getreference(r_get_maxspeed, ref, ent, ent->getchar(entidx), intret(0))
		float spd; float jmp;
		ent->getchar(entidx)->base.setspeeds(spd, jmp);
		floatret(spd);
	)

	ICOMMAND(r_get_jumpvel, "s", (const char *ref),
		getreference(r_get_jumpvel, ref, ent, ent->getchar(entidx), intret(0))
		float spd; float jmp;
		ent->getchar(entidx)->base.setspeeds(spd, jmp);
		floatret(jmp);
	)

	ICOMMAND(r_get_lastaction, "s", (const char *ref),
		getreference(r_get_lastaction, ref, ent, ent->getchar(entidx) || ent->gettrigger(entidx), intret(lastmillis))

		if(ent->getchar(entidx)) intret(ent->getchar(entidx)->lastaction);
		else if(ent->gettrigger(entidx)) intret(ent->gettrigger(entidx)->lasttrigger);
	)

	ICOMMAND(r_get_charge, "s", (const char *ref),
		getreference(r_get_charge, ref, ent, ent->getchar(entidx), intret(0))
		intret(ent->getchar(entidx)->charge);
	)

	ICOMMAND(r_get_primary, "s", (const char *ref),
		getreference(r_get_primary, ref, ent, ent->getchar(entidx), intret(0))
		intret(ent->getchar(entidx)->primary);
	)

	ICOMMAND(r_get_secondary, "s", (const char *ref),
		getreference(r_get_secondary, ref, ent, ent->getchar(entidx), intret(0))
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
		getreference(r_chat, s, speaker, speaker->getent(speakeridx), )
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
		getreference(r_trade, str, cont, cont->getchar(contidx) || cont->getcontainer(contidx), )
		if(cont->getchar(contidx) && !cont->getchar(contidx)->merchant) return;
		if(cont->getcontainer(contidx) && !cont->getcontainer(contidx)->merchant) return;
		trader->setref(cont->getent(contidx), true);
	)

	ICOMMAND(r_loot, "s", (const char *str),
		getreference(r_loot, str, cont, cont->getchar(contidx) || cont->getcontainer(contidx), )
		looter->setref(cont->getent(contidx), true);
		rpgexecute("showloot");
	)

	// World Commands

	//ICOMMAND(r_resetmap, "s", (const char *ref), - resets contents status

	ICOMMAND(r_on_map, "ss", (const char *mapref, const char *entref),
		getreference(r_on_map, mapref, map, map->getmap(mapidx), intret(0))
		getbasicreference(r_on_map, entref, ent, intret(1))

		int present = 1;
		domultiref(r_on_map, ent, ent->getent(entidx),
			present &= map->getmap(mapidx)->objs.find(ent->getent(entidx)) >= 0;
		)
		intret(present);
	)

	#define SPAWNCOMMAND(type, con) \
		ICOMMAND(r_spawn_ ## type, "sisii", (const char *ref, int *attra, const char *attrb, int *attrc, int *attrd), \
			getbasicreference(r_spawn_ ## type, ref, map, ); \
			domultiref(r_spawn_ ## type, map, map->getmap(mapidx), \
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
			) \
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
		getreference(r_teleport, ref, ent, ent->getent(entidx), )

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
		getreference(r_get_status_owner, ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), )
		reference *owner = searchstack(set, true);
		if(eff->getveffect(effidx)) owner->setref(eff->getveffect(effidx)->owner);
		else owner->setref(eff->getaeffect(effidx)->owner);
	)

	ICOMMAND(r_get_status_num, "s", (const char *ref),
		getreference(r_get_status_num, ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), intret(0))
		if(eff->getveffect(effidx)) intret(eff->getveffect(effidx)->effects.length());
		else intret(eff->getaeffect(effidx)->effects.length());
	)

	ICOMMAND(r_get_status, "si", (const char *ref, int *idx),
		getreference(r_get_status, ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), result("-1 0 0 0 0"))

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
		getreference(r_get_status_duration, ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), intret(0))
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
		getreference(r_get_status_remain, ref, eff, eff->getveffect(effidx) || eff->getaeffect(effidx), intret(0))
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
			getreference(r_sound, r, ent, ent->getent(entidx), )
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
