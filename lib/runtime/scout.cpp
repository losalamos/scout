#include <iostream>
#include <sstream>

#define SC_USE_PNG

#include "runtime/framebuffer.h"
#include "runtime/renderall.h"
#include "runtime/viewport.h"
#include "runtime/tbq.h"

using namespace std;
using namespace scout;

framebuffer_rt* _framebuffer;
tbq_rt* _tbq;
size_t _frame = 0;

static framebuffer_rt* _outFramebuffer;
static viewport_rt* _viewport; 

static const size_t IN_WIDTH = 1024;
static const size_t IN_HEIGHT = 1;

static const size_t OUT_WIDTH = 1024;
static const size_t OUT_HEIGHT = 100;

void scoutInit(int argc, char** argv){
  _framebuffer = new framebuffer_rt(IN_WIDTH, IN_HEIGHT);
  _outFramebuffer = new framebuffer_rt(OUT_WIDTH, OUT_HEIGHT);
  _viewport = new viewport_rt(0, 0, OUT_WIDTH, OUT_HEIGHT);
  _tbq = new tbq_rt;
}

void scoutSwapBuffers(){
  mapToFrameBuffer(_framebuffer->pixels,
		   IN_WIDTH,
		   IN_HEIGHT,
		   *_outFramebuffer,
		   *_viewport,
		   FILTER_NEAREST /*FILTER_LINEAR*/);

  stringstream sstr;
  sstr << "scout-" << _frame << ".png";

  save_framebuffer_as_png(_outFramebuffer, sstr.str().c_str());
  _framebuffer->clear();
  
  ++_frame;
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

float4 hsv(float h, float s, float v){
  float4 no_op;
  return no_op;
}
