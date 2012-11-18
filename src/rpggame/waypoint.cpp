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
	VAR(waypointdoublelink, 0, 1, 1);

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

	VAR(waypointstairheight, 0, 8, 32);
	VARP(experimentalwaypoint, 0, 0, 1); //temp - testing var

	void dropwaypoint(const vec &o)
	{
		waypoint *closest = closestwaypoint(o);

		if(!prev)
		{
			prev = &waypoints.add(waypoint(o));
			return;
		}

		float distp = prev->o.dist(o);
		float distc = closest->o.dist(o);
		float distn = closest->o.dist(prev->o);

		if(distp > 15 && distc > 15)
		{
			ushort ind = prev - waypoints.getbuf(); //just in case buffer position changes
			waypoint *next = &waypoints.add(waypoint(o));

			prev->links.add(next - waypoints.getbuf());
			//only link back if the node would be close to the floor; ie stairs
			if(waypointdoublelink && waypointstairheight >= raycube(o, vec(0, 0, -1))) next->links.add(ind);

			prev = next;
		}
		else if(distn > 31) //the spot is overshot, but the target was somewhere between the nodes, so we add one at an intermediary
		{
			vec pos = vec(prev->o).add(closest->o).div(2.0f);

			//just in case buffer position changes
			ushort close = closest - waypoints.getbuf(),
			       prior = prev    - waypoints.getbuf(),
			       n;

			waypoint *next = &waypoints.add(waypoint(pos));
			n = next - waypoints.getbuf();

			if(waypointdoublelink) next->links.add(close);
			next->links.add(prior);
			prev->links.add(n);
			if(waypointdoublelink) closest->links.add(n);

			prev = next;
		}
		else if(prev != closest)
		{
			int ind = closest - waypoints.getbuf();
			if(prev->links.find(ind) == -1)
				prev->links.add(ind);

			ind = prev - waypoints.getbuf();
			if(waypointdoublelink && game::player1->timeinair < 25 && closest->links.find(ind) == -1)
				closest->links.add(ind);

			prev = closest;
		}
	}

	void dropwaypoint_exp(const vec &o)
	{
		int p = prev - waypoints.getbuf();
		waypoint &wp = waypoints.add(waypoint(o));
		prev = &wp;

		bool wpair = raycube(wp.o, vec(0, 0, -waypointstairheight), 0, RAY_POLY|RAY_CLIPMAT) >= 1;
		// - 1; do not test last one
		for(int i = 0; i < waypoints.length() - 1; i++)
		{
			waypoint &ow = waypoints[i];
			if(ow.o.dist(wp.o) > 30) continue;
			bool owair = raycube(ow.o, vec(0, 0, -waypointstairheight), 0, RAY_POLY|RAY_CLIPMAT) >= 1;

			if(wpair && owair)
			{
				if(i == p)
					ow.links.add(waypoints.length() - 1);
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
					wp.links.add(i);
					ow.links.add(waypoints.length() - 1);
				}
			}
		}
	}

	void trydrop()
	{
		if(!dropwaypoints || editmode || game::player1->state == CS_DEAD)
		{
			return;
		}

		if(!waypoints.length())
		{
			prev = &waypoints.add(waypoint(game::player1->feetpos()));
			return;
		}
		else if (!prev)
			prev = closestwaypoint(game::player1->feetpos());

		if(!experimentalwaypoint)
		{
			vec feet = game::player1->feetpos();
			float dist = prev->o.dist(feet);

			if(dist > 75)
			{
				prev = NULL;
				dropwaypoint(feet);
			}
			else if(dist >= 20)
			{
				vec delta = feet.sub(prev->o).normalize().mul(24);

				feet = prev->o;
				while(dist > 20)
				{
					dist -= 24;
					feet.add(delta);

					dropwaypoint(feet);
				}
			}
			else
				dropwaypoint(feet); //link :D
		}
		else
		{
			float closest = 1024;
			loopv(waypoints)
			{
				float dst = waypoints[i].o.dist(game::player1->feetpos());
				if(dst < closest) closest = dst;
			}

			if(closest >= 16)
				dropwaypoint_exp(game::player1->feetpos());
		}
	}

	FVAR(waypointmergedist, 1, 8, 128);
	FVAR(waypointlinkmaxdist, 1, 32, 128);
	VAR(waypointmergepasses, 1, 4, 128);
	VAR(waypointmaxlinks, 3, 10, 255);

	void removewaypoint(int ind)
	{
		if(!waypoints.inrange(ind))
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr Waypoint %i not inrange", ind);
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
		}

		waypoints.remove(ind);
	}

	ICOMMAND(removewaypoint, "i", (int *ind), removewaypoint(*ind))

	void removelink(int a, int b)
	{
		if(!waypoints.inrange(a) || !waypoints.inrange(b))
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr %i or %i is not inrange", a, b);
			return;
		}

		waypoint &first = waypoints[a], &second = waypoints[b];
		int ind;
		if((ind = first.links.find(b)) >= 0)
			first.links.remove(ind);
		if((ind = second.links.find(a)) >= 0)
			second.links.remove(ind);
	}

	ICOMMAND(removewaypointlink, "ii", (int *a, int *b), removelink(*a, *b))

	void mergewaypoints(int first, int second)
	{
		waypoint &f = waypoints[first];
		waypoint &s = waypoints[second];

		int tmp;
		while((tmp = s.links.find(first)) != -1)
			s.links.remove(tmp);

		loopv(s.links)
		{
			if(f.links.find(s.links[i]) == -1)
				f.links.add(s.links[i]);
		}

		loopv(waypoints)
		{
			int tmp;
			if(i !=first && (tmp = waypoints[i].links.find(second)) >= 0)
				waypoints[i].links[tmp] = first;
		}

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

						point.links.pop();
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
					w.links.remove(j);
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

		defformatstring(wptname)("packages/%s/%s.wpt", mpath, mname);
		path(wptname);

		stream *f = opengzfile(wptname, "rb");

		if(!f)
		{
			if(msg) conoutf(CON_ERROR, "\fs\f3ERROR:\fr failed to load waypoints");
			return;
		}

		char magic[4];
		if(f->read(magic, 4) < 4 || memcmp(magic, "OWPT", 4))
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr magic mismatch (expected OWPT, got %4.4s)", magic);
			delete f;
			return;
		}

		clearwaypoints();

		ushort numwp = f->getlil<ushort>();

		loopi(numwp)
		{
			waypoint &w = waypoints.add();

			w.o.x = f->getlil<float>();
			w.o.y = f->getlil<float>();
			w.o.z = f->getlil<float>();
			int numlinks = f->getchar();
			if(numlinks > 10)
				conoutf("\fs\f6WARNING:\fr links for waypoint #%i are exorbitant (%i)", i, numlinks);
			loopj(numlinks)
				w.links.add(f->getlil<ushort>() - 1); //FPS game uses 0 to denote dummy waypoint, and use 1+ to denote real waypoints - we don't
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

		defformatstring(wptname)("packages/%s/%s.wpt", mpath, mname);
		path(wptname);

		stream *f = opengzfile(wptname, "wb");
		if(!f)
		{
			conoutf("failed to save waypoints");
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

	void findroute(int from, int to, vector<ushort> &route)
	{
		if(!waypoints.length() || !waypoints.inrange(from) || !waypoints.inrange(to) || from == to)
			return;

		//determine scores
		loopv(waypoints)
		{
			waypoints[i].score = waypoints[i].o.dist(waypoints[to].o);
			waypoints[i].parent = NULL;
		}

		//recurse
		waypoint *cur = &waypoints[from],
		         *first = cur;

		while(true)
		{
			int lowest = -1;
			//try the "series of low scores" method
			loopv(cur->links)
			{
				waypoint &l = waypoints[cur->links[i]];
				if(cur->links[i] == to ||
					( !l.parent && &l != first &&
						(lowest < 0 || l.score < waypoints[lowest].score) &&
						(l.links.length() > 1 || (l.links.length() && !waypoints[l.links[0]].parent))
					)
				)
				{
					lowest = cur->links[i];
				}
			}

			if(lowest == -1 && cur == first)
				return;
			else if (lowest == -1)
			{
				cur = cur->parent;
			}
			else
			{
				waypoints[lowest].parent = cur;
				cur = &waypoints[lowest];
			}

			if(lowest == to)
				break;
		}

		//cur should still be the destination
		int i = route.length();
		while(true)
		{
			route.add(cur - waypoints.getbuf());

			if(cur == first)
				break;

			cur = cur->parent;
		}

		//now we optimise the route, we start at the back and check the latter nodes for common nodes
		//common nodes include unused nodes between two nodes in the route (a shortcut)
		//             as well as nodes which have a link to one of the nodes later in the vector
		///just remember the vector is in reverse
		///we also start at the end of the prior movement vector; we don't want to miss out critical destinations.

		for( ; i < route.length(); i++)
		{
			//don't bother checking i + 1
			for(int j = route.length() - 1; j > i + 1; j--)
			{
				waypoint &chk = waypoints[route[j]];

				//see if there are shortcuts available; take them!
				if(chk.links.find(route[i]) >= 0)
				{
					route.remove(i + 1, j - i - 1);
					break;
				}
				//see if a shortcut can be taken by using 1 intermediary node
				//we don't check for more
				if(i + 2 <= j)
				{
					int found = -1;
					loopvk(chk.links)
					{
						if(waypoints[chk.links[k]].links.find(route[i]) >= 0)
						{
							found = k;
							break;
						}
					}
					if(found >= 0)
					{
						route[i + 1] = chk.links[found];
						route.remove(i + 2, j - i - 2);
						break;
					}
				}
			}
		}
	}

	#ifdef NO_DEBUG
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

	VARP(waypointdrawdist, 64, 256, 4096);

	void renderwaypoints()
	{
		loopv(waypoints)
		{
			waypoint &w = waypoints[i];
			if(w.o.dist(game::player1->o) > min(waypointdrawdist, maxparticledistance)) continue;

			defformatstring(ds)("%i\n%i", i, w.links.length());
			particle_textcopy(vec(0, 0, 6).add(w.o), ds, PART_TEXT, 1, 0xFF00FF, 3);

			loopvj(w.links)
			{
				waypoint &l = waypoints[w.links[j]];
				particle_flare(w.o, l.o, 0, PART_STREAK, 0x0000FF, .5);
			}
		}
		#ifdef NO_DEBUG
		if(!DEBUG_AI) return;

		for(int i = 1; i < testroute.length(); i++)
		{
			waypoint &cur  = waypoints[testroute[i-1]];
			waypoint &prev = waypoints[testroute[i  ]];
			if(cur.o.dist(game::player1->o) > min(waypointdrawdist, maxparticledistance) || prev.o.dist(game::player1->o) > min(waypointdrawdist, maxparticledistance))
				continue;

			particle_flare(vec(0, 0, 1).add(prev.o), vec(0, 0, 1).add(cur.o), 0, PART_STREAK, 0xFF0000, .5);
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

				particle_flare(vec(0, 0, i + 1).add(prev.o), vec(0, 0, i + 1).add(cur.o), 0, PART_STREAK, 0x00FF00, .5);
			}
			for(int j = max(0, ent->route.length() - 2); j < ent->route.length(); j++)
			{
				vec from = ent->abovehead();
				vec dest = vec(0, 0, i+1).add(waypoints[ent->route[j]].o);
				particle_flare(from, dest, 0, PART_STREAK, 0x00AFFF, .5);
			}
		}
		#endif
	}
}