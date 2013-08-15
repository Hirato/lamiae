#include "engine.h"

#ifdef HAS_OVR
#define __PLACEMENT_NEW_INLINE
#include "OVR.h"
#endif

namespace ovr
{
    float fitx = -1.0f, fity = 0.0f, viewoffset = 0.0f, distortoffset = 0.0f, distortscale = 1.0f, fov = 0.0f;
    float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;

    GLuint lensfbo[2] = { 0, 0 }, lenstex[2] = { 0, 0 };
    int lensw = -1, lensh = -1;

    VARFN(ovr, enabled, 0, 0, 1,
    {
        cleanup();
        cleanupgbuffer();
        if(enabled && !enable()) enabled = 0;
        if(!enabled) disable();
    });

    VARP(ovrautofov, 0, 0, 1);
    FVAR(ovrhuddist, 0.01f, 1, 100);
    FVAR(ovrhudfov, 10, 75, 150);

    VARFP(ovrscale, 0, 0, 1, { cleanup(); cleanupgbuffer(); });
    VARFP(ovralignx, -1, -1, 1, { cleanup(); cleanupgbuffer(); });
    VARFP(ovraligny, -1, -1, 1, { cleanup(); cleanupgbuffer(); });

    FVAR(ovrviewoffsetscale, 0, 8, 1e3f);
    VAR(ovrdistort, 0, 1, 1);

#ifndef HAS_OVR
    void init() {}
    void destroy() {}
    void setup() {}
    void cleanup() {}
    bool enable() { return false; }
    void disable() {}
    void reset() {}
    void update() {}
    void warp() {}
    void ortho(glmatrix &m, float dist, float fov) {}
#else
    using namespace OVR;

    DeviceManager *manager = NULL;
    HMDDevice *hmd = NULL;
    SensorDevice *sensor = NULL;
    SensorFusion fusion;
    HMDInfo hmdinfo;

    bool inited = false;

    void init()
    {
        if(inited) return;
        System::Init();
        inited = true;
    }

    static inline float distortfn(float r)
    {
        float r2 = r*r;
        return r*(hmdinfo.DistortionK[0] +
                 r2*(hmdinfo.DistortionK[1] +
                   r2*(hmdinfo.DistortionK[2] +
                     r2*hmdinfo.DistortionK[3])));
    }

    void ortho(glmatrix &m, float dist, float fov)
    {
        if(dist <= 0) dist = ovrhuddist;
        if(fov <= 0) fov = ovrhudfov;

        float size = hmdinfo.VResolution/hmdinfo.VScreenSize * 2.0f*tan(fov*RAD*0.5f)*hmdinfo.EyeToScreenDistance/distortscale;
        m.scalexy(size/hudw, size/hudh);

        float shift = (hmdinfo.EyeToScreenDistance/dist) * hmdinfo.InterpupillaryDistance/hmdinfo.HScreenSize,
              offset = hmdinfo.HResolution * (0.5f*distortoffset + shift/distortscale);
        m.jitter((viewidx ? -1 : 1)*offset/hudw, 0);
    }

    void getorient()
    {
        Quatf orient = fusion.GetOrientation();
        yaw = pitch = roll = 0;
        orient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &pitch, &roll);
        yaw /= -RAD;
        pitch /= RAD;
        roll /= -RAD;
    }

    void update()
    {
        if(!enabled) return;
        float lastyaw = yaw, lastpitch = pitch;
        getorient();
        modifyorient(yaw - lastyaw, pitch - lastpitch);
    }

    void recalc()
    {
        if(hmdinfo.HScreenSize > 0.140f) { fitx = -1.0f; fity = 0.0f; }
        else { fitx = 0.0f; fity = 1.0f; }

        viewoffset = 0.5f*hmdinfo.InterpupillaryDistance * ovrviewoffsetscale;

        distortoffset = 1.0f - 2.0f*hmdinfo.LensSeparationDistance/hmdinfo.HScreenSize;

        if(ovrdistort && (fitx || fity))
        {
            float dx = fitx - distortoffset, dy = (fity * hudh) / hudw, r = sqrtf(dx*dx + dy*dy);
            distortscale = distortfn(r) / r;
        }
        else distortscale = 1.0f;

        if(ovrautofov)
        {
            float aspect = forceaspect ? forceaspect : hudw/float(hudh);
            fov = 2*atan(0.5f*hmdinfo.VScreenSize*distortscale*aspect/hmdinfo.EyeToScreenDistance)/RAD;
        }
        else fov = 0;
    }

    void setup()
    {
        if(!enabled) return;
        if(ovrscale)
        {
            hudw = hudw/2;
            renderw = renderw/2;
        }
        else
        {
            hudw = renderw = hmdinfo.HResolution/2;
            hudh = renderh = hmdinfo.VResolution;
        }
        recalc();
        if(hudw == lensw && hudh == lensh) return;
        lensw = hudw;
        lensh = hudh;
        loopi(2)
        {
            if(!lensfbo[i]) glGenFramebuffers_(1, &lensfbo[i]);
            if(!lenstex[i]) glGenTextures(1, &lenstex[i]);
            glBindFramebuffer_(GL_FRAMEBUFFER, lensfbo[i]);
            createtexture(lenstex[i], lensw, lensh, NULL, 3, 1, GL_RGB, GL_TEXTURE_RECTANGLE);
            glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            GLfloat border[4] = { 0, 0, 0, 1 };
            glTexParameterfv(GL_TEXTURE_RECTANGLE, GL_TEXTURE_BORDER_COLOR, border);
            glFramebufferTexture2D_(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, lenstex[i], 0);
            if(glCheckFramebufferStatus_(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                fatal("failed allocating OVR buffer!");
        }
        glBindFramebuffer_(GL_FRAMEBUFFER, 0);
        useshaderbyname("ovrwarp");
        loopi(3)
        {
            glBindFramebuffer_(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, screenw, screenh);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            SDL_GL_SwapWindow(screen);
        }
    }

    void cleanup()
    {
        if(lensfbo[0])
        {
            glBindFramebuffer_(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, screenw, screenh);
        }
        loopi(2)
        {
            if(lensfbo[i]) { glDeleteFramebuffers_(1, &lensfbo[i]); lensfbo[i] = 0; }
            if(lenstex[i]) { glDeleteTextures(1, &lenstex[i]); lenstex[i] = 0; }
        }
        lensw = lensh = -1;
    }

    void warp()
    {
        int x = hudx, y = 0, w = hudw, h = hudh;
        if(ovrscale)
        {
            x = viewidx*(screenw/2);
            w = screenw/2;
            h = screenh;
        }
        else
        {
            if(ovralignx > 0) x += max(screenw - 2*hudw, 0);
            else if(!ovralignx) x += max(screenw - 2*hudw, 0)/2;
            if(ovraligny < 0) y += max(screenh - hudh, 0);
            else if(!ovraligny) y += max(screenh - hudh, 0)/2;
        }
        if(x >= screenw || y >= screenh) return;
        glBindFramebuffer_(GL_FRAMEBUFFER, 0);
        glViewport(x, y, min(w, screenw - x), min(h, screenh - y));
        glBindTexture(GL_TEXTURE_RECTANGLE, lenstex[viewidx]);
        SETSHADER(ovrwarp);
        LOCALPARAMF(lenscenter, (1 + (viewidx ? -1 : 1)*distortoffset)*0.5f*hudw, 0.5f*hudh);
        LOCALPARAMF(lensscale, 2.0f/hudw, 2.0f/(hudh*aspect), 0.5f*hudw/distortscale, 0.5f*hudh*aspect/distortscale);
        LOCALPARAMF(distortk, hmdinfo.DistortionK[0], hmdinfo.DistortionK[1], hmdinfo.DistortionK[2], hmdinfo.DistortionK[3]);
        screenquad(min(w, screenw - x)/float(w)*hudw, min(h, screenh - y)/float(h)*hudh);
    }

    void disable()
    {
        if(sensor) { sensor->Release(); sensor = NULL; }
        if(hmd) { hmd->Release(); hmd = NULL; }
        if(manager) { manager->Release(); manager = NULL; }
    }

    void reset()
    {
        if(sensor)
        {
            fusion.Reset();
            getorient();
        }
    }

    bool enable()
    {
        init();
        if(!manager) manager = DeviceManager::Create();
        if(!hmd) hmd = manager->EnumerateDevices<HMDDevice>().CreateDevice();
        if(hmd)
        {
            if(hmd->GetDeviceInfo(&hmdinfo))
            {
                if(!sensor) sensor = hmd->GetSensor();
                if(sensor)
                {
                    fusion.AttachToSensor(sensor);
                    fusion.SetPredictionEnabled(true);
                    getorient();
                }
                conoutf("detected %s (%s), %ux%u, %.1fcm x %.1fcm, %s", hmdinfo.ProductName, hmdinfo.Manufacturer, hmdinfo.HResolution, hmdinfo.VResolution, hmdinfo.HScreenSize*100.0f, hmdinfo.VScreenSize*100.0f, sensor ? "using sensor" : "no sensor");
                if(screen) SDL_SetWindowPosition(screen, 0, 0);
            }
            else { hmd->Release(); hmd = NULL; }
        }
        return hmd!=NULL;
    }

    void destroy()
    {
        disable();

#ifdef WIN32
        if(inited) { System::Destroy(); inited = false; }
#endif
    }
#endif
}
