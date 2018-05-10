//===- ComparisonCategories.cpp - Three Way Comparison Data -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Comparison Category enum and data types, which
//  store the types and expressions needed to support operator<=>
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ComparisonCategories.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "llvm/ADT/SmallVector.h"

using namespace clang;

bool ComparisonCategoryInfo::ValueInfo::hasValidIntValue() const {
  assert(VD && "must have var decl");
  if (!VD->checkInitIsICE())
    return false;

  // Before we attempt to get the value of the first field, ensure that we
  // actually have one (and only one) field.
  auto *Record = VD->getType()->getAsCXXRecordDecl();
  if (std::distance(Record->field_begin(), Record->field_end()) != 1 ||
      !Record->field_begin()->getType()->isIntegralOrEnumerationType())
    return false;

  return true;
}

/// Attempt to determine the integer value used to represent the comparison
/// category result by evaluating the initializer for the specified VarDecl as
/// a constant expression and retreiving the value of the class's first
/// (and only) field.
///
/// Note: The STL types are expected to have the form:
///    struct X { T value; };
/// where T is an integral or enumeration type.
llvm::APSInt ComparisonCategoryInfo::ValueInfo::getIntValue() const {
  assert(hasValidIntValue() && "must have a valid value");
  return VD->evaluateValue()->getStructField(0).getInt();
}

ComparisonCategoryInfo::ValueInfo *ComparisonCategoryInfo::lookupValueInfo(
    ComparisonCategoryResult ValueKind) const {
  // Check if we already have a cache entry for this value.
  auto It = llvm::find_if(
      Objects, [&](ValueInfo const &Info) { return Info.Kind == ValueKind; });
  if (It != Objects.end())
    return &(*It);

  // We don't have a cached result. Lookup the variable declaration and create
  // a new entry representing it.
  DeclContextLookupResult Lookup = Record->getCanonicalDecl()->lookup(
      &Ctx.Idents.get(ComparisonCategories::getResultString(ValueKind)));
  if (Lookup.size() != 1 || !isa<VarDecl>(Lookup.front()))
    return nullptr;
  Objects.emplace_back(ValueKind, cast<VarDecl>(Lookup.front()));
  return &Objects.back();
}

static const NamespaceDecl *lookupStdNamespace(const ASTContext &Ctx,
                                               NamespaceDecl *&StdNS) {
  if (!StdNS) {
    DeclContextLookupResult Lookup =
        Ctx.getTranslationUnitDecl()->lookup(&Ctx.Idents.get("std"));
    if (Lookup.size() == 1)
      StdNS = dyn_cast<NamespaceDecl>(Lookup.front());
  }
  return StdNS;
}

static CXXRecordDecl *lookupCXXRecordDecl(const ASTContext &Ctx,
                                          const NamespaceDecl *StdNS,
                                          ComparisonCategoryType Kind) {
  StringRef Name = ComparisonCategories::getCategoryString(Kind);
  DeclContextLookupResult Lookup = StdNS->lookup(&Ctx.Idents.get(Name));
  if (Lookup.size() == 1)
    if (CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(Lookup.front()))
      return RD;
  return nullptr;
}

const ComparisonCategoryInfo *
ComparisonCategories::lookupInfo(ComparisonCategoryType Kind) const {
  auto It = Data.find(static_cast<char>(Kind));
  if (It != Data.end())
    return &It->second;

  if (const NamespaceDecl *NS = lookupStdNamespace(Ctx, StdNS))
    if (CXXRecordDecl *RD = lookupCXXRecordDecl(Ctx, NS, Kind))
      return &Data.try_emplace((char)Kind, Ctx, RD, Kind).first->second;

  return nullptr;
}

const ComparisonCategoryInfo *
ComparisonCategories::lookupInfoForType(QualType Ty) const {
  assert(!Ty.isNull() && "type must be non-null");
  using CCT = ComparisonCategoryType;
  auto *RD = Ty->getAsCXXRecordDecl();
  if (!RD)
    return nullptr;

  // Check to see if we have information for the specified type cached.
  const auto *CanonRD = RD->getCanonicalDecl();
  for (auto &KV : Data) {
    const ComparisonCategoryInfo &Info = KV.second;
    if (CanonRD == Info.Record->getCanonicalDecl())
      return &Info;
  }

  if (!RD->getEnclosingNamespaceContext()->isStdNamespace())
    return nullptr;

  // If not, check to see if the decl names a type in namespace std with a name
  // matching one of the comparison category types.
  for (unsigned I = static_cast<unsigned>(CCT::First),
                End = static_cast<unsigned>(CCT::Last);
       I <= End; ++I) {
    CCT Kind = static_cast<CCT>(I);

    // We've found the comparison category type. Build a new cache entry for
    // it.
    if (getCategoryString(Kind) == RD->getName())
      return &Data.try_emplace((char)Kind, Ctx, RD, Kind).first->second;
  }

  // We've found nothing. This isn't a comparison category type.
  return nullptr;
}

const ComparisonCategoryInfo &ComparisonCategories::getInfoForType(QualType Ty) const {
  const ComparisonCategoryInfo *Info = lookupInfoForType(Ty);
  assert(Info && "info for comparison category not found");
  return *Info;
}

QualType ComparisonCategoryInfo::getType() const {
  assert(Record);
  return QualType(Record->getTypeForDecl(), 0);
}

StringRef ComparisonCategories::getCategoryString(ComparisonCategoryType Kind) {
  using CCKT = ComparisonCategoryType;
  switch (Kind) {
  case CCKT::WeakEquality:
    return "weak_equality";
  case CCKT::StrongEquality:
    return "strong_equality";
  case CCKT::PartialOrdering:
    return "partial_ordering";
  case CCKT::WeakOrdering:
    return "weak_ordering";
  case CCKT::StrongOrdering:
    return "strong_ordering";
  }
  llvm_unreachable("unhandled cases in switch");
}

StringRef ComparisonCategories::getResultString(ComparisonCategoryResult Kind) {
  using CCVT = ComparisonCategoryResult;
  switch (Kind) {
  case CCVT::Equal:
    return "equal";
  case CCVT::Nonequal:
    return "nonequal";
  case CCVT::Equivalent:
    return "equivalent";
  case CCVT::Nonequivalent:
    return "nonequivalent";
  case CCVT::Less:
    return "less";
  case CCVT::Greater:
    return "greater";
  case CCVT::Unordered:
    return "unordered";
  }
  llvm_unreachable("unhandled case in switch");
}

std::vector<ComparisonCategoryResult>
ComparisonCategories::getPossibleResultsForType(ComparisonCategoryType Type) {
  using CCT = ComparisonCategoryType;
  using CCR = ComparisonCategoryResult;
  std::vector<CCR> Values;
  Values.reserve(6);
  Values.push_back(CCR::Equivalent);
  bool IsStrong = (Type == CCT::StrongEquality || Type == CCT::StrongOrdering);
  if (IsStrong)
    Values.push_back(CCR::Equal);
  if (Type == CCT::StrongOrdering || Type == CCT::WeakOrdering ||
      Type == CCT::PartialOrdering) {
    Values.push_back(CCR::Less);
    Values.push_back(CCR::Greater);
  } else {
    Values.push_back(CCR::Nonequivalent);
    if (IsStrong)
      Values.push_back(CCR::Nonequal);
  }
  if (Type == CCT::PartialOrdering)
    Values.push_back(CCR::Unordered);
  return Values;
}

Optional<ComparisonCategoryType>
ComparisonCategories::computeComparisonTypeForBuiltin(QualType Ty,
                                                      bool IsMixedNullCompare) {
  using CCT = ComparisonCategoryType;
  if (const ComplexType *CT = Ty->getAs<ComplexType>()) {
    if (CT->getElementType()->hasFloatingRepresentation())
      return CCT::WeakEquality;
    return CCT::StrongEquality;
  }
  if (Ty->isIntegralOrEnumerationType())
    return CCT::StrongOrdering;
  if (Ty->hasFloatingRepresentation())
    return CCT::PartialOrdering;
  // C++2a [expr.spaceship]p7: If the composite pointer type is a function
  // pointer type, a pointer-to-member type, or std::nullptr_t, the
  // result is of type std::strong_equality
  if (Ty->isFunctionPointerType() || Ty->isMemberPointerType() ||
      Ty->isNullPtrType())
    // FIXME: consider making the function pointer case produce
    // strong_ordering not strong_equality, per P0946R0-Jax18 discussion
    // and direction polls
    return CCT::StrongEquality;
  // C++2a [expr.spaceship]p8: If the composite pointer type is an object
  // pointer type, p <=> q is of type std::strong_ordering.
  if (Ty->isPointerType()) {
    // P0946R0: Comparisons between a null pointer constant and an object
    // pointer result in std::strong_equality
    if (IsMixedNullCompare)
      return ComparisonCategoryType::StrongEquality;
    return CCT::StrongOrdering;
  }
  return None;
}

Optional<ComparisonCategoryType>
ComparisonCategories::computeComparisonTypeForBuiltin(QualType LHSTy,
                                                      QualType RHSTy) {
  QualType Args[2] = {LHSTy, RHSTy};
  SmallVector<ComparisonCategoryType, 8> TypeKinds;
  for (auto QT : Args) {
    Optional<ComparisonCategoryType> CompType =
        computeComparisonTypeForBuiltin(QT);
    if (!CompType)
      return None;
    TypeKinds.push_back(*CompType);
  }
  return computeCommonComparisonType(TypeKinds);
}

bool ComparisonCategoryInfo::isUsableWithOperator(
        ComparisonCategoryType CompKind, BinaryOperatorKind Opcode) {
  assert(BinaryOperator::isComparisonOp(Opcode));
  if (BinaryOperator::isRelationalOp(Opcode))
    return isOrdered(CompKind);
  // We either have an equality or three-way opcode. These are all OK for
  // any comparison category type.
  return true;
}

/// C++2a [class.spaceship]p4 - compute the common category type.
const ComparisonCategoryInfo *ComparisonCategories::computeCommonComparisonType(
    ArrayRef<QualType> Types) const {
  SmallVector<ComparisonCategoryType, 8> Kinds;
  // Count the number of times each comparison category type occurs in the
  // specified type list. If any type is not a comparison category, return
  // nullptr.
  for (auto Ty : Types) {
    const ComparisonCategoryInfo *Info = lookupInfoForType(Ty);
    // --- If any T is not a comparison category type, U is void.
    if (!Info)
      return nullptr;
    Kinds.push_back(Info->Kind);
  }
  Optional<ComparisonCategoryType> CommonType =
      computeCommonComparisonType(Kinds);
  if (!CommonType)
    return nullptr;
  return lookupInfo(*CommonType);
}

Optional<ComparisonCategoryType>
ComparisonCategories::computeCommonComparisonType(
    ArrayRef<ComparisonCategoryType> Types) {
  using CCT = ComparisonCategoryType;
  std::array<unsigned, static_cast<unsigned>(CCT::Last) + 1> Seen = {};
  auto Count = [&](CCT T) { return Seen[static_cast<unsigned>(T)]; };

  // Count the number of times each comparison category type occurs in the
  // specified type list. If any type is not a comparison category, return
  // nullptr.
  for (auto TyKind : Types) {
    // --- If any T is not a comparison category type, U is void.
    Seen[static_cast<unsigned>(TyKind)]++;
  }

  // --- Otherwise, if at least one Ti is std::weak_equality, or at least one
  // Ti is std::strong_equality and at least one Tj is
  // std::partial_ordering or std::weak_ordering, U is
  // std::weak_equality.
  if (Count(CCT::WeakEquality) ||
      (Count(CCT::StrongEquality) &&
       (Count(CCT::PartialOrdering) || Count(CCT::WeakOrdering))))
    return CCT::WeakEquality;

  // --- Otherwise, if at least one Ti is std::strong_equality, U is
  // std::strong_equality
  if (Count(CCT::StrongEquality))
    return CCT::StrongEquality;

  // --- Otherwise, if at least one Ti is std::partial_ordering, U is
  // std::partial_ordering.
  if (Count(CCT::PartialOrdering))
    return CCT::PartialOrdering;

  // --- Otherwise, if at least one Ti is std::weak_ordering, U is
  // std::weak_ordering.
  if (Count(CCT::WeakOrdering))
    return CCT::WeakOrdering;

  // FIXME(EricWF): What if we don't find std::strong_ordering
  // --- Otherwise, U is std::strong_ordering.
  return CCT::StrongOrdering;
}
