#include "engine.h"
#include "textedit.h"

namespace UI
{
    struct delayedupdate
    {
        enum
        {
            INT,
            FLOAT,
            STRING,
            ACTION
        } type;
        ident *id;
        union
        {
            int i;
            float f;
            char *s;
            uint *c;
        } val;
        delayedupdate() : type(ACTION), id(NULL) { val.s = NULL; }
        ~delayedupdate()
        {
            if(type == STRING) delete[] val.s;
            if(type == ACTION) freecode(val.c);
        }

        void schedule(uint *code) { type = ACTION; val.c = code; keepcode(code); }
        void schedule(ident *var, int i) { type = INT; id = var; val.i = i; }
        void schedule(ident *var, float f) { type = FLOAT; id = var; val.f = f; }
        void schedule(ident *var, char *s) { type = STRING; id = var; val.s = newstring(s); }

        int getint() const
        {
            switch(type)
            {
                case INT: return val.i;
                case FLOAT: return int(val.f);
                case STRING: return int(strtol(val.s, NULL, 0));
                default: return 0;
            }
        }

        float getfloat() const
        {
            switch(type)
            {
                case INT: return float(val.i);
                case FLOAT: return val.f;
                case STRING: return float(parsefloat(val.s));
                default: return 0;
            }
        }

        const char *getstring() const
        {
            switch(type)
            {
                case INT: return intstr(val.i);
                case FLOAT: return intstr(int(floor(val.f)));
                case STRING: return val.s;
                default: return "";
            }
        }

        void run()
        {
            if(type == ACTION) { if(val.c) execute(val.c); }
            else if(id) switch(id->type)
            {
                case ID_VAR: setvarchecked(id, getint()); break;
                case ID_FVAR: setfvarchecked(id, getfloat()); break;
                case ID_SVAR: setsvarchecked(id, getstring()); break;
                case ID_ALIAS: alias(id->name, getstring()); break;
            }
        }
    };

    static vector<delayedupdate> updatelater;
    template<class T> static void updateval(ident *id, T val, uint *onchange)
    {
        updatelater.add().schedule(id, val);
        updatelater.add().schedule(onchange);
    }
    ICOMMAND(updateval, "rse", (ident *var, char *value, uint *onchange),
    {
        updateval(var, value, onchange);
    });

#if 0
    static int getval(char *var)
    {
        ident *id = readident(var);
        if(!id) return 0;
        switch(id->type)
        {
            case ID_VAR: return *id->storage.i;
            case ID_FVAR: return int(*id->storage.f);
            case ID_SVAR: return parseint(*id->storage.s);
            case ID_ALIAS: return id->getint();
            default: return 0;
        }
    }
#endif

    struct Object;

    Object *selected = NULL, *hovering = NULL, *focused = NULL, *tooltip = NULL;
    float hoverx = 0, hovery = 0, selectx = 0, selecty = 0;
    float cursorx = 0.5f, cursory = 0.5f;

    static inline bool isselected(const Object *o) { return o==selected; }
    static inline bool ishovering(const Object *o) { return o==hovering; }
    static inline bool isfocused(const Object *o) { return o==focused; }

    static void setfocus(Object *o) { focused = o; }
    static inline void clearfocus(const Object *o)
    {
        if(o==selected) selected = NULL;
        if(o==hovering) hovering = NULL;
        if(o==focused) focused = NULL;
    }

    static inline void quad(float x, float y, float w, float h, float tx = 0, float ty = 0, float tw = 1, float th = 1)
    {
        gle::attribf(x    , y    ); gle::attribf(tx     , ty     );
        gle::attribf(x + w, y    ); gle::attribf(tx + tw, ty     );
        gle::attribf(x + w, y + h); gle::attribf(tx + tw, ty + th);
        gle::attribf(x    , y + h); gle::attribf(tx     , ty + th);
    }

    static inline void quadtri(float x, float y, float w, float h, float tx = 0, float ty = 0, float tw = 1, float th = 1)
    {
        gle::attribf(x    , y    ); gle::attribf(tx     , ty     );
        gle::attribf(x + w, y    ); gle::attribf(tx + tw, ty     );
        gle::attribf(x    , y + h); gle::attribf(tx     , ty + th);
        gle::attribf(x + w, y + h); gle::attribf(tx + tw, ty + th);
    }

    struct ClipArea
    {
        float x1, y1, x2, y2;

        ClipArea(float x, float y, float w, float h) : x1(x), y1(y), x2(x+w), y2(y+h) {}

        void intersect(const ClipArea &c)
        {
            x1 = max(x1, c.x1);
            y1 = max(y1, c.y1);
            x2 = max(x1, min(x2, c.x2));
            y2 = max(y1, min(y2, c.y2));

        }

        bool isfullyclipped(float x, float y, float w, float h)
        {
            return x1 == x2 || y1 == y2 || x >= x2 || y >= y2 || x+w <= x1 || y+h <= y1;
        }

        void scissor()
        {
            float margin = max((float(screenw)/screenh - 1)/2, 0.0f);
            int sx1 = clamp(int(floor((x1+margin)/(1 + 2*margin)*screenw)), 0, screenw),
                sy1 = clamp(int(floor(y1*screenh)), 0, screenh),
                sx2 = clamp(int(ceil((x2+margin)/(1 + 2*margin)*screenw)), 0, screenw),
                sy2 = clamp(int(ceil(y2*screenh)), 0, screenh);
            glScissor(sx1, screenh - sy2, sx2-sx1, sy2-sy1);
        }
    };

    static vector<ClipArea> clipstack;

    static void pushclip(float x, float y, float w, float h)
    {
        if(clipstack.empty()) glEnable(GL_SCISSOR_TEST);
        ClipArea &c = clipstack.add(ClipArea(x, y, w, h));
        if(clipstack.length() >= 2) c.intersect(clipstack[clipstack.length()-2]);
        c.scissor();
    }

    static void popclip()
    {
        clipstack.pop();
        if(clipstack.empty()) glDisable(GL_SCISSOR_TEST);
        else clipstack.last().scissor();
    }

    static bool isfullyclipped(float x, float y, float w, float h)
    {
        if(clipstack.empty()) return false;
        return clipstack.last().isfullyclipped(x, y, w, h);
    }

    enum
    {
        ALIGN_MASK    = 0xF,

        ALIGN_HMASK   = 0x3,
        ALIGN_HSHIFT  = 0,
        ALIGN_HNONE   = 0,
        ALIGN_LEFT    = 1,
        ALIGN_HCENTER = 2,
        ALIGN_RIGHT   = 3,

        ALIGN_VMASK   = 0xC,
        ALIGN_VSHIFT  = 2,
        ALIGN_VNONE   = 0<<2,
        ALIGN_BOTTOM  = 1<<2,
        ALIGN_VCENTER = 2<<2,
        ALIGN_TOP     = 3<<2,

        CLAMP_MASK    = 0xF0,
        CLAMP_LEFT    = 0x10,
        CLAMP_RIGHT   = 0x20,
        CLAMP_BOTTOM  = 0x40,
        CLAMP_TOP     = 0x80,

        NO_ADJUST     = ALIGN_HNONE | ALIGN_VNONE,
    };

    enum
    {
        TYPE_MISC = 0,
        TYPE_BUTTON,
        TYPE_SCROLLER,
        TYPE_SCROLLBAR,
        TYPE_SCROLLBUTTON,
        TYPE_SLIDER,
        TYPE_SLIDERBUTTON,
        TYPE_IMAGE,
        TYPE_TAG,
        TYPE_WINDOW,
        TYPE_TEXTEDITOR,
    };

    enum
    {
        ORIENT_HORIZ = 0,
        ORIENT_VERT,
    };

    struct Object
    {
        Object *parent;
        float x, y, w, h;
        uchar adjust;
        vector<Object *> children;

        Object() : parent(NULL), x(0), y(0), w(0), h(0), adjust(ALIGN_HCENTER | ALIGN_VCENTER) {}
        virtual ~Object()
        {
            clearfocus(this);
            children.deletecontents();
        }

        virtual int forks() const { return 0; }
        virtual int choosefork() const { return -1; }

        #define loopchildren(o, body) do { \
            int numforks = forks(); \
            if(numforks > 0) \
            { \
                int i = choosefork(); \
                if(children.inrange(i)) \
                { \
                    Object *o = children[i]; \
                    body; \
                } \
            } \
            for(int i = numforks; i < children.length(); i++) \
            { \
                Object *o = children[i]; \
                body; \
            } \
        } while(0)


        #define loopchildrenrev(o, body) do { \
            int numforks = forks(); \
            for(int i = children.length()-1; i >= numforks; i--) \
            { \
                Object *o = children[i]; \
                body; \
            } \
            if(numforks > 0) \
            { \
                int i = choosefork(); \
                if(children.inrange(i)) \
                { \
                    Object *o = children[i]; \
                    body; \
                } \
            } \
        } while(0)

        #define loopinchildren(o, cx, cy, body) \
            loopchildren(o, \
            { \
                float o##x = cx - o->x; \
                float o##y = cy - o->y; \
                if(o##x >= 0 && o##x < o->w && o##y >= 0 && o##y < o->h) \
                { \
                    body; \
                } \
            })

        #define loopinchildrenrev(o, cx, cy, body) \
            loopchildrenrev(o, \
            { \
                float o##x = cx - o->x; \
                float o##y = cy - o->y; \
                if(o##x >= 0 && o##x < o->w && o##y >= 0 && o##y < o->h) \
                { \
                    body; \
                } \
            })

        virtual void layout()
        {
            w = h = 0;
            loopchildren(o,
            {
                o->x = o->y = 0;
                o->layout();
                w = max(w, o->x + o->w);
                h = max(h, o->y + o->h);
            });
        }

        void adjustchildrento(float px, float py, float pw, float ph)
        {
            loopchildren(o, o->adjustlayout(px, py, pw, ph));
        }

        virtual void adjustchildren()
        {
            adjustchildrento(0, 0, w, h);
        }

        void adjustlayout(float px, float py, float pw, float ph)
        {
            switch(adjust&ALIGN_HMASK)
            {
                case ALIGN_LEFT:    x = px; break;
                case ALIGN_HCENTER: x = px + (pw - w) / 2; break;
                case ALIGN_RIGHT:   x = px + pw - w; break;
            }

            switch(adjust&ALIGN_VMASK)
            {
                case ALIGN_BOTTOM:  y = py; break;
                case ALIGN_VCENTER: y = py + (ph - h) / 2; break;
                case ALIGN_TOP:     y = py + ph - h; break;
            }

            if(adjust&CLAMP_MASK)
            {
                if(adjust&CLAMP_LEFT)   x = px;
                if(adjust&CLAMP_RIGHT)  w = px + pw - x;
                if(adjust&CLAMP_BOTTOM) y = py;
                if(adjust&CLAMP_TOP)    h = py + ph - y;
            }

            adjustchildren();
        }

        virtual Object *target(float cx, float cy)
        {
            loopinchildrenrev(o, cx, cy,
            {
                Object *c = o->target(ox, oy);
                if(c) return c;
            });
            return NULL;
        }

        virtual bool key(int code, bool isdown)
        {
            loopchildrenrev(o,
            {
                if(o->key(code, isdown)) return true;
            });
            return false;
        }

        virtual bool hoverkey(int code, bool isdown)
        {
            if(parent) return parent->hoverkey(code, isdown);
            return false;
        }

        virtual void draw(float sx, float sy)
        {
            loopchildren(o,
            {
                if(!isfullyclipped(sx + o->x, sy + o->y, o->w, o->h))
                    o->draw(sx + o->x, sy + o->y);
            });
        }

        void draw()
        {
            draw(x, y);
        }

        virtual Object *hover(float cx, float cy)
        {
            loopinchildrenrev(o, cx, cy,
            {
                Object *c = o->hover(ox, oy);
                if(c == o) { hoverx = ox; hovery = oy; }
                if(c) return c;
            });
            return NULL;
        }

        virtual void hovering(float cx, float cy)
        {
        }

        virtual Object *select(float cx, float cy)
        {
            loopinchildrenrev(o, cx, cy,
            {
                Object *c = o->select(ox, oy);
                if(c == o) { selectx = ox; selecty = oy; }
                if(c) return c;
            });
            return NULL;
        }

        virtual bool allowselect(Object *o) { return false; }

        virtual void selected(float cx, float cy)
        {
        }

        virtual const char *getname() const { return ""; }
        virtual const int gettype() const { return TYPE_MISC; }
        virtual bool takesinput() const { return true; }

        bool isnamed(const char *name) const { return !strcmp(name, getname()); }

        Object *findname(int type, const char *name, bool recurse = true, const Object *exclude = NULL) const
        {
            loopchildren(o,
            {
                if(o != exclude &&
                    o->gettype() == type &&
                    (!name || o->isnamed(name))
                )
                    return o;
            });
            if(recurse) loopchildren(o,
            {
                if(o != exclude)
                {
                    Object *found = o->findname(type, name);
                    if(found) return found;
                }
            });
            return NULL;
        }

        Object *findsibling(int type, const char *name) const
        {
            for(const Object *prev = this, *cur = parent; cur; prev = cur, cur = cur->parent)
            {
                Object *o = cur->findname(type, name, true, prev);
                if(o) return o;
            }
            return NULL;
        }

        void remove(Object *o)
        {
            children.removeobj(o);
            delete o;
        }
    };

    struct World : Object
    {
        float margin;
        int size;

        Object *hover(float cx, float cy)
        {
            loopinchildrenrev(o, cx, cy,
            {
                if(!o->takesinput()) continue;
                Object *c = o->hover(ox, oy);
                if(c == o) { hoverx = ox; hovery = oy; }
                return c;
            });
            return NULL;
        }

        Object *select(float cx, float cy)
        {
            loopinchildrenrev(o, cx, cy,
            {
                if(!o->takesinput()) continue;
                Object *c = o->select(ox, oy);
                if(c && i < children.length() - 1) { children.removeobj(o); children.add(o); }
                if(c == o) { selectx = ox; selecty = oy; }
                return c;
            });
            return NULL;
        }

        void layout()
        {
            Object::layout();

            int sw = screenw;
            size = screenh;
            if(forceaspect) sw = int(ceil(size*forceaspect));

            margin = max((float(sw)/size - 1)/2, 0.0f);
            x = -margin;
            y = 0;
            w = 1 + 2*margin;
            h = 1;

            adjustchildren();
        }

        bool takesinput() const
        {
            loopchildrenrev(o,
                if(o->takesinput())
                    return true;
            );
            return false;
        }
    };

    World *main;
    World *bgworld;
    World *ldworld;

    World *world;

    struct Color
    {
        uchar r, g, b, a;

        Color() {}
        Color(uint c) : r((c>>16)&0xFF), g((c>>8)&0xFF), b(c&0xFF), a(c>>24 ? c>>24 : 0xFF) {}
        Color(uint c, uchar a) : r((c>>16)&0xFF), g((c>>8)&0xFF), b(c&0xFF), a(a) {}
        Color(uchar r, uchar g, uchar b, uchar a = 255) : r(r), g(g), b(b), a(a) {}

        void init() { gle::colorub(r, g, b, a); }
        void attrib() { gle::attribub(r, g, b, a); }

        static void def() { gle::defcolor(4, GL_UNSIGNED_BYTE); }
    };

    struct HorizontalList : Object
    {
        float space, subw;

        HorizontalList(float space = 0) : space(space) {}

        void layout()
        {
            subw = h = 0;
            loopchildren(o,
            {
                o->x = subw;
                o->y = 0;
                o->layout();
                subw += o->w;
                h = max(h, o->y + o->h);
            });
            w = subw + space*max(children.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            float offset = 0, cspace = (w - subw) / max(children.length() - 1, 1);
            loopchildren(o,
            {
                o->x = offset;
                offset += o->w;
                o->adjustlayout(o->x, 0, offset - o->x, h);
                offset += cspace;
            });
        }
    };

    struct VerticalList : Object
    {
        float space, subh;

        VerticalList(float space = 0) : space(space) {}

        void layout()
        {
            w = subh = 0;
            loopchildren(o,
            {
                o->x = 0;
                o->y = subh;
                o->layout();
                subh += o->h;
                w = max(w, o->x + o->w);
            });
            h = subh + space*max(children.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            float offset = 0, cspace = (h - subh) / max(children.length() - 1, 1);
            loopchildren(o,
            {
                o->y = offset;
                offset += o->h;
                o->adjustlayout(0, o->y, w, offset - o->y);
                offset += cspace;
            });
        }

    };

    struct Table : Object
    {
        int columns;
        float space, subh, subw;
        vector<float> widths, heights;

        Table(int columns, float space = 0) : columns(columns), space(space) {}

        void layout()
        {
            widths.setsize(0);
            heights.setsize(0);

            int column = 0, row = 0;
            loopchildren(o,
            {
                o->layout();
                if(widths.length() <= column) widths.add(o->w);
                else if(o->w > widths[column]) widths[column] = o->w;
                if(heights.length() <= row) heights.add(o->h);
                else if(o->h > heights[row]) heights[row] = o->h;
                column = (column + 1) % columns;
                if(!column) row++;
            });

            subh = subw = 0;
            loopv(widths) subw += widths[i];
            loopv(heights) subh += heights[i];
            w = subw + space * max(widths.length() - 1, 0);
            h = subh + space * max(heights.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            int column = 0, row = 0;
            float offsetx = 0, offsety = 0,
                  cspace = (w - subw) / max(widths.length() - 1, 1),
                  rspace = (h - subh) / max(heights.length() - 1, 1);

            loopchildren(o,
            {
                o->x = offsetx;
                o->y = offsety;
                o->adjustlayout(offsetx, offsety, widths[column], heights[row]);
                offsetx += widths[column] + cspace;
                column = (column + 1) % columns;
                if(!column)
                {
                    offsetx = 0;
                    offsety += heights[row] + rspace;
                    row++;
                }
            });
        }
    };

    struct Spacer : Object
    {
        float spacew, spaceh;

        Spacer(float spacew, float spaceh) : spacew(spacew), spaceh(spaceh) {}

        void layout()
        {
            w = spacew;
            h = spaceh;
            loopchildren(o,
            {
                o->x = spacew;
                o->y = spaceh;
                o->layout();
                w = max(w, o->x + o->w);
                h = max(h, o->y + o->h);
            });
            w += spacew;
            h += spaceh;
        }

        void adjustchildren()
        {
            adjustchildrento(spacew, spaceh, w - 2*spacew, h - 2*spaceh);
        }
    };

    struct Filler : Object
    {
        float minw, minh;

        Filler(float minw, float minh) : minw(minw), minh(minh) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            return o ? o : this;
        }

        void layout()
        {
            Object::layout();

            w = max(w, minw);
            h = max(h, minh);
        }
    };

    struct Offsetter : Object
    {
        float offsetx, offsety;

        Offsetter(float offsetx, float offsety) : offsetx(offsetx), offsety(offsety) {}

        void layout()
        {
            Object::layout();

            loopchildren(o,
            {
                o->x += offsetx;
                o->y += offsety;
            });

            w += offsetx;
            h += offsety;
        }

        void adjustchildren()
        {
            adjustchildrento(offsetx, offsety, w - offsetx, h - offsety);
        }
    };

    struct Clipper : Object
    {
        float clipw, cliph, virtw, virth;

        Clipper(float clipw = 0, float cliph = 0) : clipw(clipw), cliph(cliph), virtw(0), virth(0) {}

        void layout()
        {
            Object::layout();

            virtw = w;
            virth = h;
            if(clipw) w = min(w, clipw);
            if(cliph) h = min(h, cliph);
        }

        void adjustchildren()
        {
            adjustchildrento(0, 0, virtw, virth);
        }

        void draw(float sx, float sy)
        {
            if((clipw && virtw > clipw) || (cliph && virth > cliph))
            {
                pushclip(sx, sy, w, h);
                Object::draw(sx, sy);
                popclip();
            }
            else Object::draw(sx, sy);
        }
    };

    struct Conditional : Object
    {
        uint *cond;

        Conditional(uint *cond) : cond(cond) { keepcode(cond); }
        ~Conditional() { freecode(cond); }

        int forks() const { return 2; }
        int choosefork() const { return executebool(cond) ? 0 : 1; }
    };

    struct Button : Object
    {
        uint *onselect;
        Object *tooltip;

        Button(uint *onselect) : onselect(onselect), tooltip(NULL) { keepcode(onselect); }
        ~Button() { freecode(onselect); delete tooltip; }

        int forks() const { return 3; }
        int choosefork() const { return isselected(this) ? 2 : (ishovering(this) ? 1 : 0); }

        Object *hover(float cx, float cy)
        {
            if(target(cx, cy))
            {
                UI::tooltip = this->tooltip;
                return this;
            }
            return NULL;
        }

        Object *select(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        void selected(float cx, float cy)
        {
            updatelater.add().schedule(onselect);
        }

        const int gettype() const { return TYPE_BUTTON; }
    };

    struct ConditionalButton : Button
    {
        uint *cond;

        ConditionalButton(uint *cond, uint *onselect) : Button(onselect), cond(cond) { keepcode(cond); }
        ~ConditionalButton() { freecode(cond); }

        int forks() const { return 4; }
        int choosefork() const { return execute(cond) ? 1 + Button::choosefork() : 0; }

        void selected(float cx, float cy)
        {
            if(execute(cond)) Button::selected(cx, cy);
        }
    };

    VAR(uitogglehside, 1, 0, 0);
    VAR(uitogglevside, 1, 0, 0);

    struct Toggle : Button
    {
        uint *cond;
        float split;

        Toggle(uint *cond, uint *onselect, float split = 0) : Button(onselect), cond(cond), split(split) { keepcode(cond); }
        ~Toggle() { freecode(cond); }

        int forks() const { return 4; }
        int choosefork() const { return (execute(cond) ? 2 : 0) + (ishovering(this) ? 1 : 0); }

        Object *select(float cx, float cy)
        {
            if(target(cx, cy))
            {
                uitogglehside = cx < w*split ? 0 : 1;
                uitogglevside = cy < h*split ? 0 : 1;
                return this;
            }
            return NULL;
        }
    };

    struct Scroller : Clipper
    {
        float offsetx, offsety;
        bool canscroll;

        Scroller(float clipw = 0, float cliph = 0) : Clipper(clipw, cliph), offsetx(0), offsety(0) {}

        void layout()
        {
            Clipper::layout();
            offsetx = min(offsetx, hlimit());
            offsety = min(offsety, vlimit());
        }

        Object *target(float cx, float cy)
        {
            if(cx + offsetx >= virtw || cy + offsety >= virth) return NULL;
            return Object::target(cx + offsetx, cy + offsety);
        }

        Object *hover(float cx, float cy)
        {
            if(cx + offsetx >= virtw || cy + offsety >= virth)
            {
                canscroll = false;
                return NULL;
            }
            canscroll = true;

            Object *o = Object::hover(cx + offsetx, cy + offsety);
            return o ? o : this;
        }

        Object *select(float cx, float cy)
        {
            if(cx + offsetx >= virtw || cy + offsety >= virth) return NULL;
            return Object::select(cx + offsetx, cy + offsety);
        }

        bool hoverkey(int code, bool isdown);

        void draw(float sx, float sy)
        {
            if((clipw && virtw > clipw) || (cliph && virth > cliph))
            {
                pushclip(sx, sy, w, h);
                Object::draw(sx - offsetx, sy - offsety);
                popclip();
            }
            else Object::draw(sx, sy);
        }

        float hlimit() const { return max(virtw - w, 0.0f); }
        float vlimit() const { return max(virth - h, 0.0f); }
        float hoffset() const { return offsetx / max(virtw, w); }
        float voffset() const { return offsety / max(virth, h); }
        float hscale() const { return w / max(virtw, w); }
        float vscale() const { return h / max(virth, h); }

        void addhscroll(float hscroll) { sethscroll(offsetx + hscroll); }
        void addvscroll(float vscroll) { setvscroll(offsety + vscroll); }
        void sethscroll(float hscroll) { offsetx = clamp(hscroll, 0.0f, hlimit()); }
        void setvscroll(float vscroll) { offsety = clamp(vscroll, 0.0f, vlimit()); }

        const int gettype() const { return TYPE_SCROLLER; }
    };

    struct ScrollBar : Object
    {
        float arrowsize, arrowspeed;
        int arrowdir;

        ScrollBar(float arrowsize = 0, float arrowspeed = 0) : arrowsize(arrowsize), arrowspeed(arrowspeed), arrowdir(0) {}

        int forks() const { return 5; }
        int choosefork() const
        {
            switch(arrowdir)
            {
                case -1: return isselected(this) ? 2 : (ishovering(this) ? 1 : 0);
                case 1: return isselected(this) ? 4 : (ishovering(this) ? 3 : 0);
            }
            return 0;
        }

        virtual int choosedir(float cx, float cy) const { return 0; }
        virtual int getorient() const =0;

        Object *hover(float cx, float cy)
        {
            Object *o = Object::hover(cx, cy);
            if(o) return o;
            return this;
        }

        Object *select(float cx, float cy)
        {
            Object *o = Object::select(cx, cy);
            if(o) return o;
            return target(cx, cy) ? this : NULL;
        }

        const int gettype() const { return TYPE_SCROLLBAR; }

        virtual void scrollto(float cx, float cy) {}

        bool hoverkey(int code, bool isdown)
        {
            if(code != -4 && code != -5) return Object::hoverkey(code, isdown);

            Scroller *scroller = (Scroller *) findsibling(TYPE_SCROLLER, NULL);
            if(!scroller || !scroller->canscroll) return false;

            if(!isdown) return true;

             float adjust = (code == -4 ? -.2 : .2) * arrowspeed;
            if(getorient() == ORIENT_VERT)
                scroller->addvscroll(adjust);
            else
                scroller->addhscroll(adjust);

            return true;
        }

        void selected(float cx, float cy)
        {
            arrowdir = choosedir(cx, cy);
            if(!arrowdir) scrollto(cx, cy);
            else hovering(cx, cy);
        }

        virtual void arrowscroll()
        {
        }

        void hovering(float cx, float cy)
        {
            if(isselected(this))
            {
                if(arrowdir) arrowscroll();
            }
            else
            {
                Object *button = findname(TYPE_SCROLLBUTTON, NULL, false);
                if(button && isselected(button))
                {
                    arrowdir = 0;
                    button->hovering(cx - button->x, cy - button->y);
                }
                else arrowdir = choosedir(cx, cy);
            }
        }

        bool allowselect(Object *o)
        {
            return children.find(o) >= 0;
        }

        virtual void movebutton(Object *o, float fromx, float fromy, float tox, float toy) = 0;
    };

    bool Scroller::hoverkey(int code, bool isdown)
    {
        if(code != -4 && code != -5) return Object::hoverkey(code, isdown);

        ScrollBar *slider = (ScrollBar *) findsibling(TYPE_SCROLLBAR, NULL);
        if(!slider || !canscroll) return false;;
        if(!isdown) return true;

        float adjust = (code == -4 ? -.2 : .2) * slider->arrowspeed;
        if(slider->getorient() == ORIENT_VERT)
            addvscroll(adjust);
        else
            addhscroll(adjust);

        return true;
    }

    struct ScrollButton : Object
    {
        float offsetx, offsety;

        ScrollButton() : offsetx(0), offsety(0) {}

        int forks() const { return 3; }
        int choosefork() const { return isselected(this) ? 2 : (ishovering(this) ? 1 : 0); }

        Object *hover(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        Object *select(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        void hovering(float cx, float cy)
        {
            if(isselected(this))
            {
                if(!parent || parent->gettype() != TYPE_SCROLLBAR) return;
                ScrollBar *scrollbar = (ScrollBar *) parent;
                scrollbar->movebutton(this, offsetx, offsety, cx, cy);
            }
        }

        void selected(float cx, float cy)
        {
            offsetx = cx;
            offsety = cy;
        }

        const int gettype() const { return TYPE_SCROLLBUTTON; }
    };

    struct HorizontalScrollBar : ScrollBar
    {
        HorizontalScrollBar(float arrowsize = 0, float arrowspeed = 0) : ScrollBar(arrowsize, arrowspeed) {}

        int choosedir(float cx, float cy) const
        {
            if(cx < arrowsize) return -1;
            else if(cx >= w - arrowsize) return 1;
            return 0;
        }

        int getorient() const
        {
            return ORIENT_HORIZ;
        }

        void arrowscroll()
        {
            Scroller *scroller = (Scroller *) findsibling(TYPE_SCROLLER, NULL);
            if(!scroller) return;
            scroller->addhscroll(arrowdir*arrowspeed*curtime/1000.0f);
        }

        void scrollto(float cx, float cy)
        {
            Scroller *scroller = (Scroller *) findsibling(TYPE_SCROLLER, NULL);
            if(!scroller) return;
            ScrollButton *button = (ScrollButton *) findname(TYPE_SCROLLBUTTON, NULL, false);
            if(!button) return;
            float bscale = (max(w - 2*arrowsize, 0.0f) - button->w) / (1 - scroller->hscale()),
                  offset = bscale > 1e-3f ? (cx - arrowsize)/bscale : 0;
            scroller->sethscroll(offset*scroller->virtw);
        }

        void adjustchildren()
        {
            Scroller *scroller = (Scroller *) findsibling(TYPE_SCROLLER, NULL);
            if(!scroller) return;
            ScrollButton *button = (ScrollButton *) findname(TYPE_SCROLLBUTTON, NULL, false);
            if(!button) return;
            float bw = max(w - 2*arrowsize, 0.0f)*scroller->hscale();
            button->w = max(button->w, bw);
            float bscale = scroller->hscale() < 1 ? (max(w - 2*arrowsize, 0.0f) - button->w) / (1 - scroller->hscale()) : 1;
            button->x = arrowsize + scroller->hoffset()*bscale;
            button->adjust &= ~ALIGN_HMASK;

            ScrollBar::adjustchildren();
        }

        void movebutton(Object *o, float fromx, float fromy, float tox, float toy)
        {
            scrollto(o->x + tox - fromx, o->y + toy);
        }
    };

    struct VerticalScrollBar : ScrollBar
    {
        VerticalScrollBar(float arrowsize = 0, float arrowspeed = 0) : ScrollBar(arrowsize, arrowspeed) {}

        int choosedir(float cx, float cy) const
        {
            if(cy < arrowsize) return -1;
            else if(cy >= h - arrowsize) return 1;
            return 0;
        }

        int getorient() const
        {
            return ORIENT_VERT;
        }

        void arrowscroll()
        {
            Scroller *scroller = (Scroller *) findsibling(TYPE_SCROLLER, NULL);
            if(!scroller) return;
            scroller->addvscroll(arrowdir*arrowspeed*curtime/1000.0f);
        }

        void scrollto(float cx, float cy)
        {
            Scroller *scroller = (Scroller *) findsibling(TYPE_SCROLLER, NULL);
            if(!scroller) return;
            ScrollButton *button = (ScrollButton *) findname(TYPE_SCROLLBUTTON, NULL, false);
            if(!button) return;
            float bscale = (max(h - 2*arrowsize, 0.0f) - button->h) / (1 - scroller->vscale()),
                  offset = bscale > 1e-3f ? (cy - arrowsize)/bscale : 0;
            scroller->setvscroll(offset*scroller->virth);
        }

        void adjustchildren()
        {
            Scroller *scroller = (Scroller *) findsibling(TYPE_SCROLLER, NULL);
            if(!scroller) return;
            ScrollButton *button = (ScrollButton *) findname(TYPE_SCROLLBUTTON, NULL, false);
            if(!button) return;
            float bh = max(h - 2*arrowsize, 0.0f)*scroller->vscale();
            button->h = max(button->h, bh);
            float bscale = scroller->vscale() < 1 ? (max(h - 2*arrowsize, 0.0f) - button->h) / (1 - scroller->vscale()) : 1;
            button->y = arrowsize + scroller->voffset()*bscale;
            button->adjust &= ~ALIGN_VMASK;

            ScrollBar::adjustchildren();
        }

        void movebutton(Object *o, float fromx, float fromy, float tox, float toy)
        {
            scrollto(o->x + tox, o->y + toy - fromy);
        }
    };

    static float getfval(ident *id)
    {
        switch(id->type)
        {
            case ID_VAR: return *id->storage.i;
            case ID_FVAR: return *id->storage.f;
            case ID_SVAR: return parsefloat(*id->storage.s);
            case ID_ALIAS: return id->getfloat();
            default: return 0;
        }
    }

    struct Slider : Object
    {
        ident *id;
        float vmin, vmax;
        uint *onchange;
        float arrowsize;
        float stepsize;
        int steptime;

        int laststep;
        int arrowdir;

        Slider(ident *id, float min = 0, float max = 0, uint *onchange = NULL, float arrowsize = 0, float stepsize = 1, int steptime = 1000) :
        id(id), vmin(min), vmax(max), onchange(onchange), arrowsize(arrowsize), stepsize(stepsize), steptime(steptime), laststep(0), arrowdir(0)
        {
            keepcode(onchange);
            if(vmin == 0 && vmax == 0) switch(id->type)
            {
                case ID_VAR:
                    vmin = id->minval; vmax = id->maxval;
                    break;
                case ID_FVAR:
                    vmin = id->minvalf; vmax = id->maxvalf;
                    break;

            }
        }
        ~Slider() { freecode(onchange); }

        void dostep(int n)
        {
            int maxstep = fabs(vmax - vmin) / stepsize;
            int curstep = (getfval(id) - min(vmin, vmax)) / stepsize;
            int newstep = clamp(curstep + n, 0, maxstep);

            updateval(id, min(vmax, vmin) + newstep * stepsize, onchange);
        }

        void setstep(int n)
        {
            int steps = fabs(vmax - vmin) / stepsize;
            int newstep = clamp(n, 0, steps);

            updateval(id, min(vmax, vmin) + newstep * stepsize, onchange);
        }

        bool hoverkey(int code, bool isdown)
        {
            switch(code)
            {
                case SDLK_UP:
                case SDLK_LEFT:
                    if(isdown) dostep(-1);
                    return true;
                case -4:
                    if(isdown) dostep(-3);
                    return true;
                case SDLK_DOWN:
                case SDLK_RIGHT:
                    if(isdown) dostep(1);
                    return true;
                case -5:
                    if(isdown) dostep(3);
                    return true;
            }

             return Object::hoverkey(code, isdown);
        }

        int forks() const { return 5; }
        int choosefork() const
        {
            switch(arrowdir)
            {
                case -1: return isselected(this) ? 2 : (ishovering(this) ? 1 : 0);
                case 1: return isselected(this) ? 4 : (ishovering(this) ? 3 : 0);
            }
            return 0;
        }

        virtual int choosedir(float cx, float cy) const { return 0; }

        Object *hover(float cx, float cy)
        {
            Object *o = Object::hover(cx, cy);
            if(o) return o;
            return this;
        }

        Object *select(float cx, float cy)
        {
            Object *o = Object::select(cx, cy);
            if(o) return o;
            return target(cx, cy) ? this : NULL;
        }

        const int gettype() const { return TYPE_SLIDER; }

        virtual void scrollto(float cx, float cy)
        {
        }

        void selected(float cx, float cy)
        {
            arrowdir = choosedir(cx, cy);
            if(!arrowdir) scrollto(cx, cy);
            else hovering(cx, cy);
        }

        void arrowscroll()
        {
            if(laststep + steptime > totalmillis)
                return;

            laststep = totalmillis;
            dostep(arrowdir);
        }

        void hovering(float cx, float cy)
        {
            if(isselected(this))
            {
                if(arrowdir) arrowscroll();
            }
            else
            {
                Object *button = findname(TYPE_SLIDERBUTTON, NULL, false);
                if(button && isselected(button))
                {
                    arrowdir = 0;
                    button->hovering(cx - button->x, cy - button->y);
                }
                else arrowdir = choosedir(cx, cy);
            }
        }

        bool allowselect(Object *o)
        {
            return children.find(o) >= 0;
        }

        virtual void movebutton(Object *o, float fromx, float fromy, float tox, float toy) = 0;
    };

    struct SliderButton : Object
    {
        float offsetx, offsety;

        SliderButton() : offsetx(0), offsety(0) {}

        int forks() const { return 3; }
        int choosefork() const { return isselected(this) ? 2 : (ishovering(this) ? 1 : 0); }

        Object *hover(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        Object *select(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        void hovering(float cx, float cy)
        {
            if(isselected(this))
            {
                if(!parent || parent->gettype() != TYPE_SLIDER) return;
                Slider *slider = (Slider *) parent;
                slider->movebutton(this, offsetx, offsety, cx, cy);
            }
        }

        void selected(float cx, float cy)
        {
            offsetx = cx;
            offsety = cy;
        }

        void layout()
        {
            float lastw = w, lasth = h;
            Object::layout();
            if(isselected(this))
            {
                w = lastw;
                h = lasth;
            }
        }

        const int gettype() const { return TYPE_SLIDERBUTTON; }
    };

    struct HorizontalSlider : Slider
    {
        HorizontalSlider(ident *id, float vmin = 0, float vmax = 0, uint *onchange = NULL, float arrowsize = 0, float stepsize = 1, int steptime = 1000) : Slider(id, vmin, vmax, onchange, arrowsize, stepsize, steptime) {}

        int choosedir(float cx, float cy) const
        {
            if(cx < arrowsize) return -1;
            else if(cx >= w - arrowsize) return 1;
            return 0;
        }

        void scrollto(float cx, float cy)
        {
            SliderButton *button = (SliderButton *) findname(TYPE_SLIDERBUTTON, NULL, false);
            if(!button) return;

            float pos = clamp((cx - arrowsize - button->w / 2) / (w - 2 * arrowsize - button->w), 0.f, 1.f);

            int steps = fabs(vmax - vmin) / stepsize;
            int step = lroundf(steps * pos);

            setstep(step);
        }

        void adjustchildren()
        {
            SliderButton *button = (SliderButton *) findname(TYPE_SLIDERBUTTON, NULL, false);
            if(!button) return;

            int steps = fabs(vmax - vmin) / stepsize;
            int curstep = (getfval(id) - min(vmax, vmin)) / stepsize;
            float width = max(w - 2  *arrowsize, 0.0f);

            button->w = max(button->w, width / steps);
            button->x = arrowsize + (width - button->w) * curstep / steps;
            button->adjust &= ~ALIGN_HMASK;

            Slider::adjustchildren();
        }

        void movebutton(Object *o, float fromx, float fromy, float tox, float toy)
        {
            scrollto(o->x + o->w / 2 + tox - fromx, o->y + toy);
        }
    };

    struct VerticalSlider : Slider
    {
        VerticalSlider(ident *id, float vmin = 0, float vmax = 0, uint *onchange = NULL, float arrowsize = 0, float stepsize = 1, int steptime = 1000) : Slider(id, vmin, vmax, onchange, arrowsize, stepsize, steptime) {}

        int choosedir(float cx, float cy) const
        {
            if(cy < arrowsize) return -1;
            else if(cy >= h - arrowsize) return 1;
            return 0;
        }

        void scrollto(float cx, float cy)
        {
            SliderButton *button = (SliderButton *) findname(TYPE_SLIDERBUTTON, NULL, false);
            if(!button) return;

            float pos = clamp((cy - arrowsize - button->h / 2) / (h - 2 * arrowsize - button->h), 0.f, 1.f);

            int steps = (max(vmax, vmin) - min(vmax, vmin)) / stepsize;
            int step = lroundf(steps * pos);
            setstep(step);
        }

        void adjustchildren()
        {
            SliderButton *button = (SliderButton *) findname(TYPE_SLIDERBUTTON, NULL, false);
            if(!button) return;

            int steps = (max(vmax, vmin) - min(vmax, vmin)) / stepsize + 1;
            int curstep = (getfval(id) - min(vmax, vmin)) / stepsize;
            float height = max(h - 2  *arrowsize, 0.0f);

            button->h = max(button->h, height / steps);
            button->y = arrowsize + (height - button->h) * curstep / steps;
            button->adjust &= ~ALIGN_VMASK;

            Slider::adjustchildren();
        }

        void movebutton(Object *o, float fromx, float fromy, float tox, float toy)
        {
            scrollto(o->x + o->h / 2 + tox, o->y + toy - fromy);
        }
    };

    static bool checkalphamask(Texture *tex, float x, float y)
    {
        if(!tex->alphamask)
        {
            loadalphamask(tex);
            if(!tex->alphamask) return true;
        }
        int tx = clamp(int(floor(x*tex->xs)), 0, tex->xs-1),
            ty = clamp(int(floor(y*tex->ys)), 0, tex->ys-1);
        if(tex->alphamask[ty*((tex->xs+7)/8) + tx/8] & (1<<(tx%8))) return true;
        return false;
    }

    struct Rectangle : Filler
    {
        enum { SOLID = 0, MODULATE };

        int type;
        Color color;

        Rectangle(int type, const Color &color, float minw = 0, float minh = 0) : Filler(minw, minh), type(type), color(color) {}

        void draw(float sx, float sy)
        {
            if(type==MODULATE) glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            hudnotextureshader->set();
            color.init();

            gle::defvertex(2);

            gle::begin(GL_TRIANGLE_STRIP);
            gle::attribf(sx,     sy);
            gle::attribf(sx + w, sy);
            gle::attribf(sx,     sy + h);
            gle::attribf(sx + w, sy + h);
            gle::end();

            hudshader->set();
            gle::colorf(1, 1, 1);
            gle::defvertex(2);
            gle::deftexcoord0();

            if(type==MODULATE) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            Object::draw(sx, sy);
        }
    };

    struct Gradient : Rectangle
    {
        enum { VERTICAL, HORIZONTAL };

        int dir;
        Color color2;

        Gradient(int type, int dir, const Color &color, const Color color2, float minw = 0, float minh = 0) : Rectangle(type, color, minw, minh), dir(dir), color2(color2) {}
        ~Gradient() {}

        void draw(float sx, float sy)
        {
            if(type==MODULATE) glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            hudnotextureshader->set();

            gle::defvertex(2);
            Color::def();

            gle::begin(GL_TRIANGLE_STRIP);
            gle::attribf(sx+w, sy);   (dir == HORIZONTAL ? color2 : color).attrib();
            gle::attribf(sx,   sy);   color.attrib();
            gle::attribf(sx+w, sy+h); color2.attrib();
            gle::attribf(sx,   sy+h); (dir == HORIZONTAL ? color : color2).attrib();
            gle::end();

            hudshader->set();
            gle::defvertex(2);
            gle::deftexcoord0();
            if(type==MODULATE) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Object::draw(sx, sy);
        }
    };

    struct Line : Filler
    {
        Color color;

        Line(const Color &color, float minw = 0, float minh = 0) : Filler(minw, minh), color(color) {}
        ~Line() {}

        void draw(float sx, float sy)
        {
            hudnotextureshader->set();
            color.init();

            gle::defvertex(2);
            gle::begin(GL_LINES);
            gle::attribf(sx,   sy);
            gle::attribf(sx+w, sy+h);
            gle::end();

            hudshader->set();
            gle::colorf(1, 1, 1);
            gle::defvertex(2);
            gle::deftexcoord0();

            Object::draw(sx, sy);
        }
    };

    struct Outline : Filler
    {
        Color color;

        Outline(const Color &color, float minw = 0, float minh = 0) : Filler(minw, minh), color(color) {}
        ~Outline() {}

        void draw(float sx, float sy)
        {
            hudnotextureshader->set();
            color.init();

            gle::defvertex(2);
            gle::begin(GL_LINE_LOOP);
            gle::attribf(sx,   sy);
            gle::attribf(sx+w, sy);
            gle::attribf(sx+w, sy+h);
            gle::attribf(sx,   sy+h);
            gle::end();

            hudshader->set();
            gle::colorf(1, 1, 1);
            gle::defvertex(2);
            gle::deftexcoord0();

            Object::draw(sx, sy);
        }
    };

    VARP(uialphatest, 0, 1, 1);

    struct Image : Filler
    {
        Texture *tex;

        Image(Texture *tex, float minw = 0, float minh = 0) : Filler(minw, minh), tex(tex) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o) return o;
            if(tex->bpp < 32 || !uialphatest) return this;
            return checkalphamask(tex, cx/w, cy/h) ? this : NULL;
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);
            gle::begin(GL_TRIANGLE_STRIP);
            quadtri(sx, sy, w, h);
            gle::end();

            Object::draw(sx, sy);
        }

        const int gettype() const { return TYPE_IMAGE; }
    };

    VAR(thumbtime, 0, 25, 1000);
    static int lastthumbnail = 0;

    struct Thumbnail : Image
    {
        const char *img;
        bool loaded;

        Thumbnail(const char *img, float minw, float minh) : Image(notexture, minw, minw), img(newstring(img)), loaded(false) {}
        ~Thumbnail() { delete[] img; }

        void load(bool force)
        {
            if(loaded) return;

            const char *p = img;
            Texture *t = textureget(p);
            if(t != notexture) goto finalise;

            p = makerelpath(NULL, img, "<thumbnail>", NULL);
            t = textureget(p);
            if(t != notexture) goto finalise;

            if(force || (totalmillis - lastthumbnail) >= thumbtime)
                t = textureload(p, 3, true, false);

            if(t != notexture) goto finalise;

            return;

        finalise:
            lastthumbnail = totalmillis;
            loaded = true;
            tex = t;
        }

        Object *target(float cx, float cy)
        {
            load(true);
            return Image::target(cx, cy);
        }

        void draw(float sx, float sy)
        {
            load(false);
            Image::draw(sx, sy);
        }
    };

    struct SlotViewer : Filler
    {
        int slotnum;

        SlotViewer(int slotnum, float minw = 0, float minh = 0) : Filler(minw, minh), slotnum(slotnum) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o || !texmru.inrange(slotnum)) return o;
            VSlot &vslot = lookupvslot(texmru[slotnum], false);
            if(vslot.slot->sts.length() && (vslot.slot->loaded || vslot.slot->thumbnail)) return this;
            return NULL;
        }

        void drawslot(Slot &slot, VSlot &vslot, float sx, float sy)
        {
            VSlot *layer = NULL, *decal = NULL;
            Texture *tex = notexture, *glowtex = NULL, *layertex = NULL, *decaltex = NULL;
            if(slot.loaded)
            {
                tex = slot.sts[0].t;
                if(slot.texmask&(1<<TEX_GLOW)) {
                    loopv(slot.sts) if(slot.sts[i].type==TEX_GLOW)
                    { glowtex = slot.sts[i].t; break; }
                }
                if(vslot.layer)
                {
                    layer = &lookupvslot(vslot.layer);
                    if(!layer->slot->sts.empty())
                        layertex = layer->slot->sts[0].t;
                }
                if(vslot.decal)
                {
                    decal = &lookupvslot(vslot.decal);
                    if(!decal->slot->sts.empty()) decaltex = decal->slot->sts[0].t;
                }
            }
            else if(slot.thumbnail) tex = slot.thumbnail;
            float xt, yt;
            xt = min(1.0f, tex->xs/(float)tex->ys),
            yt = min(1.0f, tex->ys/(float)tex->xs);

            vec2 tc[4] = { vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(1, 1) };
            int xoff = vslot.offset.x, yoff = vslot.offset.y;
            if(vslot.rotation)
            {
                if((vslot.rotation&5) == 1) { swap(xoff, yoff); loopk(4) swap(tc[k][0], tc[k][1]); }
                if(vslot.rotation >= 2 && vslot.rotation <= 4) { xoff *= -1; loopk(4) tc[k][0] *= -1; }
                if(vslot.rotation <= 2 || vslot.rotation == 5) { yoff *= -1; loopk(4) tc[k][1] *= -1; }
            }
            loopk(4) { tc[k][0] = tc[k][0]/xt - float(xoff)/tex->xs; tc[k][1] = tc[k][1]/yt - float(yoff)/tex->ys; }

            SETSHADER(hudrgb);

            glBindTexture(GL_TEXTURE_2D, tex->id);

            if(slot.loaded) gle::color(vslot.colorscale);
            else gle::colorf(1, 1, 1);
            gle::begin(GL_TRIANGLE_STRIP);
            gle::attribf(sx  , sy  ); gle::attrib(tc[0]);
            gle::attribf(sx+w, sy  ); gle::attrib(tc[1]);
            gle::attribf(sx  , sy+h); gle::attrib(tc[2]);
            gle::attribf(sx+w, sy+h); gle::attrib(tc[3]);
            gle::end();

            if(decaltex)
            {
                glBindTexture(GL_TEXTURE_2D, decaltex->id);
                gle::begin(GL_TRIANGLE_STRIP);
                gle::attribf(sx,      sy);      gle::attrib(tc[0]);
                gle::attribf(sx+w/2, sy);      gle::attrib(tc[1]);
                gle::attribf(sx,      sy+h/2); gle::attrib(tc[3]);
                gle::attribf(sx+w/2, sy+h/2); gle::attrib(tc[2]);
                gle::end();
            }

            if(glowtex)
            {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glBindTexture(GL_TEXTURE_2D, glowtex->id);
                gle::color(vslot.glowcolor);
                gle::begin(GL_TRIANGLE_STRIP);
                gle::attribf(sx  , sy  ); gle::attrib(tc[0]);
                gle::attribf(sx+w, sy  ); gle::attrib(tc[1]);
                gle::attribf(sx  , sy+h); gle::attrib(tc[2]);
                gle::attribf(sx+w, sy+h); gle::attrib(tc[3]);
                gle::end();

                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            if(layertex)
            {
                glBindTexture(GL_TEXTURE_2D, layertex->id);
                gle::color(layer->colorscale);
                gle::begin(GL_TRIANGLE_STRIP);
                gle::attribf(sx+w/2, sy+h/2); gle::attrib(tc[0]);
                gle::attribf(sx+w,   sy+h/2); gle::attrib(tc[1]);
                gle::attribf(sx+w/2, sy+h  ); gle::attrib(tc[2]);
                gle::attribf(sx+w,   sy+h  ); gle::attrib(tc[3]);
                gle::end();
            }
            gle::colorf(1, 1, 1);
            hudshader->set();
        }

        void draw(float sx, float sy)
        {
            if(texmru.inrange(slotnum))
            {
                VSlot &vslot = lookupvslot(texmru[slotnum], false);
                Slot &slot = *vslot.slot;
                if(slot.sts.length())
                {
                    if(slot.loaded || slot.thumbnail) drawslot(slot, vslot, sx, sy);
                    else if(totalmillis-lastthumbnail >= thumbtime)
                    {
                        loadthumbnail(slot);
                        lastthumbnail = totalmillis;
                    }
                }
            }

            Object::draw(sx, sy);
        }
    };

    struct CroppedImage : Image
    {
        float cropx, cropy, cropw, croph;

        CroppedImage(Texture *tex, float minw = 0, float minh = 0, float cropx = 0, float cropy = 0, float cropw = 1, float croph = 1)
            : Image(tex, minw, minh), cropx(cropx), cropy(cropy), cropw(cropw), croph(croph) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o) return o;
            if(tex->bpp < 32 || !uialphatest) return this;
            return checkalphamask(tex, cropx + cx/w*cropw, cropy + cy/h*croph) ? this : NULL;
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);
            gle::begin(GL_TRIANGLE_STRIP);
            quadtri(sx, sy, w, h, cropx, cropy, cropw, croph);
            gle::end();

            Object::draw(sx, sy);
        }
    };

    struct StretchedImage : Image
    {
        StretchedImage(Texture *tex, float minw = 0, float minh = 0) : Image(tex, minw, minh) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o) return o;
            if(tex->bpp < 32 || !uialphatest) return this;

            float mx, my;
            if(w <= minw) mx = cx/w;
            else if(cx < minw/2) mx = cx/minw;
            else if(cx >= w - minw/2) mx = 1 - (w - cx) / minw;
            else mx = 0.5f;
            if(h <= minh) my = cy/h;
            else if(cy < minh/2) my = cy/minh;
            else if(cy >= h - minh/2) my = 1 - (h - cy) / minh;
            else my = 0.5f;

            return checkalphamask(tex, mx, my) ? this : NULL;
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);
            gle::begin(GL_QUADS);
            float splitw = (minw ? min(minw, w) : w) / 2,
                  splith = (minh ? min(minh, h) : h) / 2,
                  vy = sy, ty = 0;
            loopi(3)
            {
                float vh = 0, th = 0;
                switch(i)
                {
                    case 0: if(splith < h - splith) { vh = splith; th = 0.5f; } else { vh = h; th = 1; } break;
                    case 1: vh = h - 2*splith; th = 0; break;
                    case 2: vh = splith; th = 0.5f; break;
                }
                float vx = sx, tx = 0;
                loopj(3)
                {
                    float vw = 0, tw = 0;
                    switch(j)
                    {
                        case 0: if(splitw < w - splitw) { vw = splitw; tw = 0.5f; } else { vw = w; tw = 1; } break;
                        case 1: vw = w - 2*splitw; tw = 0; break;
                        case 2: vw = splitw; tw = 0.5f; break;
                    }
                    quad(vx, vy, vw, vh, tx, ty, tw, th);
                    vx += vw;
                    tx += tw;
                    if(tx >= 1) break;
                }
                vy += vh;
                ty += th;
                if(ty >= 1) break;
            }
            gle::end();

            Object::draw(sx, sy);
        }
    };

    struct BorderedImage : Image
    {
        float texborder, screenborder;

        BorderedImage(Texture *tex, float texborder, float screenborder) : Image(tex), texborder(texborder), screenborder(screenborder) {}

        void layout()
        {
            Object::layout();

            w = max(w, 2*screenborder);
            h = max(h, 2*screenborder);
        }

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o) return o;
            if(tex->bpp < 32 || !uialphatest) return this;

            float mx, my;
            if(cx < screenborder) mx = cx/screenborder*texborder;
            else if(cx >= w - screenborder) mx = 1-texborder + (cx - (w - screenborder))/screenborder*texborder;
            else mx = texborder + (cx - screenborder)/(w - 2*screenborder)*(1 - 2*texborder);
            if(cy < screenborder) my = cy/screenborder*texborder;
            else if(cy >= h - screenborder) my = 1-texborder + (cy - (h - screenborder))/screenborder*texborder;
            else my = texborder + (cy - screenborder)/(h - 2*screenborder)*(1 - 2*texborder);

            return checkalphamask(tex, mx, my) ? this : NULL;
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);
            gle::begin(GL_QUADS);
            float vy = sy, ty = 0;
            loopi(3)
            {
                float vh = 0, th = 0;
                switch(i)
                {
                    case 0: vh = screenborder; th = texborder; break;
                    case 1: vh = h - 2*screenborder; th = 1 - 2*texborder; break;
                    case 2: vh = screenborder; th = texborder; break;
                }
                float vx = sx, tx = 0;
                loopj(3)
                {
                    float vw = 0, tw = 0;
                    switch(j)
                    {
                        case 0: vw = screenborder; tw = texborder; break;
                        case 1: vw = w - 2*screenborder; tw = 1 - 2*texborder; break;
                        case 2: vw = screenborder; tw = texborder; break;
                    }
                    quad(vx, vy, vw, vh, tx, ty, tw, th);
                    vx += vw;
                    tx += tw;
                }
                vy += vh;
                ty += th;
            }
            gle::end();

            Object::draw(sx, sy);
        }
    };

    struct TiledImage : Image
    {
        float tilew, tileh;

        TiledImage(Texture *tex, float tileh = 0, float tilew = 0,float minw = 0, float minh = 0) : Image(tex, minw, minh), tilew(tilew), tileh(tileh) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o) return o;
            if(tex->bpp < 32 || !uialphatest) return this;

            float dx = fmod(cx, tilew),
                  dy = fmod(cy, tileh);

            return checkalphamask(tex, dx/tilew, dy/tileh) ? this : NULL;
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);

            //we cannot use the built in OpenGL texture repeat with clamped textures.
            if(tex->clamp)
            {
                float dx = 0, dy = 0;
                gle::begin(GL_QUADS);
                while(dx < w)
                {
                    while(dy < h)
                    {
                        float dw = min(tilew, w - dx),
                              dh = min(tileh, h - dy);

                        quad(sx + dx, sy + dy, dw, dh, 0, 0, dw / tilew, dh / tileh);

                        dy += tileh;
                    }
                    dy = 0;
                    dx += tilew;
                }

                gle::end();
            }
            else
            {
                gle::begin(GL_TRIANGLE_STRIP);
                quadtri(sx, sy, w, h, 0, 0, w/tilew, h/tileh);
                gle::end();
            }

            Object::draw(sx, sy);
        }
    };

    struct ModelPreview : Filler
    {
        const char *mdl;
        int anim;
        vector <const char *> attachments[2];
        dynent ent;

        ModelPreview(float minw, float minh, const char *m, int a, const char *attach) : Filler(minw, minh), mdl(newstring(m))
        {
            int aprim = (a & ANIM_INDEX) | ANIM_LOOP;
            int asec = ((a >> 9) & ANIM_INDEX);
            if(asec) asec |= ANIM_LOOP;

            anim = aprim | (asec << ANIM_SECONDARY);

            vector<char *> elems;
            explodelist(attach, elems);

            loopv(elems)
                attachments[i % 2].add(elems[i]);
        }
        ~ModelPreview()
        {
            delete[] mdl;
            loopi(2) attachments[i].deletearrays();
        }

        void draw(float sx, float sy)
        {
            glDisable(GL_BLEND);
            // GL_SCISSOR_TEST causes problems with rendering
            // disable it and restore it afterwards.
            if(clipstack.length()) glDisable(GL_SCISSOR_TEST);

            int x = floor( (sx + world->margin) * screenw / world->w),
                dx = ceil(w * screenw / world->w),
                y = ceil(( 1 - (h + sy) ) * world->size),
                dy = ceil(h * world->size);

            gle::disable();
            modelpreview::start(x, y, dx, dy, false, clipstack.length() >= 1);

            model *m = loadmodel(mdl);
            if(m)
            {
                vec center, radius;
                m->boundbox(center, radius);
                float dist = 2.0f * max(radius.magnitude2(), 1.1f * radius.z),
                    yaw = fmod(totalmillis / 10000.f * 360.f, 360.f);
                vec o(-center.x, dist - center.y, -0.1f * dist - center.z);

                vector<modelattach> attach;

                if(attachments[1].length())
                {
                    attach.reserve(attachments[1].length());
                    loopv(attachments[1])
                    {
                        attach.add(modelattach(attachments[0][i], attachments[1][i]));
                    }

                    attach.add(modelattach());
                }

                rendermodel(mdl, anim, o, yaw, 0, 0, 0, &ent, attach.getbuf(), 0, 0, 1);

            }

            //note that modelpreview::start changes the clip area via preparegbuffer, we restore it here.
            if(clipstack.length()) clipstack.last().scissor();
            modelpreview::end();

            hudshader->set();
            gle::defvertex(2);
            gle::deftexcoord0();
            glEnable(GL_BLEND);
            if(clipstack.length()) glEnable(GL_SCISSOR_TEST);

            Object::draw(sx, sy);
        }
    };

    struct Font : Object
    {
        const char *font;

        Font(const char *f) : font(newstring(f)) {}
        ~Font() { delete[] font; }

        void layout()
        {
            pushfont();
            setfont(font);

            Object::layout();

            popfont();
        }

        void draw(float sx, float sy)
        {
            pushfont();
            setfont(font);

            Object::draw(sx, sy);

            popfont();
        }
    };

    // default size of text in terms of rows per screenful
    VARP(uitextrows, 1, 40, 200);
    FVAR(uitextscale, 1, 1.f / uitextrows, 0);
    FVAR(uicontextscale, 1, 0, 0);

    enum textstyle
    {
        UI = 0,
        CONSOLE
    };

    struct Text : Object
    {
        const char *str;
        float scale;
        float wrap;
        Color color;
        textstyle style;

        Text(const char *str, float scale = 1, float wrap = -1, const Color &color = Color(255, 255, 255), textstyle style = UI)
            : str(newstring(str)), scale(scale), wrap(wrap), color(color), style(style) {}
        ~Text() { delete[] str; }

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            return o ? o : this;
        }

        float drawscale() const { return scale / FONTH * (style == CONSOLE ? uicontextscale : uitextscale); }

        void draw(float sx, float sy)
        {
            float k = drawscale();
            pushhudmatrix();
            hudmatrix.scale(k, k, 1);
            flushhudmatrix();
            draw_text(str, int(sx/k), int(sy/k), color.r, color.g, color.b, color.a, -1, wrap <= 0 ? -1 : wrap/k);

            pophudmatrix();
            gle::colorf(1, 1, 1);

            Object::draw(sx, sy);
        }

        void layout()
        {
            Object::layout();

            int tw, th;
            float k = drawscale();
            text_bounds(str, tw, th, wrap <= 0 ? -1 : wrap/k);

            if(wrap <= 0)
                w = max(w, tw*k);
            else
                w = max(w, min(wrap, tw*k));
            h = max(h, th*k);
        }
    };

    struct EvalText : Object
    {
        uint *cmd;
        float scale;
        float wrap;
        Color color;
        textstyle style;

        tagval result;

        EvalText(uint *cmd, float scale = 1, float wrap = -1, const Color &color = Color(255, 255, 255), textstyle style = UI) : cmd(cmd), scale(scale), wrap(wrap), color(color), style(style) { keepcode(cmd); }
        ~EvalText() { freecode(cmd); result.cleanup(); }

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            return o ? o : this;
        }

        float drawscale() const { return scale / FONTH * (style == CONSOLE ? uicontextscale : uitextscale); }

        void draw(float sx, float sy)
        {
            float k = drawscale();
            pushhudmatrix();
            hudmatrix.scale(k, k, 1);
            flushhudmatrix();

            draw_text(result.getstr(), int(sx/k), int(sy/k), color.r, color.g, color.b, color.a, -1, wrap <= 0 ? -1 : wrap/k);

            pophudmatrix();
            gle::colorf(1, 1, 1);

            Object::draw(sx, sy);
        }

        void layout()
        {
            executeret(cmd, result);
            Object::layout();

            int tw, th;
            float k = drawscale();
            text_bounds(result.getstr(), tw, th, wrap <= 0 ? -1 : wrap/k);
            if(wrap <= 0) w = max(w, tw*k);
            else w = max(w, min(wrap, tw*k));
            h = max(h, th*k);
        }
    };

    struct Console : Filler
    {
        Console(float minw = 0, float minh = 0) : Filler(minw, minh) {}

        float drawscale() const { return uicontextscale / FONTH; }

        void draw(float sx, float sy)
        {
            Object::draw(sx, sy);

            float k = drawscale();
            pushhudmatrix();
            hudmatrix.translate(sx, sy, 0);
            hudmatrix.scale(k, k, 1);
            flushhudmatrix();
            renderfullconsole(w/k, h/k);
            pophudmatrix();
        }
    };

    struct TextEditor;
    TextEditor *textediting;
    int refreshrepeat = 0;

    struct TextEditor : Object
    {
        float scale, offsetx, offsety;
        editor *edit;
        char *keyfilter;

        const char *name, *initval;
        int length, height, mode;

        TextEditor(const char *name, int length, int height, float scale = 1, const char *initval = NULL, int mode = EDITORUSED, const char *keyfilter = NULL) : scale(scale), offsetx(0), offsety(0), edit(NULL), keyfilter(keyfilter ? newstring(keyfilter) : NULL), name(newstring(name)), initval(initval ? newstring(initval) : NULL), length(length), height(height), mode(mode) {}
        ~TextEditor()
        {
            DELETEA(keyfilter);
            DELETEA(name);
            DELETEA(initval);
            if(this == textediting) textediting = NULL;
            refreshrepeat++;
        }

        float drawscale() const { return scale / FONTH * uitextscale; }

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            return o ? o : this;
        }

        Object *hover(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        Object *select(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        virtual void commit() { }

        void hovering(float cx, float cy)
        {
            if(isselected(this) && isfocused(this))
            {
                float k = drawscale();
                bool dragged = max(fabs(cx - offsetx), fabs(cy - offsety)) > (FONTH/8.0f)*k;
                edit->hit(int(floor(cx/k - FONTW/2)), int(floor(cy/k)), dragged);
            }
        }

        void selected(float cx, float cy)
        {
            setfocus(this);
            edit->mark(false);
            offsetx = cx;
            offsety = cy;
        }

        bool hoverkey(int code, bool isdown)
        {
            switch(code)
            {
                case SDLK_LEFT:
                case SDLK_RIGHT:
                case SDLK_UP:
                case SDLK_DOWN:
                case -4:
                case -5:
                    if(isdown) edit->key(code);
                    return true;
            }
            return Object::hoverkey(code, isdown);
        }

        bool key(int code, bool isdown)
        {
            if(Object::key(code, isdown)) return true;
            if(!isfocused(this)) return false;
            switch(code)
            {
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_TAB:
                    if(edit->maxy != 1) break;

                case SDLK_ESCAPE:
                    setfocus(NULL);
                    return true;
            }
            if(isdown) edit->key(code);
            return true;
        }

        virtual void resetvalue()
        {
            if(initval && strcmp(edit->lines[0].text, initval))
                edit->clear(initval);
        }

        void layout()
        {
            Object::layout();

            editor *lastedit = edit;
            edit = useeditor(name, mode, false, initval);
            if(lastedit != edit)
            {
                edit->linewrap = length<0;
                edit->maxx = edit->linewrap ? -1 : length;
                edit->maxy = height <= 0 ? 1 : -1;
                edit->pixelwidth = abs(length)*FONTW;

                if(!edit->linewrap || edit->maxy != 1)
                    edit->pixelheight = FONTH*max(height, 1);
            }
            if(!isfocused(this) && edit->mode == EDITORFOCUSED)
                resetvalue();
            if(edit->linewrap && edit->maxy==1)
            {
                int temp;
                text_bounds(edit->lines[0].text, temp, edit->pixelheight, edit->pixelwidth); //only single line editors can have variable height
            }

            w = max(w, (edit->pixelwidth + FONTW) * drawscale());
            h = max(h, edit->pixelheight * drawscale());
        }

        void draw(float sx, float sy)
        {
            pushhudmatrix();
            hudmatrix.translate(sx, sy, 0);
            hudmatrix.scale(drawscale(), drawscale(), 1);
            flushhudmatrix();

            edit->draw(FONTW/2, 0, 0xFFFFFF, isfocused(this));

            pophudmatrix();

            Object::draw(sx, sy);
        }

        const int gettype() const { return TYPE_TEXTEDITOR; }
    };

    static const char *getsval(ident *id, const char *val = "")
    {
        switch(id->type)
        {
            case ID_VAR: val = intstr(*id->storage.i); break;
            case ID_FVAR: val = floatstr(*id->storage.f); break;
            case ID_SVAR: val = *id->storage.s; break;
            case ID_ALIAS: val = id->getstr(); break;
        }
        return val;
    }

    struct Field : TextEditor
    {
        ident *id;
        uint *onchange;

        Field(ident *id, int length, uint *onchange, float scale = 1, const char *keyfilter = NULL) : TextEditor(id->name, length, 0, scale, NULL, EDITORFOCUSED, keyfilter), id(id), onchange(onchange) { keepcode(onchange); }
        ~Field() { freecode(onchange); }

        void commit()
        {
            updateval(id, edit->lines[0].text, onchange);
        }

        bool hoverkey(int code, bool isdown)
        {
            return key(code, isdown) || Object::hoverkey(code, isdown);
        }

        void resetvalue()
        {
            const char *str = getsval(id);
            if(strcmp(edit->lines[0].text, str)) edit->clear(str);
        }

        bool key(int code, bool isdown)
        {
            if(Object::key(code, isdown)) return true;
            if(!isfocused(this)) return false;

            switch(code)
            {
                case SDLK_ESCAPE:
                    setfocus(NULL);
                    return true;
                case SDLK_KP_ENTER:
                case SDLK_RETURN:
                case SDLK_TAB:
                    commit();
                    setfocus(NULL);
                    return true;
            }
            if(isdown) edit->key(code);
            return true;
        }
    };

    bool input(const char *str, int len)
    {
        if(!focused || focused->gettype() != TYPE_TEXTEDITOR) return false;

        TextEditor *field = (TextEditor *) focused;
        if(field->keyfilter)
        {
            vector<char> filtered;
            loopi(len) if(strchr(field->keyfilter, str[i]))
                filtered.add(str[i]);
            field->edit->input(filtered.getbuf(), filtered.length());

        }
        else field->edit->input(str, len);

        return true;
    }

    struct NamedObject : Object
    {
        char *name;

        NamedObject(const char *name) : name(newstring(name)) {}
        ~NamedObject() { delete[] name; }

        const char *getname() const { return name; }
    };

    struct Tag : NamedObject
    {
        Tag(const char *name) : NamedObject(name) {}
        const int gettype() const { return TYPE_TAG; }
    };

    struct Window : NamedObject
    {
        uint *onhide;

        Window(const char *name, uint *onhide = NULL) : NamedObject(name), onhide(onhide)
        { keepcode(onhide); }
        ~Window() { freecode(onhide); }

        void hidden()
        {
            if(onhide) execute(onhide);
        }

        const int gettype() const { return TYPE_WINDOW; }
    };

    struct Overlay : Window
    {
        Overlay(const char *name, uint *onhide = NULL) : Window(name, onhide) {}
        ~Overlay() {}

        bool takesinput() const { return false; }

        Object *target(float, float) {return NULL;}
        Object *hover(float, float) {return NULL;}
        Object *select(float, float) {return NULL;}
    };

    vector<Object *> build;

    Window *buildwindow(const char *name, uint *contents, uint *onhide = NULL, int noinput = 0)
    {
        Window *window = noinput ? new Overlay(name, onhide) : new Window(name, onhide);
        build.add(window);
        execute(contents);
        build.pop();
        return window;
    }

    ICOMMAND(showui, "seei", (const char *name, uint *contents, uint *onhide, int *noinput),
    {
        if(build.length()) { intret(0); return; }
        Window *oldwindow = (Window *) world->findname(TYPE_WINDOW, name, false);
        if(oldwindow)
        {
            oldwindow->hidden();
            world->remove(oldwindow);
        }
        Window *window = buildwindow(name, contents, onhide, *noinput);
        world->children.add(window);
        window->parent = world;
        intret(1);
    });

    bool hideui(const char *name)
    {
        Window *window = (Window *) world->findname(TYPE_WINDOW, name, false);
        if(window)
        {
            window->hidden();
            world->remove(window);
        }
        return window!=NULL;
    }

    ICOMMAND(hideui, "s", (const char *name), intret(hideui(name) ? 1 : 0));

    ICOMMAND(replaceui, "sse", (const char *wname, const char *tname, uint *contents),
    {
        if(build.length()) { intret(0); return; }
        Window *window = (Window *) world->findname(TYPE_WINDOW, wname, false);
        if(!window) { intret(0); return; }
        Tag *tag = (Tag *) window->findname(TYPE_TAG, tname);
        if(!tag) { intret(0); return; }
        tag->children.deletecontents();
        build.add(tag);
        execute(contents);
        build.pop();
        intret(1);
    });

    int numui()
    {
        int n = 0;
        loopv(world->children)
        {
            if(world->children[i]->gettype() == TYPE_WINDOW)
                n++;
        }
        return n;
    }

    void hideallui()
    {
        loopv(world->children)
        {
            if(world->children[i]->gettype() != TYPE_WINDOW)
                continue;

            Window *window = (Window *) world->children[i];
            window->hidden();
            world->remove(window);
        }
    }

    bool uivisible(const char *name)
    {
        return world->findname(TYPE_WINDOW, name, false);
    }

    bool toggleui(const char *name)
    {
        Window *w = (Window *) world->findname(TYPE_WINDOW, name, false);
        if(w)
        {
            w->hidden();
            world->remove(w);
            return false;
        }
        defformatstring(cmd, "show%s", name);
        execident(cmd);
        return true;
    }

    void holdui(const char *name, bool on)
    {
        Window *w = (Window *) world->findname(TYPE_WINDOW, name, false);
        if(on)
        {
            if(w) return;
            defformatstring(cmd, "show%s", name);
            execident(cmd);
        }
        else if(w)
        {
            w->hidden();
            world->remove(w);
        }

    }

    void addui(Object *o, uint *children)
    {
        if(build.length())
        {
            o->parent = build.last();
            o->adjust = o->parent->adjust & ALIGN_MASK;
            build.last()->children.add(o);
        }
        if(children[0])
        {
            build.add(o);
            execute(children);
            build.pop();
        }
        o->layout();
        if(!build.length()) world->layout();
    }

    COMMAND(hideallui, "");
    ICOMMAND(uivisible, "s", (const char *name), intret(uivisible(name)));
    ICOMMAND(holdui, "sD", (const char *name, int *down), holdui(name, *down != 0));
    ICOMMAND(toggleui, "s", (const char *name), intret(toggleui(name)));

    static inline float parsepixeloffset(const tagval *t, int size)
    {
        switch(t->type)
        {
            case VAL_INT: return t->i;
            case VAL_FLOAT: return t->f;
            case VAL_NULL: return 0;
            default:
            {
                const char *s = t->getstr();
                char *end;
                float val = strtod(s, &end);
                return *end == 'p' ? val/size : val;
            }
        }
    }

    ICOMMAND(uialign, "ii", (int *xalign, int *yalign),
    {
        if(build.length()) build.last()->adjust = (build.last()->adjust & ~ALIGN_MASK) | ((clamp(*xalign, -1, 1)+2)<<ALIGN_HSHIFT) | ((clamp(*yalign, -1, 1)+2)<<ALIGN_VSHIFT);
    });

    ICOMMAND(uiclamp, "iiii", (int *left, int *right, int *bottom, int *top),
    {
        if(build.length()) build.last()->adjust = (build.last()->adjust & ~CLAMP_MASK) |
                                                  (*left ? CLAMP_LEFT : 0) |
                                                  (*right ? CLAMP_RIGHT : 0) |
                                                  (*bottom ? CLAMP_BOTTOM : 0) |
                                                  (*top ? CLAMP_TOP : 0);
    });

    ICOMMAND(uitag, "se", (char *name, uint *children),
        addui(new Tag(name), children));

    ICOMMAND(uihlist, "fe", (float *space, uint *children),
        addui(new HorizontalList(*space), children));

    ICOMMAND(uivlist, "fe", (float *space, uint *children),
        addui(new VerticalList(*space), children));

    ICOMMAND(uitable, "ife", (int *columns, float *space, uint *children),
        addui(new Table(*columns, *space), children));

    ICOMMAND(uispace, "ffe", (float *spacew, float *spaceh, uint *children),
        addui(new Spacer(*spacew, *spaceh), children));

    ICOMMAND(uifill, "ffe", (float *minw, float *minh, uint *children),
        addui(new Filler(*minw, *minh), children));

    ICOMMAND(uiclip, "ffe", (float *clipw, float *cliph, uint *children),
        addui(new Clipper(*clipw, *cliph), children));

    ICOMMAND(uiscroll, "ffe", (float *clipw, float *cliph, uint *children),
        addui(new Scroller(*clipw, *cliph), children));

    ICOMMAND(uihscrollbar, "ffe", (float *arrowsize, float *arrowspeed, uint *children),
        addui(new HorizontalScrollBar(*arrowsize, *arrowspeed), children));

    ICOMMAND(uivscrollbar, "ffe", (float *arrowsize, float *arrowspeed, uint *children),
        addui(new VerticalScrollBar(*arrowsize, *arrowspeed), children));

    ICOMMAND(uiscrollbutton, "e", (uint *children),
        addui(new ScrollButton, children));

    ICOMMAND(uihslider, "rffeffie", (ident *var, float *vmin, float *vmax, uint *onchange, float *arrowsize, float *stepsize, int *steptime, uint *children),
        addui(new HorizontalSlider(var, *vmin, *vmax, onchange, *arrowsize, *stepsize ? *stepsize : 1, *steptime), children));

    ICOMMAND(uivslider, "rffeffie", (ident *var, float *vmin, float *vmax, uint *onchange, float *arrowsize, float *stepsize, int *steptime, uint *children),
        addui(new VerticalSlider(var, *vmin, *vmax, onchange, *arrowsize, *stepsize ? *stepsize : 1, *steptime), children));

    ICOMMAND(uisliderbutton, "e", (uint *children),
        addui(new SliderButton, children));

    ICOMMAND(uioffset, "ffe", (float *offsetx, float *offsety, uint *children),
        addui(new Offsetter(*offsetx, *offsety), children));

    ICOMMAND(uibutton, "ee", (uint *onselect, uint *children),
        addui(new Button(onselect), children));

    ICOMMAND(uicond, "ee", (uint *cond, uint *children),
        addui(new Conditional(cond), children));

    ICOMMAND(uicondbutton, "eee", (uint *cond, uint *onselect, uint *children),
        addui(new ConditionalButton(cond, onselect), children));

    ICOMMAND(uitoggle, "eefe", (uint *cond, uint *onselect, float *split, uint *children),
        addui(new Toggle(cond, onselect, *split), children));

    ICOMMAND(uitooltip, "e", (uint *children),
        if(build.empty() || build.last()->gettype() != TYPE_BUTTON) return;

        Button *button = (Button *) build.last();
        if(!button->tooltip) button->tooltip = new Object();
        build.add(button->tooltip);
        execute(children);
        build.pop();
    )

    ICOMMAND(uiimage, "sffe", (char *texname, float *minw, float *minh, uint *children),
        addui(new Image(textureload(texname, 3, true, false), *minw, *minh), children));

    ICOMMAND(uithumbnail, "sffe", (char *texname, float *minw, float *minh, uint *children),
        addui(new Thumbnail(texname, *minw, *minh), children));

    ICOMMAND(uislotview, "iffe", (int *slotnum, float *minw, float *minh, uint *children),
        addui(new SlotViewer(*slotnum, *minw, *minh), children));

    ICOMMAND(uialtimage, "s", (char *texname),
    {
        if(build.empty() || build.last()->gettype() != TYPE_IMAGE) return;
        Image *image = (Image *) build.last();
        if(image && image->tex==notexture) image->tex = textureload(texname, 3, true, false);
    });

    ICOMMAND(uicolor, "iffe", (int *c, float *minw, float *minh, uint *children),
        addui(new Rectangle(Rectangle::SOLID, Color(*c), *minw, *minh), children));

    ICOMMAND(uimodcolor, "iffe", (int *c, float *minw, float *minh, uint *children),
        addui(new Rectangle(Rectangle::MODULATE, Color(*c), *minw, *minh), children));

    ICOMMAND(uistretchedimage, "sffe", (char *texname, float *minw, float *minh, uint *children),
        addui(new StretchedImage(textureload(texname, 3, true, false), *minw, *minh), children));

    ICOMMAND(uicroppedimage, "sfftttte", (char *texname, float *minw, float *minh, tagval *cropx, tagval *cropy, tagval *cropw, tagval *croph, uint *children),
        Texture *tex = textureload(texname, 3, true, false);
        addui(new CroppedImage(tex, *minw, *minh,
            parsepixeloffset(cropx, tex->xs),
            parsepixeloffset(cropy, tex->ys),
            parsepixeloffset(cropw, tex->xs),
            parsepixeloffset(croph, tex->ys)),  children));

    ICOMMAND(uiborderedimage, "stfe", (char *texname, tagval *texborder, float *screenborder, uint *children),
        Texture *tex = textureload(texname, 3, true, false);
        addui(new BorderedImage(tex,
                parsepixeloffset(texborder, tex->xs),
                *screenborder), children));

    ICOMMAND(uitiledimage, "sffffe", (char *texname, float *tilew, float *tileh, float *minw, float *minh, uint *children),
        Texture *tex = textureload(texname, 3, true, false);
        addui(new TiledImage(tex, *tilew <= 0 ? 1 : *tilew, *tileh <= 0 ? 1 : *tileh, *minw, *minh), children));

    ICOMMAND(uimodelpreview, "sisffe", (const char *model, int *anim, const char *attach, float *minw, float *minh, uint *children),
        addui(new ModelPreview(*minw, *minh, model, *anim, attach), children);
    )

    ICOMMAND(uifont, "se", (const char *f, uint *children),
             addui(new Font(f), children));

    ICOMMAND(uicolortext, "sffie", (char *text, float *scale, float *wrap, int *c, uint *children),
        addui(new Text(text, *scale <= 0 ? 1 : *scale, *wrap, Color(*c)), children));

    ICOMMAND(uitext, "sffe", (char *text, float *scale, float *wrap, uint *children),
        addui(new Text(text, *scale <= 0 ? 1 : *scale, *wrap), children));

    ICOMMAND(uicolorevaltext, "effie", (uint *cmd, float *scale, float *wrap, int *c, uint *children),
        addui(new EvalText(cmd, *scale <= 0 ? 1 : *scale, *wrap, Color(*c)), children));

    ICOMMAND(uievaltext, "effe", (uint *cmd, float *scale, float *wrap, uint *children),
        addui(new EvalText(cmd, *scale <= 0 ? 1 : *scale, *wrap), children));

    ICOMMAND(uicontext, "sfe", (char *text, float *wrap, uint *children),
        Text *o = new Text(text, 1, *wrap); o->style = CONSOLE;
        addui(o, children));

    ICOMMAND(uievalcontext, "efe", (uint *cmd, float *wrap, uint *children),
        EvalText *o = new EvalText(cmd, 1, *wrap); o->style = CONSOLE;
        addui(o, children));

    ICOMMAND(uitexteditor, "siifsise", (char *name, int *length, int *height, float *scale, char *initval, int *keep, char *filter, uint *children),
        addui(new TextEditor(name, *length, *height, *scale <= 0 ? 1 : *scale, initval, *keep ? EDITORFOREVER : EDITORUSED, filter[0] ? filter : NULL), children));

    ICOMMAND(uifield, "riefse", (ident *var, int *length, uint *onchange, float *scale, char *filter, uint *children),
        addui(new Field(var, *length, onchange, *scale <= 0 ? 1 : *scale, filter[0] ? filter : NULL), children));

    ICOMMAND(uiconsole, "ffe", (float *minw, float *minh, uint *children),
        addui(new Console(*minw, *minh), children));

    ICOMMAND(uivgradient, "iiffe", (int *c, int *c2, float *minw, float *minh, uint *children),
        addui(new Gradient(Gradient::SOLID, Gradient::VERTICAL, Color(*c), Color(*c2), *minw, *minh), children));

    ICOMMAND(uimodvgradient, "iiffe", (int *c, int *c2, float *minw, float *minh, uint *children),
        addui(new Gradient(Gradient::MODULATE, Gradient::VERTICAL, Color(*c), Color(*c2), *minw, *minh), children));

    ICOMMAND(uihgradient, "iiffe", (int *c, int *c2, float *minw, float *minh, uint *children),
        addui(new Gradient(Gradient::SOLID, Gradient::HORIZONTAL, Color(*c), Color(*c2), *minw, *minh), children));

    ICOMMAND(uimodhgradient, "iiffe", (int *c, int *c2, float *minw, float *minh, uint *children),
        addui(new Gradient(Gradient::MODULATE, Gradient::HORIZONTAL, Color(*c), Color(*c2), *minw, *minh), children));

    ICOMMAND(uioutline, "iffe", (int *c, float *minw, float *minh, uint *children),
        addui(new Outline(Color(*c), *minw, *minh), children));

    ICOMMAND(uiline, "iffe", (int *c, float *minw, float *minh, uint *children),
        addui(new Line(Color(*c), *minw, *minh), children));

    FVAR(cursorsensitivity, 1e-3f, 1, 1000);

    void resetcursor()
    {
        if(editmode || world->children.empty())
            cursorx = cursory = 0.5f;
    }

    //0 - centre always; 1 - free only when UIs are shown; 2 - always
    VARP(freecursor, 0, 1, 2);
    VARP(freeeditcursor, 0, 1, 2);

    static inline int cursormode()
    {
        return editmode ? freeeditcursor : freecursor;
    }

    bool movecursor(int &dx, int &dy)
    {
        if(cursormode() == 2 || (world->takesinput() && cursormode() >= 1))
        {
            cursorx = clamp( cursorx + dx * cursorsensitivity / screenw, 0.f, 1.f);
            cursory = clamp( cursory + dy * cursorsensitivity / screenh, 0.f, 1.f);
            if(editmode || cursormode() == 2)
            {
                if(cursorx != 1 && cursorx != 0) dx = 0;
                if(cursory != 1 && cursory != 0) dy = 0;
                return false;
            }
            return true;
        }
        return false;
    }

    bool hascursor()
    {
        if(mainmenu) return true;

        if(world->takesinput() && cursormode() >= 1)
        {
            if(world->target(cursorx*world->w, cursory*world->h))
                return true;
        }
        return false;
    }

    void getcursorpos(float &x, float &y)
    {
        if(cursormode() == 2 || (world->takesinput() && cursormode() >= 1))
        {
            x = cursorx;
            y = cursory;
        }
        else x = y = .5f;
    }

    bool keypress(int code, bool isdown)
    {
        if(!cursormode()) return false;
        switch(code)
        {
            case -5:
            case -4:
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_DOWN:
            case SDLK_UP:
            {
                if((focused && focused->hoverkey(code, isdown)) ||
                    (hovering && hovering->hoverkey(code, isdown)))
                    return true;
                return false;
            }
            case -1:
            {
                if(!hascursor()) return false;

                if(isdown)
                {
                    selected = world->select(cursorx*world->w, cursory*world->h);
                    if(selected) selected->selected(selectx, selecty);
                }
                else selected = NULL;
                return true;
            }

            default:
                return world->key(code, isdown) || textediting;
        }
    }

    VAR(mainmenu, 1, 1, 0);
    void clearmainmenu()
    {
        if(mainmenu && (isconnected() || haslocalclients()))
        {
            hideallui();
            mainmenu = 0;
        }
    }

    void setup()
    {
        main = new World();
        bgworld = new World();
        ldworld = new World();

        world = main;
    }

    void renderbackground()
    {
        world = bgworld;
        execident("showbackground");

        Object *lasttooltip = tooltip;
        tooltip = NULL;

        world->layout();
        render();

        tooltip = lasttooltip;

        hideui("background");
        world = main;
    }

    void renderprogress()
    {
        world = ldworld;
        execident("showloading");

        Object *lasttooltip = tooltip;
        tooltip = NULL;

        world->layout();
        render();

        tooltip = lasttooltip;

        hideui("loading");
        world = main;
    }

    int showchanges = 0;

    void calctextscale()
    {
        uitextscale = 1.0f/uitextrows;

        int tw = hudw, th = hudh;
        if(forceaspect) tw = int(ceil(th*forceaspect));
        gettextres(tw, th);
        uicontextscale = (FONTH*conscale)/th;
    }

    void update()
    {
        loopv(updatelater)
            updatelater[i].run();
        updatelater.shrink(0);

        if(showchanges && !world->findname(TYPE_WINDOW, NULL, false))
            execident("showchanges");

        if(mainmenu && !isconnected(true) && !world->children.length())
            execident("showmain");

        readyeditors();
        tooltip = NULL;
        calctextscale();
        //world->layout();

        if(hascursor())
        {
            hovering = world->hover(cursorx*world->w, cursory*world->h);
            if(hovering) hovering->hovering(hoverx, hovery);
        }
        else hovering = selected = NULL;

        world->layout();
        if(tooltip) { tooltip->layout(); tooltip->adjustchildren(); }
        flusheditors();

        bool wastextediting = textediting!=NULL;

        if(textediting && !isfocused(textediting))
            textediting->commit();

        if(!focused || focused->gettype() != TYPE_TEXTEDITOR)
            textediting = NULL;
        else
            textediting = (TextEditor *) focused;

        if(refreshrepeat || (textediting!=NULL) != wastextediting)
        {
            textinput(textediting != NULL, TI_GUI);
            keyrepeat(textediting != NULL, KR_GUI);
            refreshrepeat = 0;
        }
    }

    void render()
    {
        if(world->children.empty()) return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        hudmatrix.ortho(world->x, world->x + world->w, world->y + world->h, world->y, -1, 1);
        resethudmatrix();
        hudshader->set();

        gle::colorf(1, 1, 1);
        gle::defvertex(2);
        gle::deftexcoord0();

        world->draw();

        if(tooltip)
        {
            float x = world->x + (1 + world->margin * 2) * cursorx;
            float y = world->y + ( 1 ) * cursory;
            x += 0.01; y += 0.01;

            if(x + tooltip->w * .95 > 1 + world->margin)
            {
                x -= tooltip->w + 0.02;
                if(x <= -world->margin) x = -world->margin + 0.02;
            }
            if(y + tooltip->h * .95 > 1)
            {
                y -= tooltip->h + 0.02;
                if(y < 0) y = 0;
            }

            tooltip->draw(x, y);
        }

        gle::disable();
    }
}

struct change
{
    int type;
    const char *desc;

    change() {}
    change(int type, const char *desc) : type(type), desc(desc) {}
};
static vector<change> needsapply;

VARP(applydialog, 0, 1, 1);

void addchange(const char *desc, int type)
{
    if(!applydialog) return;
    loopv(needsapply) if(!strcmp(needsapply[i].desc, desc)) return;
    needsapply.add(change(type, desc));
    UI::showchanges = 1;
}

void clearchanges(int type)
{
    loopv(needsapply)
    {
        if(needsapply[i].type&type)
        {
            needsapply[i].type &= ~type;
            if(!needsapply[i].type) needsapply.remove(i--);
        }
    }
    if(needsapply.empty())
    {
        UI::showchanges = 0;
        UI::hideui("changes");
    }
}

void applychanges()
{
    static uint *resetgl = compilecode("resetgl");
    static uint *resetsound = compilecode("resetsound");
    static uint *resetshaders = compilecode("resetshaders");

    int changetypes = 0;
    loopv(needsapply) changetypes |= needsapply[i].type;
    if(changetypes&CHANGE_GFX) UI::updatelater.add().schedule(resetgl);
    else if(changetypes&CHANGE_SHADERS) UI::updatelater.add().schedule(resetshaders);
    if(changetypes&CHANGE_SOUND) UI::updatelater.add().schedule(resetsound);
}

ICOMMAND(pendingchanges, "", (), intret(needsapply.length()));
ICOMMAND(clearchanges, "", (), clearchanges(CHANGE_GFX|CHANGE_SOUND|CHANGE_SHADERS));
COMMAND(applychanges, "");

ICOMMAND(loopchanges, "se", (char *var, uint *body),
{
    loopv(needsapply) { alias(var, needsapply[i].desc); execute(body); }
});
