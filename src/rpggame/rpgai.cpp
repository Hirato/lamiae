#include "rpggame.h"

///@TODO write some directives and implement them here

//patrol - moves around in a singular area, engages stuff if needed and returns here afterwards

//attack - attacks and hunts down the entity. will atempt to search and find the entity before going on with whatever they were doing
//NOTE 1: if target is invisible make them swipe the air randomly
//NOTE 2: call for help or flee if buddies are killed

//move - move to position entity with the specified tag, or to specific coordinits

//interact - interacts with stuff, ie pick up items

//flee - flees, preferably to a nearby safezone

//watch - watches a creature

//follow - follows a creature, like a companion

//guard - like follow, but helps them in fights

//stay - stay in the same spot

namespace ai
{

	bool isduplicate(rpgchar *ent, directive *action)
	{
		loopv(ent->directives)
		{
			if(ent->directives[i]->match(action))
				return true;
		}
		return false;
	}

	enum
	{
		D_ANIMATION= 0,
		D_PATROL,
		D_ATTACK,
		D_MOVE,
		D_INTERACT,
		D_FLEE,
		D_FOLLOW,
		D_GUARD,
		D_WANDER,
		D_WATCH,
	};

	struct animation : directive
	{
		int anim, duration;
		animation() : directive(0), anim(-1), duration(0) {}
		animation(int a, int d, int p) : directive(p), anim(a), duration(d) {}
		~animation() {}

		int type() const { return D_ANIMATION; }
		bool update(rpgchar *d)
		{
			if(d->aiflags & AI_ANIM) return true;
			if((duration -= curtime) <= 0) return false;

			d->aiflags |= AI_ANIM;
			d->forceanim = anim;

			return true;
		}
		bool match(directive *action)
		{
			if(action->type() == D_ANIMATION)
			{
				animation *o = (animation *) action;
				if(o->anim == anim)
				{
					duration += o->duration;
					priority = o->priority;
					return true;
				}
			}
			return false;
		}
	};

	struct patrol : directive
	{
		patrol(int p) : directive(p) {}
		~patrol() {}
	};

	struct attack : directive
	{
		rpgent *target;
		vec lastknown, lastvel;

		attack() : directive(0), target(NULL), lastknown(vec(0,0,0)), lastvel(vec(0, 0, 0)) {}
		attack(rpgchar *vic, int p) : directive(p), target(vic)
		{
			lastknown = vic->feetpos();
			lastvel = target->vel;
		}
		~attack() {}

		int type() const {return D_ATTACK;}
		bool update(rpgchar *d)
		{
			if(target->state == CS_DEAD) return false;

			d->aiflags |= AI_ALERT;
			if(d->aiflags & AI_ATTACK) return true;
			d->aiflags |= AI_ATTACK;
			d->target = target;

			if(d->cansee(target)) {lastknown = target->feetpos(); lastvel = target->vel;}
			d->lastknown = lastknown;

			if(!(d->aiflags & AI_MOVE))
			{
				d->aiflags |= AI_MOVE;
				if(lastvel.magnitude() > 5)
				{
					d->dest = lastvel;
					d->dest.mul(d->o.dist(lastknown) / lastvel.magnitude()).add(lastknown);
				}
				d->dest = lastknown;
			}

			return true;
		}
		bool match(directive *action)
		{
			if(action->type() == D_ATTACK)
			{
				attack *o = (attack *) action;
				if(target == o->target)
				{
					//use new last known location and velocity, and priority.
					*this = *o;
					return true;
				}
			}
			return false;
		}
	};

	struct move : directive
	{
		vec pos;

		move() : directive(0), pos(vec(0,0,0)) {}
		move(vec &o, int p) : directive(p), pos(o) {}
		~move() {}

		int type() const {return D_MOVE;}
		bool update(rpgchar *d)
		{
			if(pos.dist(d->feetpos()) <= 6) return false;
			if(d->aiflags & AI_MOVE) return true;

			d->aiflags |= AI_MOVE;
			d->dest = pos;
			return true;
		}
		bool match(directive *action)
		{
			if(action->type() == D_MOVE)
			{
				move *o = (move *) action;
				if(o->pos.dist(pos) <= 1) //close enough to merge
				{
					priority = o->priority;
					return true;
				}
			}
			return false;
		}
	};

	struct interact : directive
	{
		interact(int p) : directive(p) {}
		~interact() {}
	};

	struct flee : directive
	{
		flee(int p) : directive(p) {}
		~flee() {}
	};

	struct follow : directive
	{
		rpgent *target;
		vec lastknown;
		float distance;

		follow() : directive(0), target(NULL), lastknown(vec(0, 0, 0)), distance(0) {}
		follow(rpgent *vic, float dist, int p) : directive(p), target(vic), lastknown(vic->o), distance(dist) {}
		~follow() {}

		int type() const { return D_FOLLOW; }
		bool update(rpgchar *d)
		{
			float dist = distance;
			if(d->cansee(target))
			{
				lastknown = target->feetpos();
			}
			else
			{
				dist = 0;
			}

			if(!(d->aiflags & AI_MOVE))
			{
				d->aiflags |= AI_MOVE;
				if(lastknown.dist(d->o) > dist)
				{
					d->lastknown = lastknown;
					d->dest = vec(lastknown).add(vec(lastknown).sub(d->o).rescale(dist));
				}
			}

			if(!(d->aiflags & (AI_ALERT|AI_ATTACK)))
			{
				d->aiflags |= AI_ALERT;
				d->target = target;
			}

			return true;
		}

		bool match(directive *action)
		{
			follow *o = (follow *) action;
			if(o->type() == D_FOLLOW && o->target == target)
			{
				*this = *o;
				return true;
			}
			return false;
		}
	};

	struct guard : directive
	{
		guard(int p) : directive(p) {}
		~guard() {}
	};

	struct wander : directive
	{
		vec centre, dest;
		int radius, lastupdate;

		wander() : directive(0), centre(vec(0,0,0)), dest(vec(0, 0, 0)), radius(0), lastupdate(0) {}
		wander(vec &o, int r, int p) : directive(p), centre(o), dest(o), radius(r), lastupdate(0) {}
		~wander() {}

		int type() const {return D_WANDER;}
		bool update(rpgchar *d)
		{
			if(d->aiflags & AI_MOVE) return true;

			if(lastupdate + (radius * 50) < lastmillis)
			{
				lastupdate = lastmillis;
				dest = vec(rnd(360) * RAD, 0).mul(rnd(radius + 1)).add(centre);
			}

			d->aiflags |= AI_MOVE;
			d->dest = dest;
			return true;
		}
		bool match(directive *action)
		{
			if(action->type() == D_WANDER)
			{
				wander *o = (wander *) action;
				//close enough to merge?
				if(centre.dist(o->centre) <= 1)
				{
					*this = *o;
					return true;
				}
			}
			return false;
		}
	};

	struct watch : directive
	{
		rpgent *target;
		vec lastknown;

		watch(rpgent *vic, int p) : directive(p), target(vic), lastknown(vic->o) {}
		watch() : directive(0), target(NULL), lastknown(vec(0, 0, 0)) {}
		~watch() {}

		int type() const { return D_WATCH; }
		bool update(rpgchar *d)
		{
			if(d->aiflags & (AI_ALERT | AI_ATTACK)) return true;

			if(d->cansee(target)) lastknown = target->feetpos();
			d->aiflags |= AI_ALERT;
			d->target = target;
			d->lastknown = lastknown;

			return true;
		}
		bool match(directive *action)
		{
			watch *o = (watch *) action;
			if(o->type() == D_WATCH && o->target == target)
			{
				*this = *o;
				return true;
			}

			return false;
		}
	};

	#define getreference(var, name, cond, fail, fun) \
		if(!*var) \
		{ \
			ERRORF("" #fun "; requires a reference to be specified"); \
			fail; return; \
		} \
		int name ## idx; \
		rpgscript::parseref(var, name ## idx); \
		reference *name = rpgscript::searchstack(var); \
		if(!name || !(cond)) \
		{ \
			ERRORF("" #fun "; invalid reference \"%s\" or of incompatible type", var); \
			fail; return; \
		}

	#define getentity(n, t) \
		extentity *n = NULL; \
		loopv(entities::ents) \
		{ \
			if(entities::ents[i]->type == LOCATION && entities::ents[i]->attr[0] == (t)) \
			{ \
				n = entities::ents[i]; \
				break; \
			} \
		} \
		if(!n) \
		{ \
			ERRORF("location entity with tag %i does not exist", t); \
			return; \
		}

	ICOMMAND(r_action_clear, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(entidx), , r_action_clear)

		if(DEBUG_AI) DEBUGF("clearing AI directives for entity %p", ent->getchar(entidx));
		ent->getchar(entidx)->directives.deletecontents();
	)

	ICOMMAND(r_action_attack, "ssi", (const char *entref, const char *vicref, int *p),
		static attack action;
		getreference(entref, ent, ent->getchar(entidx), , r_action_attack)
		getreference(vicref, vic, vic->getchar(vicidx), , r_action_attack)

		action = attack(vic->getchar(vicidx), *p);
		if(isduplicate(ent->getchar(entidx), &action)) return;
		if(DEBUG_AI) DEBUGF("adding attack directive to entity %p; target %p priority %i", ent->getchar(entidx), vic->getchar(vicidx), *p);
		ent->getchar(entidx)->directives.add(new attack(action));
	)

	ICOMMAND(r_action_animation, "siii", (const char *entref, int *anim, int *dur, int *p),
		static animation action;
		getreference(entref, ent, ent->getchar(entidx), , r_action_animation)

		action = animation(*anim, *dur, *p);
		if(isduplicate(ent->getchar(entidx), &action)) return;
		if(DEBUG_AI) DEBUGF("adding animation directive to entity %p; anim %i, duration %i, priority %i", ent->getchar(entidx), *anim, *dur, *p);
		ent->getchar(entidx)->directives.add(new animation(action));
	)

	ICOMMAND(r_action_follow, "ssfi", (const char *entref, const char *vicref, float *dist, int *p),
		static follow action;
		getreference(entref, ent, ent->getchar(entidx), , r_action_follow)
		getreference(vicref, vic, vic->getent(vicidx), , r_action_follow)

		action = follow(vic->getent(vicidx), *dist, *p);
		if(isduplicate(ent->getchar(entidx), &action)) return;

		if(DEBUG_AI) DEBUGF("adding follow directive to entity %p; target %p dist %f priority %i", ent->getchar(entidx), vic->getent(vicidx), *dist, *p);
		ent->getchar(entidx)->directives.add(new follow(action));
	)

	ICOMMAND(r_action_move, "sii", (const char *entref, int *loc, int *p),
		static move action;
		getreference(entref, ent, ent->getchar(entidx), , r_action_move)
		getentity(l, *loc);

		action = move(l->o, *p);
		if(isduplicate(ent->getchar(entidx), &action)) return;
		if(DEBUG_AI) DEBUGF("adding move directive to entity %p; location %i priority %i", ent->getchar(entidx), *loc, *p);
		ent->getchar(entidx)->directives.add(new move(action));
	)

	ICOMMAND(r_action_wander, "siii", (const char *entref, int *loc, int *rad, int *p),
		static wander action;
		getreference(entref, ent, ent->getchar(entidx), , r_action_wander)
		getentity(l, *loc)

		action = wander(l->o, max(0, *rad), *p);
		if(isduplicate(ent->getchar(entidx), &action)) return;
		if(DEBUG_AI) DEBUGF("adding wander directive to entity %p; loc %i, rad %i, priority %i", ent->getchar(entidx), *loc, *rad, *p);
		ent->getchar(entidx)->directives.add(new wander(action));
	)

	ICOMMAND(r_action_watch, "ssi", (const char *entref, const char *vicref, int *p),
		static watch action;
		getreference(entref, ent, ent->getchar(entidx), , r_action_watch)
		getreference(vicref, vic, vic->getent(vicidx), , r_action_watch)

		action = watch(vic->getchar(vicidx), *p);
		if(isduplicate(ent->getchar(entidx), &action)) return;
		if(DEBUG_AI) DEBUGF("adding watch directive to entity %p; target %p priority %i", ent->getchar(entidx), vic->getchar(vicidx), *p);
		ent->getchar(entidx)->directives.add(new watch(action));
	)
}
