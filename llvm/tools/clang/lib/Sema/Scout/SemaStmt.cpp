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

#include "clang/Sema/SemaInternal.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/Lookup.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/Scout/ImplicitMeshParamDecl.h"
#include "clang/AST/Scout/ImplicitColorParamDecl.h"
#include "clang/Basic/Builtins.h"
#include "clang/AST/Expr.h"
#include <map>

using namespace clang;
using namespace sema;

enum ShiftKind {
  CShift,
  EOShift
};

//check if builtin id is a cshift
static bool isCShift(unsigned id) {
  if (id == Builtin::BIcshift || id == Builtin::BIcshifti
      || id == Builtin::BIcshiftf || id == Builtin::BIcshiftd ) return true;
  return false;
}

//check if builtin id is an eoshift
static bool isEOShift(unsigned id) {
  if (id == Builtin::BIeoshift || id == Builtin::BIeoshifti
      || id == Builtin::BIeoshiftf || id == Builtin::BIeoshiftd ) return true;
  return false;
}

static bool CheckShift(unsigned id, CallExpr *E, Sema &S) {
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
//disable this check for now as Jamal needs this functionality
#if 0
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
#endif

  }
  return error;
}


// ===== Scout ================================================================
// ForAllVisitor class to check that LHS mesh field assignment
// operators do not appear as subsequent RHS values, and various other
// semantic checks

namespace {

  class ForallVisitor : public StmtVisitor<ForallVisitor> {
   public:

    enum NodeType{
      NodeNone,
      NodeLHS,
      NodeRHS
    };

    ForallVisitor(Sema& sema, ForallMeshStmt* fs)
      : sema_(sema),
        fs_(fs),
        error_(false),
        nodeType_(NodeNone) {
      (void)fs_; //suppress warning
    }

    void VisitStmt(Stmt* S) {
      VisitChildren(S);
    }

    void VisitCallExpr(CallExpr* E) {

      FunctionDecl* fd = E->getDirectCallee();

      if (fd) {
        std::string name = fd->getName();
        unsigned id = fd->getBuiltinID();
        if (name == "printf" || name == "fprintf") {
          // SC_TODO -- for now we'll warn that you're calling a print
          // function inside a parallel construct -- in the long run
          // we can either (1) force the loop to run sequentially or
          // (2) replace print function with a "special" version...
          sema_.Diag(E->getExprLoc(), diag::warn_forall_calling_io_func);
        } else if (isCShift(id) || isEOShift(id)) {
          error_ = CheckShift(id, E, sema_);
        }
      }

      VisitChildren(E);
    }

    void VisitChildren(Stmt* S) {
      for(Stmt::child_iterator I = S->child_begin(), E = S->child_end(); I != E; ++I) {
        if (Stmt* child = *I) {
          Visit(child);
        }
      }
    }

    void VisitMemberExpr(MemberExpr* E) {

      if (DeclRefExpr* dr = dyn_cast<DeclRefExpr>(E->getBase())) {
        ValueDecl* bd = dr->getDecl();

        if (isa<MeshType>(bd->getType().getCanonicalType().getTypePtr())){

          ValueDecl* md = E->getMemberDecl();

          //SC_TODO: is there a cleaner way to do this w/o an map?
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


    void VisitDeclStmt(DeclStmt* S) {

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

    void VisitBinaryOperator(BinaryOperator* S){

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
        case BO_OrAssign:
          if(DeclRefExpr* DR = dyn_cast<DeclRefExpr>(S->getLHS())){
            RefMap_::iterator itr = localMap_.find(DR->getDecl()->getName().str());
            if(itr == localMap_.end()){

              sema_.Diag(DR->getLocation(),
                         diag::warn_lhs_outside_forall) << DR->getDecl()->getName();
            }
          }

          nodeType_ = NodeLHS;
          break;
        default:
          break;
      }

      Visit(S->getLHS());
      nodeType_ = NodeRHS;
      Visit(S->getRHS());
      nodeType_ = NodeNone;
    }

    bool error(){
      return error_;
    }

  private:
    Sema& sema_;
    ForallMeshStmt *fs_;
    typedef std::map<std::string, bool> RefMap_;
    RefMap_ refMap_;
    RefMap_ localMap_;
    bool error_;
    NodeType nodeType_;
  };

} // end namespace


// We have to go in circles a bit here. First get the QualType
// from the VarDecl, then check if this is a MeshType and if so
// we can get the MeshDecl
MeshDecl *VarDecl2MeshDecl(VarDecl *VD) {
  QualType QT = VD->getType();

   if (const MeshType *MT = QT->getAs<MeshType>()) {
      return MT->getDecl();
   }
   return 0;
}

// Check forall mesh for shadowing
bool Sema::CheckForallMesh(Scope* S,
                                IdentifierInfo* RefVarInfo,
                                SourceLocation RefVarLoc,
                                VarDecl *VD) {

  // check if RefVar is a mesh member.
  // see test/scc/error/forall-mesh-shadow.sc
  if(MeshDecl *MD = VarDecl2MeshDecl(VD)) {

    // lookup RefVar name as a member. If we did an Ordinary
    // lookup we would be in the wrong IDNS. we need IDNS_Member
    // here not IDNS_Ordinary
    LookupResult MemberResult(*this, RefVarInfo, RefVarLoc,
            LookupMemberName);

    //lookup this in the MeshDecl
    LookupQualifiedName(MemberResult, MD);
    if(MemberResult.getResultKind() != LookupResult::NotFound) {
      Diag(RefVarLoc, diag::err_loop_variable_shadows_forall) << RefVarInfo;
      return false;
    }
  }

  // warn about shadowing see test/scc/warning/forall-mesh-shadow.sc
  // look up implicit mesh Ref variable
  LookupResult RefResult(*this, RefVarInfo, RefVarLoc,
          LookupOrdinaryName);
  LookupName(RefResult, S);
  if(RefResult.getResultKind() != LookupResult::NotFound) {
    Diag(RefVarLoc, diag::warn_loop_variable_shadows_forall) << RefVarInfo;
  }

  return true;
}


// ----- ActOnForallMeshRefVariable
// This call assumes the reference variable details have been parsed
// Given this, this member function takes steps to further determine
// the actual mesh type of the forall (passed in as a base mesh type)
// and creates the reference variable and its DeclStmt
bool Sema::ActOnForallMeshRefVariable(Scope* S,
                                  IdentifierInfo* MeshVarInfo,
                                  SourceLocation MeshVarLoc,
                                  ForallMeshStmt::MeshElementType RefVarType,
                                  IdentifierInfo* RefVarInfo,
                                  SourceLocation RefVarLoc,
                                  const MeshType *MT,
                                  VarDecl* VD,
                                  DeclStmt **Init) {

  if(!CheckForallMesh(S, RefVarInfo, RefVarLoc, VD)) {
    return false;
  }

  ImplicitMeshParamDecl* D;

  ImplicitMeshParamDecl::MeshElementType ET = (ImplicitMeshParamDecl::MeshElementType)RefVarType;

  if (MT->isUniform()) {
    D = ImplicitMeshParamDecl::Create(Context,
                                      CurContext,
                                      ET,
                                      RefVarLoc,
                                      RefVarInfo,
                                      QualType(cast<UniformMeshType>(MT),0), VD);
  } else if (MT->isStructured()) {
    D = ImplicitMeshParamDecl::Create(Context,
                                      CurContext,
                                      ET,
                                      RefVarLoc,
                                      RefVarInfo,
                                      QualType(cast<StructuredMeshType>(MT),0), VD);

  } else if (MT->isRectilinear()) {
    D = ImplicitMeshParamDecl::Create(Context,
                                      CurContext,
                                      ET,
                                      RefVarLoc,
                                      RefVarInfo,
                                      QualType(cast<RectilinearMeshType>(MT),0), VD);
  } else if (MT->isUnstructured()) {
    D = ImplicitMeshParamDecl::Create(Context,
                                      CurContext,
                                      ET,
                                      RefVarLoc,
                                      RefVarInfo,
                                      QualType(cast<UnstructuredMeshType>(MT),0), VD);

  } else {
    assert(false && "unknown mesh type");
    return false;
  }

  // build a DeclStmt for the ImplicitMeshParamDecl and return it via parameter list
  *Init = new (Context) DeclStmt(DeclGroupRef(D), RefVarLoc, RefVarLoc);

  PushOnScopeChains(D, S, true);
  SCLStack.push_back(D);  //SC_TODO: this seems like an ugly hack

  return true;
}

StmtResult Sema::ActOnForallMeshStmt(SourceLocation ForallLoc,
                                     ForallMeshStmt::MeshElementType ElementType,
                                     const MeshType *MT,
                                     VarDecl* MVD,
                                     IdentifierInfo* RefVarInfo,
                                     IdentifierInfo* MeshInfo,
                                     SourceLocation LParenLoc,
                                     Expr* Predicate, SourceLocation RParenLoc,
                                     DeclStmt* Init, Stmt* Body) {

  SCLStack.pop_back();

  ForallMeshStmt* FS = new (Context) ForallMeshStmt(ElementType,
                                                    RefVarInfo,
                                                    MeshInfo, MVD, MT,
                                                    ForallLoc,
                                                    Init, Body, Predicate,
                                                    LParenLoc, RParenLoc);

  // check that LHS mesh field assignment
  // operators do not appear as subsequent RHS values, and
  // perform other semantic checks
  ForallVisitor v(*this, FS);
  v.Visit(Body);

  if (v.error()){
    return StmtError();
  }

  return FS;
}


// Check forall array for shadowing
bool Sema::CheckForallArray(Scope* S,
                                  IdentifierInfo* InductionVarInfo,
                                  SourceLocation InductionVarLoc) {

  LookupResult LResult(*this, InductionVarInfo, InductionVarLoc,
      LookupOrdinaryName);

  LookupName(LResult, S);

  if(LResult.getResultKind() != LookupResult::NotFound){
    Diag(InductionVarLoc, diag::warn_loop_variable_shadows_forall) <<
        InductionVarInfo;

  }
  return true;
}


// ----- ActOnForallArrayInductionVariable
// This call  creates the Induction variable and its DeclStmt
bool Sema::ActOnForallArrayInductionVariable(Scope* S,
                                             IdentifierInfo* InductionVarInfo,
                                             SourceLocation InductionVarLoc,
                                             VarDecl **InductionVarDecl,
                                             DeclStmt **Init) {

  if(!CheckForallArray(S,InductionVarInfo, InductionVarLoc)) {
    return false;
  }

  // build the Induction Var. VarDecl and DeclStmt this is
  // similar to what is done in buildSingleCopyAssignRecursively()
  *InductionVarDecl = VarDecl::Create(Context, CurContext, InductionVarLoc, InductionVarLoc,
      InductionVarInfo, Context.IntTy, 0, SC_None);

  // zero initialize the induction var
  (*InductionVarDecl)->setInit(IntegerLiteral::Create(Context, llvm::APInt(32, 0),
      Context.IntTy, InductionVarLoc));

  // build a DeclStmt for the VarDecl and return both via parameter list
  *Init = new (Context) DeclStmt(DeclGroupRef(*InductionVarDecl),
      InductionVarLoc, InductionVarLoc);

  PushOnScopeChains(*InductionVarDecl, S, true);

  return true;
}



StmtResult Sema::ActOnForallArrayStmt(IdentifierInfo* InductionVarInfo[],
          VarDecl* InductionVarDecl[],
          Expr* Start[], Expr* End[], Expr* Stride[], size_t Dims,
          SourceLocation ForallLoc, DeclStmt* Init[], Stmt* Body) {


  ForallArrayStmt* FS =
  new (Context) ForallArrayStmt(InductionVarInfo, InductionVarDecl,
      Start, End, Stride, Dims, ForallLoc, Init, Body);

  return FS;
}


namespace {
  class RenderallVisitor : public StmtVisitor<RenderallVisitor> {
   public:

    enum NodeType{
      NodeNone,
      NodeLHS,
      NodeRHS
    };

    RenderallVisitor(Sema& sema, RenderallMeshStmt* fs)
      : sema_(sema),
        fs_(fs),
        error_(false),
        nodeType_(NodeNone),
        foundColorAssign_(false) {
      for(size_t i = 0; i < 4; ++i) {
        foundComponentAssign_[i] = false;
      }

    }

    void VisitStmt(Stmt* S) {
      VisitChildren(S);
    }

    void VisitCallExpr(CallExpr* E) {

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

    void VisitChildren(Stmt* S) {
      for(Stmt::child_iterator I = S->child_begin(), E = S->child_end(); I != E; ++I) {
        if (Stmt* child = *I) {
          Visit(child);
        }
      }
    }

    void VisitMemberExpr(MemberExpr* E) {

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


    void VisitDeclStmt(DeclStmt* S) {

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

    bool isColorExpr(Expr *E) {
      if (DeclRefExpr* DR = dyn_cast<DeclRefExpr>(E)) {
        if(isa<ImplicitColorParamDecl>(DR->getDecl())) {
          return true;
        }
      }
      return false;
    }

    void VisitBinaryOperator(BinaryOperator* S){
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
        case BO_OrAssign:
          if(DeclRefExpr* DR = dyn_cast<DeclRefExpr>(S->getLHS())){
            RefMap_::iterator itr = localMap_.find(DR->getDecl()->getName().str());
            if(itr == localMap_.end()){

              sema_.Diag(DR->getLocation(),
                         diag::warn_lhs_outside_forall) << DR->getDecl()->getName();
            }
          }

          nodeType_ = NodeLHS;
          break;
        default:
          break;
      }

      Visit(S->getLHS());
      nodeType_ = NodeRHS;
      Visit(S->getRHS());
      nodeType_ = NodeNone;
    }

    void VisitIfStmt(IfStmt* S) {
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

    bool foundColorAssign() {
      if(foundColorAssign_) {
        return true;
      }

      for(size_t i = 0; i < 4; ++i) {
        if(!foundComponentAssign_[i]) {
          return false;
        }
      }
      return true;
    }


    bool error(){
      return error_;
    }

  private:
    Sema& sema_;
    RenderallMeshStmt *fs_;
    typedef std::map<std::string, bool> RefMap_;
    RefMap_ refMap_;
    RefMap_ localMap_;
    bool error_;
    NodeType nodeType_;
    bool foundColorAssign_;
    bool foundComponentAssign_[4];
  };
} // end namespace


// ----- ActOnRenderallRefVariable
// This call assumes the reference variable details have been parsed
// (syntax checked) and issues, such as shadows, have been reported.
// Given this, this member function takes steps to further determine
// the actual mesh type of the renderall (passed in as a base mesh type)
// and creates the reference variable
bool Sema::ActOnRenderallMeshRefVariable(Scope* S,
                                         RenderallMeshStmt::MeshElementType RefVarType,
                                         IdentifierInfo* RefVarInfo,
                                         SourceLocation RefVarLoc,
                                         const MeshType *MT,
                                         VarDecl* VD) {

  ImplicitMeshParamDecl* D;

  ImplicitMeshParamDecl::MeshElementType ET = (ImplicitMeshParamDecl::MeshElementType)RefVarType;

  if (MT->isUniform()) {
    D = ImplicitMeshParamDecl::Create(Context,
                                      CurContext,
                                      ET,
                                      RefVarLoc,
                                      RefVarInfo,
                                      QualType(cast<UniformMeshType>(MT),0), VD);
  } else if (MT->isStructured()) {
    D = ImplicitMeshParamDecl::Create(Context,
                                      CurContext,
                                      ET,
                                      RefVarLoc,
                                      RefVarInfo,
                                      QualType(cast<StructuredMeshType>(MT),0), VD);

  } else if (MT->isRectilinear()) {
    D = ImplicitMeshParamDecl::Create(Context,
                                      CurContext,
                                      ET,
                                      RefVarLoc,
                                      RefVarInfo,
                                      QualType(cast<RectilinearMeshType>(MT),0), VD);
  } else if (MT->isUnstructured()) {
    D = ImplicitMeshParamDecl::Create(Context,
                                      CurContext,
                                      ET,
                                      RefVarLoc,
                                      RefVarInfo,
                                      QualType(cast<UnstructuredMeshType>(MT),0), VD);

  } else {
    assert(false && "unknown mesh type");
    return false;
  }

  PushOnScopeChains(D, S, true);
  SCLStack.push_back(D);

  // add the implicit "color" parameter
  ImplicitColorParamDecl*CD = ImplicitColorParamDecl::Create(Context, CurContext, RefVarLoc);

  PushOnScopeChains(CD, S, true);

  return true;
}


StmtResult Sema::ActOnRenderallMeshStmt(SourceLocation RenderallLoc,
                                        RenderallMeshStmt::MeshElementType ElementType,
                                        const MeshType *MT,
                                        VarDecl* MVD,    // Mesh var decl. 
                                        VarDecl* RTVD,   // Render target var decl. 
                                        IdentifierInfo* RefVarInfo,
                                        IdentifierInfo* MeshInfo,
                                        IdentifierInfo* RenderTargetInfo,
                                        SourceLocation LParenLoc,
                                        Expr* Predicate, SourceLocation RParenLoc,
                                        Stmt* Body) {

  SCLStack.pop_back();

  RenderallMeshStmt* RS = new (Context) RenderallMeshStmt(ElementType,
                                                          RefVarInfo,
                                                          MeshInfo,
                                                          RenderTargetInfo,
                                                          MVD, RTVD, MT,
                                                          RenderallLoc,
                                                          Body, Predicate,
                                                          LParenLoc, RParenLoc);

  // check that LHS mesh field assignment
  // operators do not appear as subsequent RHS values, and
  // perform other semantic checks
  RenderallVisitor v(*this, RS);
  v.Visit(Body);

  if (!v.foundColorAssign()) {
    Diag(RenderallLoc, diag::err_no_color_assignment_renderall);
    return StmtError();
  }

  if (v.error()){
    return StmtError();
  }

  return RS;
}

