// command.cpp: implements the parsing and execution of a tiny script language which
// is largely backwards compatible with the quake console language.

#include "engine.h"

#ifndef STANDALONE
VAR(confver, 1, CONFVERSION, -1);
VAR(ignoreconfver, 0, 0, 1);
#endif
hashnameset<ident> idents; // contains ALL vars/commands/aliases
vector<ident *> identmap;
ident *dummyident = NULL;

int identflags = 0;

enum
{
    MAXARGS = 25,
    MAXRESULTS = 7,
    MAXCOMARGS = 12
};

VARN(numargs, _numargs, MAXARGS, 0, 0);

static inline void freearg(tagval &v)
{
    switch(v.type)
    {
        case VAL_STR: delete[] v.s; break;
        case VAL_CODE: if(v.code[-1] == CODE_START) delete[] (uchar *)&v.code[-1]; break;
    }
}

static inline void forcenull(tagval &v)
{
    switch(v.type)
    {
        case VAL_NULL: return;
    }
    freearg(v);
    v.setnull();
}

static inline float forcefloat(tagval &v)
{
    float f = 0.0f;
    switch(v.type)
    {
        case VAL_INT: f = v.i; break;
        case VAL_STR: case VAL_MACRO: case VAL_CSTR: f = parsefloat(v.s); break;
        case VAL_FLOAT: return v.f;
    }
    freearg(v);
    v.setfloat(f);
    return f;
}

static inline int forceint(tagval &v)
{
    int i = 0;
    switch(v.type)
    {
        case VAL_FLOAT: i = v.f; break;
        case VAL_STR: case VAL_MACRO: case VAL_CSTR: i = parseint(v.s); break;
        case VAL_INT: return v.i;
    }
    freearg(v);
    v.setint(i);
    return i;
}

static inline const char *forcestr(tagval &v)
{
    const char *s = "";
    switch(v.type)
    {
        case VAL_FLOAT: s = floatstr(v.f); break;
        case VAL_INT: s = intstr(v.i); break;
        case VAL_MACRO: case VAL_CSTR: s = v.s; break;
        case VAL_STR: return v.s;
    }
    freearg(v);
    v.setstr(newstring(s));
    return s;
}

static inline void forcearg(tagval &v, int type)
{
    switch(type)
    {
        case RET_STR: if(v.type != VAL_STR) forcestr(v); break;
        case RET_INT: if(v.type != VAL_INT) forceint(v); break;
        case RET_FLOAT: if(v.type != VAL_FLOAT) forcefloat(v); break;
    }
}

void tagval::cleanup()
{
    freearg(*this);
}

static inline void freeargs(tagval *args, int &oldnum, int newnum)
{
    for(int i = newnum; i < oldnum; i++) freearg(args[i]);
    oldnum = newnum;
}

void cleancode(ident &id)
{
    if(id.code)
    {
        id.code[0] -= 0x100;
        if(int(id.code[0]) < 0x100) delete[] id.code;
        id.code = NULL;
    }
}

struct nullval : tagval
{
    nullval() { setnull(); }
} nullval;
tagval noret = nullval, *commandret = &noret;

void clear_command()
{
    enumerate(idents, ident, i,
    {
        if(i.type==ID_ALIAS)
        {
            DELETEA(i.name);
            i.forcenull();
            DELETEA(i.code);
        }
    });
}

void clearoverride(ident &i)
{
    if(!(i.flags&IDF_OVERRIDDEN)) return;
    switch(i.type)
    {
        case ID_ALIAS:
            if(i.valtype==VAL_STR)
            {
                if(!i.val.s[0]) break;
                delete[] i.val.s;
            }
            cleancode(i);
            i.valtype = VAL_STR;
            i.val.s = newstring("");
            break;
        case ID_VAR:
            *i.storage.i = i.overrideval.i;
            i.changed();
            break;
        case ID_FVAR:
            *i.storage.f = i.overrideval.f;
            i.changed();
            break;
        case ID_SVAR:
            delete[] *i.storage.s;
            *i.storage.s = i.overrideval.s;
            i.changed();
            break;
    }
    i.flags &= ~IDF_OVERRIDDEN;
}

void clearoverrides()
{
    enumerate(idents, ident, i, clearoverride(i));
}

static bool initedidents = false;
static vector<ident> *identinits = NULL;

static inline ident *addident(const ident &id)
{
    if(!initedidents)
    {
        if(!identinits) identinits = new vector<ident>;
        identinits->add(id);
        return NULL;
    }
    ident &def = idents.access(id.name, id);
    def.index = identmap.length();
    return identmap.add(&def);
}

static bool initidents()
{
    initedidents = true;
    for(int i = 0; i < MAXARGS; i++)
    {
        defformatstring(argname, "arg%d", i+1);
        newident(argname, IDF_ARG);
    }
    dummyident = newident("//dummy", IDF_UNKNOWN);
    if(identinits)
    {
        loopv(*identinits) addident((*identinits)[i]);
        DELETEP(identinits);
    }
    return true;
}
static bool forceinitidents = initidents();

static const char *sourcefile = NULL, *sourcestr = NULL;

static const char *debugline(const char *p, const char *fmt)
{
    if(!sourcestr) return fmt;
    int num = 1;
    const char *line = sourcestr;
    for(;;)
    {
        const char *end = strchr(line, '\n');
        if(!end) end = line + strlen(line);
        if(p >= line && p <= end)
        {
            static string buf;
            if(sourcefile) formatstring(buf, "%s:%d: %s", sourcefile, num, fmt);
            else formatstring(buf, "%d: %s", num, fmt);
            return buf;
        }
        if(!*end) break;
        line = end + 1;
        num++;
    }
    return fmt;
}

static struct identlink
{
    ident *id;
    identlink *next;
    int usedargs;
    identstack *argstack;
} noalias = { NULL, NULL, (1<<MAXARGS)-1, NULL }, *aliasstack = &noalias;

VAR(dbgalias, 0, 4, 1000);

static void debugalias()
{
    if(!dbgalias) return;
    int total = 0, depth = 0;
    for(identlink *l = aliasstack; l != &noalias; l = l->next) total++;
    for(identlink *l = aliasstack; l != &noalias; l = l->next)
    {
        ident *id = l->id;
        ++depth;
        if(depth < dbgalias) conoutf(CON_ERROR, "  %d) %s", total-depth+1, id->name);
        else if(l->next == &noalias) conoutf(CON_ERROR, depth == dbgalias ? "  %d) %s" : "  ..%d) %s", total-depth+1, id->name);
    }
}

static int nodebug = 0;

static void debugcode(const char *fmt, ...) PRINTFARGS(1, 2);

static void debugcode(const char *fmt, ...)
{
    if(nodebug) return;

    va_list args;
    va_start(args, fmt);
    conoutfv(CON_ERROR, fmt, args);
    va_end(args);

    debugalias();
}

static void debugcodeline(const char *p, const char *fmt, ...) PRINTFARGS(2, 3);

static void debugcodeline(const char *p, const char *fmt, ...)
{
    if(nodebug) return;

    va_list args;
    va_start(args, fmt);
    conoutfv(CON_ERROR, debugline(p, fmt), args);
    va_end(args);

    debugalias();
}

ICOMMAND(nodebug, "e", (uint *body), { nodebug++; executeret(body, *commandret); nodebug--; });

void addident(ident *id)
{
    addident(*id);
}

void pusharg(ident &id, const tagval &v, identstack &stack)
{
    stack.val = id.val;
    stack.valtype = id.valtype;
    stack.next = id.stack;
    id.stack = &stack;
    id.setval(v);
    cleancode(id);
}

void poparg(ident &id)
{
    if(!id.stack) return;
    identstack *stack = id.stack;
    if(id.valtype == VAL_STR) delete[] id.val.s;
    id.setval(*stack);
    cleancode(id);
    id.stack = stack->next;
}

ICOMMAND(push, "rTe", (ident *id, tagval *v, uint *code),
{
    if(id->type != ID_ALIAS || id->index < MAXARGS) return;
    identstack stack;
    pusharg(*id, *v, stack);
    v->type = VAL_NULL;
    id->flags &= ~IDF_UNKNOWN;
    executeret(code, *commandret);
    poparg(*id);
});

static inline void pushalias(ident &id, identstack &stack)
{
    if(id.type == ID_ALIAS && id.index >= MAXARGS)
    {
        pusharg(id, nullval, stack);
        id.flags &= ~IDF_UNKNOWN;
    }
}

static inline void popalias(ident &id)
{
    if(id.type == ID_ALIAS && id.index >= MAXARGS) poparg(id);
}

KEYWORD(local, ID_LOCAL);

static inline bool checknumber(const char *s)
{
    if(isdigit(s[0])) return true;
    else switch(s[0])
    {
        case '+': case '-': return isdigit(s[1]) || (s[1] == '.' && isdigit(s[2]));
        case '.': return isdigit(s[1]) != 0;
        default: return false;
    }
}

static inline bool checknumber(const stringslice &s) { return checknumber(s.str); }

template<class T> static inline ident *newident(const T &name, int flags)
{
    ident *id = idents.access(name);
    if(!id)
    {
        if(checknumber(name))
        {
            debugcode("number %.*s is not a valid identifier name", stringlen(name), stringptr(name));
            return dummyident;
        }
        id = addident(ident(ID_ALIAS, newstring(name), flags));
    }
    return id;
}

static inline ident *forceident(tagval &v)
{
    switch(v.type)
    {
        case VAL_IDENT: return v.id;
        case VAL_MACRO: case VAL_CSTR:
        {
            ident *id = newident(v.s, IDF_UNKNOWN);
            v.setident(id);
            return id;
        }
        case VAL_STR:
        {
            ident *id = newident(v.s, IDF_UNKNOWN);
            delete[] v.s;
            v.setident(id);
            return id;
        }
    }
    freearg(v);
    v.setident(dummyident);
    return dummyident;
}

ident *newident(const char *name, int flags)
{
    return newident<const char *>(name, flags);
}

ident *writeident(const char *name, int flags)
{
    ident *id = newident(name, flags);
    if(id->index < MAXARGS && !(aliasstack->usedargs&(1<<id->index)))
    {
        pusharg(*id, nullval, aliasstack->argstack[id->index]);
        aliasstack->usedargs |= 1<<id->index;
    }
    return id;
}

ident *readident(const char *name)
{
    ident *id = idents.access(name);
    if(id && id->index < MAXARGS && !(aliasstack->usedargs&(1<<id->index)))
       return NULL;
    return id;
}

void resetvar(char *name)
{
    ident *id = idents.access(name);
    if(!id) return;
    if(id->flags&IDF_READONLY) debugcode("variable %s is read-only", id->name);
    else clearoverride(*id);
}

COMMAND(resetvar, "s");

static inline void setarg(ident &id, tagval &v)
{
    if(aliasstack->usedargs&(1<<id.index))
    {
        if(id.valtype == VAL_STR) delete[] id.val.s;
        id.setval(v);
        cleancode(id);
    }
    else
    {
        pusharg(id, v, aliasstack->argstack[id.index]);
        aliasstack->usedargs |= 1<<id.index;
    }
}

static inline void setalias(ident &id, tagval &v)
{
    if(id.valtype == VAL_STR) delete[] id.val.s;
    id.setval(v);
    cleancode(id);
    id.flags = (id.flags & identflags) | identflags;
}

static void setalias(const char *name, tagval &v)
{
    ident *id = idents.access(name);
    if(id)
    {
        if(id->type == ID_ALIAS)
        {
            if(id->index < MAXARGS) setarg(*id, v); else setalias(*id, v);
        }
        else
        {
            debugcode("cannot redefine builtin %s with an alias", id->name);
            freearg(v);
        }
    }
    else if(checknumber(name))
    {
        debugcode("cannot alias number %s", name);
        freearg(v);
    }
    else
    {
        addident(ident(ID_ALIAS, newstring(name), v, identflags));
    }
}

void alias(const char *name, const char *str)
{
    tagval v;
    v.setstr(newstring(str));
    setalias(name, v);
}

void alias(const char *name, tagval &v)
{
    setalias(name, v);
}

ICOMMAND(alias, "sT", (const char *name, tagval *v),
{
    setalias(name, *v);
    v->type = VAL_NULL;
});

// variable's and commands are registered through globals, see cube.h

int variable(const char *name, int min, int cur, int max, int *storage, identfun fun, int flags)
{
    addident(ident(ID_VAR, name, min, max, storage, (void *)fun, flags));
    return cur;
}

float fvariable(const char *name, float min, float cur, float max, float *storage, identfun fun, int flags)
{
    addident(ident(ID_FVAR, name, min, max, storage, (void *)fun, flags));
    return cur;
}

char *svariable(const char *name, const char *cur, char **storage, identfun fun, int flags)
{
    addident(ident(ID_SVAR, name, storage, (void *)fun, flags));
    return newstring(cur);
}

struct defvar : identval
{
    char *name;
    uint *onchange;

    defvar() : name(NULL), onchange(NULL) {}

    ~defvar()
    {
        DELETEA(name);
        if(onchange) freecode(onchange);
    }

    static void changed(ident *id)
    {
        defvar *v = (defvar *)id->storage.p;
        if(v->onchange) execute(v->onchange);
    }
};

hashnameset<defvar> defvars;

#define DEFVAR(cmdname, fmt, args, body) \
    ICOMMAND(cmdname, fmt, args, \
    { \
        if(idents.access(name)) { debugcode("cannot redefine %s as a variable", name); return; } \
        name = newstring(name); \
        defvar &def = defvars[name]; \
        def.name = name; \
        def.onchange = onchange[0] ? compilecode(onchange) : NULL; \
        body; \
    });
#define DEFIVAR(cmdname, flags) \
    DEFVAR(cmdname, "siiis", (char *name, int *min, int *cur, int *max, char *onchange), \
        def.i = variable(name, *min, *cur, *max, &def.i, def.onchange ? defvar::changed : NULL, flags))
#define DEFFVAR(cmdname, flags) \
    DEFVAR(cmdname, "sfffs", (char *name, float *min, float *cur, float *max, char *onchange), \
        def.f = fvariable(name, *min, *cur, *max, &def.f, def.onchange ? defvar::changed : NULL, flags))
#define DEFSVAR(cmdname, flags) \
    DEFVAR(cmdname, "sss", (char *name, char *cur, char *onchange), \
        def.s = svariable(name, cur, &def.s, def.onchange ? defvar::changed : NULL, flags))

DEFIVAR(defvar, 0);
DEFIVAR(defvarp, IDF_PERSIST);
DEFFVAR(deffvar, 0);
DEFFVAR(deffvarp, IDF_PERSIST);
DEFSVAR(defsvar, 0);
DEFSVAR(defsvarp, IDF_PERSIST);

#define _GETVAR(id, vartype, name, retval) \
    ident *id = idents.access(name); \
    if(!id || id->type!=vartype) return retval;
#define GETVAR(id, name, retval) _GETVAR(id, ID_VAR, name, retval)
#define OVERRIDEVAR(errorval, saveval, resetval, clearval) \
    if(identflags&IDF_OVERRIDDEN || id->flags&IDF_OVERRIDE) \
    { \
        if(id->flags&IDF_PERSIST) \
        { \
            debugcode("cannot override persistent variable %s", id->name); \
            errorval; \
        } \
        if(!(id->flags&IDF_OVERRIDDEN)) { saveval; id->flags |= IDF_OVERRIDDEN; } \
        else { clearval; } \
    } \
    else \
    { \
        if(id->flags&IDF_OVERRIDDEN) { resetval; id->flags &= ~IDF_OVERRIDDEN; } \
        clearval; \
    }

void setvar(const char *name, int i, bool dofunc, bool doclamp)
{
    GETVAR(id, name, );
    OVERRIDEVAR(return, id->overrideval.i = *id->storage.i, , )
    if(doclamp) *id->storage.i = clamp(i, id->minval, id->maxval);
    else *id->storage.i = i;
    if(dofunc) id->changed();
}
void setfvar(const char *name, float f, bool dofunc, bool doclamp)
{
    _GETVAR(id, ID_FVAR, name, );
    OVERRIDEVAR(return, id->overrideval.f = *id->storage.f, , );
    if(doclamp) *id->storage.f = clamp(f, id->minvalf, id->maxvalf);
    else *id->storage.f = f;
    if(dofunc) id->changed();
}
void setsvar(const char *name, const char *str, bool dofunc)
{
    _GETVAR(id, ID_SVAR, name, );
    OVERRIDEVAR(return, id->overrideval.s = *id->storage.s, delete[] id->overrideval.s, delete[] *id->storage.s);
    *id->storage.s = newstring(str);
    if(dofunc) id->changed();
}
int getvar(const char *name)
{
    GETVAR(id, name, 0);
    return *id->storage.i;
}
int getvarmin(const char *name)
{
    GETVAR(id, name, 0);
    return id->minval;
}
int getvarmax(const char *name)
{
    GETVAR(id, name, 0);
    return id->maxval;
}
float getfvarmin(const char *name)
{
    _GETVAR(id, ID_FVAR, name, 0);
    return id->minvalf;
}
float getfvarmax(const char *name)
{
    _GETVAR(id, ID_FVAR, name, 0);
    return id->maxvalf;
}

ICOMMAND(getvarmin, "s", (char *s), intret(getvarmin(s)));
ICOMMAND(getvarmax, "s", (char *s), intret(getvarmax(s)));
ICOMMAND(getfvarmin, "s", (char *s), floatret(getfvarmin(s)));
ICOMMAND(getfvarmax, "s", (char *s), floatret(getfvarmax(s)));

bool identexists(const char *name) { return idents.access(name)!=NULL; }
ident *getident(const char *name) { return idents.access(name); }

void touchvar(const char *name)
{
    ident *id = idents.access(name);
    if(id) switch(id->type)
    {
        case ID_VAR:
        case ID_FVAR:
        case ID_SVAR:
            id->changed();
            break;
    }
}

const char *getalias(const char *name)
{
    ident *i = idents.access(name);
    return i && i->type==ID_ALIAS && (i->index >= MAXARGS || aliasstack->usedargs&(1<<i->index)) ? i->getstr() : "";
}

ICOMMAND(getalias, "s", (char *s), result(getalias(s)));

int clampvar(ident *id, int val, int minval, int maxval)
{
    if(val < minval) val = minval;
    else if(val > maxval) val = maxval;
    else return val;
    debugcode(id->flags&IDF_HEX ?
            (minval <= 255 ? "valid range for %s is %d..0x%X" : "valid range for %s is 0x%X..0x%X") :
            "valid range for %s is %d..%d",
        id->name, minval, maxval);
    return val;
}

void setvarchecked(ident *id, int val)
{
    if(id->flags&IDF_READONLY) debugcode("variable %s is read-only", id->name);
#ifndef STANDALONE
    else if(!(id->flags&IDF_OVERRIDE) || identflags&IDF_OVERRIDDEN || game::showenthelpers())
#else
    else
#endif
    {
        OVERRIDEVAR(return, id->overrideval.i = *id->storage.i, , )
        if(val < id->minval || val > id->maxval) val = clampvar(id, val, id->minval, id->maxval);
        *id->storage.i = val;
        id->changed();                                             // call trigger function if available
#ifndef STANDALONE
        if(id->flags&IDF_OVERRIDE && !(identflags&IDF_OVERRIDDEN)) game::vartrigger(id);
#endif
    }
}

static inline void setvarchecked(ident *id, tagval *args, int numargs)
{
    int val = forceint(args[0]);
    if(id->flags&IDF_HEX && numargs > 1)
    {
        val = (val << 16) | (forceint(args[1])<<8);
        if(numargs > 2) val |= forceint(args[2]);
    }
    setvarchecked(id, val);
}

float clampfvar(ident *id, float val, float minval, float maxval)
{
    if(val < minval) val = minval;
    else if(val > maxval) val = maxval;
    else return val;
    debugcode("valid range for %s is %s..%s", id->name, floatstr(minval), floatstr(maxval));
    return val;
}

void setfvarchecked(ident *id, float val)
{
    if(id->flags&IDF_READONLY) debugcode("variable %s is read-only", id->name);
#ifndef STANDALONE
    else if(!(id->flags&IDF_OVERRIDE) || identflags&IDF_OVERRIDDEN || game::showenthelpers())
#else
    else
#endif
    {
        OVERRIDEVAR(return, id->overrideval.f = *id->storage.f, , );
        if(val < id->minvalf || val > id->maxvalf) val = clampfvar(id, val, id->minvalf, id->maxvalf);
        *id->storage.f = val;
        id->changed();
#ifndef STANDALONE
        if(id->flags&IDF_OVERRIDE && !(identflags&IDF_OVERRIDDEN)) game::vartrigger(id);
#endif
    }
}

void setsvarchecked(ident *id, const char *val)
{
    if(id->flags&IDF_READONLY) debugcode("variable %s is read-only", id->name);
#ifndef STANDALONE
    else if(!(id->flags&IDF_OVERRIDE) || identflags&IDF_OVERRIDDEN || game::showenthelpers())
#else
    else
#endif
    {
        OVERRIDEVAR(return, id->overrideval.s = *id->storage.s, delete[] id->overrideval.s, delete[] *id->storage.s);
        *id->storage.s = newstring(val);
        id->changed();
#ifndef STANDALONE
        if(id->flags&IDF_OVERRIDE && !(identflags&IDF_OVERRIDDEN)) game::vartrigger(id);
#endif
    }
}

bool addcommand(const char *name, identfun fun, const char *args, int type)
{
    uint argmask = 0;
    int numargs = 0;
    bool limit = true;
    if(args) for(const char *fmt = args; *fmt; fmt++) switch(*fmt)
    {
        case 'i': case 'b': case 'f': case 'F': case 't': case 'T': case 'E': case 'N': case 'D': if(numargs < MAXARGS) numargs++; break;
        case 'S': case 's': case 'e': case 'r': case '$': if(numargs < MAXARGS) { argmask |= 1<<numargs; numargs++; } break;
        case '1': case '2': case '3': case '4': if(numargs < MAXARGS) fmt -= *fmt-'0'+1; break;
        case 'C': case 'V': limit = false; break;
        default: fatal("builtin %s declared with illegal type: %s", name, args); break;
    }
    if(limit && numargs > MAXCOMARGS) fatal("builtin %s declared with too many args: %d", name, numargs);
    addident(ident(type, name, args, argmask, numargs, (void *)fun));
    return false;
}

const char *parsestring(const char *p)
{
    for(; *p; p++) switch(*p)
    {
        case '\r':
        case '\n':
        case '\"':
            return p;
        case '^':
            if(*++p) break;
            return p;
    }
    return p;
}

int unescapestring(char *dst, const char *src, const char *end)
{
    char *start = dst;
    while(src < end)
    {
        int c = *src++;
        if(c == '^')
        {
            if(src >= end) break;
            int e = *src++;
            switch(e)
            {
                case 'n': *dst++ = '\n'; break;
                case 't': *dst++ = '\t'; break;
                case 'f': *dst++ = '\f'; break;
                default: *dst++ = e; break;
            }
        }
        else *dst++ = c;
    }
    *dst = '\0';
    return dst - start;
}

static char *conc(vector<char> &buf, tagval *v, int n, bool space, const char *prefix = NULL, int prefixlen = 0)
{
    if(prefix)
    {
        buf.put(prefix, prefixlen);
        if(space && n) buf.add(' ');
    }
    loopi(n)
    {
        const char *s = "";
        int len = 0;
        switch(v[i].type)
        {
            case VAL_INT: s = intstr(v[i].i); break;
            case VAL_FLOAT: s = floatstr(v[i].f); break;
            case VAL_STR: case VAL_CSTR: s = v[i].s; break;
            case VAL_MACRO: s = v[i].s; len = v[i].code[-1]>>8; goto haslen;
        }
        len = int(strlen(s));
    haslen:
        buf.put(s, len);
        if(i == n-1) break;
        if(space) buf.add(' ');
    }
    buf.add('\0');
    return buf.getbuf();
}

static char *conc(tagval *v, int n, bool space, const char *prefix, int prefixlen)
{
    static int vlen[MAXARGS];
    static char numbuf[3*MAXSTRLEN];
    int len = prefixlen, numlen = 0, i = 0;
    for(; i < n; i++) switch(v[i].type)
    {
        case VAL_MACRO: len += (vlen[i] = v[i].code[-1]>>8); break;
        case VAL_STR: case VAL_CSTR: len += (vlen[i] = int(strlen(v[i].s))); break;
        case VAL_INT:
            if(numlen + MAXSTRLEN > int(sizeof(numbuf))) goto overflow;
            intformat(&numbuf[numlen], v[i].i);
            numlen += (vlen[i] = strlen(&numbuf[numlen]));
            break;
        case VAL_FLOAT:
            if(numlen + MAXSTRLEN > int(sizeof(numbuf))) goto overflow;
            floatformat(&numbuf[numlen], v[i].f);
            numlen += (vlen[i] = strlen(&numbuf[numlen]));
            break;
        default: vlen[i] = 0; break;
    }
overflow:
    if(space) len += max(prefix ? i : i-1, 0);
    char *buf = newstring(len + numlen);
    int offset = 0, numoffset = 0;
    if(prefix)
    {
        memcpy(buf, prefix, prefixlen);
        offset += prefixlen;
        if(space && i) buf[offset++] = ' ';
    }
    loopj(i)
    {
        if(v[j].type == VAL_INT || v[j].type == VAL_FLOAT)
        {
            memcpy(&buf[offset], &numbuf[numoffset], vlen[j]);
            numoffset += vlen[j];
        }
        else if(vlen[j]) memcpy(&buf[offset], v[j].s, vlen[j]);
        offset += vlen[j];
        if(j==i-1) break;
        if(space) buf[offset++] = ' ';
    }
    buf[offset] = '\0';
    if(i < n)
    {
        char *morebuf = conc(&v[i], n-i, space, buf, offset);
        delete[] buf;
        return morebuf;
    }
    return buf;
}

static inline char *conc(tagval *v, int n, bool space)
{
    return conc(v, n, space, NULL, 0);
}

static inline char *conc(tagval *v, int n, bool space, const char *prefix)
{
    return conc(v, n, space, prefix, strlen(prefix));
}

static inline void skipcomments(const char *&p)
{
    for(;;)
    {
        p += strspn(p, " \t\r");
        if(p[0]!='/' || p[1]!='/') break;
        p += strcspn(p, "\n\0");
    }
}

static vector<char> strbuf[4];
static int stridx = 0;

static inline void cutstring(const char *&p, stringslice &s)
{
    p++;
    const char *end = parsestring(p);
    int maxlen = int(end-p) + 1;

    stridx = (stridx + 1)%4;
    vector<char> &buf = strbuf[stridx];
    if(buf.alen < maxlen) buf.growbuf(maxlen);

    s.str = buf.buf;
    s.len = unescapestring(buf.buf, p, end);
    p = end;
    if(*p=='\"') p++;
}

static inline char *cutstring(const char *&p)
{
    p++;
    const char *end = parsestring(p);
    char *buf = newstring(end-p);
    unescapestring(buf, p, end);
    p = end;
    if(*p=='\"') p++;
    return buf;
}

static inline const char *parseword(const char *p)
{
    const int maxbrak = 100;
    static char brakstack[maxbrak];
    int brakdepth = 0;
    for(;; p++)
    {
        p += strcspn(p, "\"/;()[] \t\r\n\0");
        switch(p[0])
        {
            case '"': case ';': case ' ': case '\t': case '\r': case '\n': case '\0': return p;
            case '/': if(p[1] == '/') return p; break;
            case '[': case '(': if(brakdepth >= maxbrak) return p; brakstack[brakdepth++] = p[0]; break;
            case ']': if(brakdepth <= 0 || brakstack[--brakdepth] != '[') return p; break;
            case ')': if(brakdepth <= 0 || brakstack[--brakdepth] != '(') return p; break;
        }
    }
    return p;
}

static inline void cutword(const char *&p, stringslice &s)
{
    s.str = p;
    p = parseword(p);
    s.len = int(p-s.str);
}

static inline char *cutword(const char *&p)
{
    const char *word = p;
    p = parseword(p);
    return p!=word ? newstring(word, p-word) : NULL;
}

#define retcode(type, defaultret) ((type) >= VAL_ANY ? ((type) == VAL_CSTR ? RET_STR : (defaultret)) : (type) << CODE_RET)
#define retcodeint(type) retcode(type, RET_INT)
#define retcodefloat(type) retcode(type, RET_FLOAT)
#define retcodeany(type) retcode(type, 0)
#define retcodestr(type) ((type) >= VAL_ANY ? RET_STR : (type) << CODE_RET)

static inline void compilestr(vector<uint> &code, const char *word, int len, bool macro = false)
{
    if(len <= 3 && !macro)
    {
        uint op = CODE_VALI|RET_STR;
        loopi(len) op |= uint(uchar(word[i]))<<((i+1)*8);
        code.add(op);
        return;
    }
    code.add((macro ? CODE_MACRO : CODE_VAL|RET_STR)|(len<<8));
    code.put((const uint *)word, len/sizeof(uint));
    size_t endlen = len%sizeof(uint);
    union
    {
        char c[sizeof(uint)];
        uint u;
    } end;
    end.u = 0;
    memcpy(end.c, word + len - endlen, endlen);
    code.add(end.u);
}

static inline void compilestr(vector<uint> &code) { code.add(CODE_VALI|RET_STR); }
static inline void compilestr(vector<uint> &code, const stringslice &word, bool macro = false) { compilestr(code, word.str, word.len, macro); }
static inline void compilestr(vector<uint> &code, const char *word, bool macro = false) { compilestr(code, word, int(strlen(word)), macro); }

static inline void compileunescapestring(vector<uint> &code, const char *&p, bool macro = false)
{
    p++;
    const char *end = parsestring(p);
    code.add(macro ? CODE_MACRO : CODE_VAL|RET_STR);
    char *buf = (char *)code.reserve(int(end-p)/sizeof(uint) + 1).buf;
    int len = unescapestring(buf, p, end);
    memset(&buf[len], 0, sizeof(uint) - len%sizeof(uint));
    code.last() |= len<<8;
    code.advance(len/sizeof(uint) + 1);
    p = end;
    if(*p == '\"') p++;
}

static inline void compileint(vector<uint> &code, int i = 0)
{
    if(i >= -0x800000 && i <= 0x7FFFFF)
        code.add(CODE_VALI|RET_INT|(i<<8));
    else
    {
        code.add(CODE_VAL|RET_INT);
        code.add(i);
    }
}

static inline void compilenull(vector<uint> &code)
{
    code.add(CODE_VALI|RET_NULL);
}

static uint emptyblock[VAL_ANY][2] =
{
    { CODE_START + 0x100, CODE_EXIT|RET_NULL },
    { CODE_START + 0x100, CODE_EXIT|RET_INT },
    { CODE_START + 0x100, CODE_EXIT|RET_FLOAT },
    { CODE_START + 0x100, CODE_EXIT|RET_STR }
};

static inline void compileblock(vector<uint> &code)
{
    code.add(CODE_EMPTY);
}

static void compilestatements(vector<uint> &code, const char *&p, int rettype, int brak = '\0', int prevargs = 0);

static inline const char *compileblock(vector<uint> &code, const char *p, int rettype = RET_NULL, int brak = '\0')
{
    int start = code.length();
    code.add(CODE_BLOCK);
    code.add(CODE_OFFSET|((start+2)<<8));
    if(p) compilestatements(code, p, VAL_ANY, brak);
    if(code.length() > start + 2)
    {
        code.add(CODE_EXIT|rettype);
        code[start] |= uint(code.length() - (start + 1))<<8;
    }
    else
    {
        code.setsize(start);
        code.add(CODE_EMPTY|rettype);
    }
    return p;
}

static inline void compileident(vector<uint> &code, ident *id = dummyident)
{
    code.add((id->index < MAXARGS ? CODE_IDENTARG : CODE_IDENT)|(id->index<<8));
}

static inline void compileident(vector<uint> &code, const stringslice &word)
{
    compileident(code, newident(word, IDF_UNKNOWN));
}

static inline void compileint(vector<uint> &code, const stringslice &word)
{
    compileint(code, word.len ? parseint(word.str) : 0);
}

static inline void compilefloat(vector<uint> &code, float f = 0.0f)
{
    if(int(f) == f && f >= -0x800000 && f <= 0x7FFFFF)
        code.add(CODE_VALI|RET_FLOAT|(int(f)<<8));
    else
    {
        union { float f; uint u; } conv;
        conv.f = f;
        code.add(CODE_VAL|RET_FLOAT);
        code.add(conv.u);
    }
}

static inline void compilefloat(vector<uint> &code, const stringslice &word)
{
    compilefloat(code, word.len ? parsefloat(word.str) : 0.0f);
}

static inline bool getbool(const char *s)
{
    switch(s[0])
    {
        case '+': case '-':
            switch(s[1])
            {
                case '0': break;
                case '.': return !isdigit(s[2]) || parsefloat(s) != 0;
                default: return true;
            }
            // fall-through
        case '0':
        {
            char *end;
            int val = int(strtoul((char *)s, &end, 0));
            if(val) return true;
            switch(*end)
            {
                case 'e': case '.': return parsefloat(s) != 0;
                default: return false;
            }
        }
        case '.': return !isdigit(s[1]) || parsefloat(s) != 0;
        case '\0': return false;
        default: return true;
    }
}

static inline bool getbool(const tagval &v)
{
    switch(v.type)
    {
        case VAL_FLOAT: return v.f!=0;
        case VAL_INT: return v.i!=0;
        case VAL_STR: case VAL_MACRO: case VAL_CSTR: return getbool(v.s);
        default: return false;
    }
}

static inline void compileval(vector<uint> &code, int wordtype, const stringslice &word = stringslice(NULL, 0))
{
    switch(wordtype)
    {
        case VAL_CANY: if(word.len) compilestr(code, word, true); else compilenull(code); break;
        case VAL_CSTR: compilestr(code, word, true); break;
        case VAL_ANY: if(word.len) compilestr(code, word); else compilenull(code); break;
        case VAL_STR: compilestr(code, word); break;
        case VAL_FLOAT: compilefloat(code, word); break;
        case VAL_INT: compileint(code, word); break;
        case VAL_COND: if(word.len) compileblock(code, word.str); else compilenull(code); break;
        case VAL_CODE: compileblock(code, word.str); break;
        case VAL_IDENT: compileident(code, word); break;
        default: break;
    }
}

static stringslice unusedword(NULL, 0);
static bool compilearg(vector<uint> &code, const char *&p, int wordtype, int prevargs = MAXRESULTS, stringslice &word = unusedword);

static void compilelookup(vector<uint> &code, const char *&p, int ltype, int prevargs = MAXRESULTS)
{
    stringslice lookup;
    switch(*++p)
    {
        case '(':
        case '[':
            if(!compilearg(code, p, VAL_CSTR, prevargs)) goto invalid;
            break;
        case '$':
            compilelookup(code, p, VAL_CSTR, prevargs);
            break;
        case '\"':
            cutstring(p, lookup);
            goto lookupid;
        default:
        {
            cutword(p, lookup);
            if(!lookup.len) goto invalid;
        lookupid:
            ident *id = newident(lookup, IDF_UNKNOWN);
            if(id) switch(id->type)
            {
                case ID_VAR:
                    code.add(CODE_IVAR|retcodeint(ltype)|(id->index<<8));
                    switch(ltype)
                    {
                        case VAL_POP: code.pop(); break;
                        case VAL_CODE: code.add(CODE_COMPILE); break;
                        case VAL_IDENT: code.add(CODE_IDENTU); break;
                    }
                    return;
                case ID_FVAR:
                    code.add(CODE_FVAR|retcodefloat(ltype)|(id->index<<8));
                    switch(ltype)
                    {
                        case VAL_POP: code.pop(); break;
                        case VAL_CODE: code.add(CODE_COMPILE); break;
                        case VAL_IDENT: code.add(CODE_IDENTU); break;
                    }
                    return;
                case ID_SVAR:
                    switch(ltype)
                    {
                        case VAL_POP: return;
                        case VAL_CANY: case VAL_CSTR: case VAL_CODE: case VAL_IDENT: case VAL_COND:
                            code.add(CODE_SVARM|(id->index<<8));
                            break;
                        default:
                            code.add(CODE_SVAR|retcodestr(ltype)|(id->index<<8));
                            break;
                    }
                    goto done;
                case ID_ALIAS:
                    switch(ltype)
                    {
                        case VAL_POP: return;
                        case VAL_CANY: case VAL_COND:
                            code.add((id->index < MAXARGS ? CODE_LOOKUPMARG : CODE_LOOKUPM)|(id->index<<8));
                            break;
                        case VAL_CSTR: case VAL_CODE: case VAL_IDENT:
                            code.add((id->index < MAXARGS ? CODE_LOOKUPMARG : CODE_LOOKUPM)|RET_STR|(id->index<<8));
                            break;
                        default:
                            code.add((id->index < MAXARGS ? CODE_LOOKUPARG : CODE_LOOKUP)|retcodestr(ltype)|(id->index<<8));
                            break;
                    }
                    goto done;
                case ID_COMMAND:
                {
                    int comtype = CODE_COM, numargs = 0;
                    if(prevargs >= MAXRESULTS) code.add(CODE_ENTER);
                    for(const char *fmt = id->args; *fmt; fmt++) switch(*fmt)
                    {
                        case 'S': compilestr(code, NULL, 0, true); numargs++; break;
                        case 's': compilestr(code); numargs++; break;
                        case 'i': compileint(code); numargs++; break;
                        case 'b': compileint(code, INT_MIN); numargs++; break;
                        case 'f': compilefloat(code); numargs++; break;
                        case 'F': code.add(CODE_DUP|RET_FLOAT); numargs++; break;
                        case 'E':
                        case 'T':
                        case 't': compilenull(code); numargs++; break;
                        case 'e': compileblock(code); numargs++; break;
                        case 'r': compileident(code); numargs++; break;
                        case '$': compileident(code, id); numargs++; break;
                        case 'N': compileint(code, -1); numargs++; break;
#ifndef STANDALONE
                        case 'D': comtype = CODE_COMD; numargs++; break;
#endif
                        case 'C': comtype = CODE_COMC; goto compilecomv;
                        case 'V': comtype = CODE_COMV; goto compilecomv;
                        case '1': case '2': case '3': case '4': break;
                    }
                    code.add(comtype|retcodeany(ltype)|(id->index<<8));
                    code.add((prevargs >= MAXRESULTS ? CODE_EXIT : CODE_RESULT_ARG) | retcodeany(ltype));
                    goto done;
                compilecomv:
                    code.add(comtype|retcodeany(ltype)|(numargs<<8)|(id->index<<13));
                    code.add((prevargs >= MAXRESULTS ? CODE_EXIT : CODE_RESULT_ARG) | retcodeany(ltype));
                    goto done;
                }
                default: goto invalid;
            }
            compilestr(code, lookup, true);
            break;
        }
    }
    switch(ltype)
    {
    case VAL_CANY: case VAL_COND:
        code.add(CODE_LOOKUPMU);
        break;
    case VAL_CSTR: case VAL_CODE: case VAL_IDENT:
        code.add(CODE_LOOKUPMU|RET_STR);
        break;
    default:
        code.add(CODE_LOOKUPU|retcodeany(ltype));
        break;
    }
done:
    switch(ltype)
    {
        case VAL_POP: code.add(CODE_POP); break;
        case VAL_CODE: code.add(CODE_COMPILE); break;
        case VAL_COND: code.add(CODE_COND); break;
        case VAL_IDENT: code.add(CODE_IDENTU); break;
    }
    return;
invalid:
    switch(ltype)
    {
        case VAL_POP: break;
        case VAL_NULL: case VAL_ANY: case VAL_CANY: case VAL_WORD: case VAL_COND: compilenull(code); break;
        default: compileval(code, ltype); break;
    }
}

static bool compileblockstr(vector<uint> &code, const char *str, const char *end, bool macro)
{
    int start = code.length();
    code.add(macro ? CODE_MACRO : CODE_VAL|RET_STR);
    char *buf = (char *)code.reserve((end-str)/sizeof(uint)+1).buf;
    int len = 0;
    while(str < end)
    {
        int n = strcspn(str, "\r/\"@]\0");
        memcpy(&buf[len], str, n);
        len += n;
        str += n;
        switch(*str)
        {
            case '\r': str++; break;
            case '\"':
            {
                const char *start = str;
                str = parsestring(str+1);
                if(*str=='\"') str++;
                memcpy(&buf[len], start, str-start);
                len += str-start;
                break;
            }
            case '/':
                if(str[1] == '/') str += strcspn(str, "\n\0");
                else buf[len++] = *str++;
                break;
            case '@':
            case ']':
                if(str < end) { buf[len++] = *str++; break; }
            case '\0': goto done;
        }
    }
done:
    memset(&buf[len], '\0', sizeof(uint)-len%sizeof(uint));
    code.advance(len/sizeof(uint)+1);
    code[start] |= len<<8;
    return true;
}

static bool compileblocksub(vector<uint> &code, const char *&p, int prevargs)
{
    stringslice lookup;
    switch(*p)
    {
        case '(':
            if(!compilearg(code, p, VAL_CANY, prevargs)) return false;
            break;
        case '[':
            if(!compilearg(code, p, VAL_CSTR, prevargs)) return false;
            code.add(CODE_LOOKUPMU);
            break;
        case '\"':
            cutstring(p, lookup);
            goto lookupid;
        default:
        {

            lookup.str = p;
            while(iscubealnum(*p) || *p=='_') p++;
            lookup.len = int(p-lookup.str);
            if(!lookup.len) return false;
        lookupid:
            ident *id = newident(lookup, IDF_UNKNOWN);
            if(id) switch(id->type)
            {
            case ID_VAR: code.add(CODE_IVAR|(id->index<<8)); goto done;
            case ID_FVAR: code.add(CODE_FVAR|(id->index<<8)); goto done;
            case ID_SVAR: code.add(CODE_SVARM|(id->index<<8)); goto done;
            case ID_ALIAS: code.add((id->index < MAXARGS ? CODE_LOOKUPMARG : CODE_LOOKUPM)|(id->index<<8)); goto done;
            }
            compilestr(code, lookup, true);
            code.add(CODE_LOOKUPMU);
        done:
            break;
        }
    }
    return true;
}

static void compileblockmain(vector<uint> &code, const char *&p, int wordtype, int prevargs)
{
    const char *line = p, *start = p;
    int concs = 0;
    for(int brak = 1; brak;)
    {
        p += strcspn(p, "@\"/[]\0");
        int c = *p++;
        switch(c)
        {
            case '\0':
                debugcodeline(line, "missing \"]\"");
                p--;
                goto done;
            case '\"':
                p = parsestring(p);
                if(*p=='\"') p++;
                break;
            case '/':
                if(*p=='/') p += strcspn(p, "\n\0");
                break;
            case '[': brak++; break;
            case ']': brak--; break;
            case '@':
            {
                const char *esc = p;
                while(*p == '@') p++;
                int level = p - (esc - 1);
                if(brak > level) continue;
                else if(brak < level) debugcodeline(line, "too many @s");
                if(!concs && prevargs >= MAXRESULTS) code.add(CODE_ENTER);
                if(concs + 2 > MAXARGS)
                {
                    code.add(CODE_CONCW|RET_STR|(concs<<8));
                    concs = 1;
                }
                if(compileblockstr(code, start, esc-1, true)) concs++;
                if(compileblocksub(code, p, prevargs + concs)) concs++;
                if(concs) start = p;
                else if(prevargs >= MAXRESULTS) code.pop();
                break;
            }
        }
    }
done:
    if(p-1 > start)
    {
        if(!concs) switch(wordtype)
        {
            case VAL_POP:
                return;
            case VAL_CODE: case VAL_COND:
                p = compileblock(code, start, RET_NULL, ']');
                return;
            case VAL_IDENT:
                compileident(code, stringslice(start, p-1));
                return;
        }
        switch(wordtype)
        {
            case VAL_CSTR: case VAL_CODE: case VAL_IDENT: case VAL_CANY: case VAL_COND:
                compileblockstr(code, start, p-1, true);
                break;
            default:
                compileblockstr(code, start, p-1, concs > 0);
                break;
        }
        if(concs > 1) concs++;
    }
    if(concs)
    {
        if(prevargs >= MAXRESULTS)
        {
            code.add(CODE_CONCM|retcodeany(wordtype)|(concs<<8));
            code.add(CODE_EXIT|retcodeany(wordtype));
        }
        else code.add(CODE_CONCW|retcodeany(wordtype)|(concs<<8));
    }
    switch(wordtype)
    {
        case VAL_POP: if(concs || p-1 > start) code.add(CODE_POP); break;
        case VAL_COND: if(!concs && p-1 <= start) compilenull(code); else code.add(CODE_COND); break;
        case VAL_CODE: if(!concs && p-1 <= start) compileblock(code); else code.add(CODE_COMPILE); break;
        case VAL_IDENT: if(!concs && p-1 <= start) compileident(code); else code.add(CODE_IDENTU); break;
        case VAL_CSTR: case VAL_CANY:
            if(!concs && p-1 <= start) compilestr(code, NULL, 0, true);
            break;
        case VAL_STR: case VAL_NULL: case VAL_ANY: case VAL_WORD:
            if(!concs && p-1 <= start) compilestr(code);
            break;
        default:
            if(!concs)
            {
                if(p-1 <= start) compileval(code, wordtype);
                else code.add(CODE_FORCE|(wordtype<<CODE_RET));
            }
            break;
    }
}

static bool compilearg(vector<uint> &code, const char *&p, int wordtype, int prevargs, stringslice &word)
{
    skipcomments(p);
    switch(*p)
    {
        case '\"':
            switch(wordtype)
            {
                case VAL_POP:
                    p = parsestring(p+1);
                    if(*p == '\"') p++;
                    break;
                case VAL_COND:
                {
                    char *s = cutstring(p);
                    if(s[0]) compileblock(code, s);
                    else compilenull(code);
                    delete[] s;
                    break;
                }
                case VAL_CODE:
                {
                    char *s = cutstring(p);
                    compileblock(code, s);
                    delete[] s;
                    break;
                }
                case VAL_WORD:
                    cutstring(p, word);
                    break;
                case VAL_ANY:
                case VAL_STR:
                    compileunescapestring(code, p);
                    break;
                case VAL_CANY:
                case VAL_CSTR:
                    compileunescapestring(code, p, true);
                    break;
                default:
                {
                    stringslice s;
                    cutstring(p, s);
                    compileval(code, wordtype, s);
                    break;
                }
            }
            return true;
        case '$': compilelookup(code, p, wordtype, prevargs); return true;
        case '(':
            p++;
            if(prevargs >= MAXRESULTS)
            {
                code.add(CODE_ENTER);
                compilestatements(code, p, wordtype > VAL_ANY ? VAL_CANY : VAL_ANY, ')');
                code.add(CODE_EXIT|retcodeany(wordtype));
            }
            else
            {
                int start = code.length();
                compilestatements(code, p, wordtype > VAL_ANY ? VAL_CANY : VAL_ANY, ')', prevargs);
                if(code.length() > start) code.add(CODE_RESULT_ARG|retcodeany(wordtype));
                else { compileval(code, wordtype); return true; }
            }
            switch(wordtype)
            {
                case VAL_POP: code.add(CODE_POP); break;
                case VAL_COND: code.add(CODE_COND); break;
                case VAL_CODE: code.add(CODE_COMPILE); break;
                case VAL_IDENT: code.add(CODE_IDENTU); break;
            }
            return true;
        case '[':
            p++;
            compileblockmain(code, p, wordtype, prevargs);
            return true;
        default:
            switch(wordtype)
            {
                case VAL_POP:
                {
                    const char *s = p;
                    p = parseword(p);
                    return p != s;
                }
                case VAL_COND:
                {
                    char *s = cutword(p);
                    if(!s) return false;
                    compileblock(code, s);
                    delete[] s;
                    return true;
                }
                case VAL_CODE:
                {
                    char *s = cutword(p);
                    if(!s) return false;
                    compileblock(code, s);
                    delete[] s;
                    return true;
                }
                case VAL_WORD:
                    cutword(p, word);
                    return word.len!=0;
                default:
                {
                    stringslice s;
                    cutword(p, s);
                    if(!s.len) return false;
                    compileval(code, wordtype, s);
                    return true;
                }
            }
    }
}

static void compilestatements(vector<uint> &code, const char *&p, int rettype, int brak, int prevargs)
{
    const char *line = p;
    stringslice idname;
    int numargs;
    for(;;)
    {
        skipcomments(p);
        idname.str = NULL;
        bool more = compilearg(code, p, VAL_WORD, prevargs, idname);
        if(!more) goto endstatement;
        skipcomments(p);
        if(p[0] == '=') switch(p[1])
        {
            case '/':
                if(p[2] != '/') break;
            case ';': case ' ': case '\t': case '\r': case '\n': case '\0':
                p++;
                if(idname.str)
                {
                    ident *id = newident(idname, IDF_UNKNOWN);
                    if(id && id->type == ID_ALIAS)
                    {
                        if(!(more = compilearg(code, p, VAL_ANY, prevargs))) compilestr(code);
                        code.add((id->index < MAXARGS ? CODE_ALIASARG : CODE_ALIAS)|(id->index<<8));
                        goto endstatement;
                    }
                    compilestr(code, idname, true);
                }
                if(!(more = compilearg(code, p, VAL_ANY))) compilestr(code);
                code.add(CODE_ALIASU);
                goto endstatement;
        }
        numargs = 0;
        if(!idname.str)
        {
        noid:
            while(numargs < MAXARGS && (more = compilearg(code, p, VAL_CANY, prevargs+numargs))) numargs++;
            code.add(CODE_CALLU|(numargs<<8));
        }
        else
        {
            ident *id = idents.access(idname);
            if(!id)
            {
                if(!checknumber(idname)) { compilestr(code, idname, true); goto noid; }
                switch(rettype)
                {
                case VAL_ANY:
                case VAL_CANY:
                {
                    char *end = (char *)idname.str;
                    int val = int(strtoul(idname.str, &end, 0));
                    if(end < idname.end()) compilestr(code, idname, rettype==VAL_CANY);
                    else compileint(code, val);
                    break;
                }
                default:
                    compileval(code, rettype, idname);
                    break;
                }
                code.add(CODE_RESULT);
            }
            else switch(id->type)
            {
                case ID_ALIAS:
                    while(numargs < MAXARGS && (more = compilearg(code, p, VAL_ANY, prevargs+numargs))) numargs++;
                    code.add((id->index < MAXARGS ? CODE_CALLARG : CODE_CALL)|(numargs<<8)|(id->index<<13));
                    break;
                case ID_COMMAND:
                {
                    int comtype = CODE_COM, fakeargs = 0;
                    bool rep = false;
                    for(const char *fmt = id->args; *fmt; fmt++) switch(*fmt)
                    {
                    case 'S':
                    case 's':
                        if(more) more = compilearg(code, p, *fmt == 's' ? VAL_CSTR : VAL_STR, prevargs+numargs);
                        if(!more)
                        {
                            if(rep) break;
                            compilestr(code, NULL, 0, *fmt=='s');
                            fakeargs++;
                        }
                        else if(!fmt[1])
                        {
                            int numconc = 1;
                            while(numargs + numconc < MAXARGS && (more = compilearg(code, p, VAL_CSTR, prevargs+numargs+numconc))) numconc++;
                            if(numconc > 1) code.add(CODE_CONC|RET_STR|(numconc<<8));
                        }
                        numargs++;
                        break;
                    case 'i': if(more) more = compilearg(code, p, VAL_INT, prevargs+numargs); if(!more) { if(rep) break; compileint(code); fakeargs++; } numargs++; break;
                    case 'b': if(more) more = compilearg(code, p, VAL_INT, prevargs+numargs); if(!more) { if(rep) break; compileint(code, INT_MIN); fakeargs++; } numargs++; break;
                    case 'f': if(more) more = compilearg(code, p, VAL_FLOAT, prevargs+numargs); if(!more) { if(rep) break; compilefloat(code); fakeargs++; } numargs++; break;
                    case 'F': if(more) more = compilearg(code, p, VAL_FLOAT, prevargs+numargs); if(!more) { if(rep) break; code.add(CODE_DUP|RET_FLOAT); fakeargs++; } numargs++; break;
                    case 'T':
                    case 't': if(more) more = compilearg(code, p, *fmt == 't' ? VAL_CANY : VAL_ANY, prevargs+numargs); if(!more) { if(rep) break; compilenull(code); fakeargs++; } numargs++; break;
                    case 'E': if(more) more = compilearg(code, p, VAL_COND, prevargs+numargs); if(!more) { if(rep) break; compilenull(code); fakeargs++; } numargs++; break;
                    case 'e': if(more) more = compilearg(code, p, VAL_CODE, prevargs+numargs); if(!more) { if(rep) break; compileblock(code); fakeargs++; } numargs++; break;
                    case 'r': if(more) more = compilearg(code, p, VAL_IDENT, prevargs+numargs); if(!more) { if(rep) break; compileident(code); fakeargs++; } numargs++; break;
                    case '$': compileident(code, id); numargs++; break;
                    case 'N': compileint(code, numargs-fakeargs); numargs++; break;
#ifndef STANDALONE
                    case 'D': comtype = CODE_COMD; numargs++; break;
#endif
                    case 'C': comtype = CODE_COMC; if(more) while(numargs < MAXARGS && (more = compilearg(code, p, VAL_CANY, prevargs+numargs))) numargs++; goto compilecomv;
                    case 'V': comtype = CODE_COMV; if(more) while(numargs < MAXARGS && (more = compilearg(code, p, VAL_CANY, prevargs+numargs))) numargs++; goto compilecomv;
                    case '1': case '2': case '3': case '4': if(more) { fmt -= *fmt-'0'+1; rep = true; } break;
                    }
                    code.add(comtype|retcodeany(rettype)|(id->index<<8));
                    break;
                compilecomv:
                    code.add(comtype|retcodeany(rettype)|(numargs<<8)|(id->index<<13));
                    break;
                }
                case ID_LOCAL:
                    if(more) while(numargs < MAXARGS && (more = compilearg(code, p, VAL_IDENT, prevargs+numargs))) numargs++;
                    if(more) while((more = compilearg(code, p, VAL_POP)));
                    code.add(CODE_LOCAL|(numargs<<8));
                    break;
                case ID_DO:
                    if(more) more = compilearg(code, p, VAL_CODE, prevargs);
                    code.add((more ? CODE_DO : CODE_NULL) | retcodeany(rettype));
                    break;
                case ID_IF:
                    if(more) more = compilearg(code, p, VAL_CANY, prevargs);
                    if(!more) code.add(CODE_NULL | retcodeany(rettype));
                    else
                    {
                        int start1 = code.length();
                        more = compilearg(code, p, VAL_CODE, prevargs+1);
                        if(!more) { code.add(CODE_POP); code.add(CODE_NULL | retcodeany(rettype)); }
                        else
                        {
                            int start2 = code.length();
                            more = compilearg(code, p, VAL_CODE, prevargs+2);
                            uint inst1 = code[start1], op1 = inst1&~CODE_RET_MASK, len1 = start2 - (start1+1);
                            if(!more)
                            {
                                if(op1 == (CODE_BLOCK|(len1<<8)))
                                {
                                    code[start1] = (len1<<8) | CODE_JUMP_FALSE;
                                    code[start1+1] = CODE_ENTER_RESULT;
                                    code[start1+len1] = (code[start1+len1]&~CODE_RET_MASK) | retcodeany(rettype);
                                    break;
                                }
                                compileblock(code);
                            }
                            else
                            {
                                uint inst2 = code[start2], op2 = inst2&~CODE_RET_MASK, len2 = code.length() - (start2+1);
                                if(op2 == (CODE_BLOCK|(len2<<8)))
                                {
                                    if(op1 == (CODE_BLOCK|(len1<<8)))
                                    {
                                        code[start1] = ((start2-start1)<<8) | CODE_JUMP_FALSE;
                                        code[start1+1] = CODE_ENTER_RESULT;
                                        code[start1+len1] = (code[start1+len1]&~CODE_RET_MASK) | retcodeany(rettype);
                                        code[start2] = (len2<<8) | CODE_JUMP;
                                        code[start2+1] = CODE_ENTER_RESULT;
                                        code[start2+len2] = (code[start2+len2]&~CODE_RET_MASK) | retcodeany(rettype);
                                        break;
                                    }
                                    else if(op1 == (CODE_EMPTY|(len1<<8)))
                                    {
                                        code[start1] = CODE_NULL | (inst2&CODE_RET_MASK);
                                        code[start2] = (len2<<8) | CODE_JUMP_TRUE;
                                        code[start2+1] = CODE_ENTER_RESULT;
                                        code[start2+len2] = (code[start2+len2]&~CODE_RET_MASK) | retcodeany(rettype);
                                        break;
                                    }
                                }
                            }
                            code.add(CODE_COM|retcodeany(rettype)|(id->index<<8));
                        }
                    }
                    break;
                case ID_RESULT:
                    if(more) more = compilearg(code, p, VAL_ANY, prevargs);
                    code.add((more ? CODE_RESULT : CODE_NULL) | retcodeany(rettype));
                    break;
                case ID_NOT:
                    if(more) more = compilearg(code, p, VAL_CANY, prevargs);
                    code.add((more ? CODE_NOT : CODE_TRUE) | retcodeany(rettype));
                    break;
                case ID_AND:
                case ID_OR:
                    if(more) more = compilearg(code, p, VAL_COND, prevargs);
                    if(!more) { code.add((id->type == ID_AND ? CODE_TRUE : CODE_FALSE) | retcodeany(rettype)); }
                    else
                    {
                        numargs++;
                        int start = code.length(), end = start;
                        while(numargs < MAXARGS)
                        {
                            more = compilearg(code, p, VAL_COND, prevargs+numargs);
                            if(!more) break;
                            numargs++;
                            if((code[end]&~CODE_RET_MASK) != (CODE_BLOCK|(uint(code.length()-(end+1))<<8))) break;
                            end = code.length();
                        }
                        if(more)
                        {
                            while(numargs < MAXARGS && (more = compilearg(code, p, VAL_COND, prevargs+numargs))) numargs++;
                            code.add(CODE_COMV|retcodeany(rettype)|(numargs<<8)|(id->index<<13));
                        }
                        else
                        {
                            uint op = id->type == ID_AND ? CODE_JUMP_RESULT_FALSE : CODE_JUMP_RESULT_TRUE;
                            code.add(op);
                            end = code.length();
                            while(start+1 < end)
                            {
                                uint len = code[start]>>8;
                                code[start] = ((end-(start+1))<<8) | op;
                                code[start+1] = CODE_ENTER;
                                code[start+len] = (code[start+len]&~CODE_RET_MASK) | retcodeany(rettype);
                                start += len+1;
                            }
                        }
                    }
                    break;
                case ID_VAR:
                    if(!(more = compilearg(code, p, VAL_INT, prevargs))) code.add(CODE_PRINT|(id->index<<8));
                    else if(!(id->flags&IDF_HEX) || !(more = compilearg(code, p, VAL_INT, prevargs+1))) code.add(CODE_IVAR1|(id->index<<8));
                    else if(!(more = compilearg(code, p, VAL_INT, prevargs+2))) code.add(CODE_IVAR2|(id->index<<8));
                    else code.add(CODE_IVAR3|(id->index<<8));
                    break;
                case ID_FVAR:
                    if(!(more = compilearg(code, p, VAL_FLOAT, prevargs))) code.add(CODE_PRINT|(id->index<<8));
                    else code.add(CODE_FVAR1|(id->index<<8));
                    break;
                case ID_SVAR:
                    if(!(more = compilearg(code, p, VAL_CSTR, prevargs))) code.add(CODE_PRINT|(id->index<<8));
                    else
                    {
                        do ++numargs;
                        while(numargs < MAXARGS && (more = compilearg(code, p, VAL_CANY, prevargs+numargs)));
                        if(numargs > 1) code.add(CODE_CONC|RET_STR|(numargs<<8));
                        code.add(CODE_SVAR1|(id->index<<8));
                    }
                    break;
            }
        }
    endstatement:
        if(more) while(compilearg(code, p, VAL_POP));
        p += strcspn(p, ")];/\n\0");
        int c = *p++;
        switch(c)
        {
            case '\0':
                if(c != brak) debugcodeline(line, "missing \"%c\"", brak);
                p--;
                return;

            case ')':
            case ']':
                if(c == brak) return;
                debugcodeline(line, "unexpected \"%c\"", c);
                break;

            case '/':
                if(*p == '/') p += strcspn(p, "\n\0");
                goto endstatement;
        }
    }
}

static void compilemain(vector<uint> &code, const char *p, int rettype = VAL_ANY)
{
    code.add(CODE_START);
    compilestatements(code, p, VAL_ANY);
    code.add(CODE_EXIT|(rettype < VAL_ANY ? rettype<<CODE_RET : 0));
}

uint *compilecode(const char *p)
{
    vector<uint> buf;
    buf.reserve(64);
    compilemain(buf, p);
    uint *code = new uint[buf.length()];
    memcpy(code, buf.getbuf(), buf.length()*sizeof(uint));
    code[0] += 0x100;
    return code;
}

static inline const uint *forcecode(tagval &v)
{
    if(v.type != VAL_CODE)
    {
        vector<uint> buf;
        buf.reserve(64);
        compilemain(buf, v.getstr());
        freearg(v);
        v.setcode(buf.getbuf()+1);
        buf.disown();
    }
    return v.code;
}

static inline void forcecond(tagval &v)
{
    switch(v.type)
    {
        case VAL_STR: case VAL_MACRO: case VAL_CSTR:
            if(v.s[0]) forcecode(v);
            else v.setint(0);
            break;
    }
}

void keepcode(uint *code)
{
    if(!code) return;
    switch(*code&CODE_OP_MASK)
    {
        case CODE_START:
            *code += 0x100;
            return;
    }
    switch(code[-1]&CODE_OP_MASK)
    {
        case CODE_START:
            code[-1] += 0x100;
            break;
        case CODE_OFFSET:
            code -= int(code[-1]>>8);
            *code += 0x100;
            break;
    }
}

void freecode(uint *code)
{
    if(!code) return;
    switch(*code&CODE_OP_MASK)
    {
        case CODE_START:
            *code -= 0x100;
            if(int(*code) < 0x100) delete[] code;
            return;
    }
    switch(code[-1]&CODE_OP_MASK)
    {
        case CODE_START:
            code[-1] -= 0x100;
            if(int(code[-1]) < 0x100) delete[] &code[-1];
            break;
        case CODE_OFFSET:
            code -= int(code[-1]>>8);
            *code -= 0x100;
            if(int(*code) < 0x100) delete[] code;
            break;
    }
}

void printvar(ident *id, int i)
{
    if(i < 0) conoutf("%s = %d", id->name, i);
    else if(id->flags&IDF_HEX && id->maxval==0xFFFFFF)
        conoutf("%s = 0x%.6X (%d, %d, %d)", id->name, i, (i>>16)&0xFF, (i>>8)&0xFF, i&0xFF);
    else
        conoutf(id->flags&IDF_HEX ? "%s = 0x%X" : "%s = %d", id->name, i);
}

void printfvar(ident *id, float f)
{
    conoutf("%s = %s", id->name, floatstr(f));
}

void printsvar(ident *id, const char *s)
{
    conoutf(strchr(s, '"') ? "%s = [%s]" : "%s = \"%s\"", id->name, s);
}

void printvar(ident *id)
{
    switch(id->type)
    {
        case ID_VAR: printvar(id, *id->storage.i); break;
        case ID_FVAR: printfvar(id, *id->storage.f); break;
        case ID_SVAR: printsvar(id, *id->storage.s); break;
    }
}

typedef void (__cdecl *comfun)();
typedef void (__cdecl *comfun1)(void *);
typedef void (__cdecl *comfun2)(void *, void *);
typedef void (__cdecl *comfun3)(void *, void *, void *);
typedef void (__cdecl *comfun4)(void *, void *, void *, void *);
typedef void (__cdecl *comfun5)(void *, void *, void *, void *, void *);
typedef void (__cdecl *comfun6)(void *, void *, void *, void *, void *, void *);
typedef void (__cdecl *comfun7)(void *, void *, void *, void *, void *, void *, void *);
typedef void (__cdecl *comfun8)(void *, void *, void *, void *, void *, void *, void *, void *);
typedef void (__cdecl *comfun9)(void *, void *, void *, void *, void *, void *, void *, void *, void *);
typedef void (__cdecl *comfun10)(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *);
typedef void (__cdecl *comfun11)(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *);
typedef void (__cdecl *comfun12)(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *);
typedef void (__cdecl *comfunv)(tagval *, int);

static const uint *skipcode(const uint *code, tagval &result = noret)
{
    int depth = 0;
    for(;;)
    {
        uint op = *code++;
        switch(op&0xFF)
        {
            case CODE_MACRO:
            case CODE_VAL|RET_STR:
            {
                uint len = op>>8;
                code += len/sizeof(uint) + 1;
                continue;
            }
            case CODE_BLOCK:
            case CODE_JUMP:
            case CODE_JUMP_TRUE:
            case CODE_JUMP_FALSE:
            case CODE_JUMP_RESULT_TRUE:
            case CODE_JUMP_RESULT_FALSE:
            {
                uint len = op>>8;
                code += len;
                continue;
            }
            case CODE_ENTER:
            case CODE_ENTER_RESULT:
                ++depth;
                continue;
            case CODE_EXIT|RET_NULL: case CODE_EXIT|RET_STR: case CODE_EXIT|RET_INT: case CODE_EXIT|RET_FLOAT:
                if(depth <= 0)
                {
                    if(&result != &noret) forcearg(result, op&CODE_RET_MASK);
                    return code;
                }
                --depth;
                continue;
        }
    }
}

#ifndef STANDALONE
static inline uint *copycode(const uint *src)
{
    const uint *end = skipcode(src);
    size_t len = end - src;
    uint *dst = new uint[len + 1];
    *dst++ = CODE_START;
    memcpy(dst, src, len*sizeof(uint));
    return dst;
}

static inline void copyarg(tagval &dst, const tagval &src)
{
    switch(src.type)
    {
        case VAL_INT:
        case VAL_FLOAT:
        case VAL_IDENT:
            dst = src;
            break;
        case VAL_STR:
        case VAL_MACRO:
        case VAL_CSTR:
            dst.setstr(newstring(src.s));
            break;
        case VAL_CODE:
            dst.setcode(copycode(src.code));
            break;
        default:
            dst.setnull();
            break;
    }
}

static inline void addreleaseaction(ident *id, tagval *args, int numargs)
{
    tagval *dst = addreleaseaction(id, numargs+1);
    if(dst) { args[numargs].setint(1); loopi(numargs+1) copyarg(dst[i], args[i]); }
    else args[numargs].setint(0);
}
#endif

static inline void callcommand(ident *id, tagval *args, int numargs, bool lookup = false)
{
    int i = -1, fakeargs = 0;
    bool rep = false;
    for(const char *fmt = id->args; *fmt; fmt++) switch(*fmt)
    {
        case 'i': if(++i >= numargs) { if(rep) break; args[i].setint(0); fakeargs++; } else forceint(args[i]); break;
        case 'b': if(++i >= numargs) { if(rep) break; args[i].setint(INT_MIN); fakeargs++; } else forceint(args[i]); break;
        case 'f': if(++i >= numargs) { if(rep) break; args[i].setfloat(0.0f); fakeargs++; } else forcefloat(args[i]); break;
        case 'F': if(++i >= numargs) { if(rep) break; args[i].setfloat(args[i-1].getfloat()); fakeargs++; } else forcefloat(args[i]); break;
        case 'S': if(++i >= numargs) { if(rep) break; args[i].setstr(newstring("")); fakeargs++; } else forcestr(args[i]); break;
        case 's': if(++i >= numargs) { if(rep) break; args[i].setcstr(""); fakeargs++; } else forcestr(args[i]); break;
        case 'T':
        case 't': if(++i >= numargs) { if(rep) break; args[i].setnull(); fakeargs++; } break;
        case 'E': if(++i >= numargs) { if(rep) break; args[i].setnull(); fakeargs++; } else forcecond(args[i]); break;
        case 'e':
            if(++i >= numargs)
            {
                if(rep) break;
                args[i].setcode(emptyblock[VAL_NULL]+1);
                fakeargs++;
            }
            else forcecode(args[i]);
            break;
        case 'r': if(++i >= numargs) { if(rep) break; args[i].setident(dummyident); fakeargs++; } else forceident(args[i]); break;
        case '$': if(++i < numargs) freearg(args[i]); args[i].setident(id); break;
        case 'N': if(++i < numargs) freearg(args[i]); args[i].setint(lookup ? -1 : i-fakeargs); break;
#ifndef STANDALONE
        case 'D': if(++i < numargs) freearg(args[i]); addreleaseaction(id, args, i); fakeargs++; break;
#endif
        case 'C': { i = max(i+1, numargs); vector<char> buf; ((comfun1)id->fun)(conc(buf, args, i, true)); goto cleanup; }
        case 'V': i = max(i+1, numargs); ((comfunv)id->fun)(args, i); goto cleanup;
        case '1': case '2': case '3': case '4': if(i+1 < numargs) { fmt -= *fmt-'0'+1; rep = true; } break;
    }
    ++i;
    #define OFFSETARG(n) n
    #define ARG(n) (id->argmask&(1<<(n)) ? (void *)args[OFFSETARG(n)].s : (void *)&args[OFFSETARG(n)].i)
    #define CALLCOM(n) \
        switch(n) \
        { \
            case 0: ((comfun)id->fun)(); break; \
            case 1: ((comfun1)id->fun)(ARG(0)); break; \
            case 2: ((comfun2)id->fun)(ARG(0), ARG(1)); break; \
            case 3: ((comfun3)id->fun)(ARG(0), ARG(1), ARG(2)); break; \
            case 4: ((comfun4)id->fun)(ARG(0), ARG(1), ARG(2), ARG(3)); break; \
            case 5: ((comfun5)id->fun)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break; \
            case 6: ((comfun6)id->fun)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); break; \
            case 7: ((comfun7)id->fun)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6)); break; \
            case 8: ((comfun8)id->fun)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7)); break; \
            case 9: ((comfun9)id->fun)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8)); break; \
            case 10: ((comfun10)id->fun)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9)); break; \
            case 11: ((comfun11)id->fun)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9), ARG(10)); break; \
            case 12: ((comfun12)id->fun)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9), ARG(10), ARG(11)); break; \
        }
    CALLCOM(i)
    #undef OFFSETARG
cleanup:
    loopk(i) freearg(args[k]);
    for(; i < numargs; i++) freearg(args[i]);
}

#define MAXRUNDEPTH 255
static int rundepth = 0;

static const uint *runcode(const uint *code, tagval &result)
{
    result.setnull();
    if(rundepth >= MAXRUNDEPTH)
    {
        debugcode("exceeded recursion limit");
        return skipcode(code, result);
    }
    ++rundepth;
    int numargs = 0;
    tagval args[MAXARGS+MAXRESULTS], *prevret = commandret;
    commandret = &result;
    for(;;)
    {
        uint op = *code++;
        switch(op&0xFF)
        {
            case CODE_START: case CODE_OFFSET: continue;

            #define RETOP(op, val) \
                case op: \
                    freearg(result); \
                    val; \
                    continue;

            RETOP(CODE_NULL|RET_NULL, result.setnull())
            RETOP(CODE_NULL|RET_STR, result.setstr(newstring("")))
            RETOP(CODE_NULL|RET_INT, result.setint(0))
            RETOP(CODE_NULL|RET_FLOAT, result.setfloat(0.0f))

            RETOP(CODE_FALSE|RET_STR, result.setstr(newstring("0")))
            case CODE_FALSE|RET_NULL:
            RETOP(CODE_FALSE|RET_INT, result.setint(0))
            RETOP(CODE_FALSE|RET_FLOAT, result.setfloat(0.0f))

            RETOP(CODE_TRUE|RET_STR, result.setstr(newstring("1")))
            case CODE_TRUE|RET_NULL:
            RETOP(CODE_TRUE|RET_INT, result.setint(1))
            RETOP(CODE_TRUE|RET_FLOAT, result.setfloat(1.0f))

            #define RETPOP(op, val) \
                RETOP(op, { --numargs; val; freearg(args[numargs]); })

            RETPOP(CODE_NOT|RET_STR, result.setstr(newstring(getbool(args[numargs]) ? "0" : "1")))
            case CODE_NOT|RET_NULL:
            RETPOP(CODE_NOT|RET_INT, result.setint(getbool(args[numargs]) ? 0 : 1))
            RETPOP(CODE_NOT|RET_FLOAT, result.setfloat(getbool(args[numargs]) ? 0.0f : 1.0f))

            case CODE_POP:
                freearg(args[--numargs]);
                continue;
            case CODE_ENTER:
                code = runcode(code, args[numargs++]);
                continue;
            case CODE_ENTER_RESULT:
                freearg(result);
                code = runcode(code, result);
                continue;
            case CODE_EXIT|RET_STR: case CODE_EXIT|RET_INT: case CODE_EXIT|RET_FLOAT:
                forcearg(result, op&CODE_RET_MASK);
                // fall-through
            case CODE_EXIT|RET_NULL:
                goto exit;
            case CODE_RESULT_ARG|RET_STR: case CODE_RESULT_ARG|RET_INT: case CODE_RESULT_ARG|RET_FLOAT:
                forcearg(result, op&CODE_RET_MASK);
                // fall-through
            case CODE_RESULT_ARG|RET_NULL:
                args[numargs++] = result;
                result.setnull();
                continue;
            case CODE_PRINT:
                printvar(identmap[op>>8]);
                continue;

            case CODE_LOCAL:
            {
                freearg(result);
                int numlocals = op>>8, offset = numargs-numlocals;
                identstack locals[MAXARGS];
                loopi(numlocals) pushalias(*args[offset+i].id, locals[i]);
                code = runcode(code, result);
                for(int i = offset; i < numargs; i++) popalias(*args[i].id);
                goto exit;
            }

            case CODE_DO|RET_NULL: case CODE_DO|RET_STR: case CODE_DO|RET_INT: case CODE_DO|RET_FLOAT:
                freearg(result);
                runcode(args[--numargs].code, result);
                freearg(args[numargs]);
                forcearg(result, op&CODE_RET_MASK);
                continue;

            case CODE_JUMP:
            {
                uint len = op>>8;
                code += len;
                continue;
            }
            case CODE_JUMP_TRUE:
            {
                uint len = op>>8;
                if(getbool(args[--numargs])) code += len;
                freearg(args[numargs]);
                continue;
            }
            case CODE_JUMP_FALSE:
            {
                uint len = op>>8;
                if(!getbool(args[--numargs])) code += len;
                freearg(args[numargs]);
                continue;
            }
            case CODE_JUMP_RESULT_TRUE:
            {
                uint len = op>>8;
                freearg(result);
                --numargs;
                if(args[numargs].type == VAL_CODE) { runcode(args[numargs].code, result); freearg(args[numargs]); }
                else result = args[numargs];
                if(getbool(result)) code += len;
                continue;
            }
            case CODE_JUMP_RESULT_FALSE:
            {
                uint len = op>>8;
                freearg(result);
                --numargs;
                if(args[numargs].type == VAL_CODE) { runcode(args[numargs].code, result); freearg(args[numargs]); }
                else result = args[numargs];
                if(!getbool(result)) code += len;
                continue;
            }

            case CODE_MACRO:
            {
                uint len = op>>8;
                args[numargs++].setmacro(code);
                code += len/sizeof(uint) + 1;
                continue;
            }

            case CODE_VAL|RET_STR:
            {
                uint len = op>>8;
                args[numargs++].setstr(newstring((const char *)code, len));
                code += len/sizeof(uint) + 1;
                continue;
            }
            case CODE_VALI|RET_STR:
            {
                char s[4] = { char((op>>8)&0xFF), char((op>>16)&0xFF), char((op>>24)&0xFF), '\0' };
                args[numargs++].setstr(newstring(s));
                continue;
            }
            case CODE_VAL|RET_NULL:
            case CODE_VALI|RET_NULL: args[numargs++].setnull(); continue;
            case CODE_VAL|RET_INT: args[numargs++].setint(int(*code++)); continue;
            case CODE_VALI|RET_INT: args[numargs++].setint(int(op)>>8); continue;
            case CODE_VAL|RET_FLOAT: args[numargs++].setfloat(*(const float *)code++); continue;
            case CODE_VALI|RET_FLOAT: args[numargs++].setfloat(float(int(op)>>8)); continue;

            case CODE_DUP|RET_NULL: args[numargs-1].getval(args[numargs]); numargs++; continue;
            case CODE_DUP|RET_INT: args[numargs].setint(args[numargs-1].getint()); numargs++; continue;
            case CODE_DUP|RET_FLOAT: args[numargs].setfloat(args[numargs-1].getfloat()); numargs++; continue;
            case CODE_DUP|RET_STR: args[numargs].setstr(newstring(args[numargs-1].getstr())); numargs++; continue;

            case CODE_FORCE|RET_STR: forcestr(args[numargs-1]); continue;
            case CODE_FORCE|RET_INT: forceint(args[numargs-1]); continue;
            case CODE_FORCE|RET_FLOAT: forcefloat(args[numargs-1]); continue;

            case CODE_RESULT|RET_NULL:
                freearg(result);
                result = args[--numargs];
                continue;
            case CODE_RESULT|RET_STR: case CODE_RESULT|RET_INT: case CODE_RESULT|RET_FLOAT:
                freearg(result);
                result = args[--numargs];
                forcearg(result, op&CODE_RET_MASK);
                continue;

            case CODE_EMPTY|RET_NULL: args[numargs++].setcode(emptyblock[VAL_NULL]+1); break;
            case CODE_EMPTY|RET_STR: args[numargs++].setcode(emptyblock[VAL_STR]+1); break;
            case CODE_EMPTY|RET_INT: args[numargs++].setcode(emptyblock[VAL_INT]+1); break;
            case CODE_EMPTY|RET_FLOAT: args[numargs++].setcode(emptyblock[VAL_FLOAT]+1); break;
            case CODE_BLOCK:
            {
                uint len = op>>8;
                args[numargs++].setcode(code+1);
                code += len;
                continue;
            }
            case CODE_COMPILE:
            {
                tagval &arg = args[numargs-1];
                vector<uint> buf;
                switch(arg.type)
                {
                    case VAL_INT: buf.reserve(8); buf.add(CODE_START); compileint(buf, arg.i); buf.add(CODE_RESULT); buf.add(CODE_EXIT); break;
                    case VAL_FLOAT: buf.reserve(8); buf.add(CODE_START); compilefloat(buf, arg.f); buf.add(CODE_RESULT); buf.add(CODE_EXIT); break;
                    case VAL_STR: case VAL_MACRO: case VAL_CSTR: buf.reserve(64); compilemain(buf, arg.s); freearg(arg); break;
                    default: buf.reserve(8); buf.add(CODE_START); compilenull(buf); buf.add(CODE_RESULT); buf.add(CODE_EXIT); break;
                }
                arg.setcode(buf.getbuf()+1);
                buf.disown();
                continue;
            }
            case CODE_COND:
            {
                tagval &arg = args[numargs-1];
                switch(arg.type)
                {
                    case VAL_STR: case VAL_MACRO: case VAL_CSTR:
                        if(arg.s[0])
                        {
                            vector<uint> buf;
                            buf.reserve(64);
                            compilemain(buf, arg.s);
                            freearg(arg);
                            arg.setcode(buf.getbuf()+1);
                            buf.disown();
                        }
                        else forcenull(arg);
                        break;
                }
                continue;
            }

            case CODE_IDENT:
                args[numargs++].setident(identmap[op>>8]);
                continue;
            case CODE_IDENTARG:
            {
                ident *id = identmap[op>>8];
                if(!(aliasstack->usedargs&(1<<id->index)))
                {
                    pusharg(*id, nullval, aliasstack->argstack[id->index]);
                    aliasstack->usedargs |= 1<<id->index;
                }
                args[numargs++].setident(id);
                continue;
            }
            case CODE_IDENTU:
            {
                tagval &arg = args[numargs-1];
                ident *id = arg.type == VAL_STR || arg.type == VAL_MACRO || arg.type == VAL_CSTR ? newident(arg.s, IDF_UNKNOWN) : dummyident;
                if(id->index < MAXARGS && !(aliasstack->usedargs&(1<<id->index)))
                {
                    pusharg(*id, nullval, aliasstack->argstack[id->index]);
                    aliasstack->usedargs |= 1<<id->index;
                }
                freearg(arg);
                arg.setident(id);
                continue;
            }

            case CODE_LOOKUPU|RET_STR:
                #define LOOKUPU(aval, sval, ival, fval, nval) { \
                    tagval &arg = args[numargs-1]; \
                    if(arg.type != VAL_STR && arg.type != VAL_MACRO && arg.type != VAL_CSTR) continue; \
                    ident *id = idents.access(arg.s); \
                    if(id) switch(id->type) \
                    { \
                        case ID_ALIAS: \
                            if(id->flags&IDF_UNKNOWN) break; \
                            freearg(arg); \
                            if(id->index < MAXARGS && !(aliasstack->usedargs&(1<<id->index))) { nval; continue; } \
                            aval; \
                            continue; \
                        case ID_SVAR: freearg(arg); sval; continue; \
                        case ID_VAR: freearg(arg); ival; continue; \
                        case ID_FVAR: freearg(arg); fval; continue; \
                        case ID_COMMAND: \
                        { \
                            freearg(arg); \
                            arg.setnull(); \
                            commandret = &arg; \
                            tagval buf[MAXARGS]; \
                            callcommand(id, buf, 0, true); \
                            forcearg(arg, op&CODE_RET_MASK); \
                            commandret = &result; \
                            continue; \
                        } \
                        default: freearg(arg); nval; continue; \
                    } \
                    debugcode("unknown alias lookup: %s", arg.s); \
                    freearg(arg); \
                    nval; \
                    continue; \
                }
                LOOKUPU(arg.setstr(newstring(id->getstr())),
                        arg.setstr(newstring(*id->storage.s)),
                        arg.setstr(newstring(intstr(*id->storage.i))),
                        arg.setstr(newstring(floatstr(*id->storage.f))),
                        arg.setstr(newstring("")));
            case CODE_LOOKUP|RET_STR:
                #define LOOKUP(aval) { \
                    ident *id = identmap[op>>8]; \
                    if(id->flags&IDF_UNKNOWN) debugcode("unknown alias lookup: %s", id->name); \
                    aval; \
                    continue; \
                }
                LOOKUP(args[numargs++].setstr(newstring(id->getstr())));
            case CODE_LOOKUPARG|RET_STR:
                #define LOOKUPARG(aval, nval) { \
                    ident *id = identmap[op>>8]; \
                    if(!(aliasstack->usedargs&(1<<id->index))) { nval; continue; } \
                    aval; \
                    continue; \
                }
                LOOKUPARG(args[numargs++].setstr(newstring(id->getstr())), args[numargs++].setstr(newstring("")));
            case CODE_LOOKUPU|RET_INT:
                LOOKUPU(arg.setint(id->getint()),
                        arg.setint(parseint(*id->storage.s)),
                        arg.setint(*id->storage.i),
                        arg.setint(int(*id->storage.f)),
                        arg.setint(0));
            case CODE_LOOKUP|RET_INT:
                LOOKUP(args[numargs++].setint(id->getint()));
            case CODE_LOOKUPARG|RET_INT:
                LOOKUPARG(args[numargs++].setint(id->getint()), args[numargs++].setint(0));
            case CODE_LOOKUPU|RET_FLOAT:
                LOOKUPU(arg.setfloat(id->getfloat()),
                        arg.setfloat(parsefloat(*id->storage.s)),
                        arg.setfloat(float(*id->storage.i)),
                        arg.setfloat(*id->storage.f),
                        arg.setfloat(0.0f));
            case CODE_LOOKUP|RET_FLOAT:
                LOOKUP(args[numargs++].setfloat(id->getfloat()));
            case CODE_LOOKUPARG|RET_FLOAT:
                LOOKUPARG(args[numargs++].setfloat(id->getfloat()), args[numargs++].setfloat(0.0f));
            case CODE_LOOKUPU|RET_NULL:
                LOOKUPU(id->getval(arg),
                        arg.setstr(newstring(*id->storage.s)),
                        arg.setint(*id->storage.i),
                        arg.setfloat(*id->storage.f),
                        arg.setnull());
            case CODE_LOOKUP|RET_NULL:
                LOOKUP(id->getval(args[numargs++]));
            case CODE_LOOKUPARG|RET_NULL:
                LOOKUPARG(id->getval(args[numargs++]), args[numargs++].setnull());

            case CODE_LOOKUPMU|RET_STR:
                LOOKUPU(id->getcstr(arg),
                        arg.setcstr(*id->storage.s),
                        arg.setstr(newstring(intstr(*id->storage.i))),
                        arg.setstr(newstring(floatstr(*id->storage.f))),
                        arg.setcstr(""));
            case CODE_LOOKUPM|RET_STR:
                LOOKUP(id->getcstr(args[numargs++]));
            case CODE_LOOKUPMARG|RET_STR:
                LOOKUPARG(id->getcstr(args[numargs++]), args[numargs++].setcstr(""));
            case CODE_LOOKUPMU|RET_NULL:
                LOOKUPU(id->getcval(arg),
                        arg.setcstr(*id->storage.s),
                        arg.setint(*id->storage.i),
                        arg.setfloat(*id->storage.f),
                        arg.setnull());
            case CODE_LOOKUPM|RET_NULL:
                LOOKUP(id->getcval(args[numargs++]));
            case CODE_LOOKUPMARG|RET_NULL:
                LOOKUPARG(id->getcval(args[numargs++]), args[numargs++].setnull());

            case CODE_SVAR|RET_STR: case CODE_SVAR|RET_NULL: args[numargs++].setstr(newstring(*identmap[op>>8]->storage.s)); continue;
            case CODE_SVAR|RET_INT: args[numargs++].setint(parseint(*identmap[op>>8]->storage.s)); continue;
            case CODE_SVAR|RET_FLOAT: args[numargs++].setfloat(parsefloat(*identmap[op>>8]->storage.s)); continue;
            case CODE_SVARM: args[numargs++].setcstr(*identmap[op>>8]->storage.s); continue;
            case CODE_SVAR1: setsvarchecked(identmap[op>>8], args[--numargs].s); freearg(args[numargs]); continue;

            case CODE_IVAR|RET_INT: case CODE_IVAR|RET_NULL: args[numargs++].setint(*identmap[op>>8]->storage.i); continue;
            case CODE_IVAR|RET_STR: args[numargs++].setstr(newstring(intstr(*identmap[op>>8]->storage.i))); continue;
            case CODE_IVAR|RET_FLOAT: args[numargs++].setfloat(float(*identmap[op>>8]->storage.i)); continue;
            case CODE_IVAR1: setvarchecked(identmap[op>>8], args[--numargs].i); continue;
            case CODE_IVAR2: numargs -= 2; setvarchecked(identmap[op>>8], (args[numargs].i<<16)|(args[numargs+1].i<<8)); continue;
            case CODE_IVAR3: numargs -= 3; setvarchecked(identmap[op>>8], (args[numargs].i<<16)|(args[numargs+1].i<<8)|args[numargs+2].i); continue;

            case CODE_FVAR|RET_FLOAT: case CODE_FVAR|RET_NULL: args[numargs++].setfloat(*identmap[op>>8]->storage.f); continue;
            case CODE_FVAR|RET_STR: args[numargs++].setstr(newstring(floatstr(*identmap[op>>8]->storage.f))); continue;
            case CODE_FVAR|RET_INT: args[numargs++].setint(int(*identmap[op>>8]->storage.f)); continue;
            case CODE_FVAR1: setfvarchecked(identmap[op>>8], args[--numargs].f); continue;

            #define OFFSETARG(n) offset+n
            case CODE_COM|RET_NULL: case CODE_COM|RET_STR: case CODE_COM|RET_FLOAT: case CODE_COM|RET_INT:
            {
                ident *id = identmap[op>>8];
                int offset = numargs-id->numargs;
                forcenull(result);
                CALLCOM(id->numargs)
                forcearg(result, op&CODE_RET_MASK);
                freeargs(args, numargs, offset);
                continue;
            }
#ifndef STANDALONE
            case CODE_COMD|RET_NULL: case CODE_COMD|RET_STR: case CODE_COMD|RET_FLOAT: case CODE_COMD|RET_INT:
            {
                ident *id = identmap[op>>8];
                int offset = numargs-(id->numargs-1);
                addreleaseaction(id, &args[offset], id->numargs-1);
                CALLCOM(id->numargs)
                forcearg(result, op&CODE_RET_MASK);
                freeargs(args, numargs, offset);
                continue;
            }
#endif
            #undef OFFSETARG

            case CODE_COMV|RET_NULL: case CODE_COMV|RET_STR: case CODE_COMV|RET_FLOAT: case CODE_COMV|RET_INT:
            {
                ident *id = identmap[op>>13];
                int callargs = (op>>8)&0x1F, offset = numargs-callargs;
                forcenull(result);
                ((comfunv)id->fun)(&args[offset], callargs);
                forcearg(result, op&CODE_RET_MASK);
                freeargs(args, numargs, offset);
                continue;
            }
            case CODE_COMC|RET_NULL: case CODE_COMC|RET_STR: case CODE_COMC|RET_FLOAT: case CODE_COMC|RET_INT:
            {
                ident *id = identmap[op>>13];
                int callargs = (op>>8)&0x1F, offset = numargs-callargs;
                forcenull(result);
                {
                    vector<char> buf;
                    buf.reserve(MAXSTRLEN);
                    ((comfun1)id->fun)(conc(buf, &args[offset], callargs, true));
                }
                forcearg(result, op&CODE_RET_MASK);
                freeargs(args, numargs, offset);
                continue;
            }

            case CODE_CONC|RET_NULL: case CODE_CONC|RET_STR: case CODE_CONC|RET_FLOAT: case CODE_CONC|RET_INT:
            case CODE_CONCW|RET_NULL: case CODE_CONCW|RET_STR: case CODE_CONCW|RET_FLOAT: case CODE_CONCW|RET_INT:
            {
                int numconc = op>>8;
                char *s = conc(&args[numargs-numconc], numconc, (op&CODE_OP_MASK)==CODE_CONC);
                freeargs(args, numargs, numargs-numconc);
                args[numargs].setstr(s);
                forcearg(args[numargs], op&CODE_RET_MASK);
                numargs++;
                continue;
            }

            case CODE_CONCM|RET_NULL: case CODE_CONCM|RET_STR: case CODE_CONCM|RET_FLOAT: case CODE_CONCM|RET_INT:
            {
                int numconc = op>>8;
                char *s = conc(&args[numargs-numconc], numconc, false);
                freeargs(args, numargs, numargs-numconc);
                result.setstr(s);
                forcearg(result, op&CODE_RET_MASK);
                continue;
            }

            case CODE_ALIAS:
                setalias(*identmap[op>>8], args[--numargs]);
                continue;
            case CODE_ALIASARG:
                setarg(*identmap[op>>8], args[--numargs]);
                continue;
            case CODE_ALIASU:
                numargs -= 2;
                setalias(args[numargs].getstr(), args[numargs+1]);
                freearg(args[numargs]);
                continue;

            #define SKIPARGS(offset) offset
            case CODE_CALL|RET_NULL: case CODE_CALL|RET_STR: case CODE_CALL|RET_FLOAT: case CODE_CALL|RET_INT:
            {
                #define FORCERESULT { \
                    freeargs(args, numargs, SKIPARGS(offset)); \
                    forcearg(result, op&CODE_RET_MASK); \
                    continue; \
                }
                #define CALLALIAS { \
                    identstack argstack[MAXARGS]; \
                    for(int i = 0; i < callargs; i++) \
                        pusharg(*identmap[i], args[offset + i], argstack[i]); \
                    int oldargs = _numargs; \
                    _numargs = callargs; \
                    int oldflags = identflags; \
                    identflags |= id->flags&IDF_OVERRIDDEN; \
                    identlink aliaslink = { id, aliasstack, (1<<callargs)-1, argstack }; \
                    aliasstack = &aliaslink; \
                    if(!id->code) id->code = compilecode(id->getstr()); \
                    uint *code = id->code; \
                    code[0] += 0x100; \
                    runcode(code+1, result); \
                    code[0] -= 0x100; \
                    if(int(code[0]) < 0x100) delete[] code; \
                    aliasstack = aliaslink.next; \
                    identflags = oldflags; \
                    for(int i = 0; i < callargs; i++) \
                        poparg(*identmap[i]); \
                    for(int argmask = aliaslink.usedargs&(~0<<callargs), i = callargs; argmask; i++) \
                        if(argmask&(1<<i)) { poparg(*identmap[i]); argmask &= ~(1<<i); } \
                    forcearg(result, op&CODE_RET_MASK); \
                    _numargs = oldargs; \
                    numargs = SKIPARGS(offset); \
                }
                forcenull(result);
                ident *id = identmap[op>>13];
                int callargs = (op>>8)&0x1F, offset = numargs-callargs;
                if(id->flags&IDF_UNKNOWN)
                {
                    debugcode("unknown command: %s", id->name);
                    FORCERESULT;
                }
                CALLALIAS;
                continue;
            }
            case CODE_CALLARG|RET_NULL: case CODE_CALLARG|RET_STR: case CODE_CALLARG|RET_FLOAT: case CODE_CALLARG|RET_INT:
            {
                forcenull(result);
                ident *id = identmap[op>>13];
                int callargs = (op>>8)&0x1F, offset = numargs-callargs;
                if(!(aliasstack->usedargs&(1<<id->index))) FORCERESULT;
                CALLALIAS;
                continue;
            }
            #undef SKIPARGS

            #define SKIPARGS(offset) offset-1
            case CODE_CALLU|RET_NULL: case CODE_CALLU|RET_STR: case CODE_CALLU|RET_FLOAT: case CODE_CALLU|RET_INT:
            {
                int callargs = op>>8, offset = numargs-callargs;
                tagval &idarg = args[offset-1];
                if(idarg.type != VAL_STR && idarg.type != VAL_MACRO && idarg.type != VAL_CSTR)
                {
                litval:
                    freearg(result);
                    result = idarg;
                    forcearg(result, op&CODE_RET_MASK);
                    while(--numargs >= offset) freearg(args[numargs]);
                    continue;
                }
                ident *id = idents.access(idarg.s);
                if(!id)
                {
                noid:
                    if(checknumber(idarg.s)) goto litval;
                    debugcode("unknown command: %s", idarg.s);
                    forcenull(result);
                    FORCERESULT;
                }
                forcenull(result);
                switch(id->type)
                {
                    default:
                        if(!id->fun) FORCERESULT;
                        // fall-through
                    case ID_COMMAND:
                        freearg(idarg);
                        callcommand(id, &args[offset], callargs);
                        forcearg(result, op&CODE_RET_MASK);
                        numargs = offset - 1;
                        continue;
                    case ID_LOCAL:
                    {
                        identstack locals[MAXARGS];
                        freearg(idarg);
                        loopj(callargs) pushalias(*forceident(args[offset+j]), locals[j]);
                        code = runcode(code, result);
                        loopj(callargs) popalias(*args[offset+j].id);
                        goto exit;
                    }
                    case ID_VAR:
                        if(callargs <= 0) printvar(id); else setvarchecked(id, &args[offset], callargs);
                        FORCERESULT;
                    case ID_FVAR:
                        if(callargs <= 0) printvar(id); else setfvarchecked(id, forcefloat(args[offset]));
                        FORCERESULT;
                    case ID_SVAR:
                        if(callargs <= 0) printvar(id); else setsvarchecked(id, forcestr(args[offset]));
                        FORCERESULT;
                    case ID_ALIAS:
                        if(id->index < MAXARGS && !(aliasstack->usedargs&(1<<id->index))) FORCERESULT;
                        if(id->valtype==VAL_NULL) goto noid;
                        freearg(idarg);
                        CALLALIAS;
                        continue;
                }
            }
            #undef SKIPARGS
        }
    }
exit:
    commandret = prevret;
    --rundepth;
    return code;
}

void executeret(const uint *code, tagval &result)
{
    runcode(code, result);
}

void executeret(const char *p, tagval &result)
{
    vector<uint> code;
    code.reserve(64);
    compilemain(code, p, VAL_ANY);
    runcode(code.getbuf()+1, result);
    if(int(code[0]) >= 0x100) code.disown();
}

void executeret(ident *id, tagval *args, int numargs, tagval &result)
{
    result.setnull();
    ++rundepth;
    tagval *prevret = commandret;
    commandret = &result;
    if(rundepth > MAXRUNDEPTH) debugcode("exceeded recursion limit");
    else if(id) switch(id->type)
    {
        default:
            if(!id->fun) break;
            // fall-through
        case ID_COMMAND:
            if(numargs < id->numargs)
            {
                tagval buf[MAXARGS];
                memcpy(buf, args, numargs*sizeof(tagval));
                callcommand(id, buf, numargs);
            }
            else callcommand(id, args, numargs);
            numargs = 0;
            break;
        case ID_VAR:
            if(numargs <= 0) printvar(id); else setvarchecked(id, args, numargs);
            break;
        case ID_FVAR:
            if(numargs <= 0) printvar(id); else setfvarchecked(id, forcefloat(args[0]));
            break;
        case ID_SVAR:
            if(numargs <= 0) printvar(id); else setsvarchecked(id, forcestr(args[0]));
            break;
        case ID_ALIAS:
            if(id->index < MAXARGS && !(aliasstack->usedargs&(1<<id->index))) break;
            if(id->valtype==VAL_NULL) break;
            #define callargs numargs
            #define offset 0
            #define op RET_NULL
            #define SKIPARGS(offset) offset
            CALLALIAS;
            #undef callargs
            #undef offset
            #undef op
            #undef SKIPARGS
            break;
    }
    freeargs(args, numargs, 0);
    commandret = prevret;
    --rundepth;
}

char *executestr(const uint *code)
{
    tagval result;
    runcode(code, result);
    if(result.type == VAL_NULL) return NULL;
    forcestr(result);
    return result.s;
}

char *executestr(const char *p)
{
    tagval result;
    executeret(p, result);
    if(result.type == VAL_NULL) return NULL;
    forcestr(result);
    return result.s;
}

char *executestr(ident *id, tagval *args, int numargs)
{
    tagval result;
    executeret(id, args, numargs, result);
    if(result.type == VAL_NULL) return NULL;
    forcestr(result);
    return result.s;
}

char *execidentstr(const char *name)
{
    ident *id = idents.access(name);
    return id ? executestr(id, NULL, 0) : NULL;
}

int execute(const uint *code)
{
    tagval result;
    runcode(code, result);
    int i = result.getint();
    freearg(result);
    return i;
}

int execute(const char *p)
{
    vector<uint> code;
    code.reserve(64);
    compilemain(code, p, VAL_INT);
    tagval result;
    runcode(code.getbuf()+1, result);
    if(int(code[0]) >= 0x100) code.disown();
    int i = result.getint();
    freearg(result);
    return i;
}

int execute(ident *id, tagval *args, int numargs)
{
    tagval result;
    executeret(id, args, numargs, result);
    int i = result.getint();
    freearg(result);
    return i;
}

int execident(const char *name, int noid)
{
    ident *id = idents.access(name);
    return id ? execute(id, NULL, 0) : noid;
}

bool executebool(const uint *code)
{
    tagval result;
    runcode(code, result);
    bool b = getbool(result);
    freearg(result);
    return b;
}

bool executebool(const char *p)
{
    tagval result;
    executeret(p, result);
    bool b = getbool(result);
    freearg(result);
    return b;
}

bool executebool(ident *id, tagval *args, int numargs)
{
    tagval result;
    executeret(id, args, numargs, result);
    bool b = getbool(result);
    freearg(result);
    return b;
}

bool execidentbool(const char *name, bool noid)
{
    ident *id = idents.access(name);
    return id ? executebool(id, NULL, 0) : noid;
}

bool execfile(const char *cfgfile, bool msg)
{
    /*const char *paths[] = {"", "config/"};
    string s;
    loopi(sizeof(paths)/sizeof(paths[0]))
    {
        formatstring(s, "%s%s", paths[i], cfgfile);
        char *buf = loadfile(path(s), NULL);
        if(buf)
        {
            execute(buf);
            delete[] buf;
            return true;
        }
    }
    if(msg) conoutf(CON_ERROR, "could not read \"%s\"", cfgfile);
    return false; */

    string s;
    copystring(s, cfgfile);
    char *buf = loadfile(path(s), NULL);
    if(!buf)
    {
        if(msg) conoutf(CON_ERROR, "could not read \"%s\"", cfgfile);
        return false;
    }
    const char *oldsourcefile = sourcefile, *oldsourcestr = sourcestr;
    sourcefile = cfgfile;
    sourcestr = buf;
    execute(buf);
    sourcefile = oldsourcefile;
    sourcestr = oldsourcestr;
    delete[] buf;
    return true;
}
ICOMMAND(exec, "s", (char *file), execfile(file));

const char *escapestring(const char *s)
{
    stridx = (stridx + 1)%4;
    vector<char> &buf = strbuf[stridx];
    buf.setsize(0);
    buf.add('"');
    for(; *s; s++) switch(*s)
    {
        case '\n': buf.put("^n", 2); break;
        case '\t': buf.put("^t", 2); break;
        case '\f': buf.put("^f", 2); break;
        case '"': buf.put("^\"", 2); break;
        case '^': buf.put("^^", 2); break;
        default: buf.add(*s); break;
    }
    buf.put("\"\0", 2);
    return buf.getbuf();
}

ICOMMAND(escape, "s", (char *s), result(escapestring(s)));
ICOMMAND(unescape, "s", (char *s),
{
    int len = strlen(s);
    char *d = newstring(len);
    unescapestring(d, s, &s[len]);
    stringret(d);
});

const char *escapeid(const char *s)
{
    const char *end = s + strcspn(s, "\"/;()[]@ \f\t\r\n\0");
    return *end ? escapestring(s) : s;
}

bool validateblock(const char *s)
{
    const int maxbrak = 100;
    static char brakstack[maxbrak];
    int brakdepth = 0;
    for(; *s; s++) switch(*s)
    {
        case '[': case '(': if(brakdepth >= maxbrak) return false; brakstack[brakdepth++] = *s; break;
        case ']': if(brakdepth <= 0 || brakstack[--brakdepth] != '[') return false; break;
        case ')': if(brakdepth <= 0 || brakstack[--brakdepth] != '(') return false; break;
        case '"': s = parsestring(s + 1); if(*s != '"') return false; break;
        case '/': if(s[1] == '/') return false; break;
        case '@': case '\f': return false;
    }
    return brakdepth == 0;
}

#ifndef STANDALONE
void writecfg(const char *name)
{
    defformatstring(confname, "config_%s.cfg", game::gameident());
    stream *f = openutf8file(path((name && name[0]) ? name : confname, true), "w");

    if(!f) return;
    f->printf("// automatically written on exit, DO NOT MODIFY\n// delete this file to have config/defaults.cfg and config/%s/defaults.cfg overwrite these settings\n// modify settings in game, or put settings in %s to override anything\n\n", game::gameident(), game::autoexec());
    f->printf("if (&& (! $ignoreconfver) (!= $confver %i)) [\n\tsleep 500 [showgui keepconf]; setdefaults\n] [\n\n", confver);

    writecrosshairs(f);
    vector<ident *> ids;
    enumerate(idents, ident, id, ids.add(&id));
    ids.sortname();
    loopv(ids)
    {
        ident &id = *ids[i];
        if(id.flags&IDF_PERSIST && !(id.flags&IDF_OVERRIDDEN)) switch(id.type)
        {
            case ID_VAR:  f->printf("%s %d\n", escapeid(id.name), *id.storage.i); break;
            case ID_FVAR: f->printf("%s %s\n", escapeid(id.name), floatstr(*id.storage.f)); break;
            case ID_SVAR: f->printf("%s %s\n", escapeid(id.name), escapestring(*id.storage.s)); break;
        }
    }
    f->printf("\n");
    writebinds(f);
    f->printf("\n");
    loopv(ids)
    {
        ident &id = *ids[i];
        if(id.type==ID_ALIAS && id.flags&IDF_PERSIST && !(id.flags&IDF_OVERRIDDEN)) switch(id.valtype)
        {
        case VAL_STR:
            if(!id.val.s[0]) break;
            if(!validateblock(id.val.s)) { f->printf("%s = %s\n", escapeid(id), escapestring(id.val.s)); break; }
        case VAL_FLOAT:
        case VAL_INT:
            f->printf("%s = [%s]\n", escapeid(id), id.getstr()); break;
        }
    }
    f->printf("\n");
    writecompletions(f);
    f->printf("\n]\n\n");
    game::writeclientinfo(f);
    delete f;
}

COMMAND(writecfg, "s");
#endif

// below the commands that implement a small imperative language. thanks to the semantics of
// () and [] expressions, any control construct can be defined trivially.

static string retbuf[4];
static int retidx = 0;

const char *intstr(int v)
{
    retidx = (retidx + 1)%4;
    intformat(retbuf[retidx], v);
    return retbuf[retidx];
}

void intret(int v)
{
    commandret->setint(v);
}

const char *floatstr(float v)
{
    retidx = (retidx + 1)%4;
    floatformat(retbuf[retidx], v);
    return retbuf[retidx];
}

void floatret(float v)
{
    commandret->setfloat(v);
}

#undef ICOMMANDNAME
#define ICOMMANDNAME(name) _stdcmd
#undef ICOMMANDSNAME
#define ICOMMANDSNAME _stdcmd

ICOMMANDK(do, ID_DO, "e", (uint *body), executeret(body, *commandret));
ICOMMANDK(if, ID_IF, "tee", (tagval *cond, uint *t, uint *f), executeret(getbool(*cond) ? t : f, *commandret));
ICOMMAND(?, "tTT", (tagval *cond, tagval *t, tagval *f), result(*(getbool(*cond) ? t : f)));

ICOMMAND(pushif, "rTe", (ident *id, tagval *v, uint *code),
{
    if(id->type != ID_ALIAS || id->index < MAXARGS) return;
    if(getbool(*v))
    {
        identstack stack;
        pusharg(*id, *v, stack);
        v->type = VAL_NULL;
        id->flags &= ~IDF_UNKNOWN;
        executeret(code, *commandret);
        poparg(*id);
    }
});

void loopiter(ident *id, identstack &stack, tagval &v)
{
    if(id->stack != &stack)
    {
        pusharg(*id, v, stack);
        id->flags &= ~IDF_UNKNOWN;
    }
    else
    {
        if(id->valtype == VAL_STR) delete[] id->val.s;
        cleancode(*id);
        id->setval(v);
    }
}

void loopend(ident *id, identstack &stack)
{
    if(id->stack == &stack) poparg(*id);
}

static inline void setiter(ident &id, int i, identstack &stack)
{
    if(i)
    {
        if(id.valtype != VAL_INT)
        {
            if(id.valtype == VAL_STR) delete[] id.val.s;
            cleancode(id);
            id.valtype = VAL_INT;
        }
        id.val.i = i;
    }
    else
    {
        tagval zero;
        zero.setint(0);
        pusharg(id, zero, stack);
        id.flags &= ~IDF_UNKNOWN;
    }
}

ICOMMAND(loop, "rie", (ident *id, int *n, uint *body),
{
    if(*n <= 0 || id->type!=ID_ALIAS) return;
    identstack stack;
    loopi(*n)
    {
        setiter(*id, i, stack);
        execute(body);
    }
    poparg(*id);
});
ICOMMAND(loopwhile, "riee", (ident *id, int *n, uint *cond, uint *body),
{
    if(*n <= 0 || id->type!=ID_ALIAS) return;
    identstack stack;
    loopi(*n)
    {
        setiter(*id, i, stack);
        if(!executebool(cond)) break;
        execute(body);
    }
    poparg(*id);
});
ICOMMAND(while, "ee", (uint *cond, uint *body), while(executebool(cond)) execute(body));

char *loopconc(ident *id, int n, uint *body, bool space)
{
    identstack stack;
    vector<char> s;
    loopi(n)
    {
        setiter(*id, i, stack);
        tagval v;
        executeret(body, v);
        const char *vstr = v.getstr();
        int len = strlen(vstr);
        if(space && i) s.add(' ');
        s.put(vstr, len);
        freearg(v);
    }
    if(n > 0) poparg(*id);
    s.add('\0');
    return newstring(s.getbuf(), s.length()-1);
}

ICOMMAND(loopconcat, "rie", (ident *id, int *n, uint *body),
{
    if(*n > 0 && id->type==ID_ALIAS) commandret->setstr(loopconc(id, *n, body, true));
});

ICOMMAND(loopconcatword, "rie", (ident *id, int *n, uint *body),
{
    if(*n > 0 && id->type==ID_ALIAS) commandret->setstr(loopconc(id, *n, body, false));
});

void concat(tagval *v, int n)
{
    commandret->setstr(conc(v, n, true));
}
COMMAND(concat, "V");

void concatword(tagval *v, int n)
{
    commandret->setstr(conc(v, n, false));
}
COMMAND(concatword, "V");

void result(tagval &v)
{
    *commandret = v;
    v.type = VAL_NULL;
}

void stringret(char *s)
{
    commandret->setstr(s);
}

void result(const char *s)
{
    commandret->setstr(newstring(s));
}

ICOMMANDK(result, ID_RESULT, "T", (tagval *v),
{
    *commandret = *v;
    v->type = VAL_NULL;
});

void format(tagval *args, int numargs)
{
    vector<char> s;
    const char *f = args[0].getstr();
    while(*f)
    {
        int c = *f++;
        if(c == '%')
        {
            int i = *f++;
            if(i >= '1' && i <= '9')
            {
                i -= '0';
                const char *sub = i < numargs ? args[i].getstr() : "";
                while(*sub) s.add(*sub++);
            }
            else s.add(i);
        }
        else s.add(c);
    }
    s.add('\0');
    result(s.getbuf());
}
COMMAND(format, "V");

static const char *liststart = NULL, *listend = NULL, *listquotestart = NULL, *listquoteend = NULL;

static inline void skiplist(const char *&p)
{
    for(;;)
    {
        p += strspn(p, " \t\r\n");
        if(p[0]!='/' || p[1]!='/') break;
        p += strcspn(p, "\n\0");
    }
}

static bool parselist(const char *&s, const char *&start = liststart, const char *&end = listend, const char *&quotestart = listquotestart, const char *&quoteend = listquoteend)
{
    skiplist(s);
    switch(*s)
    {
        case '"': quotestart = s++; start = s; s = parsestring(s); end = s; if(*s == '"') s++; quoteend = s; break;
        case '(': case '[':
            quotestart = s;
            start = s+1;
            for(int braktype = *s++, brak = 1;;)
            {
                s += strcspn(s, "\"/;()[]\0");
                int c = *s++;
                switch(c)
                {
                    case '\0': s--; quoteend = end = s; return true;
                    case '"': s = parsestring(s); if(*s == '"') s++; break;
                    case '/': if(*s == '/') s += strcspn(s, "\n\0"); break;
                    case '(': case '[': if(c == braktype) brak++; break;
                    case ')': if(braktype == '(' && --brak <= 0) goto endblock; break;
                    case ']': if(braktype == '[' && --brak <= 0) goto endblock; break;
                }
            }
        endblock:
            end = s-1;
            quoteend = s;
            break;
        case '\0': case ')': case ']': return false;
        default: quotestart = start = s; s = parseword(s); quoteend = end = s; break;
    }
    skiplist(s);
    if(*s == ';') s++;
    return true;
}

void explodelist(const char *s, vector<char *> &elems, int limit)
{
    const char *start, *end;
    while((limit < 0 || elems.length() < limit) && parselist(s, start, end))
        elems.add(newstring(start, end-start));
}

char *indexlist(const char *s, int pos)
{
    loopi(pos) if(!parselist(s)) return newstring("");
    const char *start, *end;
    return parselist(s, start, end) ? newstring(start, end-start) : newstring("");
}

int listlen(const char *s)
{
    int n = 0;
    while(parselist(s)) n++;
    return n;
}
ICOMMAND(listlen, "s", (char *s), intret(listlen(s)));

void at(tagval *args, int numargs)
{
    if(!numargs) return;
    const char *start = args[0].getstr(), *end = start + strlen(start);
    for(int i = 1; i < numargs; i++)
    {
        const char *list = start;
        int pos = args[i].getint();
        for(; pos > 0; pos--) if(!parselist(list)) break;
        if(pos > 0 || !parselist(list, start, end)) start = end = "";
    }
    commandret->setstr(newstring(start, end-start));
}
COMMAND(at, "si1V");

void substr(char *s, int *start, int *count, int *numargs)
{
    int len = strlen(s), offset = clamp(*start, 0, len);
    commandret->setstr(newstring(&s[offset], *numargs >= 3 ? clamp(*count, 0, len - offset) : len - offset));
}
COMMAND(substr, "siiN");

void sublist(const char *s, int *skip, int *count, int *numargs)
{
    int offset = max(*skip, 0), len = *numargs >= 3 ? max(*count, 0) : -1;
    loopi(offset) if(!parselist(s)) break;
    if(len < 0) { if(offset > 0) skiplist(s); commandret->setstr(newstring(s)); return; }
    const char *list = s, *start, *end, *qstart, *qend = s;
    if(len > 0 && parselist(s, start, end, list, qend)) while(--len > 0 && parselist(s, start, end, qstart, qend));
    commandret->setstr(newstring(list, qend - list));
}
COMMAND(sublist, "siiN");

ICOMMAND(stripcolors, "s", (char *s),
{
    int len = strlen(s);
    char *d = newstring(len);
    filtertext(d, s, true, len);
    stringret(d);
});
ICOMMAND(isdir, "s", (char *dir), intret(isdir(dir)));

static inline void setiter(ident &id, char *val, identstack &stack)
{
    if(id.stack == &stack)
    {
        if(id.valtype == VAL_STR) delete[] id.val.s;
        else id.valtype = VAL_STR;
        cleancode(id);
        id.val.s = val;
    }
    else
    {
        tagval t;
        t.setstr(val);
        pusharg(id, t, stack);
        id.flags &= ~IDF_UNKNOWN;
    }
}

void listfind(ident *id, const char *list, const uint *body)
{
    if(id->type!=ID_ALIAS) { intret(-1); return; }
    identstack stack;
    int n = 0;
    for(const char *s = list, *start, *end; parselist(s, start, end); n++)
    {
        char *val = newstring(start, end-start);
        setiter(*id, val, stack);
        if(executebool(body)) { intret(n); goto found; }
    }
    intret(-1);
found:
    if(n) poparg(*id);
}
COMMAND(listfind, "rse");

void looplist(ident *id, const char *list, const uint *body)
{
    if(id->type!=ID_ALIAS) return;
    identstack stack;
    int n = 0;
    for(const char *s = list, *start, *end; parselist(s, start, end); n++)
    {
        char *val = newstring(start, end-start);
        setiter(*id, val, stack);
        execute(body);
    }
    if(n) poparg(*id);
}

COMMAND(looplist, "rse");

void looplistconc(ident *id, const char *list, const uint *body, bool space)
{
    if(id->type!=ID_ALIAS) return;
    identstack stack;
    vector<char> r;
    int n = 0;
    for(const char *s = list, *start, *end; parselist(s, start, end); n++)
    {
        char *val = newstring(start, end-start);
        setiter(*id, val, stack);

        if(n && space) r.add(' ');

        tagval v;
        executeret(body, v);
        const char *vstr = v.getstr();
        int len = strlen(vstr);
        r.put(vstr, len);
        freearg(v);
    }
    if(n) poparg(*id);
    r.add('\0');
    commandret->setstr(newstring(r.getbuf(), r.length()-1));
}
ICOMMAND(looplistconcat, "rse", (ident *id, char *list, uint *body), looplistconc(id, list, body, true));
ICOMMAND(looplistconcatword, "rse", (ident *id, char *list, uint *body), looplistconc(id, list, body, false));

void listfilter(ident *id, const char *list, const uint *body)
{
    if(id->type!=ID_ALIAS) return;
    identstack stack;
    vector<char> r;
    int n = 0;
    for(const char *s = list, *start, *end, *quotestart, *quoteend; parselist(s, start, end, quotestart, quoteend); n++)
    {
        char *val = newstring(start, end-start);
        setiter(*id, val, stack);

        if(executebool(body))
        {
            if(r.length()) r.add(' ');
            r.put(quotestart, quoteend-quotestart);
        }
    }
    if(n) poparg(*id);
    r.add('\0');
    commandret->setstr(newstring(r.getbuf(), r.length()-1));
}
COMMAND(listfilter, "rse");

void prettylist(const char *s, const char *conj)
{
    vector<char> p;
    const char *start, *end;
    for(int len = listlen(s), n = 0; parselist(s, start, end); n++)
    {
        p.put(start, end - start);
        if(n+1 < len)
        {
            if(len > 2 || !conj[0]) p.add(',');
            if(n+2 == len && conj[0])
            {
                p.add(' ');
                p.put(conj, strlen(conj));
            }
            p.add(' ');
        }
    }
    p.add('\0');
    result(p.getbuf());
}
COMMAND(prettylist, "ss");

int listincludes(const char *list, const char *needle, int needlelen)
{
    int offset = 0;
    for(const char *s = list, *start, *end; parselist(s, start, end);)
    {
        int len = end - start;
        if(needlelen == len && !strncmp(needle, start, len)) return offset;
        offset++;
    }
    return -1;
}
ICOMMAND(indexof, "ss", (char *list, char *elem), intret(listincludes(list, elem, strlen(elem))));

char *listdel(const char *s, const char *del)
{
    vector<char> p;
    for(const char *start, *end, *qstart, *qend; parselist(s, start, end, qstart, qend);)
    {
        if(listincludes(del, start, end-start) < 0)
        {
            if(!p.empty()) p.add(' ');
            p.put(qstart, qend-qstart);
        }
    }
    p.add('\0');
    return newstring(p.getbuf(), p.length()-1);
}
ICOMMAND(listdel, "ss", (char *list, char *del), commandret->setstr(listdel(list, del)));

void listsplice(const char *s, const char *vals, int *skip, int *count, int *numargs)
{
    int offset = max(*skip, 0), len = *numargs >= 4 ? max(*count, 0) : -1;
    const char *list = s, *start, *end, *qstart, *qend = s;
    loopi(offset) if(!parselist(s, start, end, qstart, qend)) break;
    vector<char> p;
    if(qend > list) p.put(list, qend-list);
    if(*vals)
    {
        if(!p.empty()) p.add(' ');
        p.put(vals, strlen(vals));
    }
    while(len-- > 0) if(!parselist(s)) break;
    skiplist(s);
    switch(*s)
    {
        case '\0': case ')': case ']': break;
        default:
            if(!p.empty()) p.add(' ');
            p.put(s, strlen(s));
            break;
    }
    p.add('\0');
    commandret->setstr(newstring(p.getbuf(), p.length()-1));
}
COMMAND(listsplice, "ssiiN");

ICOMMAND(loopfiles, "rsse", (ident *id, char *dir, char *ext, uint *body),
{
    if(id->type!=ID_ALIAS) return;
    identstack stack;
    vector<char *> files;
    listfiles(dir, ext[0] ? ext : NULL, files);
    files.sort();
    files.uniquedeletearrays();
    loopv(files)
    {
        setiter(*id, files[i], stack);
        execute(body);
    }
    if(files.length()) poparg(*id);
});

//assume directories don't have extensions - AKA a hack
ICOMMAND(loopdir, "rse", (ident *id, char *dir, uint *body),
{
    if(id->type != ID_ALIAS) return;
    identstack stack;
    vector<char *> files;
    listfiles(dir, NULL, files);
    loopvrev(files)
    {
        bool redundant = false;
        if(strstr(files[i], ".")) redundant = true;
        if(!redundant) loopj(i) if(!strcmp(files[i], files[j]))
        {
            redundant = true; break;
        }
        if(redundant) { delete[] files.removeunordered(i); continue; }
    }

    loopv(files)
    {
        if(i)
        {
            if(id->valtype == VAL_STR) delete[] id->val.s;
            else id->valtype = VAL_STR;
            id->val.s = files[i];
        }
        else
        {
            tagval t;
            t.setstr(files[i]);
            pusharg(*id, t, stack);
            id->flags &= ~IDF_UNKNOWN;
        }
        execute(body);
    }
    if(files.length()) poparg(*id);
});

ICOMMAND(findfile, "s", (char *name), intret(findfile(name, "e") ? 1 : 0));

struct sortitem
{
    const char *str, *quotestart, *quoteend;

    int quotelength() const { return int(quoteend-quotestart); }
};

struct sortfun
{
    ident *x, *y;
    uint *body;

    bool operator()(const sortitem &xval, const sortitem &yval)
    {
        if(x->valtype != VAL_CSTR) x->valtype = VAL_CSTR;
        cleancode(*x);
        x->val.code = (const uint *)xval.str;
        if(y->valtype != VAL_CSTR) y->valtype = VAL_CSTR;
        cleancode(*y);
        y->val.code = (const uint *)yval.str;
        return executebool(body);
    }
};

void sortlist(char *list, ident *x, ident *y, uint *body, uint *unique)
{
    if(x == y || x->type != ID_ALIAS || y->type != ID_ALIAS) return;

    vector<sortitem> items;
    int clen = strlen(list), total = 0;
    char *cstr = newstring(clen);
    const char *curlist = list, *start, *end, *quotestart, *quoteend;
    while(parselist(curlist, start, end, quotestart, quoteend))
    {
        cstr[end - list] = '\0';
        sortitem item = { &cstr[start - list], quotestart, quoteend };
        items.add(item);
        total += item.quotelength();
    }

    identstack xstack, ystack;
    pusharg(*x, nullval, xstack); x->flags &= ~IDF_UNKNOWN;
    pusharg(*y, nullval, ystack); y->flags &= ~IDF_UNKNOWN;

    sortfun f = { x, y, body };
    items.sort(f);

    int totalunique = total, numunique = items.length();
    if(items.length() && (*unique&CODE_OP_MASK) != CODE_EXIT)
    {
        totalunique = items[0].quotelength();
        numunique = 1;
        for(int i = 1; i < items.length(); i++)
        {
            sortitem &item = items[i];
            if(f(items[i-1], item)) item.quotestart = NULL;
            else { totalunique += item.quotelength(); numunique++; }
        }
    }
    poparg(*x);
    poparg(*y);

    char *sorted = cstr;
    int sortedlen = totalunique + max(numunique - 1, 0);
    if(clen < sortedlen)
    {
        delete[] cstr;
        sorted = newstring(sortedlen);
    }

    int offset = 0;
    loopv(items)
    {
        sortitem &item = items[i];
        if(!item.quotestart) continue;
        int len = item.quotelength();
        if(i) sorted[offset++] = ' ';
        memcpy(&sorted[offset], item.quotestart, len);
        offset += len;
    }
    sorted[offset] = '\0';

    commandret->setstr(sorted);
}

COMMAND(sortlist, "srree");

#define MATHCMD(name, fmt, type, op, initval, unaryop) \
    ICOMMANDS(name, #fmt "1V", (tagval *args, int numargs), \
    { \
        type val; \
        if(numargs >= 2) \
        { \
            val = args[0].fmt; \
            type val2 = args[1].fmt; \
            op; \
            for(int i = 2; i < numargs; i++) { val2 = args[i].fmt; op; } \
        } \
        else { val = numargs > 0 ? args[0].fmt : initval; unaryop; } \
        type##ret(val); \
    })
#define MATHICMDN(name, op, initval, unaryop) MATHCMD(#name, i, int, val = val op val2, initval, unaryop)
#define MATHICMD(name, initval, unaryop) MATHICMDN(name, name, initval, unaryop)
#define MATHFCMDN(name, op, initval, unaryop) MATHCMD(#name "f", f, float, val = val op val2, initval, unaryop)
#define MATHFCMD(name, initval, unaryop) MATHFCMDN(name, name, initval, unaryop)

#define CMPCMD(name, fmt, type, op) \
    ICOMMANDS(name, #fmt "1V", (tagval *args, int numargs), \
    { \
        bool val; \
        if(numargs >= 2) \
        { \
            val = args[0].fmt op args[1].fmt; \
            for(int i = 2; i < numargs && val; i++) val = args[i-1].fmt op args[i].fmt; \
        } \
        else val = (numargs > 0 ? args[0].fmt : 0) op 0; \
        intret(int(val)); \
    })
#define CMPICMDN(name, op) CMPCMD(#name, i, int, op)
#define CMPICMD(name) CMPICMDN(name, name)
#define CMPFCMDN(name, op) CMPCMD(#name "f", f, float, op)
#define CMPFCMD(name) CMPFCMDN(name, name)

MATHICMD(+, 0, );
MATHICMD(*, 1, );
MATHICMD(-, 0, val = -val);
CMPICMDN(=, ==);
CMPICMD(!=);
CMPICMD(<);
CMPICMD(>);
CMPICMD(<=);
CMPICMD(>=);
MATHICMD(^, 0, val = ~val);
MATHICMDN(~, ^, 0, val = ~val);
MATHICMD(&, 0, );
MATHICMD(|, 0, );
MATHICMD(^~, 0, );
MATHICMD(&~, 0, );
MATHICMD(|~, 0, );
MATHICMD(<<, 0, );
MATHICMD(>>, 0, );

MATHFCMD(+, 0, );
MATHFCMD(*, 1, );
MATHFCMD(-, 0, val = -val);
CMPFCMDN(=, ==);
CMPFCMD(!=);
CMPFCMD(<);
CMPFCMD(>);
CMPFCMD(<=);
CMPFCMD(>=);

ICOMMANDK(!, ID_NOT, "t", (tagval *a), intret(getbool(*a) ? 0 : 1));
ICOMMANDK(&&, ID_AND, "E1V", (tagval *args, int numargs),
{
    if(!numargs) intret(1);
    else loopi(numargs)
    {
        if(i) freearg(*commandret);
        if(args[i].type == VAL_CODE) executeret(args[i].code, *commandret);
        else *commandret = args[i];
        if(!getbool(*commandret)) break;
    }
});
ICOMMANDK(||, ID_OR, "E1V", (tagval *args, int numargs),
{
    if(!numargs) intret(0);
    else loopi(numargs)
    {
        if(i) freearg(*commandret);
        if(args[i].type == VAL_CODE) executeret(args[i].code, *commandret);
        else *commandret = args[i];
        if(getbool(*commandret)) break;
    }
});

#define DIVCMD(name, fmt, type, op) MATHCMD(#name, fmt, type, { if(val2) op; else val = 0; }, 0, )

DIVCMD(div, i, int, val /= val2);
DIVCMD(mod, i, int, val %= val2);
DIVCMD(divf, f, float, val /= val2);
DIVCMD(modf, f, float, val = fmod(val, val2));
MATHCMD("pow", f, float, val = pow(val, val2), 0, );

ICOMMAND(sin, "f", (float *a), floatret(sin(*a*RAD)));
ICOMMAND(cos, "f", (float *a), floatret(cos(*a*RAD)));
ICOMMAND(tan, "f", (float *a), floatret(tan(*a*RAD)));
ICOMMAND(asin, "f", (float *a), floatret(asin(*a)/RAD));
ICOMMAND(acos, "f", (float *a), floatret(acos(*a)/RAD));
ICOMMAND(atan, "f", (float *a), floatret(atan(*a)/RAD));
ICOMMAND(sqrt, "f", (float *a), floatret(sqrt(*a)));
ICOMMAND(loge, "f", (float *a), floatret(log(*a)));
ICOMMAND(log2, "f", (float *a), floatret(log(*a)/M_LN2));
ICOMMAND(log10, "f", (float *a), floatret(log10(*a)));
ICOMMAND(exp, "f", (float *a), floatret(exp(*a)));
#define MINMAXCMD(name, fmt, type, op) \
    ICOMMAND(name, #fmt "1V", (tagval *args, int numargs), \
    { \
        type val = numargs > 0 ? args[0].fmt : 0; \
        for(int i = 1; i < numargs; i++) val = op(val, args[i].fmt); \
        type##ret(val); \
    })

MINMAXCMD(min, i, int, min);
MINMAXCMD(max, i, int, max);
MINMAXCMD(minf, f, float, min);
MINMAXCMD(maxf, f, float, max);
ICOMMAND(abs, "i", (int *n), intret(abs(*n)));
ICOMMAND(absf, "f", (float *n), floatret(fabs(*n)));

ICOMMAND(cond, "ee2V", (tagval *args, int numargs),
{
    for(int i = 0; i < numargs; i += 2)
    {
        if(i+1 < numargs)
        {
            if(executebool(args[i].code))
            {
                executeret(args[i+1].code, *commandret);
                break;
            }
        }
        else
        {
            executeret(args[i].code, *commandret);
            break;
        }
    }
});

#define CASECOMMAND(name, fmt, type, acc, compare) \
    ICOMMAND(name, fmt "te2V", (tagval *args, int numargs), \
    { \
        type val = acc; \
        int i; \
        for(i = 1; i+1 < numargs; i += 2) \
        { \
            if(compare) \
            { \
                executeret(args[i+1].code, *commandret); \
                return; \
            } \
        } \
    })

CASECOMMAND(case, "i", int, args[0].getint(), args[i].type == VAL_NULL || args[i].getint() == val);
CASECOMMAND(casef, "f", float, args[0].getfloat(), args[i].type == VAL_NULL || args[i].getfloat() == val);
CASECOMMAND(cases, "s", const char *, args[0].getstr(), args[i].type == VAL_NULL || !strcmp(args[i].getstr(), val));

ICOMMAND(rnd, "ii", (int *a, int *b), intret(*a - *b > 0 ? rnd(*a - *b) + *b : *b));

#define CMPSCMD(name, op) \
    ICOMMAND(name, "s1V", (tagval *args, int numargs), \
    { \
        bool val; \
        if(numargs >= 2) \
        { \
            val = strcmp(args[0].s, args[1].s) op 0; \
            for(int i = 2; i < numargs && val; i++) val = strcmp(args[i-1].s, args[i].s) op 0; \
        } \
        else val = (numargs > 0 ? args[0].s[0] : 0) op 0; \
        intret(int(val)); \
    })

CMPSCMD(strcmp, ==);
CMPSCMD(=s, ==);
CMPSCMD(!=s, !=);
CMPSCMD(<s, <);
CMPSCMD(>s, >);
CMPSCMD(<=s, <=);
CMPSCMD(>=s, >=);

ICOMMAND(echo, "C", (char *s), conoutf("\f1%s", s));
ICOMMAND(error, "C", (char *s), conoutf(CON_ERROR, "%s", s));
ICOMMAND(strstr, "ss", (char *a, char *b), { char *s = strstr(a, b); intret(s ? s-a : -1); });
ICOMMAND(strlen, "s", (char *s), intret(strlen(s)));

char *strreplace(const char *s, const char *oldval, const char *newval, const char *newval2)
{
    vector<char> buf;

    int oldlen = strlen(oldval);
    if(!oldlen) return newstring(s);
    for(int i = 0;; i++)
    {
        const char *found = strstr(s, oldval);
        if(found)
        {
            while(s < found) buf.add(*s++);
            for(const char *n = i&1 ? newval2 : newval; *n; n++) buf.add(*n);
            s = found + oldlen;
        }
        else
        {
            while(*s) buf.add(*s++);
            buf.add('\0');
            return newstring(buf.getbuf(), buf.length());
        }
    }
}

ICOMMAND(strreplace, "ssss", (char *s, char *o, char *n, char *n2), commandret->setstr(strreplace(s, o, n, n2[0] ? n2 : n)));

#ifndef STANDALONE
struct sleepcmd
{
    int delay, millis, flags;
    uint *command;
};
vector<sleepcmd> sleepcmds;

void addsleep(int *msec, uint *cmd)
{
    sleepcmd &s = sleepcmds.add();
    s.delay = max(*msec, 1);
    s.millis = lastmillis;
    s.command = cmd;
    keepcode(cmd);
    s.flags = identflags;
}

COMMANDN(sleep, addsleep, "ie");

void checksleep(int millis)
{
    loopv(sleepcmds)
    {
        sleepcmd &s = sleepcmds[i];
        if(millis - s.millis >= s.delay)
        {
            uint *cmd = s.command; // execute might create more sleep commands
            s.command = NULL;
            int oldflags = identflags;
            identflags = s.flags;
            execute(cmd);
            identflags = oldflags;
            freecode(cmd);
            if(sleepcmds.inrange(i) && !sleepcmds[i].command) sleepcmds.remove(i--);
        }
    }
}

void clearsleep(bool clearoverrides)
{
    int len = 0;
    loopv(sleepcmds) if(sleepcmds[i].command)
    {
        if(clearoverrides && !(sleepcmds[i].flags&IDF_OVERRIDDEN)) sleepcmds[len++] = sleepcmds[i];
        else freecode(sleepcmds[i].command);
    }
    sleepcmds.shrink(len);
}

void clearsleep_(int *clearoverrides)
{
    clearsleep(*clearoverrides!=0 || identflags&IDF_OVERRIDDEN);
}

COMMANDN(clearsleep, clearsleep_, "i");
#endif

