#include "rpggame.h"

namespace entities
{
	hashtable<uint, const char *> modelcache;

	vector<extentity *> ents;
	vector<extentity *> &getents() { return ents; }

	vector<int> intents;

	bool mayattach(extentity &e) { return false; }
	bool attachent(extentity &e, extentity &a) { return false; }
	int extraentinfosize() { return 64; }

	void saveextrainfo(entity &e, char *buf)
	{
		rpgentity *ent = (rpgentity *) &e;
		memcpy(buf, ent->id, 64);
	}

	void loadextrainfo(entity &e, char *buf)
	{
		rpgentity *ent = (rpgentity *) &e;
		memcpy(ent->id, buf, 64);
	}

	void animatemapmodel(const extentity &e, int &anim, int &basetime)
	{
		anim = ANIM_MAPMODEL|ANIM_LOOP;
	}

	extentity *newentity()
	{
		rpgentity *ent = new rpgentity();
		memset(ent->id, 0, 64);
		return ent;
	}
	void deleteentity(extentity *e) { intents.setsize(0); delete (rpgentity *)e; }

	void genentlist() //filters all interactive ents
	{
		loopv(ents)
		{
			const int t = ents[i]->type;
			switch(t)
			{
				case JUMPPAD:
				case CHECKPOINT:
					intents.add(i);
				default:
					break;
			}
		}
	}

	void touchents(rpgent *d)
	{
		loopv(intents)
		{
			extentity &e = *ents[intents[i]];

			switch(e.type)
			{
				case JUMPPAD:
					if(lastmillis - d->lasttouch < 500 || d->o.dist(e.o) > (e.attr[3] ? e.attr[3] : 12)) break;

					d->falling = vec(0, 0, 0);
					d->vel.add(vec(e.attr[2], e.attr[1], e.attr[0]).mul(10));

					d->lasttouch = lastmillis;
					break;

				case CHECKPOINT:
					if(d == game::player1 && d->state == CS_ALIVE && (e.attr[2] ? e.attr[2] : 12) >= d->o.dist(e.o))
						game::setcheckpoint(game::curmap->name, e.attr[1]);

					break;
			}
		}
	}

	void clearents()
	{
		//loopvrev(ents) { delete (rpgentity *) ents[i];}
		//ents.shrink(0);
		ents.deletecontents();
		intents.setsize(0);

		if (DEBUG_WORLD) DEBUGF("Clearing editmode model cache.");
		enumerate(modelcache, const char *, str,
			delete[] str;
		);
		modelcache.clear();
	}

#define DUMMIES \
	x(CRITTER, char) \
	x(ITEM, item) \
	x(OBSTACLE, obstacle) \
	x(CONTAINER, container) \
	x(PLATFORM, platform) \
	x(TRIGGER, trigger)

#define x(enum, type) rpg ## type *dummy ## type = NULL;

	DUMMIES

#undef x

	// WARNING
	// without this, windows builds will for some reason try to initialie these
	// before their dependants: read the hashtables in rpg.cpp.
	// This basically means it crashes.
	void initdummies()
	{
#define x(enum, type) if(! dummy ## type) dummy ## type = new rpg ## type();

		DUMMIES

#undef x
	}

	const char *entmodel(const entity &ent)
	{
		rpgentity &e = *((rpgentity *) &ent);
		if(e.type < CRITTER || e.type > TRIGGER || !e.id[0]) return NULL;
		// 500 million should be enough for anyone
		// we've moved way beyond 640k here!
		uint hash = (e.type - CRITTER) << 29;
		hash |= hthash(e.id) & ((1 << 29) - 1);

		const char **mdl = modelcache.access(hash);
		if(mdl) return *mdl;

		initdummies();

		const char *m = NULL;
		rpgent *dummy = NULL;
		switch(e.type)
		{
#define x(enum, type) 	case enum: dummy = dummy ## type; break;
			DUMMIES

#undef x
		}

		dummy->init(e.id);
		rpgscript::removereferences(dummy);
		dummy->resetmdl();

		if(DEBUG_WORLD) DEBUGF("Registering model \"%s\" in preview cache as %.8X", dummy->temp.mdl, hash);
		m = newstring(dummy->temp.mdl);

#define x(enum, type) \
	case enum: \
		dummy ## type -> ~rpg ## type(); \
		new (dummy ## type) rpg ## type(); \
		break;

		switch(e.type)
		{
			DUMMIES
		}
#undef x
#undef DUMMIES

		return modelcache.access(hash, m);
	}

	ICOMMAND(getentmodel, "ss", (const char *type, const char *id),
		static rpgentity ent;

		int t = 0;
		loopi(MAXENTTYPES) if(!strcmp(type, entname(i)))
		{
			ent.type = t = i; break;
		}
		if(!t) return;

		copystring(ent.id, id, 64);
		result(entmodel(ent));
	)

	ICOMMAND(r_getentlocation, "i", (int *tag),
		vec ret = vec(-1);
		loopv(ents) if(ents[i]->type == LOCATION && ents[i]->attr[0] == *tag)
		{
			ret = ents[i]->o;
			break;
		}
		static string str;
		formatstring(str, "%g %g %g", ret.x, ret.x, ret.z);
		result(str);
	)

	FVARP(entpreviewalpha, 0, .4, 1);

	void renderentities()
	{
		loopv(ents)
		{
			rpgentity &e = *((rpgentity *) ents[i]);
			const char *mdl = entmodel(e);
			if(!mdl || !entpreviewalpha) continue;


			int anim = e.type == CRITTER ? int(ANIM_IDLE) : int(ANIM_MAPMODEL),
				yaw = e.attr[0];

			rendermodel(
				mdl,
				anim|ANIM_LOOP,
				e.o,
				yaw, 0, 0,
				MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED,
				NULL, NULL, 0, 0, 1
			);
		}
	}

	void startmap()
	{
		loopv(ents)
		{
			rpgentity &e = *((rpgentity *) ents[i]);
			switch(e.type)
			{
				case CRITTER:
					spawn(e, e.id, ENT_CHAR, 1);
					break;
				case ITEM:
					spawn(e, e.id, ENT_ITEM, e.attr[1]);
					break;
				case OBSTACLE:
					spawn(e, e.id, ENT_OBSTACLE, 1);
					break;
				case CONTAINER:
					spawn(e, e.id, ENT_CONTAINER, 1);
					break;
				case PLATFORM:
					spawn(e, e.id, ENT_PLATFORM, 1);
					break;
				case TRIGGER:
					spawn(e, e.id, ENT_TRIGGER, 1);
					break;
			}
		}
	}

	void spawn(const extentity &e, const char *id, int type, int qty)
	{
		//don't spawn empty objects
		if(!id || !*id) return;

		rpgent *ent = NULL;
		switch(type)
		{
			case ENT_CHAR:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating creature and instancing to type %s", id);

				ent = new rpgchar();

				break;
			}
			case ENT_ITEM:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating item and instancing to type %s", id);

				rpgitem *d = new rpgitem();
				ent = d;

				d->quantity = max(1, qty);

				break;
			}
			case ENT_OBSTACLE:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating obstacle and instancing to type %s", id);

				ent = new rpgobstacle();

				break;
			}
			case ENT_CONTAINER:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating container and instancing to type %s", id);

				ent = new rpgcontainer();

				break;
			}
			case ENT_PLATFORM:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating platform and instancing to type %s", id);

				ent = new rpgplatform();

				break;
			}
			case ENT_TRIGGER:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating trigger and instancing to type %s", id);

				ent = new rpgtrigger();

				break;
			}
		}

		ent->init(id);
		ent->resetmdl();
		setbbfrommodel(ent, ent->temp.mdl);
		ent->o = e.o;

		if(!ent->validate())
		{
			ERRORF("Entity %p (id: %s) failed validation. It has been deleted to avoid issues", ent, id);
			delete ent; return;
		}

		if(e.type == SPAWN && e.attr[1])
		{
			ent->o.add(vec(rnd(360) * RAD, 0).mul(rnd(e.attr[1])));
			entinmap(ent);
		}
		else
		{
			ent->o.z += ent->eyeheight;
			ent->resetinterp();
		}

		ent->yaw = e.attr[0];

		game::curmap->objs.add(ent);
		ent->getsignal("spawn", false);
	}

	void teleport(rpgent *d, int dest, const int etype)
	{
		loopv(ents)
		{
			if(ents[i]->type == etype && ents[i]->attr[1] == dest)
			{
				d->yaw = ents[i]->attr[0];
				d->pitch = 0;
				d->o = ents[i]->o; entinmap(d, true);
				d->vel = vec(0, 0, 0);
				updatedynentcache(d);
				return;
			}
		}
		ERRORF("no teleport destination %i", dest);
	}

	void entradius(extentity &e, bool &color)
	{
		switch(e.type)
		{
			case TELEDEST:
			{
				vec dir(e.attr[0] * RAD, 0);
				renderentarrow(e, dir, 12);

				break;
			}
			case JUMPPAD:
			{
				vec dir(e.attr[2], e.attr[1], e.attr[0]); dir.normalize();
				renderentarrow(e, dir, e.attr[3] ? e.attr[3] : 12);
				renderentsphere(e, e.attr[3] ? e.attr[3] : 12);

				break;
			}
			case CHECKPOINT:
			{
				vec dir(e.attr[0] * RAD, 0);
				renderentarrow(e, dir, 12);
				renderentsphere(e, e.attr[2] ? e.attr[2] : 12);

				break;
			}
			case CRITTER:
			case ITEM:
			case OBSTACLE:
			case CONTAINER:
			case PLATFORM:
			case TRIGGER:
			{
				vec dir(e.attr[0] * RAD, 0);
				renderentarrow(e, dir, 12);
				break;
			}
			case SPAWN:
			{
				vec dir(e.attr[0] * RAD, 0);
				renderentarrow(e, dir, e.attr[1] ? e.attr[1] : 12);
				renderentring(e, e.attr[1], 0);

				break;
			}
			case LOCATION:
			{
				renderentring(e, e.attr[1], 0);
				break;
			}
			case CAMERA:
			{
				vec dir(e.attr[1] * RAD, e.attr[2] * RAD);
				renderentarrow(e, dir, 12);

				break;
			}
		}
	}

	bool printent(extentity &e, char *buf, int len)
	{
		return false;
	}

	void fixentity(extentity &e)
	{
		switch(e.type)
		{
			case TELEDEST:
			case CHECKPOINT:
			case CRITTER:
			case ITEM:
			case OBSTACLE:
			case CONTAINER:
			case PLATFORM:
			case TRIGGER:
			case SPAWN:
				e.attr.drop();
				e.attr.insert(0, game::player1->yaw);
		}
	}

	void renderhelpertext(extentity &e, vec &pos, char *buf, int len)
	{
		switch(e.type)
		{
			case TELEDEST:
				pos.z += 3.0f;
				nformatstring(buf, len, "Yaw: %i\nTag: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			case JUMPPAD:
				pos.z += 6.0f;
				nformatstring(buf, len, "Z: %i\nY: %i\nX: %i\nRadius: %i",
					e.attr[0],
					e.attr[1],
					e.attr[2],
					e.attr[3]
				);
				break;
			case CHECKPOINT:
				pos.z += 4.5f;
				nformatstring(buf, len, "Yaw: %i\nTag: %i\nRadius: %i",
					e.attr[0], e.attr[1], e.attr[2]
				);
				break;
			case SPAWN:
			{
				pos.z += 4.5f;
				nformatstring(buf, len, "Yaw: %i\nRadius: %i\nTag: %i",
					e.attr[0],
					e.attr[1],
					e.attr[2]
				);
				break;
			}
			case LOCATION:
			{
				pos.z += 3.0f;
				nformatstring(buf, len, "Tag: %i\nRadius: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			}
			case BLIP:
				pos.z += 3.0f;
				nformatstring(buf, len, "Texture?: %i\nTag: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			case CAMERA:
				pos.z += 6.0f;
				nformatstring(buf, len, "Tag: %i\nYaw: %i\nPitch: %i\nRoll: %i",
					e.attr[0],
					e.attr[1],
					e.attr[2],
					e.attr[3]
				);
				break;
			case CRITTER:
			case OBSTACLE:
			case CONTAINER:
			case PLATFORM:
			case TRIGGER:
				pos.z += 1.5f;
				nformatstring(buf, len, "Yaw: %i",
					e.attr[0]
				);
				break;
			case ITEM:
				pos.z += 3.0f;
				nformatstring(buf, len, "Yaw: %i\nQuantity: %i",
					e.attr[0],
					max(1, e.attr[1])
				);
		}
	}

	const char *entname(int i)
	{
		static const char *entnames[] =
		{
			"none?", "light", "mapmodel", "playerstart", "envmap", "particles", "sound", "spotlight",
			"teledest", "jumppad", "checkpoint", "spawn", "location", "reserved", "blip", "camera", "platformroute", "critter", "item", "obstacle", "container", "platform", "trigger"
		};
		return i>=0 && size_t(i)<sizeof(entnames)/sizeof(entnames[0]) ? entnames[i] : "";
	}

	const int numattrs(int type)
	{
		static const int num[] =
		{
			2, //teledest
			4, //jumppad
			3, //checkpoint
			3, //spawn
			2, //location
			0, //reserved
			2, //blip
			4, //camera
			1, //platformroute
			1, //critter
			2, //item
			1, //obstacle
			1, //container
			1, //platform
			1, //trigger
		};

		type -= ET_GAMESPECIFIC;
		return type >= 0 && size_t(type) < sizeof(num)/sizeof(num[0]) ? num[type] : 0;
	}

	const char *entnameinfo(entity &ent)
	{
		rpgentity &e = *((rpgentity *) &ent);
		return e.id;
	}

	int getentyaw(const entity &e)
	{
		switch(e.type)
		{
			case TELEDEST:
			case CHECKPOINT:
			case SPAWN:
			case CRITTER:
			case OBSTACLE:
			case CONTAINER:
			case PLATFORM:
			case TRIGGER:
			case ITEM:
				return e.attr[0];

			case CAMERA:
				return e.attr[1];

			default:
				return 0;
		}
	}

	int getentpitch(const entity &e)
	{
		switch(e.type)
		{
			case CAMERA:
				return e.attr[2];
			default:
				return 0;
		}
	}

	int getentroll(const entity &e)
	{
		switch(e.type)
		{
			case CAMERA:
				return e.attr[3];
			default:
				return 0;
		}
	}

	int getentscale(const entity &e)
	{
		switch(e.type)
		{
			default:
				return 0;
		}
	}

	bool radiusent(extentity &e)
	{
		switch(e.type)
		{
			case LIGHT:
			case ENVMAP:
			case MAPSOUND:
			case JUMPPAD:
			case SPAWN:
			case LOCATION:
				return true;
				break;
			default:
				return false;
				break;
		}
	}

	bool dirent(extentity &e)
	{
		switch(e.type)
		{
			case MAPMODEL:
			case PLAYERSTART:
			case SPOTLIGHT:
			case TELEDEST:
			case CHECKPOINT:
			case JUMPPAD:
			case CRITTER:
			case ITEM:
			case OBSTACLE:
			case CONTAINER:
			case PLATFORM:
			case TRIGGER:
			case SPAWN:
			case CAMERA:
				return true;
				break;
			default:
				return false;
				break;
		}
	}

	//mapmodels to be purely decorative
	void rumble(const extentity &e) {}
	void trigger(extentity &e){}
	void editent(int i, bool local) {}
	void writeent(entity &ent, char *buf)
	{
		rpgentity &e = *((rpgentity *) &ent);
		memcpy(buf, e.id, 64);
	}

	void readent(entity &ent, char *buf, int ver)
	{
		rpgentity &e = *((rpgentity *) &ent);
		if(game::mapgameversion >= 2)
		{
			memcpy(e.id, buf, 64);
		}
		else if(e.type >= CRITTER && e.type <= TRIGGER)
		{
			static char buf[64];
			nformatstring(buf, 64, "%d", e.attr[1]);
			e.attr.remove(1);

			memcpy(e.id, buf, 64);
		}
	}

	float dropheight(entity &e)
	{
		switch(e.type)
		{
			case TRIGGER:
			case CONTAINER:
			case OBSTACLE:
				return 0;
			default:
				return 4.0f;
		}
	}

	ICOMMAND(spawnname, "s", (const char *s),
		if(efocus)
		{
			rpgentity *e = (rpgentity *) ents[efocus];
			copystring(e->id, s, 64);
			return;
		}

		if(enthover >= 0)
		{
			rpgentity *e = (rpgentity *) ents[enthover];
			copystring(e->id, s, 64);
		}
		loopv(entgroup)
		{
			if(enthover == entgroup[i]) continue;
			rpgentity *e = (rpgentity *) ents[entgroup[i]];
			copystring(e->id, s, 64);
		}
	)
}
