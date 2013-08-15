// rendergl.cpp: core opengl rendering stuff

#include "engine.h"

bool hasVAO = false, hasTR = false, hasTSW = false, hasFBO = false, hasAFBO = false, hasDS = false, hasTF = false, hasCBF = false, hasS3TC = false, hasFXT1 = false, hasLATC = false, hasRGTC = false, hasAF = false, hasFBB = false, hasFBMS = false, hasTMS = false, hasMSS = false, hasFBMSBS = false, hasNVFBMSC = false, hasNVTMS = false, hasUBO = false, hasMBR = false, hasDB2 = false, hasDBB = false, hasTG = false, hasT4 = false, hasTQ = false, hasPF = false, hasTRG = false, hasDBT = false, hasDC = false, hasDBGO = false, hasGPU4 = false, hasGPU5 = false, hasEAL = false, hasCR = false, hasOQ2 = false;
bool mesa = false, intel = false, ati = false, nvidia = false;

int hasstencil = 0;

VAR(renderpath, 1, 0, 0);
VAR(glversion, 1, 0, 0);
VAR(glslversion, 1, 0, 0);

// GL_EXT_timer_query
PFNGLGETQUERYOBJECTI64VEXTPROC glGetQueryObjecti64v_  = NULL;
PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64v_ = NULL;

// GL_EXT_framebuffer_object
PFNGLBINDRENDERBUFFERPROC        glBindRenderbuffer_        = NULL;
PFNGLDELETERENDERBUFFERSPROC     glDeleteRenderbuffers_     = NULL;
PFNGLGENFRAMEBUFFERSPROC         glGenRenderbuffers_        = NULL;
PFNGLRENDERBUFFERSTORAGEPROC     glRenderbufferStorage_     = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC  glCheckFramebufferStatus_  = NULL;
PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer_         = NULL;
PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers_      = NULL;
PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers_         = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC    glFramebufferTexture1D_    = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D_    = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC    glFramebufferTexture3D_    = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer_ = NULL;
PFNGLGENERATEMIPMAPPROC          glGenerateMipmap_          = NULL;

// GL_EXT_framebuffer_blit
PFNGLBLITFRAMEBUFFERPROC         glBlitFramebuffer_         = NULL;

// GL_EXT_framebuffer_multisample
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample_ = NULL;

// GL_ARB_texture_multisample
PFNGLTEXIMAGE2DMULTISAMPLEPROC glTexImage2DMultisample_ = NULL;
PFNGLTEXIMAGE3DMULTISAMPLEPROC glTexImage3DMultisample_ = NULL;
PFNGLGETMULTISAMPLEFVPROC      glGetMultisamplefv_      = NULL;
PFNGLSAMPLEMASKIPROC           glSampleMaski_           = NULL;

// GL_ARB_sample_shading
PFNGLMINSAMPLESHADINGPROC glMinSampleShading_ = NULL;

// GL_ARB_draw_buffers_blend
PFNGLBLENDEQUATIONIPROC         glBlendEquationi_         = NULL;
PFNGLBLENDEQUATIONSEPARATEIPROC glBlendEquationSeparatei_ = NULL;
PFNGLBLENDFUNCIPROC             glBlendFunci_             = NULL;
PFNGLBLENDFUNCSEPARATEIPROC     glBlendFuncSeparatei_     = NULL;

// GL_NV_framebuffer_multisample_coverage
PFNGLRENDERBUFFERSTORAGEMULTISAMPLECOVERAGENVPROC glRenderbufferStorageMultisampleCoverageNV_ = NULL;

// GL_NV_texture_multisample
PFNGLTEXIMAGE2DMULTISAMPLECOVERAGENVPROC     glTexImage2DMultisampleCoverageNV_     = NULL;
PFNGLTEXIMAGE3DMULTISAMPLECOVERAGENVPROC     glTexImage3DMultisampleCoverageNV_     = NULL;
PFNGLTEXTUREIMAGE2DMULTISAMPLENVPROC         glTextureImage2DMultisampleNV_         = NULL;
PFNGLTEXTUREIMAGE3DMULTISAMPLENVPROC         glTextureImage3DMultisampleNV_         = NULL;
PFNGLTEXTUREIMAGE2DMULTISAMPLECOVERAGENVPROC glTextureImage2DMultisampleCoverageNV_ = NULL;
PFNGLTEXTUREIMAGE3DMULTISAMPLECOVERAGENVPROC glTextureImage3DMultisampleCoverageNV_ = NULL;

// OpenGL 2.1
#ifndef __APPLE__
PFNGLACTIVETEXTUREPROC    glActiveTexture_    = NULL;

PFNGLBLENDEQUATIONEXTPROC glBlendEquation_ = NULL;
PFNGLBLENDCOLOREXTPROC    glBlendColor_    = NULL;

PFNGLTEXIMAGE3DPROC        glTexImage3D_        = NULL;
PFNGLTEXSUBIMAGE3DPROC     glTexSubImage3D_     = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC glCopyTexSubImage3D_ = NULL;

PFNGLCOMPRESSEDTEXIMAGE3DPROC    glCompressedTexImage3D_    = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC    glCompressedTexImage2D_    = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DPROC    glCompressedTexImage1D_    = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D_ = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D_ = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glCompressedTexSubImage1D_ = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC   glGetCompressedTexImage_   = NULL;

PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements_ = NULL;
PFNGLMULTIDRAWARRAYSPROC   glMultiDrawArrays_   = NULL;
PFNGLMULTIDRAWELEMENTSPROC glMultiDrawElements_ = NULL;

PFNGLBLENDFUNCSEPARATEPROC     glBlendFuncSeparate_     = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate_ = NULL;
PFNGLSTENCILOPSEPARATEPROC     glStencilOpSeparate_     = NULL;
PFNGLSTENCILFUNCSEPARATEPROC   glStencilFuncSeparate_   = NULL;
PFNGLSTENCILMASKSEPARATEPROC   glStencilMaskSeparate_   = NULL;

PFNGLGENBUFFERSPROC       glGenBuffers_       = NULL;
PFNGLBINDBUFFERPROC       glBindBuffer_       = NULL;
PFNGLMAPBUFFERPROC        glMapBuffer_        = NULL;
PFNGLUNMAPBUFFERPROC      glUnmapBuffer_      = NULL;
PFNGLBUFFERDATAPROC       glBufferData_       = NULL;
PFNGLBUFFERSUBDATAPROC    glBufferSubData_    = NULL;
PFNGLDELETEBUFFERSPROC    glDeleteBuffers_    = NULL;
PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData_ = NULL;

PFNGLGENQUERIESPROC        glGenQueries_        = NULL;
PFNGLDELETEQUERIESPROC     glDeleteQueries_     = NULL;
PFNGLBEGINQUERYPROC        glBeginQuery_        = NULL;
PFNGLENDQUERYPROC          glEndQuery_          = NULL;
PFNGLGETQUERYIVPROC        glGetQueryiv_        = NULL;
PFNGLGETQUERYOBJECTIVPROC  glGetQueryObjectiv_  = NULL;
PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv_ = NULL;

PFNGLCREATEPROGRAMPROC            glCreateProgram_            = NULL;
PFNGLDELETEPROGRAMPROC            glDeleteProgram_            = NULL;
PFNGLUSEPROGRAMPROC               glUseProgram_               = NULL;
PFNGLCREATESHADERPROC             glCreateShader_             = NULL;
PFNGLDELETESHADERPROC             glDeleteShader_             = NULL;
PFNGLSHADERSOURCEPROC             glShaderSource_             = NULL;
PFNGLCOMPILESHADERPROC            glCompileShader_            = NULL;
PFNGLGETSHADERIVPROC              glGetShaderiv_              = NULL;
PFNGLGETPROGRAMIVPROC             glGetProgramiv_             = NULL;
PFNGLATTACHSHADERPROC             glAttachShader_             = NULL;
PFNGLGETPROGRAMINFOLOGPROC        glGetProgramInfoLog_        = NULL;
PFNGLGETSHADERINFOLOGPROC         glGetShaderInfoLog_         = NULL;
PFNGLLINKPROGRAMPROC              glLinkProgram_              = NULL;
PFNGLGETUNIFORMLOCATIONPROC       glGetUniformLocation_       = NULL;
PFNGLUNIFORM1FPROC                glUniform1f_                = NULL;
PFNGLUNIFORM2FPROC                glUniform2f_                = NULL;
PFNGLUNIFORM3FPROC                glUniform3f_                = NULL;
PFNGLUNIFORM4FPROC                glUniform4f_                = NULL;
PFNGLUNIFORM1FVPROC               glUniform1fv_               = NULL;
PFNGLUNIFORM2FVPROC               glUniform2fv_               = NULL;
PFNGLUNIFORM3FVPROC               glUniform3fv_               = NULL;
PFNGLUNIFORM4FVPROC               glUniform4fv_               = NULL;
PFNGLUNIFORM1IPROC                glUniform1i_                = NULL;
PFNGLUNIFORM2IPROC                glUniform2i_                = NULL;
PFNGLUNIFORM3IPROC                glUniform3i_                = NULL;
PFNGLUNIFORM4IPROC                glUniform4i_                = NULL;
PFNGLUNIFORM1IVPROC               glUniform1iv_               = NULL;
PFNGLUNIFORM2IVPROC               glUniform2iv_               = NULL;
PFNGLUNIFORM3IVPROC               glUniform3iv_               = NULL;
PFNGLUNIFORM4IVPROC               glUniform4iv_               = NULL;
PFNGLUNIFORMMATRIX2FVPROC         glUniformMatrix2fv_         = NULL;
PFNGLUNIFORMMATRIX3FVPROC         glUniformMatrix3fv_         = NULL;
PFNGLUNIFORMMATRIX4FVPROC         glUniformMatrix4fv_         = NULL;
PFNGLBINDATTRIBLOCATIONPROC       glBindAttribLocation_       = NULL;
PFNGLGETACTIVEUNIFORMPROC         glGetActiveUniform_         = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray_  = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray_ = NULL;

PFNGLVERTEXATTRIB1FPROC           glVertexAttrib1f_           = NULL;
PFNGLVERTEXATTRIB1FVPROC          glVertexAttrib1fv_          = NULL;
PFNGLVERTEXATTRIB1SPROC           glVertexAttrib1s_           = NULL;
PFNGLVERTEXATTRIB1SVPROC          glVertexAttrib1sv_          = NULL;
PFNGLVERTEXATTRIB2FPROC           glVertexAttrib2f_           = NULL;
PFNGLVERTEXATTRIB2FVPROC          glVertexAttrib2fv_          = NULL;
PFNGLVERTEXATTRIB2SPROC           glVertexAttrib2s_           = NULL;
PFNGLVERTEXATTRIB2SVPROC          glVertexAttrib2sv_          = NULL;
PFNGLVERTEXATTRIB3FPROC           glVertexAttrib3f_           = NULL;
PFNGLVERTEXATTRIB3FVPROC          glVertexAttrib3fv_          = NULL;
PFNGLVERTEXATTRIB3SPROC           glVertexAttrib3s_           = NULL;
PFNGLVERTEXATTRIB3SVPROC          glVertexAttrib3sv_          = NULL;
PFNGLVERTEXATTRIB4FPROC           glVertexAttrib4f_           = NULL;
PFNGLVERTEXATTRIB4FVPROC          glVertexAttrib4fv_          = NULL;
PFNGLVERTEXATTRIB4SPROC           glVertexAttrib4s_           = NULL;
PFNGLVERTEXATTRIB4SVPROC          glVertexAttrib4sv_          = NULL;
PFNGLVERTEXATTRIB4BVPROC          glVertexAttrib4bv_          = NULL;
PFNGLVERTEXATTRIB4IVPROC          glVertexAttrib4iv_          = NULL;
PFNGLVERTEXATTRIB4UBVPROC         glVertexAttrib4ubv_         = NULL;
PFNGLVERTEXATTRIB4UIVPROC         glVertexAttrib4uiv_         = NULL;
PFNGLVERTEXATTRIB4USVPROC         glVertexAttrib4usv_         = NULL;
PFNGLVERTEXATTRIB4NBVPROC         glVertexAttrib4Nbv_         = NULL;
PFNGLVERTEXATTRIB4NIVPROC         glVertexAttrib4Niv_         = NULL;
PFNGLVERTEXATTRIB4NUBPROC         glVertexAttrib4Nub_         = NULL;
PFNGLVERTEXATTRIB4NUBVPROC        glVertexAttrib4Nubv_        = NULL;
PFNGLVERTEXATTRIB4NUIVPROC        glVertexAttrib4Nuiv_        = NULL;
PFNGLVERTEXATTRIB4NUSVPROC        glVertexAttrib4Nusv_        = NULL;
PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer_      = NULL;

PFNGLUNIFORMMATRIX2X3FVPROC       glUniformMatrix2x3fv_       = NULL;
PFNGLUNIFORMMATRIX3X2FVPROC       glUniformMatrix3x2fv_       = NULL;
PFNGLUNIFORMMATRIX2X4FVPROC       glUniformMatrix2x4fv_       = NULL;
PFNGLUNIFORMMATRIX4X2FVPROC       glUniformMatrix4x2fv_       = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC       glUniformMatrix3x4fv_       = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC       glUniformMatrix4x3fv_       = NULL;

PFNGLDRAWBUFFERSPROC glDrawBuffers_ = NULL;
#endif

// OpenGL 3.0
PFNGLGETSTRINGIPROC           glGetStringi_           = NULL;
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation_ = NULL;

// GL_EXT_draw_buffers2
PFNGLCOLORMASKIPROC glColorMaski_ = NULL;
PFNGLENABLEIPROC    glEnablei_    = NULL;
PFNGLDISABLEIPROC   glDisablei_   = NULL;

// GL_NV_conditional_render
PFNGLBEGINCONDITIONALRENDERPROC glBeginConditionalRender_ = NULL;
PFNGLENDCONDITIONALRENDERPROC   glEndConditionalRender_   = NULL;

// GL_ARB_uniform_buffer_object
PFNGLGETUNIFORMINDICESPROC       glGetUniformIndices_       = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC     glGetActiveUniformsiv_     = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC    glGetUniformBlockIndex_    = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glGetActiveUniformBlockiv_ = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC     glUniformBlockBinding_     = NULL;
PFNGLBINDBUFFERBASEPROC          glBindBufferBase_          = NULL;
PFNGLBINDBUFFERRANGEPROC         glBindBufferRange_         = NULL;

// GL_EXT_depth_bounds_test
PFNGLDEPTHBOUNDSEXTPROC glDepthBounds_ = NULL;

// GL_ARB_color_buffer_float
PFNGLCLAMPCOLORPROC glClampColor_ = NULL;

// GL_ARB_debug_output
PFNGLDEBUGMESSAGECONTROLARBPROC  glDebugMessageControl_  = NULL;
PFNGLDEBUGMESSAGEINSERTARBPROC   glDebugMessageInsert_   = NULL;
PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallback_ = NULL;
PFNGLGETDEBUGMESSAGELOGARBPROC   glGetDebugMessageLog_   = NULL;

// GL_ARB_map_buffer_range
PFNGLMAPBUFFERRANGEPROC         glMapBufferRange_         = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC glFlushMappedBufferRange_ = NULL;

// GL_ARB_vertex_array_object
PFNGLBINDVERTEXARRAYPROC    glBindVertexArray_    = NULL;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays_ = NULL;
PFNGLGENVERTEXARRAYSPROC    glGenVertexArrays_    = NULL;
PFNGLISVERTEXARRAYPROC      glIsVertexArray_      = NULL;

void *getprocaddress(const char *name)
{
    return SDL_GL_GetProcAddress(name);
}

VAR(glerr, 0, 0, 1);

void glerror(const char *file, int line, GLenum error)
{
    const char *desc = "unknown";
    switch(error)
    {
    case GL_NO_ERROR: desc = "no error"; break;
    case GL_INVALID_ENUM: desc = "invalid enum"; break;
    case GL_INVALID_VALUE: desc = "invalid value"; break;
    case GL_INVALID_OPERATION: desc = "invalid operation"; break;
    case GL_STACK_OVERFLOW: desc = "stack overflow"; break;
    case GL_STACK_UNDERFLOW: desc = "stack underflow"; break;
    case GL_OUT_OF_MEMORY: desc = "out of memory"; break;
    }
    printf("GL error: %s:%d: %s (%x)\n", file, line, desc, error);
}

VAR(ati_pf_bug, 0, 0, 1);
VAR(useubo, 1, 0, 0);
VAR(usetexgather, 1, 0, 0);
VAR(usetexcompress, 1, 0, 0);

static bool checkseries(const char *s, const char *name, int low, int high)
{
    if(name) s = strstr(s, name);
    if(!s) return false;
    while(*s && !isdigit(*s)) ++s;
    if(!*s) return false;
    int n = 0;
    while(isdigit(*s)) n = n*10 + (*s++ - '0');
    return n >= low && n <= high;
}

VAR(dbgexts, 0, 0, 1);

hashset<const char *> glexts;

void parseglexts()
{
    if(glversion >= 300)
    {
        GLint numexts = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numexts);
        loopi(numexts)
        {
            const char *ext = (const char *)glGetStringi_(GL_EXTENSIONS, i);
            glexts.add(newstring(ext));
        }
    }
    else
    {
        const char *exts = (const char *)glGetString(GL_EXTENSIONS);
        for(;;)
        {
            while(*exts == ' ') exts++;
            if(!*exts) break;
            const char *ext = exts;
            while(*exts && *exts != ' ') exts++;
            if(exts > ext) glexts.add(newstring(ext, size_t(exts-ext)));
        }
    }
}

bool hasext(const char *ext)
{
    return glexts.access(ext)!=NULL;
}

void gl_checkextensions()
{
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    const char *version = (const char *)glGetString(GL_VERSION);
    conoutf(CON_INIT, "Renderer: %s (%s)", renderer, vendor);
    conoutf(CON_INIT, "Driver: %s", version);

#ifdef __APPLE__
    // extern int mac_osversion();
    // int osversion = mac_osversion();  /* 0x0A0600 = 10.6, assumed minimum */
#endif

    if(strstr(renderer, "Mesa") || strstr(version, "Mesa"))
    {
        mesa = true;
        if(strstr(renderer, "Intel")) intel = true;
    }
    else if(strstr(vendor, "NVIDIA"))
        nvidia = true;
    else if(strstr(vendor, "ATI") || strstr(vendor, "Advanced Micro Devices"))
        ati = true;
    else if(strstr(vendor, "Intel"))
        intel = true;

    uint glmajorversion, glminorversion;
    if(sscanf(version, " %u.%u", &glmajorversion, &glminorversion) != 2) glversion = 100;
    else glversion = glmajorversion*100 + glminorversion*10;

    if(glversion < 210) fatal("OpenGL 2.1 or greater is required!");

#ifndef __APPLE__
    glActiveTexture_ =            (PFNGLACTIVETEXTUREPROC)            getprocaddress("glActiveTexture");

    glBlendEquation_ =            (PFNGLBLENDEQUATIONPROC)            getprocaddress("glBlendEquation");
    glBlendColor_ =               (PFNGLBLENDCOLORPROC)               getprocaddress("glBlendColor");

    glTexImage3D_ =               (PFNGLTEXIMAGE3DPROC)               getprocaddress("glTexImage3D");
    glTexSubImage3D_ =            (PFNGLTEXSUBIMAGE3DPROC)            getprocaddress("glTexSubImage3D");
    glCopyTexSubImage3D_ =        (PFNGLCOPYTEXSUBIMAGE3DPROC)        getprocaddress("glCopyTexSubImage3D");

    glCompressedTexImage3D_ =     (PFNGLCOMPRESSEDTEXIMAGE3DPROC)     getprocaddress("glCompressedTexImage3D");
    glCompressedTexImage2D_ =     (PFNGLCOMPRESSEDTEXIMAGE2DPROC)     getprocaddress("glCompressedTexImage2D");
    glCompressedTexImage1D_ =     (PFNGLCOMPRESSEDTEXIMAGE1DPROC)     getprocaddress("glCompressedTexImage1D");
    glCompressedTexSubImage3D_ =  (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)  getprocaddress("glCompressedTexSubImage3D");
    glCompressedTexSubImage2D_ =  (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)  getprocaddress("glCompressedTexSubImage2D");
    glCompressedTexSubImage1D_ =  (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)  getprocaddress("glCompressedTexSubImage1D");
    glGetCompressedTexImage_ =    (PFNGLGETCOMPRESSEDTEXIMAGEPROC)    getprocaddress("glGetCompressedTexImage");

    glDrawRangeElements_ =        (PFNGLDRAWRANGEELEMENTSPROC)        getprocaddress("glDrawRangeElements");
    glMultiDrawArrays_ =          (PFNGLMULTIDRAWARRAYSPROC)          getprocaddress("glMultiDrawArrays");
    glMultiDrawElements_ =        (PFNGLMULTIDRAWELEMENTSPROC)        getprocaddress("glMultiDrawElements");

    glBlendFuncSeparate_ =        (PFNGLBLENDFUNCSEPARATEPROC)        getprocaddress("glBlendFuncSeparate");
    glBlendEquationSeparate_ =    (PFNGLBLENDEQUATIONSEPARATEPROC)    getprocaddress("glBlendEquationSeparate");
    glStencilOpSeparate_ =        (PFNGLSTENCILOPSEPARATEPROC)        getprocaddress("glStencilOpSeparate");
    glStencilFuncSeparate_ =      (PFNGLSTENCILFUNCSEPARATEPROC)      getprocaddress("glStencilFuncSeparate");
    glStencilMaskSeparate_ =      (PFNGLSTENCILMASKSEPARATEPROC)      getprocaddress("glStencilMaskSeparate");

    glGenBuffers_ =               (PFNGLGENBUFFERSPROC)               getprocaddress("glGenBuffers");
    glBindBuffer_ =               (PFNGLBINDBUFFERPROC)               getprocaddress("glBindBuffer");
    glMapBuffer_ =                (PFNGLMAPBUFFERPROC)                getprocaddress("glMapBuffer");
    glUnmapBuffer_ =              (PFNGLUNMAPBUFFERPROC)              getprocaddress("glUnmapBuffer");
    glBufferData_ =               (PFNGLBUFFERDATAPROC)               getprocaddress("glBufferData");
    glBufferSubData_ =            (PFNGLBUFFERSUBDATAPROC)            getprocaddress("glBufferSubData");
    glDeleteBuffers_ =            (PFNGLDELETEBUFFERSPROC)            getprocaddress("glDeleteBuffers");
    glGetBufferSubData_ =         (PFNGLGETBUFFERSUBDATAPROC)         getprocaddress("glGetBufferSubData");

    glGetQueryiv_ =               (PFNGLGETQUERYIVPROC)               getprocaddress("glGetQueryiv");
    glGenQueries_ =               (PFNGLGENQUERIESPROC)               getprocaddress("glGenQueries");
    glDeleteQueries_ =            (PFNGLDELETEQUERIESPROC)            getprocaddress("glDeleteQueries");
    glBeginQuery_ =               (PFNGLBEGINQUERYPROC)               getprocaddress("glBeginQuery");
    glEndQuery_ =                 (PFNGLENDQUERYPROC)                 getprocaddress("glEndQuery");
    glGetQueryObjectiv_ =         (PFNGLGETQUERYOBJECTIVPROC)         getprocaddress("glGetQueryObjectiv");
    glGetQueryObjectuiv_ =        (PFNGLGETQUERYOBJECTUIVPROC)        getprocaddress("glGetQueryObjectuiv");

    glCreateProgram_ =            (PFNGLCREATEPROGRAMPROC)            getprocaddress("glCreateProgram");
    glDeleteProgram_ =            (PFNGLDELETEPROGRAMPROC)            getprocaddress("glDeleteProgram");
    glUseProgram_ =               (PFNGLUSEPROGRAMPROC)               getprocaddress("glUseProgram");
    glCreateShader_ =             (PFNGLCREATESHADERPROC)             getprocaddress("glCreateShader");
    glDeleteShader_ =             (PFNGLDELETESHADERPROC)             getprocaddress("glDeleteShader");
    glShaderSource_ =             (PFNGLSHADERSOURCEPROC)             getprocaddress("glShaderSource");
    glCompileShader_ =            (PFNGLCOMPILESHADERPROC)            getprocaddress("glCompileShader");
    glGetShaderiv_ =              (PFNGLGETSHADERIVPROC)              getprocaddress("glGetShaderiv");
    glGetProgramiv_ =             (PFNGLGETPROGRAMIVPROC)             getprocaddress("glGetProgramiv");
    glAttachShader_ =             (PFNGLATTACHSHADERPROC)             getprocaddress("glAttachShader");
    glGetProgramInfoLog_ =        (PFNGLGETPROGRAMINFOLOGPROC)        getprocaddress("glGetProgramInfoLog");
    glGetShaderInfoLog_ =         (PFNGLGETSHADERINFOLOGPROC)         getprocaddress("glGetShaderInfoLog");
    glLinkProgram_ =              (PFNGLLINKPROGRAMPROC)              getprocaddress("glLinkProgram");
    glGetUniformLocation_ =       (PFNGLGETUNIFORMLOCATIONPROC)       getprocaddress("glGetUniformLocation");
    glUniform1f_ =                (PFNGLUNIFORM1FPROC)                getprocaddress("glUniform1f");
    glUniform2f_ =                (PFNGLUNIFORM2FPROC)                getprocaddress("glUniform2f");
    glUniform3f_ =                (PFNGLUNIFORM3FPROC)                getprocaddress("glUniform3f");
    glUniform4f_ =                (PFNGLUNIFORM4FPROC)                getprocaddress("glUniform4f");
    glUniform1fv_ =               (PFNGLUNIFORM1FVPROC)               getprocaddress("glUniform1fv");
    glUniform2fv_ =               (PFNGLUNIFORM2FVPROC)               getprocaddress("glUniform2fv");
    glUniform3fv_ =               (PFNGLUNIFORM3FVPROC)               getprocaddress("glUniform3fv");
    glUniform4fv_ =               (PFNGLUNIFORM4FVPROC)               getprocaddress("glUniform4fv");
    glUniform1i_ =                (PFNGLUNIFORM1IPROC)                getprocaddress("glUniform1i");
    glUniform2i_ =                (PFNGLUNIFORM2IPROC)                getprocaddress("glUniform2i");
    glUniform3i_ =                (PFNGLUNIFORM3IPROC)                getprocaddress("glUniform3i");
    glUniform4i_ =                (PFNGLUNIFORM4IPROC)                getprocaddress("glUniform4i");
    glUniform1iv_ =               (PFNGLUNIFORM1IVPROC)               getprocaddress("glUniform1iv");
    glUniform2iv_ =               (PFNGLUNIFORM2IVPROC)               getprocaddress("glUniform2iv");
    glUniform3iv_ =               (PFNGLUNIFORM3IVPROC)               getprocaddress("glUniform3iv");
    glUniform4iv_ =               (PFNGLUNIFORM4IVPROC)               getprocaddress("glUniform4iv");
    glUniformMatrix2fv_ =         (PFNGLUNIFORMMATRIX2FVPROC)         getprocaddress("glUniformMatrix2fv");
    glUniformMatrix3fv_ =         (PFNGLUNIFORMMATRIX3FVPROC)         getprocaddress("glUniformMatrix3fv");
    glUniformMatrix4fv_ =         (PFNGLUNIFORMMATRIX4FVPROC)         getprocaddress("glUniformMatrix4fv");
    glBindAttribLocation_ =       (PFNGLBINDATTRIBLOCATIONPROC)       getprocaddress("glBindAttribLocation");
    glGetActiveUniform_ =         (PFNGLGETACTIVEUNIFORMPROC)         getprocaddress("glGetActiveUniform");
    glEnableVertexAttribArray_ =  (PFNGLENABLEVERTEXATTRIBARRAYPROC)  getprocaddress("glEnableVertexAttribArray");
    glDisableVertexAttribArray_ = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) getprocaddress("glDisableVertexAttribArray");

    glVertexAttrib1f_ =           (PFNGLVERTEXATTRIB1FPROC)           getprocaddress("glVertexAttrib1f");
    glVertexAttrib1fv_ =          (PFNGLVERTEXATTRIB1FVPROC)          getprocaddress("glVertexAttrib1fv");
    glVertexAttrib1s_ =           (PFNGLVERTEXATTRIB1SPROC)           getprocaddress("glVertexAttrib1s");
    glVertexAttrib1sv_ =          (PFNGLVERTEXATTRIB1SVPROC)          getprocaddress("glVertexAttrib1sv");
    glVertexAttrib2f_ =           (PFNGLVERTEXATTRIB2FPROC)           getprocaddress("glVertexAttrib2f");
    glVertexAttrib2fv_ =          (PFNGLVERTEXATTRIB2FVPROC)          getprocaddress("glVertexAttrib2fv");
    glVertexAttrib2s_ =           (PFNGLVERTEXATTRIB2SPROC)           getprocaddress("glVertexAttrib2s");
    glVertexAttrib2sv_ =          (PFNGLVERTEXATTRIB2SVPROC)          getprocaddress("glVertexAttrib2sv");
    glVertexAttrib3f_ =           (PFNGLVERTEXATTRIB3FPROC)           getprocaddress("glVertexAttrib3f");
    glVertexAttrib3fv_ =          (PFNGLVERTEXATTRIB3FVPROC)          getprocaddress("glVertexAttrib3fv");
    glVertexAttrib3s_ =           (PFNGLVERTEXATTRIB3SPROC)           getprocaddress("glVertexAttrib3s");
    glVertexAttrib3sv_ =          (PFNGLVERTEXATTRIB3SVPROC)          getprocaddress("glVertexAttrib3sv");
    glVertexAttrib4f_ =           (PFNGLVERTEXATTRIB4FPROC)           getprocaddress("glVertexAttrib4f");
    glVertexAttrib4fv_ =          (PFNGLVERTEXATTRIB4FVPROC)          getprocaddress("glVertexAttrib4fv");
    glVertexAttrib4s_ =           (PFNGLVERTEXATTRIB4SPROC)           getprocaddress("glVertexAttrib4s");
    glVertexAttrib4sv_ =          (PFNGLVERTEXATTRIB4SVPROC)          getprocaddress("glVertexAttrib4sv");
    glVertexAttrib4bv_ =          (PFNGLVERTEXATTRIB4BVPROC)          getprocaddress("glVertexAttrib4bv");
    glVertexAttrib4iv_ =          (PFNGLVERTEXATTRIB4IVPROC)          getprocaddress("glVertexAttrib4iv");
    glVertexAttrib4ubv_ =         (PFNGLVERTEXATTRIB4UBVPROC)         getprocaddress("glVertexAttrib4ubv");
    glVertexAttrib4uiv_ =         (PFNGLVERTEXATTRIB4UIVPROC)         getprocaddress("glVertexAttrib4uiv");
    glVertexAttrib4usv_ =         (PFNGLVERTEXATTRIB4USVPROC)         getprocaddress("glVertexAttrib4usv");
    glVertexAttrib4Nbv_ =         (PFNGLVERTEXATTRIB4NBVPROC)         getprocaddress("glVertexAttrib4Nbv");
    glVertexAttrib4Niv_ =         (PFNGLVERTEXATTRIB4NIVPROC)         getprocaddress("glVertexAttrib4Niv");
    glVertexAttrib4Nub_ =         (PFNGLVERTEXATTRIB4NUBPROC)         getprocaddress("glVertexAttrib4Nub");
    glVertexAttrib4Nubv_ =        (PFNGLVERTEXATTRIB4NUBVPROC)        getprocaddress("glVertexAttrib4Nubv");
    glVertexAttrib4Nuiv_ =        (PFNGLVERTEXATTRIB4NUIVPROC)        getprocaddress("glVertexAttrib4Nuiv");
    glVertexAttrib4Nusv_ =        (PFNGLVERTEXATTRIB4NUSVPROC)        getprocaddress("glVertexAttrib4Nusv");
    glVertexAttribPointer_ =      (PFNGLVERTEXATTRIBPOINTERPROC)      getprocaddress("glVertexAttribPointer");

    glUniformMatrix2x3fv_ =       (PFNGLUNIFORMMATRIX2X3FVPROC)       getprocaddress("glUniformMatrix2x3fv");
    glUniformMatrix3x2fv_ =       (PFNGLUNIFORMMATRIX3X2FVPROC)       getprocaddress("glUniformMatrix3x2fv");
    glUniformMatrix2x4fv_ =       (PFNGLUNIFORMMATRIX2X4FVPROC)       getprocaddress("glUniformMatrix2x4fv");
    glUniformMatrix4x2fv_ =       (PFNGLUNIFORMMATRIX4X2FVPROC)       getprocaddress("glUniformMatrix4x2fv");
    glUniformMatrix3x4fv_ =       (PFNGLUNIFORMMATRIX3X4FVPROC)       getprocaddress("glUniformMatrix3x4fv");
    glUniformMatrix4x3fv_ =       (PFNGLUNIFORMMATRIX4X3FVPROC)       getprocaddress("glUniformMatrix4x3fv");

    glDrawBuffers_ =              (PFNGLDRAWBUFFERSPROC)              getprocaddress("glDrawBuffers");
#endif

    if(glversion >= 300)
    {
        glGetStringi_ =            (PFNGLGETSTRINGIPROC)          getprocaddress("glGetStringi");
        glBindFragDataLocation_ =  (PFNGLBINDFRAGDATALOCATIONPROC)getprocaddress("glBindFragDataLocation");
    }

    const char *glslstr = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    conoutf(CON_INIT, "GLSL: %s", glslstr ? glslstr : "unknown");

    uint glslmajorversion, glslminorversion;
    if(glslstr && sscanf(glslstr, " %u.%u", &glslmajorversion, &glslminorversion) == 2) glslversion = glslmajorversion*100 + glslminorversion;

    if(glslversion < 120) fatal("GLSL 1.20 or greater is required!");

    parseglexts();

    GLint texsize = 0, texunits = 0, vtexunits = 0, cubetexsize = 0, oqbits = 0, drawbufs = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texsize);
    hwtexsize = texsize;
    if(hwtexsize < 4096)
        fatal("Large texture support is required!");
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texunits);
    hwtexunits = texunits;
    if(hwtexunits < 16)
        fatal("Hardware does not support at least 16 texture units.");
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &vtexunits);
    hwvtexunits = vtexunits;
    if(hwvtexunits < 4)
        fatal("Hardware does not support at least 4 vertex texture units.");
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &cubetexsize);
    hwcubetexsize = cubetexsize;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &drawbufs);
    if(drawbufs < 4) fatal("Hardware does not support at least 4 draw buffers.");
    glGetQueryiv_(GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &oqbits);
    if(!oqbits)
    {
        conoutf(CON_WARN, "WARNING: No occlusion query support!");
        extern int vacubesize, oqfrags;
        vacubesize = 64;
        oqfrags = 0;
    }

    if(glversion >= 300 || hasext("GL_ARB_vertex_array_object"))
    {
        glBindVertexArray_ =    (PFNGLBINDVERTEXARRAYPROC)   getprocaddress("glBindVertexArray");
        glDeleteVertexArrays_ = (PFNGLDELETEVERTEXARRAYSPROC)getprocaddress("glDeleteVertexArrays");
        glGenVertexArrays_ =    (PFNGLGENVERTEXARRAYSPROC)   getprocaddress("glGenVertexArrays");
        glIsVertexArray_ =      (PFNGLISVERTEXARRAYPROC)     getprocaddress("glIsVertexArray");
        hasVAO = true;
        if(glversion < 300 || dbgexts) conoutf(CON_INIT, "Using GL_ARB_vertex_array_object extension.");
    }
    else if(hasext("GL_APPLE_vertex_array_object"))
    {
        glBindVertexArray_ =    (PFNGLBINDVERTEXARRAYPROC)   getprocaddress("glBindVertexArrayAPPLE");
        glDeleteVertexArrays_ = (PFNGLDELETEVERTEXARRAYSPROC)getprocaddress("glDeleteVertexArraysAPPLE");
        glGenVertexArrays_ =    (PFNGLGENVERTEXARRAYSPROC)   getprocaddress("glGenVertexArraysAPPLE");
        glIsVertexArray_ =      (PFNGLISVERTEXARRAYPROC)     getprocaddress("glIsVertexArrayAPPLE");
        hasVAO = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_APPLE_vertex_array_object extension.");
    }

    if(glversion >= 300)
    {
        hasTF = hasTRG = hasRGTC = hasPF = hasGPU4 = true;

        glClampColor_ = (PFNGLCLAMPCOLORPROC)getprocaddress("glClampColor");
        hasCBF = true;

        glColorMaski_ = (PFNGLCOLORMASKIPROC)getprocaddress("glColorMaski");
        glEnablei_ =    (PFNGLENABLEIPROC)   getprocaddress("glEnablei");
        glDisablei_ =   (PFNGLENABLEIPROC)   getprocaddress("glDisablei");
        hasDB2 = true;

        glBeginConditionalRender_ = (PFNGLBEGINCONDITIONALRENDERPROC)getprocaddress("glBeginConditionalRender");
        glEndConditionalRender_ =   (PFNGLENDCONDITIONALRENDERPROC)  getprocaddress("glEndConditionalRender");
        hasCR = true;
    }
    else
    {
        if(hasext("GL_ARB_texture_float"))
        {
            hasTF = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_float extension.");
        }
        if(hasext("GL_ARB_texture_rg"))
        {
            hasTRG = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_rg extension.");
        }
        if(hasext("GL_ARB_texture_compression_rgtc") || hasext("GL_EXT_texture_compression_rgtc"))
        {
            hasRGTC = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_compression_rgtc extension.");
        }
        if(hasext("GL_EXT_packed_float"))
        {
            hasPF = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_packed_float extension.");
        }
        if(hasext("GL_EXT_gpu_shader4"))
        {
            glBindFragDataLocation_ = (PFNGLBINDFRAGDATALOCATIONPROC)getprocaddress("glBindFragDataLocationEXT");
            hasGPU4 = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_gpu_shader4 extension.");
        }
        if(hasext("GL_ARB_color_buffer_float"))
        {
            glClampColor_ = (PFNGLCLAMPCOLORPROC)getprocaddress("glClampColorARB");
            hasCBF = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_color_buffer_float extension.");
        }
        if(hasext("GL_EXT_draw_buffers2"))
        {
            glColorMaski_ = (PFNGLCOLORMASKIPROC)getprocaddress("glColorMaskIndexedEXT");
            glEnablei_ =    (PFNGLENABLEIPROC)   getprocaddress("glEnableIndexedEXT");
            glDisablei_ =   (PFNGLENABLEIPROC)   getprocaddress("glDisableIndexedEXT");
            hasDB2 = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_draw_buffers2 extension.");
        }
        if(hasext("GL_NV_conditional_render"))
        {
            glBeginConditionalRender_ = (PFNGLBEGINCONDITIONALRENDERPROC)getprocaddress("glBeginConditionalRenderNV");
            glEndConditionalRender_ =   (PFNGLENDCONDITIONALRENDERPROC)  getprocaddress("glEndConditionalRenderNV");
            hasCR = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_NV_conditional_render extension.");
        }
    }

    if(glversion >= 300 || hasext("GL_ARB_framebuffer_object"))
    {
        glBindRenderbuffer_        = (PFNGLBINDRENDERBUFFERPROC)       getprocaddress("glBindRenderbuffer");
        glDeleteRenderbuffers_     = (PFNGLDELETERENDERBUFFERSPROC)    getprocaddress("glDeleteRenderbuffers");
        glGenRenderbuffers_        = (PFNGLGENFRAMEBUFFERSPROC)        getprocaddress("glGenRenderbuffers");
        glRenderbufferStorage_     = (PFNGLRENDERBUFFERSTORAGEPROC)    getprocaddress("glRenderbufferStorage");
        glCheckFramebufferStatus_  = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) getprocaddress("glCheckFramebufferStatus");
        glBindFramebuffer_         = (PFNGLBINDFRAMEBUFFERPROC)        getprocaddress("glBindFramebuffer");
        glDeleteFramebuffers_      = (PFNGLDELETEFRAMEBUFFERSPROC)     getprocaddress("glDeleteFramebuffers");
        glGenFramebuffers_         = (PFNGLGENFRAMEBUFFERSPROC)        getprocaddress("glGenFramebuffers");
        glFramebufferTexture1D_    = (PFNGLFRAMEBUFFERTEXTURE1DPROC)   getprocaddress("glFramebufferTexture1D");
        glFramebufferTexture2D_    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)   getprocaddress("glFramebufferTexture2D");
        glFramebufferTexture3D_    = (PFNGLFRAMEBUFFERTEXTURE3DPROC)   getprocaddress("glFramebufferTexture3D");
        glFramebufferRenderbuffer_ = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)getprocaddress("glFramebufferRenderbuffer");
        glGenerateMipmap_          = (PFNGLGENERATEMIPMAPPROC)         getprocaddress("glGenerateMipmap");
        glBlitFramebuffer_         = (PFNGLBLITFRAMEBUFFERPROC)        getprocaddress("glBlitFramebuffer");
        glRenderbufferStorageMultisample_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)getprocaddress("glRenderbufferStorageMultisample");

        hasAFBO = hasFBO = hasFBB = hasFBMS = hasDS = true;
        if(glversion < 300 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_framebuffer_object extension.");
    }
    else if(hasext("GL_EXT_framebuffer_object"))
    {
        glBindRenderbuffer_        = (PFNGLBINDRENDERBUFFERPROC)       getprocaddress("glBindRenderbufferEXT");
        glDeleteRenderbuffers_     = (PFNGLDELETERENDERBUFFERSPROC)    getprocaddress("glDeleteRenderbuffersEXT");
        glGenRenderbuffers_        = (PFNGLGENFRAMEBUFFERSPROC)        getprocaddress("glGenRenderbuffersEXT");
        glRenderbufferStorage_     = (PFNGLRENDERBUFFERSTORAGEPROC)    getprocaddress("glRenderbufferStorageEXT");
        glCheckFramebufferStatus_  = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) getprocaddress("glCheckFramebufferStatusEXT");
        glBindFramebuffer_         = (PFNGLBINDFRAMEBUFFERPROC)        getprocaddress("glBindFramebufferEXT");
        glDeleteFramebuffers_      = (PFNGLDELETEFRAMEBUFFERSPROC)     getprocaddress("glDeleteFramebuffersEXT");
        glGenFramebuffers_         = (PFNGLGENFRAMEBUFFERSPROC)        getprocaddress("glGenFramebuffersEXT");
        glFramebufferTexture1D_    = (PFNGLFRAMEBUFFERTEXTURE1DPROC)   getprocaddress("glFramebufferTexture1DEXT");
        glFramebufferTexture2D_    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)   getprocaddress("glFramebufferTexture2DEXT");
        glFramebufferTexture3D_    = (PFNGLFRAMEBUFFERTEXTURE3DPROC)   getprocaddress("glFramebufferTexture3DEXT");
        glFramebufferRenderbuffer_ = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)getprocaddress("glFramebufferRenderbufferEXT");
        glGenerateMipmap_          = (PFNGLGENERATEMIPMAPPROC)         getprocaddress("glGenerateMipmapEXT");
        hasFBO = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_framebuffer_object extension.");

        if(hasext("GL_EXT_framebuffer_blit"))
        {
            glBlitFramebuffer_     = (PFNGLBLITFRAMEBUFFERPROC)        getprocaddress("glBlitFramebufferEXT");
            hasFBB = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_framebuffer_blit extension.");
        }
        if(hasext("GL_EXT_framebuffer_multisample"))
        {
            glRenderbufferStorageMultisample_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)getprocaddress("glRenderbufferStorageMultisampleEXT");
            hasFBMS = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_framebuffer_multisample extension.");
        }
        if(hasext("GL_EXT_packed_depth_stencil") || hasext("GL_NV_packed_depth_stencil"))
        {
            hasDS = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_packed_depth_stencil extension.");
        }
    }
    else fatal("Framebuffer object support is required!");

    if(glversion >= 300 || hasext("GL_ARB_map_buffer_range"))
    {
        glMapBufferRange_         = (PFNGLMAPBUFFERRANGEPROC)        getprocaddress("glMapBufferRange");
        glFlushMappedBufferRange_ = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)getprocaddress("glFlushMappedBufferRange");
        hasMBR = true;
        if(glversion < 300 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_map_buffer_range.");
    }

    if(glversion >= 310 || hasext("GL_ARB_uniform_buffer_object"))
    {
        glGetUniformIndices_       = (PFNGLGETUNIFORMINDICESPROC)      getprocaddress("glGetUniformIndices");
        glGetActiveUniformsiv_     = (PFNGLGETACTIVEUNIFORMSIVPROC)    getprocaddress("glGetActiveUniformsiv");
        glGetUniformBlockIndex_    = (PFNGLGETUNIFORMBLOCKINDEXPROC)   getprocaddress("glGetUniformBlockIndex");
        glGetActiveUniformBlockiv_ = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)getprocaddress("glGetActiveUniformBlockiv");
        glUniformBlockBinding_     = (PFNGLUNIFORMBLOCKBINDINGPROC)    getprocaddress("glUniformBlockBinding");
        glBindBufferBase_          = (PFNGLBINDBUFFERBASEPROC)         getprocaddress("glBindBufferBase");
        glBindBufferRange_         = (PFNGLBINDBUFFERRANGEPROC)        getprocaddress("glBindBufferRange");

        useubo = 1;
        hasUBO = true;
        if(glversion < 310 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_uniform_buffer_object extension.");
    }

    if(glversion >= 310 || hasext("GL_ARB_texture_rectangle"))
    {
        hasTR = true;
        if(glversion < 310 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_rectangle extension.");
    }
    else fatal("Texture rectangle support is required!");

    if(glversion >= 320 || hasext("GL_ARB_texture_multisample"))
    {
        glTexImage2DMultisample_ = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)getprocaddress("glTexImage2DMultisample");
        glTexImage3DMultisample_ = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)getprocaddress("glTexImage3DMultisample");
        glGetMultisamplefv_      = (PFNGLGETMULTISAMPLEFVPROC)     getprocaddress("glGetMultisamplefv");
        glSampleMaski_           = (PFNGLSAMPLEMASKIPROC)          getprocaddress("glSampleMaski");
        hasTMS = true;
        if(glversion < 320 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_multisample extension.");
    }
    if(hasext("GL_EXT_framebuffer_multisample_blit_scaled"))
    {
        hasFBMSBS = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_framebuffer_multisample_blit_scaled extension.");
    }
    if(hasext("GL_NV_framebuffer_multisample_coverage"))
    {
        glRenderbufferStorageMultisampleCoverageNV_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLECOVERAGENVPROC)getprocaddress("glRenderbufferStorageMultisampleCoverageNV");
        hasNVFBMSC = true;
    }
    if(hasext("GL_NV_texture_multisample"))
    {
        glTexImage2DMultisampleCoverageNV_     = (PFNGLTEXIMAGE2DMULTISAMPLECOVERAGENVPROC)    getprocaddress("glTexImage2DMultisampleCoverageNV");
        glTexImage3DMultisampleCoverageNV_     = (PFNGLTEXIMAGE3DMULTISAMPLECOVERAGENVPROC)    getprocaddress("glTexImage3DMultisampleCoverageNV");
        glTextureImage2DMultisampleNV_         = (PFNGLTEXTUREIMAGE2DMULTISAMPLENVPROC)        getprocaddress("glTextureImage2DMultisampleNV");
        glTextureImage3DMultisampleNV_         = (PFNGLTEXTUREIMAGE3DMULTISAMPLENVPROC)        getprocaddress("glTextureImage3DMultisampleNV");
        glTextureImage2DMultisampleCoverageNV_ = (PFNGLTEXTUREIMAGE2DMULTISAMPLECOVERAGENVPROC)getprocaddress("glTextureImage2DMultisampleCoverageNV");
        glTextureImage3DMultisampleCoverageNV_ = (PFNGLTEXTUREIMAGE3DMULTISAMPLECOVERAGENVPROC)getprocaddress("glTextureImage3DMultisampleCoverageNV");
        hasNVTMS = true;
    }

    if(hasext("GL_EXT_timer_query") || hasext("GL_ARB_timer_query"))
    {
        if(hasext("GL_EXT_timer_query"))
        {
            glGetQueryObjecti64v_ =  (PFNGLGETQUERYOBJECTI64VEXTPROC)  getprocaddress("glGetQueryObjecti64vEXT");
            glGetQueryObjectui64v_ = (PFNGLGETQUERYOBJECTUI64VEXTPROC) getprocaddress("glGetQueryObjectui64vEXT");
            if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_timer_query extension.");
        }
        else
        {
            glGetQueryObjecti64v_ =  (PFNGLGETQUERYOBJECTI64VEXTPROC)  getprocaddress("glGetQueryObjecti64v");
            glGetQueryObjectui64v_ = (PFNGLGETQUERYOBJECTUI64VEXTPROC) getprocaddress("glGetQueryObjectui64v");
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_timer_query extension.");
        }
        hasTQ = true;
    }

    if(hasext("GL_EXT_texture_compression_s3tc"))
    {
        hasS3TC = true;
#ifdef __APPLE__
        usetexcompress = 1;
#else
        if(!mesa) usetexcompress = 2;
#endif
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_texture_compression_s3tc extension.");
    }
    else if(hasext("GL_EXT_texture_compression_dxt1") && hasext("GL_ANGLE_texture_compression_dxt3") && hasext("GL_ANGLE_texture_compression_dxt5"))
    {
        hasS3TC = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_texture_compression_dxt1 extension.");
    }
    if(hasext("GL_3DFX_texture_compression_FXT1"))
    {
        hasFXT1 = true;
        if(mesa) usetexcompress = max(usetexcompress, 1);
        if(dbgexts) conoutf(CON_INIT, "Using GL_3DFX_texture_compression_FXT1.");
    }
    if(hasext("GL_EXT_texture_compression_latc"))
    {
        hasLATC = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_texture_compression_latc extension.");
    }

    if(hasext("GL_EXT_texture_filter_anisotropic"))
    {
       GLint val = 0;
       glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &val);
       hwmaxaniso = val;
       hasAF = true;
       if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_texture_filter_anisotropic extension.");
    }

    if(hasext("GL_EXT_depth_bounds_test"))
    {
        glDepthBounds_ = (PFNGLDEPTHBOUNDSEXTPROC) getprocaddress("glDepthBoundsEXT");
        hasDBT = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_depth_bounds_test extension.");
    }

    if(glversion >= 320 || hasext("GL_ARB_depth_clamp"))
    {
        hasDC = true;
        if(glversion < 320 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_depth_clamp extension.");
    }
    else if(hasext("GL_NV_depth_clamp"))
    {
        hasDC = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_NV_depth_clamp extension.");
    }

    if(glversion >= 330)
    {
        hasTSW = hasEAL = hasOQ2 = true;
    }
    else
    {
        if(hasext("GL_ARB_texture_swizzle") || hasext("GL_EXT_texture_swizzle"))
        {
            hasTSW = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_swizzle extension.");
        }
        if(hasext("GL_ARB_explicit_attrib_location"))
        {
            hasEAL = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_explicit_attrib_location extension.");
        }
        if(hasext("GL_ARB_occlusion_query2"))
        {
            hasOQ2 = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_occlusion_query2 extension.");
        }
    }

    if(glversion >= 400)
    {
        hasTG = hasGPU5 = true;

        glMinSampleShading_ = (PFNGLMINSAMPLESHADINGPROC)getprocaddress("glMinSampleShading");
        hasMSS = true;

        glBlendEquationi_ =         (PFNGLBLENDEQUATIONIPROC)        getprocaddress("glBlendEquationi");
        glBlendEquationSeparatei_ = (PFNGLBLENDEQUATIONSEPARATEIPROC)getprocaddress("glBlendEquationSeparatei");
        glBlendFunci_ =             (PFNGLBLENDFUNCIPROC)            getprocaddress("glBlendFunci");
        glBlendFuncSeparatei_ =     (PFNGLBLENDFUNCSEPARATEIPROC)    getprocaddress("glBlendFuncSeparatei");
        hasDBB = true;
    }
    else
    {
        if(hasext("GL_ARB_texture_gather"))
        {
            hasTG = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_gather extension.");
        }
        else if(hasext("GL_AMD_texture_texture4"))
        {
            hasT4 = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_AMD_texture_texture4 extension.");
        }
        if(hasext("GL_ARB_gpu_shader5"))
        {
            hasGPU5 = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_gpu_shader5 extension.");
        }
        if(hasext("GL_ARB_sample_shading"))
        {
            glMinSampleShading_ = (PFNGLMINSAMPLESHADINGPROC)getprocaddress("glMinSampleShadingARB");
            hasMSS = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_sample_shading extension.");
        }
        if(hasext("GL_ARB_draw_buffers_blend"))
        {
            glBlendEquationi_ =         (PFNGLBLENDEQUATIONIPROC)        getprocaddress("glBlendEquationiARB");
            glBlendEquationSeparatei_ = (PFNGLBLENDEQUATIONSEPARATEIPROC)getprocaddress("glBlendEquationSeparateiARB");
            glBlendFunci_ =             (PFNGLBLENDFUNCIPROC)            getprocaddress("glBlendFunciARB");
            glBlendFuncSeparatei_ =     (PFNGLBLENDFUNCSEPARATEIPROC)    getprocaddress("glBlendFuncSeparateiARB");
            hasDBB = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_draw_buffers_blend extension.");
        }
    }
    if(hasTG || hasT4) usetexgather = 1;
    if(hasTG && hasGPU5 && !intel) usetexgather = 2;

    if(hasext("GL_ARB_debug_output"))
    {
        glDebugMessageControl_ =  (PFNGLDEBUGMESSAGECONTROLARBPROC) getprocaddress("glDebugMessageControlARB");
        glDebugMessageInsert_ =   (PFNGLDEBUGMESSAGEINSERTARBPROC)  getprocaddress("glDebugMessageInsertARB");
        glDebugMessageCallback_ = (PFNGLDEBUGMESSAGECALLBACKARBPROC)getprocaddress("glDebugMessageCallbackARB");
        glGetDebugMessageLog_ =   (PFNGLGETDEBUGMESSAGELOGARBPROC)  getprocaddress("glGetDebugMessageLogARB");

        hasDBGO = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_ARB_debug_output extension.");
    }

    extern int msaadepthstencil, gdepthstencil, glineardepth, msaalineardepth, lighttilebatch, batchsunlight, smgather;
    if(ati)
    {
        msaalineardepth = glineardepth = 1; // reading back from depth-stencil still buggy on newer cards, and requires stencil for MSAA
        msaadepthstencil = gdepthstencil = 1; // some older ATI GPUs do not support reading from depth-stencil textures, so only use depth-stencil renderbuffer for now
        if(checkseries(renderer, "Radeon HD", 4000, 5199)) ati_pf_bug = 1;
    }
    else if(nvidia)
    {
    }
    else if(intel)
    {
        glineardepth = 1; // causes massive slowdown in windows driver (and sometimes in linux driver) if not using linear depth
        lighttilebatch = 4;
        if(mesa) batchsunlight = 0; // causes massive slowdown in linux driver
        smgather = 1; // native shadow filter is slow
    }
}

ICOMMAND(glext, "s", (char *ext), intret(hasext(ext) ? 1 : 0));

struct timer
{
    enum { MAXQUERY = 4 };

    const char *name;
    bool gpu;
    GLuint query[MAXQUERY];
    int waiting;
    uint starttime;
    float result, print;
};
static vector<timer> timers;
static vector<int> timerorder;
static int timercycle = 0;

extern int usetimers;

timer *findtimer(const char *name, bool gpu)
{
    loopv(timers) if(!strcmp(timers[i].name, name) && timers[i].gpu == gpu)
    {
        timerorder.removeobj(i);
        timerorder.add(i);
        return &timers[i];
    }
    timerorder.add(timers.length());
    timer &t = timers.add();
    t.name = name;
    t.gpu = gpu;
    memset(t.query, 0, sizeof(t.query));
    if(gpu) glGenQueries_(timer::MAXQUERY, t.query);
    t.waiting = 0;
    t.starttime = 0;
    t.result = -1;
    t.print = -1;
    return &t;
}

timer *begintimer(const char *name, bool gpu)
{
    if(!usetimers || viewidx || inbetweenframes || (gpu && !hasTQ)) return NULL;
    timer *t = findtimer(name, gpu);
    if(t->gpu)
    {
        deferquery++;
        glBeginQuery_(GL_TIME_ELAPSED_EXT, t->query[timercycle]);
        t->waiting |= 1<<timercycle;
    }
    else t->starttime = getclockmillis();
    return t;
}

void endtimer(timer *t)
{
    if(!t) return;
    if(t->gpu)
    {
        glEndQuery_(GL_TIME_ELAPSED_EXT);
        deferquery--;
    }
    else t->result = max(float(getclockmillis() - t->starttime), 0.0f);
}

void synctimers()
{
    timercycle = (timercycle + 1) % timer::MAXQUERY;

    loopv(timers)
    {
        timer &t = timers[i];
        if(t.waiting&(1<<timercycle))
        {
            GLint available = 0;
            while(!available)
                glGetQueryObjectiv_(t.query[timercycle], GL_QUERY_RESULT_AVAILABLE, &available);
            GLuint64EXT result = 0;
            glGetQueryObjectui64v_(t.query[timercycle], GL_QUERY_RESULT, &result);
            t.result = max(float(result) * 1e-6f, 0.0f);
            t.waiting &= ~(1<<timercycle);
        }
        else t.result = -1;
    }
}

void cleanuptimers()
{
    loopv(timers)
    {
        timer &t = timers[i];
        if(t.gpu) glDeleteQueries_(timer::MAXQUERY, t.query);
    }
    timers.shrink(0);
    timerorder.shrink(0);
}

VARFN(timer, usetimers, 0, 0, 1, cleanuptimers());
VAR(frametimer, 0, 0, 1);
int framemillis = 0; // frame time (ie does not take into account the swap)

void printtimers(int conw, int conh)
{
    if(!frametimer && !usetimers) return;

    static int lastprint = 0;
    int offset = 0;
    if(frametimer)
    {
        static int printmillis = 0;
        if(totalmillis - lastprint >= 200) printmillis = framemillis;
        draw_textf("frame time %i ms", conw-20*FONTH, conh-FONTH*3/2-offset*9*FONTH/8, printmillis);
        offset++;
    }
    if(usetimers) loopv(timerorder)
    {
        timer &t = timers[timerorder[i]];
        if(t.print < 0 ? t.result >= 0 : totalmillis - lastprint >= 200) t.print = t.result;
        if(t.print < 0 || (t.gpu && !(t.waiting&(1<<timercycle)))) continue;
        draw_textf("%s%s %5.2f ms", conw-20*FONTH, conh-FONTH*3/2-offset*9*FONTH/8, t.name, t.gpu ? "" : " (cpu)", t.print);
        offset++;
    }
    if(totalmillis - lastprint >= 200) lastprint = totalmillis;
}

void gl_resize()
{
    gl_setupframe();
    glViewport(0, 0, hudw, hudh);
}

void gl_init()
{
    GLERROR;

    glClearColor(0, 0, 0, 0);
    glClearDepth(1);
    glClearStencil(0);
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glEnable(GL_LINE_SMOOTH);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);

    renderpath = R_GLSLANG;

    gle::setup();
    setupshaders();
    setuptexcompress();

    GLERROR;

    gl_resize();

    static const char * const rpnames[1] = { "GLSL shader" };
    conoutf(CON_INIT, "Rendering using the OpenGL %s path.", rpnames[renderpath]);
}

VAR(wireframe, 0, 0, 1);

ICOMMAND(getcamyaw, "", (), floatret(camera1->yaw));
ICOMMAND(getcampitch, "", (), floatret(camera1->pitch));
ICOMMAND(getcamroll, "", (), floatret(ovr::modifyroll(camera1->roll)));
ICOMMAND(getcampos, "", (),
{
    defformatstring(pos, "%s %s %s", floatstr(camera1->o.x), floatstr(camera1->o.y), floatstr(camera1->o.z));
    result(pos);
});

vec worldpos, camdir, camright, camup;

void setcammatrix()
{
    // move from RH to Z-up LH quake style worldspace
    cammatrix = viewmatrix;
    cammatrix.rotate_around_y((!drawtex ? ovr::modifyroll(camera1->roll) : camera1->roll)*RAD);
    cammatrix.rotate_around_x(camera1->pitch*-RAD);
    cammatrix.rotate_around_z(camera1->yaw*-RAD);
    cammatrix.translate(vec(camera1->o).neg());

    if(!drawtex && ovr::enabled)
        cammatrix.jitter((viewidx ? -1 : 1) * ovr::viewoffset, 0);

    cammatrix.transposedtransformnormal(vec(viewmatrix.b), camdir);
    cammatrix.transposedtransformnormal(vec(viewmatrix.a).neg(), camright);
    cammatrix.transposedtransformnormal(vec(viewmatrix.c), camup);

    camprojmatrix.mul(projmatrix, cammatrix);
    invcammatrix.invert(cammatrix);
    invprojmatrix.invert(projmatrix);
    invcamprojmatrix.invert(camprojmatrix);

    if(!drawtex)
    {
        float x, y;
        vec dir1, dir2;
        UI::getcursorpos(x, y);
        dir1 = invcamprojmatrix.perspectivetransform(vec(x*2-1, 1-2*y, 2-1));
        dir2 = invcamprojmatrix.perspectivetransform(vec(x*2-1, 1-2*y, -1));

        dir1.sub(dir2).normalize();
        if(raycubepos(camera1->o, dir1, worldpos, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
            worldpos = vec(camdir).mul(2 * worldsize).add(camera1->o);
    }
}

void setcamprojmatrix(bool init = true, bool flush = false)
{
    if(init)
    {
        setcammatrix();
        if(ovr::enabled && !drawtex)
            projmatrix.jitter((viewidx ? -1 : 1) * ovr::distortoffset, 0);
    }
    else camprojmatrix.mul(projmatrix, cammatrix);

    jitteraa(init);

    GLOBALPARAM(camprojmatrix, camprojmatrix);
    GLOBALPARAM(lineardepthscale, projmatrix.lineardepthscale()); //(invprojmatrix.c.z, invprojmatrix.d.z));

    if(flush && Shader::lastshader) Shader::lastshader->flushparams();
}

glmatrix hudmatrix, hudmatrixstack[64];
int hudmatrixpos = 0;

void resethudmatrix()
{
    hudmatrixpos = 0;
    if(ovr::enabled) ovr::ortho(hudmatrix);
    GLOBALPARAM(hudmatrix, hudmatrix);
}

void pushhudmatrix()
{
    if(hudmatrixpos >= 0 && hudmatrixpos < int(sizeof(hudmatrixstack)/sizeof(hudmatrixstack[0]))) hudmatrixstack[hudmatrixpos] = hudmatrix;
    ++hudmatrixpos;
}

void flushhudmatrix(bool flushparams)
{
    GLOBALPARAM(hudmatrix, hudmatrix);
    if(flushparams && Shader::lastshader) Shader::lastshader->flushparams();
}

void pophudmatrix(bool flush, bool flushparams)
{
    --hudmatrixpos;
    if(hudmatrixpos >= 0 && hudmatrixpos < int(sizeof(hudmatrixstack)/sizeof(hudmatrixstack[0])))
    {
        hudmatrix = hudmatrixstack[hudmatrixpos];
        if(flush) flushhudmatrix(flushparams);
    }
}

int vieww = -1, viewh = -1, viewidx = 0;
float curfov = 100, curavatarfov = 65, fovy, aspect;
int farplane;
VARP(zoominvel, 0, 250, 5000);
VARP(zoomoutvel, 0, 100, 5000);
VARP(zoomfov, 10, 35, 60);
VARP(fov, 10, 100, 150);
VAR(avatarzoomfov, 10, 25, 60);
VAR(avatarfov, 10, 65, 150);
FVAR(avatardepth, 0, 0.5f, 1);
FVARNP(aspect, forceaspect, 0, 0, 1e3f);

static int zoommillis = 0;
VARF(zoom, -1, 0, 1,
    if(zoom) zoommillis = totalmillis;
);

void disablezoom()
{
    zoom = 0;
    zoommillis = totalmillis;
}

void computezoom()
{
    if(!zoom) { curfov = fov; curavatarfov = avatarfov; return; }
    if(zoom < 0 && curfov >= fov) { zoom = 0; curfov = fov; curavatarfov = avatarfov; return; } // don't zoom-out if not zoomed-in
    int zoomvel = zoom > 0 ? zoominvel : zoomoutvel,
        oldfov = zoom > 0 ? fov : zoomfov,
        newfov = zoom > 0 ? zoomfov : fov,
        oldavatarfov = zoom > 0 ? avatarfov : avatarzoomfov,
        newavatarfov = zoom > 0 ? avatarzoomfov : avatarfov;
    float t = zoomvel ? float(zoomvel - (totalmillis - zoommillis)) / zoomvel : 0;
    if(t <= 0)
    {
        if(!zoomvel && fabs(newfov - curfov) >= 1)
        {
            curfov = newfov;
            curavatarfov = newavatarfov;
        }
        zoom = max(zoom, 0);
    }
    else
    {
        curfov = oldfov*t + newfov*(1 - t);
        curavatarfov = oldavatarfov*t + newavatarfov*(1 - t);
    }
}

FVARP(zoomsens, 1e-4f, 3, 1e4f);
FVARP(zoomaccel, 0, 0, 1000);
VARP(zoomautosens, 0, 1, 1);
FVARP(sensitivity, 1e-4f, 10, 1e4f);
FVARP(sensitivityscale, 1e-4f, 100, 1e4f);
VARP(invmouse, 0, 0, 1);
FVARP(mouseaccel, 0, 0, 1000);

VAR(thirdperson, 0, 0, 2);
FVAR(thirdpersondistance, 0, 20, 1000);
FVAR(thirdpersonup, -1000, 0, 1000);
FVAR(thirdpersonside, -1000, 0, 1000);
physent *camera1 = NULL;
bool detachedcamera = false;
bool isthirdperson() { return player!=camera1 || detachedcamera; }

void fixcamerarange()
{
    const float MAXPITCH = 90.0f;
    if(camera1->pitch>MAXPITCH) camera1->pitch = MAXPITCH;
    if(camera1->pitch<-MAXPITCH) camera1->pitch = -MAXPITCH;
    while(camera1->yaw<0.0f) camera1->yaw += 360.0f;
    while(camera1->yaw>=360.0f) camera1->yaw -= 360.0f;
}

void modifyorient(float yaw, float pitch)
{
    camera1->yaw += yaw;
    camera1->pitch += pitch;
    fixcamerarange();
    if(camera1!=player && !detachedcamera)
    {
        player->yaw = camera1->yaw;
        player->pitch = camera1->pitch;
    }
}

void mousemove(int dx, int dy)
{
    float cursens = sensitivity, curaccel = mouseaccel;

    if(zoom)
    {
        if(zoomautosens)
        {
            cursens = float(sensitivity*zoomfov)/fov;
            curaccel = float(mouseaccel*zoomfov)/fov;
        }
        else
        {
            cursens = zoomsens;
            curaccel = zoomaccel;
        }
    }
    if(curaccel && curtime && (dx || dy)) cursens += curaccel * sqrtf(dx*dx + dy*dy)/curtime;
    cursens /= sensitivityscale;

    if(game::mousemove(dx, dy, cursens))
    {
        fixcamerarange();
        return; //game processed mouse movement, don't bother
    }

    modifyorient(dx*cursens, dy*cursens*(invmouse ? 1 : -1));
}

void recomputecamera()
{
    game::setupcamera();
    computezoom();
    static physent tempcamera;

    if(!game::recomputecamera(camera1, tempcamera, detachedcamera, thirdpersondistance))
    {
        bool shoulddetach = thirdperson > 1 || game::detachcamera();
        if(!thirdperson && !shoulddetach)
        {
            camera1 = player;
            detachedcamera = false;
        }
        else
        {
            camera1 = &tempcamera;
            if(detachedcamera && shoulddetach) camera1->o = player->o;
            else
            {
                *camera1 = *player;
                detachedcamera = shoulddetach;
            }
            camera1->reset();
            camera1->type = ENT_CAMERA;
            camera1->move = -1;
            camera1->eyeheight = camera1->aboveeye = camera1->radius = camera1->xradius = camera1->yradius = 2;

            matrix3x3 orient;
            orient.identity();
            orient.rotate_around_y(ovr::modifyroll(camera1->roll)*RAD);
            orient.rotate_around_x(camera1->pitch*-RAD);
            orient.rotate_around_z(camera1->yaw*-RAD);
            vec dir = vec(orient.b).neg(), side = vec(orient.a).neg(), up = orient.c;

            if(game::collidecamera())
            {
                movecamera(camera1, dir, thirdpersondistance, 1);
                movecamera(camera1, dir, clamp(thirdpersondistance - camera1->o.dist(player->o), 0.0f, 1.0f), 0.1f);
                if(thirdpersonup)
                {
                    vec pos = camera1->o;
                    float dist = fabs(thirdpersonup);
                    if(thirdpersonup < 0) up.neg();
                    movecamera(camera1, up, dist, 1);
                    movecamera(camera1, up, clamp(dist - camera1->o.dist(pos), 0.0f, 1.0f), 0.1f);
                }
                if(thirdpersonside)
                {
                    vec pos = camera1->o;
                    float dist = fabs(thirdpersonside);
                    if(thirdpersonside < 0) side.neg();
                    movecamera(camera1, side, dist, 1);
                    movecamera(camera1, side, clamp(dist - camera1->o.dist(pos), 0.0f, 1.0f), 0.1f);
                }
            }
            else
            {
                camera1->o.add(vec(dir).mul(thirdpersondistance));
                if(thirdpersonup) camera1->o.add(vec(up).mul(thirdpersonup));
                if(thirdpersonside) camera1->o.add(vec(side).mul(thirdpersonside));
            }
        }
    }

    setviewcell(camera1->o);
}

float calcfrustumboundsphere(float nearplane, float farplane,  const vec &pos, const vec &view, vec &center)
{
    if(drawtex == DRAWTEX_MINIMAP)
    {
        center = minimapcenter;
        return minimapradius.magnitude();
    }

    float width = tan(fov/2.0f*RAD), height = width / aspect,
          cdist = ((nearplane + farplane)/2)*(1 + width*width + height*height);
    if(cdist <= farplane)
    {
        center = vec(view).mul(cdist).add(pos);
        return vec(width*nearplane, height*nearplane, cdist-nearplane).magnitude();
    }
    else
    {
        center = vec(view).mul(farplane).add(pos);
        return vec(width*farplane, height*farplane, 0).magnitude();
    }
}

extern const glmatrix viewmatrix(vec(-1, 0, 0), vec(0, 0, 1), vec(0, -1, 0));
extern const glmatrix invviewmatrix(vec(-1, 0, 0), vec(0, 0, -1), vec(0, 1, 0));
glmatrix cammatrix, projmatrix, camprojmatrix, invcammatrix, invcamprojmatrix, invprojmatrix;

FVAR(nearplane, 0.01f, 0.54f, 2.0f);

vec calcavatarpos(const vec &pos, float dist)
{
    vec eyepos;
    cammatrix.transform(pos, eyepos);
    GLdouble ydist = nearplane * tan(curavatarfov/2*RAD), xdist = ydist * aspect;
    vec4 scrpos;
    scrpos.x = eyepos.x*nearplane/xdist;
    scrpos.y = eyepos.y*nearplane/ydist;
    scrpos.z = (eyepos.z*(farplane + nearplane) - 2*nearplane*farplane) / (farplane - nearplane);
    scrpos.w = -eyepos.z;

    vec worldpos = invcamprojmatrix.perspectivetransform(scrpos);
    vec dir = vec(worldpos).sub(camera1->o).rescale(dist);
    return dir.add(camera1->o);
}

void renderavatar()
{
    if(!isthirdperson())
    {
        glmatrix oldprojmatrix = projmatrix;
        float avatarfovy = curavatarfov;
        if(ovr::enabled && ovr::fov) avatarfovy *= ovr::fov/fov;
        projmatrix.perspective(avatarfovy, aspect, nearplane, farplane);
        projmatrix.scalez(avatardepth);
        setcamprojmatrix(false);
        game::renderavatar();
        projmatrix = oldprojmatrix;
        setcamprojmatrix(false);
    }
}

FVAR(polygonoffsetfactor, -1e4f, -3.0f, 1e4f);
FVAR(polygonoffsetunits, -1e4f, -3.0f, 1e4f);
FVAR(depthoffset, -1e4f, 0.01f, 1e4f);

glmatrix nooffsetmatrix;

void enablepolygonoffset(GLenum type)
{
    if(!depthoffset)
    {
        glPolygonOffset(polygonoffsetfactor, polygonoffsetunits);
        glEnable(type);
        return;
    }

    projmatrix = nojittermatrix;
    nooffsetmatrix = projmatrix;
    projmatrix.d.z += depthoffset * projmatrix.c.z;
    setcamprojmatrix(false, true);
}

void disablepolygonoffset(GLenum type)
{
    if(!depthoffset)
    {
        glDisable(type);
        return;
    }

    projmatrix = nooffsetmatrix;
    setcamprojmatrix(false, true);
}

bool calcspherescissor(const vec &center, float size, float &sx1, float &sy1, float &sx2, float &sy2, float &sz1, float &sz2)
{
    vec e;
    cammatrix.transform(center, e);
    if(e.z > 2*size) { sx1 = sy1 = sz1 = 1; sx2 = sy2 = sz2 = -1; return false; }
    if(drawtex == DRAWTEX_MINIMAP)
    {
        vec dir(size, size, size);
        if(projmatrix.a.x < 0) dir.x = -dir.x;
        if(projmatrix.b.y < 0) dir.y = -dir.y;
        if(projmatrix.c.z < 0) dir.z = -dir.z;
        sx1 = max(projmatrix.a.x*(e.x - dir.x) + projmatrix.d.x, -1.0f);
        sx2 = min(projmatrix.a.x*(e.x + dir.x) + projmatrix.d.x, 1.0f);
        sy1 = max(projmatrix.b.y*(e.y - dir.y) + projmatrix.d.y, -1.0f);
        sy2 = min(projmatrix.b.y*(e.y + dir.y) + projmatrix.d.y, 1.0f);
        sz1 = max(projmatrix.c.z*(e.z - dir.z) + projmatrix.d.z, -1.0f);
        sz2 = min(projmatrix.c.z*(e.z + dir.z) + projmatrix.d.z, 1.0f);
        return sx1 < sx2 && sy1 < sy2 && sz1 < sz2;
    }
    float zzrr = e.z*e.z - size*size,
          dx = e.x*e.x + zzrr, dy = e.y*e.y + zzrr,
          focaldist = 1.0f/tan(fovy*0.5f*RAD);
    sx1 = sy1 = -1;
    sx2 = sy2 = 1;
    #define CHECKPLANE(c, dir, focaldist, low, high) \
    do { \
        float nzc = (cz*cz + 1) / (cz dir drt) - cz, \
              pz = (d##c)/(nzc*e.c - e.z); \
        if(pz > 0) \
        { \
            float c = (focaldist)*nzc, \
                  pc = pz*nzc; \
            if(pc < e.c) low = c; \
            else if(pc > e.c) high = c; \
        } \
    } while(0)
    if(dx > 0)
    {
        float cz = e.x/e.z, drt = sqrtf(dx)/size;
        CHECKPLANE(x, -, focaldist/aspect, sx1, sx2);
        CHECKPLANE(x, +, focaldist/aspect, sx1, sx2);
        if(ovr::enabled)
        {
            float offset = (viewidx ? -1 : 1) * ovr::distortoffset;
            if(sx1 > -1) sx1 += offset;
            if(sx2 < 1) sx2 += offset;
        }
    }
    if(dy > 0)
    {
        float cz = e.y/e.z, drt = sqrtf(dy)/size;
        CHECKPLANE(y, -, focaldist, sy1, sy2);
        CHECKPLANE(y, +, focaldist, sy1, sy2);
    }
    float z1 = min(e.z + size, -1e-3f - nearplane), z2 = min(e.z - size, -1e-3f - nearplane);
    sz1 = (z1*projmatrix.c.z + projmatrix.d.z) / (z1*projmatrix.c.w + projmatrix.d.w);
    sz2 = (z2*projmatrix.c.z + projmatrix.d.z) / (z2*projmatrix.c.w + projmatrix.d.w);
    return sx1 < sx2 && sy1 < sy2 && sz1 < sz2;
}

bool calcbbscissor(const ivec &bbmin, const ivec &bbmax, float &sx1, float &sy1, float &sx2, float &sy2)
{
#define ADDXYSCISSOR(p) do { \
        if(p.z >= -p.w) \
        { \
            float x = p.x / p.w, y = p.y / p.w; \
            sx1 = min(sx1, x); \
            sy1 = min(sy1, y); \
            sx2 = max(sx2, x); \
            sy2 = max(sy2, y); \
        } \
    } while(0)
    vec4 v[8];
    sx1 = sy1 = 1;
    sx2 = sy2 = -1;
    camprojmatrix.transform(vec(bbmin.x, bbmin.y, bbmin.z), v[0]);
    ADDXYSCISSOR(v[0]);
    camprojmatrix.transform(vec(bbmax.x, bbmin.y, bbmin.z), v[1]);
    ADDXYSCISSOR(v[1]);
    camprojmatrix.transform(vec(bbmin.x, bbmax.y, bbmin.z), v[2]);
    ADDXYSCISSOR(v[2]);
    camprojmatrix.transform(vec(bbmax.x, bbmax.y, bbmin.z), v[3]);
    ADDXYSCISSOR(v[3]);
    camprojmatrix.transform(vec(bbmin.x, bbmin.y, bbmax.z), v[4]);
    ADDXYSCISSOR(v[4]);
    camprojmatrix.transform(vec(bbmax.x, bbmin.y, bbmax.z), v[5]);
    ADDXYSCISSOR(v[5]);
    camprojmatrix.transform(vec(bbmin.x, bbmax.y, bbmax.z), v[6]);
    ADDXYSCISSOR(v[6]);
    camprojmatrix.transform(vec(bbmax.x, bbmax.y, bbmax.z), v[7]);
    ADDXYSCISSOR(v[7]);
    if(sx1 > sx2 || sy1 > sy2) return false;
    loopi(8)
    {
        const vec4 &p = v[i];
        if(p.z >= -p.w) continue;
        loopj(3)
        {
            const vec4 &o = v[i^(1<<j)];
            if(o.z <= -o.w) continue;
#define INTERPXYSCISSOR(p, o) do { \
            float t = (p.z + p.w)/(p.z + p.w - o.z - o.w), \
                  w = p.w + t*(o.w - p.w), \
                  x = (p.x + t*(o.x - p.x))/w, \
                  y = (p.y + t*(o.y - p.y))/w; \
            sx1 = min(sx1, x); \
            sy1 = min(sy1, y); \
            sx2 = max(sx2, x); \
            sy2 = max(sy2, y); \
        } while(0)
            INTERPXYSCISSOR(p, o);
        }
    }
    sx1 = max(sx1, -1.0f);
    sy1 = max(sy1, -1.0f);
    sx2 = min(sx2, 1.0f);
    sy2 = min(sy2, 1.0f);
    return true;
}

bool calcspotscissor(const vec &origin, float radius, const vec &dir, int spot, const vec &spotx, const vec &spoty, float &sx1, float &sy1, float &sx2, float &sy2, float &sz1, float &sz2)
{
    const vec2 &sc = sincos360[spot];
    float spotscale = radius*sc.y/sc.x;
    vec up = vec(spotx).rescale(spotscale), right = vec(spoty).rescale(spotscale), center = vec(dir).mul(radius).add(origin);
#define ADDXYZSCISSOR(p) do { \
        if(p.z >= -p.w) \
        { \
            float x = p.x / p.w, y = p.y / p.w, z = p.z / p.w; \
            sx1 = min(sx1, x); \
            sy1 = min(sy1, y); \
            sz1 = min(sz1, z); \
            sx2 = max(sx2, x); \
            sy2 = max(sy2, y); \
            sz2 = max(sz2, z); \
        } \
    } while(0)
    vec4 v[5];
    sx1 = sy1 = sz1 = 1;
    sx2 = sy2 = sz2 = -1;
    camprojmatrix.transform(vec(center).sub(right).sub(up), v[0]);
    ADDXYZSCISSOR(v[0]);
    camprojmatrix.transform(vec(center).add(right).sub(up), v[1]);
    ADDXYZSCISSOR(v[1]);
    camprojmatrix.transform(vec(center).sub(right).add(up), v[2]);
    ADDXYZSCISSOR(v[2]);
    camprojmatrix.transform(vec(center).add(right).add(up), v[3]);
    ADDXYZSCISSOR(v[3]);
    camprojmatrix.transform(origin, v[4]);
    ADDXYZSCISSOR(v[4]);
    if(sx1 > sx2 || sy1 > sy2 || sz1 > sz2) return false;
    loopi(4)
    {
        const vec4 &p = v[i];
        if(p.z >= -p.w) continue;
        loopj(2)
        {
            const vec4 &o = v[i^(1<<j)];
            if(o.z <= -o.w) continue;
#define INTERPXYZSCISSOR(p, o) do { \
            float t = (p.z + p.w)/(p.z + p.w - o.z - o.w), \
                  w = p.w + t*(o.w - p.w), \
                  x = (p.x + t*(o.x - p.x))/w, \
                  y = (p.y + t*(o.y - p.y))/w; \
            sx1 = min(sx1, x); \
            sy1 = min(sy1, y); \
            sz1 = min(sz1, -1.0f); \
            sx2 = max(sx2, x); \
            sy2 = max(sy2, y); \
        } while(0)
            INTERPXYZSCISSOR(p, o);
        }
        if(v[4].z > -v[4].w) INTERPXYZSCISSOR(p, v[4]);
    }
    if(v[4].z < -v[4].w) loopj(4)
    {
        const vec4 &o = v[j];
        if(o.z <= -o.w) continue;
        INTERPXYZSCISSOR(v[4], o);
    }
    sx1 = max(sx1, -1.0f);
    sy1 = max(sy1, -1.0f);
    sz1 = max(sz1, -1.0f);
    sx2 = min(sx2, 1.0f);
    sy2 = min(sy2, 1.0f);
    sz2 = min(sz2, 1.0f);
    return true;
}

void screenquad()
{
    gle::defvertex(2);
    gle::begin(GL_TRIANGLE_STRIP);
    gle::attribf(1, -1);
    gle::attribf(-1, -1);
    gle::attribf(1, 1);
    gle::attribf(-1, 1);
    gle::end();
    gle::disable();
}

#define SCREENQUAD1(x1, y1, x2, y2, sx1, sy1, sx2, sy2) { \
    gle::defvertex(2); \
    gle::deftexcoord0(); \
    gle::begin(GL_TRIANGLE_STRIP); \
    gle::attribf(x2, y1); gle::attribf(sx2, sy1); \
    gle::attribf(x1, y1); gle::attribf(sx1, sy1); \
    gle::attribf(x2, y2); gle::attribf(sx2, sy2); \
    gle::attribf(x1, y2); gle::attribf(sx1, sy2); \
    gle::end(); \
}

void screenquad(float sw, float sh)
{
    SCREENQUAD1(-1, -1, 1, 1, 0, 0, sw, sh);
    gle::disable();
}

void screenquadflipped(float sw, float sh)
{
    SCREENQUAD1(-1, -1, 1, 1, 0, sh, sw, 0);
    gle::disable();
}

#define SCREENQUAD2(x1, y1, x2, y2, sx1, sy1, sx2, sy2, tx1, ty1, tx2, ty2) { \
    gle::defvertex(2); \
    gle::deftexcoord0(); \
    gle::deftexcoord1(); \
    gle::begin(GL_TRIANGLE_STRIP); \
    gle::attribf(x2, y1); gle::attribf(sx2, sy1); gle::attribf(tx2, ty1); \
    gle::attribf(x1, y1); gle::attribf(sx1, sy1); gle::attribf(tx1, ty1); \
    gle::attribf(x2, y2); gle::attribf(sx2, sy2); gle::attribf(tx2, ty2); \
    gle::attribf(x1, y2); gle::attribf(sx1, sy2); gle::attribf(tx1, ty2); \
    gle::end(); \
}

void screenquad(float sw, float sh, float sw2, float sh2)
{
    SCREENQUAD2(-1, -1, 1, 1, 0, 0, sw, sh, 0, 0, sw2, sh2);
    gle::disable();
}

void screenquadoffset(float x, float y, float w, float h)
{
    SCREENQUAD1(-1, -1, 1, 1, x, y, x+w, y+h);
    gle::disable();
}

void screenquadoffset(float x, float y, float w, float h, float x2, float y2, float w2, float h2)
{
    SCREENQUAD2(-1, -1, 1, 1, x, y, x+w, y+h, x2, y2, x2+w2, y2+h2);
    gle::disable();
}

void hudquad(float x, float y, float w, float h, float tx, float ty, float tw, float th)
{
    SCREENQUAD1(x, y, x+w, y+h, tx, ty, tx+tw, ty+th);
}

void debugquad(float x, float y, float w, float h, float tx, float ty, float tw, float th)
{
    SCREENQUAD1(x, y, x+w, y+h, tx, ty+th, tx+tw, ty);
    gle::disable();
}

VARR(fog, 16, 4000, 1000024);
bvec fogcolor(0x80, 0x99, 0xB3);
HVARFR(fogcolour, 0, 0x8099B3, 0xFFFFFF,
{
    fogcolor = bvec((fogcolour>>16)&0xFF, (fogcolour>>8)&0xFF, fogcolour&0xFF);
});
VAR(fogoverlay, 0, 1, 1);

static float findsurface(int fogmat, const vec &v, int &abovemat)
{
    fogmat &= MATF_VOLUME;
    ivec o(v), co;
    int csize;
    do
    {
        cube &c = lookupcube(o, 0, co, csize);
        int mat = c.material&MATF_VOLUME;
        if(mat != fogmat)
        {
            abovemat = isliquid(mat) ? c.material : MAT_AIR;
            return o.z;
        }
        o.z = co.z + csize;
    }
    while(o.z < worldsize);
    abovemat = MAT_AIR;
    return worldsize;
}

static void blendfog(int fogmat, float below, float blend, float logblend, float &start, float &end, vec &fogc)
{
    switch(fogmat&MATF_VOLUME)
    {
        case MAT_WATER:
        {
            const bvec &wcol = getwatercolor(fogmat), &wdeepcol = getwaterdeepcolor(fogmat);
            int wfog = getwaterfog(fogmat), wdeep = getwaterdeep(fogmat);
            float deepfade = clamp(below/max(wdeep, wfog), 0.0f, 1.0f);
            vec color;
            color.lerp(wcol.tocolor(), wdeepcol.tocolor(), deepfade);
            fogc.add(vec(color).mul(blend));
            end += logblend*min(fog, max(wfog*2, 16));
            break;
        }

        case MAT_LAVA:
        {
            const bvec &lcol = getlavacolor(fogmat);
            int lfog = getlavafog(fogmat);
            fogc.add(lcol.tocolor().mul(blend));
            end += logblend*min(fog, max(lfog*2, 16));
            break;
        }

        default:
            fogc.add(fogcolor.tocolor().mul(blend));
            start += logblend*(fog+64)/8;
            end += logblend*fog;
            break;
    }
}

vec curfogcolor(0, 0, 0);

void setfogcolor(const vec &v)
{
    GLOBALPARAM(fogcolor, v);
}

void zerofogcolor()
{
    setfogcolor(vec(0, 0, 0));
}

void resetfogcolor()
{
    setfogcolor(curfogcolor);
}

static void setfog(int fogmat, float below = 0, float blend = 1, int abovemat = MAT_AIR)
{
    float start = 0, end = 0;
    float logscale = 256, logblend = log(1 + (logscale - 1)*blend) / log(logscale);

    curfogcolor = vec(0, 0, 0);
    blendfog(fogmat, below, blend, logblend, start, end, curfogcolor);
    if(blend < 1) blendfog(abovemat, 0, 1-blend, 1-logblend, start, end, curfogcolor);
    curfogcolor.mul(ldrscale);

    GLOBALPARAM(fogcolor, curfogcolor);
    GLOBALPARAMF(fogparams, start, end, 1/(end - start));
}

static void blendfogoverlay(int fogmat, float below, float blend, vec &overlay)
{
    float maxc;
    switch(fogmat&MATF_VOLUME)
    {
        case MAT_WATER:
        {
            const bvec &wcol = getwatercolor(fogmat), &wdeepcol = getwaterdeepcolor(fogmat);
            int wfog = getwaterfog(fogmat), wdeep = getwaterdeep(fogmat);
            float deepfade = clamp(below/max(wdeep, wfog), 0.0f, 1.0f);
            vec color = vec(wcol.r, wcol.g, wcol.b).lerp(vec(wdeepcol.r, wdeepcol.g, wdeepcol.b), deepfade);
            overlay.add(color.div(min(32.0f + max(color.r, max(color.g, color.b))*7.0f/8.0f, 255.0f)).max(0.4f).mul(blend));
            break;
        }

        case MAT_LAVA:
        {
            const bvec &lcol = getlavacolor(fogmat);
            maxc = max(lcol.r, max(lcol.g, lcol.b));
            overlay.add(vec(lcol.r, lcol.g, lcol.b).div(min(32.0f + maxc*7.0f/8.0f, 255.0f)).max(0.4f).mul(blend));
            break;
        }

        default:
            overlay.add(blend);
            break;
    }
}

void drawfogoverlay(int fogmat, float fogbelow, float fogblend, int abovemat)
{
    SETSHADER(fogoverlay);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    vec overlay(0, 0, 0);
    blendfogoverlay(fogmat, fogbelow, fogblend, overlay);
    blendfogoverlay(abovemat, 0, 1-fogblend, overlay);

    gle::color(overlay);
    screenquad();

    glDisable(GL_BLEND);
}

int drawtex = 0;

GLuint minimaptex = 0;
vec minimapcenter(0, 0, 0), minimapradius(0, 0, 0), minimapscale(0, 0, 0);

void clearminimap()
{
    if(minimaptex) { glDeleteTextures(1, &minimaptex); minimaptex = 0; }
}

VARR(minimapheight, 0, 0, 2<<16);
bvec minimapcolor(0, 0, 0);
HVARFR(minimapcolour, 0, 0, 0xFFFFFF,
{
    minimapcolor = bvec((minimapcolour>>16)&0xFF, (minimapcolour>>8)&0xFF, minimapcolour&0xFF);
});
VARR(minimapclip, 0, 0, 1);
VARFP(minimapsize, 7, 8, 10, { if(minimaptex) drawminimap(); });

void bindminimap()
{
    glBindTexture(GL_TEXTURE_2D, minimaptex);
}

void clipminimap(ivec &bbmin, ivec &bbmax, cube *c = worldroot, const ivec &co = ivec(0, 0, 0), int size = worldsize>>1)
{
    loopi(8)
    {
        ivec o(i, co, size);
        if(c[i].children) clipminimap(bbmin, bbmax, c[i].children, o, size>>1);
        else if(!isentirelysolid(c[i]) && (c[i].material&MATF_CLIP)!=MAT_CLIP)
        {
            loopk(3) bbmin[k] = min(bbmin[k], o[k]);
            loopk(3) bbmax[k] = max(bbmax[k], o[k] + size);
        }
    }
}

void drawminimap()
{
    if(!game::needminimap()) { clearminimap(); return; }

    GLERROR;
    renderprogress(0, "generating mini-map...", 0, !renderedframe);

    drawtex = DRAWTEX_MINIMAP;

    GLERROR;
    gl_setupframe(true);

    int size = 1<<minimapsize, sizelimit = min(hwtexsize, min(gw, gh));
    while(size > sizelimit) size /= 2;
    if(!minimaptex) glGenTextures(1, &minimaptex);

    extern vector<vtxarray *> valist;
    ivec bbmin(worldsize, worldsize, worldsize), bbmax(0, 0, 0);
    loopv(valist)
    {
        vtxarray *va = valist[i];
        loopk(3)
        {
            if(va->geommin[k]>va->geommax[k]) continue;
            bbmin[k] = min(bbmin[k], va->geommin[k]);
            bbmax[k] = max(bbmax[k], va->geommax[k]);
        }
    }
    if(minimapclip)
    {
        ivec clipmin(worldsize, worldsize, worldsize), clipmax(0, 0, 0);
        clipminimap(clipmin, clipmax);
        loopk(2) bbmin[k] = max(bbmin[k], clipmin[k]);
        loopk(2) bbmax[k] = min(bbmax[k], clipmax[k]);
    }

    minimapradius = vec(bbmax).sub(vec(bbmin)).mul(0.5f);
    minimapcenter = vec(bbmin).add(minimapradius);
    minimapradius.x = minimapradius.y = max(minimapradius.x, minimapradius.y);
    minimapscale = vec((0.5f - 1.0f/size)/minimapradius.x, (0.5f - 1.0f/size)/minimapradius.y, 1.0f);

    physent *oldcamera = camera1;
    static physent cmcamera;
    cmcamera = *player;
    cmcamera.reset();
    cmcamera.type = ENT_CAMERA;
    cmcamera.o = vec(minimapcenter.x, minimapcenter.y, minimapheight > 0 ? minimapheight : minimapcenter.z + minimapradius.z + 1);
    cmcamera.yaw = 0;
    cmcamera.pitch = -90;
    cmcamera.roll = 0;
    camera1 = &cmcamera;
    setviewcell(vec(-1, -1, -1));

    float oldldrscale = ldrscale, oldldrscaleb = ldrscaleb;
    int oldvieww = vieww, oldviewh = viewh;
    vieww = viewh = size;

    float zscale = max(float(minimapheight), minimapcenter.z + minimapradius.z + 1) + 1;

    projmatrix.ortho(-minimapradius.x, minimapradius.x, -minimapradius.y, minimapradius.y, 0, 2*zscale);
    setcamprojmatrix();

    resetlights();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    xtravertsva = xtraverts = glde = gbatches = vtris = vverts = 0;
    flipqueries();

    ldrscale = 1;
    ldrscaleb = ldrscale/255;
    GLOBALPARAMF(ldrscale, ldrscale);
    GLOBALPARAM(camera, camera1->o);
    GLOBALPARAMF(millis, lastmillis/1000.0f);

    visiblecubes(false);
    collectlights();
    findmaterials();

    rendergbuffer();

    rendershadowatlas();

    shademinimap(vec(minimapcolor.x*ldrscaleb, minimapcolor.y*ldrscaleb, minimapcolor.z*ldrscaleb));

    if(minimapheight > 0 && minimapheight < minimapcenter.z + minimapradius.z)
    {
        camera1->o.z = minimapcenter.z + minimapradius.z + 1;
        projmatrix.ortho(-minimapradius.x, minimapradius.x, -minimapradius.y, minimapradius.y, -zscale, zscale);
        setcamprojmatrix();
        rendergbuffer(false);
        shademinimap();
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    vieww = oldvieww;
    viewh = oldviewh;
    ldrscale = oldldrscale;
    ldrscaleb = oldldrscaleb;

    camera1 = oldcamera;
    drawtex = 0;

    readhdr(size, size, GL_RGB5, GL_UNSIGNED_BYTE, NULL, GL_TEXTURE_2D, minimaptex);
    setuptexparameters(minimaptex, NULL, 3, 1, GL_RGB5, GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat border[4] = { minimapcolor.x/255.0f, minimapcolor.y/255.0f, minimapcolor.z/255.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer_(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, hudw, hudh);
}

void drawcubemap(int size, const vec &o, float yaw, float pitch, const cubemapside &side)
{
    drawtex = DRAWTEX_ENVMAP;

    physent *oldcamera = camera1;
    static physent cmcamera;
    cmcamera = *player;
    cmcamera.reset();
    cmcamera.type = ENT_CAMERA;
    cmcamera.o = o;
    cmcamera.yaw = yaw;
    cmcamera.pitch = pitch;
    cmcamera.roll = 0;
    camera1 = &cmcamera;
    setviewcell(camera1->o);

    float fogmargin = 1 + WATER_AMPLITUDE + nearplane;
    int fogmat = lookupmaterial(vec(camera1->o.x, camera1->o.y, camera1->o.z - fogmargin))&(MATF_VOLUME|MATF_INDEX), abovemat = MAT_AIR;
    float fogbelow = 0;
    if(isliquid(fogmat&MATF_VOLUME))
    {
        float z = findsurface(fogmat, vec(camera1->o.x, camera1->o.y, camera1->o.z - fogmargin), abovemat) - WATER_OFFSET;
        if(camera1->o.z < z + fogmargin)
        {
            fogbelow = z - camera1->o.z;
        }
        else fogmat = abovemat;
    }
    else fogmat = MAT_AIR;
    setfog(abovemat);

    float oldaspect = aspect, oldfovy = fovy, oldfov = curfov, oldldrscale = ldrscale, oldldrscaleb = ldrscaleb;
    int oldfarplane = farplane, oldvieww = vieww, oldviewh = viewh;
    curfov = fovy = 90;
    aspect = 1;
    farplane = worldsize*2;
    vieww = viewh = size;
    projmatrix.perspective(fovy, aspect, nearplane, farplane);
    setcamprojmatrix();

    resetlights();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    xtravertsva = xtraverts = glde = gbatches = vtris = vverts = 0;
    flipqueries();

    ldrscale = 1;
    ldrscaleb = ldrscale/255;
    GLOBALPARAMF(ldrscale, ldrscale);
    GLOBALPARAM(camera, camera1->o);
    GLOBALPARAMF(millis, lastmillis/1000.0f);

    visiblecubes();
    GLERROR;

    rendergbuffer();
    GLERROR;

    rendershadowatlas();
    GLERROR;

    shadegbuffer();
    GLERROR;

    if(fogmat)
    {
        setfog(fogmat, fogbelow, 1, abovemat);

        renderwaterfog(fogmat, fogbelow);

        setfog(fogmat, fogbelow, clamp(fogbelow, 0.0f, 1.0f), abovemat);
    }

    rendertransparent();
    GLERROR;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    aspect = oldaspect;
    fovy = oldfovy;
    curfov = oldfov;
    farplane = oldfarplane;
    vieww = oldvieww;
    viewh = oldviewh;
    ldrscale = oldldrscale;
    ldrscaleb = oldldrscaleb;

    camera1 = oldcamera;
    drawtex = 0;
}

namespace modelpreview
{
    physent *oldcamera;
    physent camera;

    float oldaspect, oldfovy, oldfov, oldldrscale, oldldrscaleb;
    int oldfarplane, oldvieww, oldviewh;

    int x, y, w, h;
    bool background, scissor;

    void start(int x, int y, int w, int h, bool background, bool scissor)
    {
        modelpreview::x = x;
        modelpreview::y = y;
        modelpreview::w = w;
        modelpreview::h = h;
        modelpreview::background = background;
        modelpreview::scissor = scissor;

        setupgbuffer();

        useshaderbyname("modelpreview");

        drawtex = DRAWTEX_MODELPREVIEW;

        oldcamera = camera1;
        camera = *camera1;
        camera.reset();
        camera.type = ENT_CAMERA;
        camera.o = vec(0, 0, 0);
        camera.yaw = 0;
        camera.pitch = 0;
        camera.roll = 0;
        camera1 = &camera;

        oldaspect = aspect;
        oldfovy = fovy;
        oldfov = curfov;
        oldldrscale = ldrscale;
        oldldrscaleb = ldrscaleb;
        oldfarplane = farplane;
        oldvieww = vieww;
        oldviewh = viewh;

        aspect = w/float(h);
        fovy = 90;
        curfov = 2*atan2(tan(fovy/2*RAD), 1/aspect)/RAD;
        farplane = 1024;
        vieww = min(gw, w);
        viewh = min(gh, h);
        ldrscale = 1;
        ldrscaleb = ldrscale/255;

        GLOBALPARAMF(ldrscale, ldrscale);
        GLOBALPARAM(camera, camera1->o);
        GLOBALPARAMF(millis, lastmillis/1000.0f);

        projmatrix.perspective(fovy, aspect, nearplane, farplane);
        setcamprojmatrix();

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        preparegbuffer();

        resetmodelbatches();
    }

    void end()
    {
        rendermodelbatches();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        shademodelpreview(x, y, w, h, background, scissor);

        aspect = oldaspect;
        fovy = oldfovy;
        curfov = oldfov;
        farplane = oldfarplane;
        vieww = oldvieww;
        viewh = oldviewh;
        ldrscale = oldldrscale;
        ldrscaleb = oldldrscaleb;

        camera1 = oldcamera;
        drawtex = 0;
    }
}

int xtraverts, xtravertsva;

void gl_drawview()
{
    GLuint scalefbo = shouldscale();
    if(scalefbo) { vieww = gw; viewh = gh; }

    float fogmargin = 1 + WATER_AMPLITUDE + nearplane;
    int fogmat = lookupmaterial(vec(camera1->o.x, camera1->o.y, camera1->o.z - fogmargin))&(MATF_VOLUME|MATF_INDEX), abovemat = MAT_AIR;
    float fogbelow = 0;
    if(isliquid(fogmat&MATF_VOLUME))
    {
        float z = findsurface(fogmat, vec(camera1->o.x, camera1->o.y, camera1->o.z - fogmargin), abovemat) - WATER_OFFSET;
        if(camera1->o.z < z + fogmargin)
        {
            fogbelow = z - camera1->o.z;
        }
        else fogmat = abovemat;
    }
    else fogmat = MAT_AIR;
    setfog(abovemat);
    //setfog(fogmat, fogbelow, 1, abovemat);

    farplane = worldsize*2;

    resetlights();

    projmatrix.perspective(fovy, aspect, nearplane, farplane);
    setcamprojmatrix();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    ldrscale = hdr ? 0.5f : 1;
    ldrscaleb = ldrscale/255;
    GLOBALPARAMF(ldrscale, ldrscale);
    GLOBALPARAM(camera, camera1->o);
    GLOBALPARAMF(millis, lastmillis/1000.0f);

    visiblecubes();

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    rendergbuffer();

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else if(limitsky() && editmode) renderexplicitsky(true);

    renderao();
    GLERROR;

    // render grass after AO to avoid disturbing shimmering patterns
    generategrass();
    rendergrass();
    GLERROR;

    renderradiancehints();
    GLERROR;

    rendershadowatlas();
    GLERROR;

    shadegbuffer();
    GLERROR;

    if(fogmat)
    {
        setfog(fogmat, fogbelow, 1, abovemat);

        renderwaterfog(fogmat, fogbelow);

        setfog(fogmat, fogbelow, clamp(fogbelow, 0.0f, 1.0f), abovemat);
    }

    rendertransparent();
    GLERROR;

    if(fogmat) setfog(fogmat, fogbelow, 1, abovemat);

    if(editmode)
    {
        extern int outline;
        if(!wireframe && outline) renderoutline();
        GLERROR;
        rendereditmaterials();
        GLERROR;
        renderparticles();
        GLERROR;
    }

    extern int hidehud;
    if(!hidehud)
    {
        glDepthMask(GL_FALSE);
        if(editmode)
        {
            renderblendbrush();
            rendereditcursor();
        }

        extern int showenthelpers;
        extern void renderhelpers();
        if((showenthelpers==1 && game::showenthelpers()) || showenthelpers == 2) renderhelpers();

        glDepthMask(GL_TRUE);
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    if(fogoverlay && fogmat != MAT_AIR) drawfogoverlay(fogmat, fogbelow, clamp(fogbelow, 0.0f, 1.0f), abovemat);

    GLuint outfbo = scalefbo ? scalefbo : ovr::lensfbo[viewidx];
    doaa(setuppostfx(vieww, viewh, outfbo), processhdr);
    renderpostfx(outfbo);
    if(scalefbo) doscale(ovr::lensfbo[viewidx]);
}

void gl_drawmainmenu()
{
    if(ovr::enabled)
    {
        glBindFramebuffer_(GL_FRAMEBUFFER, ovr::lensfbo[viewidx]);
        glViewport(0, 0, hudw, hudh);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    renderbackground(NULL, NULL, NULL, NULL, true);
}

VARNP(damagecompass, usedamagecompass, 0, 1, 1);
VARP(damagecompassfade, 1, 1000, 10000);
VARP(damagecompasssize, 1, 30, 100);
VARP(damagecompassalpha, 1, 25, 100);
VARP(damagecompassmin, 1, 25, 1000);
VARP(damagecompassmax, 1, 200, 1000);

float damagedirs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void damagecompass(int n, const vec &loc)
{
    if(!usedamagecompass || minimized) return;
    vec delta(loc);
    delta.sub(camera1->o);
    float yaw = 0, pitch;
    if(delta.magnitude() > 4)
    {
        vectoyawpitch(delta, yaw, pitch);
        yaw -= camera1->yaw;
    }
    if(yaw >= 360) yaw = fmod(yaw, 360);
    else if(yaw < 0) yaw = 360 - fmod(-yaw, 360);
    int dir = (int(yaw+22.5f)%360)/45;
    damagedirs[dir] += max(n, damagecompassmin)/float(damagecompassmax);
    if(damagedirs[dir]>1) damagedirs[dir] = 1;

}
void drawdamagecompass(int w, int h)
{
    hudnotextureshader->set();

    int dirs = 0;
    float size = damagecompasssize/100.0f*min(h, w)/2.0f;
    loopi(8) if(damagedirs[i]>0)
    {
        if(!dirs)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            gle::colorf(1, 0, 0, damagecompassalpha/100.0f);
            gle::defvertex();
            gle::begin(GL_TRIANGLES);
        }
        dirs++;

        float logscale = 32,
              scale = log(1 + (logscale - 1)*damagedirs[i]) / log(logscale),
              offset = -size/2.0f-min(h, w)/4.0f;
        matrix3x4 m;
        m.identity();
        m.settranslation(w/2, h/2, 0);
        m.rotate_around_z(i*45*RAD);
        m.translate(0, offset, 0);
        m.scale(size*scale);

        gle::attrib(m.transform(vec2(1, 1)));
        gle::attrib(m.transform(vec2(-1, 1)));
        gle::attrib(m.transform(vec2(0, 0)));

        // fade in log space so short blips don't disappear too quickly
        scale -= float(curtime)/damagecompassfade;
        damagedirs[i] = scale > 0 ? (pow(logscale, scale) - 1) / (logscale - 1) : 0;
    }
    if(dirs) gle::end();
}

int damageblendmillis = 0;

VARFP(damagescreen, 0, 1, 1, { if(!damagescreen) damageblendmillis = 0; });
VARP(damagescreenfactor, 1, 75, 100);
VARP(damagescreenalpha, 1, 45, 100);
VARP(damagescreenfade, 0, 1000, 1000);
VARP(damagescreenmin, 1, 10, 1000);
VARP(damagescreenmax, 1, 100, 1000);

void damageblend(int n)
{
    if(!damagescreen || minimized) return;
    if(lastmillis > damageblendmillis) damageblendmillis = lastmillis;
    damageblendmillis += clamp(n, damagescreenmin, damagescreenmax)*damagescreenfactor;
}

void drawdamagescreen(int w, int h)
{
    if(lastmillis >= damageblendmillis) return;

    hudshader->set();

    static Texture *damagetex = NULL;
    if(!damagetex) damagetex = textureload("media/interface/damage", 3);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, damagetex->id);
    float fade = damagescreenalpha/100.0f;
    if(damageblendmillis - lastmillis < damagescreenfade)
        fade *= float(damageblendmillis - lastmillis)/damagescreenfade;
    gle::colorf(fade, fade, fade, fade);

    hudquad(0, 0, w, h);
}

VAR(hidestats, 0, 0, 1);
VAR(hidehud, 0, 0, 1);

VARP(crosshairsize, 0, 15, 50);
VARP(crosshaircol, 0, 1, 1);
VARP(crosshairimg, 0, 1, 1);

SVARP(uicursor, "media/interface/cursor");
VARP(cursorsize, 5, 20, 50);

vector<Texture *> crosshairs;

void loadcrosshair(const char *name, int i)
{
    if(i < 0) return;

    if(!crosshairs.inrange(i))
    {
        int len = crosshairs.length();
        crosshairs.growbuf(i + 1);
        crosshairs.advance(i + 1 - len);

        loopj(crosshairs.length() - len)
            crosshairs[j + len] = NULL;
    }

    crosshairs[i] = name ? textureload(name, 3, true) : notexture;
    if(crosshairs[i] == notexture)
        crosshairs[i] = textureload(game::defaultcrosshair(i) ? game::defaultcrosshair(i) : "media/crosshairs/default", 3, true);
}

ICOMMAND(loadcrosshair, "si", (const char *name, int *i), loadcrosshair(name, *i));

ICOMMAND(getcrosshair, "i", (int *i),
    const char *name = "media/crosshairs/default.png";
    if(crosshairs.inrange(*i))
    {
        name = crosshairs[*i] ? crosshairs[*i]->name : game::defaultcrosshair(*i);
        if(!name) name = "media/crosshairs/default.png";
    }
    result(name);
)


void writecrosshairs(stream *f)
{
    loopv(crosshairs) if(crosshairs[i] && crosshairs[i]!=notexture)
        f->printf("loadcrosshair %s %d\n", escapestring(crosshairs[i]->name), i);
    f->printf("\n");
}

void drawcrosshair(int w, int h)
{
    bool windowhit = UI::hascursor();

    if(!windowhit && hidehud && !UI::mainmenu) return;

    vec color(1, 1, 1);
    float cx = 0.5f, cy = 0.5f, chsize;
    Texture *crosshair;

    UI::getcursorpos(cx, cy);

    if(windowhit)
    {
        crosshair = textureload(uicursor, 3, true);
        chsize = cursorsize*w/900.0f;
    }
    else
    {
        int index = game::selectcrosshair(color);
        if(index < 0) return;
        if(!crosshaircol) color = vec(1, 1, 1);
        if(!crosshairimg) index = 0;
        crosshair = crosshairs.inrange(index) ? crosshairs[index] : NULL;
        if(!crosshair)
        {
            loadcrosshair(NULL, index);
            crosshair = crosshairs[index];
        }
        chsize = crosshairsize*w/900.0f;
    }
    if(crosshair->type&Texture::ALPHA) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    else glBlendFunc(GL_ONE, GL_ONE);
    gle::color(color);
    float x = cx*w - (windowhit ? 0 : chsize/2.0f);
    float y = cy*h - (windowhit ? 0 : chsize/2.0f);
    glBindTexture(GL_TEXTURE_2D, crosshair->id);

    hudquad(x, y, chsize, chsize);
}

VARP(wallclock, 0, 0, 1);
VARP(wallclock24, 0, 0, 1);
VARP(wallclocksecs, 0, 0, 1);

static time_t walltime = 0;

VARP(showfps, 0, 1, 1);
VARP(showfpsrange, 0, 0, 1);
VAR(statrate, 1, 200, 1000);

FVARP(conscale, 1e-3f, 0.33f, 1e3f);

void gl_drawhud()
{
    int w = hudw, h = hudh;
    if(forceaspect) w = int(ceil(h*forceaspect));

    gettextres(w, h);

    hudmatrix.ortho(0, w, h, 0, -1, 1);
    resethudmatrix();
    hudshader->set();

    gle::colorf(1, 1, 1);

    debuglights();

    glEnable(GL_BLEND);

    debugparticles();

    if(!UI::mainmenu)
    {
        drawdamagescreen(w, h);
        drawdamagecompass(w, h);
    }

    hudshader->set();

    float conw = w/conscale, conh = h/conscale, abovehud = conh - FONTH;
    if(!hidehud && !UI::mainmenu)
    {
        if(hidestats || !editmode)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            game::gameplayhud(w, h);
            abovehud = min(abovehud, conh*game::abovegameplayhud(w, h));
        }

        if(!hidestats)
        {
            pushhudmatrix();
            hudmatrix.scale(conscale, conscale, 1);
            flushhudmatrix();

            int roffset = 0;
            if(showfps)
            {
                static int lastfps = 0, prevfps[3] = { 0, 0, 0 }, curfps[3] = { 0, 0, 0 };
                if(totalmillis - lastfps >= statrate)
                {
                    memcpy(prevfps, curfps, sizeof(prevfps));
                    lastfps = totalmillis - (totalmillis%statrate);
                }
                int nextfps[3];
                getfps(nextfps[0], nextfps[1], nextfps[2]);
                loopi(3) if(prevfps[i]==curfps[i]) curfps[i] = nextfps[i];
                if(showfpsrange) draw_textf("fps %d+%d-%d", conw-7*FONTH, conh-FONTH*3/2, curfps[0], curfps[1], curfps[2]);
                else draw_textf("fps %d", conw-5*FONTH, conh-FONTH*3/2, curfps[0]);
                roffset += FONTH;
            }

            printtimers(conw, conh);

            if(wallclock)
            {
                if(!walltime) { walltime = time(NULL); walltime -= totalmillis/1000; if(!walltime) walltime++; }
                time_t walloffset = walltime + totalmillis/1000;
                struct tm *localvals = localtime(&walloffset);
                static string buf;
                if(localvals && strftime(buf, sizeof(buf), wallclocksecs ? (wallclock24 ? "%H:%M:%S" : "%I:%M:%S%p") : (wallclock24 ? "%H:%M" : "%I:%M%p"), localvals))
                {
                    // hack because not all platforms (windows) support %P lowercase option
                    // also strip leading 0 from 12 hour time
                    char *dst = buf;
                    const char *src = &buf[!wallclock24 && buf[0]=='0' ? 1 : 0];
                    while(*src) *dst++ = tolower(*src++);
                    *dst++ = '\0';
                    draw_text(buf, conw-5*FONTH, conh-FONTH*3/2-roffset);
                    roffset += FONTH;
                }
            }

            pophudmatrix();
        }

        rendertexturepanel(w, h);
    }

    pushhudmatrix();
    hudmatrix.scale(conscale, conscale, 1);
    flushhudmatrix();
    abovehud -= rendercommand(FONTH/2, abovehud - FONTH/2, conw-FONTH);
    if(!hidehud && !UI::uivisible("fullconsole")) renderconsole(conw, conh, abovehud - FONTH/2);
    pophudmatrix();

    drawcrosshair(w, h);

    gle::disable();
    glDisable(GL_BLEND);

    if(frametimer)
    {
        glFinish();
        framemillis = getclockmillis() - totalmillis;
    }
}

int renderw = 0, renderh = 0, hudx = 0, hudy = 0, hudw = 0, hudh = 0;

void gl_setupframe(bool force)
{
    extern int scr_w, scr_h;
    renderw = min(scr_w, screenw);
    renderh = min(scr_h, screenh);
    hudw = screenw;
    hudh = screenh;
    ovr::setup();
    if(!force) return;
    setuplights();
}

void gl_drawframe()
{
    synctimers();
    xtravertsva = xtraverts = glde = gbatches = vtris = vverts = 0;
    flipqueries();
    aspect = forceaspect ? forceaspect : hudw/float(hudh);
    float fovx = curfov;
    if(ovr::enabled && ovr::fov) fovx *= ovr::fov/fov;
    fovy = 2*atan2(tan(fovx/2*RAD), aspect)/RAD;
    vieww = hudw;
    viewh = hudh;
    loopi(2)
    {
        if(UI::mainmenu) gl_drawmainmenu();
        else gl_drawview();
        UI::render();
        gl_drawhud();
        if(!ovr::enabled) break;
        ovr::warp();
        ++viewidx;
        hudx += hudw;
    }
    viewidx = 0;
    hudx = 0;
}

void cleanupgl()
{
    clearminimap();
    cleanuptimers();
    gle::cleanup();
    ovr::cleanup();
}

void sethudnotextureshader()
{
    hudnotextureshader->set();
}

void sethudshader()
{
    hudshader->set();
}
