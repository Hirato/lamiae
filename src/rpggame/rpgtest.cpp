#include "rpggame.h"

/*
This file is to remain fully separate from the rest of the game files.
Compilation and linking should succeed without this

Most of these commands can be used as cheats and should not be available by default
*/

#ifndef NO_DEBUG

using namespace game;

namespace test
{
	void testproj(int flags, int allowed, int always)
	{
		enum
		{
			FLAG_FAST = 1,
			FLAG_FUZZY = 2,
			FLAG_FLAGS = 4
		};

		if(!curmap)
		{
			conoutf("Error: map data currently not available");
			return;
		}
		if(!effects.length())
		{
			conoutf("No effects declared, not creating projectile");
			return;
		}

		projectile *p = new projectile();
		p->init(
			player1,
			NULL, NULL,
			(flags&FLAG_FUZZY) ? 20 : 0,
			1,
			0.01 + rnd(flags & FLAG_FAST ? 10000 : 1000) / 100.0f
		);

		p->projfx = rnd(effects.length() + 1) - 1;
		p->trailfx = rnd(effects.length() + 1) - 1;
		p->deathfx = rnd(effects.length() + 1) - 1;
		p->gravity = rnd(401) - 200;
		p->dist = 200 + rnd(9800);
		p->time = 200 + rnd(9800);
		p->elasticity = rnd(101) / 100.0f;
		p->radius = 32;

		if(flags & FLAG_FLAGS)
			p->pflags = rnd(128);

		p->pflags &= (allowed ? allowed : ~allowed);
		p->pflags |= always;

		string pflags;
		pflags[0] = '\0';
		const char *flagnames[] = {"P_TIME", "P_DIST", "P_RICOCHET", "P_VOLATILE", "P_PROXIMITY", "P_STATIONARY", "P_PERSIST"};

		loopi(sizeof(flagnames)/sizeof(flagnames[0]))
		{
			if(!(p->pflags & (1 << i)))
				continue;

			if(pflags[0])
				concatstring(pflags, "|");

			concatstring(pflags, flagnames[i]);
		}

		conoutf("fx: %4i %4i %4i\tgravity: %4i\tspeed: %6.2f\tflags: %3i (%s)\ttime: %4i\tdist: %4i\telasticity: %1.2f", p->projfx, p->trailfx, p->deathfx, p->gravity, p->dir.magnitude(), p->pflags, pflags, p->time, p->dist, p->elasticity);

		curmap->projs.add(p);
	}
	ICOMMAND(testproj, "iii", (int *i, int *a, int *p), testproj(*i, *a, *p))

	void testeffect(int ind, float charge, int chargeflags)
	{
		if(!curmap)
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr map data unavailable");
			return;
		}
		if(!statuses.inrange(ind))
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr statusgroup[%i] has not been defined, aborting test", ind);
			return;
		}

		conoutf("inflicting statusgroup[%i] on player, with charge %f and chargeflags %i", ind, charge, chargeflags);

		inflict tmp = inflict(ind, ATTACK_FIRE, charge);
		player1->seffects.add(new victimeffect(NULL, &tmp, chargeflags, 1));
	}
	ICOMMAND(testeffect, "ifi", (int *i, float *m, int *f), testeffect(*i, *m, *f))

	void testareaeffect(float x, float y, float z, int ind, float charge, int chargeflags)
	{
		if(!curmap)
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr map data unavailable");
			return;
		}
		if(!statuses.inrange(ind))
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr statusgroup[%i] has not been defined, aborting test", ind);
			return;
		}
		if(!effects.length())
		{
			conoutf("\fs\f3ERROR:\fr No particle effects have been declared");
			return;
		}

		areaeffect *aeff = curmap->aeffects.add(new areaeffect());
		aeff->o = vec(x, y, z);
		aeff->group = ind;
		aeff->fx = rnd(effects.length());
		aeff->radius = rnd(161) + 32;
		if(chargeflags & CHARGE_RADIUS) aeff->radius *= charge;

		loopv(game::statuses[ind]->effects)
		{
			status *st = aeff->effects.add(game::statuses[ind]->effects[i]->dup());
			st->remain = st->duration;

			if(chargeflags & CHARGE_MAG) st->strength *= charge;
			if(chargeflags & CHARGE_DURATION) st->remain *= charge;
		}
	}
	ICOMMAND(testareaeffect, "fffifi", (float *x, float *y, float *z, int *i, float *m, int *f), testareaeffect(*x, *y, *z, *i, *m, *f))
}

#endif
