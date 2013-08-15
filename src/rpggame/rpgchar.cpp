#include "rpggame.h"

///USE apply functionality - limited to characters and derived

void use::apply(rpgchar *user)
{
	loopv(effects)
		user->seffects.add(new victimeffect(user, effects[i], chargeflags, 1));
}

void use_armour::apply(rpgchar *user)
{
	float base, extra;
	user->base.skillpotency(skill, base, extra);

	loopv(effects)
	{
		statusgroup *sg = effects[i]->status;

		int resist = 0;
		int thresh = 0;
		if(!sg->friendly)
		{
			resist = user->base.getresistance(effects[i]->element);
			thresh = user->base.getthreshold(effects[i]->element);
		}

		loopvj(sg->effects)
		{
			if(sg->effects[j]->duration >= 0)
				ERRORF("statuses[%s]->effect[%i] has a non negative duration, these will not work properly and some will be ridiculously overpowered", effects[i]->status->key, j);
			sg->effects[j]->update(user, user, resist, thresh, base, extra);
		}
	}
}

void use_weapon::apply(rpgchar *user) {} // does nothing, do it in the character update loop

///base functions
bool rpgchar::useitem(item *it, equipment *slot, int u)
{
	int count = getcount(it);
	if(!it->charges || !count) return false;

	bool shouldcompact = false;

	if(it->charges > 0)
	{
		if(it->quantity)
		{
			item *newit = new item(); it->transfer(*newit);
			it->quantity--;
			newit->quantity = 1;
			newit->charges--;

			item *add = additem(newit);
			delete newit;
			it = add;

			if(slot)
			{
				it->quantity++;
				newit->quantity--;
				slot->it = add;
			}
		}
		else if(!it->quantity && count == 1)
		{
			it->charges--;
			shouldcompact = true;
		}
		else
		{
			//ASSERT(slot)
			//ASSERT(!it->quantity)

			item *newit = new item(); it->transfer(*newit);
			newit->charges--;
			it = slot->it = additem(newit);
			delete newit;

		}

		if(this == game::player1 && it->charges >= 1 && it->charges <= 5)
			game::hudline("\fs\f3%s\fr has limited (\fs\f3%i\fr) charges remaining!", it->name, it->charges);
	}

	if(u >= 0 && it->uses[u]->type == USE_CONSUME)
	{
		it->uses[u]->apply(this);
		it->getsignal("use", false, this, u);
	}

	if(shouldcompact) compactinventory(it->base);

	return true;
}

bool rpgchar::checkammo(equipment &eq, equipment *&quiver, bool remove)
{
	//it is assumed it's valid
	use_weapon *wep = (use_weapon *) eq.it->uses[eq.use];
	ammotype *at = wep->ammo;
	item *it = NULL;

	//ASSERT(at);

	//check reserved types first...
	float *fres = NULL;
	int *ires = NULL;

	if(at->key == reserved::amm_mana)
		fres = &mana;
	else if(at->key == reserved::amm_health)
		fres = &health;
	else if(at->key == reserved::amm_experience)
		ires = &base.experience;

	if(fres || ires)
	{
		quiver = NULL;
		if((fres ? *fres : *ires) < wep->cost)
		{
			if(this == game::player1) game::hudline("\f3 insufficient %s to use", at->name);
			else if (DEBUG_AI) DEBUGF("AI attach interrupted for %p; too little %s", this, at->name);
			return false;

		}
		if(remove)
		{
			if(fres) *fres -= wep->cost;
			else *ires -= wep->cost;
		}
		return true;
	}

	if(quiver && at->items.find(quiver->it->base) >= 0) it = quiver->it;
	else if (at->items.find(eq.it->base) >= 0) { quiver = NULL; it = eq.it; }
	else
	{
		if(this == game::player1) game::hudline("The current weapon cannot use %s!", at->name);
		else if (DEBUG_AI) DEBUGF("AI attack interrupted for %p, wrong ammo equipped", this);
		return false;
	}

	if(wep->cost > 0 && getcount(it) < wep->cost)
	{
		if(this == game::player1) game::hudline("You need more %s to attack again!", at->name);
		else if (DEBUG_AI) DEBUGF("AI attack interrupted for %p, too little ammo", this);
		return false;
	}

	if(remove)
	{
		drop(it, wep->cost, false);
		//this is the remove ammo pass - let it be known the quiver is no longer valid
		if(getcount(it) == 0) return false;
	}
	return true;
}

VAR(lefthand, 1, 0, -1);

//TODO implement alternative actions, eg block and strike with 2 handed weapons.
void rpgchar::doattack(equipment *eleft, equipment *eright, equipment *quiver)
{
	use_weapon *left = eleft ? (use_weapon *) eleft->it->uses[eleft->use] : NULL,
	           *right = eright ? (use_weapon *) eright->it->uses[eright->use] : NULL,
	           *ammo = NULL;

	if(charge && !primary && !secondary)
	{
		if((left && lastprimary) || (left == right && lastsecondary))
			attack = left;
		else
			attack = right;
	}
	else if((primary || secondary) && left && left == right)
		attack = right;
	else if((primary && left) ^ (secondary && right))
		attack = (primary && left) ? left : right;

	lastprimary = primary; lastsecondary = secondary;

	//we're attacking and can attack, oh noes!
	if(attack)
	{
		lefthand = attack == left;

		//function will invalidate the quiver if needed
		if(!checkammo(attack == left ? *eleft : *eright, quiver))
		{
			lastaction = lastmillis + 100;
			primary = secondary = 0;
			return; //we lack the catalysts, don't attack
		}
		if(quiver) ammo = (use_weapon *) quiver->it->uses[quiver->use];

		float mult = attack->maxcharge;
		if(attack->charge)
		{
			if(primary || secondary)
			{
				charge += curtime;
				return; //we're charging, don't attack... yet
			}

			mult = min<float>((float)charge / attack->charge, attack->maxcharge);
			charge = 0;

			if(mult < attack->mincharge)
				return;
		}

		//HACK
		//propogate an attack signal to self and all items within instead of this
		{
			if(attack == left || attack == right)
				(attack == left ? eleft : eright)->it->getsignal("attacksound", false, this, (attack == left ? eleft : eright)->use);
			if(ammo)
				quiver->it->getsignal("attacksound", false, this, quiver->use);
			getsignal("attacksound", false, this);
		}

		{
			float potency, extra;
			base.skillpotency(attack->skill, potency, extra);
			mult += attack->basecharge;
			mult = mult * (potency + extra);
		}

		int flags = attack->chargeflags | (ammo ? ammo->chargeflags : 0);
		effect *death = attack->deatheffect;
		effect *trail = attack->traileffect;

		if(ammo)
		{
			if(ammo->deatheffect) death = ammo->deatheffect;
			if(ammo->traileffect) trail = ammo->traileffect;
		}

		switch(attack->target)
		{
			case T_SINGLE:
			case T_MULTI:
			case T_AREA:
			{
				projectile *p = game::curmap->projs.add(new projectile());
				p->init(this, attack == left ? eleft : eright, quiver, 0, mult);
				break;
			}

			case T_SELF:
				hit(this, attack, ammo, mult, flags, vec(0, 0, 1));
				if(death) death->drawaura(this, mult, effect::DEATH, 0);
				break;

			case T_SMULTI:
			case T_SAREA:
			{
				projectile *p = game::curmap->projs.add(new projectile());
				p->init(this, attack == left ? eleft : eright, quiver, 0, mult);
				p->pflags = P_VOLATILE|P_STATIONARY;

				break;
			}
			case T_HORIZ:
			case T_VERT:
			{
				vec perp;
				if(attack->target == T_HORIZ)
					perp = vec(yaw * RAD, (pitch + 90) * RAD);
				else
					perp = vec((yaw + 90) * RAD, 0);

				vec dir = vec(yaw * RAD, pitch * RAD).rotate(attack->angle / 2.f * RAD, perp);
				vec orig = vec(o).sub(vec(0, 0, eyeheight / 2));
				vector<rpgent *> hit;
				vector<vec> hits;
				int reach = (attack->range + (ammo ? ammo->range : 0)) * (flags & CHARGE_TRAVEL ? mult : 1) + radius;

				loopi(attack->angle + 1)
				{
					vec ray = vec(dir).mul(reach).add(orig);
					loopvj(game::curmap->objs)
					{
						rpgent *obj = game::curmap->objs[j];
						float dist;
						if(obj != this && game::intersect(obj, orig, ray, dist) && dist <= 1)
						{
							if(hit.find(obj) == -1) hit.add(obj);
							hits.add(vec(ray).sub(orig).mul(dist));
						}
					}

					dir.rotate(-1 * RAD, perp);
				}

				loopv(hit)
					hit[i]->hit(this, attack, ammo, mult, flags, vec(yaw * RAD, pitch * RAD));

				loopv(hits)
				{
					vec pos = vec(hits[i]).add(orig);
					if(death) death->drawsplash(pos, vec(0, 0, 0), 5, 1, effect::DEATH, 1);
				}
				if(trail) trail->drawcircle(this, attack, mult, effect::TRAIL_SINGLE, 0);

				break;
			}

			case T_CONE:
			{
				//use dot product to determine if inside cone; allow small error?
				//draw particles by using a random value up to angle and applying a random rotation around view vector...; should probably do this elsewhere anyway...
				vec view = vec(yaw * RAD, pitch *RAD);
				int reach = (attack->range + (ammo ? ammo->range : 0)) * (flags & CHARGE_TRAVEL ? mult : 1) + radius;

				loopv(game::curmap->objs)
				{
					rpgent *d = game::curmap->objs[i];
					if(d == this) continue;

					vec other = vec(d->o).sub(o).normalize();
					const vec min = vec(d->o.x - d->radius, d->o.y - d->radius, d->o.z - d->eyeheight);
					const vec max = vec(d->o.x + d->radius, d->o.y + d->radius, d->o.z + d->aboveeye);

					if(view.dot(other) > sin(attack->angle * RAD) && o.dist_to_bb(min, max) <= reach)
					{
						d->hit(this, attack, ammo, mult, flags, other);

						if(death) death->drawsplash(vec(d->o).sub(vec(0, 0, d->eyeheight / 2)), vec(0, 0, 1), 5, 1, effect::DEATH, 1);
					}
				}

				if(trail) trail->drawcone(this, attack, mult, effect::TRAIL_SINGLE, 0);
				break;
			}
		}
		//recoil
		vel.add(vec(yaw * RAD, pitch * RAD).mul( -1 * (attack->recoil + (ammo ? ammo->recoil : 0))));

		if(attack->cost)
			checkammo(attack == left ? *eleft : *eright, quiver, true); //remove ammo

		lastaction = lastmillis + attack->cooldown + (ammo ? ammo->cooldown : 0);
	}
}

void rpgchar::resetmdl()
{
	temp.mdl = (mdl && mdl[0]) ? mdl : DEFAULTMODEL;
}

///REMEMBER route IS REVERSED
void rpgchar::doai(equipment *eleft, equipment *eright, equipment *quiver)
{
	//TODO 1 - minimise friendly fire; try to avoid killing allies
	//TODO 2 - ranged attackers/spellcasters; keep distance/retreat
	//TODO 3 - ranged attacks; dodge
	//TODO 4 - use objects (ie teleports) to follow the player

	if(aiflags & AI_MOVE)
	{
		int end = ai::closestwaypoint(dest) - ai::waypoints.getbuf();
		if(!route.length() || route[0] != end)
		{
			route.setsize(0);
			int start = ai::closestwaypoint(feetpos()) - ai::waypoints.getbuf();
			ai::findroute(start, end, route);
		}

		if(route.length())
		{
			waypoint &first = ai::waypoints[route[route.length() - 1]];
			if(first.o.dist(feetpos()) <= 4)
				route.pop();
			else if(route.length() > 1)
			{
				waypoint &second = ai::waypoints[route[route.length() - 2]];
				const vec feet = feetpos();
				if(second.o.dist(feet) <= first.o.dist(second.o) && first.o.dist(feet) < second.o.dist(feet))
					route.pop();
			}
		}
	}

	if(target && target != this && aiflags & (AI_ATTACK | AI_ALERT))
	{
		vec dir = vec(lastknown).sub(feetpos());
		dir.z += target->eyeheight - eyeheight;
		vectoyawpitch(dir, yaw, pitch);
	}
	else
	{
		if(route.length() > 1)
		{
			vec dir = vec(ai::waypoints[route[route.length() - 2]].o).sub(feetpos());
			dir.mul(curtime / 200.f / dir.magnitude());
			dir.add(vec(yaw * RAD, pitch * RAD).mul(1 - (curtime / 200.0f)));
			//some basic interpolation
			//200 chosen as it's the maximum interval between updates
			vectoyawpitch(dir, yaw, pitch);
		}
	}

	primary = secondary = false;
	if(target && target != this && aiflags & AI_ALERT)
	{
		bool left = true;
		if(eleft)
		{
			attack = (use_weapon *) eleft->it->uses[eleft->use];
			if(attack->type < USE_WEAPON || !attack->effects.length() || attack->effects[0]->status->friendly)
				attack = NULL;
		}
		if(!attack && eright)
		{
			left = false;
			attack = (use_weapon *) eright->it->uses[eright->use];
			if(attack->type < USE_WEAPON || !attack->effects.length() || attack->effects[0]->status->friendly)
				attack = NULL;
		}


		if(attack)
		{
			if(attack->charge)
			{
				if(left) {primary = true;}
				else {secondary = true;}

				if((float) charge / attack->charge >= attack->maxcharge && cansee(target))
					primary = secondary = false;
			}
			else if(cansee(target) && (attack->range + radius + target->radius) >= o.dist(target->o))
			{
				if(left) primary = true;
				else secondary = true;
			}
		}
		attack = NULL;
	}

	if(aiflags & AI_MOVE && route.length())
	{
		vec dir = vec(ai::waypoints[route.last()].o).sub(feetpos());
		dir.z = 0; dir.normalize();
		dir.rotate_around_z(-yaw * RAD);

		if(fabs(dir.y) >= .7)
			move = dir.y < 0 ? -1 : 1;
		if(fabs(dir.x) >= .7)
			strafe = dir.x < 0 ? -1 : 1;
	}
}

VARP(r_aiperiod, 0, 100, 1000);

void rpgchar::update()
{
	attack = NULL;

	if(state == CS_DEAD)
	{
		move = strafe = 0;
	}
	else
	{
		base.setspeeds(maxspeed, jumpvel);
		mana =   min<float>(base.getmaxmp(),   mana + (base.getmpregen() * curtime / 1000.0f));
		health = min<float>(base.getmaxhp(), health + (base.gethpregen() * curtime / 1000.0f));


		if(health < 0)
			die(NULL);
	}

	if(state == CS_DEAD)
	{
		if(ragdoll)
			moveragdoll(this);
		else
		{
			crouchplayer(this, this == game::player1 ? 10 : 2, true);
			moveplayer(this, this == game::player1 ? 10 : 2, false);
		}

		base.resetdeltas();
		return;
	}

	if(lastai <= lastmillis)
	{
		getsignal("ai update", false, this);
		lastai = lastmillis + r_aiperiod / 2 + rnd(max(1, r_aiperiod));
	}

	//handle equipment
	equipment *eleft = NULL, //primary
	          *eright = NULL, //secondary
	          *quiver = NULL;

	base.resetdeltas(); //reset attribute changes
	loopv(equipped)
	{
		use *usable = equipped[i]->it->uses[equipped[i]->use];
		if(equipped[i]->it->charges) usable->apply(this);
		if(usable->type == USE_WEAPON)
		{
			use_weapon *wep = (use_weapon *) usable;
			if(wep->slots & SLOT_LHAND)
				eleft = equipped[i];
			if(wep->slots & SLOT_RHAND)
				eright = equipped[i];
			if(wep->slots & SLOT_QUIVER)
				quiver = equipped[i];
		}
	}

	///AI STUFF - player can use it during cutscenes
	if(this != game::player1 || camera::cutscene)
	{
		move = strafe = jumping = 0;
		aiflags = 0;
		directives.sort(directive::compare);
		loopv(directives)
		{
			if(!directives[i]->update(this))
			{
				delete directives[i]; directives.remove(i); i--;
			}
		}

		doai(eleft, eright, quiver);
	}
	else if(directives.length())
	{
		directives.deletecontents();
		move = strafe = jumping = 0;
	}

	crouchplayer(this, this == game::player1 ? 10 : 2, true);
	moveplayer(this, this == game::player1 ? 10 : 2, true);
	entities::touchents(this);

	if(lastmillis >= lastaction)
		doattack(eleft, eright, quiver);
}

VAR(animoverride, -1, 0, NUMANIMS-1);
VAR(testanims, 0, 0, 1);
VAR(testpitch, -90, 0, 90);

void renderclient(dynent *d, const char *mdlname, modelattach *attachments, int hold, int attack, int attackdelay, int lastaction, int lastpain, float scale, bool ragdoll, float trans)
{
	int anim = hold ? hold : ANIM_IDLE|ANIM_LOOP;
	float yaw = testanims ? 0 : d->yaw,
		pitch = testpitch ? testpitch : d->pitch,
		roll = d->roll;

	vec o = d->feetpos();
	int basetime = 0;
	if(animoverride) anim = (animoverride<0 ? ANIM_ALL : animoverride)|ANIM_LOOP;
	else if(d->state==CS_DEAD)
	{
		anim = ANIM_DYING|ANIM_NOPITCH;
		basetime = lastpain;
		if(ragdoll) anim |= ANIM_RAGDOLL;
		else if(lastmillis-basetime>1000) anim = ANIM_DEAD|ANIM_LOOP|ANIM_NOPITCH;
	}
	else if(d->state==CS_EDITING || d->state==CS_SPECTATOR) anim = ANIM_EDIT|ANIM_LOOP;
	else if(d->state==CS_LAGGED)                            anim = ANIM_LAG|ANIM_LOOP;
	else
	{
		if(lastmillis-lastpain < 300)
		{
			anim = ANIM_PAIN;
			basetime = lastpain;
		}
		else if(lastpain < lastaction && (attack < 0 || lastmillis-lastaction < attackdelay))
		{
			anim = attack < 0 ? -attack : attack;
			basetime = lastaction;
		}

		if(d->inwater && d->physstate<=PHYS_FALL) anim |= (((game::allowmove(d) && (d->move || d->strafe)) || d->vel.z+d->falling.z>0 ? ANIM_SWIM : ANIM_SINK)|ANIM_LOOP)<<ANIM_SECONDARY;
		else if(d->timeinair>100) anim |= (ANIM_JUMP|ANIM_END)<<ANIM_SECONDARY;
		else if(game::allowmove(d) && (d->move || d->strafe))
		{
			if(d->move>0) anim |= (ANIM_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
			else if(d->strafe) anim |= ((d->strafe>0 ? ANIM_LEFT : ANIM_RIGHT)|ANIM_LOOP)<<ANIM_SECONDARY;
			else if(d->move<0) anim |= (ANIM_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;
		}

		if(d->crouching) switch((anim>>ANIM_SECONDARY)&ANIM_INDEX)
		{
			case ANIM_IDLE: anim &= ~(ANIM_INDEX<<ANIM_SECONDARY); anim |= ANIM_CROUCH<<ANIM_SECONDARY; break;
			case ANIM_JUMP: anim &= ~(ANIM_INDEX<<ANIM_SECONDARY); anim |= ANIM_CROUCH_JUMP<<ANIM_SECONDARY; break;
			case ANIM_SWIM: anim &= ~(ANIM_INDEX<<ANIM_SECONDARY); anim |= ANIM_CROUCH_SWIM<<ANIM_SECONDARY; break;
			case ANIM_SINK: anim &= ~(ANIM_INDEX<<ANIM_SECONDARY); anim |= ANIM_CROUCH_SINK<<ANIM_SECONDARY; break;
			case 0: anim |= (ANIM_CROUCH|ANIM_LOOP)<<ANIM_SECONDARY; break;
			case ANIM_FORWARD: case ANIM_BACKWARD: case ANIM_LEFT: case ANIM_RIGHT:
				anim += (ANIM_CROUCH_FORWARD - ANIM_FORWARD)<<ANIM_SECONDARY;
				break;
		}

		if((anim&ANIM_INDEX)==ANIM_IDLE && (anim>>ANIM_SECONDARY)&ANIM_INDEX) anim >>= ANIM_SECONDARY;
	}
	if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX)) anim |= (ANIM_IDLE|ANIM_LOOP)<<ANIM_SECONDARY;
	int flags = 0;
    if(d!=game::player1) flags |= MDL_CULL_VFC | MDL_CULL_OCCLUDED | MDL_CULL_QUERY;
    if(d->type==ENT_PLAYER) flags |= MDL_FULLBRIGHT;
    else flags |= MDL_CULL_DIST;
	if(d->state == CS_LAGGED) trans = min(trans, 0.3f);
	rendermodel(mdlname, anim, o, yaw, pitch, roll, flags, d, attachments, basetime, 0, scale, trans);
}

void rpgchar::render()
{
	int lastaction = this->lastaction,
		action = ANIM_MSTRIKE,
		delay = 300,
		hold = ANIM_MHOLD|ANIM_LOOP;
	lastaction -= delay;

	vector<modelattach> attachments;
	vec *emitter = emitters;

	loopv(equipped)
	{
		use_armour *use = (use_armour *) equipped[i]->it->uses[equipped[i]->use];
		if(use->type < USE_ARMOUR || !use->vwepmdl || !use->slots) continue;

		const char *tag = NULL;
		if(use->slots & SLOT_LHAND) tag = "tag_lhand";
		else if(use->slots & SLOT_RHAND) tag = "tag_rhand";
		else if(use->slots & SLOT_LEGS) tag = "tag_legs";
		else if(use->slots & SLOT_ARMS) tag = "tag_arms";
		else if(use->slots & SLOT_TORSO) tag = "tag_torso";
		else if(use->slots & SLOT_HEAD) tag = "tag_head";
		else if(use->slots & SLOT_FEET) tag = "tag_feet";
		else if(use->slots & SLOT_QUIVER) tag = "tag_quiver";
		attachments.add(modelattach(tag, use->vwepmdl));

		if(use->idlefx)
		{
			attachments.add(modelattach("tag_partstart", emitter));
			attachments.add(modelattach("tag_partend", emitter + 1));
			emitter += 2;
		}
	}
	attachments.add(modelattach()); //delimitor

	if(aiflags & AI_ANIM) hold = (forceanim & ANIM_INDEX) | ANIM_LOOP;

	renderclient(this, temp.mdl ? temp.mdl : mdl, attachments.buf, hold, action, delay, lastaction, state == CS_ALIVE ? lastpain : 0, 1, true, temp.alpha);

	emitter = emitters;
	loopv(equipped)
	{
		use_armour *use = (use_armour *) equipped[i]->it->uses[equipped[i]->use];
		if(use->type < USE_ARMOUR || !use->vwepmdl || !use->idlefx || !use->slots)
			continue;

		use->idlefx->drawwield(emitter[0], emitter[1], 1, effect::TRAIL);
		emitter[0] = emitter[1] = vec(-1, -1, -1);
		emitter += 2;
	}
}

const char *rpgchar::getname() const {return name ? name : "Shirley";}

///Character/AI
void rpgchar::givexp(int xp)
{
	int level = base.level;
	base.givexp(xp);
	if(level != base.level)
		getsignal("level");
}

void rpgchar::equip(item *it, int u)
{
	if(primary || secondary || lastprimary || lastsecondary)
	{
		if(this == game::player1) game::hudline("You can't equip items while attacking!");
		return;
	}

	vector<item *> &inv = inventory.access(it->base, vector<item *>());

	if(inv.find(it) < 0 || it->quantity <= 0 || !it->uses.inrange(u))
		return;

	use_armour *usecase = (use_armour *) it->uses[u];
	if(usecase->type < USE_ARMOUR) return;
	if(!usecase->reqs.meet(base))
	{
		if(this == game::player1) game::hudline("You cannot wield this item! You do not meet the requirements!");
		return;
	}

	if(!usecase->slots || dequip(NULL, usecase->slots))
	{
		equipped.add(new equipment(it, u));
		it->quantity--;
		it->getsignal("equip", false, this, u);
	}
	else if(this == game::player1)
		game::hudline("Unable to equip: required slot unavailable");
}

bool rpgchar::dequip(const char *base, int slots)
{
	if(primary || secondary || lastprimary || lastsecondary)
	{
		if(this == game::player1) game::hudline("You can't dequip items while attacking!");
		return false;
	}

	if(base) base = game::hashpool.find(base, NULL);

	int rem = 0;
	loopv(equipped)
	{
		use_armour *arm = (use_armour *) equipped[i]->it->uses[equipped[i]->use];

		if((!base ? true : equipped[i]->it->base == base) && (slots ? arm->slots & slots : true))
		{
			if(equipped[i]->it->flags & item::F_CURSED)
				rem++;
			else
			{
				equipped[i]->it->quantity++;
				rpgscript::removeminorrefs(equipped[i]);
				delete equipped.remove(i--);
			}
		}
	}
	return !rem;
}

void rpgchar::die(rpgent *killer)
{
	if(DEBUG_ENT)
		DEBUGF("ent %p killed by %p%s", this, killer, state != CS_ALIVE ? "; already dead?" : "");
	if(state != CS_ALIVE)
		return;

	if(killer)
		killer->givexp((killer == this ? -100 : 25) * base.level);

	state = CS_DEAD;
	health = 0;
	route.setsize(0);
	getsignal("death", true, killer); //in case the player has an item in his inventory that will revive him or something
}

void rpgchar::revive(bool spawn)
{
	if(state != CS_DEAD)
		return;

	health = base.getmaxhp();
	mana = base.getmaxmp();
	state = CS_ALIVE;
	seffects.deletecontents();

	physent::reset();
	cleanragdoll(this);
	if(spawn) findplayerspawn(this, -1);
	else entinmap(this);

	getsignal("resurrect", true, NULL);
}

VAR(hit_friendly, 1, 0, -1);
VAR(hit_total, 1, 0, -1);

void rpgchar::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
{
	hit_friendly = 1;
	hit_total = 0;
	vector<victimeffect *> neweffects;
	bool protect = false;

	if (attacker->type() == ENT_CHAR)
	{
		rpgchar *d = (rpgchar *) attacker;
		if(game::friendlyfire >= 2 && faction == d->faction) protect = true;
		else if (game::friendlyfire >= 3 && faction->getrelation(d->faction->key) > 50) protect = true;
	}

	loopv(weapon->effects)
	{
		statusgroup *sg = weapon->effects[i]->status;
		if(!sg->friendly) hit_friendly = 0;
		victimeffect &v = *neweffects.add(new victimeffect(attacker, weapon->effects[i], weapon->chargeflags, mul));
		loopvj(v.effects) hit_total += v.effects[j]->value();
	}

	if(ammo) loopv(ammo->effects)
	{
		statusgroup *sg = ammo->effects[i]->status;
		if(!sg->friendly) hit_friendly = 0;
		victimeffect &v = *neweffects.add(new victimeffect(attacker, ammo->effects[i], weapon->chargeflags, mul));
		loopvj(v.effects) hit_total += v.effects[j]->value();
	}

	if(!hit_friendly)
	{
		if(protect)
		{
			neweffects.deletecontents();
			return;
		}

		lastpain = lastmillis;

		if(this == game::player1 && hit_total)
			damagecompass(hit_total, attacker->o);

		static vector<equipment *> armour;
		armour.setsize(0);

		loopv(equipped)
		{
			use_weapon *wep = (use_weapon *) equipped[i]->it->uses[equipped[i]->use];
			if(wep->slots &  ~(SLOT_LHAND|SLOT_RHAND))
				armour.add(equipped[i]);
		}
		if(armour.length())
		{
			int i = rnd(armour.length());
			useitem(armour[i]->it, armour[i]);
		}
	}

	loopv(neweffects) seffects.add(neweffects[i]);

	vec kickback = dir.mul(weapon->kickback + (ammo ? ammo->kickback : 0));
	if(state == CS_DEAD && ragdoll)
	{
		kickback.mul(2);
		ragdolladdvel(this, kickback);
	}
	else vel.add(kickback);

	if(this == game::player1 && !hit_friendly && hit_total) damagecompass(hit_total, attacker->o);
	getsignal("hit", false, attacker);
}

///Inventory
void rpgchar::init(const char *base)
{
	game::loadingrpgchar = this;
	rpgscript::config->setref(this, true);

	defformatstring(file, "%s/%s.cfg", game::datapath("critters"), base);
	rpgexecfile(file);

	game::loadingrpgchar = NULL;
	rpgscript::config->setnull(true);

	health = this->base.getmaxhp();
	mana = this->base.getmaxmp();
}

bool rpgchar::validate()
{
	if(!script)
	{
		ERRORF("Entity %p uses invalid script - trying fallback: null", this);
		script = DEFAULTSCR;

		if(!script) return false;
	}

	if(!faction)
	{
		ERRORF("Entity %p uses invalid faction - trying fallback: player", this);
		faction = DEFAULTFACTION;

		if(!faction) return false;
	}

	return true;
}

item *rpgchar::additem(item *it)
{
	vector<item *> &inv = inventory.access(it->base, vector<item *>());

	loopv(inv)
	{
		if(inv[i]->compare(it))
		{
			inv[i]->quantity += it->quantity;
			return inv[i];
		}
	}

	item *newit = inv.add(new item());
	it->transfer(*newit);
	return newit;
}

item *rpgchar::additem(const char *base, int q)
{
	item it;
	it.init(base);
	it.quantity = q;

	if(!it.validate()) return NULL;
	return additem(&it);
}

int rpgchar::drop(item *it, int q, bool spawn)
{
	vector<item *> &inv = inventory.access(it->base, vector<item *>());
	int rem = 0;
	if(inv.find(it) == -1) return 0;

	rem = min(q, it->quantity);
	it->quantity -= rem;

	loopv(equipped)
	{
		if(rem >= q) break;
		if(equipped[i]->it == it)
		{
			rem++;
			rpgscript::removeminorrefs(equipped[i]);
			delete equipped.remove(i--);
		}
	}

	if(rem && spawn)
	{
		rpgitem *drop = new rpgitem();
		it->transfer(*drop);
		game::curmap->objs.add(drop);

		drop->quantity = rem;
		drop->o = vec(yaw * RAD, pitch * RAD).mul(radius * 2).add(o);
		entinmap(drop);
	}

	return rem;
}

int rpgchar::drop(const char *base, int q, bool spawn)
{
	base = game::hashpool.find(base, NULL);
	if(base) return 0;

	vector<item *> &inv = inventory.access(base, vector<item *>());
	int rem = 0;

	loopv(inv)
		rem += min(q - rem, inv[i]->quantity);

	loopv(equipped)
	{
		if(rem >= q) break;
		if(equipped[i]->it->base == base)
		{
			equipped[i]->it->quantity++;
			rem++;
			rpgscript::removeminorrefs(equipped[i]);
			delete equipped.remove(i--);
		}
	}

	q = rem;

	loopv(inv)
		rem -= drop(inv[i], min(inv[i]->quantity, rem), spawn);

	return q;
}

int rpgchar::pickup(rpgitem *it)
{
	int add = it->quantity;
	if(it->weight)
		add = clamp(add, 0.f, (base.getmaxcarry() * 2 - getweight()) / it->weight);

	item *n = NULL;

	vector<item *> &inv = inventory.access(it->base, vector<item *>());
	loopv(inv) if(inv[i]->compare(it))
	{
		n = inv[i];
		break;
	}

	if(!n)
	{
		n = inv.add(new item());
		it->transfer(*n);
		n->quantity = 0;
	}

	it->quantity -= add;
	n->quantity += add;

// 	any point to this?
// 	compactinventory(it->base);
	return add;
}

int rpgchar::getitemcount(const char *base)
{
	base = game::hashpool.find(base, NULL);
	if(!base) return 0;

	vector<item *> &inv = inventory.access(base, vector<item *>());

	int count = 0;
	loopv(inv) count += inv[i]->quantity;
	loopv(equipped) if(equipped[i]->it->base == base) count++;

	return count;
}

int rpgchar::getcount(item *it)
{
	vector<item *> &inv = inventory.access(it->base, vector<item *>());

	int count = 0;
	if(inv.find(it) >= 0) count += it->quantity;
	loopv(equipped) if(equipped[i]->it == it) count++;

	return count;
}

float rpgchar::getweight()
{
	float ret = 0;

	enumerate(inventory, vector<item *>, stack,
		loopvj(stack) ret += stack[j]->quantity * stack[j]->weight;
	)
	loopv(equipped)
		ret += equipped[i]->it->weight;

	return ret;
}

void rpgchar::compactinventory(const char *base)
{
	if(!base)
	{
		enumerate(inventory, vector<item *>, itemstack,
			if(itemstack.length()) compactinventory(itemstack[0]->base);
		)

		return;
	}

	base = game::hashpool.find(base, NULL);
	if(!base) return;

	vector<item *> &stack = *inventory.access(base);
	loopvrev(stack)
	{
		if(!getcount(stack[i]))
		{
			rpgscript::removeminorrefs(stack[i]);
			delete stack.remove(i);
			continue;
		}

		loopj(i)
		{
			if(stack[i]->compare(stack[j]))
			{
				if(DEBUG_ENT)
					DEBUGF("Found duplicate item definition, merging %p into %p", stack[i], stack[j]);

				rpgscript::replacerefs(stack[i], stack[j]);
				item *it = stack.remove(i);
				stack[j]->quantity += it->quantity;

				loopv(equipped) if(equipped[i]->it == it)
					equipped[i]->it = stack[j];

				delete it;

				break;
			}
		}
	}
}

extern int fog;
extern int waterfog;

bool rpgchar::cansee(rpgent *d)
{
	//TODO     add in modifiers for sound, movement, light, invisibility and perception

	//at present this will only see if the entity is within this creature's LoS
	vec dir = vec(d->o).sub(o).normalize();
	vec view = vec(yaw * RAD, pitch * RAD);

	if(dir.dot(view) >= .25) // fov == ~150
	{
		vec pos;
		float dist = raycubepos(o, dir, pos, 0, RAY_ALPHAPOLY|RAY_CLIPMAT);
		if(physstate == PHYS_FLOAT || d->physstate == PHYS_FLOAT)
			dist = min<float>(waterfog, dist);
		dist = min<float>(fog, dist);

		if(o.dist_to_bb(d->feetpos(), d->abovehead()) <= dist)
			return true;
	}

	return false;
}
