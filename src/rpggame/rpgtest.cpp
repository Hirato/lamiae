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
			ERRORF("map data unavailable");
			return;
		}
		if(!effects.length())
		{
			ERRORF("No effects declared, not creating projectile");
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

		vector<effect *> efx;
		efx.add(NULL);
		enumerate(effects, effect, entry,
			efx.add(&entry)
		)

		p->projfx = efx[rnd(efx.length())];
		p->trailfx = efx[rnd(efx.length())];
		p->deathfx = efx[rnd(efx.length())];
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

		const char *pfx = p->projfx ? p->projfx->key : NULL,
			*tfx = p->trailfx ? p->trailfx->key : NULL,
			*dfx = p->deathfx ? p->deathfx->key : NULL;

		conoutf("fx: %s; %s; %s\tgravity: %4i\tspeed: %6.2f\tflags: %3i (%s)\ttime: %4i\tdist: %4i\telasticity: %1.2f", pfx, tfx, dfx, p->gravity, p->dir.magnitude(), p->pflags, pflags, p->time, p->dist, p->elasticity);

		curmap->projs.add(p);
	}
	ICOMMAND(testproj, "iii", (int *i, int *a, int *p), testproj(*i, *a, *p))

	void testeffect(const char *st, float charge, int chargeflags)
	{
		if(!curmap)
		{
			ERRORF("map data unavailable");
			return;
		}
		statusgroup *sg = statuses.access(st);
		if(!sg)
		{
			ERRORF("statusgroup[%s] has not been defined, aborting test", st);
			return;
		}

		conoutf("inflicting statusgroup[%s] on player, with charge %f and chargeflags %i", st, charge, chargeflags);

		inflict tmp = inflict(sg, ATTACK_FIRE, charge);
		player1->seffects.add(new victimeffect(NULL, &tmp, chargeflags, 1));
	}
	ICOMMAND(testeffect, "sfi", (const char *st, float *m, int *f), testeffect(st, *m, *f))

	void testareaeffect(float x, float y, float z, const char *st, float charge, int chargeflags)
	{
		if(!curmap)
		{
			ERRORF("map data unavailable");
			return;
		}
		if(!effects.length())
			WARNINGF("No particle effects have been declared");

		statusgroup *sg = statuses.access(st);
		if(!sg)
		{
			ERRORF("statusgroup[%s] has not been defined, aborting test", st);
			return;
		}

		vector<effect *> efx;
		efx.add(NULL);
		enumerate(effects, effect, entry,
			efx.add(&entry)
		)

		areaeffect *aeff = curmap->aeffects.add(new areaeffect());
		aeff->o = vec(x, y, z);
		aeff->group = sg;
		aeff->fx = efx[rnd(efx.length())];
		aeff->radius = rnd(161) + 32;
		if(chargeflags & CHARGE_RADIUS) aeff->radius *= charge;

		loopv(sg->effects)
		{
			status *st = aeff->effects.add(sg->effects[i]->dup());
			st->remain = st->duration;

			if(chargeflags & CHARGE_MAG) st->strength *= charge;
			if(chargeflags & CHARGE_DURATION) st->remain *= charge;
		}
	}
	ICOMMAND(testareaeffect, "fffsfi", (float *x, float *y, float *z, const char *st, float *m, int *f), testareaeffect(*x, *y, *z, st, *m, *f))
}

#endif
