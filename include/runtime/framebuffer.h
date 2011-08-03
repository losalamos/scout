/*
 * -----  Scout Programming Language -----
 *
 * This file is distributed under an open source license by Los Alamos
 * National Security, LCC.  See the file License.txt (located in the
 * top level of the source distribution) for details.
 * 
 *-----
 * 
 */

#ifndef SCOUT_FRAMEBUFFER_H_
#define SCOUT_FRAMEBUFFER_H_

#include "scout/types.h"

namespace scout
{
  struct framebuffer_t {
    
    framebuffer_t(dim_t w, dim_t h, float r=0.0f, float b=0.0f, float g=0.0f, float a=1.0f);
    ~framebuffer_t();

    void clear();
    
    dim_t     width, height;
    float4    bg_color;
    float4   *pixels;
  };

}

#ifdef SC_ENABLE_PNG
bool extern save_framebuffer_as_png(const scout::framebuffer_t* fb, const char *filename);
#endif

#endif



