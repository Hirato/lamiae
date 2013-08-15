#include "engine.h"

extern int outline;

void boxs(int orient, vec o, const vec &s)
{
    int   d = dimension(orient),
          dc= dimcoord(orient);

    float f = !outline ? 0 : (dc>0 ? 0.2f : -0.2f);
    o[D[d]] += float(dc) * s[D[d]] + f,

    gle::defvertex();
    gle::begin(GL_LINE_LOOP);

    gle::attrib(o); o[R[d]] += s[R[d]];
    gle::attrib(o); o[C[d]] += s[C[d]];
    gle::attrib(o); o[R[d]] -= s[R[d]];
    gle::attrib(o);

    xtraverts += gle::end();
}

void boxs3D(const vec &o, vec s, int g)
{
    s.mul(g);
    loopi(6)
        boxs(i, o, s);
}

void boxsgrid(int orient, vec o, vec s, int g)
{
    int   d = dimension(orient),
          dc= dimcoord(orient);
    float ox = o[R[d]],
          oy = o[C[d]],
          xs = s[R[d]],
          ys = s[C[d]],
          f = !outline ? 0 : (dc>0 ? 0.2f : -0.2f);

    o[D[d]] += dc * s[D[d]]*g + f;

    gle::defvertex();
    gle::begin(GL_LINES);
    loop(x, xs) {
        o[R[d]] += g;
        gle::attrib(o);
        o[C[d]] += ys*g;
        gle::attrib(o);
        o[C[d]] = oy;
    }
    loop(y, ys) {
        o[C[d]] += g;
        o[R[d]] = ox;
        gle::attrib(o);
        o[R[d]] += xs*g;
        gle::attrib(o);
    }
    xtraverts += gle::end();
}

selinfo sel, lastsel, savedsel;

int orient = 0;
int gridsize = 8;
ivec cor, lastcor;
ivec cur, lastcur;

extern int entediting;
bool editmode = false;
bool havesel = false;
bool hmapsel = false;
int horient  = 0;

extern int entmoving;

VARF(dragging, 0, 0, 1,
    if(!dragging || cor[0]<0) return;
    lastcur = cur;
    lastcor = cor;
    sel.grid = gridsize;
    sel.orient = orient;
);

int moving = 0;
ICOMMAND(moving, "b", (int *n),
{
    if(*n >= 0)
    {
        if(!*n || (moving<=1 && !pointinsel(sel, vec(cur).add(1)))) moving = 0;
        else if(!moving) moving = 1;
    }
    intret(moving);
});

VARF(gridpower, 0, 3, 12,
{
    if(dragging) return;
    gridsize = 1<<gridpower;
    if(gridsize>=worldsize) gridsize = worldsize/2;
    cancelsel();
});

VAR(passthroughsel, 0, 0, 1);
VAR(editing, 1, 0, 0);
VAR(selectcorners, 0, 0, 1);
VARF(hmapedit, 0, 0, 1, horient = sel.orient);

void forcenextundo() { lastsel.orient = -1; }

namespace hmap { void cancel(); }

void cubecancel()
{
    havesel = false;
    moving = dragging = hmapedit = passthroughsel = 0;
    forcenextundo();
    hmap::cancel();
}

void cancelsel()
{
    cubecancel();
    entcancel();
}

void toggleedit(bool force)
{
    if(!force)
    {
        if(!isconnected()) return;
        if(player->state!=CS_ALIVE && player->state!=CS_DEAD && player->state!=CS_EDITING) return; // do not allow dead players to edit to avoid state confusion
        if(!game::allowedittoggle()) return;         // not in most multiplayer modes
    }
    if(!(editmode = !editmode))
    {
        player->state = player->editstate;
        player->o.z -= player->eyeheight;       // entinmap wants feet pos
        entinmap(player);                       // find spawn closest to current floating pos
    }
    else
    {
        game::resetgamestate();
        player->editstate = player->state;
        player->state = CS_EDITING;
    }
    cancelsel();
    stoppaintblendmap();
    keyrepeat(editmode, KR_EDITMODE);
    editing = entediting = editmode;
    if(!force) game::edittoggled(editmode);
    execident("edittoggled");
}

bool noedit(bool view, bool msg)
{
    if(!editmode) { if(msg) conoutf(CON_ERROR, "operation only allowed in edit mode"); return true; }
    if(view || haveselent()) return false;
    vec o(sel.o), s(sel.s);
    s.mul(sel.grid / 2.0f);
    o.add(s);
    float r = max(s.x, s.y, s.z);
    bool viewable = (isvisiblesphere(r, o) != VFC_NOT_VISIBLE);
    if(!viewable && msg) conoutf(CON_ERROR, "selection not in view");
    return !viewable;
}

void reorient()
{
    sel.cx = 0;
    sel.cy = 0;
    sel.cxs = sel.s[R[dimension(orient)]]*2;
    sel.cys = sel.s[C[dimension(orient)]]*2;
    sel.orient = orient;
}

void selextend()
{
    if(noedit(true)) return;
    loopi(3)
    {
        if(cur[i]<sel.o[i])
        {
            sel.s[i] += (sel.o[i]-cur[i])/sel.grid;
            sel.o[i] = cur[i];
        }
        else if(cur[i]>=sel.o[i]+sel.s[i]*sel.grid)
        {
            sel.s[i] = (cur[i]-sel.o[i])/sel.grid+1;
        }
    }
}

ICOMMAND(edittoggle, "", (), toggleedit(false));
COMMAND(entcancel, "");
COMMAND(cubecancel, "");
COMMAND(cancelsel, "");
COMMAND(reorient, "");
COMMAND(selextend, "");

ICOMMAND(selmoved, "", (), { if(noedit(true)) return; intret(sel.o != savedsel.o ? 1 : 0); });
ICOMMAND(selsave, "", (), { if(noedit(true)) return; savedsel = sel; });
ICOMMAND(selrestore, "", (), { if(noedit(true)) return; sel = savedsel; });
ICOMMAND(selswap, "", (), { if(noedit(true)) return; swap(sel, savedsel); });

///////// selection support /////////////

cube &blockcube(int x, int y, int z, const block3 &b, int rgrid) // looks up a world cube, based on coordinates mapped by the block
{
    int dim = dimension(b.orient), dc = dimcoord(b.orient);
    ivec s(dim, x*b.grid, y*b.grid, dc*(b.s[dim]-1)*b.grid);
    s.add(b.o);
    if(dc) s[dim] -= z*b.grid; else s[dim] += z*b.grid;
    return lookupcube(s, rgrid);
}

#define loopxy(b)        loop(y,(b).s[C[dimension((b).orient)]]) loop(x,(b).s[R[dimension((b).orient)]])
#define loopxyz(b, r, f) { loop(z,(b).s[D[dimension((b).orient)]]) loopxy((b)) { cube &c = blockcube(x,y,z,b,r); f; } }
#define loopselxyz(f)    { if(local) makeundo(); loopxyz(sel, sel.grid, f); changed(sel); }
#define selcube(x, y, z) blockcube(x, y, z, sel, sel.grid)

////////////// cursor ///////////////

int selchildcount = 0, selchildmat = -1;

ICOMMAND(havesel, "", (), intret(havesel ? selchildcount : 0));
ICOMMAND(selchildcount, "", (), { if(selchildcount < 0) result(tempformatstring("1/%d", -selchildcount)); else intret(selchildcount); });
ICOMMAND(selchildmat, "s", (char *prefix), { if(selchildmat > 0) result(getmaterialdesc(selchildmat, prefix)); });

void countselchild(cube *c, const ivec &cor, int size)
{
    ivec ss = ivec(sel.s).mul(sel.grid);
    loopoctaboxsize(cor, size, sel.o, ss)
    {
        ivec o(i, cor, size);
        if(c[i].children) countselchild(c[i].children, o, size/2);
        else
        {
            selchildcount++;
            if(c[i].material != MAT_AIR && selchildmat != MAT_AIR)
            {
                if(selchildmat < 0) selchildmat = c[i].material;
                else if(selchildmat != c[i].material) selchildmat = MAT_AIR;
            }
        }
    }
}

void normalizelookupcube(const ivec &o)
{
    if(lusize>gridsize)
    {
        lu.x += (o.x-lu.x)/gridsize*gridsize;
        lu.y += (o.y-lu.y)/gridsize*gridsize;
        lu.z += (o.z-lu.z)/gridsize*gridsize;
    }
    else if(gridsize>lusize)
    {
        lu.x &= ~(gridsize-1);
        lu.y &= ~(gridsize-1);
        lu.z &= ~(gridsize-1);
    }
    lusize = gridsize;
}

void updateselection()
{
    sel.o.x = min(lastcur.x, cur.x);
    sel.o.y = min(lastcur.y, cur.y);
    sel.o.z = min(lastcur.z, cur.z);
    sel.s.x = abs(lastcur.x-cur.x)/sel.grid+1;
    sel.s.y = abs(lastcur.y-cur.y)/sel.grid+1;
    sel.s.z = abs(lastcur.z-cur.z)/sel.grid+1;
}

bool editmoveplane(const vec &o, const vec &ray, int d, float off, vec &handle, vec &dest, bool first)
{
    plane pl(d, off);
    float dist = 0.0f;
    if(!pl.rayintersect(player->o, ray, dist))
        return false;

    dest = vec(ray).mul(dist).add(player->o);
    if(first) handle = vec(dest).sub(o);
    dest.sub(handle);
    return true;
}

namespace hmap { inline bool isheightmap(int orient, int d, bool empty, cube *c); }
extern void entdrag(const vec &ray);
extern bool hoveringonent(int ent, int orient);
extern void renderentselection(const vec &o, const vec &ray, bool entmoving);
extern float rayent(const vec &o, const vec &ray, float radius, int mode, int size, int &orient, int &ent);

VAR(gridlookup, 0, 0, 1);
VAR(passthroughcube, 0, 1, 1);

VARP(showselgrid, 0, 1, 1);

void rendereditcursor()
{
    int d   = dimension(sel.orient),
        od  = dimension(orient),
        odc = dimcoord(orient);

    bool hidecursor = UI::hascursor() || blendpaintmode, hovering = false;
    hmapsel = false;

    vec dir = vec(worldpos).sub(camera1->o).normalize();

    if(moving)
    {
        static vec dest, handle;
        if(editmoveplane(vec(sel.o), camdir, od, sel.o[D[od]]+odc*sel.grid*sel.s[D[od]], handle, dest, moving==1))
        {
            if(moving==1)
            {
                dest.add(handle);
                handle = vec(ivec(handle).mask(~(sel.grid-1)));
                dest.sub(handle);
                moving = 2;
            }
            ivec o = ivec(dest).mask(~(sel.grid-1));
            sel.o[R[od]] = o[R[od]];
            sel.o[C[od]] = o[C[od]];
        }
    }
    else if(entmoving)
    {
        entdrag(dir);
    }
    else
    {
        ivec w;
        float sdist = 0, wdist = 0, t;
        int entorient = 0, ent = -1;

        wdist = rayent(camera1->o, dir, 1e16f,
                       (editmode && showmat ? RAY_EDITMAT : 0)   // select cubes first
                       | (!dragging && entediting ? RAY_ENTS : 0)
                       | RAY_SKIPFIRST
                       | (passthroughcube==1 ? RAY_PASS : 0), gridsize, entorient, ent);

        if((havesel || dragging) && !passthroughsel && !hmapedit)     // now try selecting the selection
            if(rayboxintersect(vec(sel.o), vec(sel.s).mul(sel.grid), camera1->o, dir, sdist, orient))
            {   // and choose the nearest of the two
                if(sdist < wdist)
                {
                    wdist = sdist;
                    ent   = -1;
                }
            }

        if((hovering = hoveringonent(hidecursor ? -1 : ent, entorient)))
        {
           if(!havesel)
           {
               selchildcount = 0;
               selchildmat = -1;
               sel.s = ivec(0, 0, 0);
           }
        }
        else
        {
            vec w = vec(dir).mul(wdist+0.05f).add(camera1->o);
            if(!insideworld(w))
            {
                loopi(3) wdist = min(wdist, ((dir[i] > 0 ? worldsize : 0) - camera1->o[i]) / dir[i]);
                w = vec(dir).mul(wdist-0.05f).add(camera1->o);
                if(!insideworld(w))
                {
                    wdist = 0;
                    loopi(3) w[i] = clamp(camera1->o[i], 0.0f, float(worldsize));
                }
            }
            cube *c = &lookupcube(w);
            if(gridlookup && !dragging && !moving && !havesel && hmapedit!=1) gridsize = lusize;
            int mag = lusize / gridsize;
            normalizelookupcube(w);
            if(sdist == 0 || sdist > wdist) rayboxintersect(vec(lu), vec(gridsize), camera1->o, dir, t=0, orient); // just getting orient
            cur = lu;
            cor = vec(w).mul(2).div(gridsize);
            od = dimension(orient);
            d = dimension(sel.orient);

            if(hmapedit==1 && dimcoord(horient) == (dir[dimension(horient)]<0))
            {
                hmapsel = hmap::isheightmap(horient, dimension(horient), false, c);
                if(hmapsel)
                    od = dimension(orient = horient);
            }

            if(dragging)
            {
                updateselection();
                sel.cx   = min(cor[R[d]], lastcor[R[d]]);
                sel.cy   = min(cor[C[d]], lastcor[C[d]]);
                sel.cxs  = max(cor[R[d]], lastcor[R[d]]);
                sel.cys  = max(cor[C[d]], lastcor[C[d]]);

                if(!selectcorners)
                {
                    sel.cx &= ~1;
                    sel.cy &= ~1;
                    sel.cxs &= ~1;
                    sel.cys &= ~1;
                    sel.cxs -= sel.cx-2;
                    sel.cys -= sel.cy-2;
                }
                else
                {
                    sel.cxs -= sel.cx-1;
                    sel.cys -= sel.cy-1;
                }

                sel.cx  &= 1;
                sel.cy  &= 1;
                havesel = true;
            }
            else if(!havesel)
            {
                sel.o = lu;
                sel.s.x = sel.s.y = sel.s.z = 1;
                sel.cx = sel.cy = 0;
                sel.cxs = sel.cys = 2;
                sel.grid = gridsize;
                sel.orient = orient;
                d = od;
            }

            sel.corner = (cor[R[d]]-(lu[R[d]]*2)/gridsize)+(cor[C[d]]-(lu[C[d]]*2)/gridsize)*2;
            selchildcount = 0;
            selchildmat = -1;
            countselchild(worldroot, ivec(0, 0, 0), worldsize/2);
            if(mag>=1 && selchildcount==1)
            {
                selchildmat = c->material;
                if(mag>1) selchildcount = -mag;
            }
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // cursors

    ldrnotextureshader->set();

    renderentselection(camera1->o, dir, entmoving!=0);

    enablepolygonoffset(GL_POLYGON_OFFSET_LINE);

    if(!moving && !hovering && !hidecursor)
    {
        if(hmapedit==1)
            gle::colorub(0, hmapsel ? 255 : 40, 0);
        else
            gle::colorub(120,120,120);
        boxs(orient, vec(lu), vec(lusize));
    }

    // selections
    if(havesel || moving)
    {
        d = dimension(sel.orient);
        gle::colorub(50,50,50);   // grid
        boxsgrid(sel.orient, vec(sel.o), vec(sel.s), sel.grid);
        gle::colorub(200,0,0);    // 0 reference
        boxs3D(vec(sel.o).sub(0.5f*min(gridsize*0.25f, 2.0f)), vec(min(gridsize*0.25f, 2.0f)), 1);
        gle::colorub(200,200,200);// 2D selection box
        vec co(sel.o.v), cs(sel.s.v);
        co[R[d]] += 0.5f*(sel.cx*gridsize);
        co[C[d]] += 0.5f*(sel.cy*gridsize);
        cs[R[d]]  = 0.5f*(sel.cxs*gridsize);
        cs[C[d]]  = 0.5f*(sel.cys*gridsize);
        cs[D[d]] *= gridsize;
        boxs(sel.orient, co, cs);
        if(hmapedit==1)         // 3D selection box
            gle::colorub(0,120,0);
        else
            gle::colorub(0,0,120);
        boxs3D(vec(sel.o), vec(sel.s), sel.grid);

        if (showselgrid)
        {
            vec a, b;
            gle::colorub(40, 40, 80);
            //note that vector b is multiplied by g (aka, sel.grid) inside the function, so undo that here
            loopi(3)
            {
                a = vec(sel.o); b = vec(sel.s);
                a[i] = 0; b[i] = worldsize/sel.grid;
                boxs3D(a, b, sel.grid);
            }
        }
    }

    disablepolygonoffset(GL_POLYGON_OFFSET_LINE);

    glDisable(GL_BLEND);
}

void tryedit()
{
    extern int hidehud;
    if(!editmode || hidehud || UI::mainmenu) return;
    if(blendpaintmode) trypaintblendmap();
}

//////////// ready changes to vertex arrays ////////////

static bool haschanged = false;

void readychanges(const ivec &bbmin, const ivec &bbmax, cube *c, const ivec &cor, int size)
{
    loopoctabox(cor, size, bbmin, bbmax)
    {
        ivec o(i, cor, size);
        if(c[i].ext)
        {
            if(c[i].ext->va)             // removes va s so that octarender will recreate
            {
                int hasmerges = c[i].ext->va->hasmerges;
                destroyva(c[i].ext->va);
                c[i].ext->va = NULL;
                if(hasmerges) invalidatemerges(c[i], o, size, true);
            }
            freeoctaentities(c[i]);
            c[i].ext->tjoints = -1;
        }
        if(c[i].children)
        {
            if(size<=1)
            {
                solidfaces(c[i]);
                discardchildren(c[i], true);
                brightencube(c[i]);
            }
            else readychanges(bbmin, bbmax, c[i].children, o, size/2);
        }
        else brightencube(c[i]);
    }
}

void commitchanges(bool force)
{
    if(!force && !haschanged) return;
    haschanged = false;

    extern vector<vtxarray *> valist;
    int oldlen = valist.length();
    resetclipplanes();
    entitiesinoctanodes();
    inbetweenframes = false;
    octarender();
    inbetweenframes = true;
    setupmaterials(oldlen);
    clearshadowcache();
    updatevabbs();
}

void changed(const block3 &sel, bool commit = true)
{
    if(sel.s.iszero()) return;
    readychanges(ivec(sel.o).sub(1), ivec(sel.s).mul(sel.grid).add(sel.o).add(1), worldroot, ivec(0, 0, 0), worldsize/2);
    haschanged = true;

    if(commit) commitchanges();
}

//////////// copy and undo /////////////
static inline void copycube(const cube &src, cube &dst)
{
    dst = src;
    dst.visible = 0;
    dst.merged = 0;
    dst.ext = NULL; // src cube is responsible for va destruction
    if(src.children)
    {
        dst.children = newcubes(F_EMPTY);
        loopi(8) copycube(src.children[i], dst.children[i]);
    }
}

static inline void pastecube(const cube &src, cube &dst)
{
    discardchildren(dst);
    copycube(src, dst);
}

void blockcopy(const block3 &s, int rgrid, block3 *b)
{
    *b = s;
    cube *q = b->c();
    loopxyz(s, rgrid, copycube(c, *q++));
}

block3 *blockcopy(const block3 &s, int rgrid)
{
    int bsize = sizeof(block3)+sizeof(cube)*s.size();
    if(bsize <= 0 || bsize > (100<<20)) return 0;
    block3 *b = (block3 *)new uchar[bsize];
    blockcopy(s, rgrid, b);
    return b;
}

void freeblock(block3 *b, bool alloced = true)
{
    cube *q = b->c();
    loopi(b->size()) discardchildren(*q++);
    if(alloced) delete[] b;
}

void selgridmap(selinfo &sel, int *g)                           // generates a map of the cube sizes at each grid point
{
    loopxyz(sel, -sel.grid, (*g++ = lusize, (void)c));
}

void freeundo(undoblock *u)
{
    if(!u->numents) freeblock(u->block(), false);
    delete[] (uchar *)u;
}

void pasteundo(undoblock *u)
{
    if(u->numents) pasteundoents(u);
    else
    {
        block3 *b = u->block();
        cube *s = b->c();
        int *g = u->gridmap();
        loopxyz(*b, *g++, pastecube(*s++, c));
    }
}

static inline int undosize(undoblock *u)
{
    if(u->numents) return u->numents*sizeof(undoent);
    else
    {
        block3 *b = u->block();
        cube *q = b->c();
        int size = b->size(), total = size*sizeof(int);
        loopj(size) total += familysize(*q++)*sizeof(cube);
        return total;
    }
}

struct undolist
{
    undoblock *first, *last;

    undolist() : first(NULL), last(NULL) {}

    bool empty() { return !first; }

    void add(undoblock *u)
    {
        u->next = NULL;
        u->prev = last;
        if(!first) first = last = u;
        else
        {
            last->next = u;
            last = u;
        }
    }

    undoblock *popfirst()
    {
        undoblock *u = first;
        first = first->next;
        if(first) first->prev = NULL;
        else last = NULL;
        return u;
    }

    undoblock *poplast()
    {
        undoblock *u = last;
        last = last->prev;
        if(last) last->next = NULL;
        else first = NULL;
        return u;
    }
};

undolist undos, redos;
VARP(undomegs, 0, 5, 100);                              // bounded by n megs
int totalundos = 0;

void pruneundos(int maxremain)                          // bound memory
{
    while(totalundos > maxremain && !undos.empty())
    {
        undoblock *u = undos.popfirst();
        totalundos -= u->size;
        freeundo(u);
    }
    //conoutf(CON_DEBUG, "undo: %d of %d(%%%d)", totalundos, undomegs<<20, totalundos*100/(undomegs<<20));
    while(!redos.empty())
    {
        undoblock *u = redos.popfirst();
        totalundos -= u->size;
        freeundo(u);
    }
}

void clearundos() { pruneundos(0); }

COMMAND(clearundos, "");

undoblock *newundocube(selinfo &s)
{
    int ssize = s.size(),
        selgridsize = ssize*sizeof(int),
        blocksize = sizeof(block3)+ssize*sizeof(cube);
    if(blocksize <= 0 || blocksize > (undomegs<<20)) return NULL;
    undoblock *u = (undoblock *)new uchar[sizeof(undoblock) + blocksize + selgridsize];
    u->numents = 0;
    block3 *b = (block3 *)(u + 1);
    blockcopy(s, -s.grid, b);
    int *g = (int *)((uchar *)b + blocksize);
    selgridmap(s, g);
    return u;
}

void addundo(undoblock *u)
{
    u->size = undosize(u);
    u->timestamp = totalmillis;
    undos.add(u);
    totalundos += u->size;
    pruneundos(undomegs<<20);
}

VARP(nompedit, 0, 1, 1);

void makeundoex(selinfo &s)
{
    if(nompedit && multiplayer(false)) return;
    undoblock *u = newundocube(s);
    if(u) addundo(u);
}

void makeundo()                        // stores state of selected cubes before editing
{
    if(lastsel==sel || sel.s.iszero()) return;
    lastsel=sel;
    makeundoex(sel);
}

void swapundo(undolist &a, undolist &b, const char *s)
{
    if(noedit() || (nompedit && multiplayer())) return;
    if(a.empty()) { conoutf(CON_WARN, "nothing more to %s", s); return; }
    int ts = a.last->timestamp;

    selinfo l = sel;
    while(!a.empty() && ts==a.last->timestamp)
    {
        undoblock *u = a.poplast(), *r;
        if(u->numents) r = copyundoents(u);
        else
        {
            block3 *ub = u->block();
            l.o = ub->o;
            l.s = ub->s;
            l.grid = ub->grid;
            l.orient = ub->orient;
            r = newundocube(l);
        }
        if(r)
        {
            r->size = u->size;
            r->timestamp = totalmillis;
            b.add(r);
        }
        pasteundo(u);
        if(!u->numents) changed(l, false);
        freeundo(u);
    }
    commitchanges();
    if(!hmapsel)
    {
        sel = l;
        reorient();
    }
    forcenextundo();
}

void editundo() { swapundo(undos, redos, "undo"); }
void editredo() { swapundo(redos, undos, "redo"); }

// guard against subdivision
#define protectsel(f) { undoblock *_u = newundocube(sel); f; if(_u) { pasteundo(_u); freeundo(_u); } }

vector<editinfo *> editinfos;
editinfo *localedit = NULL;

template<class B>
static void packcube(cube &c, B &buf)
{
    if(c.children)
    {
        buf.put(0xFF);
        loopi(8) packcube(c.children[i], buf);
    }
    else
    {
        cube data = c;
        lilswap(data.texture, 6);
        buf.put(c.material&0xFF);
        buf.put(c.material>>8);
        buf.put(data.edges, sizeof(data.edges));
        buf.put((uchar *)data.texture, sizeof(data.texture));
    }
}

template<class B>
static bool packblock(block3 &b, B &buf)
{
    if(b.size() <= 0 || b.size() > (1<<20)) return false;
    block3 hdr = b;
    lilswap(hdr.o.v, 3);
    lilswap(hdr.s.v, 3);
    lilswap(&hdr.grid, 1);
    lilswap(&hdr.orient, 1);
    buf.put((const uchar *)&hdr, sizeof(hdr));
    cube *c = b.c();
    loopi(b.size()) packcube(c[i], buf);
    return true;
}

template<class B>
static void unpackcube(cube &c, B &buf)
{
    int mat = buf.get();
    if(mat == 0xFF)
    {
        c.children = newcubes(F_EMPTY);
        loopi(8) unpackcube(c.children[i], buf);
    }
    else
    {
        c.material = mat | (buf.get()<<8);
        buf.get(c.edges, sizeof(c.edges));
        buf.get((uchar *)c.texture, sizeof(c.texture));
        lilswap(c.texture, 6);
    }
}

template<class B>
static bool unpackblock(block3 *&b, B &buf)
{
    if(b) { freeblock(b); b = NULL; }
    block3 hdr;
    buf.get((uchar *)&hdr, sizeof(hdr));
    lilswap(hdr.o.v, 3);
    lilswap(hdr.s.v, 3);
    lilswap(&hdr.grid, 1);
    lilswap(&hdr.orient, 1);
    if(hdr.size() > (1<<20)) return false;
    b = (block3 *)new uchar[sizeof(block3)+hdr.size()*sizeof(cube)];
    *b = hdr;
    cube *c = b->c();
    memset(c, 0, b->size()*sizeof(cube));
    loopi(b->size()) unpackcube(c[i], buf);
    return true;
}

static bool compresseditinfo(const uchar *inbuf, int inlen, uchar *&outbuf, int &outlen)
{
    uLongf len = compressBound(inlen);
    if(len > (1<<20)) return false;
    outbuf = new uchar[len];
    if(compress2((Bytef *)outbuf, &len, (const Bytef *)inbuf, inlen, Z_BEST_COMPRESSION) != Z_OK || len > (1<<16))
    {
        delete[] outbuf;
        outbuf = NULL;
        return false;
    }
    outlen = len;
    return true;
}

static bool uncompresseditinfo(const uchar *inbuf, int inlen, uchar *&outbuf, int &outlen)
{
    if(compressBound(outlen) > (1<<20)) return false;
    uLongf len = outlen;
    outbuf = new uchar[len];
    if(uncompress((Bytef *)outbuf, &len, (const Bytef *)inbuf, inlen) != Z_OK)
    {
        delete[] outbuf;
        outbuf = NULL;
        return false;
    }
    outlen = len;
    return true;
}

bool packeditinfo(editinfo *e, int &inlen, uchar *&outbuf, int &outlen)
{
    vector<uchar> buf;
    if(!e || !e->copy || !packblock(*e->copy, buf)) return false;
    inlen = buf.length();
    return compresseditinfo(buf.getbuf(), buf.length(), outbuf, outlen);
}

bool unpackeditinfo(editinfo *&e, const uchar *inbuf, int inlen, int outlen)
{
    if(e && e->copy) { freeblock(e->copy); e->copy = NULL; }
    uchar *outbuf = NULL;
    if(!uncompresseditinfo(inbuf, inlen, outbuf, outlen)) return false;
    ucharbuf buf(outbuf, outlen);
    if(!e) e = editinfos.add(new editinfo);
    if(!unpackblock(e->copy, buf))
    {
        delete[] outbuf;
        return false;
    }
    delete[] outbuf;
    return true;
}

void freeeditinfo(editinfo *&e)
{
    if(!e) return;
    editinfos.removeobj(e);
    if(e->copy) freeblock(e->copy);
    delete e;
    e = NULL;
}

struct prefabheader
{
    char magic[4];
    int version;
};

struct prefab : editinfo
{
    char *name;

    prefab() : name(NULL) {}
    ~prefab() { DELETEA(name); if(copy) freeblock(copy); }
};

static hashnameset<prefab> prefabs;

void delprefab(char *name)
{
    if(prefabs.remove(name))
        conoutf("deleted prefab %s", name);
}
COMMAND(delprefab, "s");

void saveprefab(char *name)
{
    if(!name[0] || noedit(true) || (nompedit && multiplayer())) return;
    prefab *b = prefabs.access(name);
    if(!b)
    {
        b = &prefabs[name];
        b->name = newstring(name);
    }
    if(b->copy) freeblock(b->copy);
    protectsel(b->copy = blockcopy(block3(sel), sel.grid));
    changed(sel);
    defformatstring(filename, strpbrk(name, "/\\") ? "media/%s.obr" : "media/prefab/%s.obr", name);
    path(filename);
    stream *f = opengzfile(filename, "wb");
    if(!f) { conoutf(CON_ERROR, "could not write prefab to %s", filename); return; }
    prefabheader hdr;
    memcpy(hdr.magic, "OEBR", 4);
    hdr.version = 0;
    lilswap(&hdr.version, 1);
    f->write(&hdr, sizeof(hdr));
    streambuf<uchar> s(f);
    if(!packblock(*b->copy, s)) { delete f; conoutf(CON_ERROR, "could not pack prefab %s", filename); return; }
    delete f;
    conoutf("wrote prefab file %s", filename);
}
COMMAND(saveprefab, "s");

void pasteblock(block3 &b, selinfo &sel, bool local)
{
    sel.s = b.s;
    int o = sel.orient;
    sel.orient = b.orient;
    cube *s = b.c();
    loopselxyz(if(!isempty(*s) || s->children || s->material != MAT_AIR) pastecube(*s, c); s++); // 'transparent'. old opaque by 'delcube; paste'
    sel.orient = o;
}

void pasteprefab(char *name)
{
    if(!name[0] || noedit() || (nompedit && multiplayer())) return;
    prefab *b = prefabs.access(name);
    if(!b)
    {
        defformatstring(filename, strpbrk(name, "/\\") ? "media/%s.obr" : "media/prefab/%s.obr", name);
        path(filename);
        stream *f = opengzfile(filename, "rb");
        if(!f) { conoutf(CON_ERROR, "could not read prefab %s", filename); return; }
        prefabheader hdr;
        if(f->read(&hdr, sizeof(hdr)) != sizeof(prefabheader) || memcmp(hdr.magic, "OEBR", 4)) { delete f; conoutf(CON_ERROR, "prefab %s has malformatted header", filename); return; }
        lilswap(&hdr.version, 1);
        if(hdr.version != 0) { delete f; conoutf(CON_ERROR, "prefab %s uses unsupported version", filename); return; }
        streambuf<uchar> s(f);
        block3 *copy = NULL;
        if(!unpackblock(copy, s)) { delete f; conoutf(CON_ERROR, "could not unpack prefab %s", filename); return; }
        delete f;
        b = &prefabs[name];
        b->name = newstring(name);
        b->copy = copy;
    }
    pasteblock(*b->copy, sel, true);
}
COMMAND(pasteprefab, "s");

void mpcopy(editinfo *&e, selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_COPY);
    if(e==NULL) e = editinfos.add(new editinfo);
    if(e->copy) freeblock(e->copy);
    e->copy = NULL;
    protectsel(e->copy = blockcopy(block3(sel), sel.grid));
    changed(sel);
}

void mppaste(editinfo *&e, selinfo &sel, bool local)
{
    if(e==NULL) return;
    if(local) game::edittrigger(sel, EDIT_PASTE);
    if(e->copy) pasteblock(*e->copy, sel, local);
}

void copy()
{
    if(noedit(true)) return;
    mpcopy(localedit, sel, true);
}

void pastehilite()
{
    if(!localedit) return;
    sel.s = localedit->copy->s;
    reorient();
    havesel = true;
}

void paste()
{
    if(noedit()) return;
    mppaste(localedit, sel, true);
}

COMMAND(copy, "");
COMMAND(pastehilite, "");
COMMAND(paste, "");
COMMANDN(undo, editundo, "");
COMMANDN(redo, editredo, "");

static VSlot *editingvslot = NULL;

void compacteditvslots()
{
    if(editingvslot && editingvslot->layer) compactvslot(editingvslot->layer);
    loopv(editinfos)
    {
        editinfo *e = editinfos[i];
        compactvslots(e->copy->c(), e->copy->size());
    }
    for(undoblock *u = undos.first; u; u = u->next)
        if(!u->numents)
            compactvslots(u->block()->c(), u->block()->size());
    for(undoblock *u = redos.first; u; u = u->next)
        if(!u->numents)
            compactvslots(u->block()->c(), u->block()->size());
}

///////////// height maps ////////////////

namespace hmap
{
    vector<int> textures;

    void cancel() { textures.setsize(0); }

    ICOMMAND(hmapcancel, "", (), cancel());
    ICOMMAND(hmapselect, "", (),
        int t = lookupcube(cur).texture[orient];
        int i = textures.find(t);
        if(i<0)
            textures.add(t);
        else
            textures.remove(i);
    );

    inline bool isheightmap(int o, int d, bool empty, cube *c)
    {
        return havesel ||
            (empty && isempty(*c)) ||
            textures.empty() ||
            textures.find(c->texture[o]) >= 0;
    }

    #define MAXBRUSH    64
    #define MAXBRUSHC   63
    #define MAXBRUSH2   32
    int brush[MAXBRUSH][MAXBRUSH];
    VARN(hbrushx, brushx, 0, MAXBRUSH2, MAXBRUSH);
    VARN(hbrushy, brushy, 0, MAXBRUSH2, MAXBRUSH);
    bool paintbrush = 0;
    int brushmaxx = 0, brushminx = MAXBRUSH;
    int brushmaxy = 0, brushminy = MAXBRUSH;

    void clearhbrush()
    {
        memset(brush, 0, sizeof brush);
        brushmaxx = brushmaxy = 0;
        brushminx = brushminy = MAXBRUSH;
        paintbrush = false;
    }
    COMMAND(clearhbrush, "");

    void hbrushvert(int *x, int *y, int *v)
    {
        *x += MAXBRUSH2 - brushx + 1; // +1 for automatic padding
        *y += MAXBRUSH2 - brushy + 1;
        if(*x<0 || *y<0 || *x>=MAXBRUSH || *y>=MAXBRUSH) return;
        brush[*x][*y] = clamp(*v, 0, 8);
        paintbrush = paintbrush || (brush[*x][*y] > 0);
        brushmaxx = min(MAXBRUSH-1, max(brushmaxx, *x+1));
        brushmaxy = min(MAXBRUSH-1, max(brushmaxy, *y+1));
        brushminx = max(0,          min(brushminx, *x-1));
        brushminy = max(0,          min(brushminy, *y-1));
    }
    COMMAND(hbrushvert, "iii");

    #define PAINTED     1
    #define NOTHMAP     2
    #define MAPPED      16
    uchar  flags[MAXBRUSH][MAXBRUSH];
    cube   *cmap[MAXBRUSHC][MAXBRUSHC][4];
    int    mapz[MAXBRUSHC][MAXBRUSHC];
    int    map [MAXBRUSH][MAXBRUSH];

    selinfo changes;
    bool selecting;
    int d, dc, dr, dcr, biasup, br, hws, fg;
    int gx, gy, gz, mx, my, mz, nx, ny, nz, bmx, bmy, bnx, bny;
    uint fs;
    selinfo hundo;

    cube *getcube(ivec t, int f)
    {
        t[d] += dcr*f*gridsize;
        if(t[d] > nz || t[d] < mz) return NULL;
        cube *c = &lookupcube(t, gridsize);
        if(c->children) forcemip(*c, false);
        discardchildren(*c, true);
        if(!isheightmap(sel.orient, d, true, c)) return NULL;
        if     (t.x < changes.o.x) changes.o.x = t.x;
        else if(t.x > changes.s.x) changes.s.x = t.x;
        if     (t.y < changes.o.y) changes.o.y = t.y;
        else if(t.y > changes.s.y) changes.s.y = t.y;
        if     (t.z < changes.o.z) changes.o.z = t.z;
        else if(t.z > changes.s.z) changes.s.z = t.z;
        return c;
    }

    uint getface(cube *c, int d)
    {
        return  0x0f0f0f0f & ((dc ? c->faces[d] : 0x88888888 - c->faces[d]) >> fs);
    }

    void pushside(cube &c, int d, int x, int y, int z)
    {
        ivec a;
        getcubevector(c, d, x, y, z, a);
        a[R[d]] = 8 - a[R[d]];
        setcubevector(c, d, x, y, z, a);
    }

    void addpoint(int x, int y, int z, int v)
    {
        if(!(flags[x][y] & MAPPED))
          map[x][y] = v + (z*8);
        flags[x][y] |= MAPPED;
    }

    void select(int x, int y, int z)
    {
        if((NOTHMAP & flags[x][y]) || (PAINTED & flags[x][y])) return;
        ivec t(d, x+gx, y+gy, dc ? z : hws-z);
        t.shl(gridpower);

        // selections may damage; must makeundo before
        hundo.o = t;
        hundo.o[D[d]] -= dcr*gridsize*2;
        makeundoex(hundo);

        cube **c = cmap[x][y];
        loopk(4) c[k] = NULL;
        c[1] = getcube(t, 0);
        if(!c[1] || !isempty(*c[1]))
        {   // try up
            c[2] = c[1];
            c[1] = getcube(t, 1);
            if(!c[1] || isempty(*c[1])) { c[0] = c[1]; c[1] = c[2]; c[2] = NULL; }
            else { z++; t[d]+=fg; }
        }
        else // drop down
        {
            z--;
            t[d]-= fg;
            c[0] = c[1];
            c[1] = getcube(t, 0);
        }

        if(!c[1] || isempty(*c[1])) { flags[x][y] |= NOTHMAP; return; }

        flags[x][y] |= PAINTED;
        mapz [x][y]  = z;

        if(!c[0]) c[0] = getcube(t, 1);
        if(!c[2]) c[2] = getcube(t, -1);
        c[3] = getcube(t, -2);
        c[2] = !c[2] || isempty(*c[2]) ? NULL : c[2];
        c[3] = !c[3] || isempty(*c[3]) ? NULL : c[3];

        uint face = getface(c[1], d);
        if(face == 0x08080808 && (!c[0] || !isempty(*c[0]))) { flags[x][y] |= NOTHMAP; return; }
        if(c[1]->faces[R[d]] == F_SOLID)   // was single
            face += 0x08080808;
        else                               // was pair
            face += c[2] ? getface(c[2], d) : 0x08080808;
        face += 0x08080808;                // c[3]
        uchar *f = (uchar*)&face;
        addpoint(x,   y,   z, f[0]);
        addpoint(x+1, y,   z, f[1]);
        addpoint(x,   y+1, z, f[2]);
        addpoint(x+1, y+1, z, f[3]);

        if(selecting) // continue to adjacent cubes
        {
            if(x>bmx) select(x-1, y, z);
            if(x<bnx) select(x+1, y, z);
            if(y>bmy) select(x, y-1, z);
            if(y<bny) select(x, y+1, z);
        }
    }

    void ripple(int x, int y, int z, bool force)
    {
        if(force) select(x, y, z);
        if((NOTHMAP & flags[x][y]) || !(PAINTED & flags[x][y])) return;

        bool changed = false;
        int *o[4], best, par, q = 0;
        loopi(2) loopj(2) o[i+j*2] = &map[x+i][y+j];
        #define pullhmap(I, LT, GT, M, N, A) do { \
            best = I; \
            loopi(4) if(*o[i] LT best) best = *o[q = i] - M; \
            par = (best&(~7)) + N; \
            /* dual layer for extra smoothness */ \
            if(*o[q^3] GT par && !(*o[q^1] LT par || *o[q^2] LT par)) { \
                if(*o[q^3] GT par A 8 || *o[q^1] != par || *o[q^2] != par) { \
                    *o[q^3] = (*o[q^3] GT par A 8 ? par A 8 : *o[q^3]); \
                    *o[q^1] = *o[q^2] = par; \
                    changed = true; \
                } \
            /* single layer */ \
            } else { \
                loopj(4) if(*o[j] GT par) { \
                    *o[j] = par; \
                    changed = true; \
                } \
            } \
        } while(0)

        if(biasup)
            pullhmap(0, >, <, 1, 0, -);
        else
            pullhmap(worldsize*8, <, >, 0, 8, +);

        cube **c  = cmap[x][y];
        int e[2][2];
        int notempty = 0;

        loopk(4) if(c[k]) {
            loopi(2) loopj(2) {
                e[i][j] = min(8, map[x+i][y+j] - (mapz[x][y]+3-k)*8);
                notempty |= e[i][j] > 0;
            }
            if(notempty)
            {
                c[k]->texture[sel.orient] = c[1]->texture[sel.orient];
                solidfaces(*c[k]);
                loopi(2) loopj(2)
                {
                    int f = e[i][j];
                    if(f<0 || (f==0 && e[1-i][j]==0 && e[i][1-j]==0))
                    {
                        f=0;
                        pushside(*c[k], d, i, j, 0);
                        pushside(*c[k], d, i, j, 1);
                    }
                    edgeset(cubeedge(*c[k], d, i, j), dc, dc ? f : 8-f);
                }
            }
            else
                emptyfaces(*c[k]);
        }

        if(!changed) return;
        if(x>mx) ripple(x-1, y, mapz[x][y], true);
        if(x<nx) ripple(x+1, y, mapz[x][y], true);
        if(y>my) ripple(x, y-1, mapz[x][y], true);
        if(y<ny) ripple(x, y+1, mapz[x][y], true);

#define DIAGONAL_RIPPLE(a,b,exp) if(exp) { \
            if(flags[x a][ y] & PAINTED) \
                ripple(x a, y b, mapz[x a][y], true); \
            else if(flags[x][y b] & PAINTED) \
                ripple(x a, y b, mapz[x][y b], true); \
        }

        DIAGONAL_RIPPLE(-1, -1, (x>mx && y>my)); // do diagonals because adjacents
        DIAGONAL_RIPPLE(-1, +1, (x>mx && y<ny)); //    won't unless changed
        DIAGONAL_RIPPLE(+1, +1, (x<nx && y<ny));
        DIAGONAL_RIPPLE(+1, -1, (x<nx && y>my));
    }

#define loopbrush(i) for(int x=bmx; x<=bnx+i; x++) for(int y=bmy; y<=bny+i; y++)

    void paint()
    {
        loopbrush(1)
            map[x][y] -= dr * brush[x][y];
    }

    void smooth()
    {
        int sum, div;
        loopbrush(-2)
        {
            sum = 0;
            div = 9;
            loopi(3) loopj(3)
                if(flags[x+i][y+j] & MAPPED)
                    sum += map[x+i][y+j];
                else div--;
            if(div)
                map[x+1][y+1] = sum / div;
        }
    }

    void rippleandset()
    {
        loopbrush(0)
            ripple(x, y, gz, false);
    }

    void run(int dir, int mode)
    {
        d  = dimension(sel.orient);
        dc = dimcoord(sel.orient);
        dcr= dc ? 1 : -1;
        dr = dir>0 ? 1 : -1;
        br = dir>0 ? 0x08080808 : 0;
     //   biasup = mode == dir<0;
        biasup = dir<0;
        bool paintme = paintbrush;
        int cx = (sel.corner&1 ? 0 : -1);
        int cy = (sel.corner&2 ? 0 : -1);
        hws= (worldsize>>gridpower);
        gx = (cur[R[d]] >> gridpower) + cx - MAXBRUSH2;
        gy = (cur[C[d]] >> gridpower) + cy - MAXBRUSH2;
        gz = (cur[D[d]] >> gridpower);
        fs = dc ? 4 : 0;
        fg = dc ? gridsize : -gridsize;
        mx = max(0, -gx); // ripple range
        my = max(0, -gy);
        nx = min(MAXBRUSH-1, hws-gx) - 1;
        ny = min(MAXBRUSH-1, hws-gy) - 1;
        if(havesel)
        {   // selection range
            bmx = mx = max(mx, (sel.o[R[d]]>>gridpower)-gx);
            bmy = my = max(my, (sel.o[C[d]]>>gridpower)-gy);
            bnx = nx = min(nx, (sel.s[R[d]]+(sel.o[R[d]]>>gridpower))-gx-1);
            bny = ny = min(ny, (sel.s[C[d]]+(sel.o[C[d]]>>gridpower))-gy-1);
        }
        if(havesel && mode<0) // -ve means smooth selection
            paintme = false;
        else
        {   // brush range
            bmx = max(mx, brushminx);
            bmy = max(my, brushminy);
            bnx = min(nx, brushmaxx-1);
            bny = min(ny, brushmaxy-1);
        }
        nz = worldsize-gridsize;
        mz = 0;
        hundo.s = ivec(d,1,1,5);
        hundo.orient = sel.orient;
        hundo.grid = gridsize;
        forcenextundo();

        changes.grid = gridsize;
        changes.s = changes.o = cur;
        memset(map, 0, sizeof map);
        memset(flags, 0, sizeof flags);

        selecting = true;
        select(clamp(MAXBRUSH2-cx, bmx, bnx),
               clamp(MAXBRUSH2-cy, bmy, bny),
               dc ? gz : hws - gz);
        selecting = false;
        if(paintme)
            paint();
        else
            smooth();
        rippleandset();                       // pull up points to cubify, and set
        changes.s.sub(changes.o).shr(gridpower).add(1);
        changed(changes);
    }
}

void edithmap(int dir, int mode) {
    if((nompedit && multiplayer()) || !hmapsel) return;
    hmap::run(dir, mode);
}

///////////// main cube edit ////////////////

int bounded(int n) { return n<0 ? 0 : (n>8 ? 8 : n); }

void pushedge(uchar &edge, int dir, int dc)
{
    int ne = bounded(edgeget(edge, dc)+dir);
    edgeset(edge, dc, ne);
    int oe = edgeget(edge, 1-dc);
    if((dir<0 && dc && oe>ne) || (dir>0 && dc==0 && oe<ne)) edgeset(edge, 1-dc, ne);
}

void linkedpush(cube &c, int d, int x, int y, int dc, int dir)
{
    ivec v, p;
    getcubevector(c, d, x, y, dc, v);

    loopi(2) loopj(2)
    {
        getcubevector(c, d, i, j, dc, p);
        if(v==p)
            pushedge(cubeedge(c, d, i, j), dir, dc);
    }
}

static ushort getmaterial(cube &c)
{
    if(c.children)
    {
        ushort mat = getmaterial(c.children[7]);
        loopi(7) if(mat != getmaterial(c.children[i])) return MAT_AIR;
        return mat;
    }
    return c.material;
}

VAR(invalidcubeguard, 0, 1, 1);
VAR(uselasttex, 0, 0, 1);

void mpeditface(int dir, int mode, selinfo &sel, bool local)
{
    if(mode==1 && (sel.cx || sel.cy || sel.cxs&1 || sel.cys&1)) mode = 0;
    int d = dimension(sel.orient);
    int dc = dimcoord(sel.orient);
    int seldir = dc ? -dir : dir;

    if(local)
        game::edittrigger(sel, EDIT_FACE, dir, mode);

    if(mode==1)
    {
        int h = sel.o[d]+dc*sel.grid;
        if(((dir>0) == dc && h<=0) || ((dir<0) == dc && h>=worldsize)) return;
        if(dir<0) sel.o[d] += sel.grid * seldir;
    }

    if(dc) sel.o[d] += sel.us(d)-sel.grid;
    sel.s[d] = 1;

    loopselxyz(
        if(c.children) solidfaces(c);
        ushort mat = getmaterial(c);
        discardchildren(c, true);
        c.material = mat;
        if(mode==1) // fill command
        {
            if(dir<0)
            {
                solidfaces(c);
                cube &o = blockcube(x, y, 1, sel, -sel.grid);
                if(uselasttex)
                {
                    int tex = DEFAULT_GEOM;
                    extern int curtexindex;

                    if(texmru.inrange(curtexindex))
                        tex = texmru[curtexindex];
                    loopi(6)
                        c.texture[i] = tex;
                }
                else loopi(6)
                    c.texture[i] = o.children ? DEFAULT_GEOM : o.texture[i];
            }
            else
                emptyfaces(c);
        }
        else
        {
            uint bak = c.faces[d];
            uchar *p = (uchar *)&c.faces[d];

            if(mode==2)
                linkedpush(c, d, sel.corner&1, sel.corner>>1, dc, seldir); // corner command
            else
            {
                loop(mx,2) loop(my,2)                                       // pull/push edges command
                {
                    if(x==0 && mx==0 && sel.cx) continue;
                    if(y==0 && my==0 && sel.cy) continue;
                    if(x==sel.s[R[d]]-1 && mx==1 && (sel.cx+sel.cxs)&1) continue;
                    if(y==sel.s[C[d]]-1 && my==1 && (sel.cy+sel.cys)&1) continue;
                    if(p[mx+my*2] != ((uchar *)&bak)[mx+my*2]) continue;

                    linkedpush(c, d, mx, my, dc, seldir);
                }
            }

            optiface(p, c);
            if(invalidcubeguard==1 && !isvalidcube(c))
            {
                uint newbak = c.faces[d];
                uchar *m = (uchar *)&bak;
                uchar *n = (uchar *)&newbak;
                loopk(4) if(n[k] != m[k]) // tries to find partial edit that is valid
                {
                    c.faces[d] = bak;
                    c.edges[d*4+k] = n[k];
                    if(isvalidcube(c))
                        m[k] = n[k];
                }
                c.faces[d] = bak;
            }
        }
    );
    if (mode==1 && dir>0)
        sel.o[d] += sel.grid * seldir;
}

ivec getcubevector2(cube &c, int dir, int x,int y)
{
    int d = dimension(dir);
    int dc = dimcoord(dir);

    ivec u(d,x,y,dc);
    ivec nv;
    for(int i=0;i<3;i++)
    {
        if(u[D[i]])
            nv[i]=8-edgeget(cubeedge(c,i,u[R[i]],u[C[i]]),u[D[i]]);
        else
            nv[i]=edgeget(cubeedge(c,i,u[R[i]],u[C[i]]),u[D[i]]);
    }
    return nv;
}

// like setcubevector, but make the offset (dx,dy,dz) always zero-based relative to the vertex of the cube
void setcubevector2(cube &c, int dir, int x,int y, int dx, int dy, int dz)
{
    int d = dimension(dir);
    int dc = dimcoord(dir);

    ivec u(d,x,y,dc);
    ivec nv(d,dx,dy,dz);
    for(int i=0;i<3;i++)
    {
        if(u[D[i]])
        {
            edgeset(cubeedge(c,i,u[R[i]],u[C[i]]),u[D[i]],8-nv[i]);
        }
        else
        {
            edgeset(cubeedge(c,i,u[R[i]],u[C[i]]),u[D[i]],nv[i]);
        }
    }
}

void editface(int *dir, int *mode)
{
    if(noedit(moving!=0)) return;
    if(hmapedit) edithmap(*dir, *mode);
    else mpeditface(*dir, *mode, sel, true);
}

VAR(selectionsurf, 0, 0, 1);

void pushsel(int *dir)
{
    if(noedit(moving!=0)) return;
    int d = dimension(orient);
    int s = dimcoord(orient) ? -*dir : *dir;
    sel.o[d] += s*sel.grid;
    if(selectionsurf==1)
    {
        player->o[d] += s*sel.grid;
        player->resetinterp();
    }
}

void mpdelcube(selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_DELCUBE);
    loopselxyz(discardchildren(c, true); emptyfaces(c));
}

void delcube()
{
    if(noedit()) return;
    mpdelcube(sel, true);
}

COMMAND(pushsel, "i");
COMMAND(editface, "ii");
COMMAND(delcube, "");

/////////// texture editing //////////////////

int curtexindex = -1, lasttex = 0, lasttexmillis = -1;
int texpaneltimer = 0;
vector<ushort> texmru;

void tofronttex()                                       // maintain most recently used of the texture lists when applying texture
{
    int c = curtexindex;
    if(c>=0)
    {
        texmru.insert(0, texmru.remove(c));
        curtexindex = -1;
    }
}

selinfo repsel;
int reptex = -1;

struct vslotmap
{
    int index;
    VSlot *vslot;

    vslotmap() {}
    vslotmap(int index, VSlot *vslot) : index(index), vslot(vslot) {}
};
static vector<vslotmap> remappedvslots;

VAR(usevdelta, 1, 0, 0);

static VSlot *remapvslot(int index, const VSlot &ds)
{
    loopv(remappedvslots) if(remappedvslots[i].index == index) return remappedvslots[i].vslot;
    VSlot &vs = lookupvslot(index, false);
    if(vs.index < 0 || vs.index == DEFAULT_SKY) return NULL;
    VSlot *edit = NULL;
    if(usevdelta)
    {
        VSlot ms;
        mergevslot(ms, vs, ds);
        edit = ms.changed ? editvslot(vs, ms) : vs.slot->variants;
    }
    else edit = ds.changed ? editvslot(vs, ds) : vs.slot->variants;
    if(!edit) edit = &vs;
    remappedvslots.add(vslotmap(vs.index, edit));
    return edit;
}

static void remapvslots(cube &c, const VSlot &ds, int orient, bool &findrep, VSlot *&findedit)
{
    if(c.children)
    {
        loopi(8) remapvslots(c.children[i], ds, orient, findrep, findedit);
        return;
    }
    static VSlot ms;
    if(orient<0) loopi(6)
    {
        VSlot *edit = remapvslot(c.texture[i], ds);
        if(edit)
        {
            c.texture[i] = edit->index;
            if(!findedit) findedit = edit;
        }
    }
    else
    {
        int i = visibleorient(c, orient);
        VSlot *edit = remapvslot(c.texture[i], ds);
        if(edit)
        {
            if(findrep)
            {
                if(reptex < 0) reptex = c.texture[i];
                else if(reptex != c.texture[i]) findrep = false;
            }
            c.texture[i] = edit->index;
            if(!findedit) findedit = edit;
        }
    }
}

void edittexcube(cube &c, int tex, int orient, bool &findrep)
{
    if(orient<0) loopi(6) c.texture[i] = tex;
    else
    {
        int i = visibleorient(c, orient);
        if(findrep)
        {
            if(reptex < 0) reptex = c.texture[i];
            else if(reptex != c.texture[i]) findrep = false;
        }
        c.texture[i] = tex;
    }
    if(c.children) loopi(8) edittexcube(c.children[i], tex, orient, findrep);
}

VAR(allfaces, 0, 0, 1);

void mpeditvslot(VSlot &ds, int allfaces, selinfo &sel, bool local)
{
    if(local)
    {
        if(!(lastsel==sel)) tofronttex();
        if(allfaces || !(repsel == sel)) reptex = -1;
        repsel = sel;
    }
    bool findrep = local && !allfaces && reptex < 0;
    VSlot *findedit = NULL;
    editingvslot = &ds;
    loopselxyz(remapvslots(c, ds, allfaces ? -1 : sel.orient, findrep, findedit));
    editingvslot = NULL;
    remappedvslots.setsize(0);
    if(local && findedit)
    {
        lasttex = findedit->index;
        lasttexmillis = totalmillis;
        curtexindex = texmru.find(lasttex);
        if(curtexindex < 0)
        {
            curtexindex = texmru.length();
            texmru.add(lasttex);
        }
    }
}

void vdelta(uint *body)
{
    if(noedit() || (nompedit && multiplayer())) return;
    usevdelta++;
    execute(body);
    usevdelta--;
}
COMMAND(vdelta, "e");

void vrotate(int *n)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_ROTATION;
    ds.rotation = usevdelta ? *n : clamp(*n, 0, 5);
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(vrotate, "i");

void voffset(int *x, int *y)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_OFFSET;
    ds.offset = usevdelta ? ivec2(*x, *y) : ivec2(*x, *y).max(0);
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(voffset, "ii");

void vscroll(float *s, float *t)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_SCROLL;
    ds.scroll = vec2(*s/1000.0f, *t/1000.0f);
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(vscroll, "ff");

void vscale(float *scale)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_SCALE;
    ds.scale = *scale <= 0 ? 1 : (usevdelta ? *scale : clamp(*scale, 1/8.0f, 8.0f));
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(vscale, "f");

void vlayer(int *n)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_LAYER;
    ds.layer = vslots.inrange(*n) ? *n : 0;
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(vlayer, "i");

void vdecal(int *n)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_DECAL;
    ds.decal = vslots.inrange(*n) ? *n : 0;
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(vdecal, "i");

void valpha(float *front, float *back)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_ALPHA;
    ds.alphafront = clamp(*front, 0.0f, 1.0f);
    ds.alphaback = clamp(*back, 0.0f, 1.0f);
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(valpha, "ff");

void vcolor(float *r, float *g, float *b)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_COLOR;
    ds.colorscale = vec(clamp(*r, 0.0f, 2.0f), clamp(*g, 0.0f, 2.0f), clamp(*b, 0.0f, 2.0f));
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(vcolor, "fff");

void vrefract(float *k, float *r, float *g, float *b)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_REFRACT;
    ds.refractscale = clamp(*k, 0.0f, 1.0f);
    if(ds.refractscale > 0 && (*r > 0 || *g > 0 || *b > 0))
        ds.refractcolor = vec(clamp(*r, 0.0f, 1.0f), clamp(*g, 0.0f, 1.0f), clamp(*b, 0.0f, 1.0f));
    else
        ds.refractcolor = vec(1, 1, 1);
    mpeditvslot(ds, allfaces, sel, true);

}
COMMAND(vrefract, "ffff");

void vreset()
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(vreset, "");

void vshaderparam(const char *name, float *x, float *y, float *z, float *w)
{
    if(noedit() || (nompedit && multiplayer())) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_SHPARAM;
    if(name[0])
    {
        SlotShaderParam p = { getshaderparamname(name), -1, {*x, *y, *z, *w} };
        ds.params.add(p);
    }
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(vshaderparam, "sfFFf");

void mpedittex(int tex, int allfaces, selinfo &sel, bool local)
{
    if(local)
    {
        game::edittrigger(sel, EDIT_TEX, tex, allfaces);
        if(allfaces || !(repsel == sel)) reptex = -1;
        repsel = sel;
    }
    bool findrep = local && !allfaces && reptex < 0;
    loopselxyz(edittexcube(c, tex, allfaces ? -1 : sel.orient, findrep));
}

void filltexlist()
{
    if(texmru.length()!=vslots.length())
    {
        loopvrev(texmru) if(texmru[i]>=vslots.length())
        {
            if(curtexindex > i) curtexindex--;
            else if(curtexindex == i) curtexindex = -1;
            texmru.remove(i);
        }
        loopv(vslots) if(texmru.find(i)<0) texmru.add(i);
    }
}

void compactmruvslots()
{
    remappedvslots.setsize(0);
    loopvrev(texmru)
    {
        if(vslots.inrange(texmru[i]))
        {
            VSlot &vs = *vslots[texmru[i]];
            if(vs.index >= 0)
            {
                texmru[i] = vs.index;
                continue;
            }
        }
        if(curtexindex > i) curtexindex--;
        else if(curtexindex == i) curtexindex = -1;
        texmru.remove(i);
    }
    if(vslots.inrange(lasttex))
    {
        VSlot &vs = *vslots[lasttex];
        lasttex = vs.index >= 0 ? vs.index : 0;
    }
    else lasttex = 0;
    reptex = vslots.inrange(reptex) ? vslots[reptex]->index : -1;
}

void edittex(int i, bool save = true)
{
    lasttex = i;
    lasttexmillis = totalmillis;
    if(save)
    {
        loopvj(texmru) if(texmru[j]==lasttex) { curtexindex = j; break; }
    }
    mpedittex(i, allfaces, sel, true);
}

void edittex_(int *dir)
{
    if(noedit()) return;
    filltexlist();
    texpaneltimer = 5000;
    if(!(lastsel==sel)) tofronttex();
    curtexindex = clamp(curtexindex<0 ? 0 : curtexindex+*dir, 0, texmru.length()-1);
    edittex(texmru[curtexindex], false);
}

void gettex()
{
    if(noedit(true)) return;
    filltexlist();
    int tex = -1;
    loopxyz(sel, sel.grid, tex = c.texture[sel.orient]);
    loopv(texmru) if(texmru[i]==tex)
    {
        curtexindex = i;
        tofronttex();
        return;
    }
}

void getcurtex()
{
    if(noedit(true)) return;
    filltexlist();
    int index = curtexindex < 0 ? 0 : curtexindex;
    if(!texmru.inrange(index)) return;
    intret(texmru[index]);
}

void getseltex()
{
    if(noedit(true)) return;
    cube &c = lookupcube(sel.o, -sel.grid);
    if(c.children || isempty(c)) return;
    intret(c.texture[sel.orient]);
}

void gettexname(int *tex, int *subslot)
{
    if(noedit(true) || *tex<0) return;
    VSlot &vslot = lookupvslot(*tex, false);
    Slot &slot = *vslot.slot;
    if(!slot.sts.inrange(*subslot)) return;
    result(slot.sts[*subslot].name);
}

ICOMMAND(texmrunum, "", (),
    filltexlist();
    intret(texmru.length())
);
ICOMMAND(texmruresolve, "i", (int *slot),
    filltexlist();
    if(texmru.inrange(*slot))
        intret(texmru[*slot]);
    else
        intret(*slot);
);
ICOMMAND(settex, "i", (int *slot), edittex(*slot));
COMMANDN(edittex, edittex_, "i");
COMMAND(gettex, "");
COMMAND(getcurtex, "");
COMMAND(getseltex, "");
ICOMMAND(getreptex, "", (), { if(!noedit()) intret(vslots.inrange(reptex) ? reptex : -1); });
COMMAND(gettexname, "ii");
ICOMMAND(looptexmru, "re", (ident *id, uint *body),
{
    loopstart(id, stack);
    filltexlist();
    loopv(texmru) { loopiter(id, stack, texmru[i]); execute(body); }
    loopend(id, stack);
});
ICOMMAND(numvslots, "", (), intret(vslots.length()));
ICOMMAND(numslots, "", (), intret(slots.length()));

void replacetexcube(cube &c, int oldtex, int newtex)
{
    loopi(6) if(c.texture[i] == oldtex) c.texture[i] = newtex;
    if(c.children) loopi(8) replacetexcube(c.children[i], oldtex, newtex);
}

void mpreplacetex(int oldtex, int newtex, bool insel, selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_REPLACE, oldtex, newtex, insel ? 1 : 0);
    if(insel)
    {
        loopselxyz(replacetexcube(c, oldtex, newtex));
    }
    else
    {
        loopi(8) replacetexcube(worldroot[i], oldtex, newtex);
    }
    allchanged();
}

void replace(bool insel)
{
    if(noedit()) return;
    if(reptex < 0) { conoutf(CON_ERROR, "can only replace after a texture edit"); return; }
    mpreplacetex(reptex, lasttex, insel, sel, true);
}

ICOMMAND(replace, "", (), replace(false));
ICOMMAND(replacesel, "", (), replace(true));

////////// flip and rotate ///////////////
uint dflip(uint face) { return face==F_EMPTY ? face : 0x88888888 - (((face&0xF0F0F0F0)>>4) | ((face&0x0F0F0F0F)<<4)); }
uint cflip(uint face) { return ((face&0xFF00FF00)>>8) | ((face&0x00FF00FF)<<8); }
uint rflip(uint face) { return ((face&0xFFFF0000)>>16)| ((face&0x0000FFFF)<<16); }
uint mflip(uint face) { return (face&0xFF0000FF) | ((face&0x00FF0000)>>8) | ((face&0x0000FF00)<<8); }

void flipcube(cube &c, int d)
{
    swap(c.texture[d*2], c.texture[d*2+1]);
    c.faces[D[d]] = dflip(c.faces[D[d]]);
    c.faces[C[d]] = cflip(c.faces[C[d]]);
    c.faces[R[d]] = rflip(c.faces[R[d]]);
    if(c.children)
    {
        loopi(8) if(i&octadim(d)) swap(c.children[i], c.children[i-octadim(d)]);
        loopi(8) flipcube(c.children[i], d);
    }
}

void rotatequad(cube &a, cube &b, cube &c, cube &d)
{
    cube t = a; a = b; b = c; c = d; d = t;
}

void rotatecube(cube &c, int d)   // rotates cube clockwise. see pics in cvs for help.
{
    c.faces[D[d]] = cflip (mflip(c.faces[D[d]]));
    c.faces[C[d]] = dflip (mflip(c.faces[C[d]]));
    c.faces[R[d]] = rflip (mflip(c.faces[R[d]]));
    swap(c.faces[R[d]], c.faces[C[d]]);

    swap(c.texture[2*R[d]], c.texture[2*C[d]+1]);
    swap(c.texture[2*C[d]], c.texture[2*R[d]+1]);
    swap(c.texture[2*C[d]], c.texture[2*C[d]+1]);

    if(c.children)
    {
        int row = octadim(R[d]);
        int col = octadim(C[d]);
        for(int i=0; i<=octadim(d); i+=octadim(d)) rotatequad
        (
            c.children[i+row],
            c.children[i],
            c.children[i+col],
            c.children[i+col+row]
        );
        loopi(8) rotatecube(c.children[i], d);
    }
}

void mpflip(selinfo &sel, bool local)
{
    if(local)
    {
        game::edittrigger(sel, EDIT_FLIP);
        makeundo();
    }
    int zs = sel.s[dimension(sel.orient)];
    loopxy(sel)
    {
        loop(z,zs) flipcube(selcube(x, y, z), dimension(sel.orient));
        loop(z,zs/2)
        {
            cube &a = selcube(x, y, z);
            cube &b = selcube(x, y, zs-z-1);
            swap(a, b);
        }
    }
    changed(sel);
}

void flip()
{
    if(noedit()) return;
    mpflip(sel, true);
}

void mprotate(int cw, selinfo &sel, bool local)
{
    if(local)
    {
        game::edittrigger(sel, EDIT_ROTATE, cw);
        makeundo();
    }
    int d = dimension(sel.orient);
    if(!dimcoord(sel.orient)) cw = -cw;
    int m = sel.s[C[d]] < sel.s[R[d]] ? C[d] : R[d];
    int ss = sel.s[m] = max(sel.s[R[d]], sel.s[C[d]]);
    loop(z,sel.s[D[d]]) loopi(cw>0 ? 1 : 3)
    {
        loopxy(sel) rotatecube(selcube(x,y,z), d);
        loop(y,ss/2) loop(x,ss-1-y*2) rotatequad
        (
            selcube(ss-1-y, x+y, z),
            selcube(x+y, y, z),
            selcube(y, ss-1-x-y, z),
            selcube(ss-1-x-y, ss-1-y, z)
        );
    }
    changed(sel);
}

void rotate(int *cw)
{
    if(noedit()) return;
    mprotate(*cw, sel, true);
}

COMMAND(flip, "");
COMMAND(rotate, "i");

enum { EDITMATF_EMPTY = 0x10000, EDITMATF_NOTEMPTY = 0x20000, EDITMATF_SOLID = 0x30000, EDITMATF_NOTSOLID = 0x40000 };
static const struct { const char *name; int filter; } editmatfilters[] =
{
    { "empty", EDITMATF_EMPTY },
    { "notempty", EDITMATF_NOTEMPTY },
    { "solid", EDITMATF_SOLID },
    { "notsolid", EDITMATF_NOTSOLID }
};

void setmat(cube &c, ushort mat, ushort matmask, ushort filtermat, ushort filtermask, int filtergeom)
{
    if(c.children)
        loopi(8) setmat(c.children[i], mat, matmask, filtermat, filtermask, filtergeom);
    else if((c.material&filtermask) == filtermat)
    {
        switch(filtergeom)
        {
            case EDITMATF_EMPTY: if(isempty(c)) break; return;
            case EDITMATF_NOTEMPTY: if(!isempty(c)) break; return;
            case EDITMATF_SOLID: if(isentirelysolid(c)) break; return;
            case EDITMATF_NOTSOLID: if(!isentirelysolid(c)) break; return;
        }
        if(mat!=MAT_AIR)
        {
            c.material &= matmask;
            c.material |= mat;
        }
        else c.material = MAT_AIR;
    }
}

void mpeditmat(int matid, int filter, selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_MAT, matid, filter);

    ushort filtermat = 0, filtermask = 0, matmask;
    int filtergeom = 0;
    if(filter >= 0)
    {
        filtermat = filter&0xFF;
        filtermask = filtermat&(MATF_VOLUME|MATF_INDEX) ? MATF_VOLUME|MATF_INDEX : (filtermat&MATF_CLIP ? MATF_CLIP : filtermat);
        filtergeom = filter&~0xFF;
    }
    if(matid < 0)
    {
        matid = 0;
        matmask = filtermask;
        if(isclipped(filtermat&MATF_VOLUME)) matmask &= ~MATF_CLIP;
        if(isdeadly(filtermat&MATF_VOLUME)) matmask &= ~MAT_DEATH;
    }
    else
    {
        matmask = matid&(MATF_VOLUME|MATF_INDEX) ? 0 : (matid&MATF_CLIP ? ~MATF_CLIP : ~matid);
        if(isclipped(matid&MATF_VOLUME)) matid |= MAT_CLIP;
        if(isdeadly(matid&MATF_VOLUME)) matid |= MAT_DEATH;
    }
    loopselxyz(setmat(c, matid, matmask, filtermat, filtermask, filtergeom));
}

void editmat(char *name, char *filtername)
{
    if(noedit()) return;
    int filter = -1;
    if(filtername[0])
    {
        loopi(sizeof(editmatfilters)/sizeof(editmatfilters[0])) if(!strcmp(editmatfilters[i].name, filtername)) { filter = editmatfilters[i].filter; break; }
        if(filter < 0) filter = findmaterial(filtername);
        if(filter < 0)
        {
            conoutf(CON_ERROR, "unknown material \"%s\"", filtername);
            return;
        }
    }
    int id = -1;
    if(name[0] || filter < 0)
    {
        id = findmaterial(name);
        if(id<0) { conoutf(CON_ERROR, "unknown material \"%s\"", name); return; }
    }
    mpeditmat(id, filter, sel, true);
}

COMMAND(editmat, "ss");

void rendertexturepanel(int w, int h)
{
    if((texpaneltimer -= curtime)>0 && editmode)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        pushhudmatrix();
        hudmatrix.scale(h/1800.0f, h/1800.0f, 1);
        flushhudmatrix(false);
        SETSHADER(hudrgb);

        int y = 50, gap = 10;

        gle::defvertex(2);
        gle::deftexcoord0();

        loopi(7)
        {
            int s = (i == 3 ? 285 : 220), ti = curtexindex+i-3;
            if(ti>=0 && ti<texmru.length())
            {
                VSlot &vslot = lookupvslot(texmru[ti]), *layer = NULL, *decal = NULL;
                Slot &slot = *vslot.slot;
                Texture *tex = slot.sts.empty() ? notexture : slot.sts[0].t, *glowtex = NULL, *layertex = NULL, *decaltex = NULL;
                if(slot.texmask&(1<<TEX_GLOW))
                {
                    loopvj(slot.sts) if(slot.sts[j].type==TEX_GLOW) { glowtex = slot.sts[j].t; break; }
                }
                if(vslot.layer)
                {
                    layer = &lookupvslot(vslot.layer);
                    layertex = layer->slot->sts.empty() ? notexture : layer->slot->sts[0].t;
                }
                if(vslot.decal)
                {
                    decal = &lookupvslot(vslot.decal);
                    decaltex = decal->slot->sts.empty() ? notexture : decal->slot->sts[0].t;
                }
                float sx = min(1.0f, tex->xs/(float)tex->ys), sy = min(1.0f, tex->ys/(float)tex->xs);
                int x = w*1800/h-s-50, r = s;
                vec2 tc[4] = { vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 1) };
                float xoff = vslot.offset.x, yoff = vslot.offset.y;
                if(vslot.rotation)
                {
                    if((vslot.rotation&5) == 1) { swap(xoff, yoff); loopk(4) swap(tc[k].x, tc[k].y); }
                    if(vslot.rotation >= 2 && vslot.rotation <= 4) { xoff *= -1; loopk(4) tc[k].x *= -1; }
                    if(vslot.rotation <= 2 || vslot.rotation == 5) { yoff *= -1; loopk(4) tc[k].y *= -1; }
                }
                loopk(4) { tc[k].x = tc[k].x/sx - xoff/tex->xs; tc[k].x = tc[k].x/sy - yoff/tex->ys; }
                glBindTexture(GL_TEXTURE_2D, tex->id);
                loopj(glowtex ? 3 : 2)
                {
                    if(j < 2) gle::color(vec(vslot.colorscale).mul(j), texpaneltimer/1000.0f);
                    else
                    {
                        glBindTexture(GL_TEXTURE_2D, glowtex->id);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        gle::color(vslot.glowcolor, texpaneltimer/1000.0f);
                    }
                    gle::begin(GL_TRIANGLE_STRIP);
                    gle::attribf(x,   y);   gle::attrib(tc[0]);
                    gle::attribf(x+r, y);   gle::attrib(tc[1]);
                    gle::attribf(x,   y+r); gle::attrib(tc[3]);
                    gle::attribf(x+r, y+r); gle::attrib(tc[2]);
                    xtraverts += gle::end();
                    if(j==1 && decaltex)
                    {
                        glBindTexture(GL_TEXTURE_2D, decaltex->id);
                        gle::begin(GL_TRIANGLE_STRIP);
                        gle::attribf(x,     y);     gle::attrib(tc[0]);
                        gle::attribf(x+r/2, y);     gle::attrib(tc[1]);
                        gle::attribf(x,     y+r/2); gle::attrib(tc[3]);
                        gle::attribf(x+r/2, y+r/2); gle::attrib(tc[2]);
                        xtraverts += gle::end();
                    }
                    if(j==1 && layertex)
                    {
                        gle::color(layer->colorscale, texpaneltimer/1000.0f);
                        glBindTexture(GL_TEXTURE_2D, layertex->id);
                        gle::begin(GL_TRIANGLE_STRIP);
                        gle::attribf(x+r/2, y+r/2); gle::attrib(tc[0]);
                        gle::attribf(x+r,   y+r/2); gle::attrib(tc[1]);
                        gle::attribf(x+r/2, y+r);   gle::attrib(tc[3]);
                        gle::attribf(x+r,   y+r);   gle::attrib(tc[2]);
                        xtraverts += gle::end();
                    }
                    if(!j)
                    {
                        r -= 10;
                        x += 5;
                        y += 5;
                    }
                    else if(j == 2) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
            }
            y += s+gap;
        }

        gle::disable();
        pophudmatrix(true, false);
        hudshader->set();
    }
}

#define EDITSTAT(name, type, val) \
    ICOMMAND(editstat##name, "", (), \
    { \
        extern int statrate; \
        static int laststat = 0; \
        static type prevstat = 0; \
        static type curstat = 0; \
        if(totalmillis - laststat >= statrate) \
        { \
            prevstat = curstat; \
            laststat = totalmillis - (totalmillis%statrate); \
        } \
        if(prevstat == curstat) curstat = (val); \
        type##ret(val); \
    });
EDITSTAT(wtr, int, wtris/1024);
EDITSTAT(vtr, int, (vtris*100)/max(wtris, 1));
EDITSTAT(wvt, int, wverts/1024);
EDITSTAT(vvt, int, (vverts*100)/max(wverts, 1));
EDITSTAT(evt, int, xtraverts/1024);
EDITSTAT(eva, int, xtravertsva/1024);
EDITSTAT(octa, int, allocnodes*8);
EDITSTAT(va, int, allocva);
EDITSTAT(glde, int, glde);
EDITSTAT(geombatch, int, gbatches);
EDITSTAT(oq, int, getnumqueries());
EDITSTAT(pvs, int, getnumviewcells());
