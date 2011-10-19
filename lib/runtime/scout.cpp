#include <cmath>
#include <iostream>
#include <sstream>

#ifdef __APPLE__

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include "runtime/init_mac.h"

#else

#include <GL/gl.h>
#include <GL/glu.h>

#endif

#include <SDL/SDL.h>

#define SC_USE_PNG

#include "runtime/opengl/uniform_renderall.h"
#include "runtime/tbq.h"

using namespace std;
using namespace scout;

SDL_Surface* _sdl_surface;
uniform_renderall_t* _uniform_renderall = 0;
float4* _pixels;
tbq_rt* _tbq;

static const size_t WINDOW_WIDTH = 1024;
static const size_t WINDOW_HEIGHT = 1024;

void scoutInit(int& argc, char** argv){
  cout << "t1" << endl;
  
  _tbq = new tbq_rt;
}

void scoutInit(){
  _tbq = new tbq_rt;
}

void scoutBeginRenderAll(size_t dx, size_t dy, size_t dz){
  // if this is our first render all, then initialize SDL and
  // the OpenGL runtime
  if(!_uniform_renderall){

#ifdef __APPLE__
    scoutInitMac();
#endif

    if(SDL_Init(SDL_INIT_VIDEO) < 0){
      cerr << "Error: failed to initialize SDL." << endl;
      exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);

    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

    _sdl_surface = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32,
				    SDL_HWSURFACE |
				    SDL_GL_DOUBLEBUFFER |
				    SDL_OPENGL);

    if(!_sdl_surface){
      cerr << "Error: failed to initialize SDL surface." << endl;
      exit(1);
    }

    //glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glClearColor(0.5, 0.55, 0.65, 0.0);

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    if(dy == 0){
      gluOrtho2D(0, dx, 0, dx);
      _uniform_renderall = __sc_init_uniform_renderall(dx);
    }
    else{
      gluOrtho2D(0, dx, 0, dy);
      _uniform_renderall = __sc_init_uniform_renderall(dx, dy);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapBuffers();
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  _pixels = __sc_map_uniform_colors(_uniform_renderall);
}

void scoutEndRenderAll(){
  /*
  float* px = (float*)_pixels;

  for(size_t i = 0; i < 512; ++i){
    cout << "px[0]: " << px[i*4] << endl;
    cout << "px[1]: " << px[i*4 + 1] << endl;
    cout << "px[2]: " << px[i*4 + 2] << endl;
    cout << "px[3]: " << px[i*4 + 3] << endl;
    
    cout << "-----------------------" << endl;
  }
  */

  __sc_unmap_uniform_colors(_uniform_renderall);
  __sc_exec_uniform_renderall(_uniform_renderall);
  SDL_GL_SwapBuffers();
}

void scoutEnd(){
  __sc_destroy_uniform_renderall(_uniform_renderall);
}

double cshift(double a, int dx, int axis){
  return 0.0;
}

float cshift(float a, int dx, int axis){
  return 0.0;
}

int cshift(int a, int dx, int axis){
  return 0.0;
}
