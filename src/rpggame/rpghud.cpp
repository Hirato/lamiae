#include "rpggame.h"

namespace game
{
	bool needminimap() { return game::mapdata; }

	void quad(int x, int y, int xs, int ys)
	{
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2i(x,    y);
		glTexCoord2f(1, 0); glVertex2i(x+xs, y);
		glTexCoord2f(0, 1); glVertex2i(x,    y+ys);
		glTexCoord2f(1, 1); glVertex2i(x+xs, y+ys);
		glEnd();
	}

	float abovegameplayhud(int w, int h)
	{
		if(editmode || game::ispaused()) return 1;

		return 1350.f / 1600.f;
	}

	int clipconsole(int w, int h)
	{
		return 0;
	}

	const char *defaultcrosshair(int index)
	{
		switch(index)
		{
			case 2: return "packages/crosshairs/empty";
			case 1: return "packages/crosshairs/edit";
			default: return "packages/crosshairs/default";
		}
	}

	int selectcrosshair(float &r, float &g, float &b)
	{
		if(camera::cutscene) return 2;
		if(editmode)
		{
			r = b = 0.5;
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
		const int coords[4][2] = { {0,0}, {1, 0}, {1, 1}, {0,1}};
		if(!minimap || !d) return;
		float offset = min<float>(getworldsize() * minimapfrac / 2.0f, minimapmaxdist);

		glPushMatrix();
		glTranslatef(x + dx / 2, y + dy / 2, 0);
		glDisable(GL_BLEND);
		if(minimapup) glRotatef(d->yaw + 180, 0, 0, -1);

		if(curmap->flags & mapinfo::F_NOMINIMAP)
		{
			glColor3f(0, 0, 0);
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_TRIANGLE_FAN);
			loopi(16)
			{
				vec dir(M_PI / 8 * i, 0);
				glVertex2i(x + dir.x * dx / 2, y + dir.y * dy / 2);
			}
			glEnd();
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);

			settexture("data/rpg/hud/player", 3);
			glBegin(GL_TRIANGLE_FAN);
			loopi(4)
			{
				vec dir((d->yaw) * RAD + M_PI * i / 2.0f, 0);
				glTexCoord2f(coords[i][0], coords[i][1]);
				glVertex2i(dx / 16 * dir.x, dy / 16 * dir.y);
			}
			glEnd();
		}
		else
		{
			vec pos = vec(d->o).sub(minimapcenter).mul(minimapscale).add(.5f);
			glColor3f(1, 1, 1);
			bindminimap();
			glBegin(GL_TRIANGLE_FAN);
			loopi(16)
			{
				vec dir(M_PI / 8 * i, 0);
				glTexCoord2f(1.0f - (pos.x + dir.x * offset * minimapscale.x), pos.y + dir.y * offset * minimapscale.y);
				glVertex2i(dx * dir.x / 2, dy * dir.y / 2);
			}
			glEnd();
			glEnable(GL_BLEND);

			settexture("data/rpg/hud/player", 3);
			glBegin(GL_QUADS);
			loopv(curmap->objs)
			{
				rpgent *m = curmap->objs[i];
				vec pos = vec(m->o).sub(d->o).div(offset); pos.z = 0;
				vec col = m->blipcol();

				if(pos.magnitude() >= 1) continue;

				glColor4f(col.x, col.y, col.z, 3 - 3 * pos.magnitude());

				loopi(4)
				{
					vec dir((m->yaw) * RAD + M_PI * i / 2.0f, 0);
					glTexCoord2f(coords[i][0], coords[i][1]);
					glVertex2i(pos.x * dx / 2 + dx / 24 * dir.x, pos.y * dy / 2 + dy / 24 * dir.y);
				}
			}
			glEnd();

			settexture("data/rpg/hud/blip", 3);
			glBegin(GL_QUADS);
			loopv(curmap->projs)
			{
				projectile &p = *curmap->projs[i];
				vec pos = vec(p.o).sub(d->o).div(offset); pos.z = 0;
				if(pos.magnitude() >= 1)
					continue;

				glColor4f(p.owner != d, p.owner == d, 0, min<float>(1, 3 - 3 * pos.magnitude()));

				glTexCoord2i(0, 0);
				glVertex2i(pos.x * dx / 2 - dx / 64, pos.y * dy / 2 - dy / 64);
				glTexCoord2i(1, 0);
				glVertex2i(pos.x * dx / 2 + dx / 64, pos.y * dy / 2 - dy / 64);
				glTexCoord2i(1, 1);
				glVertex2i(pos.x * dx / 2 + dx / 64, pos.y * dy / 2 + dy / 64);
				glTexCoord2i(0, 1);
				glVertex2i(pos.x * dx / 2 - dx / 64, pos.y * dy / 2 + dy / 64);
			}

			//TODO loopv(curmap->blips) {}
			glEnd();
		}

		glColor3f(1, 1, 1);
		settexture("data/rpg/hud/compass", 3);
		quad(-dx / 2 - 2, -dy / 2 - 2, dx + 4, dy + 4);
		glPopMatrix();
	}
	ICOMMAND(r_hud_minimap, "sffff", (const char *r, float *x, float *y, float *dx, float *dy),
		int idx;
		rpgscript::parseref(r, idx);
		reference *ent = rpgscript::searchstack(r);
		drawminimap(ent->getent(idx), *x, *y, *dx, *dy);
	)

	void drawhorizbar(const char *img, float x, float y, float dx, float dy, float progress, int colour)
	{
		vec col((colour >> 16) & 255, (colour >> 8) & 255, colour & 255);
		col.div(255.f);

		settexture(*img ? img : "data/rpg/hud/hbar", 3);
		glBegin(GL_TRIANGLE_FAN);

		glColor3fv(col.v);
		glTexCoord2f(0, 0);        glVertex2i(x , y);
		glTexCoord2f(progress, 0); glVertex2i(x + dx * progress, y);
		glTexCoord2f(progress, 1); glVertex2i(x + dx * progress, y + dy);
		glTexCoord2f(0, 1);        glVertex2i(x, y + dy);

		glEnd();
	}
	ICOMMAND(r_hud_horizbar, "sfffffii", (const char *i, float *x, float *y, float *dx, float *dy, float *p, int *col),
		drawhorizbar(i, *x, *y, *dx, *dy, *p, *col);
	)

	void drawvertbar(const char *img, float x, float y, float dx, float dy, float progress, int colour)
	{
		vec col((colour >> 16) & 255, (colour >> 8) & 255, colour & 255);
		col.div(255.f);

		settexture(*img ? img : "data/rpg/hud/vbar", 3);
		glBegin(GL_TRIANGLE_FAN);

		glColor3fv(col.v);
		glTexCoord2f(0, 0);        glVertex2i(x , y);
		glTexCoord2f(1, 0);        glVertex2i(x + dx, y);
		glTexCoord2f(1, progress); glVertex2i(x + dx, y + dy * progress);
		glTexCoord2f(0, progress); glVertex2i(x, y + dy * progress);

		glEnd();
	}
	ICOMMAND(r_hud_vertbar, "sfffffi", (const char *i, float *x, float *y, float *dx, float *dy, float *p, int *col),
		drawvertbar(i, *x, *y, *dx, *dy, *p, *col);
	)

	void drawtext(float x, float y, float size, int colour, const char *str)
	{
		glPushMatrix();
		glTranslatef(x, y, 0);
		glScalef(size, size, size);

		vec col((colour >> 16) & 255, (colour >> 8) & 255, colour & 255);
		draw_text(str, 0, 0, col.x, col.y, col.z);

		glPopMatrix();
	}
	ICOMMAND(r_hud_text, "fffis", (float *x, float *y, float *sz, int *col, const char *s),
		drawtext(*x, *y, *sz, *col, s);
	)

	void drawimage(const char *img, float x, float y, float dx, float dy, int colour)
	{
		settexture(img, 3);
		vec col = vec((colour >> 16) & 255, (colour >> 8) & 255, colour & 255).div(255.f);

		glColor3fv(col.v);
		quad(x, y, dx, dy);
	}
	ICOMMAND(r_hud_image, "sffffi", (const char *i, float *x, float *y, float *dx, float *dy, int *c),
		drawimage(i, *x, *y, *dx, *dy, *c);
	)

	void drawsolid(float x, float y, float dx, float dy, int colour)
	{
		glDisable(GL_TEXTURE_2D);
		vec col = vec((colour >> 16) & 255, (colour >> 8) & 255, colour & 255).div(255.f);

		glColor3fv(col.v);
		quad(x, y, dx, dy);
		glEnable(GL_TEXTURE_2D);
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

	uint *hud = NULL;
	FVAR(hud_right, 1, 0, -1);
	FVAR(hud_bottom, 1, 0, -1);

	ICOMMAND(r_hud, "e", (uint *body),
		freecode(hud);
		keepcode((hud = body));
	)

	FVARP(hudlinescale, 0.1, 0.75, 1);

	void gameplayhud(int w, int h)
	{
		if(!mapdata || !curmap || editmode)
			return;

		glPushMatrix();
		float scale = min (w / 1600.0f, h / 1200.0f);
		glScalef(scale, scale, 1);

		hud_right = w / scale, hud_bottom = h / scale; // top and left are ALWAYS 0

		if(rpggui::open()) {}
		else if(camera::cutscene) camera::render(ceil(hud_right), ceil(hud_bottom));
		else if(hud) execute(hud);

		if(lines.length())
		{
			glPushMatrix();
			glTranslatef(0, 160, 0);

			glScalef(hudlinescale, hudlinescale, hudlinescale);
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

			glPopMatrix();
		}

		if(DEBUG_WORLD)
		{
			glPushMatrix();
			glTranslatef(hud_right - 300, 350, 0);
			glScalef(.5, .5, .5);

			draw_textf("curmap: %s", 0, 0, curmap->name);
			draw_textf("blips: %i", 0, 50, curmap->blips.length());
			draw_textf("entities: %i", 0, 100, curmap->objs.length());
			draw_textf("persistent effects: %i", 0, 150, curmap->aeffects.length());
			draw_textf("projectiles: %i", 0, 200, curmap->projs.length());

			glPopMatrix();
		}

		glPopMatrix();
	}
}
