#include "rpggame.h"

namespace camera
{
	static float linear(int startmillis, int duration, bool delta)
	{
		if(!duration) return 1;

		int elapsed;
		if(delta) elapsed = min(curtime, duration + startmillis - lastmillis);
		else elapsed = lastmillis - startmillis;

		return clamp((float)elapsed / duration, 0.f, 1.0f);
	}

	static float dlinear(int startmillis, int duration, bool delta)
	{
		if(!duration) return 0;

		if(delta) return linear(startmillis, duration, true);
		else return 1 - linear(startmillis, duration, false);
	}

	static float sine(int startmillis, int duration, bool delta)
	{
		if(!duration) return 1;
		if(delta)
		{
			float prev = sine(startmillis + curtime, duration, false);
			float next = sine(startmillis, duration, false);
			return next - prev;
		}
		else
		{
			int elapsed = min(lastmillis - startmillis, duration);
			return clamp(sinf(PI * elapsed / duration / 2), 0.f, 1.0f);
		}
	}

	static float sine2(int startmillis, int duration, bool delta)
	{
		if(!duration) return 1;
		if(delta)
		{
			float prev = sine2(startmillis + curtime, duration, false);
			float next = sine2(startmillis, duration, false);
			return next - prev;
		}
		else
		{
			float ret = sine(startmillis, duration, false);
			return ret * ret;
		}
	}

	static float cosine(int startmillis, int duration, bool delta)
	{
		if(!duration) return 0;
		if(delta)
		{
			float prev = cosine(startmillis + curtime, duration, false);
			float next = cosine(startmillis, duration, false);
			return prev - next;
		}
		else
		{
			int elapsed = min(lastmillis - startmillis, duration);
			return clamp(cosf(PI * elapsed / 2 / duration), 0.f, 1.0f);
		}
	}

	static float cosine2(int startmillis, int duration, bool delta)
	{
		if(!duration) return 0;
		if(delta)
		{
			float prev = cosine2(startmillis + curtime, duration, false);
			float next = cosine2(startmillis, duration, false);
			return prev - next;
		}
		else
		{
			float ret = cosine(startmillis, duration, false);
			return ret * ret;
		}
	}

	static float one(int startmillis, int duration, bool delta)
	{
		return (startmillis == lastmillis || !delta) ? 1 : 0;
	}

	struct action;

	bool cutscene = false;
	bool cancel = false;
	uint *post = NULL;
	physent camera;
	physent *attached = NULL;
	float (*interpfunc)(int, int, bool) = linear;
	vector<action *> pending;
	vector<action *> *actionvec = &pending;

	void cleanup(bool clear)
	{
		if(!clear && post)
			execute(post);

		cutscene = false;
        cancel = false;
		freecode(post); post = NULL;
		attached = NULL;
		interpfunc = linear;
		pending.deletecontents();
		actionvec = &pending;
	}

	struct action
	{
		int startmillis, duration;
		uint *successors;
		float (*interp)(int, int, bool);

		action(int d, uint *s) : startmillis(lastmillis), duration(d), successors(s), interp(interpfunc) { keepcode(successors); }
		virtual ~action() { freecode(successors); }

		virtual bool update()
		{
			if(lastmillis - startmillis >=duration)
			{
				if(successors)
				{
					float (*ib)(int, int, bool) = interpfunc;
					interpfunc = interp;
					execute(successors);
					interpfunc = ib;
				}
				return false;
			}
			return true;
		}
		virtual void render(int w, int h) {}
		virtual void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Delay/Generic", 100, 0 + hoffset);
				hoffset += 64;
			}

			draw_textf("Duration: %i (%f%%)", 100, 0 + hoffset, duration, (lastmillis - startmillis) * 100.f / duration);
			draw_textf("Successors: %p", 100, 64 + hoffset, successors);
			hoffset += 192; //3 * 64; an extra one so there's a bit of padding between entries
		}
		virtual void getsignal(const char *sig) {}
	};

	struct signal : action
	{
		const char *name;
		bool received;

		signal(const char *n, uint *s) : action(0, s), name(newstring(n)), received(false) {}
		~signal() {delete[] name;}

		bool update()
		{
			if(received)
			{
				if(successors)
				{
					float (*ib)(int, int, bool) = interpfunc;
					interpfunc = interp;
					execute(successors);
					interpfunc = ib;
				}
			}
			return true;
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Delay/Signal", 100, 0 + hoffset);
				hoffset += 64;
			}

			draw_textf("Successors: %p", 100, 64 + hoffset, successors);
			hoffset += 128;
		}

		void getsignal(const char *sig)
		{
			if(!strcmp(name, sig))
				received = true;
		}
	};

	void update()
	{
		if(!cutscene) return;

		attached = NULL;

		loopv(pending)
		{
			if(!pending[i]->update())
				delete pending.remove(i--);
		}

		if(!pending.length() || cancel)
			cleanup(false);
	}

	void render(int w, int h)
	{
		loopv(pending)
			pending[i]->render(w, h);

		int offset = 128;
		if(DEBUG_CAMERA) loopv(pending)
		{
			glPushMatrix();
			glScalef(.3, .3, 1);

			pending[i]->debug(offset, true);

			glPopMatrix();
		}
	}

	void getsignal(const char *signal)
	{
		loopv(pending)
			pending[i]->getsignal(signal);
	}

	struct move : action
	{
		float dx, dy, dz;

		move(float x, float y, float z, int d, uint *s) : action(d, s), dx(x), dy(y), dz(z) {}
		~move() {}

		bool update()
		{
			camera.o.add(vec(dx, dy, dz).mul(interp(startmillis, duration, true)));
			return action::update();
		}
		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Action/Move", 100, 0 + hoffset);
				hoffset += 64;
			}
			draw_textf("Delta: vec(%f, %f, %f)", 100, 0 + hoffset, dx, dy, dz);
			hoffset += 64;
			action::debug(hoffset, false);
		}
	};

	struct view : action
	{
		float dyaw, dpitch, droll;

		view(float y, float p, float r, int d, uint *s) : action(d, s), dyaw(y), dpitch(p), droll(r) {}
		~view() {}

		bool update()
		{
			float mul = interp(startmillis, duration, true);
			camera.yaw += dyaw * mul;
			camera.pitch += dpitch * mul;
			camera.roll += droll * mul;

			return action::update();
		}
		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Action/View", 100, 0 + hoffset);
				hoffset += 64;
			}
			draw_textf("DYaw: %f; DPitch: %f; DRoll: %f", 100, 0 + hoffset, dyaw, dpitch, droll);
			hoffset += 64;
			action::debug(hoffset, false);
		}
	};

	//NOTE, the next 3 actions depend on game specific methods for identifying entities
	struct focus : action
	{
		dynent *ent;
		float interp, lead;

		focus(dynent *e, float i, float l, int d, uint *s) : action(d, s), ent(e), interp(max<float>(0, i)), lead(l) {}
		~focus() {}

		bool update()
		{
			vec ideal = vec(ent->vel).add(ent->falling).mul(lead).add(ent->o).sub(camera.o).normalize();
			vec current = vec(camera.yaw * RAD, camera.pitch * RAD);
			float mul = 1;
			if(interp) mul = min<float>(1, (1 - ideal.dot(current)) * interp * curtime / 1000); // 1 - ideal.dot(current) == 0 when ideal == current

			ideal.mul(mul).add(current.mul(1 - mul));
			vectoyawpitch(ideal, camera.yaw, camera.pitch);

			return action::update();
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Action/Focus", 100, 0 + hoffset);
				hoffset += 64;
			}
			draw_textf("Target: %p", 100, 0 + hoffset, ent);
			draw_textf("Interpolation: %f", 100, 64 + hoffset, interp);
			draw_textf("Lead: %f", 100, 128 + hoffset, lead);
			hoffset += 192;

			action::debug(hoffset, false);
		}
	};

	struct follow : action
	{
		dynent *ent;
		float interp, tail, height;

		follow(dynent *e, float i, float t, float h, int d, uint *s) : action(d, s), ent(e), interp(max<float>(0, i)), tail(t), height(h) {}
		~follow() {}

		bool update()
		{
			vec pos = vec(ent->yaw * RAD, ent->pitch * RAD).mul(-tail).add(ent->o);
			pos.z += height;
			float mul = 1;
			if(interp) mul = min<float>(1, interp * curtime / 1000);
			camera.o.add(pos.sub(camera.o).mul(mul));

			return action::update();
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Action/Follow", 100, 0 + hoffset);
				hoffset += 64;
			}
			draw_textf("Target: %p", 100, 0 + hoffset, ent);
			draw_textf("Interp: %f", 100, 64 + hoffset, interp);
			draw_textf("Delta: <-x, -y> == %f; z = %f", 100, 128 + hoffset, tail, height);
			hoffset += 192;

			action::debug(hoffset, false);
		}
	};

	struct viewport : action
	{
		dynent *ent;

		viewport(dynent *e, int d, uint *s) : action(d, s), ent(e) {}
		~viewport() {}

		bool update()
		{
			attached = ent;
			camera = *ent;

			return action::update();
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Action/Viewport", 100, 0 + hoffset);
				hoffset += 64;
			}
			draw_textf("Target: %p", 100, 0 + hoffset);
			hoffset += 64;

			action::debug(hoffset, false);
		}
	};

	struct container : action
	{
		vector<action *> pending;

		container(uint *c, int d, uint *s) : action(d, s)
		{
			vector<action *> *vbak = actionvec;
			float (*fbak)(int, int, bool) = interpfunc;
			actionvec = &pending;
			interpfunc = interp;

			execute(c);

			interpfunc = fbak;
			actionvec = vbak;
		}
		~container() {pending.deletecontents();}

		bool update()
		{
			vector<action *> *vbak = actionvec;
			float (*fbak)(int, int, bool) = interpfunc;
			actionvec = &pending;
			interpfunc = interp;

			loopv(pending)
				if(!pending[i]->update())
					delete pending.remove(i--);

			interpfunc = fbak;
			actionvec = vbak;

			if(!pending.length())
			{
				if(successors)
				{
					float (*ib)(int, int, bool) = interpfunc;
					interpfunc = interp;
					execute(successors);
					interpfunc = ib;
				}
				return false;
			}
			return true;
		}
		void render(int w, int h)
		{
			loopv(pending)
				pending[i]->render(w, h);
		}
		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Container/Generic", 100, 0 + hoffset);
				hoffset += 64;
			}

			draw_textf("Children: %i", 100, 0 + hoffset, pending.length());
			hoffset += 64;
			action::debug(hoffset, false); //useless here, probably useful for derived classes

			glPushMatrix();
			glTranslatef(128, 0, 0);

			loopv(pending)
				pending[i]->debug(hoffset, true);

			glPopMatrix();
		}

		void getsignal(const char *signal)
		{
			vector<action *> *vbak = actionvec;
			float (*fbak)(int, int, bool) = interpfunc;
			actionvec = &pending;
			interpfunc = interp;

			loopv(pending)
				pending[i]->getsignal(signal);

			interpfunc = fbak;
			actionvec = vbak;
		}
	};

	struct translate : container
	{
		float dx, dy;

		translate(float x, float y, uint *c, int d, uint *s) : container(c, d, s), dx(x), dy(y) {}
		~translate() {}

		void render(int w, int h)
		{
			float mul = interp(startmillis, duration, false);

			glPushMatrix();
			glTranslatef(dx * mul, dy * mul, 0);
			container::render(w, h);
			glPopMatrix();
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Container/Translate", 100, 0 + hoffset);
				hoffset += 64;
			}
			draw_textf("Delta: <%f, %f>", 100, 0 + hoffset, dx, dy);
			hoffset += 64;

			container::debug(hoffset, false);
		}
	};

	struct scale : container
	{
		float dx, dy;

		scale(float x, float y, uint *c, int d, uint *s) : container(c, d, s), dx(x), dy(y) {}
		~scale() {}

		void render(int w, int h)
		{
			float mul = interp(startmillis, duration, false);
			vec d = vec(dx, dy, 1).mul(mul).add(1 - mul);

			glPushMatrix();
			glScalef(d.x, d.y, d.z);
			container::render(w, h);
			glPopMatrix();
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Container/Scale", 100, 0 + hoffset);
				hoffset += 64;
			}
			draw_textf("Delta: <%f, %f>", 100, 0 + hoffset, dx, dy);
			hoffset += 64;

			container::debug(hoffset, false);
		}
	};

	//REMINDER: glRotate uses degrees
	struct rotate : container
	{
		float angle;

		rotate(float a, uint *c, int d, uint *s) : container(c, d, s), angle(a) {}
		~rotate() {}

		void render(int w, int h)
		{
			glPushMatrix();
			glRotatef(0, 0, -1, angle * interp(startmillis, duration, false));
			container::render(w, h);
			glPopMatrix();
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Container/Rotation", 100, 0 + hoffset);
				hoffset += 64;
			}
			draw_textf("Angle: %f", 100, 0 + hoffset, angle);
			hoffset += 64;

			container::debug(hoffset, false);
		}
	};

	struct text : action
	{
		float x, y, width;
		int colour; //this should probably be unsigned...
		const char *str;

		text(float x, float y, float w, int c, const char *t, int d, uint *s) : action(d, s), x(x), y(y), width(w), colour(c), str((t && *t) ? newstring(t) : NULL) {}
		~text() {delete[] str;}

		void render(int w, int h)
		{
			int a = (colour >> 24) & 255,
				r = (colour >> 16) & 255,
				g = (colour >> 8) & 255,
				b = colour & 255;
			a = (a ? a : 255) * interp(startmillis, duration, false);

			draw_text(str, x, y, r, g, b, a, -1, width);
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Element/Text", 100, 0 + hoffset);
				hoffset += 64;
			}
			draw_textf("Origin: <%f, %f>", 100, 0 + hoffset, x, y);
			draw_textf("Width: %f", 100, 64 + hoffset, width);
			draw_textf("Colour: %.8X", 100, 128 + hoffset, colour);
			hoffset += 192;

			action::debug(hoffset, false);
		}
	};

	struct image : action
	{
		float x, y, dx, dy;
		int colour;
		const char *img;

		image(float x, float y, float dx, float dy, int c, const char *i, int d, uint *s) : action(d, s), x(x), y(y), dx(dx), dy(dy), colour(c), img((i && *i) ? newstring(i) : NULL) {}
		~image() {delete[] img;}

		void render(int w, int h)
		{
			if(!img) return;

			int a = (colour >> 24) & 255,
				r = (colour >> 16) & 255,
				g = (colour >> 8) & 255,
				b = colour & 255;
			a = (a ? a : 255) * interp(startmillis, duration, false);
			settexture(img, true);

			glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
			glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2i(0, 0); glVertex2f(x, y);
			glTexCoord2i(1, 0); glVertex2f(x + dx, y);
			glTexCoord2i(0, 1); glVertex2f(x, y + dy);
			glTexCoord2i(1, 1); glVertex2f(x + dx, y + dy);
			glEnd();
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Element/Image", 100, 0 + hoffset);
				hoffset += 64;
			}

			draw_textf("Origin: <%f, %f>", 100, 0 + hoffset, x, y);
			draw_textf("Size/Span: <%f, %f>", 100, 64 + hoffset, dx, dy);
			draw_textf("Colour: %.8X", 100, 128 + hoffset, colour);
			draw_textf("Path: %s", 100, 192 + hoffset, img);
			hoffset += 256;

			action::debug(hoffset, false);
		}
	};

	struct solid : action
	{
		float x, y, dx, dy;
		int colour;
		bool modulate;

		solid(float x, float y, float dx, float dy, int c, bool m, int d, uint *s) : action(d, s), x(x), y(y), dx(dx), dy(dy), colour(c), modulate(m) {}
		~solid() {}

		void render(int w, int h)
		{
			int a = (colour >> 24) & 255,
				r = (colour >> 16) & 255,
				g = (colour >> 8) & 255,
				b = colour & 255;
			a = (a ? a : 255) * interp(startmillis, duration, false);
			glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);

			if(modulate) glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			glDisable(GL_TEXTURE_2D);
			setnotextureshader();

			glBegin(GL_TRIANGLE_STRIP);
			glVertex2i(x, y);
			glVertex2i(x + dx, y);
			glVertex2i(x, y + dy);
			glVertex2i(x + dx, y + dy);
			glEnd();

			if(modulate) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_TEXTURE_2D);
			setdefaultshader();
		}

		void debug(int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Element/Solid", 100, 0 + hoffset);
				hoffset += 64;
			}

			draw_textf("Origin: <%f, %f>", 100, 0 + hoffset, x, y);
			draw_textf("Size/Span: <%f, %f>", 100, 64 + hoffset, dx, dy);
			draw_textf("Colour: %.8X", 100, 128 + hoffset, colour);
			draw_textf("Modulate: %i", 100, 192 + hoffset, modulate);
			hoffset += 256;

			action::debug(hoffset, false);
		}
	};

	#define INTERPCMD(name, func) \
		ICOMMAND(cs_interp_ ## name, "e", (uint *body), \
			if(!cutscene) return; \
			 \
			float (*ib)(int, int, bool) = interpfunc; \
			interpfunc = func; \
			execute(body); \
			interpfunc = ib; \
		)

	INTERPCMD(linear, linear);
	INTERPCMD(dlinear, dlinear);
	INTERPCMD(sin, sine);
	INTERPCMD(sin2, sine2);
	INTERPCMD(cos, cosine);
	INTERPCMD(cos2, cosine2);
	INTERPCMD(one, one);

	//note that there's a limit of 8 arguments (see engine/command.cpp), use wisely and design the above with that limit in mind

	#define GETCAM \
	extentity *cam = NULL; \
	loopv(entities::ents) \
	{ \
		if(entities::ents[i]->type == CAMERA && entities::ents[i]->attr[0] == *tag) \
			cam = entities::ents[i]; \
	}

	#define GETREF(var, ref) \
		reference *var = NULL; \
		int var ## idx; \
		rpgscript::parseref(ref, var ## idx); \
		if(*ref) var = rpgscript::searchstack(ref);

	ICOMMAND(cs_start, "s", (const char *s),
		//reuse camera state
		if(cutscene)
		{
			if(post) execute(post);
			freecode(post);
			post = NULL;
		}
		else
		{
			camera = *game::player1;
		}

		static string file;
		formatstring(file)("data/rpg/games/%s/cutscenes/%s.cfg", game::data, s);
		cutscene = true;
		cancel = false;
		execfile(file);
	)

	//ICOMMAND(cs_interrupt, "D", (int *down),
	ICOMMAND(cs_interrupt, "", (),
		if(cutscene) cancel = true;
		intret(cancel);
	)

	ICOMMAND(cs_post, "e", (uint *body),
		freecode(post);
		keepcode((post = body));
	)

	ICOMMAND(cs_action_generic, "ie", (int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new action(*d, s));
	)

	ICOMMAND(cs_action_signal, "se", (const char *sig, uint *s),
		if(!cutscene) return;
		actionvec->add(new signal(sig, s));
	)

	ICOMMAND(cs_action_movedelta, "fffie", (float *x, float *y, float *z, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new move(*x, *y, *z, *d, s));
	)

	ICOMMAND(cs_action_moveset, "fffie", (float *x, float *y, float *z, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new move(*x - camera.o.x, *y - camera.o.y, *z - camera.o.z, *d, s));
	)

	ICOMMAND(cs_action_movecamera, "iie", (int *tag, int *d, uint *s),
		if(!cutscene) return;

		GETCAM
		if(cam)
		{
			vec delta = vec(cam->o).sub(camera.o);
			actionvec->add(new move(delta.x, delta.y, delta.z, *d, s));
		}
		else
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr camera entity %i does not exist, using dummy action instead of move", *tag);
			actionvec->add(new action(*d, s));
		}
	)

	ICOMMAND(cs_action_moveent, "sie", (const char *r, int *d, uint *s),
		if(!cutscene) return;

		GETREF(ent, r)
		if(ent && ent->getent(entidx))
		{
			vec delta = vec(ent->getent(entidx)->o).sub(camera.o);
			actionvec->add(new move(delta.x, delta.y, delta.z, *d, s));
		}
		else
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr reference \'%s\' does not exist or is not an entity, using dummy action instead of move", r);
			actionvec->add(new action(*d, s));
		}
	)

	ICOMMAND(cs_action_viewdelta, "fffie", (float *y, float *p, float *r, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new view(*y, *p, *r, *d, s));
	)

	ICOMMAND(cs_action_viewset, "fffie", (float *y, float *p, float *r, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new view(*y - camera.yaw, *p - camera.pitch, *r - camera.roll, *d, s));
	)

	ICOMMAND(cs_action_viewcamera, "iie", (int *tag, int *d, uint *s),
		if(!cutscene) return;

		GETCAM
		if(cam)
		{
			vec delta = vec(cam->attr[1], cam->attr[2], cam->attr[3]).sub(vec(camera.yaw, camera.pitch, camera.roll));
			actionvec->add(new view(delta.x, delta.y, delta.z, *d, s));
		}
		else
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr camera entity %i does not exist, using dummy action instead of view", *tag);
			actionvec->add(new action(*d, s));
		}
	)

	ICOMMAND(cs_action_viewent, "sie", (const char *r, int *d, uint *s),
		if(!cutscene) return;

		GETREF(ent, r)
		if(ent && ent->getent(entidx))
		{
			vec delta = vec(ent->getent(entidx)->yaw, ent->getent(entidx)->pitch, ent->getent(entidx)->roll).sub(vec(camera.yaw, camera.pitch, camera.roll));
			actionvec->add(new view(delta.x, delta.y, delta.z, *d, s));
		}
		else
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr reference \'%s\' does not exist or is not an entity, using dummy action instead of view", r);
			actionvec->add(new action(*d, s));
		}
	)

	ICOMMAND(cs_action_focus, "sffie", (const char *r, float *i, float *l, int *d, uint *s),
		if(!cutscene) return;

		GETREF(ent, r)
		if(ent && ent->getent(entidx))
		{
			actionvec->add(new focus(ent->getent(entidx), *i, *l, *d, s));
		}
		else
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr reference \'%s\' does not exist or is not an entity, using dummy action instead of focus", r);
			actionvec->add(new action(*d, s));
		}
	)

	ICOMMAND(cs_action_follow, "sfffie", (const char *r, float *i, float *t, float *h, int *d, uint *s),
		if(!cutscene) return;

		GETREF(ent, r)
		if(ent && ent->getent(entidx))
		{
			actionvec->add(new follow(ent->getent(entidx), *i, *t, *h, *d, s));
		}
		else
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr reference \'%s\' does not exist or is not an entity, using dummy action instead of follow", r);
			actionvec->add(new action(*d, s));
		}
	)

	ICOMMAND(cs_action_viewport, "sie", (const char *r, int *d, uint *s),
		if(!cutscene) return;

		GETREF(ent, r)
		if(ent && ent->getent(entidx))
		{
			actionvec->add(new viewport(ent->getent(entidx), *d, s));
		}
		else
		{
			conoutf(CON_ERROR, "\fs\f3ERROR:\fr reference \'%s\' does not exist or is not an entity, using dummy action instead of viewport", r);
			actionvec->add(new action(*d, s));
		}
	)

	ICOMMAND(cs_container_generic, "eie", (uint *c, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new container(c, *d, s)); //useful for grouping things into layers, eg, a background layer and a subtitle layer
	)

	ICOMMAND(cs_container_translate, "ffeie", (float *x, float *y, uint *c, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new translate(*x, *y, c, *d, s));
	)

	ICOMMAND(cs_container_scale, "ffeie", (float *x, float *y, uint *c, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new scale(*x, *y, c, *d, s));
	)

	ICOMMAND(cs_container_rotate, "feie", (float *a, uint *c, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new rotate(*a, c, *d, s));
	)

	ICOMMAND(cs_element_text, "fffisie", (float *x, float *y, float *w, int *c, const char *t, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new text(*x, *y, *w, *c, t, *d, s));
	)

	ICOMMAND(cs_element_image, "ffffisie", (float *x, float *y, float *dx, float *dy, int *c, const char *p, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new image(*x, *y, *dx, *dy, *c, p, *d, s));
	)

	ICOMMAND(cs_element_solid, "ffffiiie", (float *x, float *y, float *dx, float *dy, int *c, int *m, int *d, uint *s),
		if(!cutscene) return;

		actionvec->add(new solid(*x, *y, *dx, *dy, *c, *m, *d, s));
	)
}

namespace game
{
	bool recomputecamera(physent *&camera1, physent &tempcamera, bool &detachedcamera, float &thirdpersondistance)
	{
		if(camera::cutscene)
		{
			camera1 = &tempcamera;
			tempcamera = camera::camera;
			detachedcamera = true;

			return true;
		}
		return false;
	}
}

#if 0
	struct sound : action
	{
		int ind;

		sound(int i, int d, const char *s) : action(d, s), ind(i)
		{
			playsound(ind, NULL, NULL, 0, 0, -1, 0, d);
		}
		~sound() {}

		void debug(int w, int h, int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Sound", 100, 0 + hoffset);
				hoffset += 64;
			}

			draw_textf("Index: %i", 100, 0 + hoffset, ind);
			hoffset += 64;
			action::debug(w, h, hoffset);
		}
	};

	struct soundfile : action
	{
		const char *path;

		soundfile(const char *p, int d, const char *s) : action(d, s), path(newstring(p))
		{
			playsoundname(path, NULL, 0, 0, 0, -1, 0, d);
		}
		~soundfile() {delete[] path;}

		void debug(int w, int h, int &hoffset, bool type)
		{
			if(type)
			{
				draw_text("Type: Sound File", 100, 0 + hoffset);
				hoffset += 64;
			}

			draw_textf("Path: %s", 100, 0 + hoffset, path);
			hoffset += 64;
			action::debug(w, h, hoffset);
		}
	};
#endif
