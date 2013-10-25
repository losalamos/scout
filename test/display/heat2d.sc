/*
<<<<<<< HEAD
 * -----  Scout Programming Language -----
 *
 * This file is distributed under an open source license by Los Alamos
 * National Security, LCC.  See the file License.txt (located in the
 * top level of the source distribution) for details.
 *
 *-----
 *
 * Simplistic 2D heat transfer...
 *
 */
=======
 * ###########################################################################
 * Copyright (c) 2013, Los Alamos National Security, LLC.
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
 * Simplistic 2D heat transfer...
 *
 * ##### 
 */ 
>>>>>>> 331f45ad55fb625f198d765bff49b3d4fc0a6ce5

int main(int argc, char *argv[])
{
  const int NTIME_STEPS     = 100;
  const float MAX_TEMP      = 100.0f;

  uniform mesh HeatMeshType{
  cells:
    float t1, t2;
  };

  HeatMeshType heat_mesh[512, 512];
  
<<<<<<< HEAD
  forall cells c of heat_mesh {
=======
  forall cells c in heat_mesh {
>>>>>>> 331f45ad55fb625f198d765bff49b3d4fc0a6ce5
    t1 = MAX_TEMP;
    t2 = MAX_TEMP;

    if (c.position.x == 0 || c.position.x == (heat_mesh.width-1)) {
      t1 = 0.0;
      t2 = 0.0;
    }

    if (c.position.y == 0 || c.position.y == (heat_mesh.height-1)) {
      t1 = 0.0;
      t2 = 0.0;
    }
  }

  const float dx    = 1.0f / heat_mesh.width;
  const float dy    = 1.0f / heat_mesh.height;
  const float alpha = 0.0000039f;
  const float dt    = 0.4 * (alpha / 4.0f) * ((1.0f / (dx * dx)) + (1.0f / (dy * dy)));

  // Time steps loop.
  for(unsigned int t = 0; t < NTIME_STEPS; ++t) {

<<<<<<< HEAD
    forall cells c of heat_mesh {
=======
    forall cells c in heat_mesh {
>>>>>>> 331f45ad55fb625f198d765bff49b3d4fc0a6ce5

      if (c.position.x > 0 && c.position.x < heat_mesh.width-1 &&
          c.position.y > 0 && c.position.y < heat_mesh.height-1) {

        float d2dx2 = cshift(c.t1, 1, 0) - 2.0f * c.t1 + cshift(c.t1, -1, 0);
        d2dx2 /= dx * dx;

        float d2dy2 = cshift(c.t1, 0, 1) - 2.0f * c.t1 + cshift(c.t1, 0, -1);
        d2dy2 /= dy * dy;

        t2 = (alpha * dt * (d2dx2 + d2dy2)) + c.t1;
      }
    }

<<<<<<< HEAD
    forall cells c of heat_mesh {
      t1 = t2;
    }

    renderall cells c of heat_mesh {
=======
    forall cells c in heat_mesh {
      t1 = t2;
    }

    renderall cells c in heat_mesh {
>>>>>>> 331f45ad55fb625f198d765bff49b3d4fc0a6ce5
      float norm_t1 = t1 / MAX_TEMP;
      float hue = 240.0f - 240.0f * norm_t1;
      color = hsv(hue, 1.0f, 1.0f);
    }
  }

  return 0;
}
