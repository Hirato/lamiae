#include "engine.h"

extern int tqaamovemask, tqaamovemaskreduce, tqaamovemaskprec, tqaapack;

int tqaaframe = 0;
GLuint tqaaprevtex = 0, tqaacurtex = 0, tqaamasktex = 0, tqaafbo[3] = { 0, 0, 0 };
glmatrixf tqaaprevmvp;

void loadtqaashaders()
{
    loadhdrshaders(tqaapack ? AA_RESERVED : AA_VELOCITY);

    if(tqaamovemask) useshaderbyname("tqaamaskmovement");
    if(tqaapack) useshaderbyname("tqaapackvelocity");
    useshaderbyname(tqaamovemask ? "tqaaresolvemasked" : "tqaaresolve");
}

void setuptqaa(int w, int h)
{
    if(!tqaaprevtex) glGenTextures(1, &tqaaprevtex);
    if(!tqaacurtex) glGenTextures(1, &tqaacurtex);
    createtexture(tqaaprevtex, w, h, NULL, 3, 1, GL_RGBA8, GL_TEXTURE_RECTANGLE_ARB);
    createtexture(tqaacurtex, w, h, NULL, 3, 1, GL_RGBA8, GL_TEXTURE_RECTANGLE_ARB);
    if(tqaamovemask)
    {
        if(!tqaamasktex) glGenTextures(1, &tqaamasktex);
        int maskw = (w + (1<<tqaamovemaskreduce) - 1) >> tqaamovemaskreduce, maskh = (h + (1<<tqaamovemaskreduce) - 1) >> tqaamovemaskreduce;
        createtexture(tqaamasktex, maskw, maskh, NULL, 3, 1, hasTRG && tqaamovemaskprec ? GL_R8 : GL_RGBA8, GL_TEXTURE_RECTANGLE_ARB);
    }
    loopi(tqaamovemask ? 3 : 2)
    {
        if(!tqaafbo[i]) glGenFramebuffers_(1, &tqaafbo[i]);
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, tqaafbo[i]);
        GLuint tex = 0;
        switch(i)
        {
            case 0: tex = tqaacurtex; break;
            case 1: tex = tqaaprevtex; break;
            case 2: tex = tqaamasktex; break;
        }
        glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, tex, 0);
        if(i <= 1) bindgdepth();
        if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
            fatal("failed allocating TQAA buffer!");
    }
    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);

    tqaaprevmvp.identity();

    loadtqaashaders();
}

void cleanuptqaa()
{
    if(tqaaprevtex) { glDeleteTextures(1, &tqaaprevtex); tqaaprevtex = 0; }
    if(tqaacurtex) { glDeleteTextures(1, &tqaacurtex); tqaacurtex = 0; }
    if(tqaamasktex) { glDeleteTextures(1, &tqaamasktex); tqaamasktex = 0; }
    loopi(3) if(tqaafbo[i]) { glDeleteFramebuffers_(1, &tqaafbo[i]); tqaafbo[i] = 0; }

    tqaaframe = 0;
}

VARFP(tqaa, 0, 0, 1, cleanupaa());
VARF(tqaapack, 0, 0, 1, cleanupaa());
FVAR(tqaareproject, 0, 300, 1e3f);
FVAR(tqaareprojectscale, 0, 4, 1e3f);
VARFP(tqaamovemask, 0, 1, 1, cleanuptqaa());
VARFP(tqaamovemaskreduce, 0, 0, 2, cleanuptqaa());
VARFP(tqaamovemaskprec, 0, 1, 1, cleanuptqaa());
VARP(tqaaquincunx, 0, 1, 1);

void setaavelocityparams(GLenum tmu)
{
    if(tmu!=GL_TEXTURE0_ARB) glActiveTexture_(tmu);
    if(msaasamples) glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msdepthtex);
    else glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gdepthtex);
    glMatrixMode(GL_TEXTURE);
    glmatrixf reproject;
    reproject.mul(tqaaframe ? tqaaprevmvp : screenmatrix, worldmatrix);
    vec2 jitter = tqaaframe&1 ? vec2(0.5f, 0.5f) : vec2(-0.5f, -0.5f);
    if(tqaaframe) reproject.jitter(jitter.x, jitter.y);
    glLoadMatrixf(reproject.v);
    glMatrixMode(GL_MODELVIEW);
    float maxvel = sqrtf(vieww*vieww + viewh*viewh)/tqaareproject;
    LOCALPARAM(maxvelocity, (maxvel, 1/maxvel, tqaareprojectscale));
    if(tmu!=GL_TEXTURE0_ARB) glActiveTexture_(GL_TEXTURE0_ARB);
}

void packtqaa(GLuint packfbo)
{
    if(tqaamovemask)
    {
        int maskw = (vieww + (1<<tqaamovemaskreduce) - 1) >> tqaamovemaskreduce, maskh = (viewh + (1<<tqaamovemaskreduce) - 1) >> tqaamovemaskreduce;
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, tqaafbo[2]);
        if(!tqaaframe)
        {
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        SETSHADER(tqaamaskmovement);
        if(msaasamples) glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msnormaltex);
        else glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gnormaltex);
        if(tqaamovemaskreduce)
        {
            glViewport(0, 0, maskw, maskh);
            if(!msaasamples)
            {
                glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }
        }
        glBlendFunc(GL_ONE, GL_SRC_ALPHA);
        glEnable(GL_BLEND);
        screenquad(maskw<<tqaamovemaskreduce, maskh<<tqaamovemaskreduce);
        glDisable(GL_BLEND);
        if(tqaamovemaskreduce)
        {
            if(!msaasamples)
            {
                glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }
            glViewport(0, 0, vieww, viewh);
        }
    }

    if(tqaapack)
    {
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, packfbo);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
        SETSHADER(tqaapackvelocity);
        setaavelocityparams();
        screenquad(vieww, viewh);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
}

void resolvetqaa(GLuint outfbo)
{
    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, outfbo);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tqaacurtex);
    glActiveTexture_(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tqaaframe ? tqaaprevtex : tqaacurtex);
    glActiveTexture_(GL_TEXTURE2_ARB);
    if(msaasamples) glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msdepthtex);
    else glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gdepthtex);
    if(tqaamovemask)
    {
       glActiveTexture_(GL_TEXTURE3_ARB);
       glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tqaamasktex);
    }
    glActiveTexture_(GL_TEXTURE0_ARB);
    if(tqaamovemask) 
    {
        SETSHADER(tqaaresolvemasked); 
        LOCALPARAM(movemaskscale, (1/float(1<<tqaamovemaskreduce)));
    }
    else SETSHADER(tqaaresolve);
    float maxvel = sqrtf(vieww*vieww + viewh*viewh)/tqaareproject;
    LOCALPARAM(maxvelocity, (maxvel, 1/maxvel, tqaareprojectscale));
    vec4 quincunx(0, 0, 0, 0);
    if(tqaaquincunx) quincunx = tqaaframe&1 ? vec4(0.25f, 0.25f, -0.25f, -0.25f) : vec4(-0.25f, -0.25f, 0.25f, 0.25f);
    LOCALPARAM(quincunx, (quincunx));
    screenquad(vieww, viewh, vieww/float(1<<tqaamovemaskreduce), viewh/float(1<<tqaamovemaskreduce));

    swap(tqaafbo[0], tqaafbo[1]);
    swap(tqaacurtex, tqaaprevtex);
    tqaaprevmvp = screenmatrix;
    tqaaframe++;
}

void dotqaa(GLuint outfbo = 0)
{
    timer *tqaatimer = begintimer("tqaa");

    packtqaa(tqaafbo[0]);
    resolvetqaa(outfbo);

    endtimer(tqaatimer);
}

GLuint fxaafbo = 0, fxaatex = 0;

extern int fxaaquality, fxaagreenluma;

static Shader *fxaashader = NULL;

void loadfxaashaders()
{
    loadhdrshaders(tqaa ? (tqaapack ? AA_RESERVED : AA_VELOCITY) : (!fxaagreenluma ? AA_LUMA : AA_UNUSED));

    string opts;
    int optslen = 0;
    if(fxaagreenluma || (tqaa && !tqaapack)) opts[optslen++] = 'g';
    opts[optslen] = '\0';

    defformatstring(fxaaname)("fxaa%d%s", fxaaquality, opts);
    fxaashader = generateshader(fxaaname, "fxaashaders %d \"%s\"", fxaaquality, opts);
}

void clearfxaashaders()
{
    fxaashader = NULL;
}

void setupfxaa(int w, int h)
{
    if(!fxaatex) glGenTextures(1, &fxaatex);
    if(!fxaafbo) glGenFramebuffers_(1, &fxaafbo);
    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, fxaafbo);
    createtexture(fxaatex, w, h, NULL, 3, 1, tqaa || !fxaagreenluma ? GL_RGBA8 : GL_RGB, GL_TEXTURE_RECTANGLE_ARB);
    glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, fxaatex, 0);
    bindgdepth();
    if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
        fatal("failed allocating FXAA buffer!");
    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);

    loadfxaashaders();
}

void cleanupfxaa()
{
    if(fxaafbo) { glDeleteFramebuffers_(1, &fxaafbo); fxaafbo = 0; }
    if(fxaatex) { glDeleteTextures(1, &fxaatex); fxaatex = 0; }

    clearfxaashaders();
}

VARFP(fxaa, 0, 0, 1, cleanupfxaa());
VARFP(fxaaquality, 0, 1, 3, cleanupfxaa());
VARFP(fxaagreenluma, 0, 0, 1, cleanupfxaa());

void dofxaa(GLuint outfbo = 0)
{
    timer *fxaatimer = begintimer("fxaa");

    if(tqaa) packtqaa(fxaafbo);

    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, tqaa ? tqaafbo[0] : outfbo);
    fxaashader->set();
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, fxaatex);
    screenquad(vieww, viewh);

    if(tqaa) resolvetqaa(outfbo);

    endtimer(fxaatimer);
}
 
GLuint smaaareatex = 0, smaasearchtex = 0, smaafbo[3] = { 0, 0, 0 }, smaatex[3] = { 0, 0, 0 };

extern int smaaquality, smaagreenluma, smaacoloredge, smaadepthmask, smaastencil;

static Shader *smaalumaedgeshader = NULL, *smaacoloredgeshader = NULL, *smaablendweightshader = NULL, *smaaneighborhoodshader = NULL;

void loadsmaashaders()
{
    loadhdrshaders(tqaa ? (tqaapack ? AA_RESERVED : AA_VELOCITY) : (!smaagreenluma && !smaacoloredge ? AA_LUMA : AA_UNUSED));

    string opts;
    int optslen = 0;
    if(smaadepthmask || smaastencil) opts[optslen++] = 'd';
    if(smaagreenluma || (tqaa && !tqaapack)) opts[optslen++] = 'g';
    if(tqaa) opts[optslen++] = 't';
    opts[optslen] = '\0';

    defformatstring(lumaedgename)("SMAALumaEdgeDetection%d%s", smaaquality, opts);
    defformatstring(coloredgename)("SMAAColorEdgeDetection%d%s", smaaquality, opts);
    defformatstring(blendweightname)("SMAABlendingWeightCalculation%d%s", smaaquality, opts);
    defformatstring(neighborhoodname)("SMAANeighborhoodBlending%d%s", smaaquality, opts);
    smaalumaedgeshader = lookupshaderbyname(lumaedgename);
    smaacoloredgeshader = lookupshaderbyname(coloredgename);
    smaablendweightshader = lookupshaderbyname(blendweightname);
    smaaneighborhoodshader = lookupshaderbyname(neighborhoodname);

    if(smaalumaedgeshader && smaacoloredgeshader && smaablendweightshader && smaaneighborhoodshader) return;

    generateshader(NULL, "smaashaders %d \"%s\"", smaaquality, opts);
    smaalumaedgeshader = lookupshaderbyname(lumaedgename);
    if(!smaalumaedgeshader) smaalumaedgeshader = nullshader;
    smaacoloredgeshader = lookupshaderbyname(coloredgename);
    if(!smaacoloredgeshader) smaacoloredgeshader = nullshader;
    smaablendweightshader = lookupshaderbyname(blendweightname);
    if(!smaablendweightshader) smaablendweightshader = nullshader;
    smaaneighborhoodshader = lookupshaderbyname(neighborhoodname);
    if(!smaaneighborhoodshader) smaaneighborhoodshader = nullshader;
}

void clearsmaashaders()
{
    smaalumaedgeshader = NULL;
    smaacoloredgeshader = NULL;
    smaablendweightshader = NULL;
    smaaneighborhoodshader = NULL;
}

#include "AreaTex.h"
#include "SearchTex.h"

void setupsmaa(int w, int h)
{
    if(!smaaareatex) glGenTextures(1, &smaaareatex);
    if(!smaasearchtex) glGenTextures(1, &smaasearchtex);
    createtexture(smaaareatex, AREATEX_WIDTH, AREATEX_HEIGHT, areaTexBytes, 3, 1, GL_LUMINANCE_ALPHA, GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, false);
    createtexture(smaasearchtex, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, searchTexBytes, 3, 0, GL_LUMINANCE, GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, false);
    loopi(3)
    {
        if(!smaatex[i]) glGenTextures(1, &smaatex[i]);
        if(!smaafbo[i]) glGenFramebuffers_(1, &smaafbo[i]);
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, smaafbo[i]);
        GLenum format = GL_RGB;
        switch(i)
        {
            case 0: format = tqaa || (!smaagreenluma && !smaacoloredge) ? GL_RGBA8 : GL_RGB; break;
            case 1: format = hasTRG && !tqaa ? GL_RG8 : GL_RGBA8; break;
            case 2: format = GL_RGBA8; break;
        }  
        createtexture(smaatex[i], w, h, NULL, 3, 1, format, GL_TEXTURE_RECTANGLE_ARB);
        glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, smaatex[i], 0);
        if(!i || (smaadepthmask || smaastencil)) bindgdepth();
        if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
            fatal("failed allocating SMAA buffer!");
    }
    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);

    loadsmaashaders();
}

void cleanupsmaa()
{
    if(smaaareatex) { glDeleteTextures(1, &smaaareatex); smaaareatex = 0; }
    if(smaasearchtex) { glDeleteTextures(1, &smaasearchtex); smaasearchtex = 0; }
    loopi(3) if(smaafbo[i]) { glDeleteFramebuffers_(1, &smaafbo[i]); smaafbo[i] = 0; }
    loopi(3) if(smaatex[i]) { glDeleteTextures(1, &smaatex[i]); smaatex[i] = 0; }

    clearsmaashaders();
}

VARFP(smaa, 0, 0, 1, cleanupsmaa());
VARFP(smaaquality, 0, 2, 3, cleanupsmaa());
VARP(smaacoloredge, 0, 0, 1);
VARFP(smaagreenluma, 0, 0, 1, cleanupsmaa());
VARF(smaadepthmask, 0, 1, 1, cleanupsmaa());
VARF(smaastencil, 0, 1, 1, cleanupsmaa());
VAR(debugsmaa, 0, 0, 5);

void viewsmaa()
{
    int w = min(screen->w, screen->h)*1.0f, h = (w*screen->h)/screen->w, tw = gw, th = gh;
    rectshader->set();
    glColor3f(1, 1, 1);
    switch(debugsmaa)
    {
        case 1: glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaatex[0]); break;
        case 2: glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaatex[1]); break;
        case 3: glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaatex[2]); break;
        case 4: glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaaareatex); tw = AREATEX_WIDTH; th = AREATEX_HEIGHT; break;
        case 5: glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaasearchtex); tw = SEARCHTEX_WIDTH; th = SEARCHTEX_HEIGHT; break;
    }
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0, th); glVertex2i(0, 0);
    glTexCoord2f(tw, th); glVertex2i(w, 0);
    glTexCoord2f(0, 0); glVertex2i(0, h);
    glTexCoord2f(tw, 0); glVertex2i(w, h);
    glEnd();
    notextureshader->set();
}

void dosmaa(GLuint outfbo = 0)
{
    timer *smaatimer = begintimer("smaa");

    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, smaafbo[1]);
    if(smaadepthmask || smaastencil)
    {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | (msaasamples ? GL_DEPTH_BUFFER_BIT | ((gdepthstencil && hasDS) || gstencil ? GL_STENCIL_BUFFER_BIT : 0) : 0));
    }
    if(smaadepthmask)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glDepthRange(1, 1);
    }
    else if(smaastencil && ((gdepthstencil && hasDS) || gstencil))
    {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 0x10, ~0);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }
    if(smaacoloredge) smaacoloredgeshader->set();
    else smaalumaedgeshader->set();
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaatex[0]);
    if(tqaa)
    {
        glActiveTexture_(GL_TEXTURE1_ARB);
        if(msaasamples) glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msnormaltex);
        else glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gnormaltex);
        glActiveTexture_(GL_TEXTURE0_ARB);
    }
    screenquad(vieww, viewh);

    if(tqaa) packtqaa(smaafbo[0]);

    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, smaafbo[2]);
    if(smaadepthmask)
    {
        glDepthFunc(GL_EQUAL);
        glDepthMask(GL_FALSE);
    }
    else if(smaastencil && ((gdepthstencil && hasDS) || gstencil))
    {
        glStencilFunc(GL_EQUAL, 0x10, ~0);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }
    if(smaadepthmask || smaastencil) glClear(GL_COLOR_BUFFER_BIT);
    smaablendweightshader->set();
    LOCALPARAM(subsamples, (tqaa ? (tqaaframe&1) + 1 : 0)); 
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaatex[1]);
    glActiveTexture_(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaaareatex);
    glActiveTexture_(GL_TEXTURE2_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaasearchtex);
    glActiveTexture_(GL_TEXTURE0_ARB);
    screenquad(vieww, viewh);
    if(smaadepthmask)
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glDepthRange(0, 1);
    }
    else if(smaastencil && ((gdepthstencil && hasDS) || gstencil)) glDisable(GL_STENCIL_TEST);

    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, tqaa ? tqaafbo[0] : outfbo);
    smaaneighborhoodshader->set();
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaatex[0]);
    glActiveTexture_(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, smaatex[2]);
    glActiveTexture_(GL_TEXTURE0_ARB);
    screenquad(vieww, viewh);

    if(tqaa) resolvetqaa(outfbo);

    endtimer(smaatimer);
}

void setupaa(int w, int h)
{
    if(smaa) { if(!smaafbo[0]) setupsmaa(w, h); }
    else if(fxaa) { if(!fxaafbo) setupfxaa(w, h); }

    if(tqaa && !tqaafbo[0]) setuptqaa(w, h);
}

glmatrixf nojittermatrix;
bool aamask = false;

void jitteraa()
{
    nojittermatrix = projmatrix;

    if(tqaa)
    {
        vec2 jitter = tqaaframe&1 ? vec2(0.25f, 0.25f) : vec2(-0.25f, -0.25f);
        projmatrix.jitter(jitter.x*2.0f/vieww, jitter.y*2.0f/viewh);
    }

    aamask = false;
    GLOBALPARAM(aamask, (0.0f));
}

void setaamask(bool val)
{
    if(!tqaa || !tqaamovemask || aamask == val) return;
    
    aamask = val;

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(aamask ? nojittermatrix.v : projmatrix.v);
    glMatrixMode(GL_MODELVIEW);

    GLOBALPARAM(aamask, (aamask ? 1.0f : 0.0f));
}
 
bool maskedaa()
{
    return tqaa && tqaamovemask;
}
   
void doaa(GLuint outfbo, void (*resolve)(GLuint, int))
{
    if(smaa) { resolve(smaafbo[0], tqaa ? (tqaapack ? AA_RESERVED : AA_VELOCITY) : (!smaagreenluma && !smaacoloredge ? AA_LUMA : AA_UNUSED)); dosmaa(outfbo); }
    else if(fxaa) { resolve(fxaafbo, tqaa ? (tqaapack ? AA_RESERVED : AA_VELOCITY) : (!fxaagreenluma ? AA_LUMA : AA_UNUSED)); dofxaa(outfbo); }
    else if(tqaa) { resolve(tqaafbo[0], tqaapack ? AA_RESERVED : AA_VELOCITY); dotqaa(outfbo); }
    else resolve(outfbo, AA_UNUSED);
}

bool debugaa()
{
    if(debugsmaa) viewsmaa();
    else return false;
    return true;
}

void cleanupaa()
{
    cleanupsmaa();
    cleanupfxaa();
    cleanuptqaa();
}

