//
// Copyright(c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// entry_points_egl.cpp : Implements the EGL entry points.

#include "libGLESv2/entry_points_egl.h"
#include "libGLESv2/entry_points_egl_ext.h"
#include "libGLESv2/entry_points_gles_2_0_ext.h"
#include "libGLESv2/entry_points_gles_3_0_ext.h"
#include "libGLESv2/global_state.h"

#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/Texture.h"
#include "libANGLE/Surface.h"

#include "common/debug.h"
#include "common/version.h"

#include <EGL/eglext.h>

namespace egl
{

// EGL object validation
static bool ValidateDisplay(Display *display)
{
    if (display == EGL_NO_DISPLAY)
    {
        SetGlobalError(Error(EGL_BAD_DISPLAY));
        return false;
    }

    if (!display->isInitialized())
    {
        SetGlobalError(Error(EGL_NOT_INITIALIZED));
        return false;
    }

    return true;
}

static bool ValidateConfig(Display *display, EGLConfig config)
{
    if (!ValidateDisplay(display))
    {
        return false;
    }

    if (!display->isValidConfig(config))
    {
        SetGlobalError(Error(EGL_BAD_CONFIG));
        return false;
    }

    return true;
}

static bool ValidateContext(Display *display, gl::Context *context)
{
    if (!ValidateDisplay(display))
    {
        return false;
    }

    if (!display->isValidContext(context))
    {
        SetGlobalError(Error(EGL_BAD_CONTEXT));
        return false;
    }

    return true;
}

static bool ValidateSurface(Display *display, Surface *surface)
{
    if (!ValidateDisplay(display))
    {
        return false;
    }

    if (!display->isValidSurface(surface))
    {
        SetGlobalError(Error(EGL_BAD_SURFACE));
        return false;
    }

    return true;
}

// EGL 1.0
EGLint EGLAPIENTRY GetError(void)
{
    EVENT("()");

    EGLint error = GetGlobalError();
    SetGlobalError(Error(EGL_SUCCESS));
    return error;
}

EGLDisplay EGLAPIENTRY GetDisplay(EGLNativeDisplayType display_id)
{
    EVENT("(EGLNativeDisplayType display_id = 0x%0.8p)", display_id);

    return Display::getDisplay(display_id, AttributeMap());
}

EGLBoolean EGLAPIENTRY Initialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLint *major = 0x%0.8p, EGLint *minor = 0x%0.8p)",
          dpy, major, minor);

    if (dpy == EGL_NO_DISPLAY)
    {
        SetGlobalError(Error(EGL_BAD_DISPLAY));
        return EGL_FALSE;
    }

    Display *display = static_cast<Display*>(dpy);

    Error error = display->initialize();
    if (error.isError())
    {
        SetGlobalError(error);
        return EGL_FALSE;
    }

    if (major) *major = 1;
    if (minor) *minor = 4;

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY Terminate(EGLDisplay dpy)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p)", dpy);

    if (dpy == EGL_NO_DISPLAY)
    {
        SetGlobalError(Error(EGL_BAD_DISPLAY));
        return EGL_FALSE;
    }

    Display *display = static_cast<Display*>(dpy);
    gl::Context *context = GetGlobalContext();

    if (display->isValidContext(context))
    {
        SetGlobalContext(NULL);
        SetGlobalDisplay(NULL);
    }

    display->terminate();

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

const char *EGLAPIENTRY QueryString(EGLDisplay dpy, EGLint name)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLint name = %d)", dpy, name);

    Display *display = static_cast<Display*>(dpy);
    if (!(display == EGL_NO_DISPLAY && name == EGL_EXTENSIONS) && !ValidateDisplay(display))
    {
        return NULL;
    }

    const char *result;
    switch (name)
    {
      case EGL_CLIENT_APIS:
        result = "OpenGL_ES";
        break;
      case EGL_EXTENSIONS:
        result = Display::getExtensionString(display);
        break;
      case EGL_VENDOR:
        result = display->getVendorString();
        break;
      case EGL_VERSION:
        result = "1.4 (ANGLE " ANGLE_VERSION_STRING ")";
        break;
      default:
        SetGlobalError(Error(EGL_BAD_PARAMETER));
        return NULL;
    }

    SetGlobalError(Error(EGL_SUCCESS));
    return result;
}

EGLBoolean EGLAPIENTRY GetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig *configs = 0x%0.8p, "
          "EGLint config_size = %d, EGLint *num_config = 0x%0.8p)",
          dpy, configs, config_size, num_config);

    Display *display = static_cast<Display*>(dpy);

    if (!ValidateDisplay(display))
    {
        return EGL_FALSE;
    }

    if (!num_config)
    {
        SetGlobalError(Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    const EGLint attribList[] =    {EGL_NONE};

    if (!display->getConfigs(configs, attribList, config_size, num_config))
    {
        SetGlobalError(Error(EGL_BAD_ATTRIBUTE));
        return EGL_FALSE;
    }

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY ChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, const EGLint *attrib_list = 0x%0.8p, "
          "EGLConfig *configs = 0x%0.8p, EGLint config_size = %d, EGLint *num_config = 0x%0.8p)",
          dpy, attrib_list, configs, config_size, num_config);

    Display *display = static_cast<Display*>(dpy);

    if (!ValidateDisplay(display))
    {
        return EGL_FALSE;
    }

    if (!num_config)
    {
        SetGlobalError(Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    const EGLint attribList[] =    {EGL_NONE};

    if (!attrib_list)
    {
        attrib_list = attribList;
    }

    display->getConfigs(configs, attrib_list, config_size, num_config);

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY GetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLint attribute = %d, EGLint *value = 0x%0.8p)",
          dpy, config, attribute, value);

    Display *display = static_cast<Display*>(dpy);

    if (!ValidateConfig(display, config))
    {
        return EGL_FALSE;
    }

    if (!display->getConfigAttrib(config, attribute, value))
    {
        SetGlobalError(Error(EGL_BAD_ATTRIBUTE));
        return EGL_FALSE;
    }

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLSurface EGLAPIENTRY CreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLNativeWindowType win = 0x%0.8p, "
          "const EGLint *attrib_list = 0x%0.8p)", dpy, config, win, attrib_list);

    Display *display = static_cast<Display*>(dpy);

    if (!ValidateConfig(display, config))
    {
        return EGL_NO_SURFACE;
    }

    if (!display->isValidNativeWindow(win))
    {
        SetGlobalError(Error(EGL_BAD_NATIVE_WINDOW));
        return EGL_NO_SURFACE;
    }

    EGLSurface surface = EGL_NO_SURFACE;
    Error error = display->createWindowSurface(win, config, attrib_list, &surface);
    if (error.isError())
    {
        SetGlobalError(error);
        return EGL_NO_SURFACE;
    }

    return surface;
}

EGLSurface EGLAPIENTRY CreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, const EGLint *attrib_list = 0x%0.8p)",
          dpy, config, attrib_list);

    Display *display = static_cast<Display*>(dpy);

    if (!ValidateConfig(display, config))
    {
        return EGL_NO_SURFACE;
    }

    EGLSurface surface = EGL_NO_SURFACE;
    Error error = display->createOffscreenSurface(config, NULL, attrib_list, &surface);
    if (error.isError())
    {
        SetGlobalError(error);
        return EGL_NO_SURFACE;
    }

    return surface;
}

EGLSurface EGLAPIENTRY CreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLNativePixmapType pixmap = 0x%0.8p, "
          "const EGLint *attrib_list = 0x%0.8p)", dpy, config, pixmap, attrib_list);

    Display *display = static_cast<Display*>(dpy);

    if (!ValidateConfig(display, config))
    {
        return EGL_NO_SURFACE;
    }

    UNIMPLEMENTED();   // FIXME

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_NO_SURFACE;
}

EGLBoolean EGLAPIENTRY DestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p)", dpy, surface);

    Display *display = static_cast<Display*>(dpy);
    Surface *eglSurface = static_cast<Surface*>(surface);

    if (!ValidateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE)
    {
        SetGlobalError(Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    display->destroySurface((Surface*)surface);

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY QuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint attribute = %d, EGLint *value = 0x%0.8p)",
          dpy, surface, attribute, value);

    Display *display = static_cast<Display*>(dpy);
    Surface *eglSurface = (Surface*)surface;

    if (!ValidateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE)
    {
        SetGlobalError(Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    switch (attribute)
    {
      case EGL_VG_ALPHA_FORMAT:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_VG_COLORSPACE:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_CONFIG_ID:
        *value = eglSurface->getConfigID();
        break;
      case EGL_HEIGHT:
        *value = eglSurface->getHeight();
        break;
      case EGL_HORIZONTAL_RESOLUTION:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_LARGEST_PBUFFER:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_MIPMAP_TEXTURE:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_MIPMAP_LEVEL:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_MULTISAMPLE_RESOLVE:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_PIXEL_ASPECT_RATIO:
        *value = eglSurface->getPixelAspectRatio();
        break;
      case EGL_RENDER_BUFFER:
        *value = eglSurface->getRenderBuffer();
        break;
      case EGL_SWAP_BEHAVIOR:
        *value = eglSurface->getSwapBehavior();
        break;
      case EGL_TEXTURE_FORMAT:
        *value = eglSurface->getTextureFormat();
        break;
      case EGL_TEXTURE_TARGET:
        *value = eglSurface->getTextureTarget();
        break;
      case EGL_VERTICAL_RESOLUTION:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_WIDTH:
        *value = eglSurface->getWidth();
        break;
      case EGL_POST_SUB_BUFFER_SUPPORTED_NV:
        *value = eglSurface->isPostSubBufferSupported();
        break;
      case EGL_FIXED_SIZE_ANGLE:
        *value = eglSurface->isFixedSize();
        break;
      default:
        SetGlobalError(Error(EGL_BAD_ATTRIBUTE));
        return EGL_FALSE;
    }

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLContext EGLAPIENTRY CreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLContext share_context = 0x%0.8p, "
          "const EGLint *attrib_list = 0x%0.8p)", dpy, config, share_context, attrib_list);

    // Get the requested client version (default is 1) and check it is 2 or 3.
    EGLint clientMajorVersion = 1;
    EGLint clientMinorVersion = 0;
    EGLint contextFlags = 0;
    bool resetNotification = false;
    bool robustAccess = false;

    if (attrib_list)
    {
        for (const EGLint* attribute = attrib_list; attribute[0] != EGL_NONE; attribute += 2)
        {
            switch (attribute[0])
            {
              case EGL_CONTEXT_CLIENT_VERSION:
                clientMajorVersion = attribute[1];
                break;
              case EGL_CONTEXT_MINOR_VERSION:
                clientMinorVersion = attribute[1];
                break;

              case EGL_CONTEXT_FLAGS_KHR:
                contextFlags = attribute[1];
                break;

              case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
                // Only valid for OpenGL (non-ES) contexts
                SetGlobalError(Error(EGL_BAD_ATTRIBUTE));
                return EGL_NO_CONTEXT;

              case EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT:
                if (attribute[1] != EGL_TRUE && attribute[1] != EGL_FALSE)
                {
                    SetGlobalError(Error(EGL_BAD_ATTRIBUTE));
                    return EGL_NO_CONTEXT;
                }

                robustAccess = (attribute[1] == EGL_TRUE);
                break;

              case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR:
                META_ASSERT(EGL_LOSE_CONTEXT_ON_RESET_EXT == EGL_LOSE_CONTEXT_ON_RESET_KHR);
                META_ASSERT(EGL_NO_RESET_NOTIFICATION_EXT == EGL_NO_RESET_NOTIFICATION_KHR);
                // same as EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT, fall through
              case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT:
                if (attribute[1] == EGL_LOSE_CONTEXT_ON_RESET_EXT)
                {
                    resetNotification = true;
                }
                else if (attribute[1] != EGL_NO_RESET_NOTIFICATION_EXT)
                {
                    SetGlobalError(Error(EGL_BAD_ATTRIBUTE));
                    return EGL_NO_CONTEXT;
                }
                break;
              default:
                SetGlobalError(Error(EGL_BAD_ATTRIBUTE));
                return EGL_NO_CONTEXT;
            }
        }
    }

    if ((clientMajorVersion != 2 && clientMajorVersion != 3) || clientMinorVersion != 0)
    {
        SetGlobalError(Error(EGL_BAD_CONFIG));
        return EGL_NO_CONTEXT;
    }

    // Note: EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR does not apply to ES
    const EGLint validContextFlags = (EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR |
                                      EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR);
    if ((contextFlags & ~validContextFlags) != 0)
    {
        SetGlobalError(Error(EGL_BAD_ATTRIBUTE));
        return EGL_NO_CONTEXT;
    }

    if ((contextFlags & EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR) > 0)
    {
        robustAccess = true;
    }

    if (robustAccess)
    {
        // Unimplemented
        SetGlobalError(Error(EGL_BAD_CONFIG));
        return EGL_NO_CONTEXT;
    }

    Display *display = static_cast<Display*>(dpy);

    if (share_context)
    {
        gl::Context* sharedGLContext = static_cast<gl::Context*>(share_context);

        if (sharedGLContext->isResetNotificationEnabled() != resetNotification)
        {
            SetGlobalError(Error(EGL_BAD_MATCH));
            return EGL_NO_CONTEXT;
        }

        if (sharedGLContext->getClientVersion() != clientMajorVersion)
        {
            SetGlobalError(Error(EGL_BAD_CONTEXT));
            return EGL_NO_CONTEXT;
        }

        // Can not share contexts between displays
        if (sharedGLContext->getRenderer() != display->getRenderer())
        {
            SetGlobalError(Error(EGL_BAD_MATCH));
            return EGL_NO_CONTEXT;
        }
    }

    if (!ValidateConfig(display, config))
    {
        return EGL_NO_CONTEXT;
    }

    EGLContext context = EGL_NO_CONTEXT;
    Error error = display->createContext(config, clientMajorVersion, static_cast<gl::Context*>(share_context),
                                         resetNotification, robustAccess, &context);
    if (error.isError())
    {
        SetGlobalError(error);
        return EGL_NO_CONTEXT;
    }

    return context;
}

EGLBoolean EGLAPIENTRY DestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLContext ctx = 0x%0.8p)", dpy, ctx);

    Display *display = static_cast<Display*>(dpy);
    gl::Context *context = static_cast<gl::Context*>(ctx);

    if (!ValidateContext(display, context))
    {
        return EGL_FALSE;
    }

    if (ctx == EGL_NO_CONTEXT)
    {
        SetGlobalError(Error(EGL_BAD_CONTEXT));
        return EGL_FALSE;
    }

    if (context == GetGlobalContext())
    {
        SetGlobalDisplay(NULL);
        SetGlobalContext(NULL);
    }

    display->destroyContext(context);

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY MakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface draw = 0x%0.8p, EGLSurface read = 0x%0.8p, EGLContext ctx = 0x%0.8p)",
          dpy, draw, read, ctx);

    Display *display = static_cast<Display*>(dpy);
    gl::Context *context = static_cast<gl::Context*>(ctx);

    bool noContext = (ctx == EGL_NO_CONTEXT);
    bool noSurface = (draw == EGL_NO_SURFACE || read == EGL_NO_SURFACE);
    if (noContext != noSurface)
    {
        SetGlobalError(Error(EGL_BAD_MATCH));
        return EGL_FALSE;
    }

    if (ctx != EGL_NO_CONTEXT && !ValidateContext(display, context))
    {
        return EGL_FALSE;
    }

    if (dpy != EGL_NO_DISPLAY && display->isInitialized())
    {
        rx::Renderer *renderer = display->getRenderer();
        if (renderer->testDeviceLost())
        {
            display->notifyDeviceLost();
            return EGL_FALSE;
        }

        if (renderer->isDeviceLost())
        {
            SetGlobalError(Error(EGL_CONTEXT_LOST));
            return EGL_FALSE;
        }
    }

    Surface *drawSurface = static_cast<Surface*>(draw);
    Surface *readSurface = static_cast<Surface*>(read);

    if ((draw != EGL_NO_SURFACE && !ValidateSurface(display, drawSurface)) ||
        (read != EGL_NO_SURFACE && !ValidateSurface(display, readSurface)))
    {
        return EGL_FALSE;
    }

    if (draw != read)
    {
        UNIMPLEMENTED();   // FIXME
    }

    SetGlobalDisplay(display);
    SetGlobalDrawSurface(drawSurface);
    SetGlobalReadSurface(readSurface);
    SetGlobalContext(context);

    if (context != nullptr && display != nullptr && drawSurface != nullptr)
    {
        context->makeCurrent(drawSurface);
    }

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLSurface EGLAPIENTRY GetCurrentSurface(EGLint readdraw)
{
    EVENT("(EGLint readdraw = %d)", readdraw);

    if (readdraw == EGL_READ)
    {
        SetGlobalError(Error(EGL_SUCCESS));
        return GetGlobalReadSurface();
    }
    else if (readdraw == EGL_DRAW)
    {
        SetGlobalError(Error(EGL_SUCCESS));
        return GetGlobalDrawSurface();
    }
    else
    {
        SetGlobalError(Error(EGL_BAD_PARAMETER));
        return EGL_NO_SURFACE;
    }
}

EGLDisplay EGLAPIENTRY GetCurrentDisplay(void)
{
    EVENT("()");

    EGLDisplay dpy = GetGlobalDisplay();

    SetGlobalError(Error(EGL_SUCCESS));
    return dpy;
}

EGLBoolean EGLAPIENTRY QueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLContext ctx = 0x%0.8p, EGLint attribute = %d, EGLint *value = 0x%0.8p)",
          dpy, ctx, attribute, value);

    Display *display = static_cast<Display*>(dpy);
    gl::Context *context = static_cast<gl::Context*>(ctx);

    if (!ValidateContext(display, context))
    {
        return EGL_FALSE;
    }

    UNIMPLEMENTED();   // FIXME

    SetGlobalError(Error(EGL_SUCCESS));
    return 0;
}

EGLBoolean EGLAPIENTRY WaitGL(void)
{
    EVENT("()");

    UNIMPLEMENTED();   // FIXME

    SetGlobalError(Error(EGL_SUCCESS));
    return 0;
}

EGLBoolean EGLAPIENTRY WaitNative(EGLint engine)
{
    EVENT("(EGLint engine = %d)", engine);

    UNIMPLEMENTED();   // FIXME

    SetGlobalError(Error(EGL_SUCCESS));
    return 0;
}

EGLBoolean EGLAPIENTRY SwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p)", dpy, surface);

    Display *display = static_cast<Display*>(dpy);
    Surface *eglSurface = (Surface*)surface;

    if (!ValidateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (display->getRenderer()->isDeviceLost())
    {
        SetGlobalError(Error(EGL_CONTEXT_LOST));
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE)
    {
        SetGlobalError(Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    Error error = eglSurface->swap();
    if (error.isError())
    {
        SetGlobalError(error);
        return EGL_FALSE;
    }

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY CopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLNativePixmapType target = 0x%0.8p)", dpy, surface, target);

    Display *display = static_cast<Display*>(dpy);
    Surface *eglSurface = static_cast<Surface*>(surface);

    if (!ValidateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (display->getRenderer()->isDeviceLost())
    {
        SetGlobalError(Error(EGL_CONTEXT_LOST));
        return EGL_FALSE;
    }

    UNIMPLEMENTED();   // FIXME

    SetGlobalError(Error(EGL_SUCCESS));
    return 0;
}

// EGL 1.1
EGLBoolean EGLAPIENTRY BindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint buffer = %d)", dpy, surface, buffer);

    Display *display = static_cast<Display*>(dpy);
    Surface *eglSurface = static_cast<Surface*>(surface);

    if (!ValidateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (buffer != EGL_BACK_BUFFER)
    {
        SetGlobalError(Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE || eglSurface->getWindowHandle())
    {
        SetGlobalError(Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    if (eglSurface->getBoundTexture())
    {
        SetGlobalError(Error(EGL_BAD_ACCESS));
        return EGL_FALSE;
    }

    if (eglSurface->getTextureFormat() == EGL_NO_TEXTURE)
    {
        SetGlobalError(Error(EGL_BAD_MATCH));
        return EGL_FALSE;
    }

    gl::Context *context = GetGlobalContext();
    if (context)
    {
        gl::Texture2D *textureObject = context->getTexture2D();
        ASSERT(textureObject != NULL);

        if (textureObject->isImmutable())
        {
            SetGlobalError(Error(EGL_BAD_MATCH));
            return EGL_FALSE;
        }

        eglSurface->bindTexImage(textureObject, buffer);
    }

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY SurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint attribute = %d, EGLint value = %d)",
        dpy, surface, attribute, value);

    Display *display = static_cast<Display*>(dpy);
    Surface *eglSurface = static_cast<Surface*>(surface);

    if (!ValidateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    UNIMPLEMENTED();   // FIXME

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY ReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint buffer = %d)", dpy, surface, buffer);

    Display *display = static_cast<Display*>(dpy);
    Surface *eglSurface = static_cast<Surface*>(surface);

    if (!ValidateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (buffer != EGL_BACK_BUFFER)
    {
        SetGlobalError(Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE || eglSurface->getWindowHandle())
    {
        SetGlobalError(Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    if (eglSurface->getTextureFormat() == EGL_NO_TEXTURE)
    {
        SetGlobalError(Error(EGL_BAD_MATCH));
        return EGL_FALSE;
    }

    gl::Texture2D *texture = eglSurface->getBoundTexture();

    if (texture)
    {
        eglSurface->releaseTexImage(buffer);
    }

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY SwapInterval(EGLDisplay dpy, EGLint interval)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLint interval = %d)", dpy, interval);

    Display *display = static_cast<Display*>(dpy);

    if (!ValidateDisplay(display))
    {
        return EGL_FALSE;
    }

    Surface *draw_surface = static_cast<Surface*>(GetGlobalDrawSurface());

    if (draw_surface == NULL)
    {
        SetGlobalError(Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    draw_surface->setSwapInterval(interval);

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}


// EGL 1.2
EGLBoolean EGLAPIENTRY BindAPI(EGLenum api)
{
    EVENT("(EGLenum api = 0x%X)", api);

    switch (api)
    {
      case EGL_OPENGL_API:
      case EGL_OPENVG_API:
        SetGlobalError(Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;   // Not supported by this implementation
      case EGL_OPENGL_ES_API:
        break;
      default:
        SetGlobalError(Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    SetGlobalAPI(api);

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLenum EGLAPIENTRY QueryAPI(void)
{
    EVENT("()");

    EGLenum API = GetGlobalAPI();

    SetGlobalError(Error(EGL_SUCCESS));
    return API;
}

EGLSurface EGLAPIENTRY CreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLenum buftype = 0x%X, EGLClientBuffer buffer = 0x%0.8p, "
          "EGLConfig config = 0x%0.8p, const EGLint *attrib_list = 0x%0.8p)",
          dpy, buftype, buffer, config, attrib_list);

    Display *display = static_cast<Display*>(dpy);

    if (!ValidateConfig(display, config))
    {
        return EGL_NO_SURFACE;
    }

    if (buftype != EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE || !buffer)
    {
        SetGlobalError(Error(EGL_BAD_PARAMETER));
        return EGL_NO_SURFACE;
    }

    EGLSurface surface = EGL_NO_SURFACE;
    Error error = display->createOffscreenSurface(config, buffer, attrib_list, &surface);
    if (error.isError())
    {
        SetGlobalError(error);
        return EGL_NO_SURFACE;
    }

    return surface;
}

EGLBoolean EGLAPIENTRY ReleaseThread(void)
{
    EVENT("()");

    MakeCurrent(EGL_NO_DISPLAY, EGL_NO_CONTEXT, EGL_NO_SURFACE, EGL_NO_SURFACE);

    SetGlobalError(Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY WaitClient(void)
{
    EVENT("()");

    UNIMPLEMENTED();   // FIXME

    SetGlobalError(Error(EGL_SUCCESS));
    return 0;
}

// EGL 1.4
EGLContext EGLAPIENTRY GetCurrentContext(void)
{
    EVENT("()");

    gl::Context *context = GetGlobalContext();

    SetGlobalError(Error(EGL_SUCCESS));
    return static_cast<EGLContext>(context);
}

// EGL 1.5
EGLSync EGLAPIENTRY CreateSync(EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLenum type = 0x%X, const EGLint* attrib_list = 0x%0.8p)", dpy, type, attrib_list);

    UNIMPLEMENTED();
    return EGL_NO_SYNC;
}

EGLBoolean EGLAPIENTRY DestroySync(EGLDisplay dpy, EGLSync sync)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSync sync = 0x%0.8p)", dpy, sync);

    UNIMPLEMENTED();
    return EGL_FALSE;
}

EGLint EGLAPIENTRY ClientWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSync sync = 0x%0.8p, EGLint flags = 0x%X, EGLTime timeout = %d)", dpy, sync, flags, timeout);

    UNIMPLEMENTED();
    return 0;
}

EGLBoolean EGLAPIENTRY GetSyncAttrib(EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSync sync = 0x%0.8p, EGLint attribute = 0x%X, EGLAttrib *value = 0x%0.8p)", dpy, sync, attribute, value);

    UNIMPLEMENTED();
    return EGL_FALSE;
}

EGLDisplay EGLAPIENTRY GetPlatformDisplay(EGLenum platform, void *native_display, const EGLAttrib *attrib_list)
{
    EVENT("(EGLenum platform = %d, void* native_display = 0x%0.8p, const EGLint* attrib_list = 0x%0.8p)",
          platform, native_display, attrib_list);

    UNIMPLEMENTED();
    return EGL_NO_DISPLAY;
}

EGLSurface EGLAPIENTRY CreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, void* native_window = 0x%0.8p, const EGLint* attrib_list = 0x%0.8p)",
          dpy, config, native_window, attrib_list);

    UNIMPLEMENTED();
    return EGL_NO_SURFACE;
}

EGLSurface EGLAPIENTRY CreatePlatformPixmapSurface(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, void* native_pixmap = 0x%0.8p, const EGLint* attrib_list = 0x%0.8p)",
          dpy, config, native_pixmap, attrib_list);

    UNIMPLEMENTED();
    return EGL_NO_SURFACE;
}

EGLBoolean EGLAPIENTRY WaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSync sync = 0x%0.8p, EGLint flags = 0x%X)", dpy, sync, flags);

    UNIMPLEMENTED();
    return EGL_FALSE;
}

__eglMustCastToProperFunctionPointerType EGLAPIENTRY GetProcAddress(const char *procname)
{
    EVENT("(const char *procname = \"%s\")", procname);

    struct Extension
    {
        const char *name;
        __eglMustCastToProperFunctionPointerType address;
    };

    static const Extension extensions[] =
    {
        { "eglQuerySurfacePointerANGLE", (__eglMustCastToProperFunctionPointerType)QuerySurfacePointerANGLE },
        { "eglPostSubBufferNV", (__eglMustCastToProperFunctionPointerType)PostSubBufferNV },
        { "eglGetPlatformDisplayEXT", (__eglMustCastToProperFunctionPointerType)GetPlatformDisplayEXT },
        { "glBlitFramebufferANGLE", (__eglMustCastToProperFunctionPointerType)gl::BlitFramebufferANGLE },
        { "glRenderbufferStorageMultisampleANGLE", (__eglMustCastToProperFunctionPointerType)gl::RenderbufferStorageMultisampleANGLE },
        { "glDeleteFencesNV", (__eglMustCastToProperFunctionPointerType)gl::DeleteFencesNV },
        { "glGenFencesNV", (__eglMustCastToProperFunctionPointerType)gl::GenFencesNV },
        { "glIsFenceNV", (__eglMustCastToProperFunctionPointerType)gl::IsFenceNV },
        { "glTestFenceNV", (__eglMustCastToProperFunctionPointerType)gl::TestFenceNV },
        { "glGetFenceivNV", (__eglMustCastToProperFunctionPointerType)gl::GetFenceivNV },
        { "glFinishFenceNV", (__eglMustCastToProperFunctionPointerType)gl::FinishFenceNV },
        { "glSetFenceNV", (__eglMustCastToProperFunctionPointerType)gl::SetFenceNV },
        { "glGetTranslatedShaderSourceANGLE", (__eglMustCastToProperFunctionPointerType)gl::GetTranslatedShaderSourceANGLE },
        { "glTexStorage2DEXT", (__eglMustCastToProperFunctionPointerType)gl::TexStorage2DEXT },
        { "glGetGraphicsResetStatusEXT", (__eglMustCastToProperFunctionPointerType)gl::GetGraphicsResetStatusEXT },
        { "glReadnPixelsEXT", (__eglMustCastToProperFunctionPointerType)gl::ReadnPixelsEXT },
        { "glGetnUniformfvEXT", (__eglMustCastToProperFunctionPointerType)gl::GetnUniformfvEXT },
        { "glGetnUniformivEXT", (__eglMustCastToProperFunctionPointerType)gl::GetnUniformivEXT },
        { "glGenQueriesEXT", (__eglMustCastToProperFunctionPointerType)gl::GenQueriesEXT },
        { "glDeleteQueriesEXT", (__eglMustCastToProperFunctionPointerType)gl::DeleteQueriesEXT },
        { "glIsQueryEXT", (__eglMustCastToProperFunctionPointerType)gl::IsQueryEXT },
        { "glBeginQueryEXT", (__eglMustCastToProperFunctionPointerType)gl::BeginQueryEXT },
        { "glEndQueryEXT", (__eglMustCastToProperFunctionPointerType)gl::EndQueryEXT },
        { "glGetQueryivEXT", (__eglMustCastToProperFunctionPointerType)gl::GetQueryivEXT },
        { "glGetQueryObjectuivEXT", (__eglMustCastToProperFunctionPointerType)gl::GetQueryObjectuivEXT },
        { "glDrawBuffersEXT", (__eglMustCastToProperFunctionPointerType)gl::DrawBuffersEXT },
        { "glVertexAttribDivisorANGLE", (__eglMustCastToProperFunctionPointerType)gl::VertexAttribDivisorANGLE },
        { "glDrawArraysInstancedANGLE", (__eglMustCastToProperFunctionPointerType)gl::DrawArraysInstancedANGLE },
        { "glDrawElementsInstancedANGLE", (__eglMustCastToProperFunctionPointerType)gl::DrawElementsInstancedANGLE },
        { "glGetProgramBinaryOES", (__eglMustCastToProperFunctionPointerType)gl::GetProgramBinaryOES },
        { "glProgramBinaryOES", (__eglMustCastToProperFunctionPointerType)gl::ProgramBinaryOES },
        { "glGetBufferPointervOES", (__eglMustCastToProperFunctionPointerType)gl::GetBufferPointervOES },
        { "glMapBufferOES", (__eglMustCastToProperFunctionPointerType)gl::MapBufferOES },
        { "glUnmapBufferOES", (__eglMustCastToProperFunctionPointerType)gl::UnmapBufferOES },
        { "glMapBufferRangeEXT", (__eglMustCastToProperFunctionPointerType)gl::MapBufferRangeEXT },
        { "glFlushMappedBufferRangeEXT", (__eglMustCastToProperFunctionPointerType)gl::FlushMappedBufferRangeEXT },
        { "", NULL },
    };

    for (const Extension *extension = &extensions[0]; extension->address != nullptr; extension++)
    {
        if (strcmp(procname, extension->name) == 0)
        {
            return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(extension->address);
        }
    }

    return NULL;
}

}
