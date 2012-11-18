#include "rpggame.h"

/*
This file is configures the following items

* scripts
* particle effects
* status effect groups
* item bases
* item uses
* recipes
* ammotypes
* character bases
* factions
* obstacles
* containers
* platforms
* triggers
* mapscripts
* merchants

This file is also used to declare the following items (but disallows subsequent changes)

* Global Variables
* Item Categories

*/

namespace game
{
	#define CHECK(x, c, vec, select) \
		x *loading ## x = NULL; \
		static x *check ## x () \
		{ \
			if(! loading ## x) \
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr " #c " not defined or being loaded"); \
			return loading ## x; \
		} \
		ICOMMAND(r_select_ ## c, "se", (const char *ref, uint *contents), \
			x *old = loading ## x; \
			loading ## x = NULL; \
			\
			int objidx; \
			rpgscript::parseref(ref, objidx);\
			reference *obj = rpgscript::searchstack(ref, false); \
			if(obj) \
			{ \
				int idx = select; \
				if((vec).inrange(idx)) \
					loading ## x = (vec)[idx]; \
				if(loading ## x && DEBUG_VCONF) \
					conoutf("\fs\f2DEBUG:\fr successfully selected \"" #c "\" from reference %s", ref); \
			} \
			\
			if(!loading ##x) \
			{ \
				int idx = parseint(ref); \
				if(vec.inrange(idx)) \
				{ \
					if(DEBUG_VCONF) \
						conoutf("\fs\f2DEBUG:\fr successfully selected \"" #c "\" using index %i", idx); \
					loading ## x = vec[idx]; \
				} \
			} \
			if(loading ## x) \
				execute(contents); \
			else \
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr unable to select reference %s as type " #c, ref); \
			loading ## x = old; \
		) \
		ICOMMAND(r_num_ ## c, "", (), \
			if(DEBUG_VCONF) \
				conoutf("\fs\f2DEBUG:\fr r_num_" #c " requested, returning %i", vec.length()); \
			intret(vec.length()); \
		)

	CHECK(script, script, scripts, -1)
	CHECK(effect, effect, effects, -1)
	CHECK(statusgroup, status, statuses,
		obj->getveffect(objidx) ? obj->getveffect(objidx)->group :
		obj->getaeffect(objidx) ? obj->getaeffect(objidx)->group :
		-1
	)
	CHECK(faction, faction, factions,
		  obj->getchar(objidx) ? obj->getchar(objidx)->faction :
		  obj->getcontainer(objidx) ? obj->getcontainer(objidx)->faction :
		  -1
	)
	CHECK(recipe, recipe, recipes, -1)
	CHECK(ammotype, ammo, ammotypes, -1)
	CHECK(mapscript, mapscript, mapscripts, -1)
	CHECK(merchant, merchant, merchants,
		obj->getchar(objidx) ? obj->getchar(objidx)->merchant :
		obj->getcontainer(objidx) ? obj->getcontainer(objidx)->merchant :
		-1
	)

	#undef CHECK

	//These don't have a vector counter part

	#define CHECK(x, c, select) \
		x *loading ## x = NULL; \
		static x *check ##x () \
		{ \
			if(! loading ## x) \
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr " #c " not being loaded"); \
			return loading ## x; \
		} \
		ICOMMAND(r_select_ ## c, "se", (const char *ref, uint *body),  \
			x *old = loading ## x; \
			loading ## x = NULL; \
			 \
			int objidx; \
			rpgscript::parseref(ref, objidx); \
			reference *obj = rpgscript::searchstack(ref, false); \
			if(obj) \
			{ \
				loading ## x = select; \
				if(loading ## x && DEBUG_VCONF) \
					conoutf("\fs\f2DEBUG:\fr successfully selected \"" #c "\" from reference %s", ref); \
			} \
			if(loading ## x) \
				execute(body); \
			else \
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr unable to select reference %s as type " #c, ref); \
			loading ## x = old; \
		)

	CHECK(item, item,
		obj->getinv(objidx) ? obj->getinv(objidx) :
		obj->getequip(objidx) ? obj->getequip(objidx)->it :
		NULL
	)
	CHECK(rpgchar, char, obj->getchar(objidx))
	CHECK(rpgobstacle, obstacle, obj->getobstacle(objidx))
	CHECK(rpgcontainer, container, obj->getcontainer(objidx))
	CHECK(rpgplatform, platform, obj->getplatform(objidx))
	CHECK(rpgtrigger, trigger, obj->gettrigger(objidx))

	#undef CHECK

	use *loadinguse = NULL;
	static use *checkuse(int type)
	{
		if(!loadinguse)
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr use not defined or being loaded");
		else if(loadinguse->type < type)
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr trying to set an unavailable use property");
		else
			return loadinguse;
		return NULL;
	}
	ICOMMAND(r_select_item_use, "se", (const char *ref, uint *contents),
		use *old = loadinguse;
		loadinguse = NULL;
		int objidx;
		rpgscript::parseref(ref, objidx);
		reference *obj = rpgscript::searchstack(ref, false);

		if(obj && obj->getequip(objidx))
		{
			if(DEBUG_VCONF)
				conoutf("\fs\f2DEBUG:\fr successfully selected \"use\" from reference %s", ref);
			loadinguse = obj->getequip(objidx)->it->uses[obj->getequip(objidx)->use];
		}
		else if(loadingitem)
		{
			int idx = parseint(ref);
			if(loadingitem->uses.inrange(idx))
			{
				if(DEBUG_VCONF)
					conoutf("\fs\f2DEBUG:\fr successfully selected \"use\" using index %i", idx);
				loadinguse = loadingitem->uses[idx];
			}
		}

		if(loadinguse) execute(contents);
		else conoutf(CON_ERROR, "\fs\f3ERROR:\fr unable to select reference %s as type use", ref);
		loadinguse = old;
	)
	ICOMMAND(r_num_item_use, "", (),
		if(loadingitem)
			intret(loadingitem->uses.length());
		else intret(0);
	)

	/// examples
	/// NOTE: the following variable names are reserved: i, f, s, x, y, z
	/// #define START(n, f, a, b)  ICOMMAND(r_script ## n, f, a, b)
	/// #define INIT script *e = checkscript()       parent variable must always be named e, e->var is modified (see below for var)
	/// #define DEBUG "scripts[%i]"
	/// #define DEBUG_IND scripts.length() - 1

	#define INTSET(name, var, min, max) \
		START(name, "i", (int *i), \
			INIT \
			if(!e) \
			{ \
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr " #name " requires you to have a valid object selected first"); \
				return; \
			} \
			e->var = clamp((int) max, (int) min, *i); \
			if(e->var != *i) \
				conoutf("\fs\f6WARNING:\fr value provided for " DEBUG "->" #var " exceeded limits", DEBUG_IND); \
			if(DEBUG_CONF || e->var != *i) \
				conoutf(DEBUG "->" #var " = %i (%.8X)", DEBUG_IND, e->var, e->var); \
		)

	#define INTGET(name, var) \
		START(name ## _get, "", (), \
			INIT \
			if(!e) \
			{ \
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr " #name " requires you to have a valid object selected first"); \
				return; \
			} \
			if(DEBUG_VCONF) \
				conoutf("\fs\f2DEBUG:\fr request for " #name " on %p, returning %i", e, e->var); \
			intret(e->var); \
		)

	#define INTN(name, var, min, max) \
		INTSET(name, var, min, max) \
		INTGET(name, var)

	#define INT(var, min, max) INTN(var, var, min, max)

	#define FLOATSET(name, var, min, max) \
		START(name, "f", (float *f), \
			INIT \
			if(!e) \
			{ \
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr " #name " requires you to have a valid object selected first"); \
				return; \
			} \
			e->var = clamp((float) max, (float) min, *f); \
			if(e->var != *f) \
				conoutf("\fs\f6WARNING:\fr value provided for " DEBUG "->" #var " exceeded limits", DEBUG_IND); \
			if(DEBUG_CONF || e->var != *f) \
				conoutf(DEBUG "->" #var " = %g", DEBUG_IND, e->var); \
		)

	#define FLOATGET(name, var) \
		START(name ## _get, "", (), \
			INIT \
			if(!e) \
			{ \
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr " #name " requires you to have a valid object selected first"); \
				return; \
			} \
			if(DEBUG_VCONF) \
				conoutf("\fs\f2DEBUG:\fr request for " #name " on %p, returning %f", e, e->var); \
			floatret(e->var); \
		)

	#define FLOATN(name, var, min, max) \
		FLOATSET(name, var, min, max) \
		FLOATGET(name, var)

	#define FLOAT(var, min, max) FLOATN(var, var, min, max)

	#define STRINGN(name, var, body) \
		START(name, "C", (const char *s), \
			INIT \
			if(!e) \
			{ \
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr " #name " requires you to have a valid object selected first"); \
				return; \
			} \
			if(e->var) delete[] e->var; \
			e->var = newstring(s); \
			body; \
			if(DEBUG_CONF) \
				conoutf(CON_DEBUG, DEBUG "->" #var " = %s", DEBUG_IND, e->var); \
		) \
		START(name ## _get, "", (), \
			INIT \
			if(!e) return; \
			if(DEBUG_VCONF) \
				conoutf("\fs\f2DEBUG:\fr request for " #name " on %p, returning %s", e, e->var); \
			result(e->var ? e->var : ""); \
		)
	#define STRING(var) STRINGN(var, var, )
	#define MODEL(var) STRINGN(var, var, preloadmodel(e->var))

	#define VECN(name, var, l1, l2, l3, h1, h2, h3) \
		START(name, "fff", (float *x, float *y, float *z), \
			INIT \
			if(!e) return; \
			e->var.x = clamp((float)h1, (float)l1, *x); \
			e->var.y = clamp((float)h2, (float)l2, *y); \
			e->var.z = clamp((float)h3, (float)l3, *z); \
			if(e->var.x != *x || e->var.y != *y || e->var.z != *z) \
				conoutf("\fs\f6WARNING:\fr value provided for " DEBUG "->" #var " exceeded limits", DEBUG_IND); \
			if(DEBUG_CONF || e->var.x != *x || e->var.y != *y || e->var.z != *z) \
				conoutf(DEBUG "->" #var " = (%g, %g, %g)", DEBUG_IND, e->var.x, e->var.y, e->var.z); \
		) \
		START(name ## _get, "", (), \
			INIT \
			if(!e) return; \
			defformatstring(res)("%g %g %g", e->var.x, e->var.y, e->var.z); \
			if(DEBUG_VCONF) \
				conoutf("\fs\f2DEBUG:\fr request for " #name " on %p, returning %s", e, res); \
			result(res); \
		)
	#define VEC(var, l1, l2, l3, h1, h2, h3) VECN(var, var, l1, l2, l3, h1, h2, h3)

	#define BOOLN(name, var) \
		START(name, "i", (int *i), \
			INIT \
			if(!e) return; \
			e->var = *i != 0; \
			if(DEBUG_CONF) \
				conoutf(CON_DEBUG, DEBUG "->" #var " = %i", DEBUG_IND, e->var); \
		)\
		START(name ## _get, "", (), \
			INIT \
			if(!e) return; \
			if(DEBUG_VCONF) \
				conoutf("\fs\f2DEBUG:\fr request for " #name " on %p, returning %i", e, e->var); \
			intret(e->var); \
		)
	#define BOOL(var) BOOLN(var, var)

	#define STATREQ(var) \
		INTN(var ## _strength, var.attrs[STAT_STRENGTH], 0, 100) \
		INTN(var ## _endurance, var.attrs[STAT_ENDURANCE], 0, 100) \
		INTN(var ## _agility, var.attrs[STAT_AGILITY], 0, 100) \
		INTN(var ## _charisma, var.attrs[STAT_CHARISMA], 0, 100) \
		INTN(var ## _wisdom, var.attrs[STAT_WISDOM], 0, 100) \
		INTN(var ## _intelligence, var.attrs[STAT_INTELLIGENCE], 0, 100) \
		INTN(var ## _luck, var.attrs[STAT_LUCK], 0, 100) \
		\
		INTN(var ## _armour, var.skills[SKILL_ARMOUR], 0, 100) \
		INTN(var ## _diplomacy, var.skills[SKILL_DIPLOMACY], 0, 100) \
		INTN(var ## _magic, var.skills[SKILL_MAGIC], 0, 100) \
		INTN(var ## _marksman, var.skills[SKILL_MARKSMAN], 0, 100) \
		INTN(var ## _melee, var.skills[SKILL_MELEE], 0, 100) \
		INTN(var ## _stealth, var.skills[SKILL_STEALTH], 0, 100) \
		INTN(var ## _craft, var.skills[SKILL_CRAFT], 0, 100)

	#define STATS(var) \
		INTN(var ## _level, var.level, 1, 1000) \
		INTN(var ## _experience, var.experience, 0, 0x7FFFFFFF) \
		INTN(var ## _statpoints, var.statpoints, 0, 500) \
		INTN(var ## _skillpoints, var.skillpoints, 0, 500) \
		INTN(var ## _strength, var.baseattrs[STAT_STRENGTH], 1, 100) \
		INTN(var ## _endurance, var.baseattrs[STAT_ENDURANCE], 1, 100) \
		INTN(var ## _agility, var.baseattrs[STAT_AGILITY], 1, 100) \
		INTN(var ## _charisma, var.baseattrs[STAT_CHARISMA], 1, 100) \
		INTN(var ## _wisdom, var.baseattrs[STAT_WISDOM], 1, 100) \
		INTN(var ## _intelligence, var.baseattrs[STAT_INTELLIGENCE], 1, 100) \
		INTN(var ## _luck, var.baseattrs[STAT_LUCK], 1, 100) \
		\
		INTGET(var ## _delta_strength, var.deltaattrs[STAT_STRENGTH]) \
		INTGET(var ## _delta_endurance, var.deltaattrs[STAT_ENDURANCE]) \
		INTGET(var ## _delta_agility, var.deltaattrs[STAT_AGILITY]) \
		INTGET(var ## _delta_charisma, var.deltaattrs[STAT_CHARISMA]) \
		INTGET(var ## _delta_wisdom, var.deltaattrs[STAT_WISDOM]) \
		INTGET(var ## _delta_intelligence, var.deltaattrs[STAT_INTELLIGENCE]) \
		INTGET(var ## _delta_luck, var.deltaattrs[STAT_LUCK]) \
		\
		INTN(var ## _armour, var.baseskills[SKILL_ARMOUR], 0, 100) \
		INTN(var ## _diplomacy, var.baseskills[SKILL_DIPLOMACY], 0, 100) \
		INTN(var ## _magic, var.baseskills[SKILL_MAGIC], 0, 100) \
		INTN(var ## _marksman, var.baseskills[SKILL_MARKSMAN], 0, 100) \
		INTN(var ## _melee, var.baseskills[SKILL_MELEE], 0, 100) \
		INTN(var ## _stealth, var.baseskills[SKILL_STEALTH], 0, 100) \
		INTN(var ## _craft, var.baseskills[SKILL_CRAFT], 0, 100) \
		\
		INTGET(var ## _delta_armour, var.deltaskills[SKILL_ARMOUR]) \
		INTGET(var ## _delta_diplomacy, var.deltaskills[SKILL_DIPLOMACY]) \
		INTGET(var ## _delta_magic, var.deltaskills[SKILL_MAGIC]) \
		INTGET(var ## _delta_marksman, var.deltaskills[SKILL_MARKSMAN]) \
		INTGET(var ## _delta_melee, var.deltaskills[SKILL_MELEE]) \
		INTGET(var ## _delta_stealth, var.deltaskills[SKILL_STEALTH]) \
		INTGET(var ## _delta_craft, var.deltaskills[SKILL_CRAFT]) \
		\
		INTN(var ## _fire_thresh, var.bonusthresh[ATTACK_FIRE], -500, 500) \
		INTN(var ## _water_thresh, var.bonusthresh[ATTACK_WATER], -500, 500) \
		INTN(var ## _air_thresh, var.bonusthresh[ATTACK_AIR], -500, 500) \
		INTN(var ## _earth_thresh, var.bonusthresh[ATTACK_EARTH], -500, 500) \
		INTN(var ## _arcane_thresh, var.bonusthresh[ATTACK_ARCANE], -500, 500) \
		INTN(var ## _mind_thresh, var.bonusthresh[ATTACK_MIND], -500, 500) \
		INTN(var ## _holy_thresh, var.bonusthresh[ATTACK_HOLY], -500, 500) \
		INTN(var ## _darkness_thresh, var.bonusthresh[ATTACK_DARKNESS], -500, 500) \
		INTN(var ## _slash_thresh, var.bonusthresh[ATTACK_SLASH], -500, 500) \
		INTN(var ## _blunt_thresh, var.bonusthresh[ATTACK_BLUNT], -500, 500) \
		INTN(var ## _pierce_thresh, var.bonusthresh[ATTACK_PIERCE], -500, 500) \
		\
		INTGET(var ## _delta_fire_thresh, var.deltathresh[ATTACK_FIRE]) \
		INTGET(var ## _delta_water_thresh, var.deltathresh[ATTACK_WATER]) \
		INTGET(var ## _delta_air_thresh, var.deltathresh[ATTACK_AIR]) \
		INTGET(var ## _delta_earth_thresh, var.deltathresh[ATTACK_EARTH]) \
		INTGET(var ## _delta_arcane_thresh, var.deltathresh[ATTACK_ARCANE]) \
		INTGET(var ## _delta_mind_thresh, var.deltathresh[ATTACK_MIND]) \
		INTGET(var ## _delta_holy_thresh, var.deltathresh[ATTACK_HOLY]) \
		INTGET(var ## _delta_darkness_thresh, var.deltathresh[ATTACK_DARKNESS]) \
		INTGET(var ## _delta_slash_thresh, var.deltathresh[ATTACK_SLASH]) \
		INTGET(var ## _delta_blunt_thresh, var.deltathresh[ATTACK_BLUNT]) \
		INTGET(var ## _delta_pierce_thresh, var.deltathresh[ATTACK_PIERCE]) \
		\
		INTN(var ## _fire_resist, var.bonusresist[ATTACK_FIRE], -200, 100) \
		INTN(var ## _water_resist, var.bonusresist[ATTACK_WATER], -200, 100) \
		INTN(var ## _air_resist, var.bonusresist[ATTACK_AIR], -200, 100) \
		INTN(var ## _earth_resist, var.bonusresist[ATTACK_EARTH], -200, 100) \
		INTN(var ## _arcane_resist, var.bonusresist[ATTACK_ARCANE], -200, 100) \
		INTN(var ## _mind_resist, var.bonusresist[ATTACK_MIND], -200, 100) \
		INTN(var ## _holy_resist, var.bonusresist[ATTACK_HOLY], -200, 100) \
		INTN(var ## _darkness_resist, var.bonusresist[ATTACK_DARKNESS], -200, 100) \
		INTN(var ## _slash_resist, var.bonusresist[ATTACK_SLASH], -200, 100) \
		INTN(var ## _blunt_resist, var.bonusresist[ATTACK_BLUNT], -200, 100) \
		INTN(var ## _pierce_resist, var.bonusresist[ATTACK_PIERCE], -200, 100) \
		\
		INTGET(var ## _delta_fire_resist, var.deltaresist[ATTACK_FIRE]) \
		INTGET(var ## _delta_water_resist, var.deltaresist[ATTACK_WATER]) \
		INTGET(var ## _delta_air_resist, var.deltaresist[ATTACK_AIR]) \
		INTGET(var ## _delta_earth_resist, var.deltaresist[ATTACK_EARTH]) \
		INTGET(var ## _delta_arcane_resist, var.deltaresist[ATTACK_ARCANE]) \
		INTGET(var ## _delta_mind_resist, var.deltaresist[ATTACK_MIND]) \
		INTGET(var ## _delta_holy_resist, var.deltaresist[ATTACK_HOLY]) \
		INTGET(var ## _delta_darkness_resist, var.deltaresist[ATTACK_DARKNESS]) \
		INTGET(var ## _delta_slash_resist, var.deltaresist[ATTACK_SLASH]) \
		INTGET(var ## _delta_blunt_resist, var.deltaresist[ATTACK_BLUNT]) \
		INTGET(var ## _delta_pierce_resist, var.deltaresist[ATTACK_PIERCE]) \
		\
		INTN(var ## _maxspeed, var.bonusmovespeed, 0, 100) \
		INTN(var ## _jumpvel, var.bonusjumpvel, 0, 100) \
		INTN(var ## _maxhealth, var.bonushealth, 0, 100000) \
		INTN(var ## _maxmana, var.bonusmana, 0, 100000) \
		INTN(var ## _crit, var.bonuscrit, -100, 100) \
		FLOATN(var ## _healthregen, var.bonushregen, 0, 1000) \
		FLOATN(var ## _manaregen, var.bonusmregen, 0, 1000) \
		\
		INTGET(var ## _delta_maxspeed, var.deltamovespeed) \
		INTGET(var ## _delta_jumpvel, var.deltajumpvel) \
		INTGET(var ## _delta_maxhealth, var.deltahealth) \
		INTGET(var ## _delta_maxmana, var.deltamana) \
		INTGET(var ## _delta_crit, var.deltacrit) \
		FLOATGET(var ## _delta_healthregen, var.deltahregen) \
		FLOATGET(var ## _delta_manaregen, var.deltamregen) \

	ICOMMAND(r_script_node, "see", (const char *n, uint *txt, uint *scr),
		script *s = checkscript();
		if(!s) return;

		dialogue *node = s->chat.access(n);
		if(!node)
		{
			static dialogue dummy;
			const char *name = newstring(n);

			node = &s->chat.access(name, dummy);
			node->node = name;
		}

		freecode(node->text);
		node->text = txt;
		keepcode(txt);

		freecode(node->script);
		node->script = scr;
		keepcode(scr);
	)

	signal *getsignal(const char *sig, hashtable<const char *, signal> &table)
	{
		signal *listen = table.access(sig);
		if(listen) return listen;

		const char *name = newstring(sig);
		listen = &table.access(name, signal());
		listen->name = name;
		return listen;
	}

	ICOMMAND(r_script_signal, "se", (const char *sig, uint *code),
		script *scr = checkscript();
		if(!scr) return;

		signal *listen = getsignal(sig, scr->listeners);
		listen->setcode(code);
	)

	ICOMMAND(r_script_signal_append, "se", (const char *sig, uint *code),
		script *scr = checkscript();
		if(!scr) return;

		signal *listen = getsignal(sig, scr->listeners);
		listen->appendcode(code);
	)

	ICOMMAND(r_mapscript_signal, "se", (const char *sig, uint *code),
		mapscript *scr = checkmapscript();
		if(!scr) return;

		signal *listen = getsignal(sig, scr->listeners);
		listen->setcode(code);
	)

	ICOMMAND(r_mapscript_signal_append, "se", (const char *sig, uint *code),
		mapscript *scr = checkmapscript();
		if(!scr) return;

		signal *listen = getsignal(sig, scr->listeners);
		listen->appendcode(code);
	)

	#define START(n, f, a, b) ICOMMAND(r_effect_ ##n, f, a, b)
	#define INIT effect *e = checkeffect();
	#define DEBUG "effect[%i]"
	#define DEBUG_IND effects.find(e)

	INT(flags, 0, FX_MAX)
	INT(decal, -1, DECAL_MAX - 1)

	MODEL(mdl)
	VEC(spin, -100, -100, -100, 100, 100, 100)
	INT(particle, 0, PART_LENS_FLARE - 1)
	INT(colour, 0, 0xFFFFFF)
	INT(fade, 1, 120000)
	INT(gravity, -10000, 100000)
	FLOAT(size, 0.01f, 100.0f)

	INT(lightflags, 0, 7)
	INT(lightfade, 0, 120000)
	INT(lightradius, 16, 2048)
	INT(lightinitradius, 16, 2048)
	VEC(lightcol, -1, -1, -1, 1, 1, 1)
	VEC(lightinitcol, -1, -1, -1, 1, 1, 1)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_status_ ##n, f, a, b)
	#define INIT statusgroup *e = checkstatusgroup();
	#define DEBUG "statusgroup[%i]"
	#define DEBUG_IND statuses.find(e)

	ICOMMAND(r_status_addgeneric, "iiif", (int *t, int *s, int *d, float *v),
		if(STATUS_INVALID_GENERIC(*t))
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr trying to create a generic status effect of non-generic or invalid type, '%i'", *t);
			return;
		}
		statusgroup *e = checkstatusgroup();
		if(!e) return;

		status_generic *g = new status_generic();
		e->effects.add(g);

		g->type = *t;
		g->duration = *d;
		g->strength = *s;
		g->variance = clamp(*v, 0.f, 1.0f);

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr adding generic effect (%i %i %i), to statusgroup %i", *t, *d, *s, DEBUG_IND);
	)

	ICOMMAND(r_status_addpolymorph, "siif", (const char *m, int *st, int *d, float *v),
		statusgroup *e = checkstatusgroup();
		if(!e) return;

		status_polymorph *p = new status_polymorph();
		e->effects.add(p);
		p->type = STATUS_POLYMORPH;
		p->strength = *st;
		p->duration = *d;
		p->mdl = newstring(m);
		preloadmodel(p->mdl);
		p->variance = clamp(*v, 0.f, 1.0f);

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr adding polymorph effect (%s), to statusgroup %i", m, DEBUG_IND);
	)

	ICOMMAND(r_status_addlight, "fffiif", (float *r, float *g, float *b, int *str, int *d, float *v),
		statusgroup *e = checkstatusgroup();
		if(!e) return;

		status_light *l = new status_light();
		e->effects.add(l);
		l->type = STATUS_LIGHT;

		l->colour.x = clamp((float) 1, (float) -1, *r);
		l->colour.y = clamp((float) 1, (float) -1, *g);
		l->colour.z = clamp((float) 1, (float) -1, *b);
		l->strength = clamp((int) 512, (int) 8, *str);
		l->duration = *d;
		l->variance = clamp(*v, 0.f, 1.0f);

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr adding light effect (%f %f %f --> %i), to statusgroup %i", l->colour.x, l->colour.y, l->colour.z, l->strength, DEBUG_IND);
	)

	ICOMMAND(r_status_addsignal, "siif", (char *s, int *str, int *d, float *v),
		INIT if(!e) return;

		status_signal *sig = new status_signal();
		e->effects.add(sig);
		sig->type = STATUS_SIGNAL;
		sig->strength = *str;
		sig->variance = clamp(*v, 0.f, 1.0f);
		sig->signal = newstring(s);
		sig->duration = *d;
	)

	ICOMMAND(r_status_addscript, "siif", (char *s, int *str, int *d, float *v),
		INIT if(!e) return;

		status_script *scr = new status_script();
		e->effects.add(scr);

		scr->type = STATUS_SCRIPT;
		scr->script = newstring(s);
		scr->strength = *str;
		scr->duration = *d;
		scr->variance = clamp(*v, 0.f, 1.0f);
	)

	ICOMMAND(r_status_get_effect, "i", (int *idx),
		statusgroup *e = checkstatusgroup();
		if(!e && e->effects.inrange(*idx)) return;
		status *s = e->effects[*idx];
		string str;
		switch(s->type)
		{
			case STATUS_LIGHT:
			{
				status_light  *ls = (status_light *) s;
				formatstring(str)("%i %i %i %f %f %f %f", STATUS_LIGHT, s->strength, s->duration, s->variance, ls->colour.x, ls->colour.y, ls->colour.z);
				break;
			}
			case STATUS_POLYMORPH:
			{
				status_polymorph *ps = (status_polymorph *) s;
				formatstring(str)("%i %i %i %f \"%s\"", STATUS_POLYMORPH, s->strength, s->duration, s->variance, ps->mdl);
				break;
			}
			case STATUS_SIGNAL:
			{
				status_signal *ss = (status_signal *) s;
				formatstring(str)("%i %i %i %f \"%s\"", s->type, s->strength, s->duration, s->variance, ss->signal);
				break;
			}
			case STATUS_SCRIPT: //return script?
			default:
				formatstring(str)("%i %i %i %f", s->type, s->strength, s->duration, s->variance);
				break;
		}
		result(str);
	)

	ICOMMAND(r_status_num_effect, "", (),
		INIT
		if(!e) return;
		intret(e->effects.length());
	)

	BOOL(friendly)
	STRING(icon)
	STRING(name)
	STRING(description)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_item_ ##n, f, a, b)
	#define INIT item *e = checkitem();
	#define DEBUG "item[%p]"
	#define DEBUG_IND loadingitem

	ICOMMAND(r_item_use_new_consumable, "", (),
		item *e = checkitem();
		if(!e) return;

		loadinguse = (e->uses.add(new use(e->script)));
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr created new consumable use (%i) for " DEBUG, e->uses.length() - 1, DEBUG_IND);
	)

	ICOMMAND(r_item_use_new_armour, "", (),
		item *e = checkitem();
		if(!e) return;

		loadinguse = (e->uses.add(new use_armour(e->script)));
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr created new armour use (%i) for " DEBUG, e->uses.length() - 1, DEBUG_IND);
	)

	ICOMMAND(r_item_use_new_weapon, "", (),
		item *e = checkitem();
		if(!e) return;

		loadinguse = (e->uses.add(new use_weapon(e->script)));
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr created new weapon use (%i) for " DEBUG, e->uses.length() - 1, DEBUG_IND);
	)


	STRING(name)
	STRING(icon)
	STRING(description)
	MODEL(mdl)

	INT(quantity, 0, 0xFFFFFF)
	INT(script, 0, scripts.length() - 1)
	INT(category, 0, categories.length() - 1)
	INT(flags, 0, item::F_MAX)
	INT(value, 0, 0xFFFFFF)
	INT(maxdurability, 0, 0xFFFFFF)
	FLOAT(weight, 0, 0xFFFF)
	FLOAT(durability, 0, 0xFFFFFF)
	FLOAT(recovery, 0, 1)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_item_use_ ##n, f, a, b)
	#define INIT CAST *e = (CAST *) checkuse(TYPE);
	#define DEBUG "item_base[%p]->uses[%i]"
	#define DEBUG_IND loadingitem, loadingitem->uses.find(loadinguse)

	#define CAST use
	#define TYPE USE_CONSUME

	START(type_get, "", (),
		INIT
		intret(e ? e->type : -1);
	)

	STRING(name)
	STRING(description)
	STRING(icon)
	INT(script, 0, scripts.length() - 1)
	INT(cooldown, 0, 0xFFFF)
	INT(chargeflags, 0, CHARGE_MAX)

	START(new_status, "iif", (int *st, int *el, float *m),
		INIT if(!e) return;
		e->effects.add(new inflict(*st, *el, *m));
	)

	#undef CAST
	#undef TYPE
	#define CAST use_armour
	#define TYPE USE_ARMOUR

	MODEL(vwepmdl)
	MODEL(hudmdl)
	INT(idlefx, -1, effects.length() - 1)
	STATREQ(reqs)
	INT(slots, 0, SLOT_MAX)
	INT(skill, -1, SKILL_MAX - 1)

	#undef CAST
	#undef TYPE
	#define CAST use_weapon
	#define TYPE USE_WEAPON

	INT(range, 0, 1024)
	INT(angle, 0, 360)
	INT(lifetime, 0, 0xFFFF)
	INT(gravity, -1000, 1000)
	INT(projeffect, -1, effects.length() - 1)
	INT(traileffect, -1, effects.length() - 1)
	INT(deatheffect, -1, effects.length() - 1)
	INT(cost, 0, 0xFFFF)
	INT(pflags, 0, P_MAX)
	INT(ammo, -3, ammotypes.length() - 1)
	INT(target, 0, T_MAX - 1)
	INT(radius, 0, 0xFFFF)
	INT(kickback, -0xFFFF, 0xFFFF)
	INT(recoil, -0xFFFF, 0xFFFF)
	INT(charge, 0, 0xFFFF)
	FLOAT(basecharge, 0, 100)
	FLOAT(mincharge, 0, 100)
	FLOAT(maxcharge, 0, 100)
	FLOAT(elasticity, 0, 1)
	FLOAT(speed, 0, 100)

	#undef CAST
	#undef TYPE

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_recipe_ ##n, f, a, b)
	#define INIT recipe *e = checkrecipe();
	#define DEBUG "recipe[%i]"
	#define DEBUG_IND recipes.find(e)

	ICOMMAND(r_recipe_add_ingredient, "ii", (int *base, int *qty),
		recipe *e = checkrecipe();
		if(!e) return;
		if(*qty <= 0) {conoutf(CON_ERROR, "\fs\f3ERROR:\fr can't add an ingredient with a quantity of <= 0"); return;}

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr added ingredient %i (%i) to " DEBUG, *base, *qty, DEBUG_IND);

		e->ingredients.add(recipe::ingredient(*base, *qty));
		e->optimise()
	)

	ICOMMAND(r_recipe_add_catalyst, "ii", (int *base, int *qty),
		recipe *e = checkrecipe();
		if(!e) return;
		if(*qty <= 0) {conoutf(CON_ERROR, "\fs\f3ERROR:\fr can't add an ingredient with a quantity of <= 0"); return;}

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr added catalyst %i (%i) to " DEBUG, *base, *qty, DEBUG_IND);

		e->catalysts.add(recipe::ingredient(*base, *qty));
		e->optimise()
	)

	ICOMMAND(r_recipe_add_product, "ii", (int *base, int *qty),
		recipe *e = checkrecipe();
		if(!e) return;
		if(*qty <= 0) {conoutf(CON_ERROR, "\fs\f3ERROR:\fr can't add an ingredient with a quantity of <= 0"); return;}

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr added product %i (%i) to " DEBUG, *base, *qty, DEBUG_IND);

		e->products.add(recipe::ingredient(*base, *qty));
		e->optimise()
	)

	ICOMMAND(r_recipe_get_ingredient, "i", (int *idx),
		recipe *e = checkrecipe();
		if(!e || !e->ingredients.inrange(*idx))
		{
			result("-1 0");
			return;
		}
		defformatstring(str)("%i %i", e->ingredients[*idx].base, e->ingredients[*idx].quantity);
		result(str);
	)

	ICOMMAND(r_recipe_get_catalyst, "i", (int *idx),
		recipe *e = checkrecipe();
		if(!e || !e->catalysts.inrange(*idx))
		{
			result("-1 0");
			return;
		}
		defformatstring(str)("%i %i", e->catalysts[*idx].base, e->catalysts[*idx].quantity);
		result(str);
	)

	ICOMMAND(r_recipe_get_product, "i", (int *idx),
		recipe *e = checkrecipe();
		if(!e || !e->products.inrange(*idx))
		{
			result("-1 0");
			return;
		}
		defformatstring(str)("%i %i", e->products[*idx].base, e->products[*idx].quantity);
		result(str);
	)

	ICOMMAND(r_recipe_num_ingredient, "", (),
		recipe *e = checkrecipe();
		if(!e) { intret(0); return; }
		intret(e->ingredients.length());
	)

	ICOMMAND(r_recipe_num_catalyst, "", (),
		recipe *e = checkrecipe();
		if(!e) { intret(0); return; }
		intret(e->catalysts.length());
	)

	ICOMMAND(r_recipe_num_product, "", (),
		recipe *e = checkrecipe();
		if(!e) { intret(0); return; }
		intret(e->products.length());
	)

	STRING(name)
	INT(flags, 0, recipe::MAX)
	STATREQ(reqs)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_ammo_ ##n, f, a, b)
	#define INIT ammotype *e = checkammotype();
	#define DEBUG "ammotype[%i]"
	#define DEBUG_IND ammotypes.find(e)

	ICOMMAND(r_ammo_add_item, "i", (int *i),
		INIT
		if(!e) return;

		e->items.add(*i);
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr Classified item %i as member of " DEBUG " (%s)", *i, DEBUG_IND, e->name ? e->name : "unnamed");
	)

	ICOMMAND(r_ammo_num_item,  "", (),
		INIT
		if(!e) {intret(0); return;}

		intret(e->items.length());
	)

	ICOMMAND(r_ammo_get_item, "i", (int *idx),
		INIT
		if(!e && !e->items.inrange(*idx)) {intret(-1); return;}

		intret(e->items[*idx]);
	)

	STRING(name)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_char_ ##n, f, a, b)
	#define INIT rpgchar *e = checkrpgchar();
	#define DEBUG "rpgchar[%p]"
	#define DEBUG_IND loadingrpgchar

	STRING(name)
	MODEL(mdl)
	STRING(portrait)
	INT(script, 0, scripts.length() - 1)
	INT(faction, 0, factions.length() - 1)
	INT(merchant, -1, merchants.length() - 1)
	STATS(base)
	FLOAT(health, 0, e->base.getmaxhp())
	FLOAT(mana, 0, e->base.getmaxmp())

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_faction_ ##n, f, a, b)
	#define INIT faction *e = checkfaction();
	#define DEBUG "faction[%i]"
	#define DEBUG_IND factions.find(e)

	ICOMMAND(r_faction_set_relation, "ii", (int *o, int *f),
		INIT
		if(!e) return;
		e->setrelation(*o, *f);
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr faction %i's liking of faction %i is now %i", DEBUG_IND, *o, *f);
	)

	ICOMMAND(r_faction_get_relation, "i", (int *o),
		INIT
		if (!e) return;

		intret(e->getrelation(*o));
	)

	STRING(name)
	STRING(logo)
	INT(base, 0, 100)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_obstacle_ ##n, f, a, b)
	#define INIT rpgobstacle *e = checkrpgobstacle();
	#define DEBUG "rpgobstacle[%p]"
	#define DEBUG_IND loadingrpgobstacle

	MODEL(mdl)
	INT(weight, 0, 0xFFFF)
	INT(script, 0, scripts.length() - 1)
	INT(flags, 0, rpgobstacle::F_MAX)


	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_container_ ##n, f, a, b)
	#define INIT rpgcontainer *e = checkrpgcontainer();
	#define DEBUG "rpgcontainer[%p]"
	#define DEBUG_IND loadingrpgcontainer

	MODEL(mdl)
	STRING(name)
	INT(faction, -1, factions.length() - 1)
	INT(merchant, -1, merchants.length() - 1)
	INT(capacity, 0, 0xFFFF)
	INT(script, 0, scripts.length() - 1)
	INT(lock, 0, 100)
	INT(magelock, 0, 100)


	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_platform_ ##n, f, a, b)
	#define INIT rpgplatform *e = checkrpgplatform();
	#define DEBUG "rpgplatform[%p]"
	#define DEBUG_IND loadingrpgplatform

	MODEL(mdl)
	INT(speed, 1, 1000)
	INT(flags, 0, rpgplatform::F_MAX)
	INT(script, 0, scripts.length() - 1)

	START(addroute, "ii", (int *from, int *to),
		INIT
		if(!e) return;
		vector<int> &detours = e->routes.access(*from, vector<int>());
		if(detours.find(*to) == -1)
		{
			detours.add(*to);
			if(DEBUG_CONF)
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr registered detour; " DEBUG "->routes[%i].add(%i)", DEBUG_IND, *from, *to);
		}
		else if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr route from %i to %i already registred for " DEBUG, *from, *to, DEBUG_IND);
	)


	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_trigger_ ##n, f, a, b)
	#define INIT rpgtrigger *e = checkrpgtrigger();
	#define DEBUG "rpgtrigger[%p]"
	#define DEBUG_IND loadingrpgtrigger


	MODEL(mdl)
	STRING(name)
	INT(flags, 0, rpgtrigger::F_MAX)
	INT(script, 0, scripts.length() - 1)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_merchant_ ##n, f, a, b)
	#define INIT merchant *e = checkmerchant();
	#define DEBUG "merchants[%i]"
	#define DEBUG_IND merchants.find(e)

	INT(currency, 0, 0xFFFF)
	INT(credit, 0, 0xFFFFFF)

	START(setrate, "iff", (int *cat, float *buy, float *sell),
		INIT
		if(!e || *cat < 0) return;
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr setting " DEBUG "->rate[%i] to %f%% and %f%%", DEBUG_IND, *cat, *buy * 100, *sell * 100);

		while(!e->rates.inrange(*cat)) e->rates.add(merchant::rate(0, 0));
		e->rates[*cat] = merchant::rate(*buy, *sell);
	)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	ICOMMAND(r_global_new, "s", (const char *s),
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr created new global variable - variables[%i] = \"%s\"", variables.length(), s);
		variables.add(newstring(s));
		intret(variables.length() - 1);
	)

	ICOMMAND(r_tip_new, "s", (const char *s),
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr added tip...\n%s", s);
		tips.add(newstring(s));
	)

	ICOMMAND(r_cat_new, "s", (const char *s),
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr Registering item category[%i] as  \"%s\"", categories.length(), s);
		categories.add(newstring(s));
		intret(categories.length() - 1);
	)
}
