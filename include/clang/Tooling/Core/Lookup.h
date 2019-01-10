//===--- Lookup.h - Framework for clang refactoring tools --*- C++ -*------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines helper methods for clang tools performing name lookup.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLING_CORE_LOOKUP_H
#define LLVM_CLANG_TOOLING_CORE_LOOKUP_H

#include "clang/Basic/LLVM.h"
#include <string>

namespace clang {

class DeclContext;
class FunctionDecl;
class NamedDecl;
class NestedNameSpecifier;

namespace tooling {

/// Emulate a lookup to replace one nested name specifier with another using as
/// few additional namespace qualifications as possible.
///
/// This does not perform a full C++ lookup so ADL will not work.
///
/// \param Use The nested name to be replaced.
/// \param UseContext The context in which the nested name is contained. This
///                   will be used to minimize namespace qualifications.
/// \param FromDecl The declaration to which the nested name points.
/// \param ReplacementString The replacement nested name. Must be fully
///                          qualified including a leading "::".
/// \returns The new name to be inserted in place of the current nested name.
std::string replaceNestedName(const NestedNameSpecifier *Use,
                              const DeclContext *UseContext,
                              const NamedDecl *FromDecl,
                              StringRef ReplacementString,
                              bool RequireFunctionQualification = false);

std::string createQualifiedName(const DeclContext *UseContext,
                                const NamedDecl *ForDecl);

struct RenameOptions {
  bool Minimize = true;
  bool OmitLeadingQualifiers = true;
  bool OmitInlineNamespaces = true;
};

std::string qualifyFunctionNameInContext(const DeclContext *UseContext,
                                         const FunctionDecl *FromDecl,
                                         RenameOptions Opts = RenameOptions());

} // end namespace tooling
} // end namespace clang

#endif // LLVM_CLANG_TOOLING_CORE_LOOKUP_H
