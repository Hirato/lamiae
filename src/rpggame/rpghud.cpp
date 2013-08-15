#include "rpggame.h"

namespace game
{
	bool needminimap() { return game::connected; }

	void quad(int x, int y, int xs, int ys)
	{
		gle::begin(GL_TRIANGLE_STRIP);
		gle::attribf(x,    y);    gle::attribf(0, 0);
		gle::attribf(x+xs, y);    gle::attribf(1, 0);
		gle::attribf(x,    y+ys); gle::attribf(0, 1);
		gle::attribf(x+xs, y+ys); gle::attribf(1, 1);
		gle::end();
	}

	float abovegameplayhud(int w, int h)
	{
		if(editmode || game::ispaused()) return 1;

		return 1350.f / 1600.f;
	}

	float clipconsole(int w, int h)
	{
		return 0;
	}

	const char *defaultcrosshair(int index)
	{
		switch(index)
		{
			case 2: return "media/crosshairs/empty";
			case 1: return "media/crosshairs/edit";
			default: return "media/crosshairs/default";
		}
	}

	int selectcrosshair(vec &color)
	{
		if(camera::cutscene) return 2;
		if(editmode)
		{
			color = vec(.5, 1, .5);
			return 1;
		}
		return 0;
	}

	VARP(minimapmaxdist, 128, 1536, 16384);
	FVARP(minimapfrac, .1, .5, 1);
	VARP(minimapup, 0, 1, 1);
	VARP(minimap, 0, 1, 1);

	void drawminimap(rpgent *d, float x, float y, float dx, float dy)
	{
		const float coords[4][2] = { {0,0}, {1, 0}, {1, 1}, {0,1}};
		if(!minimap || !d) return;
		float offset = min<float>(getworldsize() * minimapfrac / 2.0f, minimapmaxdist);

		pushhudmatrix();
		hudmatrix.translate(x + dx / 2, y + dy / 2, 0);
		if(minimapup) hudmatrix.rotate((d->yaw + 180) * RAD, vec(0, 0, -1));
		flushhudmatrix();

		if(curmap->flags & mapinfo::F_NOMINIMAP)
		{
			gle::colorf(0, 0, 0);
			gle::defvertex(2);
			sethudnotextureshader();

			gle::begin(GL_TRIANGLE_FAN);
			loopi(16)
			{
				vec dir(M_PI / 8 * i, 0);
				gle::attribf(x + dir.x * dx / 2, y + dir.y * dy / 2);
			}
			gle::end();

			sethudshader();
			gle::colorf(1, 1, 1);
			gle::defvertex(2);
			gle::deftexcoord0();

			settexture("media/icons/player", 3);
			gle::begin(GL_TRIANGLE_FAN);
			loopi(4)
			{
				vec dir((d->yaw) * RAD + M_PI * i / 2.0f, 0);

				gle::attribf(dx / 16 * dir.x, dy / 16 * dir.y);
				gle::attrib(coords[i]);
			}
			gle::end();
		}
		else
		{
			vec pos = vec(d->o).sub(minimapcenter).mul(minimapscale).add(.5f);
			gle::colorf(1, 1, 1);
			bindminimap();

			gle::begin(GL_TRIANGLE_FAN);
			loopi(16)
			{
				vec dir(M_PI / 8 * i, 0);
				gle::attribf(dx * dir.x / 2, dy * dir.y / 2);
				gle::attribf(1.0f - (pos.x + dir.x * offset * minimapscale.x), pos.y + dir.y * offset * minimapscale.y);
			}
			gle::end();

			gle::defvertex(2);
			gle::deftexcoord0();
			gle::defcolor(4);

			settexture("media/icons/player", 3);
			gle::begin(GL_QUADS);
			loopv(curmap->objs)
			{
				rpgent *m = curmap->objs[i];
				vec pos = vec(m->o).sub(d->o).div(offset); pos.z = 0;
				if(pos.magnitude() >= 1) continue;

				vec4 col = vec4(m->blipcol(), min<float>(1, 3 - 3 * pos.magnitude()));

				loopi(4)
				{
					vec dir((m->yaw) * RAD + M_PI * i / 2.0f, 0);
					gle::attribf(pos.x * dx / 2 + dx / 24 * dir.x, pos.y * dy / 2 + dy / 24 * dir.y);
					gle::attribf(coords[i][0], coords[i][1]);
					gle::attrib(col);
				}
			}
			gle::end();

			settexture("media/icons/blip", 3);
			gle::begin(GL_QUADS);
			loopv(curmap->projs)
			{
				projectile &p = *curmap->projs[i];
				vec pos = vec(p.o).sub(d->o).div(offset); pos.z = 0;
				if(pos.magnitude() >= 1)
					continue;

				vec4 col = vec4(p.owner != d, p.owner == d, 0, min<float>(1, 3 - 3 * pos.magnitude()));

				gle::attribf(pos.x * dx / 2 - dx / 64, pos.y * dy / 2 - dy / 64);
				gle::attribf(0, 0);
				gle::attrib(col);
				gle::attribf(pos.x * dx / 2 + dx / 64, pos.y * dy / 2 - dy / 64);
				gle::attribf(1, 0);
				gle::attrib(col);
				gle::attribf(pos.x * dx / 2 + dx / 64, pos.y * dy / 2 + dy / 64);
				gle::attribf(1, 1);
				gle::attrib(col);
				gle::attribf(pos.x * dx / 2 - dx / 64, pos.y * dy / 2 + dy / 64);
				gle::attribf(0, 1);
				gle::attrib(col);
			}

			//TODO loopv(curmap->blips) {}
			gle::end();

			gle::defvertex(2);
			gle::deftexcoord0();
			gle::disable();
		}

		gle::colorf(1, 1, 1);

		settexture("media/icons/compass", 3);
		quad(-dx / 2 - 2, -dy / 2 - 2, dx + 4, dy + 4);

		pophudmatrix();
	}
	ICOMMAND(r_hud_minimap, "sffff", (const char *r, float *x, float *y, float *dx, float *dy),
		int idx;
		rpgscript::parseref(r, idx);
		reference *ent = rpgscript::searchstack(r);
		drawminimap(ent->getent(idx), *x, *y, *dx, *dy);
	)

	void drawhorizbar(const char *img, float x, float y, float dx, float dy, float progress, int colour)
	{
		settexture(*img ? img : "media/icons/hbar", 3);
		gle::colorub((colour >> 16) & 255, (colour >> 8) & 255, colour & 255);

		gle::begin(GL_TRIANGLE_FAN);
		gle::attribf(x , y);                     gle::attribf(0, 0);
		gle::attribf(x + dx * progress, y);      gle::attribf(progress, 0);
		gle::attribf(x + dx * progress, y + dy); gle::attribf(progress, 1);
		gle::attribf(x, y + dy);                 gle::attribf(0, 1);
		gle::end();
	}
	ICOMMAND(r_hud_horizbar, "sfffffii", (const char *i, float *x, float *y, float *dx, float *dy, float *p, int *col),
		drawhorizbar(i, *x, *y, *dx, *dy, *p, *col);
	)

	void drawvertbar(const char *img, float x, float y, float dx, float dy, float progress, int colour)
	{
		settexture(*img ? img : "media/icons/vbar", 3);
		gle::colorub((colour >> 16) & 255, (colour >> 8) & 255, colour & 255);

		gle::begin(GL_TRIANGLE_FAN);
		gle::attribf(x , y);                     gle::attribf(0, 0);
		gle::attribf(x + dx, y);                 gle::attribf(1, 0);
		gle::attribf(x + dx, y + dy * progress); gle::attribf(1, progress);
		gle::attribf(x, y + dy * progress);      gle::attribf(0, progress);
		gle::end();
	}
	ICOMMAND(r_hud_vertbar, "sfffffi", (const char *i, float *x, float *y, float *dx, float *dy, float *p, int *col),
		drawvertbar(i, *x, *y, *dx, *dy, *p, *col);
	)

	void drawtext(float x, float y, float size, int colour, const char *str)
	{
		pushhudmatrix();
		hudmatrix.translate(x, y, 0);
		hudmatrix.scale(size, size, size);
		flushhudmatrix();

		vec col((colour >> 16) & 255, (colour >> 8) & 255, colour & 255);
		draw_text(str, 0, 0, col.x, col.y, col.z);

		pophudmatrix();
	}
	ICOMMAND(r_hud_text, "fffis", (float *x, float *y, float *sz, int *col, const char *s),
		drawtext(*x, *y, *sz, *col, s);
	)

	void drawimage(const char *img, float x, float y, float dx, float dy, int colour)
	{
		settexture(img, 3);
		gle::colorub((colour >> 16) & 255, (colour >> 8) & 255, colour & 255);

		quad(x, y, dx, dy);
	}
	ICOMMAND(r_hud_image, "sffffi", (const char *i, float *x, float *y, float *dx, float *dy, int *c),
		drawimage(i, *x, *y, *dx, *dy, *c);
	)

	void drawsolid(float x, float y, float dx, float dy, int colour)
	{
		gle::colorub((colour >> 16) & 255, (colour >> 8) & 255, colour & 255);
		gle::defvertex(2);

		gle::begin(GL_TRIANGLE_STRIP);
		gle::attribf(x,    y);
		gle::attribf(x+dx, y);
		gle::attribf(x,    y+dy);
		gle::attribf(x+dx, y+dy);
		gle::end();

		gle::defvertex(2);
		gle::deftexcoord0();
	}

	ICOMMAND(r_hud_solid, "ffffi", (float *x, float *y, float *dx, float *dy, int *c),
		drawsolid(*x, *y, *dx, *dy, *c);
	)

	struct line
	{
		int start;
		const char *text;

		line(const char *t) : start(totalmillis), text(newstring(t)) {}
		~line() { delete[] text; }
	};

	vector<line *> lines;

	void hudline(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		static char buf[512];
		vformatstring(buf, fmt, args, 512);
		filtertext(buf, buf);
		lines.add(new line(buf));
		conoutf(CON_INFO, "%s", buf);

		va_end(args);
	}

	ICOMMAND(hudline, "C", (const char *t), hudline("%s", t));

	struct hudset
	{
		const char *name;
		uint *code;

		hudset(const char *n, uint *c) : name(newstring(n)), code(c) {}
		~hudset() { delete[] name; freecode(code); }
	};

	vector<hudset *> huds;
	FVAR(hud_right, 1, 0, -1);
	FVAR(hud_bottom, 1, 0, -1);

	ICOMMAND(r_hud, "se", (const char *name, uint *body),
		keepcode(body);
		loopv(huds)	if(!strcmp(huds[i]->name, name))
		{
			freecode(huds[i]->code);
			huds[i]->code = body;
			return;
		}
		huds.add(new hudset(name, body));
	)

	void clearhuds()
	{
		huds.deletecontents();
	}
	COMMANDN(r_clearhud, clearhuds, "");

	FVARP(hudlinescale, 0.1, 0.75, 1);

	void gameplayhud(int w, int h)
	{
		if(!connected || !curmap || editmode)
			return;

		gle::colorf(1, 1, 1);
		gle::defvertex(2);
		gle::deftexcoord0();

		float scale = min (w / 1600.0f, h / 1200.0f);
		pushhudmatrix();
		hudmatrix.scale(scale, scale, 1);
		flushhudmatrix();

		hud_right = w / scale, hud_bottom = h / scale; // top and left are ALWAYS 0

		if(rpggui::open()) {}
		else if(camera::cutscene) camera::render(ceil(hud_right), ceil(hud_bottom));
		else loopv(huds) rpgexecute(huds[i]->code);

		if(lines.length())
		{
			pushhudmatrix();
			hudmatrix.translate(0, 160, 0);
			hudmatrix.scale(hudlinescale, hudlinescale, hudlinescale);
			flushhudmatrix();

			float width = hud_right / hudlinescale;

			float textoffset = 0;
			loopv(lines)
			{
				int elapsed = totalmillis - lines[i]->start;
				if(elapsed > 4000)
				{
					delete lines.remove(i--);
					continue;
				}

				int alpha = 255;
				if(elapsed < 300)
					alpha = 255 * elapsed / 300;
				if(elapsed > 3700)
					alpha = 255 * (4000 - elapsed) / 300;

				float w, h;
				text_boundsf(lines[i]->text, w, h, width * .9);
				w = (width * 0.9 - w) / 2;
				draw_text(lines[i]->text, width * 0.05 + w, textoffset, 255, 224, 128, alpha, -1, width * 0.9);
				textoffset += h * alpha / 255;
			}

			pophudmatrix();
		}

		if(DEBUG_WORLD)
		{
			pushhudmatrix();
			hudmatrix.translate(hud_right - 300, 350, 0);
			hudmatrix.scale(.5, .5, .5);
			flushhudmatrix();

			draw_textf("curmap: %s", 0, 0, curmap->name);
			draw_textf("blips: %i", 0, 50, curmap->blips.length());
			draw_textf("entities: %i", 0, 100, curmap->objs.length());
			draw_textf("persistent effects: %i", 0, 150, curmap->aeffects.length());
			draw_textf("projectiles: %i", 0, 200, curmap->projs.length());

			pophudmatrix();
		}

		pophudmatrix();
	}
}
