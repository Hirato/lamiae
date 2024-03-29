﻿#include "rpggame.h"

namespace game
{
	VAR(mapgameversion, 1, 0, -1);
	VARP(debug, 0, 0, DEBUG_MAX);
	VAR(forceverbose, 1, 0, -1);

	hashset<const char *> hashpool(1 << 12);

	// GAME DEFINITIONS
	hashset<script> scripts;       ///scripts, includes dialogue
	hashset<effect> effects;       ///pretty particle effects for spells and stuff
	hashset<statusgroup> statuses; ///status effect definitions to transfer onto victims
	hashset<faction> factions;
	hashset<ammotype> ammotypes;
	hashset<mapscript> mapscripts;
	hashset<recipe> recipes;
	hashset<merchant> merchants;

	vector<const char *> categories, tips;
	hashnameset<rpgvar> variables;
	hashnameset<journal> journals;

	string data; //data folder
	hashnameset<mapinfo> mapdata;
	mapinfo *curmap = NULL;
	bool connected = false;
	bool transfer = false;
	bool abort = false;

	rpgchar *player1 = new rpgchar();

	const char *cpmap = NULL;
	int cpnum = -1;

	void setcheckpoint(const char *map, int dest)
	{
		if(cpmap && cpnum == dest && !strcmp(cpmap, map)) return;

		DELETEA(cpmap);
		cpmap = newstring(map);
		cpnum = dest;
		conoutf("Checkpoint set!");
	}
	ICOMMAND(setcheckpoint, "si", (const char *m, int *d), setcheckpoint(m, *d));

	const char *queryhashpool(const char *str)
	{
		const char **ret = hashpool.access(str);
		if(ret) return *ret;
		if(DEBUG_WORLD)
			DEBUGF("Registered [ %s ] in hashpool; %i entries", str, hashpool.length() + 1);
		str = newstring(str);
		return hashpool.access(str, str);
	}

	const char *queryhashpool(const stringslice &str)
	{
		const char **ret = hashpool.access(str);
		if(ret) return *ret;
		const char *newstr = newstring(str);
		if(DEBUG_WORLD)
			DEBUGF("Registered [ %s ] in hashpool; %i entries", newstr, hashpool.length() + 1);
		return hashpool.access(newstr, newstr);
	}

	//important variables/configuration
	SVAR(firstmap, "");
	VAR(gameversion, 0, 0, 0x7FFFFFFF);
	VAR(compatversion, 0, 0, 0x7FFFFFFF);
	VARP(friendlyfire, 0, 1, 3);
	//

	ICOMMAND(primaryattack, "D", (int *down),
		player1->primary = *down != 0 && ! player1->secondary;
	)

	ICOMMAND(secondaryattack, "D", (int *down),
		player1->secondary = *down != 0 && ! player1->primary;
	)

//TODO redo hotkeys
// 	vector<equipment> hotkeys;
// 	equipment pending(-1, -1);
//
	ICOMMAND(hotkey, "i", (int *n),
		if(*n < 0) return;

		if(rpggui::open() && rpggui::hotkey(*n))
			return;
// 		else if(pending.base >= 0)
// 		{
// 			while(!hotkeys.inrange(*n))
// 				hotkeys.add(equipment(-1, -1));
//
// 			hotkeys[*n] = pending;
// 			pending = equipment(-1, -1);
// 		}
// 		else if(!rpggui::open() && hotkeys.inrange(*n) && hotkeys[*n].base >= 0 && hotkeys[*n].use >= 0) //only allow with GUIs closed
// 		{
// 			int i = player1->getitemcount(hotkeys[*n].base);
// 			if(!i) return;
// 			if(!player1->dequip(hotkeys[*n].base, -1))
// 				player1->equip(hotkeys[*n].base, hotkeys[*n].use);
// 		}
	)
//
// 	ICOMMAND(hotkey_get, "i", (int *idx),
// 		if(!hotkeys.inrange(*idx))
// 		{
// 			result("-1 -1");
// 			return;
// 		}
// 		defformatstring(ret, "%i %i", hotkeys[*idx].base, hotkeys[*idx].use);
// 		result(ret);
// 	)
//
// 	ICOMMAND(hotkey_set_pending, "ii", (int *it, int *u),
// 		pending.base = *it;
// 		pending.use = *u;
// 	)
//
// 	ICOMMAND(hotkey_get_pending, "ii", (int *it, int *u),
// 		defformatstring(str, "%i %i", pending.base, pending.use);
// 		result(str);
// 	)
//
// 	ICOMMAND(hotkey_clear, "i", (int *idx),
// 		if(hotkeys.inrange(*idx))
// 			hotkeys[*idx] = equipment(-1, -1);
// 	)

	const char *datapath(const char *subdir)
	{
		static string pth;
		if(!connected || !subdir)
			copystring(pth, "games");
		else if(subdir[0])
			formatstring(pth, "games/%s/%s", data, subdir);
		else
			formatstring(pth, "games/%s", data);

		return pth;
	}
	ICOMMAND(datapath, "s", (const char *s), result(datapath(s)));

	ICOMMAND(include, "s", (const char *pth),
		static string file;
		formatstring(file, "%s/%s.cfg", datapath(), pth);
		rpgexecfile(file);
	)

	struct collision
	{
		rpgent *d, *o;
		vec dir;

		collision(rpgent *de, rpgent *oe, const vec &ddir) : d(de), o(oe), dir(ddir) {}
		collision(rpgent *de, rpgent *oe) : d(de), o(oe) {}

		static void add(physent *d, physent *o, const vec &dir);
		static void add(physent *d, physent *o);
	};
	vector<collision> colcache, mergecache;

	void collision::add(physent *d, physent *o, const vec &dir)
	{
		loopv(colcache)
		{
			if((colcache[i].d == d && colcache[i].o == o) || (colcache[i].d == o && colcache[i].o == d))
				return;
		}

		colcache.add(collision((rpgent *) d, (rpgent *) o, dir));
	}

	void collision::add(physent *d, physent *o)
	{
		loopv(mergecache)
		{
			if(mergecache[i].d == d || mergecache[i].o == d || mergecache[i].d == o || mergecache[i].o == o)
				return;
		}
		mergecache.add(collision((rpgent *) d, (rpgent *) o));
	}

	void dynentcollide(physent *d, physent *o, const vec &dir)
	{
		rpgent *a = (rpgent *) d,
			*b = (rpgent *) o;

		collision::add(a, b, dir);
		if(a->type() == ENT_ITEM && b->type() == ENT_ITEM)
			collision::add(a, b);

	}

	void cleangame()
	{
		gameversion = compatversion = 0;
		friendlyfire = 1;

		colcache.setsize(0);
		mergecache.setsize(0);
		camera::cleanup(true);
		DELETEA(cpmap); cpnum = -1;
		extern void clearhuds();
		clearhuds();

		if(DEBUG_WORLD)
			DEBUGF("resetting map directory");
		setmapdir(NULL);

		if(DEBUG_WORLD)
			DEBUGF("clearing map data; curmap: %p", curmap);

		if(curmap) curmap->objs.removeobj(player1);
		mapdata.clear();
		curmap = NULL;

		if(DEBUG_WORLD)
			DEBUGF(
				"clearing %i scripts, "
				"%i effects, "
				"%i status effects, "
				"%i ammotypes, "
				"%i factions, "
				"%i mapscripts, "
				"%i recipes, "
				"%i merchants, "
				"%i journal buckets, "
				"%i variables, "
				"%i item categories, "
				"and %i tips",
				scripts.length(),
				effects.length(),
				statuses.length(),
				ammotypes.length(),
				factions.length(),
				mapscripts.length(),
				recipes.length(),
				merchants.length(),
				journals.length(),
				variables.length(),
				categories.length(),
				tips.length());

		scripts.clear();
		effects.clear();
		statuses.clear();
		ammotypes.clear();
		factions.clear();
		mapscripts.clear();
		recipes.clear();
		merchants.clear();
		journals.clear();
		variables.clear();
		categories.deletearrays();
		tips.deletearrays();

		//We reset the player here so he has a clean slate on a new game.
		//we do it here so his locals are freed cleanly.
		player1->~rpgchar();
		new (player1) rpgchar();

		rpgscript::clean();

		if(DEBUG_WORLD)
			DEBUGF("Clearing hashpool of %i entries", hashpool.length());
		enumerate(hashpool, const char *, str,
			if(DEBUG_VWORLD) conoutf("freeing hashpool[%s]...", str);
			delete[] str;
		)
		hashpool.clear();
	}

	template<class T>
	static inline void loadassets(const char *dir, T *&var, hashset<T> &objects, bool clear = true)
	{
		if(clear) objects.clear();
		vector<char *> files;
		string file;

		listfiles(dir, "cfg", files);
		loopv(files)
		{
			const char *hash = queryhashpool(files[i]);

			if((var = objects.access(hash)))
			{
				if(clear)
				{
					if(DEBUG_WORLD) DEBUGF("Duplicate instance: \"%s/%s.cfg\" - likely a mod present", dir, files[i]);
					continue;
				}
				var->~T();
				new (var) T();
			}

			if(DEBUG_WORLD)
				DEBUGF("registering hash %s from \"%s/%s.cfg\"", hash, dir, files[i]);

			if(!var) var = &objects[hash];
			var->key = hash;
			formatstring(file, "%s/%s.cfg", dir, files[i]);
			rpgexecfile(file);
		}

		files.deletearrays();
		var = NULL;
	}

	template<class T>
	static inline void loadassets_2pass(const char *dir, T *&var, hashset<T> &objects, bool clear = true)
	{
		if(clear) objects.clear();
		vector<char *> files;
		string file;

		listfiles(dir, "cfg", files);

		if(clear) loopv(files)
		{
			const char *hash = queryhashpool(files[i]);

			if(objects.access(hash))
			{
				if(DEBUG_WORLD) DEBUGF("Duplicate instance: \"%s/%s.cfg\" - likely a mod present", dir, files[i]);
				delete[] files.remove(i--);
				continue;
			}

			if(DEBUG_WORLD)
				DEBUGF("registering hash: %s", hash);

			objects[hash].key = hash;
		}
		else loopvrev(files)
		{
			bool redundant = false;
			loopj(i) if(!strcmp(files[i], files[j])) { redundant = true; break; }
			if(redundant) { delete[] files.remove(i); continue; }

			const char *hash = queryhashpool(files[i]);
			if((var = objects.access(hash)))
			{
				var->~T();
				new (var) T();
				var->key = hash;
			}
			else objects[hash].key = hash;
		}

		loopv(files)
		{
			//the item is already registered...
			const char *hash = files[i];
			var = &objects[hash];
			formatstring(file, "%s/%s.cfg", dir, files[i]);

			if(DEBUG_WORLD)
				DEBUGF("initialising hash %s from \"%s\"", hash, file);

			rpgexecfile(file);
		}

		files.deletearrays();
		var = NULL;
	}

	void loadassets(const char *dir, bool definitions = false, bool clear = true)
	{
		forceverbose++;
		string pth;

		if(!definitions)
		{
			if(DEBUG_WORLD) DEBUGF("loading variables");
			formatstring(pth, "%s/variables.cfg", dir);
			rpgexecfile(pth);
			if(DEBUG_WORLD) DEBUGF("loading tips");
			formatstring(pth, "%s/tips.cfg", dir);
			rpgexecfile(pth);
		}

		//Note that items, characters, triggers, obstacles, containers and platforms are not initialised here.
		//They are templates and are called when the entity is spawned.
		//The one exception is the player, see below.

		#define RANGECHECK(x) \
		if(!(x).length()) \
		{ \
			ERRORF(#x " must contain at least one item"); \
			abort = true; \
		}

		if(DEBUG_WORLD) DEBUGF("loading categories");
		categories.deletearrays();
		formatstring(pth, "%s/categories.cfg", dir);
		rpgexecfile(pth);
		RANGECHECK(categories);

		if(DEBUG_WORLD) DEBUGF("loading merchants");
		formatstring(pth, "%s/merchants", dir);
		loadassets(pth, loadingmerchant, merchants, clear);

		if(DEBUG_WORLD) DEBUGF("loading scripts");
		formatstring(pth, "%s/scripts", dir);
		loadassets(pth, loadingscript, scripts, clear);
		RANGECHECK(scripts);

		if(DEBUG_WORLD) DEBUGF("loading particle effects");
		formatstring(pth, "%s/effects", dir);
		loadassets(pth, loadingeffect, effects, clear);

		if(DEBUG_WORLD) DEBUGF("loading statuses");
		formatstring(pth, "%s/statuses", dir);
		loadassets(pth, loadingstatusgroup, statuses, clear);

		if(DEBUG_WORLD) DEBUGF("loading ammotypes");
		formatstring(pth, "%s/ammo", dir);
		loadassets(pth, loadingammotype, ammotypes, clear);

		if(DEBUG_WORLD) DEBUGF("loading factions");
		formatstring(pth, "%s/factions", dir);
		loadassets_2pass(pth, loadingfaction, factions, clear);
		RANGECHECK(factions);

		if(DEBUG_WORLD) DEBUGF("loading mapscripts");
		formatstring(pth, "%s/mapscripts", dir);
		loadassets(pth, loadingmapscript, mapscripts, clear);
		RANGECHECK(mapscripts);

		if(DEBUG_WORLD) DEBUGF("loading recipes");
		formatstring(pth, "%s/recipes", dir);
		loadassets(pth, loadingrecipe, recipes, clear);

		reserved::load();

		if(!definitions)
		{
			if(DEBUG_WORLD) DEBUGF("loading player");
			player1->init("../player");

			if(!player1->validate())
			{
				ERRORF("Player definition bad, aborting!");
				abort = true;
			}
		}
		forceverbose--;
	}

	ICOMMAND(r_rehash, "", (),
		if(!connected) return;
		conoutf("Reloading configuration files in %s", datapath());
		loadassets(datapath(), true, false);
		if(abort)
		{
			ERRORF("game breaking errors were encountered while parsing files; aborting current game");
			abort = false;
			localdisconnect();
		}
	)

	bool newgame(const char *game, bool restore)
	{
		static bool nonewgame = false;
		if(nonewgame) return false;
		forceverbose++;

		if(!connected)
		{
			if(DEBUG_WORLD)
				DEBUGF("Trying to start game but not 'connected' - connecting and temporarily disabling newgame");
			nonewgame = true;
			localconnect();
			nonewgame = false;
		}
		if(!game || !*game) game = "default";
		if(DEBUG_WORLD)
			DEBUGF("starting new game: %s", game);

		cleangame();
		rpgscript::init();

		copystring(data, game);
		if(DEBUG_WORLD)
			DEBUGF("setting map directory to: %s", datapath("maps"));
		setmapdir(datapath("maps"));

		string dir;
		copystring(dir, datapath());
		loadassets(dir, false);
		if(abort)
		{
			ERRORF("Critical errors were encountered while initialising the game; aborting");
			abort = false; localdisconnect();
			forceverbose--;
			return false;
		}
		emptymap(0, true, NULL, false);
		concatstring(dir, ".cfg");
		if(!rpgexecfile(dir))
			WARNINGF("game properties not defined");
		else if(DEBUG_WORLD)
			DEBUGF("Loaded game properties");

		if(restore) { forceverbose--; return true; }

		player1->health = player1->base.getmaxhp();
		player1->mana = player1->base.getmaxmp();
		player1->getsignal("spawn", false);

		forceverbose--;
		return true;
	}
	ICOMMAND(newgame, "s", (const char *s), newgame(s));

	void initclient()
	{
	}

	void spawnplayer(rpgent *d)
	{
		findplayerspawn(d, -1);
	}

	void respawn()
	{
		if(player1->state==CS_DEAD && !camera::cutscene)
		{
			player1->revive();
			if(cpmap)
			{
				if(strcmp(cpmap, curmap->name))
				{
					mapinfo *dst = accessmap(cpmap);
					dst->loadactions.add(new action_teleport(player1, cpnum, CHECKPOINT));
					load_world(cpmap);
				}
				else
				{
					entities::teleport(player1, cpnum, CHECKPOINT);
				}
			}
		}
	}

	mapinfo *accessmap(const char *name)
	{
		if(!connected)
		{
			ERRORF("Cannot retrieve map information for \"%s\" - no game in progress!", name);
			return NULL;
		}

		mapinfo *ret = mapdata.access(name);
		if(DEBUG_WORLD)
			DEBUGF("searching for map data (%p)", ret);

		if(!ret)
		{
			const char *newname = newstring(name);
			ret = &mapdata[newname];
			ret->name = newname;

			if(DEBUG_WORLD)
				DEBUGF("map data not found - creating (%p)", ret);
		}

		return ret;
	}

	int lasttip = -1;

	void clearmap(mapinfo *map)
	{
		colcache.setsize(0);
		mergecache.setsize(0);
		while(map->objs.ulen)
		{
			rpgent *ent = map->objs.pop();
// 			conoutf("freeing %p", ent);
			rpgscript::removereferences(ent);
			delete ent;
		}

		//map->objs.deletecontents();
		map->projs.deletecontents();
		map->aeffects.deletecontents();
		map->blips.setsize(0);
		map->areatriggers.setsize(0);
	}

	bool firstupdate = false;

	void startmap(const char *name)
	{
		entities::genentlist();
		ai::clearwaypoints();
		camera::cleanup(false);
		firstupdate = true;
		if(!name)
			return;

		forceverbose++;

		ai::loadwaypoints(name);

		if(curmap)
		{
			if(DEBUG_WORLD)
				DEBUGF("removing player from curmap vector");
			curmap->objs.removeobj(player1);
			if(curmap->flags & mapinfo::F_VOLATILE)
			{
				if(DEBUG_WORLD)
					DEBUGF("curmap volatile, clearing before map  change");
				clearmap(curmap);
				curmap->loaded = false;
			}
		}

		mapinfo *lastmap = curmap;
		curmap = accessmap(name);
		rpgscript::changemap();
		entities::initareatriggers();

		if(DEBUG_WORLD)
			DEBUGF("adding player to curmap vector");
		curmap->objs.add(player1);
		if(lastmap) lastmap->getsignal("leave", false);

		if(!curmap->loaded)
		{
			if(DEBUG_WORLD)
				DEBUGF("map not loaded - initialising");

			entities::startmap();
			curmap->getsignal("load", false);
			curmap->loaded = true;
		}
		curmap->getsignal("enter", false);

		if(!transfer)
			spawnplayer(player1);

		transfer = false;

		//loop in reverse to not erase new queued actions - ie persistent scripts
		loopvrev(curmap->loadactions)
		{
			curmap->loadactions[i]->exec();
			delete curmap->loadactions.remove(i);
		}

		if(DEBUG_WORLD)
		{
			enumerate(mapdata, mapinfo, info,
				DEBUGF("map %s", info.name);
				DEBUGF("script %s", info.script->key);
				DEBUGF("flags %i", info.flags);
				DEBUGF("loaded %i", info.loaded);
				DEBUGF("map objects %i", info.objs.length());
				DEBUGF("deferred actions %i", info.loadactions.length());
				DEBUGF("area triggers %i", info.areatriggers.length());
			);
		}

		if(tips.inrange(lasttip))
			conoutf("\f2%s", tips[lasttip]);

		forceverbose--;
	}

	void openworld(const char *name)
	{
		if(!connected) {conoutf("No game in progress"); return;}

		if(name && *name && load_world(name))
			return;

		emptymap(10, true, "untitled");
	}

	void changemap(const char *name)
	{
		if(!connected)
		{
			WARNINGF("No game is in progress, starting default game, this may mean things don't work as intended!");
			newgame(NULL);
		}
		openworld(name);
	}
	COMMANDN(map, changemap, "s");

	void forceedit(const char *name)
	{
		if(!connected)
		{
			WARNINGF("No game is in progress, starting default game, this may mean things don't work as intended!");
			newgame(NULL);
		}
		openworld(name);
	}

	void updateworld()
	{
		rpggui::forcegui();
		if(!connected || !curtime) return;
		if(!curmap)
		{
			openworld(firstmap);
			return;
		}

		physicsframe();
		if(firstupdate)
		{
			if(DEBUG_WORLD) DEBUGF("curmap was recently opened; skipping update so entities don't take a %ims step", curtime);
			firstupdate = false;
			return;
		}
		if(editmode)
		{
			crouchplayer(player1, 10, true);
			moveplayer(player1, 10, false);
			return;
		}
		if(rpggui::open()) return;

		loopv(curmap->objs)
		{
			rpgent *d = curmap->objs[i];
			float eye = d->maxheight;

			d->temp.mdloverride = NULL;
			d->temp.alpha = d->temp.scale = 1;
			d->temp.light = vec4(0, 0, 0, 0);
			d->update();
			loopvj(d->seffects)
			{
				if(!d->seffects[j]->update(d))
				{
					delete d->seffects[j];
					d->seffects.remove(j);
					j--;
				}
			}
			setbbfrommodel(d, d->getmdl(), d->getscale());

			eye = d->maxheight - eye;
			if(eye)
			{
				if(DEBUG_ENT)
					DEBUGF("ent has new maxheight of %f, applying positional delta of %f", d->eyeheight, eye);
				d->o.z += eye;
				d->newpos.z += eye;
			}
		}

		loopv(colcache)
		{
			//note dir is the direction d is from o based on the collision point

			collision &c = colcache[i];
			c.o->getsignal("collide", false, c.d);
			c.d->getsignal("collide", false, c.o);

			//first parse platforms - no discrimination
			rpgplatform *lower = (rpgplatform *) (c.dir.z > 0 ? c.o : c.d);
			rpgent *upper = lower == c.o ? c.d : c.o;

			if(c.dir.z && lower->type() == ENT_PLATFORM && upper->type() != ENT_PLATFORM)
				lower->stack.add(upper);

			//now do pushing - limit to obstacles, NPCs and items
			rpgent *first = c.o, *second = c.d;

			if((first->type() == ENT_CHAR || first->type() == ENT_ITEM || first->type() == ENT_OBSTACLE) &&
				(second->type() == ENT_CHAR || second->type() == ENT_ITEM || second->type() == ENT_OBSTACLE))
			{
				vec vel1, vel2;

				vecfromyawpitch(second->yaw, 0, second->move, second->strafe, vel1);
				vecfromyawpitch(first->yaw, 0, first->move, first->strafe, vel2);

				vel1.mul(second->maxspeed * curtime / 700.f).add(first->vel);
				vel2.mul(first->maxspeed * curtime / 700.f).add(second->vel);

				first->vel = vel1;
				second->vel = vel2;


// 				//TODO factor strength, weight and direction into this
//
// 				vel1.add(vec(first->vel).add(first->falling).mul(.2));;
// 				vel2.add(vec(second->vel).add(second->falling).mul(.2));

				//first->vel.add(vel2);
				//second->vel.add(vel1);
			}
		}
		colcache.setsize(0);

		loopv(mergecache)
		{
			rpgitem *a = (rpgitem *) mergecache[i].d;
			rpgitem *b = (rpgitem *) mergecache[i].o;

// 			conoutf("attempting to merge items %p and %p; %i %i --> %i", a, b, a->quantity, b->quantity, a->quantity + b->quantity);

			if(a->additem(b))
			{
// 				conoutf("Merge successful! %i %i", a->quantity, b->quantity);
				b->quantity = 0;
				rpgscript::obits.add(b);
			}
		}
		mergecache.setsize(0);

		ai::trydrop();

		loopvrev(curmap->projs)
			if(!curmap->projs[i]->update())
				delete curmap->projs.remove(i);

		loopvrev(curmap->aeffects)
			if(!curmap->aeffects[i]->update())
				delete curmap->aeffects.remove(i);

		loopv(curmap->areatriggers)
			curmap->areatriggers[i].update();

// 		curmap->getsignal("update");
		rpgscript::update();
		camera::update();
	}

	void preload()
	{
		///rpgconfig queues models for preloading as they are assigned - so just make sure the default model is loaded.
		preloadmodel(DEFAULTMODEL);
	}

	void suicide(physent *d)
	{
		((rpgent *) d)->die();
	}

	void physicstrigger(physent *d, bool local, int floorlevel, int waterlevel, int material)
	{
		if(d->state!=CS_ALIVE||d->type==ENT_INANIMATE) return;
		switch(material)
		{
			case MAT_LAVA:
				if (waterlevel==0) break;
				playsound(S_BURN, d==player1 ? NULL : &d->o);
				loopi(60)
				{
					vec o = d->o;
					o.z -= d->eyeheight *i/60.0f;
					regular_particle_flame(PART_FLAME, o, 6, 2, 0x903020, 3, 2.0f);
					regular_particle_flame(PART_SMOKE, vec(o.x, o.y, o.z + 8.0f), 6, 2, 0x303020, 1, 4.0f, 100.0f, 2000.0f, -20);
				}
				break;
			case MAT_WATER:
				if (waterlevel==0) break;
				playsound(waterlevel > 0 ? S_SPLASH1 : S_SPLASH2 , d==player1 ? NULL : &d->o);
				particle_splash(PART_WATER, 200, 200, d->o, (watercolour.x<<16) | (watercolour.y<<8) | watercolour.z, 0.5);
				break;
			default:
				if (floorlevel==0) break;
				playsound(floorlevel > 0 ? S_JUMP : S_LAND, local ? NULL : &d->o);
				break;
		}
	}

	///NOTE: dist is a multiplier for the delta between from and to, if there's 10 units difference and you need to travel 5, dist is 0.5
	bool intersect(rpgent *d, const vec &from, const vec &to, float &dist)   // if lineseg hits entity bounding box
	{
		vec bottom(d->feetpos()), top(d->o);
		//crop the bounding box to 1/3rd of its height when dead
		if(d->state == CS_DEAD) top.z -= 2 * (d->aboveeye + d->eyeheight) / 3;
		top.z += d->aboveeye;

		return linecylinderintersect(from, to, bottom, top, d->radius, dist);
	}

	rpgent *intersectclosest(const vec &from, const vec &to, rpgent *at, float &bestdist, float maxdist)
	{
		rpgent *best = NULL;
		bestdist = maxdist;
		float dist;

		loopv(curmap->objs)
		{
			rpgent *o = curmap->objs[i];
			if(o == at || !intersect(o, from, to, dist)) continue;

			if(dist < bestdist)
			{
				best = o;
				bestdist = dist;
			}
		}

		return best;
	}

	bool showenthelpers()
	{
		return editmode;
	}

	#ifdef NO_DEBUG
	VAR(gamespeed, 1, 100, -1);
	#else
	VAR(gamespeed, 10, 100, 1000);
	#endif

	bool ispaused()
	{
		return false;
	}
	int scaletime(int t) { return t * gamespeed; }

	bool canjump()
	{
		respawn();
		return player1->state!=CS_DEAD;
	}

	bool cancrouch()
	{
		return player1->state!=CS_DEAD;
	}

	bool mousemove(int &dx, int &dy, float &cursens)
	{
		if(camera::cutscene)
			return true; //disable mouse input

		return false;
	}

	//we can't actually do anything to prevent newmap invocations.
	void newmap(int size)
	{
		startmap("untitled");
	}

	void gameconnect(bool _remote) { if(!connected) { connected = true; newgame(NULL); } }

	void gamedisconnect(bool cleanup)
	{
		cleangame();
		connected = false;
		if(editmode) toggleedit(true);
	}

	const char *getclientmap() { return curmap ? curmap->name : ""; }
	void edittrigger(const selinfo &sel, int op, int arg1, int arg2, int arg3, const VSlot *vs) {}

	void resetgamestate() {}

	bool cansave()
	{
		if(editmode || curmap->flags & mapinfo::F_NOSAVE || camera::cutscene || rpggui::open() || rpgscript::stack.length() > 1)
			return false;
		return true;
	}

	VARP(edittogglereset, 0, 1, 1);

	void edittoggled(bool on)
	{
		if(!on && curmap)
		{
			if(edittogglereset)
			{
				if(DEBUG_WORLD)
					DEBUGF("resetting game state for current map");

				curmap->objs.removeobj(player1);
				clearmap(curmap);
				curmap->objs.add(player1);
				entities::startmap();
				curmap->getsignal("load");
			}

			entities::genentlist();
			entities::initareatriggers();
		}

	}

	const char *getscreenshotinfo()
	{
		if(connected) return data;
		return "title";
	}


	const char *getmapinfo()
	{
		if(tips.length())
		{
			lasttip = rnd(tips.length());
			return tips[lasttip];
		}
		lasttip = -1;
		return NULL;
	}

	dynent *iterdynents(int i)
	{
		if(curmap)
		{
			if(i < curmap->objs.length())
				return curmap->objs[i];
		}
		else
		{
			if(DEBUG_WORLD)
				DEBUGF("dynent requested while not in a game, returning player");
			return player1;
		}

		return NULL;
	}

	int maxsoundradius(int n)
	{
		switch(n)
		{
			case S_JUMP:
			case S_LAND:
				return 340;
			default:
				return 500;
		}
	}

	int numdynents()
	{
		return curmap ? curmap->objs.length() : 0;
	}

	const char *gameident()		{return "rpg";}
	const char *autoexec()		{return "autoexec.cfg";}
	const char *savedservers()	{return NULL;}

	void writegamedata(vector<uchar> &extras)
	{
		extras.pad(sizeof(int));
		ucharbuf buf(extras.getbuf(), extras.length());

		putint(buf, MAPGAMEVERSION);
	}
	void readgamedata (vector<uchar> &extras)
	{
		mapgameversion = 1;
		if(!extras.length()) return;

		ucharbuf buf(extras.getbuf(), extras.length());
		mapgameversion = getint(buf);
	}

	void loadconfigs() {}
	bool detachcamera() { return player1->state == CS_DEAD; }

	void toserver(char *text) { execute(text); } //since we don't talk, just execute if the / is forgotten
}
