//===--- CGMeshLayout.h - LLVM Record Layout Information ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_CODEGEN_CGMESHLAYOUT_H
#define CLANG_CODEGEN_CGMESHLAYOUT_H

#include "../CGRecordLayout.h"
#include "clang/AST/scout/MeshDecl.h"

namespace llvm {
  class StructType;
}

namespace clang {
namespace CodeGen {

/// CGMeshLayout - This class handles mesh layout info while lowering AST types 
/// to LLVM types.
///
/// These layout objects are only created on demand as IR generation requires.
class CGMeshLayout {
  friend class CodeGenTypes;

  CGMeshLayout(const CGMeshLayout &) LLVM_DELETED_FUNCTION;
  void operator=(const CGMeshLayout &) LLVM_DELETED_FUNCTION;

private:
  /// The LLVM type corresponding to this record layout; used when
  /// laying it out as a complete object.
  llvm::StructType *CompleteObjectType;

  /// Map from (non-bit-field) struct field to the corresponding llvm struct
  /// type field no. This info is populated by record builder.
  llvm::DenseMap<const MeshFieldDecl *, unsigned> FieldInfo;

  /// Map from (bit-field) struct field to the corresponding llvm struct type
  /// field no. This info is populated by record builder.
  llvm::DenseMap<const MeshFieldDecl *, CGBitFieldInfo> BitFields;

  /// False if any direct or indirect subobject of this class, when
  /// considered as a complete object, requires a non-zero bitpattern
  /// when zero-initialized.
  bool IsZeroInitializable : 1;

public:
  CGMeshLayout(llvm::StructType *CompleteObjectType,
               bool IsZeroInitializable)
    : CompleteObjectType(CompleteObjectType),
      IsZeroInitializable(IsZeroInitializable) {}

  /// \brief Return the "complete object" LLVM type associated with
  /// this record.
  llvm::StructType *getLLVMType() const {
    return CompleteObjectType;
  }

  /// \brief Check whether this struct can be C++ zero-initialized
  /// with a zeroinitializer.
  bool isZeroInitializable() const {
    return IsZeroInitializable;
  }

  /// \brief Return llvm::StructType element number that corresponds to the
  /// field FD.
  unsigned getLLVMFieldNo(const MeshFieldDecl *FD) const {
    assert(FieldInfo.count(FD) && "Invalid field for record!");
    return FieldInfo.lookup(FD);
  }

  /// \brief Return the BitFieldInfo that corresponds to the field FD.
  const CGBitFieldInfo &getBitFieldInfo(const MeshFieldDecl *FD) const {
    assert(FD->isBitField() && "Invalid call for non bit-field decl!");
    llvm::DenseMap<const MeshFieldDecl *, CGBitFieldInfo>::const_iterator
      it = BitFields.find(FD);
    assert(it != BitFields.end() && "Unable to find bitfield info");
    return it->second;
  }

  void print(raw_ostream &OS) const;
  void dump() const;
};

}  // end namespace CodeGen
}  // end namespace clang

#endif