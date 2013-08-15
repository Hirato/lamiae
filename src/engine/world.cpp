// world.cpp: core map management stuff

#include "engine.h"

VARR(mapversion, 1, OCTAVERSION, 0);
VARNR(mapscale, worldscale, 1, 0, 0);
VARNR(mapsize, worldsize, 1, 0, 0);
SVARR(maptitle, "Untitled Map by Unknown");

VAR(octaentsize, 0, 64, 1024);
VAR(entselradius, 0, 2, 10);

static inline void mmboundbox(const entity &e, model *m, vec &center, vec &radius)
{
    m->boundbox(center, radius);
    if(e.attr[4] > 0) { float scale = e.attr[4]/100.0f; center.mul(scale); radius.mul(scale); }
    rotatebb(center, radius, e.attr[1], e.attr[2], e.attr[3]);
}

static inline void mmcollisionbox(const entity &e, model *m, vec &center, vec &radius)
{
    m->collisionbox(center, radius);
    if(e.attr[4] > 0) { float scale = e.attr[4]/100.0f; center.mul(scale); radius.mul(scale); }
    rotatebb(center, radius, e.attr[1], e.attr[2], e.attr[3]);
}

bool getentboundingbox(const extentity &e, ivec &o, ivec &r)
{
    switch(e.type)
    {
        case ET_EMPTY:
            return false;
        case ET_MAPMODEL:
        {
            model *m = loadmapmodel(e.attr[0]);
            if(m)
            {
                vec center, radius;
                mmboundbox(e, m, center, radius);
                center.add(e.o);
                radius.max(entselradius);
                o = vec(center).sub(radius);
                r = vec(center).add(radius).add(1);
                break;
            }
        }
        // invisible mapmodels use entselradius
        default:
            o = vec(e.o).sub(entselradius);
            r = vec(e.o).add(entselradius+1);
            break;
    }
    return true;
}

enum
{
    MODOE_ADD      = 1<<0,
    MODOE_UPDATEBB = 1<<1
};

void modifyoctaentity(int flags, int id, extentity &e, cube *c, const ivec &cor, int size, const ivec &bo, const ivec &br, int leafsize, vtxarray *lastva = NULL)
{
    loopoctabox(cor, size, bo, br)
    {
        ivec o(i, cor, size);
        vtxarray *va = c[i].ext && c[i].ext->va ? c[i].ext->va : lastva;
        if(c[i].children != NULL && size > leafsize)
            modifyoctaentity(flags, id, e, c[i].children, o, size>>1, bo, br, leafsize, va);
        else if(flags&MODOE_ADD)
        {
            if(!c[i].ext || !c[i].ext->ents) ext(c[i]).ents = new octaentities(o, size);
            octaentities &oe = *c[i].ext->ents;
            switch(e.type)
            {
                case ET_MAPMODEL:
                    if(loadmapmodel(e.attr[0]))
                    {
                        if(va)
                        {
                            va->bbmin.x = -1;
                            if(oe.mapmodels.empty()) va->mapmodels.add(&oe);
                        }
                        oe.mapmodels.add(id);
                        oe.bbmin.min(bo).max(oe.o);
                        oe.bbmax.max(br).min(ivec(oe.o).add(oe.size));
                        break;
                    }
                    // invisible mapmodel
                default:
                    oe.other.add(id);
                    break;
            }

        }
        else if(c[i].ext && c[i].ext->ents)
        {
            octaentities &oe = *c[i].ext->ents;
            switch(e.type)
            {
                case ET_MAPMODEL:
                    if(loadmapmodel(e.attr[0]))
                    {
                        oe.mapmodels.removeobj(id);
                        if(va)
                        {
                            va->bbmin.x = -1;
                            if(oe.mapmodels.empty()) va->mapmodels.removeobj(&oe);
                        }
                        oe.bbmin = oe.bbmax = oe.o;
                        oe.bbmin.add(oe.size);
                        loopvj(oe.mapmodels)
                        {
                            extentity &e = *entities::getents()[oe.mapmodels[j]];
                            ivec eo, er;
                            if(getentboundingbox(e, eo, er))
                            {
                                oe.bbmin.min(eo);
                                oe.bbmax.max(er);
                            }
                        }
                        oe.bbmin.max(oe.o);
                        oe.bbmax.min(ivec(oe.o).add(oe.size));
                        break;
                    }
                    // invisible mapmodel
                default:
                    oe.other.removeobj(id);
                    break;
            }
            if(oe.mapmodels.empty() && oe.other.empty())
                freeoctaentities(c[i]);
        }
        if(c[i].ext && c[i].ext->ents) c[i].ext->ents->query = NULL;
        if(va && va!=lastva)
        {
            if(lastva)
            {
                if(va->bbmin.x < 0) lastva->bbmin.x = -1;
            }
            else if(flags&MODOE_UPDATEBB) updatevabb(va);
        }
    }
}

vector<int> outsideents;

static bool modifyoctaent(int flags, int id, extentity &e)
{
    if(flags&MODOE_ADD ? e.flags&EF_OCTA : !(e.flags&EF_OCTA)) return false;

    ivec o, r;
    if(!getentboundingbox(e, o, r)) return false;

    if(!insideworld(e.o))
    {
        int idx = outsideents.find(id);
        if(flags&MODOE_ADD)
        {
            if(idx < 0) outsideents.add(id);
        }
        else if(idx >= 0) outsideents.removeunordered(idx);
    }
    else
    {
        int leafsize = octaentsize, limit = max(r.x - o.x, max(r.y - o.y, r.z - o.z));
        while(leafsize < limit) leafsize *= 2;
        int diff = ~(leafsize-1) & ((o.x^r.x)|(o.y^r.y)|(o.z^r.z));
        if(diff && (limit > octaentsize/2 || diff < leafsize*2)) leafsize *= 2;
        modifyoctaentity(flags, id, e, worldroot, ivec(0, 0, 0), worldsize>>1, o, r, leafsize);
    }
    e.flags ^= EF_OCTA;
    if(e.type == ET_LIGHT) clearlightcache(id);
    else if(e.type == ET_PARTICLES) clearparticleemitters();
    return true;
}

static inline bool modifyoctaent(int flags, int id)
{
    vector<extentity *> &ents = entities::getents();
    return ents.inrange(id) && modifyoctaent(flags, id, *ents[id]);
}

static inline void addentity(int id)    { modifyoctaent(MODOE_ADD|MODOE_UPDATEBB, id); }
static inline void removeentity(int id) { modifyoctaent(MODOE_UPDATEBB, id); }

void freeoctaentities(cube &c)
{
    if(!c.ext) return;
    if(entities::getents().length())
    {
        while(c.ext->ents && !c.ext->ents->mapmodels.empty()) removeentity(c.ext->ents->mapmodels.pop());
        while(c.ext->ents && !c.ext->ents->other.empty())     removeentity(c.ext->ents->other.pop());
    }
    if(c.ext->ents)
    {
        delete c.ext->ents;
        c.ext->ents = NULL;
    }
}

void entitiesinoctanodes()
{
    vector<extentity *> &ents = entities::getents();
    loopv(ents) modifyoctaent(MODOE_ADD, i, *ents[i]);
}

static inline void findents(octaentities &oe, int low, int high, bool notspawned, const vec &pos, const vec &radius, vector<int> &found)
{
    vector<extentity *> &ents = entities::getents();
    loopv(oe.other)
    {
        int id = oe.other[i];
        extentity &e = *ents[id];
        if(e.type >= low && e.type <= high && (e.spawned() || notspawned) && vec(e.o).mul(radius).squaredlen() <= 1) found.add(id);
    }
}

static inline void findents(cube *c, const ivec &o, int size, const ivec &bo, const ivec &br, int low, int high, bool notspawned, const vec &pos, const vec &radius, vector<int> &found)
{
    loopoctabox(o, size, bo, br)
    {
        if(c[i].ext && c[i].ext->ents) findents(*c[i].ext->ents, low, high, notspawned, pos, radius, found);
        if(c[i].children && size > octaentsize)
        {
            ivec co(i, o, size);
            findents(c[i].children, co, size>>1, bo, br, low, high, notspawned, pos, radius, found);
        }
    }
}

void findents(int low, int high, bool notspawned, const vec &pos, const vec &radius, vector<int> &found)
{
    vec invradius(1/radius.x, 1/radius.y, 1/radius.z);
    ivec bo = vec(pos).sub(radius).sub(1),
         br = vec(pos).add(radius).add(1);
    int diff = (bo.x^br.x) | (bo.y^br.y) | (bo.z^br.z) | octaentsize,
        scale = worldscale-1;
    if(diff&~((1<<scale)-1) || uint(bo.x|bo.y|bo.z|br.x|br.y|br.z) >= uint(worldsize))
    {
        findents(worldroot, ivec(0, 0, 0), 1<<scale, bo, br, low, high, notspawned, pos, invradius, found);
        return;
    }
    cube *c = &worldroot[octastep(bo.x, bo.y, bo.z, scale)];
    if(c->ext && c->ext->ents) findents(*c->ext->ents, low, high, notspawned, pos, invradius, found);
    scale--;
    while(c->children && !(diff&(1<<scale)))
    {
        c = &c->children[octastep(bo.x, bo.y, bo.z, scale)];
        if(c->ext && c->ext->ents) findents(*c->ext->ents, low, high, notspawned, pos, invradius, found);
        scale--;
    }
    if(c->children && 1<<scale >= octaentsize) findents(c->children, ivec(bo).mask(~((2<<scale)-1)), 1<<scale, bo, br, low, high, notspawned, pos, invradius, found);
}

char *entname(entity &e)
{
    static string fullentname;
    copystring(fullentname, entities::entname(e.type));
    const char *einfo = entities::entnameinfo(e);
    if(*einfo)
    {
        concatstring(fullentname, ": ");
        concatstring(fullentname, einfo);
    }
    return fullentname;
}

extern selinfo sel;
extern bool havesel;
int entlooplevel = 0;
int efocus = -1, enthover = -1, oldhover = -1;
VAR(entorient, 1, -1, -1);
bool undonext = true;

VARF(entediting, 0, 0, 1, { if(!entediting) { entcancel(); efocus = enthover = -1; } });

bool noentedit()
{
    if(!editmode) { conoutf(CON_ERROR, "operation only allowed in edit mode"); return true; }
    return !entediting;
}

bool pointinsel(const selinfo &sel, const vec &o)
{
    return(o.x <= sel.o.x+sel.s.x*sel.grid
        && o.x >= sel.o.x
        && o.y <= sel.o.y+sel.s.y*sel.grid
        && o.y >= sel.o.y
        && o.z <= sel.o.z+sel.s.z*sel.grid
        && o.z >= sel.o.z);
}

vector<int> entgroup;

bool haveselent()
{
    return entgroup.length() > 0;
}

void entcancel()
{
    entgroup.shrink(0);
}

void entadd(int id)
{
    undonext = true;
    entgroup.add(id);
}

undoblock *newundoent()
{
    int numents = entgroup.length();
    if(numents <= 0) return NULL;
    undoblock *u = (undoblock *)new uchar[sizeof(undoblock) + numents*sizeof(undoent)];
    u->numents = numents;
    undoent *e = (undoent *)(u + 1);
    loopv(entgroup)
    {
        e->i = entgroup[i];
        e->e.attr.disown(); //points to random values; this causes problems
        e->e = *entities::getents()[entgroup[i]];
        e++;
    }
    return u;
}

void makeundoent()
{
    if(!undonext) return;
    undonext = false;
    oldhover = enthover;
    undoblock *u = newundoent();
    if(u) addundo(u);
}

void detachentity(extentity &e)
{
    if(!e.attached) return;
    e.attached->attached = NULL;
    e.attached = NULL;
}

VAR(attachradius, 1, 100, 1000);

void attachentity(extentity &e)
{
    switch(e.type)
    {
        case ET_SPOTLIGHT:
            break;

        default:
            if(e.type<ET_GAMESPECIFIC || !entities::mayattach(e)) return;
            break;
    }

    detachentity(e);

    vector<extentity *> &ents = entities::getents();
    int closest = -1;
    float closedist = 1e10f;
    loopv(ents)
    {
        extentity *a = ents[i];
        if(a->attached) continue;
        switch(e.type)
        {
            case ET_SPOTLIGHT:
                if(a->type!=ET_LIGHT) continue;
                break;

            default:
                if(e.type<ET_GAMESPECIFIC || !entities::attachent(e, *a)) continue;
                break;
        }
        float dist = e.o.dist(a->o);
        if(dist < closedist)
        {
            closest = i;
            closedist = dist;
        }
    }
    if(closedist>attachradius) return;
    e.attached = ents[closest];
    ents[closest]->attached = &e;
}

void attachentities()
{
    vector<extentity *> &ents = entities::getents();
    loopv(ents) attachentity(*ents[i]);
}

// convenience macros implicitly define:
// e         entity, currently edited ent
// n         int,    index to currently edited ent
#define addimplicit(f)  { if(entgroup.empty() && enthover>=0) { entadd(enthover); undonext = (enthover != oldhover); f; entgroup.drop(); } else f; }
#define entfocus(i, f)  { int n = efocus = (i); if(n>=0) { extentity &e = *entities::getents()[n]; f; } }
#define entedit(i, f) \
{ \
    entfocus(i, \
    int oldtype = e.type; \
    removeentity(n);  \
    f; \
    if(oldtype!=e.type) detachentity(e); \
    if(e.type!=ET_EMPTY) { addentity(n); if(oldtype!=e.type) attachentity(e); } \
    entities::editent(n, true)); \
    clearshadowcache(); \
}
#define addgroup(exp)   { loopv(entities::getents()) entfocus(i, if(exp) entadd(n)); }
#define setgroup(exp)   { entcancel(); addgroup(exp); }
#define groupeditloop(f){ entlooplevel++; int _ = efocus; loopv(entgroup) entedit(entgroup[i], f); efocus = _; entlooplevel--; }
#define groupeditpure(f){ if(entlooplevel>0) { entedit(efocus, f); } else groupeditloop(f); }
#define groupeditundo(f){ makeundoent(); groupeditpure(f); }
#define groupedit(f)    { addimplicit(groupeditundo(f)); }

vec getselpos()
{
    vector<extentity *> &ents = entities::getents();
    if(entgroup.length() && ents.inrange(entgroup[0])) return ents[entgroup[0]]->o;
    if(ents.inrange(enthover)) return ents[enthover]->o;
    return vec(sel.o);
}

undoblock *copyundoents(undoblock *u)
{
    entcancel();
    undoent *e = u->ents();
    loopi(u->numents)
        entadd(e[i].i);
    undoblock *c = newundoent();
    loopi(u->numents) if(e[i].e.type==ET_EMPTY)
        entgroup.removeobj(e[i].i);
    return c;
}

void pasteundoents(undoblock *u)
{
    undoent *ue = u->ents();
    loopi(u->numents)
        entedit(ue[i].i, (entity &)e = ue[i].e);
}

void entflip()
{
    if(noentedit()) return;
    int d = dimension(sel.orient);
    float mid = sel.s[d]*sel.grid/2+sel.o[d];
    groupeditundo(e.o[d] -= (e.o[d]-mid)*2);
}

void entrotate(int *cw)
{
    if(noentedit()) return;
    int d = dimension(sel.orient);
    int dd = (*cw<0) == dimcoord(sel.orient) ? R[d] : C[d];
    float mid = sel.s[dd]*sel.grid/2+sel.o[dd];
    vec s(sel.o.v);
    groupeditundo(
        e.o[dd] -= (e.o[dd]-mid)*2;
        e.o.sub(s);
        swap(e.o[R[d]], e.o[C[d]]);
        e.o.add(s);
    );
}

float getentyaw(const entity &e)
{
    switch(e.type)
    {
        case ET_MAPMODEL:
            return e.attr[1];
        case ET_PLAYERSTART:
            return e.attr[0];
        default:
            return entities::getentyaw(e);
    }
}

void entselectionbox(const entity &e, vec &eo, vec &es)
{
    model *m = NULL;
    const char *mname = entities::entmodel(e);
    if(mname) m = loadmodel(mname);
    else if(e.type == ET_MAPMODEL) m = loadmapmodel(e.attr[0]);

    if(m)
    {
        mmcollisionbox(e, m, eo, es);
        es.max(entselradius);
        eo.add(e.o);
    }
    else
    {
        es = vec(entselradius);
        eo = e.o;
    }

    eo.sub(es);
    es.mul(2);
}

VAR(entselsnap, 0, 0, 1);
VAR(entmovingshadow, 0, 1, 1);

extern void boxs(int orient, vec o, const vec &s);
extern void boxs3D(const vec &o, vec s, int g);
extern bool editmoveplane(const vec &o, const vec &ray, int d, float off, vec &handle, vec &dest, bool first);

int entmoving = 0;

void entdrag(const vec &ray)
{
    if(noentedit() || !haveselent()) return;

    float r = 0, c = 0;
    static vec dest, handle;
    vec eo, es;
    int d = dimension(entorient),
        dc= dimcoord(entorient);

    entfocus(entgroup.last(),
        entselectionbox(e, eo, es);

        if(!editmoveplane(e.o, ray, d, eo[d] + (dc ? es[d] : 0), handle, dest, entmoving==1))
            return;

        ivec g = dest;
        int z = g[d]&(~(sel.grid-1));
        g.add(sel.grid/2).mask(~(sel.grid-1));
        g[d] = z;

        r = (entselsnap ? g[R[d]] : dest[R[d]]) - e.o[R[d]];
        c = (entselsnap ? g[C[d]] : dest[C[d]]) - e.o[C[d]];
    );

    if(entmoving==1) makeundoent();
    groupeditpure(e.o[R[d]] += r; e.o[C[d]] += c);
    entmoving = 2;
}

VARP(showenthelpers, 0, 1, 2); // editmode, always
VARP(showentradius, 0, 1, 2); //1 when selected, 2 always
VARP(showentdir, 0, 1, 2);

void renderentring(const extentity &e, float radius, int axis)
{
    if(radius <= 0 || !showentradius) return;
    gle::defvertex();
    gle::begin(GL_LINE_LOOP);
    loopi(15)
    {
        vec p(e.o);
        const vec2 &sc = sincos360[i*(360/15)];
        p[axis>=2 ? 1 : 0] += radius*sc.x;
        p[axis>=1 ? 2 : 1] += radius*sc.y;
        gle::attrib(p);
    }
    gle::end();
}

void renderentsphere(const extentity &e, float radius)
{
    if(radius <= 0||!showentradius) return;
    loopk(3) renderentring(e, radius, k);
}

void renderentattachment(const extentity &e)
{
    if(!e.attached || !showentdir) return;
    gle::defvertex();
    gle::begin(GL_LINES);
    gle::attrib(e.o);
    gle::attrib(e.attached->o);
    gle::end();
}

void renderentarrow(const extentity &e, const vec &dir, float radius)
{
    if(radius <= 0 || !showentdir) return;
    float arrowsize = min(radius/8, 0.5f);
    vec target = vec(dir).mul(radius).add(e.o), arrowbase = vec(dir).mul(radius - arrowsize).add(e.o), spoke;
    spoke.orthogonal(dir);
    spoke.normalize();
    spoke.mul(arrowsize);

    gle::defvertex();

    gle::begin(GL_LINES);
    gle::attrib(e.o);
    gle::attrib(target);
    gle::end();

    gle::begin(GL_TRIANGLE_FAN);
    gle::attrib(target);
    loopi(5) gle::attrib(vec(spoke).rotate(2*M_PI*i/4.0f, dir).add(arrowbase));
    gle::end();
}

void renderentcone(const extentity &e, const vec &dir, float radius, float angle)
{
    if(radius <= 0 || !showentdir) return;
    vec spot = vec(dir).mul(radius*cosf(angle*RAD)).add(e.o), spoke;
    spoke.orthogonal(dir);
    spoke.normalize();
    spoke.mul(radius*sinf(angle*RAD));

    gle::defvertex();

    gle::begin(GL_LINES);
    loopi(8)
    {
        gle::attrib(e.o);
        gle::attrib(vec(spoke).rotate(2*M_PI*i/8.0f, dir).add(spot));
    }
    gle::end();

    gle::begin(GL_LINE_LOOP);
    loopi(8) gle::attrib(vec(spoke).rotate(2*M_PI*i/8.0f, dir).add(spot));
    gle::end();
}

void renderentradius(extentity &e, bool color)
{
    switch(e.type)
    {
        case ET_LIGHT:
            if(e.attr[0] < 0) break;
            if(color) gle::colorf(e.attr[1]/255.0f, e.attr[2]/255.0f, e.attr[3]/255.0f);
            renderentsphere(e, e.attr[0]);
            break;

        case ET_SPOTLIGHT:
            if(e.attached)
            {
                if(color) gle::colorf(e.attached->attr[1]/255.0f, e.attached->attr[2]/255.0f, e.attached->attr[3]/255.0f);
                float radius = e.attached->attr[0];
                if(radius <= 0) break;
                vec dir = vec(e.o).sub(e.attached->o).normalize();
                float angle = clamp(int(e.attr[0]), 1, 89);
                renderentattachment(e);
                renderentcone(*e.attached, dir, radius, angle);
            }
            break;

        case ET_SOUND:
            if(color) gle::colorf(0, 1, 1);
            renderentsphere(e, e.attr[1]);
            renderentsphere(e, min(e.attr[1], e.attr[2]));
            break;

        case ET_ENVMAP:
        {
            extern int envmapradius;
            if(color) gle::colorf(0, 1, 1);
            renderentsphere(e, e.attr[0] ? max(0, min(10000, int(e.attr[0]))) : envmapradius);
            break;
        }

        case ET_MAPMODEL:
        {
            if(color) gle::colorf(0, 1, 1);
            entities::entradius(e, color);
            vec dir;
            vecfromyawpitch(e.attr[1], e.attr[2], 1, 0, dir);
            renderentarrow(e, dir, 4);
            break;
        }
        case ET_PLAYERSTART:
        {
            if(color) gle::colorf(0, 1, 1);
            vec dir;
            vecfromyawpitch(e.attr[0], 0, 1, 0, dir);
            renderentarrow(e, dir, 4);
            break;
        }

        default:
            if(e.type>=ET_GAMESPECIFIC)
            {
                if(color) gle::colorf(0, 1, 1);
                entities::entradius(e, color);
            }
            break;
    }
}

void renderhelpers()
{
    if(showentradius < 2 && showentdir < 2) return;
    loopv(entities::getents())
    {
        extentity &e = *entities::getents()[i];

        if((showentradius == 2 && entities::radiusent(e))|| (showentdir == 2 && entities::dirent(e)))
        {
            glDepthFunc(GL_GREATER);
            gle::colorf(0.25f, 0.25f, 0.25f);
            renderentradius(e, false);
            glDepthFunc(GL_LESS);
            renderentradius(e, true);
        }
    }
}

void renderentselection(const vec &o, const vec &ray, bool entmoving)
{
    if(!noentedit())
    {
        vec eo, es;

        gle::colorub(0, 40, 0);
        loopv(entgroup) entfocus(entgroup[i],
            entselectionbox(e, eo, es);
            boxs3D(eo, es, 1);
        );

        if(enthover >= 0)
        {
            entfocus(enthover, entselectionbox(e, eo, es)); // also ensures enthover is back in focus
            boxs3D(eo, es, 1);

            if(entmoving && entmovingshadow==1)
            {
                vec a, b;
                gle::colorub(20, 20, 20);
                (a = eo).x = eo.x - fmod(eo.x, worldsize); (b = es).x = a.x + worldsize; boxs3D(a, b, 1);
                (a = eo).y = eo.y - fmod(eo.y, worldsize); (b = es).y = a.x + worldsize; boxs3D(a, b, 1);
                (a = eo).z = eo.z - fmod(eo.z, worldsize); (b = es).z = a.x + worldsize; boxs3D(a, b, 1);
            }
            gle::colorub(150,0,0);
            boxs(entorient, eo, es);
        }
        if(showenthelpers>=1 && (entgroup.length() || enthover >= 0))
        {
            glDepthFunc(GL_GREATER);
            gle::colorf(0.25f, 0.25f, 0.25f);
            loopv(entgroup)
            {
                extentity &e = *entities::getents()[entgroup[i]];
                if((showentdir == 2 && entities::dirent(e)) || (showentradius == 2 && entities::radiusent(e))) continue;
                renderentradius(e, false);
            }
            if(enthover>=0) entfocus(enthover, renderentradius(e, false));
            glDepthFunc(GL_LESS);
            loopv(entgroup)
            {
                extentity &e = *entities::getents()[entgroup[i]];
                if((showentdir == 2 && entities::dirent(e)) || (showentradius == 2 && entities::radiusent(e))) continue;
                renderentradius(e, true);
            }
            if(enthover>=0) entfocus(enthover, renderentradius(e, true));
            gle::disable();
        }
    }
}

bool enttoggle(int id)
{
    undonext = true;
    int i = entgroup.find(id);
    if(i < 0)
        entadd(id);
    else
        entgroup.remove(i);
    return i < 0;
}

bool hoveringonent(int ent, int orient)
{
    if(noentedit()) return false;
    entorient = orient;
    if((efocus = enthover = ent) >= 0)
        return true;
    efocus   = entgroup.empty() ? -1 : entgroup.last();
    enthover = -1;
    return false;
}

VAR(entitysurf, 0, 0, 1);

ICOMMAND(entadd, "", (),
{
    if(enthover >= 0 && !noentedit())
    {
        if(entgroup.find(enthover) < 0) entadd(enthover);
        if(entmoving > 1) entmoving = 1;
    }
});

ICOMMAND(enttoggle, "", (),
{
    if(enthover < 0 || noentedit() || !enttoggle(enthover)) { entmoving = 0; intret(0); }
    else { if(entmoving > 1) entmoving = 1; intret(1); }
});

ICOMMAND(entmoving, "b", (int *n),
{
    if(*n >= 0)
    {
        if(!*n || enthover < 0 || noentedit()) entmoving = 0;
        else
        {
            if(entgroup.find(enthover) < 0) { entadd(enthover); entmoving = 1; }
            else if(!entmoving) entmoving = 1;
        }
    }
    intret(entmoving);
});

void entpush(int *dir)
{
    if(noentedit()) return;
    int d = dimension(entorient);
    int s = dimcoord(entorient) ? -*dir : *dir;
    if(entmoving)
    {
        groupeditpure(e.o[d] += float(s*sel.grid)); // editdrag supplies the undo
    }
    else
        groupedit(e.o[d] += float(s*sel.grid));
    if(entitysurf==1)
    {
        player->o[d] += float(s*sel.grid);
        player->resetinterp();
    }
}

VAR(entautoviewdist, 0, 25, 100);
void entautoview(int *dir)
{
    if(!haveselent()) return;
    static int s = 0;
    vec v(player->o);
    v.sub(worldpos);
    v.normalize();
    v.mul(entautoviewdist);
    int t = s + *dir;
    s = abs(t) % entgroup.length();
    if(t<0 && s>0) s = entgroup.length() - s;
    entfocus(entgroup[s],
        v.add(e.o);
        player->o = v;
        player->resetinterp();
    );
}

COMMAND(entautoview, "i");
COMMAND(entflip, "");
COMMAND(entrotate, "i");
COMMAND(entpush, "i");

void delent()
{
    if(noentedit()) return;
    groupedit(e.type = ET_EMPTY;);
    entcancel();
}

int findtype(const char *what)
{
    for(int i = 0; *entities::entname(i); i++) if(!strcmp(what, entities::entname(i))) return i;
    conoutf(CON_ERROR, "unknown entity type \"%s\"", what);
    return ET_EMPTY;
}

VAR(entdrop, 0, 2, 3);

bool dropentity(entity &e, int drop = -1)
{
    vec radius(4.0f, 4.0f, 4.0f);
    if(drop<0) drop = entdrop;
    if(e.type == ET_MAPMODEL)
    {
        model *m = loadmapmodel(e.attr[0]);
        if(m)
        {
            vec center;
            m->boundbox(center, radius);
            if(e.attr[4] > 0)
            {
                center.mul(e.attr[4] / 100.0f);
                radius.mul(e.attr[4] / 100.0f);
            }
            rotatebb(center, radius, e.attr[1], e.attr[2], e.attr[3]);
            radius.x += fabs(center.x);
            radius.y += fabs(center.y);
        }
        radius.z = 0.0f;
    }
    switch(drop)
    {
    case 1:
        if(e.type != ET_LIGHT && e.type != ET_SPOTLIGHT)
            dropenttofloor(&e);
        break;
    case 2:
    case 3:
        int cx = 0, cy = 0;
        if(sel.cxs == 1 && sel.cys == 1)
        {
            cx = (sel.cx ? 1 : -1) * sel.grid / 2;
            cy = (sel.cy ? 1 : -1) * sel.grid / 2;
        }
        e.o = vec(sel.o);
        int d = dimension(sel.orient), dc = dimcoord(sel.orient);
        e.o[R[d]] += sel.grid / 2 + cx;
        e.o[C[d]] += sel.grid / 2 + cy;
        if(!dc)
            e.o[D[d]] -= radius[D[d]];
        else
            e.o[D[d]] += sel.grid + radius[D[d]];

        if(drop == 3)
            dropenttofloor(&e);
        break;
    }
    return true;
}

void dropent()
{
    if(noentedit()) return;
    groupedit(dropentity(e));
}

void attachent()
{
    if(noentedit()) return;
    groupedit(attachentity(e));
}

COMMAND(attachent, "");

static int keepents = 0;

extentity *newentity(bool local, const vec &o, int type, int *attrs, int &idx)
{
    vector<extentity *> &ents = entities::getents();
    if(local)
    {
        idx = -1;
        for(int i = keepents; i < ents.length(); i++) if(ents[i]->type == ET_EMPTY) { idx = i; break; }
        if(idx < 0 && ents.length() >= MAXENTS) { conoutf("too many entities"); return NULL; }
    }
    else while(ents.length() < idx) ents.add(entities::newentity())->type = ET_EMPTY;
    extentity &e = *entities::newentity();
    e.o = o;
    loopi(getattrnum(type))
        e.attr.add(attrs[i]);

    e.type = type;
    if(local)
    {
        switch(type)
        {
            case ET_PARTICLES:
                switch(e.attr[0])
                {
                    case 0: case 1: case 2: case 6: case 7: case 9: case 10: case 11: case 12: case 13: case 14: case 15:
                        if(!e.attr[3]) e.attr[3] |= 0x0F0F0F;
                        if(e.attr[0] != 6 && e.attr[0] != 7) break;
                    case 5: case 8:
                        if(!e.attr[2]) e.attr[2] |= 0x0F0F0F;
                        break;
                }
                break;
            case ET_PLAYERSTART:
                e.attr.drop();
                e.attr.insert(0, camera1->yaw);
                break;
        }
        entities::fixentity(e);
    }
    if(ents.inrange(idx)) { entities::deleteentity(ents[idx]); ents[idx] = &e; }
    else { idx = ents.length(); ents.add(&e); }
    return &e;
}

const int getattrnum(int type)
{
    if(type >= ET_GAMESPECIFIC)
        return entities::numattrs(type);

    static const int num[] =
    {
        0, //empty
        6, //light
        5, //mapmodel
        2, //playerstart
        3, //envmap
        9, //particles
        3, //sound
        1  //spotlight
    };
    return type >= 0 && size_t(type) < sizeof(num)/sizeof(num[0]) ? num[type] : 0;
}

ICOMMAND(getattrnum, "s", (char *s),
    int type = findtype(s);
    intret(getattrnum(type));
)

void newentity(int type, int *attrs)
{
    int idx;
    extentity *t = newentity(true, player->o, type, attrs, idx);
    if(!t) return;

    dropentity(*t);
    t->type = ET_EMPTY;
    enttoggle(idx);
    makeundoent();
    entedit(idx, e.type = type);
}

void newent(tagval *args, int num)
{
    if(noentedit() || num == 0) return;
    int type = findtype(args[0].getstr());
    const int numattrs = getattrnum(type);
    int *attrs = new int[numattrs];

    loopi(numattrs)
        attrs[i] = (num - 1 > i) ? args[i + 1].getint() : 0;

    if(type != ET_EMPTY)
        newentity(type, attrs);

    delete[] attrs;
}

int entcopygrid;
struct entitycopy
{
    entity e;
    vector<char> extra;

    entitycopy(entity &e) : e(e) {}
};

vector<entitycopy> entcopybuf;

void entcopy()
{
    if(noentedit()) return;
    entcopygrid = sel.grid;
    entcopybuf.shrink(0);
    loopv(entgroup)
        entfocus(entgroup[i],
            entitycopy &ec = entcopybuf.add(entitycopy(e));
            ec.e.o.sub(vec(sel.o));
            int sz = entities::extraentinfosize();
            if(sz)
            {
                ec.extra.reserve(sz);
                entities::saveextrainfo(e, ec.extra.getbuf());
            }
        );
}

void entpaste()
{
    if(noentedit() || !entcopybuf.length()) return;

    entcancel();
    float m = float(sel.grid)/float(entcopygrid);
    loopv(entcopybuf)
    {
        entitycopy &ec = entcopybuf[i];
        entity &c = ec.e;
        vec o = vec(c.o).mul(m).add(vec(sel.o));
        int idx;
        extentity *e = newentity(true, o, ET_EMPTY, c.attr.getbuf(), idx);
        if(!e) continue;
        if(ec.extra.capacity())
            entities::loadextrainfo(*e, ec.extra.getbuf());
        entadd(idx);
        keepents = max(keepents, idx + 1);
    }
    keepents = 0;
    int j = 0;
    groupeditundo(e.attr = entcopybuf[j].e.attr; e.type = entcopybuf[j++].e.type;);
}

COMMAND(newent, "V");
COMMAND(delent, "");
COMMAND(dropent, "");
COMMAND(entcopy, "");
COMMAND(entpaste, "");

void entset(tagval *args, int num)
{
    if(noentedit()) return;
    int type = num ? findtype(args[0].getstr()) : 0;
    int numattrs = getattrnum(type);
    int *attrs = new int[numattrs];

    loopi(numattrs)
        attrs[i] = (num - 1 >= i) ? args[i + 1].getint() : 0;

    if(type != ET_EMPTY)
        groupedit(e.type=type;
                  e.attr.setsize(0);
                  e.attr.put(attrs, numattrs));
    delete[] attrs;
}

void printent(extentity &e, char *buf, int len)
{
    switch(e.type)
    {
        case ET_PARTICLES:
            if(printparticles(e, buf, len)) return;
            break;
        default:
            if(e.type >= ET_GAMESPECIFIC && entities::printent(e, buf, len)) return;
            break;
    }
    loopv(e.attr)
    {
        static string tmp;
        formatstring(tmp, "%s%d", i ? " " : "", e.attr[i]);
        concatstring(buf, tmp, len);
    }
}

void nearestent()
{
    if(noentedit()) return;
    int closest = -1;
    float closedist = 1e16f;
    vector<extentity *> &ents = entities::getents();
    loopv(ents)
    {
        extentity &e = *ents[i];
        if(e.type == ET_EMPTY) continue;
        float dist = e.o.dist(player->o);
        if(dist < closedist)
        {
            closest = i;
            closedist = dist;
        }
    }
    if(closest >= 0) entadd(closest);
}

ICOMMAND(enthavesel,"",  (), addimplicit(intret(entgroup.length())));
ICOMMAND(entselect, "e", (uint *body), if(!noentedit()) addgroup(e.type != ET_EMPTY && entgroup.find(n)<0 && executebool(body)));
ICOMMAND(entloop,   "e", (uint *body), if(!noentedit()) addimplicit(groupeditloop(((void)e, execute(body)))));
ICOMMAND(insel,     "",  (), entfocus(efocus, intret(pointinsel(sel, e.o))));
ICOMMAND(entget,    "",  (),
    entfocus(efocus,
        defformatstring(s, "%s ", entities::entname(e.type));
        printent(e, s, MAXSTRLEN);
        result(s)
    )
);
ICOMMAND(entindex,  "",  (), intret(efocus));
ICOMMAND(entgetinfo,    "",  (), entfocus(efocus,result(entities::entnameinfo(e))));
COMMAND(entset, "V");
COMMAND(nearestent, "");

void enttype(char *type, int *numargs)
{
    if(*numargs >= 1)
    {
        int typeidx = findtype(type);
        if(typeidx != ET_EMPTY) groupedit(
            e.type = typeidx;
            int numattrs = getattrnum(typeidx);
            while(e.attr.length() < numattrs) e.attr.add(0);
            while(e.attr.length() > numattrs) e.attr.drop();
        );
    }
    else entfocus(efocus,
    {
        result(entities::entname(e.type));
    })
}

void entattr(int *attr, int *val, int *numargs)
{
    if(*numargs >= 2)
    {
        groupedit(
            if(e.attr.inrange(*attr))
                e.attr[*attr] = *val;
        );
    }
    else entfocus(efocus,
    {
        if(e.attr.inrange(*attr))
            intret(e.attr[*attr]);
    });
}

COMMAND(enttype, "sN");
COMMAND(entattr, "iiN");

int findentity(int type, int index, int attr1, int attr2)
{
    const vector<extentity *> &ents = entities::getents();
    for(int i = index; i<ents.length(); i++)
    {
        extentity &e = *ents[i];
        if(e.type == type &&
            (attr1 < 0 || e.attr.length() <= 0 || e.attr[0] == attr1 ) &&
            (attr1 < 0 || e.attr.length() <= 1 || e.attr[1] == attr2 )
        )
        return i;
    }
    loopj(min(index, ents.length()))
    {
        extentity &e = *ents[j];
        if(e.type == type &&
            (attr1 < 0 || e.attr.length() <= 0 || e.attr[0] == attr1 ) &&
            (attr1 < 0 || e.attr.length() <= 1 || e.attr[1] == attr2 )
        )
            return j;
    }
    return -1;
}

int spawncycle = -1;

void findplayerspawn(dynent *d, int forceent, int tag)   // place at random spawn. also used by monsters!
{
    int pick = forceent;
    if(pick<0)
    {
        int r = rnd(10)+1;
        loopi(r) spawncycle = findentity(ET_PLAYERSTART, spawncycle+1, -1, tag);
        pick = spawncycle;
    }
    if(pick!=-1)
    {
        const vector<extentity *> &ents = entities::getents();
        d->pitch = 0;
        d->roll = 0;
        for(int attempt = pick;;)
        {
            d->o = ents[attempt]->o;
            d->yaw = ents[attempt]->attr[0];
            if(entinmap(d, true)) break;
            attempt = findentity(ET_PLAYERSTART, attempt+1, -1, tag);
            if(attempt<0 || attempt==pick)
            {
                d->o = ents[attempt]->o;
                d->yaw = ents[attempt]->attr[0];
                entinmap(d);
                break;
            }
        }
    }
    else
    {
        d->o.x = d->o.y = d->o.z = 0.5f*worldsize;
        d->o.z += 1;
        entinmap(d);
    }
    if(d == player) ovr::reset();
}

void splitocta(cube *c, int size)
{
    if(size <= 0x1000) return;
    loopi(8)
    {
        if(!c[i].children) c[i].children = newcubes(isempty(c[i]) ? F_EMPTY : F_SOLID);
        splitocta(c[i].children, size>>1);
    }
}

void resetmap()
{
    clearoverrides();
    clearmapsounds();
    resetblendmap();
    clearlights();
    clearpvs();
    clearslots();
    clearparticles();
    cleardecals();
    clearsleep();
    cancelsel();
    pruneundos();
    clearmapcrc();

    entities::clearents();
    outsideents.setsize(0);
}

void startmap(const char *name)
{
    game::startmap(name);
    ovr::reset();
}

bool emptymap(int scale, bool force, const char *mname, bool usecfg)    // main empty world creation routine
{
    if(!force && !editmode)
    {
        conoutf(CON_ERROR, "newmap only allowed in edit mode");
        return false;
    }

    resetmap();

    setvar("mapscale", scale<10 ? 10 : (scale>16 ? 16 : scale), true, false);
    setvar("mapsize", 1<<worldscale, true, false);

    texmru.shrink(0);
    freeocta(worldroot);
    worldroot = newcubes(F_EMPTY);
    loopi(4) solidfaces(worldroot[i]);

    if(worldsize > 0x1000) splitocta(worldroot, worldsize>>1);

    UI::clearmainmenu();

    if(usecfg)
    {
        identflags |= IDF_OVERRIDDEN;
        execfile("config/default_map_settings.cfg", false);
        identflags &= ~IDF_OVERRIDDEN;
    }

    initlights();
    allchanged(true);

    startmap(mname);
    return true;
}

bool enlargemap(bool force)
{
    if(!force && !editmode)
    {
        conoutf(CON_ERROR, "mapenlarge only allowed in edit mode");
        return false;
    }
    if(worldsize >= 1<<16) return false;

    while(outsideents.length()) removeentity(outsideents.pop());

    worldscale++;
    worldsize *= 2;
    cube *c = newcubes(F_EMPTY);
    c[0].children = worldroot;
    loopi(3) solidfaces(c[i+1]);
    worldroot = c;

    if(worldsize > 0x1000) splitocta(worldroot, worldsize>>1);

    enlargeblendmap();

    allchanged();

    return true;
}

static bool isallempty(cube &c)
{
    if(!c.children) return isempty(c);
    loopi(8) if(!isallempty(c.children[i])) return false;
    return true;
}

void shrinkmap()
{
    extern int nompedit;
    if(noedit(true) || (nompedit && multiplayer())) return;
    if(worldsize <= 1<<10) return;

    int octant = -1;
    loopi(8) if(!isallempty(worldroot[i]))
    {
        if(octant >= 0) return;
        octant = i;
    }
    if(octant < 0) return;

    while(outsideents.length()) removeentity(outsideents.pop());

    if(!worldroot[octant].children) subdividecube(worldroot[octant], false, false);
    cube *root = worldroot[octant].children;
    worldroot[octant].children = NULL;
    freeocta(worldroot);
    worldroot = root;
    worldscale--;
    worldsize /= 2;

    ivec offset(octant, ivec(0, 0, 0), worldsize);
    vector<extentity *> &ents = entities::getents();
    loopv(ents) ents[i]->o.sub(vec(offset));

    shrinkblendmap(octant);

    allchanged();

    conoutf("shrunk map to size %d", worldscale);
}

void newmap(int *i) { bool force = !isconnected(); if(force) game::forceedit(""); if(emptymap(*i, force, NULL)) game::newmap(max(*i, 0)); }
void mapenlarge() { if(enlargemap(false)) game::newmap(-1); }
COMMAND(newmap, "i");
COMMAND(mapenlarge, "");
COMMAND(shrinkmap, "");

void mapname()
{
    result(game::getclientmap());
}

COMMAND(mapname, "");

void mpeditent(int i, const vec &o, int type, int *attrs, bool local)
{
    if(i < 0 || i >= MAXENTS) return;
    vector<extentity *> &ents = entities::getents();
    if(ents.length()<=i)
    {
        extentity *e = newentity(local, o, type, attrs, i);
        if(!e) return;
        addentity(i);
        attachentity(*e);
    }
    else
    {
        extentity &e = *ents[i];
        removeentity(i);
        int oldtype = e.type;
        if(oldtype!=type) detachentity(e);
        e.type = type;
        e.o = o;
        e.attr.setsize(0);
        e.attr.put(attrs, getattrnum(type));
        addentity(i);
        if(oldtype!=type) attachentity(e);
    }
    entities::editent(i, local);
    clearshadowcache();
}

int getworldsize() { return worldsize; }
int getmapversion() { return mapversion; }







