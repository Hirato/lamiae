VARP(softexplosion, 0, 1, 1);
VARP(softexplosionblend, 1, 16, 64);

//cache our unit hemisphere
static GLushort *hemiindices = NULL;
static vec *hemiverts = NULL;
static int heminumverts = 0, heminumindices = 0;
static GLuint hemivbuf = 0, hemiebuf = 0;

static void subdivide(int depth, int face);

static void genface(int depth, int i1, int i2, int i3)
{
    int face = heminumindices; heminumindices += 3;
    hemiindices[face]   = i1;
    hemiindices[face+1] = i2;
    hemiindices[face+2] = i3;
    subdivide(depth, face);
}

static void subdivide(int depth, int face)
{
    if(depth-- <= 0) return;
    int idx[6];
    loopi(3) idx[i] = hemiindices[face+i];
    loopi(3)
    {
        int vert = heminumverts++;
        hemiverts[vert] = vec(hemiverts[idx[i]]).add(hemiverts[idx[(i+1)%3]]).normalize(); //push on to unit sphere
        idx[3+i] = vert;
        hemiindices[face+i] = vert;
    }
    subdivide(depth, face);
    loopi(3) genface(depth, idx[i], idx[3+i], idx[3+(i+2)%3]);
}

//subdiv version wobble much more nicely than a lat/longitude version
static void inithemisphere(int hres, int depth)
{
    const int tris = hres << (2*depth);
    heminumverts = heminumindices = 0;
    DELETEA(hemiverts);
    DELETEA(hemiindices);
    hemiverts = new vec[tris+1];
    hemiindices = new GLushort[tris*3];
    hemiverts[heminumverts++] = vec(0.0f, 0.0f, 1.0f); //build initial 'hres' sided pyramid
    loopi(hres) hemiverts[heminumverts++] = vec(sincos360[(360*i)/hres], 0.0f);
    loopi(hres) genface(depth, 0, i+1, 1+(i+1)%hres);

    if(hasVBO)
    {
        if(!hemivbuf) glGenBuffers_(1, &hemivbuf);
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, hemivbuf);
        glBufferData_(GL_ARRAY_BUFFER_ARB, heminumverts*sizeof(vec), hemiverts, GL_STATIC_DRAW_ARB);
        DELETEA(hemiverts);

        if(!hemiebuf) glGenBuffers_(1, &hemiebuf);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, hemiebuf);
        glBufferData_(GL_ELEMENT_ARRAY_BUFFER_ARB, heminumindices*sizeof(GLushort), hemiindices, GL_STATIC_DRAW_ARB);
        DELETEA(hemiindices);
    }
}

static GLuint expmodtex[2] = {0, 0};
static GLuint lastexpmodtex = 0;

static GLuint createexpmodtex(int size, float minval)
{
    uchar *data = new uchar[size*size], *dst = data;
    loop(y, size) loop(x, size)
    {
        float dx = 2*float(x)/(size-1) - 1, dy = 2*float(y)/(size-1) - 1;
        float z = max(0.0f, 1.0f - dx*dx - dy*dy);
        if(minval) z = sqrtf(z);
        else loopk(2) z *= z;
        *dst++ = uchar(max(z, minval)*255);
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    createtexture(tex, size, size, data, 3, 2, GL_ALPHA);
    delete[] data;
    return tex;
}

static struct spherevert
{
    vec pos;
    float s, t;
} *sphereverts = NULL;
static GLushort *sphereindices = NULL;
static int spherenumverts = 0, spherenumindices = 0;
static GLuint spherevbuf = 0, sphereebuf = 0;

static void initsphere(int slices, int stacks)
{
    DELETEA(sphereverts);
    spherenumverts = (stacks+1)*(slices+1);
    sphereverts = new spherevert[spherenumverts];
    float ds = 1.0f/slices, dt = 1.0f/stacks, t = 1.0f;
    loopi(stacks+1)
    {
        float rho = M_PI*(1-t), s = 0.0f;
        loopj(slices+1)
        {
            float theta = j==slices ? 0 : 2*M_PI*s;
            spherevert &v = sphereverts[i*(slices+1) + j];
            v.pos = vec(-sin(theta)*sin(rho), cos(theta)*sin(rho), cos(rho));
            v.s = s;
            v.t = t;
            s += ds;
        }
        t -= dt;
    }

    DELETEA(sphereindices);
    spherenumindices = stacks*slices*3*2;
    sphereindices = new ushort[spherenumindices];
    GLushort *curindex = sphereindices;
    loopi(stacks)
    {
        loopk(slices)
        {
            int j = i%2 ? slices-k-1 : k;

            *curindex++ = i*(slices+1)+j;
            *curindex++ = (i+1)*(slices+1)+j;
            *curindex++ = i*(slices+1)+j+1;

            *curindex++ = i*(slices+1)+j+1;
            *curindex++ = (i+1)*(slices+1)+j;
            *curindex++ = (i+1)*(slices+1)+j+1;
        }
    }

    if(hasVBO)
    {
        if(!spherevbuf) glGenBuffers_(1, &spherevbuf);
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, spherevbuf);
        glBufferData_(GL_ARRAY_BUFFER_ARB, spherenumverts*sizeof(spherevert), sphereverts, GL_STATIC_DRAW_ARB);
        DELETEA(sphereverts);

        if(!sphereebuf) glGenBuffers_(1, &sphereebuf);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, sphereebuf);
        glBufferData_(GL_ELEMENT_ARRAY_BUFFER_ARB, spherenumindices*sizeof(GLushort), sphereindices, GL_STATIC_DRAW_ARB);
        DELETEA(sphereindices);
    }
}

VARP(explosion2d, 0, 0, 1);

static void setupexplosion()
{
    if(!expmodtex[0]) expmodtex[0] = createexpmodtex(64, 0);
    if(!expmodtex[1]) expmodtex[1] = createexpmodtex(64, 0.25f);
    lastexpmodtex = 0;

    if(softparticles && softexplosion)
    {
        if(explosion2d) SETSHADER(explosion2dsoft); else SETSHADER(explosion3dsoft);
    }
    else if(explosion2d) SETSHADER(explosion2d); else SETSHADER(explosion3d);

    if(explosion2d)
    {
        if(!hemiverts && !hemivbuf) inithemisphere(5, 2);
        if(hasVBO)
        {
            glBindBuffer_(GL_ARRAY_BUFFER_ARB, hemivbuf);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, hemiebuf);
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(vec), hemiverts);
    }
    else
    {
        if(!sphereverts && !spherevbuf) initsphere(12, 6);

        if(hasVBO)
        {
            glBindBuffer_(GL_ARRAY_BUFFER_ARB, spherevbuf);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, sphereebuf);
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(spherevert), &sphereverts->pos);
        glTexCoordPointer(2, GL_FLOAT, sizeof(spherevert), &sphereverts->s);
    }
}

static void drawexpverts(int numverts, int numindices, GLushort *indices)
{
    if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, 0, numverts-1, numindices, GL_UNSIGNED_SHORT, indices);
    else glDrawElements(GL_TRIANGLES, numindices, GL_UNSIGNED_SHORT, indices);
    xtraverts += numindices;
    glde++;
}

static void drawexplosion(bool inside, float r, float g, float b, float a)
{
    if(lastexpmodtex != expmodtex[inside ? 1 : 0])
    {
        glActiveTexture_(GL_TEXTURE1_ARB);
        lastexpmodtex = expmodtex[inside ? 1 :0];
        glBindTexture(GL_TEXTURE_2D, lastexpmodtex);
        glActiveTexture_(GL_TEXTURE0_ARB);
    }
    int passes = inside ? 2 : 1;
    if(!explosion2d)
    {
        if(inside) glScalef(1, 1, -1);
        loopi(passes)
        {
            glColor4f(r, g, b, i ? a/2 : a);
            if(i) glDepthFunc(GL_GEQUAL);
            drawexpverts(spherenumverts, spherenumindices, sphereindices);
            if(i) glDepthFunc(GL_LESS);
        }
        return;
    }
    loopi(passes)
    {
        glColor4f(r, g, b, i ? a/2 : a);
        if(i)
        {
            glScalef(1, 1, -1);
            glDepthFunc(GL_GEQUAL);
        }
        if(inside)
        {
            if(passes >= 2)
            {
                glCullFace(GL_FRONT);
                drawexpverts(heminumverts, heminumindices, hemiindices);
                glCullFace(GL_BACK);
            }
            glScalef(1, 1, -1);
        }
        drawexpverts(heminumverts, heminumindices, hemiindices);
        if(i) glDepthFunc(GL_LESS);
    }
}

static void cleanupexplosion()
{
    glDisableClientState(GL_VERTEX_ARRAY);
    if(!explosion2d) glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
}

static void deleteexplosions()
{
    loopi(2) if(expmodtex[i]) { glDeleteTextures(1, &expmodtex[i]); expmodtex[i] = 0; }
    if(hemivbuf) { glDeleteBuffers_(1, &hemivbuf); hemivbuf = 0; }
    if(hemiebuf) { glDeleteBuffers_(1, &hemiebuf); hemiebuf = 0; }
    DELETEA(hemiverts);
    DELETEA(hemiindices);
    if(spherevbuf) { glDeleteBuffers_(1, &spherevbuf); spherevbuf = 0; }
    if(sphereebuf) { glDeleteBuffers_(1, &sphereebuf); sphereebuf = 0; }
    DELETEA(sphereverts);
    DELETEA(sphereindices);
}

static const float WOBBLE = 1.25f;

struct fireballrenderer : listrenderer
{
    fireballrenderer(const char *texname)
        : listrenderer(texname, 0, PT_FIREBALL|PT_SHADER)
    {}

    void startrender()
    {
        setupexplosion();
    }

    void endrender()
    {
        cleanupexplosion();
    }

    void cleanup()
    {
        deleteexplosions();
    }

    void seedemitter(particleemitter &pe, const vec &o, const vec &d, int fade, float size, int gravity)
    {
        pe.maxfade = max(pe.maxfade, fade);
        pe.extendbb(o, (size+1+pe.ent->attr[1])*WOBBLE);
    }

    void renderpart(listparticle *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
    {
        float pmax = p->val,
              size = p->fade ? float(ts)/p->fade : 1,
              psize = p->size + pmax * size;

        if(isfoggedsphere(psize*WOBBLE, p->o)) return;

        glPushMatrix();
        glTranslatef(o.x, o.y, o.z);

        bool inside = o.dist(camera1->o) <= psize*WOBBLE;
        vec oc(o);
        oc.sub(camera1->o);

        float yaw = inside ? camera1->yaw : atan2(oc.y, oc.x)/RAD - 90,
        pitch = (inside ? camera1->pitch : asin(oc.z/oc.magnitude())/RAD) - 90;
        vec rotdir;
        if(explosion2d)
        {
            glRotatef(yaw, 0, 0, 1);
            glRotatef(pitch, 1, 0, 0);
            rotdir = vec(0, 0, 1);
        }
        else
        {
            vec s(1, 0, 0), t(0, 1, 0);
            s.rotate(pitch*RAD, vec(-1, 0, 0));
            s.rotate(yaw*RAD, vec(0, 0, -1));
            t.rotate(pitch*RAD, vec(-1, 0, 0));
            t.rotate(yaw*RAD, vec(0, 0, -1));

            rotdir = vec(-1, 1, -1).normalize();
            s.rotate(-lastmillis/7.0f*RAD, rotdir);
            t.rotate(-lastmillis/7.0f*RAD, rotdir);

            LOCALPARAM(texgenS, (0.5f*s.x, 0.5f*s.y, 0.5f*s.z, 0.5f));
            LOCALPARAM(texgenT, (0.5f*t.x, 0.5f*t.y, 0.5f*t.z, 0.5f));
        }

        LOCALPARAM(center, (o));
        LOCALPARAM(animstate, (size, psize, pmax, float(lastmillis)));
        if(2*(p->size + pmax)*WOBBLE >= softexplosionblend)
        {
            LOCALPARAM(softparams, (-1.0f/softexplosionblend, 0, inside ? blend/(2*255.0f) : 0));
        }
        else
        {
            LOCALPARAM(softparams, (0, -1, inside ? blend/(2*255.0f) : 0));
        }

        glRotatef(lastmillis/7.0f, -rotdir.x, rotdir.y, -rotdir.z);
        glScalef(-psize, psize, -psize);
        drawexplosion(inside, color[0]*ldrscaleb, color[1]*ldrscaleb, color[2]*ldrscaleb, blend/255.0f);

        glPopMatrix();
    }
};
static fireballrenderer fireballs("packages/particles/explosion"), bluefireballs("<mad:.6,.8,1>packages/particles/explosion");

