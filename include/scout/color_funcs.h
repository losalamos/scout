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

#ifndef SCOUT_COLOR_FUNCS_H_
#define SCOUT_COLOR_FUNCS_H_

#include "scout/Runtime/types.h"

  inline float red(const scout::float4& color)
  { return color.components[0]; }

  inline float green(const scout::float4& color)
  { return color.components[1]; }

  inline float blue(const scout::float4& color)
  { return color.components[2]; }

  inline float alpha(const scout::float4& color)
  { return color.components[3]; }

}


#endif


  
  




  

