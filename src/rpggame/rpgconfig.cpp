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
				ERRORF(#c " not defined or being loaded"); \
			return loading ## x; \
		} \
		ICOMMAND(r_select_ ## c, "se", (const char *ref, uint *contents), \
			x *old = loading ## x; \
			loading ## x = NULL; \
			\
			int objidx; bool hasobjidx; \
			reference *obj = rpgscript::searchstack(ref, objidx, hasobjidx); \
			if(obj) \
			{ \
				loading ## x = select; \
				if(loading ## x && DEBUG_VCONF) \
					DEBUGF("successfully selected \"" #c "\" from reference %s", ref); \
			} \
			\
			if(!loading ##x) \
			{ \
				if((loading ## x = vec.access(ref)) && DEBUG_VCONF) \
					DEBUGF("successfully selected \"" #c "\" using hash %s", ref); \
			} \
			if(loading ## x) \
			{\
				rpgscript::config->setref(loading ## x, true); \
				rpgexecute(contents); \
				rpgscript::config->setnull(true); \
			} \
			else \
				ERRORF("unable to select reference %s as type " #c, ref); \
			loading ## x = old; \
		) \
		ICOMMAND(r_num_ ## c, "", (), \
			if(DEBUG_VCONF) \
				DEBUGF("r_num_" #c " requested, returning %i", vec.length()); \
			intret(vec.length()); \
		)

	CHECK(script, script, scripts, NULL)
	CHECK(effect, effect, effects, NULL)
	CHECK(statusgroup, status, statuses,
		obj->getveffect(objidx) ? obj->getveffect(objidx)->group :
		obj->getaeffect(objidx) ? obj->getaeffect(objidx)->group :
		NULL
	)
	CHECK(faction, faction, factions,
		  obj->getchar(objidx) ? obj->getchar(objidx)->faction :
		  obj->getcontainer(objidx) ? obj->getcontainer(objidx)->faction :
		  NULL
	)
	CHECK(recipe, recipe, recipes, NULL)
	CHECK(ammotype, ammo, ammotypes, NULL)
	CHECK(mapscript, mapscript, mapscripts, NULL)
	CHECK(merchant, merchant, merchants,
		obj->getchar(objidx) ? obj->getchar(objidx)->merchant :
		obj->getcontainer(objidx) ? obj->getcontainer(objidx)->merchant :
		NULL
	)

	#undef CHECK

	//These don't have a vector counter part

	rpgent *loadingrpgent = NULL;
	static rpgent *checkrpgent ()
	{
		if(! loadingrpgent)
			ERRORF("[rpgent] not being loaded");
		return loadingrpgent;
	}
	ICOMMAND(r_select_ent, "se", (const char *ref, uint *body),
		rpgent *old = loadingrpgent;
		loadingrpgent = NULL;

		int objidx; bool hasobjidx;
		reference *obj = rpgscript::searchstack(ref, objidx, hasobjidx);
		if(obj)
		{
			loadingrpgent = obj->getent(objidx);
			if(loadingrpgent && DEBUG_VCONF)
				DEBUGF("successfully selected [rpgent] from reference %s", ref);
		}
		if(loadingrpgent)
			rpgexecute(body);
		else
			ERRORF("unable to select reference %s as type [rpgent]", ref);
		loadingrpgent = old;
	)

	#define CHECK(x, c, select) \
		x *loading ## x = NULL; \
		static x *check ##x () \
		{ \
			if(! loading ## x) \
				ERRORF(#c " not being loaded"); \
			return loading ## x; \
		} \
		ICOMMAND(r_select_ ## c, "se", (const char *ref, uint *body),  \
			x *old = loading ## x; \
			loading ## x = NULL; \
			rpgent *oldrpgent = loadingrpgent; \
			 \
			int objidx; bool hasobjidx; \
			reference *obj = rpgscript::searchstack(ref, objidx, hasobjidx); \
			if(obj) \
			{ \
				loading ## x = select; \
				loadingrpgent = obj->getent(objidx); \
				if(loading ## x && DEBUG_VCONF) \
					DEBUGF("successfully selected \"" #c "\" from reference %s", ref); \
			} \
			if(loading ## x) \
				rpgexecute(body); \
			else \
				ERRORF("unable to select reference %s as type " #c, ref); \
			loading ## x = old; \
			loadingrpgent = oldrpgent; \
		)

	//FIXME can generate multiple copies of otherwise identical items.
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
			ERRORF("use not defined or being loaded");
		else if(loadinguse->type < type)
			ERRORF("trying to set an unavailable use property");
		else
			return loadinguse;
		return NULL;
	}
	ICOMMAND(r_select_item_use, "se", (const char *ref, uint *contents),
		use *old = loadinguse;
		loadinguse = NULL;
		int objidx; bool hasobjidx;
		reference *obj = rpgscript::searchstack(ref, objidx, hasobjidx);

		if(obj && obj->getequip(objidx))
		{
			if(DEBUG_VCONF)
				DEBUGF("successfully selected \"use\" from reference %s", ref);
			loadinguse = obj->getequip(objidx)->it->uses[obj->getequip(objidx)->use];
		}
		else if(loadingitem)
		{
			int idx = parseint(ref);
			if(loadingitem->uses.inrange(idx))
			{
				if(DEBUG_VCONF)
					DEBUGF("successfully selected \"use\" using index %i", idx);
				loadinguse = loadingitem->uses[idx];
			}
		}

		if(loadinguse) rpgexecute(contents);
		else ERRORF("unable to select reference %s as type use", ref);
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
	/// #define DEBUG_STR "scripts[%i]"
	/// #define DEBUG_IND scripts.length() - 1

	#define PREAMBLE(name, prep, lookup, print) \
		INIT \
		if(!e) \
		{ \
			ERRORF(#name " requires you to have a valid object selected first"); \
			return; \
		} \
		if(*numargs <= 0) \
		{ \
			prep \
			if(*numargs == -1) { lookup; } \
			else { print; } \
			return; \
		} \

	#define INTNF(name, var, min, max, body) \
		START(name, "iN$", (int *i, int *numargs, ident *self), \
			PREAMBLE(name, , intret(e->var), printvar(self, e->var)) \
			e->var = clamp(*i, int(min), int(max)); \
			body; \
			if(e->var != *i) \
				WARNINGF("value provided for " DEBUG_STR "->" #var " exceeded limits", DEBUG_IND); \
			if(DEBUG_CONF || e->var != *i) \
				conoutf(DEBUG_STR "->" #var " = %i (0x%.8X)", DEBUG_IND, e->var, e->var); \
		)

	#define INTF(var, min, max, body) INTNF(var, var, min, max, body)
	#define INTN(name, var, min, max) INTNF(name, var, min, max, )
	#define INT(var, min, max) INTF(var, min, max, )

	#define INTNRO(name, var) \
		START(name, "N$", (int *numargs, ident *self), \
			PREAMBLE(name, , intret(e->var), printvar(self, e->var)) \
			conoutf(CON_WARN, #name " is read-only"); \
		)

	#define INTRO(var) INTNRO(var, var)

	#define FLOATNF(name, var, min, max, body) \
		START(name, "fN$", (float *f, int *numargs, ident *self), \
			PREAMBLE(name, , floatret(e->var), printfvar(self, e->var)) \
			e->var = clamp(*f, float(min), float(max)); \
			body; \
			if(e->var != *f) \
				WARNINGF("value provided for " DEBUG_STR "->" #var " exceeded limits", DEBUG_IND); \
			if(DEBUG_CONF || e->var != *f) \
				conoutf(DEBUG_STR "->" #var " = %g", DEBUG_IND, e->var); \
		)

	#define FLOATF(var, min, max, body) FLOATNF(var, var, min, max, body)
	#define FLOATN(name, var, min, max) FLOATNF(name, var, min, max, )
	#define FLOAT(var, min, max) FLOATF(var, min, max, )

	#define FLOATNRO(name, var) \
		START(name, "N$", (int *numargs, ident *self), \
			PREAMBLE(name, , floatret(e->var), printfvar(self, e->var)) \
			conoutf(CON_WARN, #name " is read-only"); \
		)

	#define FLOATRO(var) INTNRO(var, var)

	#define STRINGNF(name, var, body) \
		START(name, "sN$", (const char *s, int *numargs, ident *self), \
			PREAMBLE(name, , result(e->var ? e->var : ""), printsvar(self, e->var ? e->var : "")) \
			if(e->var) delete[] e->var; \
			e->var = newstring(s); \
			body; \
			if(DEBUG_CONF) \
				conoutf(CON_DEBUG, DEBUG_STR "->" #var " = %s", DEBUG_IND, e->var); \
		)

	#define STRINGF(var, body) STRINGNF(var, var, body)
	#define STRINGN(name, var) STRINGNF(name, var, )
	#define STRING(var) STRINGN(var, var)
	#define MODEL(var) STRINGF(var, preloadmodel(e->var))

	#define STRINGNRO(name, var) \
		START(name, "N$", (int *numargs, ident *self), \
			PREAMBLE(name, , result(e->var), printsvar(self, e->var)) \
			conoutf(CON_WARN, #name " is read-only"); \
		)

	#define STRINGRO(var) STRINGNRO(var, var);

	#define VECNF(name, var, l1, l2, l3, h1, h2, h3, body) \
		START(name, "fffN$", (float *x, float *y, float *z, int *numargs, ident *self), \
			PREAMBLE(name, defformatstring(res, "%g %g %g", e->var.x, e->var.y, e->var.z);, result(res), printsvar(self, res)) \
			const vec res(*x, *y, *z); \
			const vec vmin(h1, h2, h3); \
			const vec vmax(l1, l2, l3); \
			e->var = vec(res).min(vmin).max(vmax); \
			body; \
			if(e->var != res) \
				WARNINGF("value provided for " DEBUG_STR "->" #var " exceeded limits", DEBUG_IND); \
			if(DEBUG_CONF || e->var != res) \
				conoutf(DEBUG_STR "->" #var " = (%g, %g, %g)", DEBUG_IND, e->var.x, e->var.y, e->var.z); \
		)

	#define HASHNF(name, var, verify, body) \
		START(name, "sN$", (const char *s, int *numargs, ident *self), \
			PREAMBLE(name, , result(e->var ? e->var : ""), printsvar(self, e->var ? e->var : "")) \
			if(!(verify)) \
			{ \
				ERRORF("\"%s\" is not a valid hash value for " DEBUG_STR "->" #var " - The command was ignored", s, DEBUG_IND); \
				return; \
			} \
			if(! *s ) e->var = NULL; \
			else e->var = queryhashpool(s); \
			body; \
			if(DEBUG_CONF) \
				conoutf(DEBUG_STR "->" #var " = %s", DEBUG_IND, s); \
		)

	#define HASHF(var, verify, body) HASHNF(var, var, verify, body)
	#define HASHN(name, var, verify) HASHNF(name, var, verify, )
	#define HASH(var, verify) HASHF(var, verify, )

	#define SHASHNF(name, var, ht, ver, body) \
		START(name, "sN$", (const char *s, int *numargs, ident *self), \
			PREAMBLE(name, , result(e->var ? e->var->key : ""), printsvar(self, e->var ? e->var->key : "")) \
			void **v = (void **) &e->var; \
			void *ptr = NULL; \
			if((ver || s[0]) && !(ptr = (void *) ht.access(s))) \
			{ \
				ERRORF("\"%s\" is not a valid hash value for " DEBUG_STR "->" #var " - The command was ignored", s, DEBUG_IND); \
				return; \
			} \
			else s = ""; \
			*v = ptr; \
			body; \
			if(DEBUG_CONF) \
				conoutf(DEBUG_STR "->" #var " = %s", DEBUG_IND, s); \
		)

	#define SHASHF(var, ht, ver, body) SHASHNF(var, var, ht, ver, body)
	#define SHASHN(name, var, ht, ver) SHASHNF(name, var, ht, ver, )
	#define SHASH(var, ht, ver) SHASHF(var, ht, ver, )

	#define VECF(var, l1, l2, l3, h1, h2, h3, body) VECNF(var, var, l1, l2, l3, h1, h2, h3, body)
	#define VECN(name, var, l1, l2, l3, h1, h2, h3) VECNF(name, var, l1, l2, l3, h1, h2, h3, )
	#define VEC(var, l1, l2, l3, h1, h2, h3) VECF(var, l1, l2, l3, h1, h2, h3, )

	#define VECNRO(name, var) \
		START(name, "N$", (int *numargs, ident *self), \
			PREAMBLE(name, defformatstring(res, "%g %g %g", e->var.x, e->var.y, e->var.z);, result(res), printsvar(self, res)) \
			conoutf(CON_WARN, #name " is read-only"); \
		)

	#define VECRO(var) VECNRO(var, var)

	#define BOOLFN(name, var, body) \
		START(name, "iN$", (int *i, int *numargs, ident *self), \
			PREAMBLE(name, , intret(e->var), printvar(self, e->var)) \
			e->var = *i != 0; \
			body; \
			if(DEBUG_CONF) \
				conoutf(CON_DEBUG, DEBUG_STR "->" #var " = %i", DEBUG_IND, e->var); \
		)

	#define BOOLF(var, body) BOOLFN(var, var, body)
	#define BOOLN(name, var) BOOLFN(name, var, )
	#define BOOL(var) BOOLF(var, )

	#define BOOLNRO(var, name) INTNRO(var, name);
	#define BOOLRO(var) INTNRO(var, var)

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
		INTN(var ## _strength, var.baseattrs[STAT_STRENGTH], 1, 100) \
		INTN(var ## _endurance, var.baseattrs[STAT_ENDURANCE], 1, 100) \
		INTN(var ## _agility, var.baseattrs[STAT_AGILITY], 1, 100) \
		INTN(var ## _charisma, var.baseattrs[STAT_CHARISMA], 1, 100) \
		INTN(var ## _wisdom, var.baseattrs[STAT_WISDOM], 1, 100) \
		INTN(var ## _intelligence, var.baseattrs[STAT_INTELLIGENCE], 1, 100) \
		INTN(var ## _luck, var.baseattrs[STAT_LUCK], 1, 100) \
		\
		INTNRO(var ## _delta_strength, var.deltaattrs[STAT_STRENGTH]) \
		INTNRO(var ## _delta_endurance, var.deltaattrs[STAT_ENDURANCE]) \
		INTNRO(var ## _delta_agility, var.deltaattrs[STAT_AGILITY]) \
		INTNRO(var ## _delta_charisma, var.deltaattrs[STAT_CHARISMA]) \
		INTNRO(var ## _delta_wisdom, var.deltaattrs[STAT_WISDOM]) \
		INTNRO(var ## _delta_intelligence, var.deltaattrs[STAT_INTELLIGENCE]) \
		INTNRO(var ## _delta_luck, var.deltaattrs[STAT_LUCK]) \
		\
		INTN(var ## _armour, var.baseskills[SKILL_ARMOUR], 0, 100) \
		INTN(var ## _diplomacy, var.baseskills[SKILL_DIPLOMACY], 0, 100) \
		INTN(var ## _magic, var.baseskills[SKILL_MAGIC], 0, 100) \
		INTN(var ## _marksman, var.baseskills[SKILL_MARKSMAN], 0, 100) \
		INTN(var ## _melee, var.baseskills[SKILL_MELEE], 0, 100) \
		INTN(var ## _stealth, var.baseskills[SKILL_STEALTH], 0, 100) \
		INTN(var ## _craft, var.baseskills[SKILL_CRAFT], 0, 100) \
		\
		INTNRO(var ## _delta_armour, var.deltaskills[SKILL_ARMOUR]) \
		INTNRO(var ## _delta_diplomacy, var.deltaskills[SKILL_DIPLOMACY]) \
		INTNRO(var ## _delta_magic, var.deltaskills[SKILL_MAGIC]) \
		INTNRO(var ## _delta_marksman, var.deltaskills[SKILL_MARKSMAN]) \
		INTNRO(var ## _delta_melee, var.deltaskills[SKILL_MELEE]) \
		INTNRO(var ## _delta_stealth, var.deltaskills[SKILL_STEALTH]) \
		INTNRO(var ## _delta_craft, var.deltaskills[SKILL_CRAFT]) \
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
		INTNRO(var ## _delta_fire_thresh, var.deltathresh[ATTACK_FIRE]) \
		INTNRO(var ## _delta_water_thresh, var.deltathresh[ATTACK_WATER]) \
		INTNRO(var ## _delta_air_thresh, var.deltathresh[ATTACK_AIR]) \
		INTNRO(var ## _delta_earth_thresh, var.deltathresh[ATTACK_EARTH]) \
		INTNRO(var ## _delta_arcane_thresh, var.deltathresh[ATTACK_ARCANE]) \
		INTNRO(var ## _delta_mind_thresh, var.deltathresh[ATTACK_MIND]) \
		INTNRO(var ## _delta_holy_thresh, var.deltathresh[ATTACK_HOLY]) \
		INTNRO(var ## _delta_darkness_thresh, var.deltathresh[ATTACK_DARKNESS]) \
		INTNRO(var ## _delta_slash_thresh, var.deltathresh[ATTACK_SLASH]) \
		INTNRO(var ## _delta_blunt_thresh, var.deltathresh[ATTACK_BLUNT]) \
		INTNRO(var ## _delta_pierce_thresh, var.deltathresh[ATTACK_PIERCE]) \
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
		INTNRO(var ## _delta_fire_resist, var.deltaresist[ATTACK_FIRE]) \
		INTNRO(var ## _delta_water_resist, var.deltaresist[ATTACK_WATER]) \
		INTNRO(var ## _delta_air_resist, var.deltaresist[ATTACK_AIR]) \
		INTNRO(var ## _delta_earth_resist, var.deltaresist[ATTACK_EARTH]) \
		INTNRO(var ## _delta_arcane_resist, var.deltaresist[ATTACK_ARCANE]) \
		INTNRO(var ## _delta_mind_resist, var.deltaresist[ATTACK_MIND]) \
		INTNRO(var ## _delta_holy_resist, var.deltaresist[ATTACK_HOLY]) \
		INTNRO(var ## _delta_darkness_resist, var.deltaresist[ATTACK_DARKNESS]) \
		INTNRO(var ## _delta_slash_resist, var.deltaresist[ATTACK_SLASH]) \
		INTNRO(var ## _delta_blunt_resist, var.deltaresist[ATTACK_BLUNT]) \
		INTNRO(var ## _delta_pierce_resist, var.deltaresist[ATTACK_PIERCE]) \
		\
		INTN(var ## _maxspeed, var.bonusmovespeed, 0, 100) \
		INTN(var ## _jumpvel, var.bonusjumpvel, 0, 100) \
		INTN(var ## _maxhealth, var.bonushealth, 0, 100000) \
		INTN(var ## _maxmana, var.bonusmana, 0, 100000) \
		INTN(var ## _crit, var.bonuscrit, -100, 100) \
		FLOATN(var ## _healthregen, var.bonushregen, 0, 1000) \
		FLOATN(var ## _manaregen, var.bonusmregen, 0, 1000) \
		\
		INTNRO(var ## _delta_maxspeed, var.deltamovespeed) \
		INTNRO(var ## _delta_jumpvel, var.deltajumpvel) \
		INTNRO(var ## _delta_maxhealth, var.deltahealth) \
		INTNRO(var ## _delta_maxmana, var.deltamana) \
		INTNRO(var ## _delta_crit, var.deltacrit) \
		FLOATNRO(var ## _delta_healthregen, var.deltahregen) \
		FLOATNRO(var ## _delta_manaregen, var.deltamregen)

	ICOMMAND(r_script_node, "see", (const char *n, uint *txt, uint *scr),
		script *s = checkscript();
		if(!s) return;

		dialogue *node = s->chat.access(n);
		if(!node)
		{
			if(DEBUG_CONF)
				DEBUGF("scripts[%s]->node[%s] registered", s->key, n);
			const char *name = queryhashpool(n);

			node = &s->chat[name];
			node->node = name;
		}

		if(DEBUG_CONF)
			DEBUGF("scripts[%s]->node[%s] set", s->key, n);

		freecode(node->text);
		node->text = txt;
		keepcode(txt);

		freecode(node->script);
		node->script = scr;
		keepcode(scr);
	)

	signal *getsignal(const char *sig, hashnameset<signal> &table)
	{
		signal *listen = table.access(sig);
		if(listen) return listen;

		const char *name = queryhashpool(sig);
		listen = &table[name];
		listen->name = name;
		return listen;
	}

	ICOMMAND(r_script_signal, "se", (const char *sig, uint *code),
		script *scr = checkscript();
		if(!scr) return;

		if(DEBUG_CONF)
			DEBUGF("scripts[%s]->listeners[%s]->code set", scr->key, sig);
		signal *listen = getsignal(sig, scr->listeners);
		listen->setcode(code);
	)

	ICOMMAND(r_script_signal_append, "se", (const char *sig, uint *code),
		script *scr = checkscript();
		if(!scr) return;

		if(DEBUG_CONF)
			DEBUGF("scripts[%s]->listeners[%s]->code appended onto", scr->key, sig);
		signal *listen = getsignal(sig, scr->listeners);
		listen->appendcode(code);
	)

	ICOMMAND(r_mapscript_signal, "se", (const char *sig, uint *code),
		mapscript *scr = checkmapscript();
		if(!scr) return;

		if(DEBUG_CONF)
			DEBUGF("scripts[%s]->listeners[%s]->code set", scr->key, sig);
		signal *listen = getsignal(sig, scr->listeners);
		listen->setcode(code);
	)

	ICOMMAND(r_mapscript_signal_append, "se", (const char *sig, uint *code),
		mapscript *scr = checkmapscript();
		if(!scr) return;

		if(DEBUG_CONF)
			DEBUGF("scripts[%s]->listeners[%s]->code appended onto", scr->key, sig);
		signal *listen = getsignal(sig, scr->listeners);
		listen->appendcode(code);
	)

	#define START(n, f, a, b) ICOMMAND(r_effect_ ##n, f, a, b)
	#define INIT effect *e = checkeffect();
	#define DEBUG_STR "effect[%s]"
	#define DEBUG_IND e->key

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
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_status_ ##n, f, a, b)
	#define INIT statusgroup *e = checkstatusgroup();
	#define DEBUG_STR "statusgroup[%s]"
	#define DEBUG_IND e->key

	ICOMMAND(r_status_addgeneric, "iiif", (int *t, int *s, int *d, float *v),
		if(STATUS_INVALID_GENERIC(*t))
		{
			ERRORF("trying to create a generic status effect of non-generic or invalid type, '%i'", *t);
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
			DEBUGF("adding generic effect (%i %i %i), to " DEBUG_STR, *t, *d, *s, DEBUG_IND);
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
			DEBUGF("adding polymorph effect (%s), to " DEBUG_STR, m, DEBUG_IND);
	)

	ICOMMAND(r_status_addlight, "fffiif", (float *r, float *g, float *b, int *str, int *d, float *v),
		statusgroup *e = checkstatusgroup();
		if(!e) return;

		status_light *l = new status_light();
		e->effects.add(l);
		l->type = STATUS_LIGHT;

		l->colour = vec(*r, *g, *b).clamp(-1, 1);
		l->strength = clamp(*str, 8, 512);
		l->duration = *d;
		l->variance = clamp(*v, 0.f, 1.0f);

		if(DEBUG_CONF)
			DEBUGF("adding light effect (%f %f %f --> %i), to " DEBUG_STR, l->colour.x, l->colour.y, l->colour.z, l->strength, DEBUG_IND);
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

		if(DEBUG_CONF)
			DEBUGF("adding signal effect %s (%i %i %f) to" DEBUG_STR, s, *str, *d, *v, DEBUG_IND)
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

		if(DEBUG_CONF)
		{
			DEBUGF("adding script effect (%i %i %f) to" DEBUG_STR, *str, *d, *v, DEBUG_IND);
			DEBUGF("script: [%s]", s);
		}
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
				formatstring(str, "%i %i %i %f %f %f %f", STATUS_LIGHT, s->strength, s->duration, s->variance, ls->colour.x, ls->colour.y, ls->colour.z);
				break;
			}
			case STATUS_POLYMORPH:
			{
				status_polymorph *ps = (status_polymorph *) s;
				formatstring(str, "%i %i %i %f \"%s\"", STATUS_POLYMORPH, s->strength, s->duration, s->variance, ps->mdl);
				break;
			}
			case STATUS_SIGNAL:
			{
				status_signal *ss = (status_signal *) s;
				formatstring(str, "%i %i %i %f \"%s\"", s->type, s->strength, s->duration, s->variance, ss->signal);
				break;
			}
			case STATUS_SCRIPT: //return script?
			default:
				formatstring(str, "%i %i %i %f", s->type, s->strength, s->duration, s->variance);
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
	SHASH(persisteffect, effects, 0)
	SHASH(areaeffect, effects, 0)

	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_ent_ ##n, f, a, b)
	#define INIT rpgent *e = checkrpgent();
	#define DEBUG_STR "[rpgent][%p]"
	#define DEBUG_IND loadingrpgent

	//Generic entity types
	VECF(o, 0, 0, 0, getworldsize(), getworldsize(), getworldsize(), e->resetinterp())
	VEC(vel, -10000, -10000, -10000, 10000, 10000, 10000)
	VEC(falling, 0, 0, -10000, 0, 0, 10000)
	FLOAT(yaw, 0, 360)
	FLOAT(pitch, -90, 90)
	FLOAT(roll, -180, 180)

	//TODO custom bounding boxes for worlspace entities.
// 	FLOAT(radius)
// 	FLOAT(eyeheight)
// 	FLOAT(aboveeye)
// 	FLOAT(xradius)
// 	FLOAT(yradius)
// 	FLOAT(zmargin)
// 	INT(collidetype, COLLIDE_AABB, COLLIDE_ELLIPSE);

	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_item_ ##n, f, a, b)
	#define INIT item *e = checkitem();
	#define DEBUG_STR "item[%p]"
	#define DEBUG_IND loadingitem

	ICOMMAND(r_item_use_new_consumable, "", (),
		item *e = checkitem();
		if(!e) return;

		if(!e->charges) e->charges = 1;

		loadinguse = (e->uses.add(new use(e->script)));
		if(DEBUG_CONF)
			DEBUGF("created new consumable use (%i) for " DEBUG_STR, e->uses.length() - 1, DEBUG_IND);
	)

	ICOMMAND(r_item_use_new_armour, "", (),
		item *e = checkitem();
		if(!e) return;

		if(!e->charges) e->charges = -1;

		loadinguse = (e->uses.add(new use_armour(e->script)));
		if(DEBUG_CONF)
			DEBUGF("created new armour use (%i) for " DEBUG_STR, e->uses.length() - 1, DEBUG_IND);
	)

	ICOMMAND(r_item_use_new_weapon, "", (),
		item *e = checkitem();
		if(!e) return;

		if(!e->charges) e->charges = -1;

		loadinguse = (e->uses.add(new use_weapon(e->script)));
		if(DEBUG_CONF)
			DEBUGF("created new weapon use (%i) for " DEBUG_STR, e->uses.length() - 1, DEBUG_IND);
	)


	STRING(name)
	STRING(icon)
	STRING(description)
	MODEL(mdl)
	SHASH(script, scripts, 1)

	VEC(colour, 0, 0, 0, 1, 1, 1)
	INT(quantity, 0, 0xFFFFFF)
	INT(category, 0, categories.length() - 1)
	INT(flags, 0, item::F_MAX)
	INT(value, 0, 0xFFFFFF)
	INT(maxdurability, 0, 0xFFFFFF)
	INT(charges, -1, 0xFFFF)
	FLOAT(scale, 0.2, 5);
	FLOAT(weight, 0, 0xFFFF)
	FLOAT(durability, 0, 0xFFFFFF)
	FLOAT(recovery, 0, 1)

	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_item_use_ ##n, f, a, b)
	#define INIT CAST *e = (CAST *) checkuse(TYPE);
	#define DEBUG_STR "item[%p]->uses[%i]"
	#define DEBUG_IND loadingitem, loadingitem->uses.find(loadinguse)

	#define CAST use
	#define TYPE USE_CONSUME

	INTRO(type)
	SHASH(script, scripts, 1)

	STRING(name)
	STRING(description)
	STRING(icon)
	INT(cooldown, 0, 0xFFFF)
	INT(chargeflags, 0, CHARGE_MAX)

	START(new_status, "sif", (const char *st, int *el, float *m),
		INIT if(!e) return;
		statusgroup *sg = statuses.access(st);
		if(st)
		{
			if(DEBUG_CONF)
				DEBUGF(DEBUG_STR " received status effect %s of element %i; multiplier %f", DEBUG_IND, st, *el, *m);
			e->effects.add(new inflict(sg, *el, *m));
		}
		else
			ERRORF("r_item_use_new_status: statuseffect \"%s\" does not exist", st);
	)

	#undef CAST
	#undef TYPE
	#define CAST use_armour
	#define TYPE USE_ARMOUR

	MODEL(vwepmdl)
	MODEL(hudmdl)
	SHASH(idlefx, effects, 0)

	STATREQ(reqs)
	INT(slots, 0, SLOT_MAX)
	INT(skill, -1, SKILL_MAX - 1)

	#undef CAST
	#undef TYPE
	#define CAST use_weapon
	#define TYPE USE_WEAPON

	SHASH(projeffect, effects, 0)
	SHASH(traileffect, effects, 0)
	SHASH(deatheffect, effects, 0)
	SHASH(ammo, ammotypes, 1)

	INT(range, 0, 1024)
	INT(angle, 0, 360)
	INT(lifetime, 0, 0xFFFF)
	INT(gravity, -1000, 1000)
	INT(cost, 0, 0xFFFF)
	INT(pflags, 0, P_MAX)
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
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_recipe_ ##n, f, a, b)
	#define INIT recipe *e = checkrecipe();
	#define DEBUG_STR "recipe[%s]"
	#define DEBUG_IND e->key

	ICOMMAND(r_recipe_add_ingredient, "si", (const char *base, int *qty),
		recipe *e = checkrecipe();
		if(!e) return;
		if(*qty <= 0) {ERRORF("can't add an ingredient with a quantity of <= 0"); return;}

		if(DEBUG_CONF)
			DEBUGF("added ingredient %s (%i) to " DEBUG_STR, base, *qty, DEBUG_IND);

		e->ingredients.add(recipe::ingredient(queryhashpool(base), *qty));
		e->optimise()
	)

	ICOMMAND(r_recipe_add_catalyst, "si", (const char *base, int *qty),
		recipe *e = checkrecipe();
		if(!e) return;
		if(*qty <= 0) {ERRORF("can't add an ingredient with a quantity of <= 0"); return;}

		if(DEBUG_CONF)
			DEBUGF("added catalyst %s (%i) to " DEBUG_STR, base, *qty, DEBUG_IND);

		e->catalysts.add(recipe::ingredient(queryhashpool(base), *qty));
		e->optimise()
	)

	ICOMMAND(r_recipe_add_product, "si", (const char *base, int *qty),
		recipe *e = checkrecipe();
		if(!e) return;
		if(*qty <= 0) {ERRORF("can't add an ingredient with a quantity of <= 0"); return;}

		if(DEBUG_CONF)
			DEBUGF("added product %s (%i) to " DEBUG_STR, base, *qty, DEBUG_IND);

		e->products.add(recipe::ingredient(queryhashpool(base), *qty));
		e->optimise()
	)

	ICOMMAND(r_recipe_get_ingredient, "i", (int *idx),
		recipe *e = checkrecipe();
		if(!e || !e->ingredients.inrange(*idx))
		{
			result("-1 0");
			return;
		}
		defformatstring(str, "\"%s\" %i", e->ingredients[*idx].base, e->ingredients[*idx].quantity);
		result(str);
	)

	ICOMMAND(r_recipe_get_catalyst, "i", (int *idx),
		recipe *e = checkrecipe();
		if(!e || !e->catalysts.inrange(*idx))
		{
			result("-1 0");
			return;
		}
		defformatstring(str, "\"%s\" %i", e->catalysts[*idx].base, e->catalysts[*idx].quantity);
		result(str);
	)

	ICOMMAND(r_recipe_get_product, "i", (int *idx),
		recipe *e = checkrecipe();
		if(!e || !e->products.inrange(*idx))
		{
			result("-1 0");
			return;
		}
		defformatstring(str, "\"%s\" %i", e->products[*idx].base, e->products[*idx].quantity);
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
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_ammo_ ##n, f, a, b)
	#define INIT ammotype *e = checkammotype();
	#define DEBUG_STR "ammotype[%s]"
	#define DEBUG_IND e->key

	ICOMMAND(r_ammo_add_item, "s", (const char *s),
		INIT
		if(!e) return;

		e->items.add(queryhashpool(s));
		if(DEBUG_CONF)
			DEBUGF("Classified item %s as member of " DEBUG_STR " (%s)", s, DEBUG_IND, e->name ? e->name : "unnamed");
	)

	ICOMMAND(r_ammo_num_item,  "", (),
		INIT
		if(!e) {intret(0); return;}

		intret(e->items.length());
	)

	ICOMMAND(r_ammo_get_item, "i", (int *idx),
		INIT
		if(!e && !e->items.inrange(*idx)) {intret(-1); return;}

		result(e->items[*idx]);
	)

	STRING(name)

	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_char_ ##n, f, a, b)
	#define INIT rpgchar *e = checkrpgchar();
	#define DEBUG_STR "rpgchar[%p]"
	#define DEBUG_IND loadingrpgchar

	STRING(name)
	MODEL(mdl)
	STRING(portrait)
	SHASH(script, scripts, 1)
	SHASH(faction, factions, 1)
	SHASH(merchant, merchants, 0)

	VEC(colour, 0, 0, 0, 1, 1, 1)
	STATS(base)
	FLOAT(health, 0, e->base.getmaxhp())
	FLOAT(mana, 0, e->base.getmaxmp())
	FLOAT(scale, 0.2, 5);

	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_faction_ ##n, f, a, b)
	#define INIT faction *e = checkfaction();
	#define DEBUG_STR "faction[%s]"
	#define DEBUG_IND e->key

	ICOMMAND(r_faction_set_relation, "si", (const char *o, int *f),
		INIT
		if(!e) return;

		if(!factions.access(o))
		{
			ERRORF(DEBUG_STR "->relations[%s]; cannot set as faction %s does not exist", DEBUG_IND, o, o);
			return;
		}

		e->setrelation(queryhashpool(o), *f);
		if(DEBUG_CONF)
			DEBUGF(DEBUG_STR "->relations[%s] = %i", DEBUG_IND, o, *f);
	)

	ICOMMAND(r_faction_get_relation, "s", (const char *o),
		INIT
		if (!e) return;

		intret(e->getrelation(o));
	)

	STRING(name)
	STRING(logo)
	INT(base, 0, 100)

	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_obstacle_ ##n, f, a, b)
	#define INIT rpgobstacle *e = checkrpgobstacle();
	#define DEBUG_STR "rpgobstacle[%p]"
	#define DEBUG_IND loadingrpgobstacle

	MODEL(mdl)
	SHASH(script, scripts, 1)
	VEC(colour, 0, 0, 0, 1, 1, 1)
	INT(weight, 0, 0xFFFF)
	INT(flags, 0, rpgobstacle::F_MAX)
	FLOAT(scale, 0.2, 5);


	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_container_ ##n, f, a, b)
	#define INIT rpgcontainer *e = checkrpgcontainer();
	#define DEBUG_STR "rpgcontainer[%p]"
	#define DEBUG_IND loadingrpgcontainer

	MODEL(mdl)
	STRING(name)
	SHASH(script, scripts, 1)
	SHASH(faction, factions, 0)
	SHASH(merchant, merchants, 0)
	VEC(colour, 0, 0, 0, 1, 1, 1)
	INT(capacity, 0, 0xFFFF)
	INT(lock, 0, 100)
	INT(magelock, 0, 100)
	FLOAT(scale, 0.2, 5);


	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_platform_ ##n, f, a, b)
	#define INIT rpgplatform *e = checkrpgplatform();
	#define DEBUG_STR "rpgplatform[%p]"
	#define DEBUG_IND loadingrpgplatform

	MODEL(mdl)
	SHASH(script, scripts, 1)
	VEC(colour, 0, 0, 0, 1, 1, 1)
	INT(speed, 1, 1000)
	INT(flags, 0, rpgplatform::F_MAX)
	FLOAT(scale, 0.2, 5);

	START(addroute, "ii", (int *from, int *to),
		INIT
		if(!e) return;
		vector<int> &detours = e->routes[*from];
		if(detours.find(*to) == -1)
		{
			detours.add(*to);
			if(DEBUG_CONF)
				DEBUGF("registered detour; " DEBUG_STR "->routes[%i].add(%i)", DEBUG_IND, *from, *to);
		}
		else if(DEBUG_CONF)
			DEBUGF("route from %i to %i already registred for " DEBUG_STR, *from, *to, DEBUG_IND);
	)

	START(delroute, "ii", (int *from, int *to),
		INIT
		vector <int> *detours = NULL;
		if(!e || !(detours = e->routes.access(*from))) return;

		if(DEBUG_CONF)
			DEBUGF("attempting to remove route from %i to %i for " DEBUG_STR, *from, *to, DEBUG_IND);
		detours->removeobj(*to);
	)

	START(clearroutes, "", (),
		INIT
		if(!e) return;
		if(DEBUG_CONF)
			DEBUGF("clearing all route information for platform " DEBUG_STR, DEBUG_IND);
		e->routes.clear();
	)


	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_trigger_ ##n, f, a, b)
	#define INIT rpgtrigger *e = checkrpgtrigger();
	#define DEBUG_STR "rpgtrigger[%p]"
	#define DEBUG_IND loadingrpgtrigger


	MODEL(mdl)
	STRING(name)
	SHASH(script, scripts, 1)
	VEC(colour, 0, 0, 0, 1, 1, 1)
	INT(flags, 0, rpgtrigger::F_MAX)
	FLOAT(scale, 0.2, 5);

	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_merchant_ ##n, f, a, b)
	#define INIT merchant *e = checkmerchant();
	#define DEBUG_STR "merchants[%s]"
	#define DEBUG_IND e->key

	HASH(currency, true)
	INT(credit, 0, 0xFFFFFF)

	START(setrate, "iff", (int *cat, float *buy, float *sell),
		INIT
		if(!e || *cat < 0) return;
		if(DEBUG_CONF)
			DEBUGF("setting " DEBUG_STR "->rate[%i] to %f%% and %f%%", DEBUG_IND, *cat, *buy * 100, *sell * 100);

		while(!e->rates.inrange(*cat)) e->rates.add(merchant::rate(0, 0));
		e->rates[*cat] = merchant::rate(*buy, *sell);
	)

	#undef START
	#undef INIT
	#undef DEBUG_STR
	#undef DEBUG_IND

	ICOMMAND(r_tip_new, "s", (const char *s),
		if(DEBUG_CONF)
			DEBUGF("added tip...\n%s", s);
		tips.add(newstring(s));
	)

	ICOMMAND(r_cat_new, "s", (const char *s),
		if(DEBUG_CONF)
			DEBUGF("Registering item category[%i] as  \"%s\"", categories.length(), s);
		categories.add(newstring(s));
		intret(categories.length() - 1);
	)
}
