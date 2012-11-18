// shader.cpp: OpenGL GLSL shader management

#include "engine.h"

Shader *Shader::lastshader = NULL;

Shader *nullshader = NULL, *defaultshader = NULL, *rectshader = NULL, *cubemapshader = NULL, *notextureshader = NULL, *nocolorshader = NULL, *foggedshader = NULL, *foggednotextureshader = NULL, *ldrshader = NULL, *ldrnotextureshader = NULL, *stdworldshader = NULL, *rsmworldshader = NULL;

static hashtable<const char *, GlobalShaderParamState> globalparams(256);
static hashtable<const char *, int> localparams(256);
static hashtable<const char *, Shader> shaders(256);
static Shader *slotshader = NULL;
static vector<SlotShaderParam> slotparams;
static bool standardshader = false, forceshaders = true;

VAR(maxtexcoords, 1, 0, 0);
VAR(maxvsuniforms, 1, 0, 0);
VAR(maxfsuniforms, 1, 0, 0);
VAR(maxvaryings, 1, 0, 0);
//VAR(dbgshader, 0, 0, 2);
VAR(dbgshader, 0, 1, 2);

void loadshaders()
{
    standardshader = true;
    execfile("data/glsl.cfg");
    standardshader = false;

    nullshader = lookupshaderbyname("null");
    defaultshader = lookupshaderbyname("default");
    stdworldshader = lookupshaderbyname("stdworld");
    if(!nullshader || !defaultshader || !stdworldshader) fatal("cannot find shader definitions");

    dummyslot.shader = stdworldshader;

    rectshader = lookupshaderbyname("rect");
    cubemapshader = lookupshaderbyname("cubemap");
    notextureshader = lookupshaderbyname("notexture");
    nocolorshader = lookupshaderbyname("nocolor");
    foggedshader = lookupshaderbyname("fogged");
    foggednotextureshader = lookupshaderbyname("foggednotexture");
    ldrshader = lookupshaderbyname("ldr");
    ldrnotextureshader = lookupshaderbyname("ldrnotexture");
    rsmworldshader = lookupshaderbyname("rsmworld");

    defaultshader->set();
}

Shader *lookupshaderbyname(const char *name)
{
    Shader *s = shaders.access(name);
    return s && s->detailshader ? s : NULL;
}

Shader *generateshader(const char *name, const char *fmt, ...)
{
    Shader *s = name ? lookupshaderbyname(name) : NULL;
    if(!s)
    {
        defvformatstring(cmd, fmt, fmt);
        standardshader = true;
        execute(cmd);
        standardshader = false;
        s = name ? lookupshaderbyname(name) : NULL;
        if(!s) s = nullshader;
    }
    return s;
}

static void showglslinfo(GLenum type, GLuint obj, const char *name, const char *source)
{
    GLint length = 0;
    if(type) glGetShaderiv_(obj, GL_INFO_LOG_LENGTH, &length);
    else glGetProgramiv_(obj, GL_INFO_LOG_LENGTH, &length);
    if(length > 1)
    {
        conoutf(CON_ERROR, "GLSL ERROR (%s:%s)", type == GL_VERTEX_SHADER ? "VS" : (type == GL_FRAGMENT_SHADER ? "FS" : "PROG"), name);
        FILE *l = getlogfile();
        if(l)
        {
            GLchar *log = new GLchar[length];
            if(type) glGetShaderInfoLog_(obj, length, &length, log);
            else glGetProgramInfoLog_(obj, length, &length, log);
            fprintf(l, "%s\n", log);
            if(source) loopi(1000)
            {
                const char *next = strchr(source, '\n');
                fprintf(l, "%d: ", i+1);
                fwrite(source, 1, next ? next - source + 1 : strlen(source), l);
                if(!next) { fputc('\n', l); break; }
                source = next + 1;
            }
            delete[] log;
        }
    }
}

static void compileglslshader(GLenum type, GLuint &obj, const char *def, const char *name, bool msg = true)
{
    const GLchar *source = (const GLchar *)(def + strspn(def, " \t\r\n"));
    obj = glCreateShader_(type);
    glShaderSource_(obj, 1, &source, NULL);
    glCompileShader_(obj);
    GLint success;
    glGetShaderiv_(obj, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        if(msg) showglslinfo(type, obj, name, source);
        glDeleteShader_(obj);
        obj = 0;
    }
    else if(dbgshader > 1 && msg) showglslinfo(type, obj, name, source);
}

VAR(dbgubo, 0, 0, 1);

static void bindglsluniform(Shader &s, UniformLoc &u)
{
    u.loc = glGetUniformLocation_(s.program, u.name);
    if(!u.blockname) return;
    if(hasUBO)
    {
        GLuint bidx = glGetUniformBlockIndex_(s.program, u.blockname);
        GLuint uidx = GL_INVALID_INDEX;
        glGetUniformIndices_(s.program, 1, &u.name, &uidx);
        if(bidx != GL_INVALID_INDEX && uidx != GL_INVALID_INDEX)
        {
            GLint sizeval = 0, offsetval = 0, strideval = 0;
            glGetActiveUniformBlockiv_(s.program, bidx, GL_UNIFORM_BLOCK_DATA_SIZE, &sizeval);
            if(sizeval <= 0) return;
            glGetActiveUniformsiv_(s.program, 1, &uidx, GL_UNIFORM_OFFSET, &offsetval);
            if(u.stride > 0)
            {
                glGetActiveUniformsiv_(s.program, 1, &uidx, GL_UNIFORM_ARRAY_STRIDE, &strideval);
                if(strideval > u.stride) return;
            }
            u.offset = offsetval;
            u.size = sizeval;
            glUniformBlockBinding_(s.program, bidx, u.binding);
            if(dbgubo) conoutf(CON_DEBUG, "UBO: %s:%s:%d, offset: %d, size: %d, stride: %d", u.name, u.blockname, u.binding, offsetval, sizeval, strideval);
        }
    }
    else if(hasBUE)
    {
        GLint size = glGetUniformBufferSize_(s.program, u.loc), stride = 0;
        if(size <= 0) return;
        if(u.stride > 0)
        {
            defformatstring(elem1name)("%s[1]", u.name);
            GLint elem1loc = glGetUniformLocation_(s.program, elem1name);
            if(elem1loc == -1) return;
            GLintptr elem0off = glGetUniformOffset_(s.program, u.loc),
                     elem1off = glGetUniformOffset_(s.program, elem1loc);
            stride = elem1off - elem0off;
            if(stride > u.stride) return;
        }
        u.offset = 0;
        u.size = size;
        if(dbgubo) conoutf(CON_DEBUG, "BUE: %s:%s:%d, offset: %d, size: %d, stride: %d", u.name, u.blockname, u.binding, 0, size, stride);
    }
}

static void linkglslprogram(Shader &s, bool msg = true)
{
    s.program = s.vsobj && s.psobj ? glCreateProgram_() : 0;
    GLint success = 0;
    if(s.program)
    {
        glAttachShader_(s.program, s.vsobj);
        glAttachShader_(s.program, s.psobj);
        loopv(s.attriblocs)
        {
            AttribLoc &a = s.attriblocs[i];
            glBindAttribLocation_(s.program, a.loc, a.name);
        }
        glLinkProgram_(s.program);
        glGetProgramiv_(s.program, GL_LINK_STATUS, &success);
    }
    if(success)
    {
        glUseProgram_(s.program);
        loopi(16)
        {
            static const char * const texnames[16] = { "tex0", "tex1", "tex2", "tex3", "tex4", "tex5", "tex6", "tex7", "tex8", "tex9", "tex10", "tex11", "tex12", "tex13", "tex14", "tex15" };
            GLint loc = glGetUniformLocation_(s.program, texnames[i]);
            if(loc != -1) glUniform1i_(loc, i);
        }
        loopv(s.defaultparams)
        {
            SlotShaderParamState &param = s.defaultparams[i];
            param.loc = glGetUniformLocation_(s.program, param.name);
        }
        loopv(s.uniformlocs) bindglsluniform(s, s.uniformlocs[i]);
        glUseProgram_(0);
    }
    else if(s.program)
    {
        if(msg) showglslinfo(GL_FALSE, s.program, s.name, NULL);
        glDeleteProgram_(s.program);
        s.program = 0;
    }
}

bool checkglslsupport()
{
    const GLchar *vsstr =
        "void main(void) {\n"
        "    gl_Position = ftransform();\n"
        "}\n";
#if 0
    /* check if GLSL profile supports loops
     */
    const GLchar *psstr =
        "uniform int N;\n"
        "uniform vec4 delta;\n"
        "void main(void) {\n"
        "   vec4 test = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "   for(int i = 0; i < N; i++)  test += delta;\n"
        "   gl_FragColor = test;\n"
        "}\n";
#else
    const GLchar *psstr =
        "void main(void) {\n"
        "   gl_FragColor = vec4(0.0);\n"
        "}\n";
#endif
    GLuint vsobj = glCreateShader_(GL_VERTEX_SHADER), psobj = glCreateShader_(GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram_();
    GLint success = 0;
    if(vsobj && psobj && program)
    {
        glShaderSource_(vsobj, 1, &vsstr, NULL);
        glCompileShader_(vsobj);
        glGetShaderiv_(vsobj, GL_COMPILE_STATUS, &success);
        if(success)
        {
            glShaderSource_(psobj, 1, &psstr, NULL);
            glCompileShader_(psobj);
            glGetShaderiv_(psobj, GL_COMPILE_STATUS, &success);
            if(success)
            {
                glAttachShader_(program, vsobj);
                glAttachShader_(program, psobj);
                glLinkProgram_(program);
                glGetProgramiv_(program, GL_LINK_STATUS, &success);
            }
        }
    }
    if(vsobj) glDeleteShader_(vsobj);
    if(psobj) glDeleteShader_(psobj);
    if(program) glDeleteProgram_(program);
    return success!=0;
}

int getlocalparam(const char *name)
{
    return localparams.access(name, int(localparams.numelems));
}

static int addlocalparam(Shader &s, const char *name, int loc, int size, GLenum format)
{
    int idx = getlocalparam(name);
    if(idx >= s.localparamremap.length())
    {
        int n = idx + 1 - s.localparamremap.length();
        memset(s.localparamremap.pad(n), 0xFF, n);
    }
    s.localparamremap[idx] = s.localparams.length();

    LocalShaderParamState &l = s.localparams.add();
    l.name = name;
    l.loc = loc;
    l.size = size;
    l.format = format;
    return idx;
}

GlobalShaderParamState *getglobalparam(const char *name)
{
    GlobalShaderParamState *param = globalparams.access(name);
    if(!param)
    {
        param = &globalparams[name];
        param->name = name;
        memset(param->val, -1, sizeof(param->val));
        param->version = -1;
    }
    return param;
}

static GlobalShaderParamUse *addglobalparam(Shader &s, GlobalShaderParamState *param, int loc, int size, GLenum format)
{
    GlobalShaderParamUse &g = s.globalparams.add();
    g.param = param;
    g.version = -2;
    g.loc = loc;
    g.size = size;
    g.format = format;
    return &g;
}

static void setglsluniformformat(Shader &s, const char *name, GLenum format, int size)
{
    switch(format)
    {
        case GL_FLOAT:
        case GL_FLOAT_VEC2:
        case GL_FLOAT_VEC3:
        case GL_FLOAT_VEC4:
        case GL_FLOAT_MAT2:
        case GL_FLOAT_MAT3:
        case GL_FLOAT_MAT4:
            break;
        default:
            return;
    }
    if(!strncmp(name, "gl_", 3)) return;

    int loc = glGetUniformLocation_(s.program, name);
    if(loc < 0) return;
    loopvj(s.defaultparams) if(s.defaultparams[j].loc == loc)
    {
        s.defaultparams[j].format = format;
        return;
    }
    loopvj(s.uniformlocs) if(s.uniformlocs[j].loc == loc) return;
    loopvj(s.globalparams) if(s.globalparams[j].loc == loc) return;
    loopvj(s.localparams) if(s.localparams[j].loc == loc) return;

    name = getshaderparamname(name);
    GlobalShaderParamState *param = globalparams.access(name);
    if(param) addglobalparam(s, param, loc, size, format);
    else addlocalparam(s, name, loc, size, format);
}

static void allocglslactiveuniforms(Shader &s)
{
    GLint numactive = 0;
    glGetProgramiv_(s.program, GL_ACTIVE_UNIFORMS, &numactive);
    string name;
    loopi(numactive)
    {
        GLsizei namelen = 0;
        GLint size = 0;
        GLenum format = GL_FLOAT_VEC4;
        name[0] = '\0';
        glGetActiveUniform_(s.program, i, sizeof(name)-1, &namelen, &size, &format, name);
        if(namelen <= 0 || size <= 0) continue;
        name[clamp(int(namelen), 0, (int)sizeof(name)-2)] = '\0';
        char *brak = strchr(name, '[');
        if(brak) *brak = '\0';
        setglsluniformformat(s, name, format, size);
    }
}

void Shader::allocparams(Slot *slot)
{
    if(slot)
    {
#define UNIFORMTEX(name, tmu) \
        do { \
            loc = glGetUniformLocation_(program, name); \
            int val = tmu; \
            if(loc != -1) glUniform1i_(loc, val); \
        } while(0)
        int loc, tmu = 1;
        if(type & SHADER_ENVMAP) UNIFORMTEX("envmap", tmu++);
        if(type & SHADER_REFRACT)
        {
            UNIFORMTEX("refractmask", 7);
            UNIFORMTEX("refractlight", 8);
        }
        UNIFORMTEX("blendmap", 7);
        int stex = 0;
        loopv(slot->sts)
        {
            Slot::Tex &t = slot->sts[i];
            switch(t.type)
            {
                case TEX_DIFFUSE: UNIFORMTEX("diffusemap", 0); break;
                case TEX_NORMAL: UNIFORMTEX("normalmap", tmu++); break;
                case TEX_GLOW: UNIFORMTEX("glowmap", tmu++); break;
                case TEX_DECAL: UNIFORMTEX("decal", tmu++); break;
                case TEX_SPEC: if(t.combined<0) UNIFORMTEX("specmap", tmu++); break;
                case TEX_DEPTH: if(t.combined<0) UNIFORMTEX("depthmap", tmu++); break;
                case TEX_UNKNOWN:
                {
                    defformatstring(sname)("stex%d", stex++);
                    UNIFORMTEX(sname, tmu++);
                    break;
                }
            }
        }
    }
    allocglslactiveuniforms(*this);
}

int GlobalShaderParamState::nextversion = 0;

void GlobalShaderParamState::resetversions()
{
    enumerate(shaders, Shader, s,
    {
        loopv(s.globalparams)
        {
            GlobalShaderParamUse &u = s.globalparams[i];
            if(u.version != u.param->version) u.version = -2;
        }
    });
    nextversion = 0;
    enumerate(globalparams, GlobalShaderParamState, g, { g.version = ++nextversion; });
    enumerate(shaders, Shader, s,
    {
        loopv(s.globalparams)
        {
            GlobalShaderParamUse &u = s.globalparams[i];
            if(u.version >= 0) u.version = u.param->version;
        }
    });
}

static inline void setslotparam(SlotShaderParamState &l, uint &mask, int i, const float *val)
{
    if(!(mask&(1<<i)))
    {
        mask |= 1<<i;
        switch(l.format)
        {
            case GL_FLOAT:      glUniform1fv_(l.loc, 1, val); break;
            case GL_FLOAT_VEC2: glUniform2fv_(l.loc, 1, val); break;
            case GL_FLOAT_VEC3: glUniform3fv_(l.loc, 1, val); break;
            case GL_FLOAT_VEC4: glUniform4fv_(l.loc, 1, val); break;
        }
    }
}

void Shader::setslotparams(Slot &slot, VSlot &vslot)
{
    uint unimask = 0;
    loopv(vslot.params)
    {
        SlotShaderParam &p = vslot.params[i];
        if(!defaultparams.inrange(p.loc)) continue;
        SlotShaderParamState &l = defaultparams[p.loc];
        setslotparam(l, unimask, p.loc, p.val);
    }
    loopv(slot.params)
    {
        SlotShaderParam &p = slot.params[i];
        if(!defaultparams.inrange(p.loc)) continue;
        SlotShaderParamState &l = defaultparams[p.loc];
        setslotparam(l, unimask, p.loc, p.val);
    }
    loopv(defaultparams)
    {
        SlotShaderParamState &l = defaultparams[i];
        setslotparam(l, unimask, i, l.val);
    }
}

void Shader::bindprograms()
{
    if(this == lastshader || type&(SHADER_DEFERRED|SHADER_INVALID)) return;
    glUseProgram_(program);
    lastshader = this;
}

bool Shader::compile()
{
    if(!vsstr) vsobj = !reusevs || reusevs->type&SHADER_INVALID ? 0 : reusevs->vsobj;
    else compileglslshader(GL_VERTEX_SHADER,   vsobj, vsstr, name, dbgshader || !variantshader);
    if(!psstr) psobj = !reuseps || reuseps->type&SHADER_INVALID ? 0 : reuseps->psobj;
    else compileglslshader(GL_FRAGMENT_SHADER, psobj, psstr, name, dbgshader || !variantshader);
    linkglslprogram(*this, !variantshader);
    return program!=0;
}

void Shader::cleanup(bool invalid)
{
    detailshader = NULL;
    used = false;
    native = true;
    if(vsobj) { if(reusevs) glDeleteShader_(vsobj); vsobj = 0; }
    if(psobj) { if(reuseps) glDeleteShader_(psobj); psobj = 0; }
    if(program) { glDeleteProgram_(program); program = 0; }
    localparams.setsize(0);
    localparamremap.setsize(0);
    globalparams.setsize(0);
    if(standard || invalid)
    {
        type = SHADER_INVALID;
        loopi(MAXVARIANTROWS) variants[i].setsize(0);
        DELETEA(vsstr);
        DELETEA(psstr);
        DELETEA(defer);
        defaultparams.setsize(0);
        attriblocs.setsize(0);
        uniformlocs.setsize(0);
        altshader = NULL;
        loopi(MAXSHADERDETAIL) fastshader[i] = this;
        reusevs = reuseps = NULL;
    }
    else loopv(defaultparams) defaultparams[i].loc = -1;
}

static void genattriblocs(Shader &s, const char *vs, const char *ps)
{
    static int len = strlen("#pragma CUBE2_attrib");
    string name;
    int loc;
    while((vs = strstr(vs, "#pragma CUBE2_attrib")))
    {
        if(sscanf(vs, "#pragma CUBE2_attrib %100s %d", name, &loc) == 2)
            s.attriblocs.add(AttribLoc(getshaderparamname(name), loc));
        vs += len;
    }
}

static void genuniformlocs(Shader &s, const char *vs, const char *ps)
{
    static int len = strlen("#pragma CUBE2_uniform");
    string name, blockname;
    int binding, stride;
    while((vs = strstr(vs, "#pragma CUBE2_uniform")))
    {
        int numargs = sscanf(vs, "#pragma CUBE2_uniform %100s %100s %d %d", name, blockname, &binding, &stride);
        if(numargs >= 3) s.uniformlocs.add(UniformLoc(getshaderparamname(name), getshaderparamname(blockname), binding, numargs >= 4 ? stride : 0));
        else if(numargs >= 1) s.uniformlocs.add(UniformLoc(getshaderparamname(name)));
        vs += len;
    }
}

Shader *newshader(int type, const char *name, const char *vs, const char *ps, Shader *variant = NULL, int row = 0)
{
    if(Shader::lastshader)
    {
        glUseProgram_(0);
        Shader::lastshader = NULL;
    }

    Shader *exists = shaders.access(name);
    char *rname = exists ? exists->name : newstring(name);
    Shader &s = shaders[rname];
    s.name = rname;
    s.vsstr = newstring(vs);
    s.psstr = newstring(ps);
    DELETEA(s.defer);
    s.type = type;
    s.variantshader = variant;
    s.standard = standardshader;
    if(forceshaders) s.forced = true;
    s.reusevs = s.reuseps = NULL;
    if(variant)
    {
        int row = 0, col = 0;
        if(!vs[0] || sscanf(vs, "%d , %d", &row, &col) >= 1)
        {
            DELETEA(s.vsstr);
            s.reusevs = !vs[0] ? variant : (variant->variants[row].inrange(col) ? variant->variants[row][col] : NULL);
        }
        row = col = 0;
        if(!ps[0] || sscanf(ps, "%d , %d", &row, &col) >= 1)
        {
            DELETEA(s.psstr);
            s.reuseps = !ps[0] ? variant : (variant->variants[row].inrange(col) ? variant->variants[row][col] : NULL);
        }
    }
    if(variant) loopv(variant->defaultparams) s.defaultparams.add(variant->defaultparams[i]);
    else loopv(slotparams) s.defaultparams.add(slotparams[i]);
    s.attriblocs.setsize(0);
    s.uniformlocs.setsize(0);
    genattriblocs(s, vs, ps);
    genuniformlocs(s, vs, ps);
    if(!s.compile())
    {
        s.cleanup(true);
        if(variant) shaders.remove(rname);
        return NULL;
    }
    if(variant) variant->variants[row].add(&s);
    s.fixdetailshader();
    return &s;
}

void setupshaders()
{
    GLint val;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &val);
    maxvsuniforms = val/4;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &val);
    maxfsuniforms = val/4;
    glGetIntegerv(GL_MAX_VARYING_FLOATS, &val);
    maxvaryings = val;
    glGetIntegerv(GL_MAX_TEXTURE_COORDS, &val);
    maxtexcoords = val;

    standardshader = true;
    nullshader = newshader(0, "<init>null",
        "void main(void) {\n"
        "   gl_Position = gl_Vertex;\n"
        "}\n",
        "void main(void) {\n"
        "   gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);\n"
        "}\n");
    defaultshader = newshader(0, "<init>default",
        "void main(void) {\n"
        "    gl_Position = ftransform();\n"
        "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "    gl_FrontColor = gl_Color;\n"
        "}\n",
        "uniform sampler2D tex0;\n"
        "void main(void) {\n"
        "    gl_FragColor = gl_Color * texture2D(tex0, gl_TexCoord[0].xy);\n"
        "}\n");
    notextureshader = newshader(0, "<init>notexture",
        "void main(void) {\n"
        "    gl_Position = ftransform();\n"
        "    gl_FrontColor = gl_Color;\n"
        "}\n",
        "void main(void) {\n"
        "    gl_FragColor = gl_Color;\n"
        "}\n");
    standardshader = false;

    if(!nullshader || !defaultshader || !notextureshader) fatal("failed to setup shaders");

    dummyslot.shader = nullshader;
}

static const char *findglslmain(const char *s)
{
    const char *main = strstr(s, "main");
    if(!main) return NULL;
    for(; main >= s; main--) switch(*main) { case '\r': case '\n': case ';': return main + 1; }
    return s;
}

static void gengenericvariant(Shader &s, const char *sname, const char *vs, const char *ps, int row)
{
    bool vschanged = false, pschanged = false;
    vector<char> vsv, psv;
    vsv.put(vs, strlen(vs)+1);
    psv.put(ps, strlen(ps)+1);

    static const int len = strlen("#pragma CUBE2_variant"), olen = strlen("override");
    for(char *vspragma = vsv.getbuf();; vschanged = true)
    {
        vspragma = strstr(vspragma, "#pragma CUBE2_variant");
        if(!vspragma) break;
        memset(vspragma, ' ', len);
        vspragma += len;
        if(!strncmp(vspragma, "override", olen))
        {
            memset(vspragma, ' ', olen);
            vspragma += olen;
            char *end = vspragma + strcspn(vspragma, "\n\r");
            int endlen = strspn(end, "\n\r");
            memset(end, ' ', endlen);
        }
    }
    for(char *pspragma = psv.getbuf();; pschanged = true)
    {
        pspragma = strstr(pspragma, "#pragma CUBE2_variant");
        if(!pspragma) break;
        memset(pspragma, ' ', len);
        pspragma += len;
        if(!strncmp(pspragma, "override", olen))
        {
            memset(pspragma, ' ', olen);
            pspragma += olen;
            char *end = pspragma + strcspn(pspragma, "\n\r");
            int endlen = strspn(end, "\n\r");
            memset(end, ' ', endlen);
        }
    }
    defformatstring(varname)("<variant:%d,%d>%s", s.variants[row].length(), row, sname);
    defformatstring(reuse)("%d", row);
    newshader(s.type, varname, vschanged ? vsv.getbuf() : reuse, pschanged ? psv.getbuf() : reuse, &s, row);
}

static void genfogshader(vector<char> &vsbuf, vector<char> &psbuf, const char *vs, const char *ps)
{
    const char *vspragma = strstr(vs, "#pragma CUBE2_fog"), *pspragma = strstr(ps, "#pragma CUBE2_fog");
    if(!vspragma && !pspragma) return;
    static const int pragmalen = strlen("#pragma CUBE2_fog");
    const char *vsmain = findglslmain(vs), *vsend = strrchr(vs, '}');
    if(vsmain && vsend)
    {
        vsbuf.put(vs, vsmain - vs);
        const char *fogparams = "\nvarying float fogcoord;\n";
        vsbuf.put(fogparams, strlen(fogparams));
        vsbuf.put(vsmain, vsend - vsmain);
        const char *vsdef = "\n#define FOG_COORD ";
        const char *vsfog = "\nfogcoord = -dot((FOG_COORD), gl_ModelViewMatrixTranspose[2]);\n";
        int clen = 0;
        if(vspragma)
        {
            vspragma += pragmalen;
            while(*vspragma && !iscubespace(*vspragma)) vspragma++;
            vspragma += strspn(vspragma, " \t\v\f");
            clen = strcspn(vspragma, "\r\n");
        }
        if(clen <= 0) { vspragma = "gl_Vertex"; clen = strlen(vspragma); }
        vsbuf.put(vsdef, strlen(vsdef));
        vsbuf.put(vspragma, clen);
        vsbuf.put(vsfog, strlen(vsfog));
        vsbuf.put(vsend, strlen(vsend)+1);
    }
    const char *psmain = findglslmain(ps), *psend = strrchr(ps, '}');
    if(psmain && psend)
    {
        psbuf.put(ps, psmain - ps);
        const char *fogparams = "\nuniform vec3 fogcolor, fogparams;\nvarying float fogcoord;\n";
        psbuf.put(fogparams, strlen(fogparams));
        psbuf.put(psmain, psend - psmain);
        const char *psdef = "\n#define FOG_COLOR ";
        const char *psfog =
            pspragma && !strncmp(pspragma+pragmalen, "rgba", 4) ?
                "\ngl_FragColor = mix((FOG_COLOR), gl_FragColor, clamp((fogparams.y + fogcoord)*fogparams.z, 0.0, 1.0));\n" :
                "\ngl_FragColor.rgb = mix((FOG_COLOR).rgb, gl_FragColor.rgb, clamp((fogparams.y + fogcoord)*fogparams.z, 0.0, 1.0));\n";
        int clen = 0;
        if(pspragma)
        {
            pspragma += pragmalen;
            while(iscubealpha(*pspragma)) pspragma++;
            while(*pspragma && !iscubespace(*pspragma)) pspragma++;
            pspragma += strspn(pspragma, " \t\v\f");
            clen = strcspn(pspragma, "\r\n");
        }
        if(clen <= 0) { pspragma = "fogcolor"; clen = strlen(pspragma); }
        psbuf.put(psdef, strlen(psdef));
        psbuf.put(pspragma, clen);
        psbuf.put(psfog, strlen(psfog));
        psbuf.put(psend, strlen(psend)+1);
    }
}

static void genuniformdefs(vector<char> &vsbuf, vector<char> &psbuf, const char *vs, const char *ps, Shader *variant = NULL)
{
    if(variant ? variant->defaultparams.empty() : slotparams.empty()) return;
    const char *vsmain = findglslmain(vs), *psmain = findglslmain(ps);
    if(!vsmain || !psmain) return;
    vsbuf.put(vs, vsmain - vs);
    psbuf.put(ps, psmain - ps);
    if(variant) loopv(variant->defaultparams)
    {
        defformatstring(uni)("\nuniform vec4 %s;\n", variant->defaultparams[i].name);
        vsbuf.put(uni, strlen(uni));
        psbuf.put(uni, strlen(uni));
    }
    else loopv(slotparams)
    {
        defformatstring(uni)("\nuniform vec4 %s;\n", slotparams[i].name);
        vsbuf.put(uni, strlen(uni));
        psbuf.put(uni, strlen(uni));
    }
    vsbuf.put(vsmain, strlen(vsmain)+1);
    psbuf.put(psmain, strlen(psmain)+1);
}

VAR(defershaders, 0, 1, 1);

void defershader(int *type, const char *name, const char *contents)
{
    Shader *exists = shaders.access(name);
    if(exists && !(exists->type&SHADER_INVALID)) return;
    if(!defershaders) { execute(contents); return; }
    char *rname = exists ? exists->name : newstring(name);
    Shader &s = shaders[rname];
    s.name = rname;
    DELETEA(s.defer);
    s.defer = newstring(contents);
    s.type = SHADER_DEFERRED | *type;
    s.standard = standardshader;
}

void useshader(Shader *s)
{
    if(!(s->type&SHADER_DEFERRED) || !s->defer) return;

    char *defer = s->defer;
    s->defer = NULL;
    bool wasstandard = standardshader, wasforcing = forceshaders;
    int oldflags = identflags;
    standardshader = s->standard;
    forceshaders = false;
    identflags &= ~IDF_PERSIST;
    slotparams.shrink(0);
    execute(defer);
    identflags = oldflags;
    forceshaders = wasforcing;
    standardshader = wasstandard;
    delete[] defer;

    if(s->type&SHADER_DEFERRED)
    {
        DELETEA(s->defer);
        s->type = SHADER_INVALID;
    }
}

void fixshaderdetail()
{
    if(initing) return;
    // must null out separately because fixdetailshader can recursively set it
    enumerate(shaders, Shader, s, { if(!s.forced) s.detailshader = NULL; });
    enumerate(shaders, Shader, s, { if(s.forced) s.fixdetailshader(); });
    linkslotshaders();
}

int Shader::uniformlocversion()
{
    static int version = 0;
    if(++version >= 0) return version;
    version = 0;
    enumerate(shaders, Shader, s, { loopvj(s.uniformlocs) s.uniformlocs[j].version = -1; });
    return version;
}

VARF(nativeshaders, 0, 1, 1, fixshaderdetail());
VARFP(shaderdetail, 0, MAXSHADERDETAIL, MAXSHADERDETAIL, fixshaderdetail());

void Shader::fixdetailshader(bool force, bool recurse)
{
    Shader *alt = this;
    detailshader = NULL;
    do
    {
        Shader *cur = shaderdetail < MAXSHADERDETAIL ? alt->fastshader[shaderdetail] : alt;
        if(cur->type&SHADER_DEFERRED && force) useshader(cur);
        if(!(cur->type&SHADER_INVALID))
        {
            if(cur->type&SHADER_DEFERRED) break;
            detailshader = cur;
            if(cur->native || !nativeshaders) break;
        }
        alt = alt->altshader;
    } while(alt && alt!=this);

    if(recurse && detailshader) loopi(MAXVARIANTROWS) loopvj(detailshader->variants[i]) detailshader->variants[i][j]->fixdetailshader(force, false);
}

Shader *useshaderbyname(const char *name)
{
    Shader *s = shaders.access(name);
    if(!s) return NULL;
    if(!s->detailshader) s->fixdetailshader();
    s->forced = true;
    return s;
}

void shader(int *type, char *name, char *vs, char *ps)
{
    if(lookupshaderbyname(name)) return;

    defformatstring(info)("shader %s", name);
    renderprogress(loadprogress, info);
    vector<char> vsbuf, psbuf, vsbak, psbak;
#define GENSHADER(cond, body) \
    if(cond) \
    { \
        if(vsbuf.length()) { vsbak.setsize(0); vsbak.put(vs, strlen(vs)+1); vs = vsbak.getbuf(); vsbuf.setsize(0); } \
        if(psbuf.length()) { psbak.setsize(0); psbak.put(ps, strlen(ps)+1); ps = psbak.getbuf(); psbuf.setsize(0); } \
        body; \
        if(vsbuf.length()) vs = vsbuf.getbuf(); \
        if(psbuf.length()) ps = psbuf.getbuf(); \
    }
    GENSHADER(slotparams.length(), genuniformdefs(vsbuf, psbuf, vs, ps));
    GENSHADER(strstr(vs, "#pragma CUBE2_fog") || strstr(ps, "#pragma CUBE2_fog"), genfogshader(vsbuf, psbuf, vs, ps));
    Shader *s = newshader(*type, name, vs, ps);
    if(s)
    {
    }
    slotparams.shrink(0);
}

void variantshader(int *type, char *name, int *row, char *vs, char *ps, int *maxvariants)
{
    if(*row < 0)
    {
        shader(type, name, vs, ps);
        return;
    }

    Shader *s = lookupshaderbyname(name);
    if(!s) return;

    defformatstring(varname)("<variant:%d,%d>%s", s->variants[*row].length(), *row, name);
    if(*maxvariants > 0)
    {
        int numvariants = 0;
        loopi(MAXVARIANTROWS) numvariants += s->variants[i].length();
        defformatstring(info)("shader %s", name);
        renderprogress(numvariants / float(*maxvariants), info);
    }
    vector<char> vsbuf, psbuf, vsbak, psbak;
    GENSHADER(s->defaultparams.length(), genuniformdefs(vsbuf, psbuf, vs, ps, s));
    GENSHADER(strstr(vs, "#pragma CUBE2_fog") || strstr(ps, "#pragma CUBE2_fog"), genfogshader(vsbuf, psbuf, vs, ps));
    Shader *v = newshader(*type, varname, vs, ps, s, *row);
    if(v)
    {
        if(strstr(ps, "#pragma CUBE2_variant") || strstr(vs, "#pragma CUBE2_variant")) gengenericvariant(*s, varname, vs, ps, *row);
    }
}

void setshader(char *name)
{
    slotparams.shrink(0);
    Shader *s = shaders.access(name);
    if(!s)
    {
        conoutf(CON_ERROR, "no such shader: %s", name);
    }
    else slotshader = s;
}

static float *findslotparam(Slot &s, const char *name)
{
    loopv(s.params)
    {
        SlotShaderParam &param = s.params[i];
        if(!strcmp(name, param.name)) return param.val;
    }
    if(!s.shader->detailshader) return NULL;
    loopv(s.shader->detailshader->defaultparams)
    {
        SlotShaderParamState &param = s.shader->detailshader->defaultparams[i];
        if(!strcmp(name, param.name)) return param.val;
    }
    return NULL;
}

static float *findslotparam(VSlot &s, const char *name)
{
    loopv(s.params)
    {
        SlotShaderParam &param = s.params[i];
        if(!strcmp(name, param.name)) return param.val;
    }
    return findslotparam(*s.slot, name);
}

void resetslotshader()
{
    slotshader = NULL;
    slotparams.shrink(0);
}

void setslotshader(Slot &s)
{
    s.shader = slotshader;
    if(!s.shader)
    {
        s.shader = stdworldshader;
        return;
    }
    loopv(slotparams) s.params.add(slotparams[i]);
}

static void linkslotshaderparams(vector<SlotShaderParam> &params, Shader *sh, bool load)
{
    if(sh) loopv(params)
    {
        int loc = -1;
        SlotShaderParam &param = params[i];
        loopv(sh->defaultparams)
        {
            SlotShaderParamState &dparam = sh->defaultparams[i];
            if(dparam.name==param.name)
            {
                if(memcmp(param.val, dparam.val, sizeof(param.val))) loc = i;
                break;
            }
        }
        param.loc = loc;
    }

    else if(load) loopv(params) params[i].loc = -1;
}

void linkslotshader(Slot &s, bool load)
{
    if(!s.shader) return;

    if(load && !s.shader->detailshader) s.shader->fixdetailshader();

    Shader *sh = s.shader->detailshader;
    linkslotshaderparams(s.params, sh, load);
}

void linkvslotshader(VSlot &s, bool load)
{
    if(!s.slot->shader) return;

    Shader *sh = s.slot->shader->detailshader;
    linkslotshaderparams(s.params, sh, load);

    if(!sh) return;

    if(s.slot->texmask&(1<<TEX_GLOW))
    {
        const float *cparam = findslotparam(s, "glowcolor");
        if(cparam) loopk(3) s.glowcolor[k] = clamp(cparam[k], 0.0f, 1.0f);
    }
}

void altshader(char *origname, char *altname)
{
    Shader *orig = shaders.access(origname), *alt = shaders.access(altname);
    if(!orig || !alt) return;
    orig->altshader = alt;
    orig->fixdetailshader(false);
}

void fastshader(char *nice, char *fast, int *detail)
{
    Shader *ns = shaders.access(nice), *fs = shaders.access(fast);
    if(!ns || !fs) return;
    loopi(min(*detail+1, MAXSHADERDETAIL)) ns->fastshader[i] = fs;
    ns->fixdetailshader(false);
}

COMMAND(shader, "isss");
COMMAND(variantshader, "isissi");
COMMAND(setshader, "s");
COMMAND(altshader, "ss");
COMMAND(fastshader, "ssi");
COMMAND(defershader, "iss");
ICOMMAND(forceshader, "s", (const char *name), useshaderbyname(name));

void isshaderdefined(char *name)
{
    Shader *s = lookupshaderbyname(name);
    intret(s ? 1 : 0);
}

void isshadernative(char *name)
{
    Shader *s = lookupshaderbyname(name);
    intret(s && s->native ? 1 : 0);
}

COMMAND(isshaderdefined, "s");
COMMAND(isshadernative, "s");

static hashset<const char *> shaderparamnames(256);

const char *getshaderparamname(const char *name)
{
    const char **exists = shaderparamnames.access(name);
    if(exists) return *exists;
    name = newstring(name);
    shaderparamnames[name] = name;
    return name;
}

void addslotparam(const char *name, float x, float y, float z, float w)
{
    if(name) name = getshaderparamname(name);
    loopv(slotparams)
    {
        SlotShaderParam &param = slotparams[i];
        if(param.name==name)
        {
            param.val[0] = x;
            param.val[1] = y;
            param.val[2] = z;
            param.val[3] = w;
            return;
        }
    }
    SlotShaderParam param = {name, -1, {x, y, z, w}};
    slotparams.add(param);
}

ICOMMAND(setuniformparam, "sffff", (char *name, float *x, float *y, float *z, float *w), addslotparam(name, *x, *y, *z, *w));
ICOMMAND(setshaderparam, "sffff", (char *name, float *x, float *y, float *z, float *w), addslotparam(name, *x, *y, *z, *w));
ICOMMAND(defuniformparam, "sffff", (char *name, float *x, float *y, float *z, float *w), addslotparam(name, *x, *y, *z, *w));

#define NUMPOSTFXBINDS 10

struct postfxtex
{
    GLuint id;
    int scale, used;

    postfxtex() : id(0), scale(0), used(-1) {}
};
vector<postfxtex> postfxtexs;
int postfxbinds[NUMPOSTFXBINDS];
GLuint postfxfb = 0;
int postfxw = 0, postfxh = 0;

struct postfxpass
{
    Shader *shader;
    vec4 params;
    uint inputs, freeinputs;
    int outputbind, outputscale;

    postfxpass() : shader(NULL), inputs(1), freeinputs(1), outputbind(0), outputscale(0) {}
};
vector<postfxpass> postfxpasses;

static int allocatepostfxtex(int scale)
{
    loopv(postfxtexs)
    {
        postfxtex &t = postfxtexs[i];
        if(t.scale==scale && t.used < 0) return i;
    }
    postfxtex &t = postfxtexs.add();
    t.scale = scale;
    glGenTextures(1, &t.id);
    createtexture(t.id, max(postfxw>>scale, 1), max(postfxh>>scale, 1), NULL, 3, 1, GL_RGB, GL_TEXTURE_RECTANGLE_ARB);
    return postfxtexs.length()-1;
}

void cleanuppostfx(bool fullclean)
{
    if(fullclean && postfxfb)
    {
        glDeleteFramebuffers_(1, &postfxfb);
        postfxfb = 0;
    }

    loopv(postfxtexs) glDeleteTextures(1, &postfxtexs[i].id);
    postfxtexs.shrink(0);

    postfxw = 0;
    postfxh = 0;
}

GLuint setuppostfx(int w, int h)
{
    if(postfxpasses.empty()) return 0;

    if(postfxw != w || postfxh != h)
    {
        cleanuppostfx(false);
        postfxw = w;
        postfxh = h;
    }

    loopi(NUMPOSTFXBINDS) postfxbinds[i] = -1;
    loopv(postfxtexs) postfxtexs[i].used = -1;

    if(!postfxfb) glGenFramebuffers_(1, &postfxfb);
    glBindFramebuffer_(GL_FRAMEBUFFER_EXT, postfxfb);
    int tex = allocatepostfxtex(0);
    glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, postfxtexs[tex].id, 0);

    postfxbinds[0] = tex;
    postfxtexs[tex].used = 0;

    return postfxfb;
}

void renderpostfx()
{
    loopv(postfxpasses)
    {
        postfxpass &p = postfxpasses[i];

        int tex = -1;
        if(!postfxpasses.inrange(i+1))
        {
            glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
        }
        else
        {
            tex = allocatepostfxtex(p.outputscale);
            glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, postfxtexs[tex].id, 0);
        }

        int w = tex >= 0 ? max(postfxw>>postfxtexs[tex].scale, 1) : postfxw,
            h = tex >= 0 ? max(postfxh>>postfxtexs[tex].scale, 1) : postfxh;
        glViewport(0, 0, w, h);
        p.shader->set();
        LOCALPARAM(params, (p.params));
        int tw = w, th = h, tmu = 0;
        loopj(NUMPOSTFXBINDS) if(p.inputs&(1<<j) && postfxbinds[j] >= 0)
        {
            if(!tmu)
            {
                tw = max(postfxw>>postfxtexs[postfxbinds[j]].scale, 1);
                th = max(postfxh>>postfxtexs[postfxbinds[j]].scale, 1);
            }
            else glActiveTexture_(GL_TEXTURE0_ARB + tmu);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, postfxtexs[postfxbinds[j]].id);
            ++tmu;
        }
        if(tmu) glActiveTexture_(GL_TEXTURE0_ARB);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0,  0);  glVertex2f(-1, -1);
        glTexCoord2f(tw, 0);  glVertex2f( 1, -1);
        glTexCoord2f(0,  th); glVertex2f(-1,  1);
        glTexCoord2f(tw, th); glVertex2f( 1,  1);
        glEnd();

        loopj(NUMPOSTFXBINDS) if(p.freeinputs&(1<<j) && postfxbinds[j] >= 0)
        {
            postfxtexs[postfxbinds[j]].used = -1;
            postfxbinds[j] = -1;
        }
        if(tex >= 0)
        {
            if(postfxbinds[p.outputbind] >= 0) postfxtexs[postfxbinds[p.outputbind]].used = -1;
            postfxbinds[p.outputbind] = tex;
            postfxtexs[tex].used = p.outputbind;
        }
    }
}

static bool addpostfx(const char *name, int outputbind, int outputscale, uint inputs, uint freeinputs, const vec4 &params)
{
    if(!*name) return false;
    Shader *s = useshaderbyname(name);
    if(!s)
    {
        conoutf(CON_ERROR, "no such postfx shader: %s", name);
        return false;
    }
    postfxpass &p = postfxpasses.add();
    p.shader = s;
    p.outputbind = outputbind;
    p.outputscale = outputscale;
    p.inputs = inputs;
    p.freeinputs = freeinputs;
    p.params = params;
    return true;
}

void clearpostfx()
{
    postfxpasses.shrink(0);
    cleanuppostfx(false);
}

COMMAND(clearpostfx, "");

ICOMMAND(addpostfx, "siisffff", (char *name, int *bind, int *scale, char *inputs, float *x, float *y, float *z, float *w),
{
    int inputmask = inputs[0] ? 0 : 1;
    int freemask = inputs[0] ? 0 : 1;
    bool freeinputs = true;
    for(; *inputs; inputs++)
    {
        if(isdigit(*inputs))
        {
            inputmask |= 1<<(*inputs-'0');
            if(freeinputs) freemask |= 1<<(*inputs-'0');
        }
        else if(*inputs=='+') freeinputs = false;
        else if(*inputs=='-') freeinputs = true;
    }
    inputmask &= (1<<NUMPOSTFXBINDS)-1;
    freemask &= (1<<NUMPOSTFXBINDS)-1;
    addpostfx(name, clamp(*bind, 0, NUMPOSTFXBINDS-1), max(*scale, 0), inputmask, freemask, vec4(*x, *y, *z, *w));
});

ICOMMAND(setpostfx, "sffff", (char *name, float *x, float *y, float *z, float *w),
{
    clearpostfx();
    if(name[0]) addpostfx(name, 0, 0, 1, 1, vec4(*x, *y, *z, *w));
});

void cleanupshaders()
{
    cleanuppostfx(true);

    nullshader = defaultshader = notextureshader = NULL;
    enumerate(shaders, Shader, s, s.cleanup());
    Shader::lastshader = NULL;
    glUseProgram_(0);
}

void reloadshaders()
{
    identflags &= ~IDF_PERSIST;
    loadshaders();
    identflags |= IDF_PERSIST;

    linkslotshaders();
    enumerate(shaders, Shader, s,
    {
        if(!s.standard && !(s.type&(SHADER_DEFERRED|SHADER_INVALID)) && !s.variantshader)
        {
            defformatstring(info)("shader %s", s.name);
            renderprogress(0.0, info);
            if(!s.compile()) s.cleanup(true);
            loopi(MAXVARIANTROWS) loopvj(s.variants[i])
            {
                Shader *v = s.variants[i][j];
                if((v->reusevs && v->reusevs->type&SHADER_INVALID) ||
                   (v->reuseps && v->reuseps->type&SHADER_INVALID) ||
                   !v->compile())
                    v->cleanup(true);
            }
        }
        if(s.forced && !s.detailshader) s.fixdetailshader();
    });
}

void resetshaders()
{
    cleanupgbuffer();
    cleanupshaders();
    setupshaders();
    initgbuffer();
    reloadshaders();
    allchanged(true);
}
COMMAND(resetshaders, "");

void setupblurkernel(int radius, float sigma, float *weights, float *offsets)
{
    if(radius<1 || radius>MAXBLURRADIUS) return;
    sigma *= 2*radius;
    float total = 1.0f/sigma;
    weights[0] = total;
    offsets[0] = 0;
    // rely on bilinear filtering to sample 2 pixels at once
    // transforms a*X + b*Y into (u+v)*[X*u/(u+v) + Y*(1 - u/(u+v))]
    loopi(radius)
    {
        float weight1 = exp(-((2*i)*(2*i)) / (2*sigma*sigma)) / sigma,
              weight2 = exp(-((2*i+1)*(2*i+1)) / (2*sigma*sigma)) / sigma,
              scale = weight1 + weight2,
              offset = 2*i+1 + weight2 / scale;
        weights[i+1] = scale;
        offsets[i+1] = offset;
        total += 2*scale;
    }
    loopi(radius+1) weights[i] /= total;
    for(int i = radius+1; i <= MAXBLURRADIUS; i++) weights[i] = offsets[i] = 0;
}

void setblurshader(int pass, int size, int radius, float *weights, float *offsets, GLenum target)
{
    if(radius<1 || radius>MAXBLURRADIUS) return;
    static Shader *blurshader[7][2] = { { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL } },
                  *blurrectshader[7][2] = { { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL } };
    Shader *&s = (target == GL_TEXTURE_RECTANGLE_ARB ? blurrectshader : blurshader)[radius-1][pass];
    if(!s)
    {
        defformatstring(name)("blur%c%d%s", 'x'+pass, radius, target == GL_TEXTURE_RECTANGLE_ARB ? "rect" : "");
        s = lookupshaderbyname(name);
    }
    s->set();
    LOCALPARAM(weights, (weights[0], weights[1], weights[2], weights[3]));
    LOCALPARAM(weights2, (weights[4], weights[5], weights[6], weights[7]));
    LOCALPARAM(offsets, (
        pass==0 ? offsets[1]/size : offsets[0]/size,
        pass==1 ? offsets[1]/size : offsets[0]/size,
        (offsets[2] - offsets[1])/size,
        (offsets[3] - offsets[2])/size));
    loopk(4)
    {
        static LocalShaderParam offsets2[4] = { "offset4", "offset5", "offset6", "offset7" };
        offsets2[k].set(
            pass==0 ? offsets[4+k]/size : offsets[0]/size,
            pass==1 ? offsets[4+k]/size : offsets[0]/size,
            0, 0);
    }
}

