// main.cpp: initialisation & main loop
#include "engine.h"

SVARO(version, "0.1.0");
string imagelogo = "data/lamiae";

extern void cleargamma();

void cleanup()
{
    recorder::stop();
    cleanupserver();
    SDL_ShowCursor(1);
    SDL_WM_GrabInput(SDL_GRAB_OFF);
    cleargamma();
    freeocta(worldroot);
    extern void clear_command(); clear_command();
    extern void clear_console(); clear_console();
    extern void clear_mdls();    clear_mdls();
    extern void clear_sound();   clear_sound();
    closelogfile();
    SDL_Quit();
}

void quit()                     // normal exit
{
    extern void writeinitcfg();
    writeinitcfg();
    writeservercfg();
    abortconnect();
    disconnect();
    localdisconnect();
    writecfg();
    cleanup();
    exit(EXIT_SUCCESS);
}

void fatal(const char *s, ...)    // failure exit
{
    static int errors = 0;
    errors++;

    if(errors <= 2) // print up to one extra recursive error
    {
        defvformatstring(msg,s,s);
        logoutf("%s", msg);

        if(errors <= 1) // avoid recursion
        {
            if(SDL_WasInit(SDL_INIT_VIDEO))
            {
                SDL_ShowCursor(1);
                SDL_WM_GrabInput(SDL_GRAB_OFF);
                cleargamma();
            }
            #ifdef WIN32
                MessageBox(NULL, msg, "Lamiae fatal error", MB_OK|MB_SYSTEMMODAL);
            #endif
            SDL_Quit();
        }
    }
    exit(EXIT_FAILURE);
}

SDL_Surface *screen = NULL;

VAR(curtime, 1, 0, 0);
VAR(totalmillis, 1, 1, 0);
VAR(lastmillis, 1, 1, 0);

dynent *player = NULL;

int initing = NOT_INITING;

bool initwarning(const char *desc, int level, int type)
{
    if(initing < level)
    {
        addchange(desc, type);
        return true;
    }
    return false;
}

#define SCR_MINW 320
#define SCR_MINH 200
#define SCR_MAXW 10000
#define SCR_MAXH 10000
#define SCR_DEFAULTW 1024
#define SCR_DEFAULTH 768

VARF(scr_w, SCR_MINW, -1, SCR_MAXW, initwarning("screen resolution"));
VARF(scr_h, SCR_MINH, -1, SCR_MAXH, initwarning("screen resolution"));
VARF(colorbits, 0, 0, 32, initwarning("color depth"));
VARF(depthbits, 0, 0, 32, initwarning("depth-buffer precision"));
VARF(stencilbits, 0, 0, 32, initwarning("stencil-buffer precision"));
VARF(fsaa, -1, -1, 16, initwarning("anti-aliasing"));
VARF(vsync, -1, -1, 1, initwarning("vertical sync"));

void writeinitcfg()
{
    stream *f = openutf8file("init.cfg", "w");
    if(!f) return;
    f->printf("// automatically written on exit, DO NOT MODIFY\n// modify settings in game\n");
    extern int fullscreen;
    f->printf("fullscreen %d\n", fullscreen);
    f->printf("scr_w %d\n", scr_w);
    f->printf("scr_h %d\n", scr_h);
    f->printf("colorbits %d\n", colorbits);
    f->printf("depthbits %d\n", depthbits);
    f->printf("stencilbits %d\n", stencilbits);
    f->printf("fsaa %d\n", fsaa);
    f->printf("vsync %d\n", vsync);
    extern int sound, soundchans, soundfreq, soundbufferlen;
    f->printf("sound %d\n", sound);
    f->printf("soundchans %d\n", soundchans);
    f->printf("soundfreq %d\n", soundfreq);
    f->printf("soundbufferlen %d\n", soundbufferlen);
    delete f;
}

COMMAND(quit, "");

static void getbackgroundres(int &w, int &h)
{
    float wk = 1, hk = 1;
    if(w < 1024) wk = 1024.0f/w;
    if(h < 768) hk = 768.0f/h;
    wk = hk = max(wk, hk);
    w = int(ceil(w*wk));
    h = int(ceil(h*hk));
}

extern void renderbackground(const char *caption, Texture *mapshot, const char *mapname, const char *mapinfo, bool restore, bool force);
extern void restorebackground();

float loadprogress = 0;

void renderprogress(float bar, const char *text, GLuint tex, bool background)   // also used during loading
{
    if(!inbetweenframes || envmapping) return;

    clientkeepalive();      // make sure our connection doesn't time out while loading maps etc.

    #ifdef __APPLE__
    interceptkey(SDLK_UNKNOWN); // keep the event queue awake to avoid 'beachball' cursor
    #endif

    extern int sdl_backingstore_bug;
    if(background || sdl_backingstore_bug > 0) restorebackground();

    int w = screen->w, h = screen->h;
    if(forceaspect) w = int(ceil(h*forceaspect));
    if(forceaspect) w = int(ceil(h*forceaspect));
    getbackgroundres(w, h);
    gettextres(w, h);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    notextureshader->set();

    ///TODO replace this with a texture of some sort
    //use rendererframe to check for backdrops
    float fh = 0.03 * min(w, h) + FONTH;
    float fw = 0.02 * min(w, h) + min(fh * 100, w * .75f);
    float fx = (w - fw) / 2;
    float fy = h - fh - FONTH + 0.01 * min(w, h);

    glColor3f(1, 1, 1);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(fx, fy);
    glVertex2f(fx + fw, fy);
    glVertex2f(fx, fy + fh);
    glVertex2f(fx + fw, fy + fh);
    glEnd();

    glColor3f(0, 0, 0);
    glBegin(GL_LINES);
    glVertex2f(fx, fy);
    glVertex2f(fx + fw, fy);

    glVertex2f(fx + fw, fy);
    glVertex2f(fx + fw, fy + fh);

    glVertex2f(fx + fw, fy + fh);
    glVertex2f(fx, fy + fh);

    glVertex2f(fx, fy + fh);
    glVertex2f(fx, fy);
    glEnd();


    float bh = 0.01 * min(w, h) + FONTH;
    float bw = min(fh * 100, w * .75f);
    float bx = (w - bw) / 2;
    float by = h - bh - FONTH;

    if(bar > 0)
    {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(bx, by);
        glVertex2f(bx + bw * bar, by);
        glVertex2f(bx, by + bh);
        glVertex2f(bx + bw * bar, by + bh);
        glEnd();
    }

    defaultshader->set();

    if(text)
    {
        glEnable(GL_BLEND);
        int tw, th;
        text_bounds(text, tw, th);
        float offset = 0.005 * min(w, h);
        float tsz = min(.5f, (bw - 2 * offset) / tw);
        float tx = bx + offset + (bw - tw * tsz) / 2;
        float ty = by + offset + (FONTH - FONTH * tsz) / 2;

        glPushMatrix();
        glTranslatef(tx, ty, 0);
        glScalef(tsz, tsz, 1);
        draw_text(text, 0, 0);
        glPopMatrix();
        glDisable(GL_BLEND);
    }

    if(tex)
    {
        float sz = 0.35 * min(w, h);
        float x = .5 * w - sz / 2;
        float y = .5 * h - sz / 2;

        glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_TRIANGLE_STRIP);

        glTexCoord2f(0, 0); glVertex2f(x, y);
        glTexCoord2f(1, 0); glVertex2f(x + sz, y);
        glTexCoord2f(0, 1); glVertex2f(x, y + sz);
        glTexCoord2f(1, 1); glVertex2f(x + sz, y + sz);

        glEnd();
        glEnable(GL_BLEND);

        settexture("data/mapshot_frame", 3);
        glBegin(GL_TRIANGLE_STRIP);

        glTexCoord2f(0, 0); glVertex2f(x, y);
        glTexCoord2f(1, 0); glVertex2f(x + sz, y);
        glTexCoord2f(0, 1); glVertex2f(x, y + sz);
        glTexCoord2f(1, 1); glVertex2f(x + sz, y + sz);

        glEnd();
        glDisable(GL_BLEND);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    swapbuffers();
}

void keyrepeat(bool on)
{
    SDL_EnableKeyRepeat(on ? SDL_DEFAULT_REPEAT_DELAY : 0,
                             SDL_DEFAULT_REPEAT_INTERVAL);
}

bool grabinput = false, minimized = false;

void inputgrab(bool on)
{
#ifndef WIN32
    if(!(screen->flags & SDL_FULLSCREEN)) SDL_WM_GrabInput(SDL_GRAB_OFF);
    else
#endif
    SDL_WM_GrabInput(on ? SDL_GRAB_ON : SDL_GRAB_OFF);
    SDL_ShowCursor(on ? SDL_DISABLE : SDL_ENABLE);
}

void setfullscreen(bool enable)
{
    if(!screen) return;
#if defined(WIN32) || defined(__APPLE__)
    initwarning(enable ? "fullscreen" : "windowed");
#else
    if(enable == !(screen->flags&SDL_FULLSCREEN))
    {
        SDL_WM_ToggleFullScreen(screen);
        inputgrab(grabinput);
    }
#endif
}

#ifdef _DEBUG
VARF(fullscreen, 0, 0, 1, setfullscreen(fullscreen!=0));
#else
VARF(fullscreen, 0, 1, 1, setfullscreen(fullscreen!=0));
#endif

void screenres(int *w, int *h)
{
#if !defined(WIN32) && !defined(__APPLE__)
    if(initing >= INIT_RESET)
    {
#endif
        scr_w = clamp(*w, SCR_MINW, SCR_MAXW);
        scr_h = clamp(*h, SCR_MINH, SCR_MAXH);
#if defined(WIN32) || defined(__APPLE__)
        initwarning("screen resolution");
#else
        return;
    }
    SDL_Surface *surf = SDL_SetVideoMode(clamp(*w, SCR_MINW, SCR_MAXW), clamp(*h, SCR_MINH, SCR_MAXH), 0, SDL_OPENGL|(screen->flags&SDL_FULLSCREEN ? SDL_FULLSCREEN : SDL_RESIZABLE));
    if(!surf) return;
    screen = surf;
    scr_w = screen->w;
    scr_h = screen->h;
    glViewport(0, 0, scr_w, scr_h);
#endif
}

COMMAND(screenres, "ii");

static int curgamma = 100;
VARFP(gamma, 30, 100, 300,
{
    if(gamma == curgamma) return;
    curgamma = gamma;
    float f = gamma/100.0f;
    if(SDL_SetGamma(f,f,f)==-1) conoutf(CON_ERROR, "Could not set gamma: %s", SDL_GetError());
});

void restoregamma()
{
    if(curgamma == 100) return;
    float f = curgamma/100.0f;
    SDL_SetGamma(1, 1, 1);
    SDL_SetGamma(f, f, f);
}

void cleargamma()
{
    if(curgamma != 100) SDL_SetGamma(1, 1, 1);
}

VAR(dbgmodes, 0, 0, 1);

int desktopw = 0, desktoph = 0;

void setupscreen(int &usedcolorbits, int &useddepthbits, int &usedfsaa)
{
    int flags = SDL_RESIZABLE;
    #if defined(WIN32) || defined(__APPLE__)
    flags = 0;
    #endif
    if(fullscreen) flags = SDL_FULLSCREEN;
    SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL|flags);
    if(modes && modes!=(SDL_Rect **)-1)
    {
        int widest = -1, best = -1;
        for(int i = 0; modes[i]; i++)
        {
            if(dbgmodes) conoutf(CON_DEBUG, "mode[%d]: %d x %d", i, modes[i]->w, modes[i]->h);
            if(widest < 0 || modes[i]->w > modes[widest]->w || (modes[i]->w == modes[widest]->w && modes[i]->h > modes[widest]->h))
                widest = i;
        }
        if(scr_w < 0 || scr_h < 0)
        {
            int w = scr_w, h = scr_h, ratiow = desktopw, ratioh = desktoph;
            if(w < 0 && h < 0) { w = SCR_DEFAULTW; h = SCR_DEFAULTH; }
            if(ratiow <= 0 || ratioh <= 0) { ratiow = modes[widest]->w; ratioh = modes[widest]->h; }
            for(int i = 0; modes[i]; i++) if(modes[i]->w*ratioh == modes[i]->h*ratiow)
            {
                if(w <= modes[i]->w && h <= modes[i]->h && (best < 0 || modes[i]->w < modes[best]->w))
                    best = i;
            }
        }
        if(best < 0)
        {
            int w = scr_w, h = scr_h;
            if(w < 0 && h < 0) { w = SCR_DEFAULTW; h = SCR_DEFAULTH; }
            else if(w < 0) w = (h*SCR_DEFAULTW)/SCR_DEFAULTH;
            else if(h < 0) h = (w*SCR_DEFAULTH)/SCR_DEFAULTW;
            for(int i = 0; modes[i]; i++)
            {
                if(w <= modes[i]->w && h <= modes[i]->h && (best < 0 || modes[i]->w < modes[best]->w || (modes[i]->w == modes[best]->w && modes[i]->h < modes[best]->h)))
                    best = i;
            }
        }
        if(flags&SDL_FULLSCREEN)
        {
            if(best >= 0) { scr_w = modes[best]->w; scr_h = modes[best]->h; }
            else if(desktopw > 0 && desktoph > 0) { scr_w = desktopw; scr_h = desktoph; }
            else if(widest >= 0) { scr_w = modes[widest]->w; scr_h = modes[widest]->h; }
        }
        else if(best < 0)
        {
            scr_w = min(scr_w >= 0 ? scr_w : (scr_h >= 0 ? (scr_h*SCR_DEFAULTW)/SCR_DEFAULTH : SCR_DEFAULTW), (int)modes[widest]->w);
            scr_h = min(scr_h >= 0 ? scr_h : (scr_w >= 0 ? (scr_w*SCR_DEFAULTH)/SCR_DEFAULTW : SCR_DEFAULTH), (int)modes[widest]->h);
        }
        if(dbgmodes) conoutf(CON_DEBUG, "selected %d x %d", scr_w, scr_h);
    }
    if(scr_w < 0 && scr_h < 0) { scr_w = SCR_DEFAULTW; scr_h = SCR_DEFAULTH; }
    else if(scr_w < 0) scr_w = (scr_h*SCR_DEFAULTW)/SCR_DEFAULTH;
    else if(scr_h < 0) scr_h = (scr_w*SCR_DEFAULTH)/SCR_DEFAULTW;

    bool hasbpp = true;
    if(colorbits)
        hasbpp = SDL_VideoModeOK(scr_w, scr_h, colorbits, SDL_OPENGL|flags)==colorbits;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#if SDL_VERSION_ATLEAST(1, 2, 11)
    if(vsync>=0) SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);
#endif
    static int configs[] =
    {
        0x7, /* try everything */
        0x6, 0x5, 0x3, /* try disabling one at a time */
        0x4, 0x2, 0x1, /* try disabling two at a time */
        0 /* try disabling everything */
    };
    int config = 0;
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    if(!fsaa)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    }
    loopi(sizeof(configs)/sizeof(configs[0]))
    {
        config = configs[i];
        if(!depthbits && config&1) continue;
        if(!stencilbits && config&2) continue;
        if(fsaa<=0 && config&4) continue;
        if(depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, config&1 ? depthbits : 0);
        if(stencilbits)
        {
            hasstencil = config&2 ? stencilbits : 0;
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, hasstencil);
        }
        else hasstencil = 0;
        if(fsaa>0)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, config&4 ? 1 : 0);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config&4 ? fsaa : 0);
        }
        screen = SDL_SetVideoMode(scr_w, scr_h, hasbpp ? colorbits : 0, SDL_OPENGL|flags);
        if(screen) break;
    }
    if(!screen) fatal("Unable to create OpenGL screen: %s", SDL_GetError());
    else
    {
        if(!hasbpp) conoutf(CON_WARN, "%d bit color buffer not supported - disabling", colorbits);
        if(depthbits && (config&1)==0) conoutf(CON_WARN, "%d bit z-buffer not supported - disabling", depthbits);
        if(stencilbits && (config&2)==0) conoutf(CON_WARN, "Stencil buffer not supported - disabling");
        if(fsaa>0 && (config&4)==0) conoutf(CON_WARN, "%dx anti-aliasing not supported - disabling", fsaa);
    }

    scr_w = screen->w;
    scr_h = screen->h;

    usedcolorbits = hasbpp ? colorbits : 0;
    useddepthbits = config&1 ? depthbits : 0;
    usedfsaa = config&4 ? fsaa : 0;
}

extern void resetgl();

COMMAND(resetgl, "");

vector<SDL_Event> events;

void pushevent(const SDL_Event &e)
{
    events.add(e);
}

static bool filterevent(const SDL_Event &event)
{
    switch(event.type)
    {
        case SDL_MOUSEMOTION:
            #ifndef WIN32
            if(grabinput && !(screen->flags&SDL_FULLSCREEN))
            {
                if(event.motion.x == screen->w / 2 && event.motion.y == screen->h / 2)
                    return false;  // ignore any motion events generated by SDL_WarpMouse
                #ifdef __APPLE__
                if(event.motion.y == 0)
                    return false;  // let mac users drag windows via the title bar
                #endif
            }
            #endif
            break;
    }
    return true;
}

static inline bool pollevent(SDL_Event &event)
{
    while(SDL_PollEvent(&event))
    {
        if(filterevent(event)) return true;
    }
    return false;
}

bool interceptkey(int sym)
{
    static int lastintercept = SDLK_UNKNOWN;
    int len = lastintercept == sym ? events.length() : 0;
    SDL_Event event;
    while(pollevent(event))
    {
        switch(event.type)
        {
            case SDL_MOUSEMOTION: break;
            default: pushevent(event); break;
        }
    }
    lastintercept = sym;
    if(sym != SDLK_UNKNOWN) for(int i = len; i < events.length(); i++)
    {
        if(events[i].type == SDL_KEYDOWN && events[i].key.keysym.sym == sym) { events.remove(i); return true; }
    }
    return false;
}

static void ignoremousemotion()
{
    SDL_Event e;
    SDL_PumpEvents();
    while(SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_MOUSEMOTION)));
}

static void resetmousemotion()
{
#ifndef WIN32
    if(grabinput && !(screen->flags&SDL_FULLSCREEN))
    {
        SDL_WarpMouse(screen->w / 2, screen->h / 2);
    }
#endif
}

static void checkmousemotion(int &dx, int &dy)
{
    loopv(events)
    {
        SDL_Event &event = events[i];
        if(event.type != SDL_MOUSEMOTION)
        {
            if(i > 0) events.remove(0, i);
            return;
        }
        dx += event.motion.xrel;
        dy += event.motion.yrel;
    }
    events.setsize(0);
    SDL_Event event;
    while(pollevent(event))
    {
        if(event.type != SDL_MOUSEMOTION)
        {
            events.add(event);
            return;
        }
        dx += event.motion.xrel;
        dy += event.motion.yrel;
    }
}

void checkinput()
{
    SDL_Event event;
    int lasttype = 0, lastbut = 0;
    bool mousemoved = false;
    while(events.length() || pollevent(event))
    {
        if(events.length()) event = events.remove(0);

        switch(event.type)
        {
            case SDL_QUIT:
                quit();
                return;

            #if !defined(WIN32) && !defined(__APPLE__)
            case SDL_VIDEORESIZE:
                screenres(&event.resize.w, &event.resize.h);
                break;
            #endif

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                keypress(event.key.keysym.sym, event.key.state==SDL_PRESSED, uni2cube(event.key.keysym.unicode));
                break;

            case SDL_ACTIVEEVENT:
                if(event.active.state & SDL_APPINPUTFOCUS)
                    inputgrab(grabinput = event.active.gain!=0);
                if(event.active.state & SDL_APPACTIVE)
                    minimized = !event.active.gain;
                break;

            case SDL_MOUSEMOTION:
                if(grabinput)
                {
                    int dx = event.motion.xrel, dy = event.motion.yrel;
                    checkmousemotion(dx, dy);
                    if(!UI::movecursor(dx, dy) && !UI::hascursor())
                        mousemove(dx, dy);
                    mousemoved = true;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if(lasttype==event.type && lastbut==event.button.button) break; // why?? get event twice without it
                keypress(-event.button.button, event.button.state!=0, 0);
                lasttype = event.type;
                lastbut = event.button.button;
                break;
        }
    }
    if(mousemoved) resetmousemotion();
}


void swapbuffers()
{
    recorder::capture();
    SDL_GL_SwapBuffers();
}

VARF(gamespeed, 10, 100, 1000, if(multiplayer()) gamespeed = 100);

VARF(paused, 0, 0, 1, if(multiplayer()) paused = 0);

VAR(menufps, 0, 60, 1000);
VARP(maxfps, 0, 200, 1000);

void limitfps(int &millis, int curmillis)
{
    int limit = UI::mainmenu && menufps ? (maxfps ? min(maxfps, menufps) : menufps) : maxfps;
    if(!limit) return;
    static int fpserror = 0;
    int delay = 1000/limit - (millis-curmillis);
    if(delay < 0) fpserror = 0;
    else
    {
        fpserror += 1000%limit;
        if(fpserror >= limit)
        {
            ++delay;
            fpserror -= limit;
        }
        if(delay > 0)
        {
            SDL_Delay(delay);
            millis += delay;
        }
    }
}

#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
void stackdumper(unsigned int type, EXCEPTION_POINTERS *ep)
{
    if(!ep) fatal("unknown type");
    EXCEPTION_RECORD *er = ep->ExceptionRecord;
    CONTEXT *context = ep->ContextRecord;
    string out, t;
    formatstring(out)("Lamiae Win32 Exception: 0x%x [0x%x]\n\n", er->ExceptionCode, er->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ? er->ExceptionInformation[1] : -1);
#ifdef _AMD64_
    STACKFRAME64 sf = {{context->Rip, 0, AddrModeFlat}, {}, {context->Rbp, 0, AddrModeFlat}, {context->Rsp, 0, AddrModeFlat}, 0};
    while(::StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), GetCurrentThread(), &sf, context, NULL, ::SymFunctionTableAccess, ::SymGetModuleBase, NULL))
    {
        union { IMAGEHLP_SYMBOL64 sym; char symext[sizeof(IMAGEHLP_SYMBOL64) + sizeof(string)]; };
        sym.SizeOfStruct = sizeof(sym);
        sym.MaxNameLength = sizeof(symext) - sizeof(sym);
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(line);
        DWORD64 symoff;
        DWORD lineoff;
        if(SymGetSymFromAddr64(GetCurrentProcess(), sf.AddrPC.Offset, &symoff, &sym) && SymGetLineFromAddr64(GetCurrentProcess(), sf.AddrPC.Offset, &lineoff, &line))
#else
    STACKFRAME sf = {{context->Eip, 0, AddrModeFlat}, {}, {context->Ebp, 0, AddrModeFlat}, {context->Esp, 0, AddrModeFlat}, 0};
    while(::StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &sf, context, NULL, ::SymFunctionTableAccess, ::SymGetModuleBase, NULL))
    {
        union { IMAGEHLP_SYMBOL sym; char symext[sizeof(IMAGEHLP_SYMBOL) + sizeof(string)]; };
        sym.SizeOfStruct = sizeof(sym);
        sym.MaxNameLength = sizeof(symext) - sizeof(sym);
        IMAGEHLP_LINE line;
        line.SizeOfStruct = sizeof(line);
        DWORD symoff, lineoff;
        if(SymGetSymFromAddr(GetCurrentProcess(), sf.AddrPC.Offset, &symoff, &sym) && SymGetLineFromAddr(GetCurrentProcess(), sf.AddrPC.Offset, &lineoff, &line))
#endif
        {
            char *del = strrchr(line.FileName, '\\');
            formatstring(t)("%s - %s [%d]\n", sym.Name, del ? del + 1 : line.FileName, line.LineNumber);
             concatstring(out, t);
         }
     }
    fatal(out);
}
#endif

#define MAXFPSHISTORY 60

int fpspos = 0, fpshistory[MAXFPSHISTORY];

void resetfpshistory()
{
    loopi(MAXFPSHISTORY) fpshistory[i] = 1;
    fpspos = 0;
}

void updatefpshistory(int millis)
{
    fpshistory[fpspos++] = max(1, min(1000, millis));
    if(fpspos>=MAXFPSHISTORY) fpspos = 0;
}

void getframemillis(float &avg, float &bestdiff, float &worstdiff)
{
    int total = fpshistory[MAXFPSHISTORY-1], best = total, worst = total;
    loopi(MAXFPSHISTORY-1)
    {
        int millis = fpshistory[i];
        total += millis;
        if(millis < best) best = millis;
        if(millis > worst) worst = millis;
    }

    avg = total/float(MAXFPSHISTORY);
    best = best - avg;
    worstdiff = avg - worst;
}

void getfps(int &fps, int &bestdiff, int &worstdiff)
{
    int total = fpshistory[MAXFPSHISTORY-1], best = total, worst = total;
    loopi(MAXFPSHISTORY-1)
    {
        int millis = fpshistory[i];
        total += millis;
        if(millis < best) best = millis;
        if(millis > worst) worst = millis;
    }

    fps = (1000*MAXFPSHISTORY)/total;
    bestdiff = 1000/best-fps;
    worstdiff = fps-1000/worst;
}

void getfps_(int *raw)
{
    int fps, bestdiff, worstdiff;
    if(*raw) fps = 1000/fpshistory[(fpspos+MAXFPSHISTORY-1)%MAXFPSHISTORY];
    else getfps(fps, bestdiff, worstdiff);
    intret(fps);
}

COMMANDN(getfps, getfps_, "i");

bool inbetweenframes = false, renderedframe = true;

static bool findarg(int argc, char **argv, const char *str)
{
    for(int i = 1; i<argc; i++) if(strstr(argv[i], str)==argv[i]) return true;
    return false;
}

static int clockrealbase = 0, clockvirtbase = 0;
static void clockreset() { clockrealbase = SDL_GetTicks(); clockvirtbase = totalmillis; }
VARFP(clockerror, 990000, 1000000, 1010000, clockreset());
VARFP(clockfix, 0, 0, 1, clockreset());

void setdefaults()
{
    execfile("data/defaults.cfg");
    defformatstring(game)("data/%s/defaults.cfg", game::gameident());
    execfile(game, false);
}
COMMAND(setdefaults, "");

int dedicated = 0;

int getclockmillis()
{
    int millis = SDL_GetTicks() - clockrealbase;
    if(clockfix) millis = int(millis*(double(clockerror)/1000000));
    millis += clockvirtbase;
    return max(millis, totalmillis);
}

int main(int argc, char **argv)
{
    #ifdef WIN32
    //atexit((void (__cdecl *)(void))_CrtDumpMemoryLeaks);
    #ifndef _DEBUG
    #ifndef __GNUC__
    __try {
    #endif
    #endif
    #endif

    setlogfile(NULL);
    char *load = NULL, *initscript = NULL;

    initing = INIT_RESET;
    for(int i = 1; i<argc; i++)
    {
        if(argv[i][0]=='-') switch(argv[i][1])
        {
            case 'q':
            {
                const char *dir = sethomedir(&argv[i][2]);
                if(dir) logoutf("Using home directory: %s", dir);
                break;
            }
        }
    }
    execfile("init.cfg", false);
    for(int i = 1; i<argc; i++)
    {
        if(argv[i][0]=='-') switch(argv[i][1])
        {
            case 'q': /* parsed first */ break;
            case 'r': /* compat, ignore */ break;
            case 'k':
            {
                const char *dir = addpackagedir(&argv[i][2]);
                if(dir) logoutf("Adding package directory: %s", dir);
                break;
            }
            case 'g': logoutf("Setting log file", &argv[i][2]); setlogfile(&argv[i][2]); break;
            case 'd': dedicated = atoi(&argv[i][2]); if(dedicated<=0) dedicated = 2; break;
            case 'w': scr_w = clamp(atoi(&argv[i][2]), SCR_MINW, SCR_MAXW); if(!findarg(argc, argv, "-h")) scr_h = -1; break;
            case 'h': scr_h = clamp(atoi(&argv[i][2]), SCR_MINH, SCR_MAXH); if(!findarg(argc, argv, "-w")) scr_w = -1; break;
            case 'z': depthbits = atoi(&argv[i][2]); break;
            case 'b': colorbits = atoi(&argv[i][2]); break;
            case 'a': fsaa = atoi(&argv[i][2]); break;
            case 'v': vsync = atoi(&argv[i][2]); break;
            case 't': fullscreen = atoi(&argv[i][2]); break;
            case 's': stencilbits = atoi(&argv[i][2]); break;
            case 'f': /* compat, ignore */ break;
            case 'l':
            {
                char pkgdir[] = "packages/";
                load = strstr(path(&argv[i][2]), path(pkgdir));
                if(load) load += sizeof(pkgdir)-1;
                else load = &argv[i][2];
                break;
            }
            case 'x': initscript = &argv[i][2]; break;
            default: if(!serveroption(argv[i])) gameargs.add(argv[i]); break;
        }
        else gameargs.add(argv[i]);
    }
    initing = NOT_INITING;

    if(dedicated <= 1)
    {
        logoutf("init: sdl");
        int par = 0;
        #ifdef _DEBUG
        par = SDL_INIT_NOPARACHUTE;
        #ifdef WIN32
        SetEnvironmentVariable("SDL_DEBUG", "1");
        #endif
        #endif
        if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_AUDIO|par)<0) fatal("Unable to initialize SDL: %s", SDL_GetError());
    }

    logoutf("init: net");
    if(enet_initialize()<0) fatal("Unable to initialise network module");
    atexit(enet_deinitialize);
    enet_time_set(0);

    logoutf("init: game");
    game::parseoptions(gameargs);
    initserver(dedicated>0, dedicated>1);  // never returns if dedicated
    ASSERT(dedicated <= 1);
    game::initclient();

    logoutf("init: video: mode");
    const SDL_VideoInfo *video = SDL_GetVideoInfo();
    if(video)
    {
        desktopw = video->current_w;
        desktoph = video->current_h;
    }
    int usedcolorbits = 0, useddepthbits = 0, usedfsaa = 0;
    setupscreen(usedcolorbits, useddepthbits, usedfsaa);

    logoutf("init: video: misc");
    defformatstring(capt)("Lamiae %s", version);
    SDL_WM_SetCaption(capt, NULL);
    keyrepeat(false);
    SDL_ShowCursor(0);

    logoutf("init: gl");
    gl_checkextensions();
    gl_init(scr_w, scr_h, usedcolorbits, useddepthbits, usedfsaa);
    notexture = textureload("data/notexture");
    if(!notexture) fatal("could not find core textures");
    UI::setup();

    logoutf("init: console");
    if(!execfile("data/stdlib.cfg", false)) fatal("cannot find data files (you are running from the wrong folder, try .bat file in the main folder)");   // this is the first file we load.
    if(!execfile("data/font.cfg", false)) fatal("cannot find font definitions");
    if(!setfont("default")) fatal("no default font specified");

    inbetweenframes = true;
    renderbackground("initialising...");

    logoutf("init: world");
    camera1 = player = game::iterdynents(0);
    emptymap(0, true, NULL, false);

    logoutf("init: sound");
    initsound();

    logoutf("init: cfg");
    execfile("data/keymap.cfg");
    execfile("data/stdedit.cfg");
    defformatstring(confname)("data/%s/std.cfg", game::gameident());
    execfile(confname, false);

    formatstring(confname)("data/%s/sounds.cfg", game::gameident());
    if(!execfile(confname, false))
        execfile("data/sounds.cfg");

    execfile("data/brush.cfg");
    execfile("mybrushes.cfg", false);

    execfile("data/ui.cfg");
    formatstring(confname)("data/%s/ui.cfg", game::gameident());
    execfile(confname, false);

    if(game::savedservers()) execfile(game::savedservers(), false);

    identflags |= IDF_PERSIST;

    initing = INIT_LOAD;
    formatstring(confname)("config_%s.cfg", game::gameident());
    if(!execfile(confname, false))
        setdefaults();

    execfile(game::autoexec(), false);
    initing = NOT_INITING;

    identflags &= ~IDF_PERSIST;
    initing = INIT_GAME;

    formatstring(confname)("data/%s/game.cfg", game::gameident());
    execfile(confname, false);

    initing = NOT_INITING;

    logoutf("init: shaders");
    initgbuffer();
    loadshaders();
    particleinit();
    initdecals();

    identflags |= IDF_PERSIST;

    logoutf("init: mainloop");

    if(execfile("once.cfg", false)) remove(findfile("once.cfg", "rb"));

    if(load)
    {
        logoutf("init: localconnect");
        //localconnect();
        game::changemap(load);
    }

    if(initscript) execute(initscript);

    initmumble();
    resetfpshistory();

    inputgrab(grabinput = true);
    ignoremousemotion();

    for(;;)
    {
        static int frames = 0;
        int millis = getclockmillis();
        limitfps(millis, totalmillis);
        int elapsed = millis-totalmillis;
        if(multiplayer(false)) curtime = game::ispaused() ? 0 : elapsed;
        else
        {
            static int timeerr = 0;
            int scaledtime = elapsed*gamespeed + timeerr;
            curtime = scaledtime/100;
            timeerr = scaledtime%100;
            if(curtime>200) curtime = 200;
            if(paused || game::ispaused()) curtime = 0;
        }
        lastmillis += curtime;
        totalmillis = millis;
        extern void updatetime();
        updatetime();

        checkinput();
        UI::update();
        tryedit();

        if(lastmillis)
        {
            game::updateworld();
        }

        checksleep(lastmillis);

        serverslice(false, 0);

        if(frames) updatefpshistory(elapsed);
        frames++;

        // miscellaneous general game effects
        recomputecamera();
        updateparticles();
        updatesounds();

        if(minimized) continue;

        if(!UI::mainmenu) setupframe(screen->w, screen->h);

        inbetweenframes = false;

        if(UI::mainmenu) gl_drawmainmenu(screen->w, screen->h);
        else gl_drawframe(screen->w, screen->h);
        swapbuffers();
        renderedframe = inbetweenframes = true;
    }

    ASSERT(0);
    return EXIT_FAILURE;

    #if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
    } __except(stackdumper(0, GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) { return 0; }
    #endif
}
