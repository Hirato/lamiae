#include "rpggame.h"

namespace entities
{
	hashtable<uint, const char *> modelcache;

	rpgchar      *dummychar      = new rpgchar     ();
	rpgitem      *dummyitem      = new rpgitem     ();
	rpgobstacle  *dummyobstacle  = new rpgobstacle ();
	rpgcontainer *dummycontainer = new rpgcontainer();
	rpgplatform  *dummyplatform  = new rpgplatform ();
	rpgtrigger   *dummytrigger   = new rpgtrigger  ();

	vector<extentity *> ents;
	vector<extentity *> &getents() { return ents; }

	vector<int> intents;

	bool mayattach(extentity &e) { return false; }
	bool attachent(extentity &e, extentity &a) { return false; }
	int extraentinfosize() {return 0;}

	void animatemapmodel(const extentity &e, int &anim, int &basetime)
	{
		anim = ANIM_MAPMODEL|ANIM_LOOP;
	}

	extentity *newentity() { return new rpgentity(); }
	void deleteentity(extentity *e) { intents.setsize(0); delete (rpgentity *)e; }

	void genentlist() //filters all interactive ents
	{
		loopv(ents)
		{
			const int t = ents[i]->type;
			if(t == JUMPPAD)
				intents.add(i);
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

	const char *entmodel(const entity &e)
	{
		if(e.type < CRITTER || e.type > TRIGGER) return NULL;
		// 500 million should be enough for anyone
		// we've moved way beyond 640k here!
		uint hash = (e.type - CRITTER) << 29;
		hash |= e.attr[1] & ((1 << 29) - 1);

		const char **mdl = modelcache.access(hash);
		if(mdl) return *mdl;


		const char *m = NULL;
		rpgent *dummy = NULL;
		switch(e.type)
		{
			case CRITTER:   dummy = dummychar;      break;
			case ITEM:      dummy = dummyitem;      break;
			case OBSTACLE:  dummy = dummyobstacle;  break;
			case CONTAINER: dummy = dummycontainer; break;
			case PLATFORM:  dummy = dummyplatform;  break;
			case TRIGGER:   dummy = dummytrigger;   break;
		}

		dummy->init(e.attr[1]);
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
			x(CRITTER,   char     );
			x(ITEM,      item     );
			x(OBSTACLE,  obstacle );
			x(CONTAINER, container);
			x(PLATFORM,  platform );
			x(TRIGGER,   trigger  );
		}
#undef x

		return modelcache.access(hash, m);
	}

	FVARP(entpreviewalpha, 0, .4, 1);

	void renderentities()
	{
		loopv(ents)
		{
			extentity &e = *ents[i];
			const char *mdl = entmodel(e);

			if(mdl && entpreviewalpha)
				rendermodel(
					mdl,
					(e.type == CRITTER ? ANIM_IDLE : ANIM_MAPMODEL)|ANIM_LOOP,
					e.o,
					e.attr[0] + 90, 0, 0,
					MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED,
					NULL, NULL, 0, 0, 1
				);
		}
	}

	void startmap()
	{
		loopv(ents)
		{
			extentity &e = *ents[i];
			switch(e.type)
			{
				case CRITTER:
					spawn(e, e.attr[1], ENT_CHAR, 1);
					break;
				case ITEM:
					spawn(e, e.attr[1], ENT_ITEM, e.attr[2]);
					break;
				case OBSTACLE:
					spawn(e, e.attr[1], ENT_OBSTACLE, 1);
					break;
				case CONTAINER:
					spawn(e, e.attr[1], ENT_CONTAINER, 1);
					break;
				case PLATFORM:
					spawn(e, e.attr[1], ENT_PLATFORM, 1);
					break;
				case TRIGGER:
					spawn(e, e.attr[1], ENT_TRIGGER, 1);
					break;
			}
		}
	}

	void spawn(const extentity &e, int ind, int type, int qty)
	{
		rpgent *ent = NULL;
		switch(type)
		{
			case ENT_CHAR:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating creature and instancing to type %i", ind);

				ent = new rpgchar();

				break;
			}
			case ENT_ITEM:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating item and instancing to type %i", ind);

				rpgitem *d = new rpgitem();
				ent = d;

				d->quantity = max(1, qty);

				break;
			}
			case ENT_OBSTACLE:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating obstacle and instancing to type %i", ind);

				ent = new rpgobstacle();

				break;
			}
			case ENT_CONTAINER:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating container and instancing to type %i", ind);

				ent = new rpgcontainer();

				break;
			}
			case ENT_PLATFORM:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating platform and instancing to type %i", ind);

				ent = new rpgplatform();

				break;
			}
			case ENT_TRIGGER:
			{
				if(DEBUG_ENT)
					DEBUGF("Creating trigger and instancing to type %i", ind);

				ent = new rpgtrigger();

				break;
			}
		}

		ent->init(ind);
		ent->resetmdl();
		setbbfrommodel(ent, ent->temp.mdl);
		ent->o = e.o;

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

	void teleport(rpgent *d, int dest)
	{
		loopv(ents)
		{
			if(ents[i]->type == TELEDEST && ents[i]->attr[1] == dest)
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
				renderentsphere(e, e.attr[3] ? e.attr[3] : 12);

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

	bool printent(extentity &e, char *buf)
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
				e.attr.pop();
				e.attr.insert(0, game::player1->yaw);
		}
	}

	void renderhelpertext(extentity &e, int &colour, vec &pos, string &tmp)
	{
		switch(e.type)
		{
			case TELEDEST:
				pos.z += 3.0f;
				formatstring(tmp)("Yaw: %i\nTag: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			case JUMPPAD:
				pos.z += 6.0f;
				formatstring(tmp)("Z: %i\nY: %i\nX: %i\nRadius: %i",
					e.attr[0],
					e.attr[1],
					e.attr[2],
					e.attr[3]
				);
				break;
			case CHECKPOINT:
				pos.z += 1.5f;
				formatstring(tmp)("Yaw: %i",
					e.attr[0]
				);
				break;
			case SPAWN:
			{
				pos.z += 4.5f;
				formatstring(tmp)("Yaw: %i\nRadius: %i\nTag: %i",
					e.attr[0],
					e.attr[1],
					e.attr[2]
				);
				break;
			}
			case LOCATION:
			{
				pos.z += 3.0f;
				formatstring(tmp)("Tag: %i\nRadius: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			}
			case BLIP:
				pos.z += 3.0f;
				formatstring(tmp)("Texture?: %i\nTag: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			case CAMERA:
				pos.z += 6.0f;
				formatstring(tmp)("Tag: %i\nYaw: %i\nPitch: %i\nRoll: %i",
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
				pos.z += 3.0f;
				formatstring(tmp)("Yaw: %i\nIndex: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			case ITEM:
				pos.z += 4.5f;
				formatstring(tmp)("Yaw: %i\nIndex: %i\nQuantity: %i",
					e.attr[0],
					e.attr[1],
					max(1, e.attr[2])
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
			1, //checkpoint
			3, //spawn
			2, //location
			0, //reserved
			2, //blip
			4, //camera
			1, //platformroute
			2, //critter
			3, //item
			2, //obstacle
			2, //container
			2, //platform
			2, //trigger
		};

		type -= ET_GAMESPECIFIC;
		return type >= 0 && size_t(type) < sizeof(num)/sizeof(num[0]) ? num[type] : 0;
	}

	const char *entnameinfo(entity &e)
	{
		return "";
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

	int *getmodelattr(extentity &e)
	{
		return NULL;
	}

	bool checkmodelusage(extentity &e, int i)
	{
		return false;
	}

	//mapmodels to be purely decorative
	void rumble(const extentity &e) {}
	void trigger(extentity &e){}
	void editent(int i, bool local) {}
	void writeent(entity &e, char *buf) {}

	void readent(entity &e, char *buf, int ver)
	{
		if(ver <= 30) switch(e.type)
		{
			case TELEDEST:
			case SPAWN:
				e.attr[0] = (int(e.attr[0])+180)%360;
				break;
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
}
