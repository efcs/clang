//===--- UsualDeallocationInfo.h --------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the UsualDeallocationInfo type.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_USUAL_DEALLOCATION_INFO_H
#define LLVM_CLANG_AST_USUAL_DEALLOCATION_INFO_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/SmallVector.h"

namespace clang {
class ASTContext;
class FunctionDecl;

class UsualDeallocationInfo {
  UsualDeallocationInfo(const FunctionDecl *FD) : FD(FD) {}

public:
  UsualDeallocationInfo() = default;

  /// Determine whether the specified function is a usual deallocation function
  /// (C++ [basic.stc.dynamic.deallocation]p2), which is an overloaded delete or
  /// delete[] operator with a particular signature. If \p FD is a
  /// CXXMethodDecl, PreventedBy is populated with the declarations of the
  /// functions of the same kind if they were the reason for the specified
  /// function being considered non-usual. This is used by
  /// Sema::isUsualDeallocationFunction to reconsider the answer based on the
  /// context.
  static UsualDeallocationInfo Create(const ASTContext &Context,
                                      const FunctionDecl *FD);

  bool isDelete() const { return IsDelete; }
  bool isUsual() const { return IsUsual; }
  bool isDestroying() const { return IsDestroying; }
  bool isSized() const { return IsSized; }
  bool isAligned() const { return IsAligned; }
  const SmallVectorImpl<const FunctionDecl *> &getPreventedBy() const;

public:
  const FunctionDecl *FD;

protected:
  bool IsDelete = false;
  bool IsUsual = false;
  bool IsDestroying = false;
  bool IsSized = false;
  bool IsAligned = false;
  SmallVector<const FunctionDecl *, 5> PreventedBy;
};

} // end namespace clang

#endif // LLVM_CLANG_AST_USUAL_DEALLOCATION_INFO_H