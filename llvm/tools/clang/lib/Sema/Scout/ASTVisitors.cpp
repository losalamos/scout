/*
 * ###########################################################################
 * Copyright (c) 2014, Los Alamos National Security, LLC.
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

#include "clang/Sema/Scout/ASTVisitors.h"

using namespace clang;
using namespace clang::sema;

namespace clang {
  namespace sema {

    bool isPosition(unsigned id) {
      if (id == Builtin::BIposition || id == Builtin::BIpositionx
          || id == Builtin::BIpositiony || id == Builtin::BIpositionz
          || id == Builtin::BIpositionw) return true;
      return false;
    }

    bool isMPosition(unsigned id) {
      if (id == Builtin::BImposition || id == Builtin::BImpositionx
          || id == Builtin::BImpositiony || id == Builtin::BImpositionz) return true;
      return false;
    }

    bool isCShift(unsigned id) {
      if (id == Builtin::BIcshift || id == Builtin::BIcshifti
          || id == Builtin::BIcshiftf || id == Builtin::BIcshiftd ) return true;
      return false;
    }

    //check if builtin id is an eoshift
    bool isEOShift(unsigned id) {
      if (id == Builtin::BIeoshift || id == Builtin::BIeoshifti
          || id == Builtin::BIeoshiftf || id == Builtin::BIeoshiftd ) return true;
      return false;
    }

    bool CheckShift(unsigned id, CallExpr *E, Sema &S) {
      bool error = false;
      unsigned kind = 0;
      if (isCShift(id)) kind = ShiftKind::CShift;
      if (isEOShift(id)) kind = ShiftKind::EOShift;

      unsigned args = E->getNumArgs();

      // max number of args is 4 for cshift and 5 for eoshift
      if (args > kind + 4) {
        S.Diag(E->getRParenLoc(), diag::err_shift_args) << kind;
        error = true;
      } else {
        Expr* fe = E->getArg(0);

        if (ImplicitCastExpr* ce = dyn_cast<ImplicitCastExpr>(fe)) {
          fe = ce->getSubExpr();
        }

        if (MemberExpr* me = dyn_cast<MemberExpr>(fe)) {
          if (DeclRefExpr* dr = dyn_cast<DeclRefExpr>(me->getBase())) {
            ValueDecl* bd = dr->getDecl();
            const Type *T= bd->getType().getCanonicalType().getTypePtr();
            if (!isa<MeshType>(T)) {
              S.Diag(fe->getExprLoc(), diag::err_shift_field) << kind;
              error = true;
            }
            if(isa<StructuredMeshType>(T)) {
              S.Diag(fe->getExprLoc(), diag::err_shift_not_allowed) << kind << 0;
              error = true;
            }
            if(isa<UnstructuredMeshType>(T)) {
              S.Diag(fe->getExprLoc(), diag::err_shift_not_allowed) << kind << 1;
              error = true;
            }

          }
        } else {
          S.Diag(fe->getExprLoc(), diag::err_shift_field) << kind;
          error = true;
        }
        // only allow integers for shift values
        for (unsigned i = kind+1; i < args; i++) {
          Expr *arg = E->getArg(i);
          // remove unary operator if it exists
          if(UnaryOperator *UO = dyn_cast<UnaryOperator>(arg)) {
            arg = UO->getSubExpr();
          }
          if(!isa<IntegerLiteral>(arg)) {
            S.Diag(arg->getExprLoc(), diag::err_shift_nonint) << kind;
            error = true;
          }
        }
      }
      return error;
    }



    void ForallVisitor::VisitBinaryOperator(BinaryOperator* S) {
      nodeType_ = NodeNone;

      switch(S->getOpcode()){
        case BO_Assign:
        case BO_MulAssign:
        case BO_DivAssign:
        case BO_RemAssign:
        case BO_AddAssign:
        case BO_SubAssign:
        case BO_ShlAssign:
        case BO_ShrAssign:
        case BO_AndAssign:
        case BO_XorAssign:
        case BO_OrAssign: {
   
          Expr *E = S->getLHS();

          // deal w/ array case
          if(ArraySubscriptExpr *ASR = dyn_cast<ArraySubscriptExpr>(E)) {
            E = ASR->getBase();
            if(ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(E)) {
              E = ICE->getSubExpr();
            }
          }

          if(DeclRefExpr* DR = dyn_cast<DeclRefExpr>(E)){
            RefMap_::iterator itr = localMap_.find(DR->getDecl()->getName().str());
            if(itr == localMap_.end()){

              if (!isTask_ && !isReduction_) sema_.Diag(DR->getLocation(),
                                                        diag::warn_lhs_outside_forall) << DR->getDecl()->getName();
            }
          }

          nodeType_ = NodeLHS;
          break;
        }
        default:
          break;
      }

      Visit(S->getLHS());
      nodeType_ = NodeRHS;
      Visit(S->getRHS());
      nodeType_ = NodeNone;
    }

    void ForallVisitor::VisitCallExpr(CallExpr* E) {

      MeshElementType FET = fs_->getMeshElementRef();
      FunctionDecl* fd = E->getDirectCallee();

      if (fd) {
        std::string name = fd->getName();
        unsigned id = fd->getBuiltinID();
        if (name == "printf" || name == "fprintf") {
          // SC_TODO -- for now we'll warn that you're calling a print
          // function inside a parallel construct -- in the long run
          // we can either (1) force the loop to run sequentially or
          // (2) replace print function with a "special" version...
          if (!isTask_) sema_.Diag(E->getExprLoc(), diag::warn_forall_calling_io_func);
        } else if (isCShift(id) || isEOShift(id)) {
          if (!isTask_) error_ = CheckShift(id, E, sema_);
        } else if (id == Builtin::BIhead || id == Builtin::BItail) {
          // check if we are in forall edges or faces
          switch(FET) {
            case Cells:
            case Vertices:
              sema_.Diag(E->getExprLoc(), diag::err_forall_non_edges);
              break;
            case Faces: // include this but warn as faces=edges in 2d
              //SC_TODO: need to set EdgeIndex in forall faces 2d
              sema_.Diag(E->getExprLoc(), diag::warn_forall_non_edges);
              break;
            case Edges:
              break;
            default:
              assert(false && "invalid forall case");
          }
        } else if (isPosition(id)) {
          //check if we are in forall cells/vertices
          switch(FET) {
            case Cells:
            case Vertices:
              break;
            case Faces:
            case Edges:
              sema_.Diag(E->getExprLoc(), diag::err_forall_non_cells_vertices);
              break;
            default:
              assert(false && "invalid forall case");
          }

        } else if (isMPosition(id)) {
     
          // is this where I need to check that we are in a forall in an ALE mesh? 
      
          // right now only allow this builtin on vertices
          switch(FET) {
            case Vertices:
              break;
            case Faces:
            case Cells:
            case Edges:
              sema_.Diag(E->getExprLoc(), diag::err_forall_non_vertices);
              break;
            default:
              assert(false && "invalid forall case");
          }
        }
      }

      VisitChildren(E);
    }

    void ForallVisitor::VisitDeclStmt(DeclStmt* S) {

      DeclGroupRef declGroup = S->getDeclGroup();

      for(DeclGroupRef::iterator itr = declGroup.begin(),
            itrEnd = declGroup.end(); itr != itrEnd; ++itr){
        Decl* decl = *itr;

        if (NamedDecl* nd = dyn_cast<NamedDecl>(decl)) {
          localMap_.insert(make_pair(nd->getName().str(), true));
        }
      }

      VisitChildren(S);
    }

    void ForallVisitor::VisitMemberExpr(MemberExpr* E) {

      if (DeclRefExpr* dr = dyn_cast<DeclRefExpr>(E->getBase())) {
        ValueDecl* bd = dr->getDecl();

        if (isa<MeshType>(bd->getType().getCanonicalType().getTypePtr())){

          ValueDecl* md = E->getMemberDecl();

          std::string ref = bd->getName().str() + "." + md->getName().str();

          if (nodeType_ == NodeLHS) {
            refMap_.insert(make_pair(ref, true));
            meshAccess_ = true;
          } else if (nodeType_ == NodeRHS) {
            RefMap_::iterator itr = refMap_.find(ref);
            meshAccess_ = true;
            if (itr != refMap_.end()) {
              if (!isTask_) sema_.Diag(E->getMemberLoc(), diag::err_rhs_after_lhs_forall);
              error_ = true;
            }
          }
        }
      }
    }

    void ForallVisitor::VisitStmt(Stmt* S) {

      // Upon construction of the visitor we have already visited the
      // invoking 'forall'.  In order to analyze the nesting of forall
      // statements we have to first see if the next statement we are
      // visiting is another forall (per a child relationship with the
      // statements in the body of the 'invoking' forall visitor).
        
      if (S->getStmtClass() == Stmt::ForallMeshStmtClass) {
        // We are visiting a nested forall within the scope of our
        // originally invoked forall.
        ForallMeshStmt *FMS = cast<ForallMeshStmt>(S);

        // Next, save the statement on our stack of visited foralls so
        // we can include them in our checking...  Note the constructor
        // pushes the 'invoking' forall onto the stack...
        forallStmts.push_back(FMS);

        // To avoid walking the stack to determine if there is an
        // overlapping/repeating mesh element kind in a nested forall
        // construct we use a bitmask to help us catch these cases...
        if ((forallStmtMask & FMS->getMeshElementRef()) != 0) {
          // We have a forall construct that repeats the mesh element
          // type (cell, vertices, etc.).  Next we need to determine
          // if the mesh instance is idential (to identify potentially
          // correct nested mesh constructs like AMR).  
          ForallStmtStack::iterator it = forallStmts.begin();
          while(it != forallStmts.end() && (*it) != FMS) {
            // We have to handle to different potential constructs
            // here.  The first is an explicit mesh -- in which case
            // we will have identical mesh info details. If we don't
            // have the same mesh info it is still possible we're
            // referring to the same mesh via an implicit mesh
            // parameter; in this case we have to see if our mesh var
            // decl is actually an implicit mesh param... 
            bool same_mesh = (*it)->getMeshInfo() == FMS->getMeshInfo();
            if (! same_mesh) {
              const ImplicitMeshParamDecl* IP = dyn_cast<ImplicitMeshParamDecl>(FMS->getMeshVarDecl());
              same_mesh = IP != 0;
            }
                
            if (same_mesh) {
              // Based on the element that is repeated in the loop
              // construct we build a string to help us provide a
              // detailed error message...
              std::string nested_elem_kind;
              switch(FMS->getMeshElementRef()) {
                    
                case Cells:
                  nested_elem_kind = "cells";
                  break;

                case Vertices:
                  nested_elem_kind = "vertices";
                  break;

                case Faces:
                  nested_elem_kind = "faces";
                  break;

                case Edges:
                  nested_elem_kind = "edges";
                  break;

                case AllElements:
                default:
                  assert(false && "unexpected mesh element kind/type");
                  break;
              }
                      
              sema_.Diag(FMS->getForAllLoc(), diag::err_forall_mesh_repeated_nesting)
                << nested_elem_kind
                << (*it)->getForAllLoc().printToString(sema_.getSourceManager());
              error_ = true;
              break;
            } else {
              // SC_TODO: Although at the design level it is perfectly
              // reasonable to envision nested mesh constructs, we
              // currently don't have the codegen support to handle
              // such constructs cleanly...  For now we'll generate a
              // unsupported construct error...
              sema_.Diag(FMS->getForAllLoc(), diag::err_nested_meshes_unsupported);
              error_ = true;
              break;
            }
            ++it;
          }
        }

        // Update our forall element info mask and then process all
        // the children of the forall statement (the body of the
        // forall).  Note that this path actually leads to a recursive
        // execution as VisitChildren() will in turn call back into
        // this member function to visit each child statement...
        // 
        // SC_TODO: We currently don't support analysis across
        // function invocations within forall loops (e.g. a call to a
        // function within a forall body that itself has an internal
        // forall that leads to an incorrect/unsupported nesting.
        forallStmtMask = forallStmtMask | FMS->getMeshElementRef();
        VisitChildren(S);
        // After visiting the body of the forall we need to reset
        // our mask and remove the forall from the stack...
        forallStmtMask &= ~FMS->getMeshElementRef();
        (void)forallStmts.pop_back();
      } else {
        VisitChildren(S);
      }
    }

    void ForallReductionVisitor::VisitBinaryOperator(BinaryOperator* S){
      if(S->getOpcode() != BO_AddAssign){
        return;
      }
  
      if(!isa<DeclRefExpr>(S->getLHS())){
        return;
      }
  
      isReduction_ = true;
    }
  
    void RenderallVisitor::VisitBinaryOperator(BinaryOperator* S) {
      switch(S->getOpcode()){
        case BO_Assign:
          if(S->getOpcode() == BO_Assign){

            // "color = " case
            if(isColorExpr(S->getLHS())) {
              foundColorAssign_ = true;
              // "color.{rgba} = " case
            } else if(ExtVectorElementExpr* VE = dyn_cast<ExtVectorElementExpr>(S->getLHS())) {
              if(isColorExpr(VE->getBase())) { // make sure Base is a color expr

                //only allow .r .g .b .a not combos like .rg
                if (VE->getNumElements() == 1) {
                  const char *namestart = VE->getAccessor().getNameStart();
                  // find the index for this accesssor name {r,g,b,a} -> {0,1,2,3}
                  int idx = ExtVectorType::getAccessorIdx(*namestart);
                  foundComponentAssign_[idx] = true;
                }
              }
            }
          } else {
            VisitChildren(S);
          }
          break;
        case BO_MulAssign:
        case BO_DivAssign:
        case BO_RemAssign:
        case BO_AddAssign:
        case BO_SubAssign:
        case BO_ShlAssign:
        case BO_ShrAssign:
        case BO_AndAssign:
        case BO_XorAssign:
        case BO_OrAssign: {
          Expr *E = S->getLHS();

          // deal w/ array case
          if(ArraySubscriptExpr *ASR = dyn_cast<ArraySubscriptExpr>(E)) {
            E = ASR->getBase();
            if(ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(E)) {
              E = ICE->getSubExpr();
            }
          }

          if(DeclRefExpr* DR = dyn_cast<DeclRefExpr>(E)){
            RefMap_::iterator itr = localMap_.find(DR->getDecl()->getName().str());
            if(itr == localMap_.end()){

              sema_.Diag(DR->getLocation(),
                         diag::warn_lhs_outside_forall) << DR->getDecl()->getName();
            }
          }

          nodeType_ = NodeLHS;
          break;
        }
        default:
          break;
      }

      Visit(S->getLHS());
      nodeType_ = NodeRHS;
      Visit(S->getRHS());
      nodeType_ = NodeNone;
    }

    void RenderallVisitor::VisitCallExpr(CallExpr* E) {

      FunctionDecl* fd = E->getDirectCallee();

      if (fd) {
        std::string name = fd->getName();
        unsigned id = fd->getBuiltinID();
        if (name == "printf" || name == "fprintf") {
          // SC_TODO -- for now we'll warn that you're calling a print
          // function inside a parallel construct -- in the long run
          // we can either (1) force the loop to run sequentially or
          // (2) replace print function with a "special" version...
          sema_.Diag(E->getExprLoc(), diag::warn_renderall_calling_io_func);
        } else if (isCShift(id) || isEOShift(id)) {
          error_ = CheckShift(id, E, sema_);
        }
      }

      VisitChildren(E);
    }

    void RenderallVisitor::VisitDeclStmt(DeclStmt* S) {

      DeclGroupRef declGroup = S->getDeclGroup();

      for(DeclGroupRef::iterator itr = declGroup.begin(),
            itrEnd = declGroup.end(); itr != itrEnd; ++itr){
        Decl* decl = *itr;

        if (NamedDecl* nd = dyn_cast<NamedDecl>(decl)) {
          localMap_.insert(make_pair(nd->getName().str(), true));
        }
      }

      VisitChildren(S);
    }


    void RenderallVisitor::VisitIfStmt(IfStmt* S) {
      size_t ic = 0;
      for(Stmt::child_iterator I = S->child_begin(),
            E = S->child_end(); I != E; ++I){

        if(Stmt* child = *I) {
          if(isa<CompoundStmt>(child) || isa<IfStmt>(child)) {
            RenderallVisitor v(sema_,fs_);
            v.Visit(child);
            if(v.foundColorAssign()) {
              foundColorAssign_ = true;
            } else {
              foundColorAssign_ = false;
              break;
            }
          } else {
            Visit(child);
          }
          ++ic;
        }
      }
      if(ic == 2) {
        foundColorAssign_ = false;
      }
    }

    void RenderallVisitor::VisitMemberExpr(MemberExpr* E) {

      if (DeclRefExpr* dr = dyn_cast<DeclRefExpr>(E->getBase())) {
        ValueDecl* bd = dr->getDecl();

        if (isa<MeshType>(bd->getType().getCanonicalType().getTypePtr())){

          ValueDecl* md = E->getMemberDecl();

          std::string ref = bd->getName().str() + "." + md->getName().str();

          if (nodeType_ == NodeLHS) {
            refMap_.insert(make_pair(ref, true));
          } else if (nodeType_ == NodeRHS) {
            RefMap_::iterator itr = refMap_.find(ref);
            if (itr != refMap_.end()) {
              sema_.Diag(E->getMemberLoc(), diag::err_rhs_after_lhs_forall);
              error_ = true;
            }
          }
        }
      }
    }


    void TaskStmtVisitor::VisitCallExpr(CallExpr* E) {

      FunctionDecl* fd = E->getDirectCallee();

      if (fd) {
        std::string name = fd->getName();
        if (name == "printf" || name == "fprintf") {

          if(sema_.getLangOpts().ScoutLegionSupport) {
            // in legion mode printf in task is error
            //sema_.Diag(E->getExprLoc(), diag::err_task_calling_io_func);
            // leave as a warning for now...
            sema_.Diag(E->getExprLoc(), diag::warn_task_calling_io_func);
          } else {
            //  warn that you're calling a print function inside a task
            sema_.Diag(E->getExprLoc(), diag::warn_task_calling_io_func);
          }
        }
        TaskDeclVisitor v(sema_, fd);
        if(fd->getBody()) {
          v.VisitStmt(fd->getBody());
        } else {
          // can't find body assume function accesses mesh
          meshAccess_ = true;
        }
        VisitChildren(fd->getBody());
      }

      VisitChildren(E);
    }

    void TaskStmtVisitor::VisitChildren(Stmt* S) {
      if(S) {
        for(Stmt::child_iterator I = S->child_begin(), E = S->child_end(); I != E; ++I) {
          if (Stmt* child = *I) {
            Visit(child);
          }
        }
      }
    }

    void TaskStmtVisitor::VisitDeclRefExpr(DeclRefExpr *E) {
      if(VarDecl *VD = dyn_cast<VarDecl>(E->getDecl())) {
        if(VD->hasGlobalStorage() && !VD->getType().isConstQualified()) {
          sema_.Diag(E->getLocStart(), diag::err_nonpure_task_fuction);
        }
        if(VD->isStaticDataMember()) {
          sema_.Diag(E->getLocStart(), diag::err_nonpure_task_fuction);
        }
      }
      VisitChildren(E);
    }


    void TaskStmtVisitor::VisitForallMeshStmt(ForallMeshStmt *S) {
      ForallVisitor v(sema_, S, true);
      v.Visit(S);
      if(v.getMeshAccess()) meshAccess_ = true;

      VisitChildren(S);
    }


    void TaskDeclVisitor::VisitStmt(Stmt* S) {
      if(FD_->isTaskSpecified()) {
        if(S) {
          for(Stmt::child_iterator I = S->child_begin(), E = S->child_end(); I != E; ++I) {
            if (Stmt* child = *I) {
              TaskStmtVisitor v(sema_, child);
              v.Visit(child);
              if (v.getMeshAccess()) meshAccess_ = true;
            }
          }
          // don't allow tasks w/ no mesh access in legion mode
          if(sema_.getLangOpts().ScoutLegionSupport && !meshAccess_) {
            sema_.Diag(S->getLocStart(), diag::err_nomesh_task_fuction);
          }
        }
        // make sure there is only one mesh parameter if we are in legion mode
        if(sema_.getLangOpts().ScoutLegionSupport) {
          int meshcount = 0;
          for(unsigned i = 0; i < FD_->getNumParams(); i++) {
            ParmVarDecl *PVD = FD_->getParamDecl(i);
            const Type *T = PVD->getType().getCanonicalType().getTypePtr();
            if(T->isPointerType() || T->isReferenceType()) {
              if(isa<UniformMeshType>(T->getPointeeType())) {
                meshcount++;
              }
            }
          }
          if(meshcount > 1) {
            sema_.Diag(S->getLocStart(), diag::err_more_than_one_mesh_task_fuction);
          }
        }
      }
    }

    

  } // end namespace sema
} // end namespace clang

