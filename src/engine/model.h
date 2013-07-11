enum { MDL_MD3 = 0, MDL_MD5, MDL_OBJ, MDL_SMD, MDL_IQM, NUMMODELTYPES };

struct model
{
    float spinyaw, spinpitch, spinroll, offsetyaw, offsetpitch, offsetroll;
    bool collide, ellipsecollide, shadow, alphashadow, depthoffset;
    float scale;
    vec translate;
    BIH *bih;
    vec bbcenter, bbradius, bbextend;
    float eyeheight, collideradius, collideheight;
    int batch;

    model() : spinyaw(0), spinpitch(0), spinroll(0), offsetyaw(0), offsetpitch(0), offsetroll(0), collide(true), ellipsecollide(false), shadow(true), alphashadow(true), depthoffset(false), scale(1.0f), translate(0, 0, 0), bih(0), bbcenter(0, 0, 0), bbradius(-1, -1, -1), bbextend(0, 0, 0), eyeheight(0.9f), collideradius(0), collideheight(0), batch(-1) {}

    virtual ~model() { DELETEP(bih); }
    virtual void calcbb(vec &center, vec &radius) = 0;
    virtual int intersect(int anim, int basetime, int basetime2, const vec &pos, float yaw, float pitch, float roll, dynent *d, modelattach *a, float size, const vec &o, const vec &ray, float &dist, int mode) = 0;
    virtual void render(int anim, int basetime, int basetime2, const vec &o, float yaw, float pitch, float roll, dynent *d, modelattach *a = NULL, float size = 1, float trans = 1) = 0;
    virtual bool load() = 0;
    virtual const char *name() const = 0;
    virtual int type() const = 0;
    virtual BIH *setBIH() { return 0; }
    virtual bool envmapped() const { return false; }
    virtual bool skeletal() const { return false; }
    virtual bool animated() const { return false; }
    virtual bool pitched() const { return true; }
    virtual bool alphatested() const { return false; }

    virtual void setshader(Shader *shader) {}
    virtual void setenvmap(float envmapmin, float envmapmax, Texture *envmap) {}
    virtual void setspec(float spec) {}
    virtual void setambient(float ambient) {}
    virtual void setglow(float glow, float glowdelta, float glowpulse) {}
    virtual void setalphatest(float alpha) {}
    virtual void setfullbright(float fullbright) {}
    virtual void setcullface(bool cullface) {}

    virtual void genshadowmesh(vector<vec> &tris, const matrix3x4 &orient) {}
    virtual void preloadBIH() { if(!bih) setBIH(); }
    virtual void preloadshaders() {}
    virtual void preloadmeshes() {}
    virtual void cleanup() {}

    virtual void startrender() {}
    virtual void endrender() {}

    void boundbox(vec &center, vec &radius)
    {
        if(bbradius.x < 0) calcbb(bbcenter, bbradius);
        center = bbcenter;
        radius = vec(bbradius).add(bbextend);
    }

    void collisionbox(vec &center, vec &radius)
    {
        boundbox(center, radius);
        if(collideradius)
        {
            center.x = center.y = 0;
            radius.x = radius.y = collideradius;
        }
        if(collideheight)
        {
            center.z = radius.z = collideheight/2;
        }
    }

    float boundsphere(vec &center)
    {
        vec radius;
        boundbox(center, radius);
        return radius.magnitude();
    }

    float above()
    {
        vec center, radius;
        boundbox(center, radius);
        return center.z+radius.z;
    }
};

