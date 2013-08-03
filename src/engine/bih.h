struct BIHNode
{
    short split[2];
    ushort child[2];

    int axis() const { return child[0]>>14; }
    int childindex(int which) const { return child[which]&0x3FFF; }
    bool isleaf(int which) const { return (child[1]&(1<<(14+which)))!=0; }
};

struct BIH
{
    struct tri : triangle
    {
        float tc[6];
        Texture *tex;
    };

    int maxdepth;
    int numnodes;
    BIHNode *nodes;
    int numtris;
    tri *tris, *noclip;

    vec bbmin, bbmax;
    float radius;

    BIH(vector<tri> *t);

    ~BIH()
    {
        DELETEA(nodes);
        DELETEA(tris);
    }

    void build(vector<BIHNode> &buildnodes, ushort *indices, int numindices, const vec &vmin, const vec &vmax, int depth = 1);

    bool traverse(const vec &o, const vec &ray, float maxdist, float &dist, int mode);
    bool traverse(const vec &o, const vec &ray, const vec &invray, float maxdist, float &dist, int mode, BIHNode *curnode, float tmin, float tmax);
    bool triintersect(const tri &t, const vec &o, const vec &ray, float maxdist, float &dist, int mode);

    bool boxcollide(physent *d, const vec &dir, float cutoff, const vec &o, int yaw, int pitch, int roll, float scale = 1);
    bool ellipsecollide(physent *d, const vec &dir, float cutoff, const vec &o, int yaw, int pitch, int roll, float scale = 1);

    template<int C, class M>
    void collide(physent *d, const vec &dir, float cutoff, const vec &center, const vec &radius, const M &orient, float &dist, BIHNode *curnode, const vec &bo, const vec &br);
    template<int C, class M>
    void tricollide(const tri &t, physent *d, const vec &dir, float cutoff, const vec &center, const vec &radius, const M &orient, float &dist, const vec &bo, const vec &br);

    void preload();
};

extern bool mmintersect(const extentity &e, const vec &o, const vec &ray, float maxdist, int mode, float &dist);

