/*
 * ###########################################################################
 * Copyright (c) 2010, Los Alamos National Security, LLC.
 * All rights reserved.
 *
 *  Copyright 2010. Los Alamos National Security, LLC. This software was
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

#include "scout/Runtime/cpu/tbq.h"

using namespace std;
using namespace scout;

#include <vector>
#include <cmath>
#include <iostream>
#include <cassert>
#include <cstring>
#include <map>

#include "scout/Runtime/cpu/CpuUtilities.h"

using namespace std;
using namespace scout;

namespace{

struct BlockLiteral{
  void* isa;
  int flags;
  int reserved;
  void (*invoke)(void*, ...);
  struct BlockDescriptor{
    unsigned long int reserved;
    unsigned long int size;
    void (*copy_helper)(void* dst, void* src);
    void (*dispose_helper)(void* src);
    const char* signature;
  }*descriptor;

  // some of these fields may not actually be present depending
  // on the mesh dimensions
  uint32_t* xStart;
  uint32_t* xEnd;
  uint32_t* yStart;
  uint32_t* yEnd;
  uint32_t* zStart;
  uint32_t* zEnd;

  // ... void* captured fields
};

void* _runThread(void* t);

class Thread{
public:
  Thread(){

  }

  virtual ~Thread(){

  }

  void start(){
    pthread_create(&thread_, 0, _runThread, (void*)this);
  }

  void stop() {
    pthread_cancel(thread_);
  }

  virtual void run() = 0;

  void await(){
    pthread_join(thread_, 0);
  }

private:
  pthread_t thread_;
};

class Mutex{
public:
  Mutex(){
    pthread_mutex_init(&mutex_, 0);
  }

  ~Mutex(){
    pthread_mutex_destroy(&mutex_);
  }

  void lock(){
    pthread_mutex_lock(&mutex_);
  }

  bool tryLock(){
    return pthread_mutex_trylock(&mutex_) == 0;
  }

  void unlock(){
    pthread_mutex_unlock(&mutex_);
  }

  pthread_mutex_t& mutex(){
    return mutex_;
  }

private:
  pthread_mutex_t mutex_;
};

void* _runThread(void* t){
  Thread* thread = static_cast<Thread*>(t);
  thread->run();
  return 0;
};

class Condition{
public:
  Condition(Mutex& mutex)
  : mutex_(mutex){
    pthread_cond_init(&condition_, 0);
  }

  ~Condition(){
    pthread_cond_destroy(&condition_);
  }

  void await(){
    pthread_cond_wait(&condition_, &mutex_.mutex());
  }

  void signal(){
    pthread_cond_signal(&condition_);
  }

  void broadcast(){
    pthread_cond_broadcast(&condition_);
  }

  pthread_cond_t& condition(){
    return condition_;
  }

private:
  Mutex& mutex_;
  pthread_cond_t condition_;
};

class VSem{
public:
  VSem(int count)
  : count_(count),
    maxCount_(0),
    condition_(mutex_){

  }

  VSem(int count, int maxCount)
  : count_(count),
    maxCount_(maxCount),
    condition_(mutex_){

  }

  void acquire(){
    mutex_.lock();
    while(count_ <= 0){
      condition_.await();
    }
    --count_;
    mutex_.unlock();
  }

  bool tryAcquire(){
    mutex_.lock();
    if(count_ > 0){
      --count_;
      mutex_.unlock();
      return true;
    }
    mutex_.unlock();
    return false;
  }

  void release(){
    mutex_.lock();
    if(maxCount_ == 0 || count_ < maxCount_){
      ++count_;
    }
    condition_.signal();
    mutex_.unlock();
  }

private:
  Mutex mutex_;
  Condition condition_;
  int count_;
  int maxCount_;
};

struct Item{
  void* blockLiteral;
  uint32_t dimensions;
  uint32_t xStart;
  uint32_t xEnd;
  uint32_t yStart;
  uint32_t yEnd;
  uint32_t zStart;
  uint32_t zEnd;
};

class Queue{
public:
  Queue()
  : i_(0){

  }

  void reset(){
    queue_.clear();
    i_ = 0;
  }

  void add(Item* item){
    queue_.push_back(item);
  }

  Item* get(){
    mutex_.lock();

    if(i_ >= queue_.size()){
      mutex_.unlock();
      return 0;
    }

    Item* item = queue_[i_++];
    mutex_.unlock();

    return item;
  }

private:
  typedef vector<Item*> Queue_;

  Mutex mutex_;
  Queue_ queue_;
  size_t i_;
};

typedef vector<Queue*> QueueVec;

class MeshThread : public Thread{
public:
  MeshThread()
  : beginSem_(0),
    finishSem_(0),
    queue_(0){

  }

  void begin(Queue* queue){
    queue_ = queue;
    beginSem_.release();
  }

  void finish(){
    finishSem_.acquire();
  }

  void run(){
    Item* item;
    BlockLiteral* bl;

    for(;;){
      beginSem_.acquire();

      for(;;){
        item = queue_->get();

        if(!item){
          break;
        }

        bl = (BlockLiteral*)item->blockLiteral;

        switch(item->dimensions){
        case 3:
          bl->zStart = new uint32_t(item->zStart);
          bl->zEnd = new uint32_t(item->zEnd);
        case 2:
          bl->yStart = new uint32_t(item->yStart);
          bl->yEnd = new uint32_t(item->yEnd);
        case 1:
          bl->xStart = new uint32_t(item->xStart);
          bl->xEnd = new uint32_t(item->xEnd);
        }

        bl->invoke(bl);

        switch (item->dimensions) {
        case 3:
          delete bl->zStart;
          delete bl->zEnd;
        case 2:
          delete bl->yStart;
          delete bl->yEnd;
        case 1:
          delete bl->xStart;
          delete bl->xEnd;
        }
        free(item->blockLiteral);
        delete item;
      }

      finishSem_.release();
    }
  }

private:
  Queue* queue_;
  VSem beginSem_;
  VSem finishSem_;
};

typedef vector<MeshThread*> ThreadVec;

} // end namespace

namespace scout{

class tbq_rt_{
public:
  tbq_rt_(tbq_rt* o)
  : o_(o){

    system_rt sysinfo;

    size_t n = sysinfo.totalProcessingUnits();

    for(size_t i = 0; i < n; ++i){
      MeshThread* ti = new MeshThread;
      ti->start();
      threadVec_.push_back(ti);
    }

    queue_ = new Queue;
  }

  ~tbq_rt_(){
    size_t n = threadVec_.size();

    for(size_t i = 0; i < n; ++i){
      threadVec_[i]->stop();
    }

    for(size_t i = 0; i < n; ++i){
      threadVec_[i]->await();
      delete threadVec_[i];
    }

    delete queue_;
  }

  void* createSubBlock(BlockLiteral* bl,
      size_t numDimensions,
      size_t numFields){

    void* bp = malloc(sizeof(BlockLiteral) 
        + (numFields+6)*sizeof(void*));

    if (bp == NULL) {
      cout << "bad malloc"<< endl;
      return NULL;
    }
    BlockLiteral* b = (BlockLiteral*)bp;
    b->isa = bl->isa;
    b->flags = bl->flags;
    b->reserved = bl->reserved;
    b->invoke = bl->invoke;
    b->descriptor = bl->descriptor;

    size_t offset = bl->descriptor->size + 2*numDimensions*sizeof(void*);

    memcpy((char*)bp + offset, (char*)bl + offset,
        (numFields - 2*numDimensions)*sizeof(void*));

    return bp;
  }

  Item* createItem(BlockLiteral* bl, int numDimensions, size_t start, size_t end) {
    size_t x, y;
    Item* item = new Item;
    item->dimensions = numDimensions;

    switch (numDimensions) {
    case 1:
      item->xStart = start;
      item->xEnd = end;
      break;
    case 2:
      x = (*bl->xEnd - *bl->xStart);
      item->xStart = start % x;
      item->xEnd = end % x;
      item->yStart = start / x;
      item->yEnd = end / x;
      break;
    case 3:
      x = (*bl->xEnd - *bl->xStart);
      y = (*bl->yEnd - *bl->yStart);
      item->xStart = (start % (x * y)) % x;
      item->xEnd = (end % (x * y)) % x;
      item->yStart = (start % (x * y)) / x;
      item->yEnd = end % (x * y) / x;
      item->zStart = start / (x * y);
      item->zEnd = end / (x * y);
      break;
    }
    return item;
  }

  size_t findExtent(BlockLiteral* bl, int numDimensions) {
    size_t extent = 1;
    switch (numDimensions) {
    case 3:
      extent *= (*bl->zEnd - *bl->zStart);
    case 2:
      extent *= (*bl->yEnd - *bl->yStart);
    case 1:
      extent *= (*bl->xEnd - *bl->xStart);
    }
    return extent;
  }

  void queueBlocks(void* blockLiteral, int numDimensions, int numFields) {
    BlockLiteral* bl = (BlockLiteral*) blockLiteral;
    size_t extent, chunk, end;

    extent = findExtent(bl, numDimensions);
    chunk = extent / (threadVec_.size() * 2);

    for (size_t i = 0; i < extent; i += chunk) {
      end = i + chunk;

      if (end > extent) {
        end = extent;
      }

      Item* item = createItem(bl, numDimensions, i, end);
      item->blockLiteral = createSubBlock(bl, numDimensions,
          numFields);

      queue_->add(item);
    }
  }


  void run(void* blockLiteral, int numDimensions, int numFields){
    queueBlocks(blockLiteral, numDimensions, numFields);

    size_t n = threadVec_.size();

    for(size_t i = 0; i < n; ++i){
      threadVec_[i]->begin(queue_);
    }

    // run ...

    for(size_t i = 0; i < n; ++i){
      threadVec_[i]->finish();
    }
    queue_->reset();
  }

private:
  typedef map<void*, Queue*> QueueMap_;

  tbq_rt* o_;
  Queue *queue_;
  ThreadVec threadVec_;
};

} // end namespace scout

tbq_rt::tbq_rt(){
  x_ = new tbq_rt_(this);
}

tbq_rt::~tbq_rt(){
  delete x_;
}

void tbq_rt::run(void* blockLiteral, int numDimensions, int numFields){
  x_->run(blockLiteral, numDimensions, numFields);
}