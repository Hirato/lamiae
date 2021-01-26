#include "rpggame.h"

/**
	us V them - the differences between us and the upstream sauerbraten FPS
	1) we don't use 0 to denote a dummy waypoint
	2) numlinks is not limited, but we enforce a soft limit of 255 (them; 6)
*/

extern int maxparticledistance;

namespace ai
{
	vector<waypoint> waypoints;

	waypoint *prev = NULL;
	VARF(dropwaypoints, 0, 0, 1, prev = NULL);

	void clearwaypoints()
	{
		waypoints.setsize(0);
		prev = NULL;
	}
	COMMAND(clearwaypoints, "");

	waypoint *closestwaypoint(const vec &o)
	{
		waypoint *result = NULL;
		float dist = 1e16;
		loopv(waypoints)
		{
			float newdist = waypoints[i].o.dist(o);
			if(newdist < dist)
			{
				result = &waypoints[i];
				dist = newdist;
			}
		}

		return result;
	}

	//note, single direction link.
	void linkwaypoint(waypoint &from, waypoint &to)
	{
		if(&from == &to) return; //don't allow node to be linked to itself
		ushort id = &to - waypoints.getbuf();
		if(from.links.find(id) == -1)
		{
			ushort fromid = &from - waypoints.getbuf();
			from.links.add(id);
			to.revlinks.add(fromid);
		}
	}

	void unlinkwaypoint(waypoint &from, waypoint &to)
	{
		ushort fromid = &from - waypoints.getbuf();
		ushort toid = &to - waypoints.getbuf();

		from.links.removeobj(toid);
		to.revlinks.removeobj(fromid);
	}

	VAR(waypointstairheight, 0, 8, 32);
	FVAR(waypointmergedist, 1, 8, 128);
	FVAR(waypointlinkmaxdist, 1, 32, 128);
	VAR(waypointmergepasses, 1, 4, 128);
	VAR(waypointmaxlinks, 3, 10, 255);


	void dropwaypoint(const vec &o)
	{
		int p = prev ? prev - waypoints.getbuf() : -1;
		waypoint &wp = waypoints.add(waypoint(o));
		prev = &wp;

		int wmat = lookupmaterial(wp.o);
		bool wpair = !isliquid(wmat) && raycube(wp.o, vec(0, 0, -waypointstairheight), 0, RAY_POLY|RAY_CLIPMAT) >= 1;
		// - 1; do not test last one
		for(int i = 0; i < waypoints.length() - 1; i++)
		{
			waypoint &ow = waypoints[i];
			if(ow.o.dist(wp.o) > (waypointlinkmaxdist - 2)) continue;
			int omat = lookupmaterial(ow.o);
			bool owair = !isliquid(omat) && raycube(ow.o, vec(0, 0, -waypointstairheight), 0, RAY_POLY|RAY_CLIPMAT) >= 1;

			if(wpair && owair)
			{
				if(i == p)
					linkwaypoint(ow, wp);
			}
			else
			{
				vec from, ray;
				if(!wpair && !owair && wp.o.z != ow.o.z)
				{
					if(wp.o.z > ow.o.z)
					{
						from = wp.o;
						raycubepos(ow.o, vec(0, 0, 1), ray, wp.o.z - ow.o.z, RAY_POLY|RAY_CLIPMAT);
						ray.sub(from);
					}
					else
					{
						raycubepos(wp.o, vec(0, 0, 1), from, ow.o.z - wp.o.z, RAY_POLY|RAY_CLIPMAT);
						ray = vec(ow.o).sub(from);
					}
				}
				else
				{
					from = wp.o;
					ray = vec(ow.o).sub(wp.o);
				}

				if(raycube(from, ray, 0, RAY_POLY|RAY_CLIPMAT) >= 1)
				{
					linkwaypoint(wp, ow);
					linkwaypoint(ow, wp);
				}
			}
		}
	}


	void trydrop()
	{
		if(!dropwaypoints || editmode || game::player1->state == CS_DEAD)
			return;

		waypoint *closest = closestwaypoint(game::player1->feetpos());

		float dist = 1e16;
		loopv(waypoints)
		{
			float dst = waypoints[i].o.dist(game::player1->feetpos());
			if(dst < dist) dist = dst;
		}

		if(dist >= (waypointlinkmaxdist / 2))
		{
			dropwaypoint(game::player1->feetpos());
		}
		else if (prev != closest)
		{
			//single direction link as fallback
			if(prev) linkwaypoint(*prev, *closest);
			prev = closest;
		}
	}

	void removewaypoint(int ind)
	{
		if(!waypoints.inrange(ind))
		{
			ERRORF("Waypoint %i not inrange", ind);
			return;
		}

		loopv(waypoints)
		{
			waypoint &w = waypoints[i];

			loopvj(w.links)
			{
				if(w.links[j] == ind)
				{
					w.links.remove(j);
					j--;
				}
				else if(w.links[j] > ind)
					w.links[j]--;
			}
			loopvj(w.revlinks)
			{
				if(w.revlinks[j] == ind)
					w.revlinks.remove(j--);
				else if(w.revlinks[j] > ind)
					w.revlinks[j]--;
			}
		}

		waypoints.remove(ind);
	}

	ICOMMAND(removewaypoint, "i", (int *ind), removewaypoint(*ind))

	void removelink(int a, int b)
	{
		if(!waypoints.inrange(a) || !waypoints.inrange(b))
		{
			ERRORF("%i or %i is not inrange", a, b);
			return;
		}

		waypoint &first = waypoints[a], &second = waypoints[b];
		unlinkwaypoint(first, second);
	}

	ICOMMAND(removewaypointlink, "ii", (int *a, int *b), removelink(*a, *b))

	void mergewaypoints(int first, int second)
	{
		waypoint &f = waypoints[first];
		waypoint &s = waypoints[second];

		//assume the secondary's links'
		loopv(s.links)
		{
			if(f.links.find(s.links[i]) == -1)
				linkwaypoint(f, waypoints[s.links[i]]);
		}
		//assume the secondary's reverse links'
		loopv(s.revlinks)
		{
			if(f.revlinks.find(s.revlinks[i]) == -1)
				linkwaypoint(waypoints[s.revlinks[i]], f);
		}
		//average their distances
		f.o.add(s.o).div(2.0f);

		removewaypoint(second);
	}

	static inline bool sortlinks(ushort a, ushort b)
	{
		waypoint &first  = waypoints[a],
		&second = waypoints[b];

		return first.score < second.score;
	}

	void optimisewaypoints()
	{
		loopi(waypointmergepasses)
		{
			loopvj(waypoints)
			{
				loopvk(waypoints)
				{
					if(j == k)
						continue;

					waypoint &f = waypoints[j];
					waypoint &s = waypoints[k];
					if(f.o.dist(s.o) <= waypointmergedist)
					{
						conoutf("merging points %i and %i", j, k);
						mergewaypoints(j, k);
						if (j > k--) j--;
					}
				}
				if(waypoints[j].links.length() > waypointmaxlinks)
				{
					//we remove the longest ones above the limit.
					//this requires sorting first
					waypoint &point = waypoints[j];
					loopvk(point.links)
					{
						waypoint &link = waypoints[point.links[k]];
						link.score = link.o.dist(point.o);
					}

					point.links.sort(sortlinks);

					while(point.links.length() > waypointmaxlinks)
					{
						waypoint &tmp = waypoints[point.links.last()];
						conoutf("removing link from %i to %i with distance %i", i, point.links.last(), tmp.score);
						unlinkwaypoint(point, tmp);
					}
				}
			}
		}

		loopv(waypoints)
		{
			if(!waypoints[i].links.length())
			{
				conoutf("removing node %i (no links!)", i);
				removewaypoint(i);
				i--;
			}
			loopvj(waypoints[i].links)
			{
				waypoint &w = waypoints[i],
				         &l = waypoints[waypoints[i].links[j]];

				if(w.links[j] == i || w.o.dist(l.o) > waypointlinkmaxdist)
				{
					conoutf("removing link from %i to %i (too far, or to self)", i, waypoints[i].links[j]);
					unlinkwaypoint(w, l);
					j--;
					continue;
				}
			}
		}
	}
	COMMAND(optimisewaypoints, "");

	void loadwaypoints(const char *name, bool msg)
	{
		if(name && *name)
			getmapfilenames(name);

		defformatstring(wptname, "%s/%s.wpt", mpath, mname);
		path(wptname);

		stream *f = opengzfile(wptname, "rb");

		if(!f)
		{
			if(msg) ERRORF("failed to load waypoints");
			return;
		}

		char magic[4];
		if(f->read(magic, 4) < 4 || memcmp(magic, "OWPT", 4))
		{
			ERRORF("magic mismatch (expected OWPT, got %4.4s)", magic);
			delete f;
			return;
		}

		clearwaypoints();

		ushort numwp = f->getlil<ushort>();
		waypoints.reserve(numwp); //allocate them ahead of time
		loopi(numwp) waypoints.add(); //need to call their constructors too

		loopi(numwp)
		{
			waypoint &w = waypoints[i];

			w.o.x = f->getlil<float>();
			w.o.y = f->getlil<float>();
			w.o.z = f->getlil<float>();
			int numlinks = f->getchar();
			if(numlinks > 10)
				WARNINGF("waypoint #%i has a large number of links! (%i)", i, numlinks);

			loopj(numlinks)
			{
				ushort wp = f->getlil<ushort>() - 1; //FPS game uses 0 to denote dummy waypoint, and use 1+ to denote real waypoints - we don't
				w.links.add(wp);
				waypoints[wp].revlinks.add(i);
			}
		}

		conoutf("successfully loaded %i waypoints", waypoints.length());

		delete f;
	}
	ICOMMAND(loadwaypoints, "s", (const char *s), loadwaypoints(s, true));

	void savewaypoints(const char *name)
	{
		if(waypoints.length() <= 1)
			return;

		if(name && *name)
			getmapfilenames(name);

		defformatstring(wptname, "%s/%s.wpt", mpath, mname);
		path(wptname);

		stream *f = opengzfile(wptname, "wb");
		if(!f)
		{
			ERRORF("failed to save waypoints");
			return;
		}

		f->write("OWPT", 4);
		f->putlil<ushort>(waypoints.length());

		for(int i = 0; i < waypoints.length(); i++)
		{
			waypoint &w = waypoints[i];

			f->putlil<float>(w.o.x);
			f->putlil<float>(w.o.y);
			f->putlil<float>(w.o.z);

			f->putchar(w.links.length());
			loopvj(w.links)
				f->putlil<ushort>(w.links[j] + 1); //FPS game uses 0 to denote dummy waypoint, and use 1+ to denote real waypoints - we don't
		}

		conoutf("successfully saved %i waypoints", waypoints.length());
		delete f;
	}
	COMMAND(savewaypoints, "s");

	//this is an attempt at an A-STAR search algorithm
	//note that waypoints are considered equidistant; this is sort of true

	int topscore = UINT16_MAX;

	void findroute(int from, int to, vector<ushort> &route)
	{
		if(!waypoints.length() || !waypoints.inrange(from) || !waypoints.inrange(to))
			return;

		if(from == to)
		{
			route.add(to);
			return;
		}

		//first, we reset the statuses of the nodes
		loopv(waypoints)
		{
			waypoints[i].score = UINT16_MAX;
			waypoints[i].parent = NULL;
		}


		//excessive, but should be plenty
		static queue<ushort, 2048> wpqueue;

		waypoints[to].score = 0;
		wpqueue.add(to);

		//We're doing a breadth first search
		//Our target starts with a score of 0, and everything linked to that gets a score of 1; everything linked to those gets a score of 2, etc
		//waypoints are roughly equidistant, so the score ends up being a number of steps that need to be taken to reach the target
		//This gets us a very clear and direct route
		//It also allows us to easily influence the pathfinding to favour less efficient routes.
		// i.e. we're linked to a teleporter (TODO),
		//also allows considering obstacles like projectiles (TODO),
		//and area effects (TODO),
		//and even traffic (TODO)
		while(wpqueue.length())
		{
			waypoint *wpto = &waypoints[wpqueue.remove()];
			ushort score = wpto->score + 1;
			loopv(wpto->revlinks)
			{
				waypoint *wpfrom = &waypoints[wpto->revlinks[i]];
				if(wpfrom->score > score)
				{
					wpfrom->score = score;
					wpfrom->parent = wpto;
					wpqueue.add(wpfrom - waypoints.getbuf());
				}
			}
		}

		//build the route; we go from start to end, travelling to each "parent", this is in order
		vector<ushort> subroute;
		waypoint *rt = &waypoints[from];
		topscore = rt->score;
		while(rt)
		{
			subroute.add(rt - waypoints.getbuf());
			rt = rt->parent;
		}

		//we actually want it in reverse order for our route; the top most node should be the next node we wish to visit
		//the route may already contain other pathfinding, which we don't want to touch.'
		loopvrev(subroute)
			route.add(subroute[i]);

		return;
	}

	#ifndef NO_DEBUG
	vector<ushort> testroute;
	ICOMMAND(waypointtest, "V", (tagval *args, int num),
		testroute.setsize(0);
		loopirev(num - 1)
		{
			int f = args[i].getint(); int t = args[i + 1].getint();
			conoutf("finding route between from %i to %i", f, t);
			findroute(f, t, testroute);
		}
	)
	#endif

	VARP(waypointdrawdist, 64, 512, 4096);
	VARP(waypointlabeldist, 64, 128, 1024);

	void renderwaypoints()
	{
		loopv(waypoints)
		{
			waypoint &w = waypoints[i];
			float dist = w.o.dist(game::player1->o);
			if(dist > min(waypointdrawdist, maxparticledistance)) continue;

			if(dist < waypointlabeldist)
			{
				defformatstring(ds, "%i\n%i", i, w.links.length());
				particle_textcopy(vec(0, 0, 6).add(w.o), ds, PART_TEXT, 1, 0xFF00FF, 3);
			}

			loopvj(w.links)
			{
				waypoint &l = waypoints[w.links[j]];
				int colour = 0x00007F;
				if(DEBUG_VAI) //will probably induce epilepsy
				{
					ushort score = min(127, 127 * max(w.score, l.score) / topscore);
					colour = (score) << 16 | (127 - score) << 8;
				}
				particle_flare(w.o, l.o, 0, PART_STREAK, colour, .5);
			}
		}

		if(dropwaypoints && prev)
			regular_particle_splash(PART_EDIT, 2, 100, prev->o, 0xFF0000, .5);

		#ifndef NO_DEBUG
		if(!DEBUG_AI) return;

		for(int i = 1; i < testroute.length(); i++)
		{
			waypoint &cur  = waypoints[testroute[i-1]];
			waypoint &prev = waypoints[testroute[i  ]];
			if(cur.o.dist(game::player1->o) > min(waypointdrawdist, maxparticledistance) || prev.o.dist(game::player1->o) > min(waypointdrawdist, maxparticledistance))
				continue;

			particle_flare(vec(0, 0, 1).add(prev.o), vec(0, 0, 1).add(cur.o), 0, PART_STREAK, DEBUG_VAI ? 0x0000FF : 0xFF0000, .75);
		}
		loopv(game::curmap->objs)
		{
			rpgchar *ent = (rpgchar *) game::curmap->objs[i];
			if(ent->type() != ENT_CHAR || !ent->route.length())
				continue;

			for(int j = 1; j < ent->route.length(); j++)
			{
				waypoint &cur  = waypoints[ent->route[j - 1]];
				waypoint &prev = waypoints[ent->route[j    ]];
				if(cur.o.dist(game::player1->o) > min(waypointdrawdist, maxparticledistance) || prev.o.dist(game::player1->o) > min(waypointdrawdist, maxparticledistance))
					continue;

				particle_flare(vec(0, 0, i + 1).add(prev.o), vec(0, 0, i + 1).add(cur.o), 0, PART_STREAK, DEBUG_VAI ? 0x0000FF : 0x00FF00, .75);
			}
			for(int j = max(0, ent->route.length() - 2); j < ent->route.length(); j++)
			{
				vec from = ent->abovehead();
				vec dest = vec(0, 0, i+1).add(waypoints[ent->route[j]].o);
				particle_flare(from, dest, 0, PART_STREAK, DEBUG_VAI ? 0x007F00 : 0x00007F, .5);
			}
		}
		#endif
	}
}
