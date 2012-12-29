#include "rpggame.h"

namespace game
{
	VARP(debug, 0, 0, DEBUG_MAX);
	VAR(forceverbose, 1, 0, -1);
	rpgchar *player1 = new rpgchar();

	// GAME DEFINITIONS
	vector<script *> scripts; ///scripts, includes dialogue
	vector<effect *> effects; ///pretty particle effects for spells and stuff
	vector<statusgroup *> statuses; ///status effect definitions to transfer onto victims
	vector<faction *> factions;
	vector<ammotype *> ammotypes;
	vector<mapscript *> mapscripts;
	vector<recipe *> recipes;
	vector<merchant *> merchants;

	vector<const char *> categories, tips;
	hashset<rpgvar> variables;
	hashset<journal> journals;

	string data; //data folder
	hashset<mapinfo> *mapdata = NULL;
	mapinfo *curmap = NULL;
	bool connected = false;
	bool transfer = false;
	bool abort = false;

	//important variables/configuration
	SVAR(firstmap, "");
	VAR(gameversion, 0, 0, 0x7FFFFFFF);
	VAR(compatversion, 0, 0, 0x7FFFFFFF);
	//friendlyfire
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
// 		defformatstring(ret)("%i %i", hotkeys[*idx].base, hotkeys[*idx].use);
// 		result(ret);
// 	)
//
// 	ICOMMAND(hotkey_set_pending, "ii", (int *it, int *u),
// 		pending.base = *it;
// 		pending.use = *u;
// 	)
//
// 	ICOMMAND(hotkey_get_pending, "ii", (int *it, int *u),
// 		defformatstring(str)("%i %i", pending.base, pending.use);
// 		result(str);
// 	)
//
// 	ICOMMAND(hotkey_clear, "i", (int *idx),
// 		if(hotkeys.inrange(*idx))
// 			hotkeys[*idx] = equipment(-1, -1);
// 	)

	ICOMMAND(include, "s", (const char *pth),
		static string file;
		formatstring(file)("data/rpg/games/%s/%s.cfg", data, pth);
		execfile(file);
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

		colcache.setsize(0);
		mergecache.setsize(0);
		camera::cleanup(true);

		if(mapdata)
		{
			if(DEBUG_WORLD)
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr clearing map data: %p %p", mapdata, curmap);

			if(curmap)
				curmap->objs.removeobj(player1);

			delete mapdata; mapdata = NULL;
			curmap = NULL;
		}

		if(DEBUG_WORLD)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr clearing %i scripts, %i effects, %i status effects, %i ammotypes, %i mapscripts, %i recipes, %i merchants, %i item categories and %i tips",
				scripts.length(), effects.length(), statuses.length(),
				ammotypes.length(), factions.length(), mapscripts.length(),
				recipes.length(), merchants.length(), categories.length(), tips.length());

		scripts.deletecontents();
		effects.deletecontents();
		statuses.deletecontents();
		ammotypes.deletecontents();
		factions.deletecontents();
		mapscripts.deletecontents();
		recipes.deletecontents();
		merchants.deletecontents();
		categories.deletearrays();
		tips.deletearrays();
		variables.clear();
		journals.clear();

		//We reset the player here so he has a clean slate on a new game.
		player1->~rpgchar();
		new (player1) rpgchar();

		rpgscript::clean();
	}

	template<class T>
	static inline void loadassets(const char *dir, T *&var, vector<T*> &objects)
	{
		objects.deletecontents();
		vector<char *> files;
		string file;

		listfiles(dir, "cfg", files);
		loopv(files)
		{
			char *end;
			int idx = strtol(files[i], &end, 10);
			if(end == files[i] || *end != '\0' || idx < 0)
			{
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr can't load \"%s/%s.cfg\" - invalid index or file name; skipping", dir, files[i]);
				continue;
			}
			if(DEBUG_WORLD)
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr idx = %i, vector has %i; expanding if necessary", idx, objects.length());
			while(!objects.inrange(idx)) objects.add(NULL);

			if(objects[idx])
			{
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr object with index %i already defined in %s; skipping", idx, dir);
				continue;
			}

			objects[idx] = var = new T();

			formatstring(file)("%s/%s.cfg", dir, files[i]);
			if(DEBUG_WORLD) conoutf("\fs\f2DEBUG:\fr executing %s for index %i", file, idx);
			execfile(file);
		}
		files.deletearrays();
		var = NULL;

		loopv(objects)
		{
			if(!objects[i])
			{
				conoutf(CON_ERROR, "\fs\f3ERROR:\fr index %i not defined in %s; problems WILL occur if referenced.", i, dir);
				abort = true;
			}
		}
	}

	void loadassets(const char *dir, bool definitions = false)
	{
		forceverbose++;
		string pth;

		if(!definitions)
		{
			formatstring(pth)("%s/variables.cfg", dir);
			execfile(pth);
			formatstring(pth)("%s/tips.cfg", dir);
			execfile(pth);
		}

		//Note that items, characters, triggers, obstacles, containers and platforms are not initialised here.
		//They are templates and are called when the entity is spawned.
		//The one exception is the player, see below.

		#define RANGECHECK(x) \
		if(!(x).length()) \
		{ \
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr " #x " must contain at least one item"); \
			abort = true; \
		}

		categories.deletearrays();
		formatstring(pth)("%s/categories.cfg", dir);
		execfile(pth);
		RANGECHECK(categories);
		formatstring(pth)("%s/merchants", dir);
		loadassets(pth, loadingmerchant, merchants);
		formatstring(pth)("%s/scripts", dir);
		loadassets(pth, loadingscript, scripts);
		RANGECHECK(scripts);
		formatstring(pth)("%s/effects", dir);
		loadassets(pth, loadingeffect, effects);
		formatstring(pth)("%s/statuses", dir);
		loadassets(pth, loadingstatusgroup, statuses);
		formatstring(pth)("%s/ammo", dir);
		loadassets(pth, loadingammotype, ammotypes);
		formatstring(pth)("%s/factions", dir);
		loadassets(pth, loadingfaction, factions);
		RANGECHECK(factions);
		formatstring(pth)("%s/mapscripts", dir);
		loadassets(pth, loadingmapscript, mapscripts);
		RANGECHECK(mapscripts);
		formatstring(pth)("%s/recipes", dir);
		loadassets(pth, loadingrecipe, recipes);


		if(!definitions)
		{
			loadingrpgchar = player1;
			formatstring(pth)("%s/player.cfg", dir);
			execfile(pth);
			loadingrpgchar = NULL;
		}
		forceverbose--;
	}

	ICOMMAND(r_rehash, "", (),
		if(!mapdata) return;
		conoutf("Reloading configuration files in data/rpg/games/%s", data);
		defformatstring(dir)("data/rpg/games/%s", data);
		loadassets(dir, true);
		if(abort)
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr game breaking errors were encountered while parsing files; aborting current game");
			abort = false;
			localdisconnect();
		}
	)

	void newgame(const char *game, bool restore)
	{
		static bool nonewgame = false;
		if(nonewgame) return;
		forceverbose++;

		if(!connected)
		{
			if(DEBUG_WORLD)
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr Trying to start game but not 'connected' - connecting and temporarily disabling newgame");
			nonewgame = true;
			localconnect();
			nonewgame = false;
		}
		if(!game || !*game) game = "default";
		if(DEBUG_WORLD)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr starting new game: %s", game);

		cleangame();
		rpgscript::init();

		mapdata = new hashset<mapinfo>();
		copystring(data, game);

		defformatstring(dir)("data/rpg/games/%s", game);
		loadassets(dir, false);
		if(abort)
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr Critical errors were encountered while initialising the game; aborting");
			abort = false; localdisconnect();
			forceverbose--;
			return;
		}
		emptymap(0, true, NULL, false);
		concatstring(dir, ".cfg");
		if(!execfile(dir))
			conoutf(CON_WARN, "\fs\f6WARNING:\fr game properties not defined");
		else if(DEBUG_WORLD)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr Loaded game properties");

		player1->resetmdl();
		if(restore) { forceverbose--; return; }

		player1->health = player1->base.getmaxhp();
		player1->mana = player1->base.getmaxmp();
		player1->getsignal("spawn", false);

		forceverbose--;
	}
	ICOMMAND(newgame, "s", (const char *s), newgame(s));

	void initclient()
	{
	}

	void spawnplayer(rpgent *d)
	{
		findplayerspawn(d, -1);
	}

	void respawnself(bool death = false)
	{
		player1->revive();
	}

	void respawn()
	{
		if(player1->state==CS_DEAD)
		{
			respawnself(true);
		}
	}

	mapinfo *accessmap(const char *name)
	{
		if(!mapdata)
		{
			conoutf(CON_ERROR, "\fs\f2ERROR:\fr no map data; cannot access data for \"%s\"", name);
			return NULL;
		}

		mapinfo *dummy = mapdata->access(name);
		if(DEBUG_WORLD)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr finding map data (%p)", dummy);

		if(!dummy)
		{
			const char *newname = newstring(name);
			dummy = &mapdata->access(newname, mapinfo());
			dummy->name = newname;

			if(DEBUG_WORLD)
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr map data not found - creating (%p)", dummy);
		}

		return dummy;
	}

	int lasttip = -1;

	void clearmap(mapinfo *map)
	{
		colcache.setsize(0);
		mergecache.setsize(0);
		loopvrev(map->objs)
		{
			rpgent *ent = map->objs[i];
// 			conoutf("freeing %p", ent);
			rpgscript::removereferences(ent);
			delete ent;
		}

		map->objs.deletecontents();
		map->projs.deletecontents();
		map->aeffects.deletecontents();
		map->blips.setsize(0);
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
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr removing player from curmap vector");
			curmap->objs.removeobj(player1);
			if(curmap->flags & mapinfo::F_VOLATILE)
			{
				if(DEBUG_WORLD)
					conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr curmap volatile, clearing before map  change");
				clearmap(curmap);
				curmap->loaded = false;
			}
		}

		curmap = accessmap(name);
		rpgscript::changemap();

		if(DEBUG_WORLD)
			conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr adding player to curmap vector");
		curmap->objs.add(player1);

		if(!curmap->loaded)
		{
			if(DEBUG_WORLD)
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr map not loaded - initialising");

			entities::startmap();
			curmap->getsignal("load", false);
			curmap->loaded = true;
		}

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
			enumerate(*mapdata, mapinfo, info,
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr map %s", info.name);
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr map objects %i", info.objs.length());
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr loaded %i", info.loaded);
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr deferred actions %i", info.loadactions.length());
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
			conoutf(CON_WARN, "WARNING: No game is in progress, starting default game, this may mean things don't work as intended!");
			newgame(NULL);
		}
		openworld(name);
	}
	COMMANDN(map, changemap, "s");

	void forceedit(const char *name)
	{
		if(!connected)
		{
			conoutf(CON_WARN, "WARNING: No game is in progress, starting default game, this may mean things don't work as intended!");
			newgame(NULL);
		}
		openworld(name);
	}

	void updateworld()
	{
		rpggui::forcegui();
		if(!mapdata || !curtime) return;
		if(!curmap)
		{
			openworld(firstmap);
			return;
		}

		physicsframe();
		if(firstupdate)
		{
			if(DEBUG_WORLD) conoutf("\fs\f2DEBUG:\fr curmap was recently opened; skipping update so entities don't take a %ims step", curtime);
			firstupdate = false;
			return;
		}
		if(editmode)
		{
			moveplayer(player1, 10, false);
			return;
		}

		loopv(curmap->objs)
		{
			rpgent *d = curmap->objs[i];
			float eye = d->eyeheight;

			d->resetmdl();
			d->temp.alpha = 1;
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
			setbbfrommodel(d, d->temp.mdl);

			eye = d->eyeheight - eye;
			if(eye)
			{
				if(DEBUG_ENT)
					conoutf("\fs\f2DEBUG:\fr ent has new eyeheight of %f, applying positional delta of %f", d->eyeheight, eye);
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

		curmap->getsignal("update");
		rpgscript::update();
		camera::update();
	}

	void preload()
	{
		///rpgconfig queues models for preloading as they are assigned - so just make sure the defualt model is loaded.
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
				particle_splash(PART_WATER, 200, 200, d->o, (watercolor.x<<16) | (watercolor.y<<8) | watercolor.z, 0.5);
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

	#ifndef NO_DEBUG
	VAR(gamespeed, 1, 100, -1);
	#else
	VAR(gamespeed, 10, 100, 1000);
	#endif

	bool ispaused()
	{
		if(rpggui::open()) return true;
		return false;
	}
	int scaletime(int t) { return t * gamespeed; }

	bool canjump()
	{
		respawn();
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

	void gameconnect(bool _remote) { connected = true; if(!mapdata) newgame(NULL); }

	void gamedisconnect(bool cleanup)
	{
		cleangame();
		connected = false;
	}

	const char *getclientmap() { return curmap ? curmap->name : ""; }
	void edittrigger(const selinfo &sel, int op, int arg1, int arg2, int arg3) {}

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
		entities::intents.setsize(0);
		if(!on && curmap)
		{
			if(edittogglereset)
			{
				if(DEBUG_WORLD)
					conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr resetting game state for current map");

				curmap->objs.removeobj(player1);
				clearmap(curmap);
				curmap->objs.add(player1);
				entities::startmap();
				curmap->getsignal("load");
			}

			entities::genentlist();
		}

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
				conoutf(CON_DEBUG, "\fs\f2DEBUG:\fr dynent requested while not in a game, returning player");
			return player1;
		}

		return NULL;
	}

	int numdynents()
	{
		return curmap ? curmap->objs.length() : 0;
	}

	const char *gameident()		{return "rpg";}
	const char *autoexec()		{return "autoexec.cfg";}
	const char *savedservers()	{return NULL;}

	void writegamedata(vector<char> &extras) {}
	void readgamedata (vector<char> &extras) {}

	void writemapdata(stream *f) {} //do we save rpg declarations per map or not?
	void loadconfigs() {}
	bool detachcamera() { return player1->state == CS_DEAD; }
	void toserver(char *text) { execute(text); } //since we don't talk, just execute if the / is forgotten
}
