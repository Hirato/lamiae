#include "engine.h"

static hashnameset<font> fonts;
static font *fontdef = NULL;
static int fontdeftex = 0;

font *curfont = NULL;
int curfonttex = 0;

void newfont(char *name, char *tex, int *defaultw, int *defaulth, int *scale)
{
    font *f = &fonts[name];
    if(!f->name) f->name = newstring(name);
    f->texs.shrink(0);
    f->texs.add(textureload(tex));
    f->chars.shrink(0);
    f->charoffset = '!';
    f->defaultw = *defaultw;
    f->defaulth = *defaulth;
    f->scale = *scale > 0 ? *scale : f->defaulth;
    f->bordermin = 0.49f;
    f->bordermax = 0.5f;
    f->outlinemin = -1;
    f->outlinemax = 0;

    fontdef = f;
    fontdeftex = 0;
}

void fontborder(float *bordermin, float *bordermax)
{
    if(!fontdef) return;

    fontdef->bordermin = *bordermin;
    fontdef->bordermax = max(*bordermax, *bordermin+0.01f);
}

void fontoutline(float *outlinemin, float *outlinemax)
{
    if(!fontdef) return;

    fontdef->outlinemin = min(*outlinemin, *outlinemax-0.01f);
    fontdef->outlinemax = *outlinemax;
}

void fontoffset(char *c)
{
    if(!fontdef) return;

    fontdef->charoffset = c[0];
}

void fontscale(int *scale)
{
    if(!fontdef) return;

    fontdef->scale = *scale > 0 ? *scale : fontdef->defaulth;
}

void fonttex(char *s)
{
    if(!fontdef) return;

    Texture *t = textureload(s);
    loopv(fontdef->texs) if(fontdef->texs[i] == t) { fontdeftex = i; return; }
    fontdeftex = fontdef->texs.length();
    fontdef->texs.add(t);
}

void fontchar(float *x, float *y, float *w, float *h, float *offsetx, float *offsety, float *advance)
{
    if(!fontdef) return;

    font::charinfo &c = fontdef->chars.add();
    c.x = *x;
    c.y = *y;
    c.w = *w ? *w : fontdef->defaultw;
    c.h = *h ? *h : fontdef->defaulth;
    c.offsetx = *offsetx;
    c.offsety = *offsety;
    c.advance = *advance ? *advance : c.offsetx + c.w;
    c.tex = fontdeftex;
}

void fontskip(int *n)
{
    if(!fontdef) return;
    loopi(max(*n, 1))
    {
        font::charinfo &c = fontdef->chars.add();
        c.x = c.y = c.w = c.h = c.offsetx = c.offsety = c.advance = 0;
        c.tex = 0;
    }
}

COMMANDN(font, newfont, "ssiii");
COMMAND(fontborder, "ff");
COMMAND(fontoutline, "ff");
COMMAND(fontoffset, "s");
COMMAND(fontscale, "i");
COMMAND(fonttex, "s");
COMMAND(fontchar, "fffffff");
COMMAND(fontskip, "i");

void fontalias(const char *dst, const char *src)
{
    font *s = fonts.access(src);
    if(!s) return;
    font *d = &fonts[dst];
    if(!d->name) d->name = newstring(dst);
    d->texs = s->texs;
    d->chars = s->chars;
    d->charoffset = s->charoffset;
    d->defaultw = s->defaultw;
    d->defaulth = s->defaulth;
    d->scale = s->scale;
    d->bordermin = s->bordermin;
    d->bordermax = s->bordermax;
    d->outlinemin = s->outlinemin;
    d->outlinemax = s->outlinemax;

    fontdef = d;
    fontdeftex = d->texs.length()-1;
}

COMMAND(fontalias, "ss");

font *findfont(const char *name)
{
    return fonts.access(name);
}

bool setfont(const char *name)
{
    font *f = fonts.access(name);
    if(!f) return false;
    curfont = f;
    return true;
}
ICOMMAND(setfont, "s", (const char *name),
    if(!setfont(name))
        conoutf("font %s does not exist", name);
)

static vector<font *> fontstack;

void pushfont()
{
    fontstack.add(curfont);
}

bool popfont()
{
    if(fontstack.empty()) return false;
    curfont = fontstack.pop();
    return true;
}

void gettextres(int &w, int &h)
{
    if(w < MINRESW || h < MINRESH)
    {
        if(MINRESW > w*MINRESH/h)
        {
            h = h*MINRESW/w;
            w = MINRESW;
        }
        else
        {
            w = w*MINRESH/h;
            h = MINRESH;
        }
    }
}

float text_widthf(const char *str)
{
    float width, height;
    text_boundsf(str, width, height);
    return width;
}
ICOMMAND(text_width, "sfN", (char *s, float *scale, int *numargs), intret(ceil(text_widthf(s) * (*numargs >= 2 ? *scale : 1))))

#define FONTTAB (4*FONTW)
#define TEXTTAB(x) ((int((x)/FONTTAB)+1.0f)*FONTTAB)

void tabify(const char *str, int *numtabs)
{
    int tw = max(*numtabs, 0)*FONTTAB-1, tabs = 0;
    for(float w = text_widthf(str); w <= tw; w = TEXTTAB(w)) ++tabs;
    int len = strlen(str);
    char *tstr = newstring(len + tabs);
    memcpy(tstr, str, len);
    memset(&tstr[len], '\t', tabs);
    tstr[len+tabs] = '\0';
    stringret(tstr);
}

COMMAND(tabify, "si");

void draw_textf(const char *fstr, float left, float top, ...)
{
    defvformatstring(str, top, fstr);
    draw_text(str, left, top);
}

const matrix4x3 *textmatrix = NULL;
float textscale = 1;

static float draw_char(Texture *&tex, int c, float x, float y, float scale)
{
    font::charinfo &info = curfont->chars[c-curfont->charoffset];
    if(tex != curfont->texs[info.tex])
    {
        xtraverts += gle::end();
        tex = curfont->texs[info.tex];
        glBindTexture(GL_TEXTURE_2D, tex->id);
    }

    x *= textscale;
    y *= textscale;
    scale *= textscale;

    float x1 = x + scale*info.offsetx,
          y1 = y + scale*info.offsety,
          x2 = x + scale*(info.offsetx + info.w),
          y2 = y + scale*(info.offsety + info.h),
          tx1 = info.x / tex->xs,
          ty1 = info.y / tex->ys,
          tx2 = (info.x + info.w) / tex->xs,
          ty2 = (info.y + info.h) / tex->ys;

    if(textmatrix)
    {
        gle::attrib(textmatrix->transform(vec2(x1, y1))); gle::attribf(tx1, ty1);
        gle::attrib(textmatrix->transform(vec2(x2, y1))); gle::attribf(tx2, ty1);
        gle::attrib(textmatrix->transform(vec2(x2, y2))); gle::attribf(tx2, ty2);
        gle::attrib(textmatrix->transform(vec2(x1, y2))); gle::attribf(tx1, ty2);
    }
    else
    {
        gle::attribf(x1, y1); gle::attribf(tx1, ty1);
        gle::attribf(x2, y1); gle::attribf(tx2, ty1);
        gle::attribf(x2, y2); gle::attribf(tx2, ty2);
        gle::attribf(x1, y2); gle::attribf(tx1, ty2);
    }

    return scale*info.advance;
}

VARP(textbright, 0, 85, 100);

//stack[sp] is current color index
static void text_color(char c, char *stack, int size, int &sp, bvec colour, int a)
{
    if(c=='s') // save color
    {
        c = stack[sp];
        if(sp<size-1) stack[++sp] = c;
    }
    else
    {
        xtraverts += gle::end();
        if(c=='r') { if(sp > 0) --sp; c = stack[sp]; } // restore color
        else stack[sp] = c;
        switch(c)
        {
            case '0': colour = bvec( 64, 255, 128); break; // green: player talk
            case '1': colour = bvec( 96, 160, 255); break; // blue: "echo" command
            case '2': colour = bvec(255, 192,  64); break; // yellow: gameplay messages
            case '3': colour = bvec(255,  64,  64); break; // red: important errors
            case '4': colour = bvec(128, 128, 128); break; // gray
            case '5': colour = bvec(192,  64, 192); break; // magenta
            case '6': colour = bvec(255, 128,   0); break; // orange
            case '7': colour = bvec(255, 255, 255); break; // white
            case '8': colour = bvec( 80, 207, 229); break; // "Tesseract Blue"
            case '9': colour = bvec(160, 240, 120); break;

            case 'A': colour = bvec(255, 224, 192); break; //apricot
            case 'B': colour = bvec(192,  96,   0); break; //brown
            case 'C': colour = bvec(255, 224,  96); break; //corn
            case 'D': colour = bvec(64,  160, 192); break; //dodger blue
            case 'E': colour = bvec(96 , 224, 128); break; //emerald
            case 'F': colour = bvec(255,  64, 255); break; //fuchsia
            case 'G': colour = bvec(255, 224,  64); break; //gold
            case 'H': colour = bvec(224, 160, 255); break; //heliotrope
            case 'I': colour = bvec(128,  64, 192); break; //indigo
            case 'J': colour = bvec(  0, 192, 128); break; //jade
            case 'K': colour = bvec(192, 176, 144); break; //khaki
            case 'L': colour = bvec(255, 224,  32); break; //lemon
            case 'M': colour = bvec(160, 255, 160); break; //mint
            case 'N': colour = bvec(255, 224, 160); break; //navajo white
            case 'O': colour = bvec(160, 160,  64); break; //olive
            case 'P': colour = bvec(255, 192, 208); break; //pink
            //case 'Q': nothing
            case 'R': colour = bvec(255,  64, 128); break; //rose
            case 'S': colour = bvec(224, 224, 224); break; //silver
            case 'T': colour = bvec( 64, 224, 224); break; //turquoise
            case 'U': colour = bvec( 32,  32, 192); break; //ultramarine
            case 'V': colour = bvec(192,  96, 224); break; //violet
            case 'W': colour = bvec(255, 224, 176); break; //wheat
            //case 'X': nothing
            case 'Y': colour = bvec(255, 255,  96); break; //yellow
            case 'Z': colour = bvec(224, 192, 160); break; //zinnwaldite
            default: gle::color(colour, a); return;
        }
        if(textbright != 100) colour.scale(textbright, 100);
        gle::color(colour, a);
    }
}

#define TEXTSKELETON \
    float y = 0, x = 0, scale = curfont->scale/float(curfont->defaulth);\
    int i;\
    for(i = 0; str[i]; i++)\
    {\
        TEXTINDEX(i)\
        int c = uchar(str[i]);\
        if(c=='\t')      { x = TEXTTAB(x); TEXTWHITE(i) }\
        else if(c==' ')  { x += scale*curfont->defaultw; TEXTWHITE(i) }\
        else if(c=='\n') { TEXTLINE(i) x = 0; y += FONTH; }\
        else if(c=='\f') { if(str[i+1]) { i++; TEXTCOLOR(i) }}\
        else if(curfont->chars.inrange(c-curfont->charoffset))\
        {\
            float cw = scale*curfont->chars[c-curfont->charoffset].advance;\
            if(cw <= 0) continue;\
            if(maxwidth != -1)\
            {\
                int j = i;\
                float w = cw;\
                for(; str[i+1]; i++)\
                {\
                    int c = uchar(str[i+1]);\
                    if(c=='\f') { if(str[i+2]) i++; continue; }\
                    if(!curfont->chars.inrange(c-curfont->charoffset)) break;\
                    float cw = scale*curfont->chars[c-curfont->charoffset].advance;\
                    if(cw <= 0 || w + cw > maxwidth) break;\
                    w += cw;\
                }\
                if(x + w > maxwidth && x > 0) { (void)j; TEXTLINE(j-1) x = 0; y += FONTH; }\
                TEXTWORD\
            }\
            else\
            { TEXTCHAR(i) }\
        }\
    }

//all the chars are guaranteed to be either drawable or color commands
#define TEXTWORDSKELETON \
                for(; j <= i; j++)\
                {\
                    TEXTINDEX(j)\
                    int c = uchar(str[j]);\
                    if(c=='\f') { if(str[j+1]) { j++; TEXTCOLOR(j) }}\
                    else { float cw = scale*curfont->chars[c-curfont->charoffset].advance; TEXTCHAR(j) }\
                }

#define TEXTEND(cursor) if(cursor >= i) { do { TEXTINDEX(cursor); } while(0); }

int text_visible(const char *str, float hitx, float hity, int maxwidth)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx) if(y+FONTH > hity && x >= hitx) return idx;
    #define TEXTLINE(idx) if(y+FONTH > hity) return idx;
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += cw; TEXTWHITE(idx)
    #define TEXTWORD TEXTWORDSKELETON
    TEXTSKELETON
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
    return i;
}

//inverse of text_visible
void text_posf(const char *str, int cursor, float &cx, float &cy, int maxwidth)
{
    #define TEXTINDEX(idx) if(idx == cursor) { cx = x; cy = y; break; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx)
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += cw;
    #define TEXTWORD TEXTWORDSKELETON if(i >= cursor) break;
    cx = cy = 0;
    TEXTSKELETON
    TEXTEND(cursor)
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

void text_boundsf(const char *str, float &width, float &height, int maxwidth)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx) if(x > width) width = x;
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += cw;
    #define TEXTWORD x += w;
    width = 0;
    TEXTSKELETON
    height = y + FONTH;
    TEXTLINE(_)
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

Shader *textshader = NULL;

VARP(verttextcursor, 0, 0, 1);
void draw_text(const char *str, float left, float top, int r, int g, int b, int a, int cursor, int maxwidth)
{
    #define TEXTINDEX(idx) if(idx == cursor) { cx = x; cy = y; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx)
    #define TEXTCOLOR(idx) if(usecolor) text_color(str[idx], colorstack, sizeof(colorstack), colorpos, color, a);
    #define TEXTCHAR(idx) draw_char(tex, c, left+x, top+y, scale); x += cw;
    #define TEXTWORD TEXTWORDSKELETON
    char colorstack[10];
    colorstack[0] = '\0'; //indicate user color
    bvec color(r, g, b);
    if(textbright != 100) color.scale(textbright, 100);
    int colorpos = 0;
    float cx = -FONTW, cy = 0;
    bool usecolor = true;
    if(a < 0) { usecolor = false; a = -a; }
    Texture *tex = curfont->texs[0];
    (textshader ? textshader : hudtextshader)->set();
    LOCALPARAMF(textparams, curfont->bordermin, curfont->bordermax, curfont->outlinemin, curfont->outlinemax);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    gle::color(color, a);
    gle::defvertex(textmatrix ? 3 : 2);
    gle::deftexcoord0();
    gle::begin(GL_QUADS);
    TEXTSKELETON
    TEXTEND(cursor)
    xtraverts += gle::end();
    if(cursor >= 0 && (totalmillis/250)&1)
    {
        gle::color(color, a);
        gle::begin(GL_QUADS);
        if(maxwidth >= 0 && cx >= maxwidth && cx > 0) { cx = 0; cy += FONTH; }
        if(verttextcursor)
        {
            if(curfont->chars.inrange('|' - curfont->charoffset))
            {
                cx -= curfont->chars['|' - curfont->charoffset].w * curfont->scale / float(curfont->defaulth);
            }
            draw_char(tex, '|', left+cx, top+cy, scale);
        }
        else draw_char(tex, '_', left+cx, top+cy, scale);
        xtraverts += gle::end();
    }

    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

void reloadfonts()
{
    enumerate(fonts, font, f,
        loopv(f.texs) if(!reloadtexture(*f.texs[i])) fatal("failed to reload font texture");
    );
}

