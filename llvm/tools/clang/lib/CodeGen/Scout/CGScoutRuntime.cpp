/*
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
 * ##### 
 */ 

#include "Scout/CGScoutRuntime.h"
#include "CodeGenFunction.h"

using namespace clang;
using namespace CodeGen;

CGScoutRuntime::~CGScoutRuntime() {}

// call the correct scout runtime Initializer
llvm::Function *CGScoutRuntime::ModuleInitFunction() {

  std::string funcName;
#if 0 //CUDA/OPENCL disabled for now
  if(CGM.getLangOpts().ScoutNvidiaGPU){
   funcName = "__scrt_init_cuda";
  } else if(CGM.getLangOpts().ScoutAMDGPU){
   funcName = "__scrt_init_opengl";
  } else {
#endif
   funcName = "__scrt_init_cpu";
  //}

  llvm::Function *scrtInit = CGM.getModule().getFunction(funcName);
  if(!scrtInit) {
   std::vector<llvm::Type*> args;

   llvm::FunctionType *FTy =
   llvm::FunctionType::get(llvm::Type::getVoidTy(CGM.getLLVMContext()),
                            args, false);

   scrtInit = llvm::Function::Create(FTy,
                                     llvm::Function::ExternalLinkage,
                                     funcName,
                                     &CGM.getModule());
  }
  return scrtInit;
}

// build a function call to a scout runtime function t
// SC_TODO: could we use CreateRuntimeFunction? or GetOrCreateLLVMFunction?
llvm::Function *CGScoutRuntime::ScoutRuntimeFunction(std::string funcName, std::vector<llvm::Type*> Params ) {

  llvm::Function *Func = CGM.getModule().getFunction(funcName);
  if(!Func){
    llvm::FunctionType *FTy =
        llvm::FunctionType::get(llvm::Type::getVoidTy(CGM.getLLVMContext()),
            Params, false);

    Func = llvm::Function::Create(FTy,
        llvm::Function::ExternalLinkage,
        funcName,
        &CGM.getModule());
  }
  return Func;
}

llvm::Function *CGScoutRuntime::ScoutRuntimeFunction(std::string funcName, std::vector<llvm::Type*> Params, 
    llvm::Type* retType) {

  llvm::Function *Func = CGM.getModule().getFunction(funcName);
  if(!Func){
    llvm::FunctionType *FTy =
        llvm::FunctionType::get(retType, Params, false);

    Func = llvm::Function::Create(FTy,
        llvm::Function::ExternalLinkage,
        funcName,
        &CGM.getModule());
  }
  return Func;
}

// build function call to __scrt_renderall_uniform_begin()
llvm::Function *CGScoutRuntime::RenderallUniformBeginFunction() {
  std::string funcName = "__scrt_renderall_uniform_begin";
  std::vector<llvm::Type*> Params;

  for(int i=0; i < 3; i++) {
    Params.push_back(llvm::Type::getInt32Ty(CGM.getLLVMContext()));
  }
  return ScoutRuntimeFunction(funcName, Params);
}

// build function call to __scrt_renderall_end()
llvm::Function *CGScoutRuntime::RenderallEndFunction() {
  std::string funcName = "__scrt_renderall_end";

  std::vector<llvm::Type*> Params;
  return ScoutRuntimeFunction(funcName, Params);
}

// build function call to __scrt_create_window()
llvm::Function *CGScoutRuntime::CreateWindowFunction() {
  std::string funcName = "__scrt_create_window";

  std::vector<llvm::Type*> Params;
  Params.push_back(llvm::IntegerType::get(CGM.getModule().getContext(), 16));
  Params.push_back(llvm::IntegerType::get(CGM.getModule().getContext(), 16));

  return ScoutRuntimeFunction(
      funcName, Params, 
      /*retType*/ 
      llvm::PointerType::get(llvm::IntegerType::get(CGM.getModule().getContext(), 8), 0));

      // not sure this works
      // llvm::PointerType::get(llvm::Type::getVoidTy(CGM.getLLVMContext()),0));
}

// get Value for global runtime variable __scrt_renderall_uniform_colors
llvm::Value *CGScoutRuntime::RenderallUniformColorsGlobal(CodeGenFunction &CGF) {
  std::string varName = "__scrt_renderall_uniform_colors";
  llvm::Type *flt4PtrTy = llvm::PointerType::get(
      llvm::VectorType::get(llvm::Type::getFloatTy(CGM.getLLVMContext()), 4), 0);

  if (!CGM.getModule().getNamedGlobal(varName)) {

    new llvm::GlobalVariable(CGM.getModule(),
        flt4PtrTy,
        false,
        llvm::GlobalValue::ExternalLinkage,
        0,
        varName);
  }

  llvm::Value *Color = CGM.getModule().getNamedGlobal(varName);

  llvm::Value *ColorPtr  = CGF.Builder.CreateAlloca(flt4PtrTy, 0, "color.ptr");
  CGF.Builder.CreateStore(CGF.Builder.CreateLoad(Color, "runtime.color"), ColorPtr);

  return ColorPtr;
}


llvm::Type *CGScoutRuntime::convertScoutSpecificType(const Type *T) {
  llvm::LLVMContext& Ctx = CGM.getLLVMContext();
  if (T->isScoutWindowType()) {
    return llvm::PointerType::get(llvm::StructType::create(Ctx, "scout.window_t"), 0);
  } else if (T->isScoutImageType()) {
    return llvm::PointerType::get(llvm::StructType::create(Ctx, "scout.image_t"), 0);
  } else {
    llvm_unreachable("Unexpected scout type!");
    return 0;
  }
}
