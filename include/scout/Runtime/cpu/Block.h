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
 * Notes:
 * This is the BlockLiteral struct that is built by the Codegen in CGBlocks.cpp
 *
 * #####
 */

#ifndef __SC_CPU_BLOCK_H__
#define __SC_CPU_BLOCK_H__

#include <stdint.h>

namespace scout {
  namespace cpu {

    struct BlockDescriptor {
      unsigned long int reserved;
      unsigned long int size;
      void (*copy_helper) (void *dst, void *src);
      void (*dispose_helper) (void *src);
      const char *signature;
    };

    struct BlockLiteral {
      void *isa;
      int flags;
      int reserved;
      void (*invoke) (void *, ...);
      struct BlockDescriptor *descriptor;

      // some of these fields may not actually be present depending
      // on the mesh dimensions
      uint32_t *xStart;
      uint32_t *xEnd;
      uint32_t *yStart;
      uint32_t *yEnd;
      uint32_t *zStart;
      uint32_t *zEnd;

      // ... void* captured fields
    };

    // find total extent (product of dimensions)
    int findExtent(BlockLiteral *bl, int numDimensions);

    // Blocks are what we insert into the queues
    // It is a blockLiteral + the number of dimensions and fields.
    class Block {
    public:
      Block(BlockLiteral * bl, int numDimensions, int numFields, int start,
          int end);
      ~Block();
      BlockLiteral* getBlockLiteral();
      uint32_t getNDimensions() { return nDimensions_;}
      uint32_t getNFields() {return nFields_;}
    private:
      BlockLiteral *blockLiteral_;
      uint32_t nDimensions_;
      uint32_t nFields_;
    };
  }
}
#endif // __SC_CPU_BLOCK_H__