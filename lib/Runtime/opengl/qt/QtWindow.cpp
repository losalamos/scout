/*
 * ###########################################################################
 * Copyright (c) 2015, Los Alamos National Security, LLC.
 * All rights reserved.
 *
 *  Copyright 2015. Los Alamos National Security, LLC. This software was
 *  produced under U.S. Government contract DE-AC52-06NA25396 for Los
 *  Alamos National Laboratory (LANL), which is operated by Los Alamos
 *  National Security, LLC for the U.S. Department of Energy. The
 *  U.S. Government has rights to use, reproduce, and distribute this
 *  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY,
 *  LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY
 *  FOR THE USE OF THIS SOFTWARE.  If software is modified to produce
 *  derivative works, such modified software should be clearly marked,
 *  so as not to confuse it with the version available from LANL.
 *
 *  Additionally, redistribution and use in source and binary forms,
 *  with or without modification, are permitted provided that the
 *  following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Los Alamos National Security, LLC, Los
 *      Alamos National Laboratory, LANL, the U.S. Government, nor the
 *      names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior
 *      written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
 *  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 *  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 * ###########################################################################
 *
 * Notes
 *
 * #####
 */

#include "scout/Runtime/opengl/qt/QtWindow.h"

#include <iostream>
#include <mutex>

#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>

#include <QApplication>

#include "scout/Runtime/opengl/glRenderable.h"

using namespace std;
using namespace scout;

namespace{

  class Global{
  public:
    Global(){
      argc_ = 1;
      argv_[0] = strdup("scout");
      app_ = new QApplication(argc_, argv_);

      QSurfaceFormat format;
      format.setProfile(QSurfaceFormat::CompatibilityProfile);
      format.setVersion(4, 1);
      //format.setSamples(16);
      //format.setDepthBufferSize(24);
      //format.setStencilBufferSize(8);
      //format.setAlphaBufferSize(8);
      //format.setRedBufferSize(8);
      //format.setGreenBufferSize(8);
      //format.setBlueBufferSize(8);
      //format.setDepthBufferSize(24);
      //format.setSamples(4);

      QSurfaceFormat::setDefaultFormat(format);
    }

    void pollEvents(){
      app_->processEvents(QEventLoop::AllEvents, 1);
    }

  private:
    int argc_;
    char* argv_[1];
    QApplication* app_;
  };
  
  Global* _global = 0;
  mutex _mutex;

} // end namespace

QtWindow::QtWindow(unsigned short width, unsigned short height, QWindow* parent)
  : QWindow(parent),
    context_(0),
    device_(0),
    currentRenderable_(0){

  setSurfaceType(QWindow::OpenGLSurface);

  QSurfaceFormat format;
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setVersion(4, 1);
  format.setSamples(16);
  setFormat(format);

  resize(width, height);
}

QtWindow::~QtWindow(){}

void QtWindow::init(){
  _mutex.lock();
  if(!_global){
    _global = new Global;
  }
  _mutex.unlock();
}

void QtWindow::pollEvents(){
  _mutex.lock();
  _global->pollEvents();
  _mutex.unlock();
}

void QtWindow::makeContextCurrent(){
  if(!context_){
    context_ = new QOpenGLContext(this);
    context_->setFormat(requestedFormat());
    context_->create();
  }

  context_->makeCurrent(this);

  if(!device_){
    device_ = new QOpenGLPaintDevice;
  }

  device_->setSize(size());

  glViewport(0, 0, width(), height());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void QtWindow::paint(){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  for(glRenderable* r : renderables_) {
    r->render(NULL);
  }
}

void QtWindow::swapBuffers(){
  context_->swapBuffers(this);
}
