#include "engine.h"

#define NUMCAUSTICS 32

static Texture *caustictex[NUMCAUSTICS] = { NULL };

void loadcaustics(bool force)
{
    static bool needcaustics = false;
    if(force) needcaustics = true;
    if(!caustics || !needcaustics) return;
    useshaderbyname("caustics");
    if(caustictex[0]) return;
    loopi(NUMCAUSTICS)
    {
        defformatstring(name)("<grey>packages/caustics/caust%.2d.png", i);
        caustictex[i] = textureload(name);
    }
}

void cleanupcaustics()
{
    loopi(NUMCAUSTICS) caustictex[i] = NULL;
}

VARFR(causticscale, 0, 50, 10000, preloadwatershaders());
VARFR(causticmillis, 0, 75, 1000, preloadwatershaders());
FVARR(causticcontrast, 0, 0.6f, 2);
FVARR(causticoffset, 0, 0.7f, 1);
VARFP(caustics, 0, 1, 1, { loadcaustics(); preloadwatershaders(); });

void setupcaustics(int tmu, float surface = -1e16f)
{
    if(!caustictex[0]) loadcaustics(true);

    vec s = vec(0.011f, 0, 0.0066f).mul(100.0f/causticscale), t = vec(0, 0.011f, 0.0066f).mul(100.0f/causticscale);
    int tex = (lastmillis/causticmillis)%NUMCAUSTICS;
    float frac = float(lastmillis%causticmillis)/causticmillis;
    loopi(2)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+tmu+i);
        glBindTexture(GL_TEXTURE_2D, caustictex[(tex+i)%NUMCAUSTICS]->id);
    }
    glActiveTexture_(GL_TEXTURE0_ARB);
    float blendscale = causticcontrast, blendoffset = 1;
    if(surface > -1e15f)
    {
        float bz = surface + camera1->o.z + (vertwater ? WATER_AMPLITUDE : 0);
        GLfloat m[16] =
        {
            s.x, t.x,  0, 0,
            s.y, t.y,  0, 0,
            s.z, t.z, -1, 0,
              0,   0, bz, 1
        };
        glMatrixMode(GL_TEXTURE);
        glLoadMatrixf(m);
        glMultMatrixf(worldmatrix.v);
        glMatrixMode(GL_MODELVIEW);
        blendscale *= 0.5f;
        blendoffset = 0;
    }
    else
    {
        GLOBALPARAM(causticsS, (s));
        GLOBALPARAM(causticsT, (t));
    }
    GLOBALPARAM(causticsblend, (blendscale*(1-frac), blendscale*frac, blendoffset - causticoffset*blendscale));
}

void rendercaustics(float surface, float syl, float syr)
{
    if(!caustics || !causticscale || !causticmillis) return;
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
    setupcaustics(0, surface);
    SETSHADER(caustics);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(1, -1);
    glVertex2f(-1, -1);
    glVertex2f(1, syr);
    glVertex2f(-1, syl);
    glEnd();
}

void renderwaterfog(int mat, float surface)
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glActiveTexture_(GL_TEXTURE9_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gdepthtex);
    glActiveTexture_(GL_TEXTURE0_ARB);

    vec p[4] =
    {
        invmvpmatrix.perspectivetransform(vec(-1, -1, -1)),
        invmvpmatrix.perspectivetransform(vec(-1, 1, -1)),
        invmvpmatrix.perspectivetransform(vec(1, -1, -1)),
        invmvpmatrix.perspectivetransform(vec(1, 1, -1))
    };
    float bz = surface + camera1->o.z + (vertwater ? WATER_AMPLITUDE : 0),
          syl = p[1].z > p[0].z ? 2*(bz - p[0].z)/(p[1].z - p[0].z) - 1 : 1,
          syr = p[3].z > p[2].z ? 2*(bz - p[2].z)/(p[3].z - p[2].z) - 1 : 1;

    if(mat == MAT_WATER)
    {
        GLOBALPARAM(waterdeepcolor, (waterdeepcolor.x*ldrscaleb, waterdeepcolor.y*ldrscaleb, waterdeepcolor.z*ldrscaleb));
        ivec deepfade = ivec(waterdeepfadecolor.x, waterdeepfadecolor.y, waterdeepfadecolor.z).mul(waterdeep);
        GLOBALPARAM(waterdeepfade, (deepfade.x ? 255.0f/deepfade.x : 1e4f, deepfade.y ? 255.0f/deepfade.y : 1e4f, deepfade.z ? 255.0f/deepfade.z : 1e4f, waterdeep ? 1.0f/waterdeep : 1e4f));

        rendercaustics(surface, syl, syr);
    }
    else
    {
        GLOBALPARAM(waterdeepcolor, (0, 0, 0));
        GLOBALPARAM(waterdeepfade, (0, 0, 0));
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    vec4 d = mvmatrix.getrow(2);
    GLfloat m[16] =
    {
        d.x, 0,  0, 0,
        d.y, 1,  0, 0,
        d.z, 0, -1, 0,
        d.w, 0, bz, 1
    };
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(m);
    glMultMatrixf(worldmatrix.v);
    glMatrixMode(GL_MODELVIEW);

    SETSHADER(waterfog);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(1, -1);
    glVertex2f(-1, -1);
    glVertex2f(1, syr);
    glVertex2f(-1, syl);
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

VARFP(waterreflect, 0, 1, 1, { preloadwatershaders(); });
VARR(waterreflectstep, 1, 32, 10000);
FVARR(waterrefract, 0, 0.1f, 1e3f);
VARFP(waterenvmap, 0, 1, 1, { preloadwatershaders(); });

/* vertex water */
VARP(watersubdiv, 0, 2, 3);
VARP(waterlod, 0, 1, 3);

static int wx1, wy1, wx2, wy2, wsize;
static float whscale, whoffset;

#define VERTW(vertw, defbody, body) \
    static inline void def##vertw() \
    { \
        varray::defattrib(varray::ATTRIB_VERTEX, 3, GL_FLOAT); \
        defbody; \
    } \
    static inline void vertw(float v1, float v2, float v3) \
    { \
        float angle = float((v1-wx1)*(v2-wy1))*float((v1-wx2)*(v2-wy2))*whscale+whoffset; \
        float s = angle - int(angle) - 0.5f; \
        s *= 8 - fabs(s)*16; \
        float h = WATER_AMPLITUDE*s-WATER_OFFSET; \
        varray::attrib<float>(v1, v2, v3+h); \
        body; \
    }
#define VERTWN(vertw, defbody, body) \
    static inline void def##vertw() \
    { \
        varray::defattrib(varray::ATTRIB_VERTEX, 3, GL_FLOAT); \
        defbody; \
    } \
    static inline void vertw(float v1, float v2, float v3) \
    { \
        float h = -WATER_OFFSET; \
        varray::attrib<float>(v1, v2, v3+h); \
        body; \
    }
#define VERTWT(vertwt, defbody, body) \
    VERTW(vertwt, defbody, { \
        float v = angle - int(angle+0.25f) - 0.25f; \
        v *= 8 - fabs(v)*16; \
        float duv = 0.5f*v; \
        body; \
    })

VERTW(vertwt, {
    varray::defattrib(varray::ATTRIB_TEXCOORD0, 2, GL_FLOAT);
}, {
    varray::attrib<float>(v1/8.0f, v2/8.0f);
})
VERTWN(vertwtn, {
    varray::defattrib(varray::ATTRIB_TEXCOORD0, 2, GL_FLOAT);
}, {
    varray::attrib<float>(v1/8.0f, v2/8.0f);
})

static float lavaxk = 1.0f, lavayk = 1.0f, lavascroll = 0.0f;

VERTW(vertl, {
    varray::defattrib(varray::ATTRIB_TEXCOORD0, 2, GL_FLOAT);
}, {
    varray::attrib<float>(lavaxk*(v1+lavascroll), lavayk*(v2+lavascroll));
})
VERTWN(vertln, {
    varray::defattrib(varray::ATTRIB_TEXCOORD0, 2, GL_FLOAT);
}, {
    varray::attrib<float>(lavaxk*(v1+lavascroll), lavayk*(v2+lavascroll));
})

#define renderwaterstrips(vertw, z) { \
    def##vertw(); \
    for(int x = wx1; x<wx2; x += subdiv) \
    { \
        varray::begin(GL_TRIANGLE_STRIP); \
        vertw(x,        wy1, z); \
        vertw(x+subdiv, wy1, z); \
        for(int y = wy1; y<wy2; y += subdiv) \
        { \
            vertw(x,        y+subdiv, z); \
            vertw(x+subdiv, y+subdiv, z); \
        } \
        xtraverts += varray::end(); \
    } \
}

void rendervertwater(uint subdiv, int xo, int yo, int z, uint size, uchar mat)
{
    wx1 = xo;
    wy1 = yo;
    wx2 = wx1 + size,
    wy2 = wy1 + size;
    wsize = size;
    whscale = 59.0f/(23.0f*wsize*wsize)/(2*M_PI);

    ASSERT((wx1 & (subdiv - 1)) == 0);
    ASSERT((wy1 & (subdiv - 1)) == 0);

    switch(mat)
    {
        case MAT_WATER:
        {
            whoffset = fmod(float(lastmillis/600.0f/(2*M_PI)), 1.0f);
            renderwaterstrips(vertwt, z);
            break;
        }

        case MAT_LAVA:
        {
            whoffset = fmod(float(lastmillis/2000.0f/(2*M_PI)), 1.0f);
            renderwaterstrips(vertl, z);
            break;
        }
    }
}

uint calcwatersubdiv(int x, int y, int z, uint size)
{
    float dist;
    if(camera1->o.x >= x && camera1->o.x < x + size &&
       camera1->o.y >= y && camera1->o.y < y + size)
        dist = fabs(camera1->o.z - float(z));
    else
    {
        vec t(x + size/2, y + size/2, z + size/2);
        dist = t.dist(camera1->o) - size*1.42f/2;
    }
    uint subdiv = watersubdiv + int(dist) / (32 << waterlod);
    if(subdiv >= 8*sizeof(subdiv))
        subdiv = ~0;
    else
        subdiv = 1 << subdiv;
    return subdiv;
}

uint renderwaterlod(int x, int y, int z, uint size, uchar mat)
{
    if(size <= (uint)(32 << waterlod))
    {
        uint subdiv = calcwatersubdiv(x, y, z, size);
        if(subdiv < size * 2) rendervertwater(min(subdiv, size), x, y, z, size, mat);
        return subdiv;
    }
    else
    {
        uint subdiv = calcwatersubdiv(x, y, z, size);
        if(subdiv >= size)
        {
            if(subdiv < size * 2) rendervertwater(size, x, y, z, size, mat);
            return subdiv;
        }
        uint childsize = size / 2,
             subdiv1 = renderwaterlod(x, y, z, childsize, mat),
             subdiv2 = renderwaterlod(x + childsize, y, z, childsize, mat),
             subdiv3 = renderwaterlod(x + childsize, y + childsize, z, childsize, mat),
             subdiv4 = renderwaterlod(x, y + childsize, z, childsize, mat),
             minsubdiv = subdiv1;
        minsubdiv = min(minsubdiv, subdiv2);
        minsubdiv = min(minsubdiv, subdiv3);
        minsubdiv = min(minsubdiv, subdiv4);
        if(minsubdiv < size * 2)
        {
            if(minsubdiv >= size) rendervertwater(size, x, y, z, size, mat);
            else
            {
                if(subdiv1 >= size) rendervertwater(childsize, x, y, z, childsize, mat);
                if(subdiv2 >= size) rendervertwater(childsize, x + childsize, y, z, childsize, mat);
                if(subdiv3 >= size) rendervertwater(childsize, x + childsize, y + childsize, z, childsize, mat);
                if(subdiv4 >= size) rendervertwater(childsize, x, y + childsize, z, childsize, mat);
            }
        }
        return minsubdiv;
    }
}

#define renderwaterquad(vertwn, z) \
    { \
        if(varray::data.empty()) { def##vertwn(); varray::begin(GL_QUADS); } \
        vertwn(x, y, z); \
        vertwn(x+rsize, y, z); \
        vertwn(x+rsize, y+csize, z); \
        vertwn(x, y+csize, z); \
        xtraverts += 4; \
    }

void renderflatwater(int x, int y, int z, uint rsize, uint csize, uchar mat)
{
    switch(mat)
    {
        case MAT_WATER:
            renderwaterquad(vertwtn, z);
            break;

        case MAT_LAVA:
            renderwaterquad(vertln, z);
            break;
    }
}

VARFP(vertwater, 0, 1, 1, allchanged());

static inline void renderwater(const materialsurface &m, int mat = MAT_WATER)
{
    if(!vertwater || minimapping) renderflatwater(m.o.x, m.o.y, m.o.z, m.rsize, m.csize, mat);
    else if(renderwaterlod(m.o.x, m.o.y, m.o.z, m.csize, mat) >= (uint)m.csize * 2)
        rendervertwater(m.csize, m.o.x, m.o.y, m.o.z, m.csize, mat);
}

void renderlava(const materialsurface &m, Texture *tex, float scale)
{
    lavaxk = 8.0f/(tex->xs*scale);
    lavayk = 8.0f/(tex->ys*scale);
    lavascroll = lastmillis/1000.0f;
    renderwater(m, MAT_LAVA);
}

bvec watercolor(0x01, 0x21, 0x2C), waterdeepcolor(0x01, 0x0A, 0x10), waterdeepfadecolor(0x60, 0xBF, 0xFF), waterrefractcolor(0xFF, 0xFF, 0xFF), waterfallcolor(0, 0, 0), waterfallrefractcolor(0xFF, 0xFF, 0xFF);
HVARFR(watercolour, 0, 0x01212C, 0xFFFFFF,
{
    if(!watercolour) watercolour = 0x01212C;
    watercolor = bvec((watercolour>>16)&0xFF, (watercolour>>8)&0xFF, watercolour&0xFF);
});
HVARFR(waterdeepcolour, 0, 0x010A10, 0xFFFFFF,
{
    if(!waterdeepcolour) waterdeepcolour = 0x010A10;
    waterdeepcolor = bvec((waterdeepcolour>>16)&0xFF, (waterdeepcolour>>8)&0xFF, waterdeepcolour&0xFF);
});
HVARFR(waterdeepfade, 0, 0x60BFFF, 0xFFFFFF,
{
    if(!waterdeepfade) waterdeepfade = 0x60BFFF;
    waterdeepfadecolor = bvec((waterdeepfade>>16)&0xFF, (waterdeepfade>>8)&0xFF, waterdeepfade&0xFF);
});
HVARFR(waterrefractcolour, 0, 0xFFFFFF, 0xFFFFFF,
{
    if(!waterrefractcolour) waterrefractcolour = 0xFFFFFF;
    waterrefractcolor = bvec((waterrefractcolour>>16)&0xFF, (waterrefractcolour>>8)&0xFF, waterrefractcolour&0xFF);
});
VARR(waterfog, 0, 30, 10000);
VARR(waterdeep, 0, 50, 10000);
HVARFR(waterfallcolour, 0, 0, 0xFFFFFF,
{
    waterfallcolor = bvec((waterfallcolour>>16)&0xFF, (waterfallcolour>>8)&0xFF, waterfallcolour&0xFF);
});
HVARFR(waterfallrefractcolour, 0, 0, 0xFFFFFF,
{
    waterfallrefractcolor = bvec((waterfallrefractcolour>>16)&0xFF, (waterfallrefractcolour>>8)&0xFF, waterfallrefractcolour&0xFF);
});
bvec lavacolor(0xFF, 0x40, 0x00);
HVARFR(lavacolour, 0, 0xFF4000, 0xFFFFFF,
{
    if(!lavacolour) lavacolour = 0xFF4000;
    lavacolor = bvec((lavacolour>>16)&0xFF, (lavacolour>>8)&0xFF, lavacolour&0xFF);
});
VARR(lavafog, 0, 50, 10000);

VARR(waterspec, 0, 150, 200);

VARFP(waterfallenv, 0, 1, 1, preloadwatershaders());
FVARR(waterfallrefract, 0, 0.1f, 1e3f);
VARR(waterfallspec, 0, 150, 200);

void preloadwatershaders(bool force)
{
    static bool needwater = false;
    if(force) needwater = true;
    if(!needwater) return;

    if(caustics && causticscale && causticmillis)
    {
        if(waterreflect) useshaderbyname("waterreflectcaustics");
        else if(waterenvmap) useshaderbyname("waterenvcaustics");
        else useshaderbyname("watercaustics");
    }
    else
    {
        if(waterreflect) useshaderbyname("waterreflect");
        else if(waterenvmap) useshaderbyname("waterenv");
        else useshaderbyname("water");
    }

    useshaderbyname("underwater");

    if(waterfallenv) useshaderbyname("waterfallenv");
    useshaderbyname("waterfall");

    useshaderbyname("waterfog");

    useshaderbyname("waterminimap");
}

static float wfwave, wfscroll, wfxscale, wfyscale;

static void renderwaterfall(const materialsurface &m, float offset, const vec *normal = NULL)
{
    if(varray::data.empty())
    {
        varray::defattrib(varray::ATTRIB_VERTEX, 3, GL_FLOAT);
        if(normal) varray::defattrib(varray::ATTRIB_NORMAL, 3, GL_FLOAT);
        varray::defattrib(varray::ATTRIB_TEXCOORD0, 2, GL_FLOAT);
        varray::begin(GL_QUADS);
    }
    float x = m.o.x, y = m.o.y, zmin = m.o.z, zmax = zmin;
    if(m.ends&1) zmin += -WATER_OFFSET-WATER_AMPLITUDE;
    if(m.ends&2) zmax += wfwave;
    int csize = m.csize, rsize = m.rsize;
#define GENFACEORIENT(orient, v0, v1, v2, v3) \
        case orient: v0 v1 v2 v3 break;
#undef GENFACEVERTX
#define GENFACEVERTX(orient, vert, mx,my,mz, sx,sy,sz) \
            { \
                vec v(mx sx, my sy, mz sz); \
                varray::attrib<float>(v.x, v.y, v.z); \
                GENFACENORMAL \
                varray::attrib<float>(wfxscale*v.y, -wfyscale*(v.z+wfscroll)); \
            }
#undef GENFACEVERTY
#define GENFACEVERTY(orient, vert, mx,my,mz, sx,sy,sz) \
            { \
                vec v(mx sx, my sy, mz sz); \
                varray::attrib<float>(v.x, v.y, v.z); \
                GENFACENORMAL \
                varray::attrib<float>(wfxscale*v.x, -wfyscale*(v.z+wfscroll)); \
            }
#define GENFACENORMAL varray::attrib<float>(n.x, n.y, n.z);
    if(normal)
    {
        vec n = *normal;
        switch(m.orient) { GENFACEVERTSXY(x, x, y, y, zmin, zmax, /**/, + csize, /**/, + rsize, + offset, - offset) }
    }
#undef GENFACENORMAL
#define GENFACENORMAL
    else switch(m.orient) { GENFACEVERTSXY(x, x, y, y, zmin, zmax, /**/, + csize, /**/, + rsize, + offset, - offset) }
#undef GENFACENORMAL
#undef GENFACEORIENT
#undef GENFACEVERTX
#define GENFACEVERTX(o,n, x,y,z, xv,yv,zv) GENFACEVERT(o,n, x,y,z, xv,yv,zv)
#undef GENFACEVERTY
#define GENFACEVERTY(o,n, x,y,z, xv,yv,zv) GENFACEVERT(o,n, x,y,z, xv,yv,zv)
}

FVARR(lavaglowmin, 0, 0.25f, 2);
FVARR(lavaglowmax, 0, 1.0f, 2);
VARR(lavaspec, 0, 25, 200);

void renderlava()
{
    if(lavasurfs.empty() && lavafallsurfs.empty()) return;

    MSlot &lslot = lookupmaterialslot(MAT_LAVA, false);

    SETSHADER(lava);
    float t = lastmillis/2000.0f;
    t -= floor(t);
    t = 1.0f - 2*fabs(t-0.5f);
    t = 0.5f + 0.5f*t;
    LOCALPARAM(lavaglow, (0.5f*(lavaglowmin + (lavaglowmax-lavaglowmin)*t)));
    LOCALPARAM(lavaspec, (0.5f*lavaspec/100.0f));

    if(lavasurfs.length())
    {
        Texture *tex = lslot.sts.inrange(0) ? lslot.sts[0].t: notexture;
        glBindTexture(GL_TEXTURE_2D, tex->id);
        glActiveTexture_(GL_TEXTURE1_ARB);
        glBindTexture(GL_TEXTURE_2D, lslot.sts.inrange(2) ? lslot.sts[2].t->id : notexture->id);
        glActiveTexture_(GL_TEXTURE0_ARB);

        loopv(lavasurfs) renderlava(lavasurfs[i], tex, lslot.scale);
        xtraverts += varray::end();
    }

    if(!minimapping && lavafallsurfs.length())
    {
        Texture *tex = lslot.sts.inrange(1) ? lslot.sts[1].t : (lslot.sts.inrange(0) ? lslot.sts[0].t : notexture);
        float angle = fmod(float(lastmillis/2000.0f/(2*M_PI)), 1.0f),
              s = angle - int(angle) - 0.5f;
        s *= 8 - fabs(s)*16;
        wfwave = vertwater ? WATER_AMPLITUDE*s-WATER_OFFSET : -WATER_OFFSET;
        wfscroll = 16.0f*lastmillis/3000.0f;
        wfxscale = TEX_SCALE/(tex->xs*lslot.scale);
        wfyscale = TEX_SCALE/(tex->ys*lslot.scale);

        glBindTexture(GL_TEXTURE_2D, tex->id);
        glActiveTexture_(GL_TEXTURE1_ARB);
        glBindTexture(GL_TEXTURE_2D, lslot.sts.inrange(3) ? lslot.sts[3].t->id : notexture->id);
        glActiveTexture_(GL_TEXTURE0_ARB);

        loopv(lavafallsurfs)
        {
            materialsurface &m = lavafallsurfs[i];
            renderwaterfall(m, 0.1f, &matnormals[m.orient]);
        }
        xtraverts += varray::end();
    }
}

void renderwaterfalls()
{
    if(waterfallsurfs.empty()) return;

    MSlot &wslot = lookupmaterialslot(MAT_WATER);

    Texture *tex = wslot.sts.inrange(1) ? wslot.sts[1].t : notexture;
    float angle = fmod(float(lastmillis/600.0f/(2*M_PI)), 1.0f),
          s = angle - int(angle) - 0.5f;
    s *= 8 - fabs(s)*16;
    wfwave = vertwater ? WATER_AMPLITUDE*s-WATER_OFFSET : -WATER_OFFSET;
    wfscroll = 16.0f*lastmillis/1000.0f;
    wfxscale = TEX_SCALE/(tex->xs*wslot.scale);
    wfyscale = TEX_SCALE/(tex->ys*wslot.scale);

    bvec color = waterfallcolor.iszero() ? watercolor : waterfallcolor, refractcolor = waterfallrefractcolor.iszero() ? waterrefractcolor : waterfallrefractcolor;
    float colorscale = (0.5f/255), refractscale = colorscale/ldrscale;
    GLOBALPARAM(waterfallcolor, (color.x*colorscale, color.y*colorscale, color.z*colorscale));
    GLOBALPARAM(waterfallrefract, (refractcolor.x*refractscale, refractcolor.y*refractscale, refractcolor.z*refractscale, waterfallrefract*viewh));
    GLOBALPARAM(waterfallspec, (0.5f*waterfallspec/100.0f));

    if(waterfallenv) SETSHADER(waterfallenv);
    else SETSHADER(waterfall);

    glBindTexture(GL_TEXTURE_2D, tex->id);
    glActiveTexture_(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_2D, wslot.sts.inrange(4) ? wslot.sts[4].t->id : notexture->id);
    glActiveTexture_(GL_TEXTURE2_ARB);
    glBindTexture(GL_TEXTURE_2D, wslot.sts.inrange(5) ? wslot.sts[5].t->id : notexture->id);
    if(waterfallenv)
    {
        glActiveTexture_(GL_TEXTURE3_ARB);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, lookupenvmap(wslot));
    }
    glActiveTexture_(GL_TEXTURE0_ARB);

    loopv(waterfallsurfs)
    {
        materialsurface &m = waterfallsurfs[i];
        renderwaterfall(m, 0.1f, &matnormals[m.orient]);
    }
    xtraverts += varray::end();
}

void renderwater()
{
    if(watersurfs.empty()) return;

    MSlot &wslot = lookupmaterialslot(MAT_WATER);

    glBindTexture(GL_TEXTURE_2D, wslot.sts.inrange(2) ? wslot.sts[2].t->id : notexture->id);
    glActiveTexture_(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_2D, wslot.sts.inrange(3) ? wslot.sts[3].t->id : notexture->id);
    if(caustics && causticscale && causticmillis) setupcaustics(2);
    if(waterenvmap && !waterreflect && !minimapping)
    {
        glActiveTexture_(GL_TEXTURE4_ARB);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, lookupenvmap(wslot));
    }
    glActiveTexture_(GL_TEXTURE0_ARB);

    float colorscale = 0.5f/255, refractscale = colorscale/ldrscale, reflectscale = 0.5f/ldrscale;
    GLOBALPARAM(watercolor, (watercolor.x*colorscale, watercolor.y*colorscale, watercolor.z*colorscale));
    GLOBALPARAM(waterdeepcolor, (waterdeepcolor.x*colorscale, waterdeepcolor.y*colorscale, waterdeepcolor.z*colorscale));
    GLOBALPARAM(waterfog, (waterfog ? 1.0f/waterfog : 1e4f));
    ivec deepfade = ivec(waterdeepfadecolor.x, waterdeepfadecolor.y, waterdeepfadecolor.z).mul(waterdeep);
    GLOBALPARAM(waterdeepfade, (deepfade.x ? 255.0f/deepfade.x : 1e4f, deepfade.y ? 255.0f/deepfade.y : 1e4f, deepfade.z ? 255.0f/deepfade.z : 1e4f, waterdeep ? 1.0f/waterdeep : 1e4f));
    GLOBALPARAM(waterspec, (0.5f*waterspec/100.0f));
    GLOBALPARAM(waterreflect, (reflectscale, reflectscale, reflectscale, waterreflectstep));
    GLOBALPARAM(waterrefract, (waterrefractcolor.x*refractscale, waterrefractcolor.y*refractscale, waterrefractcolor.z*refractscale, waterrefract*viewh));

    #define SETWATERSHADER(which, name) \
    do { \
        static Shader *name##shader = NULL; \
        if(!name##shader) name##shader = lookupshaderbyname(#name); \
        which##shader = name##shader; \
    } while(0)

    Shader *aboveshader = NULL;
    if(minimapping) SETWATERSHADER(above, waterminimap);
    else if(caustics && causticscale && causticmillis)
    {
        if(waterreflect) SETWATERSHADER(above, waterreflectcaustics);
        else if(waterenvmap) SETWATERSHADER(above, waterenvcaustics);
        else SETWATERSHADER(above, watercaustics);
    }
    else
    {
        if(waterreflect) SETWATERSHADER(above, waterreflect);
        else if(waterenvmap) SETWATERSHADER(above, waterenv);
        else SETWATERSHADER(above, water);
    }

    Shader *belowshader = NULL;
    if(!minimapping) SETWATERSHADER(below, underwater);

    aboveshader->set();
    loopv(watersurfs)
    {
        materialsurface &m = watersurfs[i];
        if(camera1->o.z < m.o.z - WATER_OFFSET) continue;
        renderwater(m);
    }
    xtraverts += varray::end();

    if(belowshader)
    {
        belowshader->set();
        loopv(watersurfs)
        {
            materialsurface &m = watersurfs[i];
            if(camera1->o.z >= m.o.z - WATER_OFFSET) continue;
            renderwater(m);
        }
        xtraverts += varray::end();
    }
}
