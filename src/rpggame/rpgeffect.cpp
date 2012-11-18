#include "rpggame.h"

//essentially a collection of functions to greatly simplify the use of assorted effects while making the aesthetics more consistent

inline void setvars(effect *e, int type, int &fade, int &gravity, int &num)
{
	num = 1;
	fade = e->fade;
	gravity = e->gravity;

	switch(type)
	{
		case effect::PROJ:
			fade = 1;
			gravity = 0;
			break;
		case effect::TRAIL_SINGLE:
			num = 15;
			break;
		case effect::DEATH:
			num = 20;
			break;
		case effect::DEATH_PROLONG:
			if(e->particle == PART_EXPLOSION || e->particle == PART_EXPLOSION_BLUE)
				fade = 1;
			break;
		default:
			break;
	}
}

FVARP(partmul, .1, 2, 10);

void effect::drawsphere(vec &o, float radius, float size, int type, int elapse)
{
	if(size <= 0) return;

	size *= this->size;
	int fade, gravity, num;
	setvars(this, type, fade, gravity, num);
	num *= .1 * radius / (1 + size) * partmul * (elapse ? logf(elapse) / 3 : 1);
	if(elapse && !num && rnd(int(10 / partmul))) return; //sometimes particles should not be drawn
	num = max(1, num);

	switch(particle)
	{
		case PART_EXPLOSION:
		case PART_EXPLOSION_BLUE:
			particle_fireball(o, radius, particle, fade, colour, size);
			break;
		case PART_STREAK:
		case PART_LIGHTNING:
			if(!curtime) break;
			loopi(num)
			{
				vec offset = vec(rnd(360) * RAD, rnd(360) * RAD).mul(radius);
				vec offset2 = vec(rnd(360) * RAD, rnd(360) * RAD).mul(radius);
				particle_flare(vec(o).sub(offset), vec(o).add(offset2), fade, particle, colour, size);
			}
			break;
		default:
			loopi(num)
			{
				vec offset = vec(rnd(360) * RAD, rnd(360) * RAD).mul(radius);
				particle_splash(particle, 2, fade, offset.add(o), colour, size, max<int>(1, radius), gravity);
			}
			break;
	}
}

void effect::drawsplash(vec &o, vec dir, float radius, float size, int type, int elapse)
{
	if(size <= 0) return;

	size *= this->size;
	int fade, gravity, num;
	setvars(this, type, fade, gravity, num);
	num *= .1 * radius / (1 + size) * partmul * (elapse ? logf(elapse) / 3 : 1);
	if(elapse && !num && rnd(int(10 / partmul))) return; //sometimes particles should not be drawn
	num = max(1, num);

	switch(particle)
	{
		case PART_EXPLOSION:
		case PART_EXPLOSION_BLUE:
			particle_fireball(o, radius ? radius : size, particle, fade, colour, size);
			break;
		case PART_STREAK:
		case PART_LIGHTNING:
			if(!curtime) break;

			if(!radius) //assume num == 1
			{
				vec offset = vec(dir).mul(-2 * size).add(o);
				particle_flare(offset, o, fade, particle, colour, size);
			}
			else
			{
				num = (num / 2 + num % 2);
				loopi(num)
				{
					vec offset = vec(rnd(360) * RAD, (90 - rnd(180)) * RAD).mul(radius);
					vec offset2 = vec(rnd(360) * RAD, rnd(360) * RAD).mul(radius);
					particle_flare(vec(o).sub(offset), vec(o).add(offset2), fade, particle, colour, size);
				}
			}
			break;
		default:
			particle_splash(particle, num, fade, o, colour, size, max<int>(1, radius), gravity);
			break;
	}
}

VARP(linemaxsteps, 8, 32, 1024);
VARP(linemininterval, 1, 8, 32);

bool effect::drawline(vec &from, vec &to, float size, int type, int elapse)
{
	if(size <= 0) return false;

	size *= this->size;
	int fade, gravity, num;
	setvars(this, type, fade, gravity, num);
	num *= from.dist(to) / (10 * size) * partmul * (elapse ? logf(elapse) / 3 : 1);
	if(particle == PART_STREAK || particle == PART_LIGHTNING)
		num /= 2;

	num = min<int>(min(num, linemaxsteps), from.dist(to) / linemininterval);
	if(!num) return false;
	vec delta = vec(to).sub(from).div(num);

	loopi(num)
	{
		switch(particle)
		{
			case PART_EXPLOSION:
			case PART_EXPLOSION_BLUE:
				particle_fireball(from, size * 2, particle, fade, colour, size * 2);
				break;
			case PART_STREAK:
			case PART_LIGHTNING:
			{
				if(!curtime) return false;
				vec start = vec(rnd(360) * RAD, rnd(360) * RAD).mul(4 * size).add(from);
				vec end = vec(delta).mul(1.5).add(start);

				particle_flare(start, end, fade, particle, colour, size);
				break;
			}
			default:
				particle_splash(particle, 2, fade, from, colour, size, max<int>(1, size * 5), gravity);
				break;
		}
		from.add(delta);
	}

	return true;
}

void effect::drawwield(vec &from, vec &to, float size, int type, int elapse)
{
	if(size <= 0) return;

	if(particle == PART_STREAK || particle == PART_LIGHTNING)
	{
		drawline(from, to, size, type, elapse);
		return;
	}

	size *= this->size;
	int fade, gravity, num;
	setvars(this, type, fade, gravity, num);

	num *= partmul * (elapse ? (logf(elapse) / 3) : 1) / (1 + size);

	if(elapse && !num && rnd(int(10 / partmul))) return;
	num = max<int>(1, num);

	vec delta = vec(to).sub(from);

	loopi(num)
	{
		vec pt = vec(delta).mul((float) rnd(0x10000) / 0xFFFF).add(from);
		switch(particle)
		{
			case PART_EXPLOSION:
			case PART_EXPLOSION_BLUE:
				particle_fireball(pt, size * 2, particle, fade, colour, size * 4);
				break;
			default:
				particle_splash(particle,  2, fade, pt, colour, size, max<int>(1, size * 5), gravity);
		}
	}
}

void effect::drawaura(rpgent *d, float size, int type, int elapse)
{
	if(size <= 0) return;

	size *= this->size;
	int fade, gravity, num;
	setvars(this, type, fade, gravity, num);

	num *= .2 * PI * d->radius / (1 + size) * partmul * (elapse ? logf(elapse) / 3 : 1);
	if(elapse && !num && rnd(int(10 / partmul))) return; //sometimes particles should not be drawn
	num = max<int>(1, num);

	loopi(num)
	{
		vec pos = vec(rnd(360) * RAD, 0).mul(d->radius + size).add(d->feetpos());
		switch(particle)
		{
			case PART_EXPLOSION:
			case PART_EXPLOSION_BLUE:
				particle_fireball(pos, size, particle, fade, colour, size);
				break;
			case PART_STREAK:
			case PART_LIGHTNING:
				if(!curtime) return;
				particle_flare(pos, vec(0, 0, d->eyeheight + d->aboveeye).add(pos), fade, particle, colour, size);
				break;
			default:
				if(gravity >= 0)
					pos.add(vec(0, 0, d->eyeheight + d->aboveeye));

				particle_splash(particle, 2, fade, pos, colour, size, max<int>(1, size * 2), gravity);
				break;
		}
	}
}

void effect::drawcircle(vec &o, vec dir, vec &axis, int angle, float radius, float size, int type, int elapse)
{
	if(size <= 0) return;

	size *= this->size;
	int fade, gravity, num;
	setvars(this, type, fade, gravity, num);

	num *= angle * (particle == PART_STREAK || particle == PART_LIGHTNING ? 1 : partmul) /
		size / 30 * (elapse ? logf(elapse) / 3 : 1);
	num = max<int>(1, num);

	loopi(num)
	{
		vec ray = vec(dir).mul(radius);
		switch(particle)
		{
			case PART_EXPLOSION:
			case PART_EXPLOSION_BLUE:
				ray.mul(rnd(101)/100.f).add(o);
				particle_fireball(ray, size, particle, fade, colour, size);
				break;
			case PART_STREAK:
			case PART_LIGHTNING:
				if(!curtime) return;
				particle_flare(o, ray.add(o), fade, particle, colour, size);
				break;
			default:
				ray.mul(rnd(101)/100.f).add(o);
				particle_splash(particle, 2, fade, ray, colour, size, max<int>(1, size * 2), gravity);
				break;
		}
		dir.rotate(angle * RAD / num, axis);
	}
}

void effect::drawcircle(rpgent *d, use_weapon *wep, float size, int type, int elapse)
{
	vec axis;
	if(wep->target == T_HORIZ)
		axis = vec(d->yaw * RAD, (d->pitch + 90) * RAD);
	else
		axis = vec((d->yaw + 90) * RAD, 0);

	vec dir = vec(d->yaw * RAD, d->pitch * RAD).rotate(-wep->angle / 2.f * RAD, axis);
	vec o = vec(d->o).sub(vec(0, 0, d->eyeheight / 2));
	drawcircle(o, dir, axis, wep->angle, d->radius + wep->range, size, type, elapse);
}

void effect::drawcone(vec &o, vec dir, vec &axis, int angle, float radius, float size, int type, int elapse)
{
	if(size <= 0) return;

	size *= this->size;
	int fade, gravity, num;
	setvars(this, type, fade, gravity, num);

	num *= angle * (particle == PART_STREAK || particle == PART_LIGHTNING ? 1 : partmul) /
		size / 30 * (elapse ? logf(elapse) / 3 : 1);
	num = max<int>(1, num);

	loopi(num)
	{
		vec ray = vec(dir).rotate(rnd(angle + 1) * RAD, axis).rotate(rnd(360), dir).mul(radius);
		switch(particle)
		{
			case PART_EXPLOSION:
			case PART_EXPLOSION_BLUE:
				ray.mul(rnd(101)/100.f).add(o);
				particle_fireball(ray, size, particle, fade, colour, size);
				break;
			case PART_STREAK:
			case PART_LIGHTNING:
				if(!curtime) return;
				particle_flare(o, ray.add(o), fade, particle, colour, size);
				break;
			default:
				ray.mul(rnd(101)/100.f).add(o);
				particle_splash(particle, 2, fade, ray, colour, size, max<int>(1, size * 2), gravity);
				break;
		}
	}
}
void effect::drawcone(rpgent *d, use_weapon *wep, float size, int type, int elapse)
{
	vec dir = vec(d->yaw * RAD, d->pitch * RAD);
	vec axis = vec(d->yaw * RAD, (d->pitch + 90) * RAD);
	drawcone(d->o, dir, axis, wep->angle, d->radius + wep->range, size, type, elapse);
}
