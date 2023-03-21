#ifndef __RENDER_H__
#define __RENDER_H__


#ifdef FIXED_RENDER

#else // #ifdef FIXED_RENDER

#include "../cvar.h"
#include "original/render.h"
#include "original/model.h"
#include "original/r_shared.h"
#include "original/d_local.h"
#include "original/d_iface.h"

#ifdef GLQUAKE
#include "gl_model.h"
#endif

#endif // #ifdef FIXED_RENDER

#endif // #ifndef __RENDER_H__
