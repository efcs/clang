//===- unittest/Tooling/LookupTest.cpp ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TestVisitor.h"
#include "clang/Tooling/Core/Lookup.h"
using namespace clang;

namespace {
struct GetDeclsVisitor : TestVisitor<GetDeclsVisitor> {
  std::function<void(CallExpr *)> OnCall;
  std::function<void(RecordTypeLoc)> OnRecordTypeLoc;
  SmallVector<Decl *, 4> DeclStack;

  bool VisitCallExpr(CallExpr *Expr) {
    if (OnCall)
      OnCall(Expr);
    return true;
  }

  bool VisitRecordTypeLoc(RecordTypeLoc Loc) {
    if (OnRecordTypeLoc)
      OnRecordTypeLoc(Loc);
    return true;
  }

  bool TraverseDecl(Decl *D) {
    DeclStack.push_back(D);
    bool Ret = TestVisitor::TraverseDecl(D);
    DeclStack.pop_back();
    return Ret;
  }
};

TEST(LookupTest, replaceNestedFunctionName) {
  GetDeclsVisitor Visitor;

  auto replaceCallExpr = [&](const CallExpr *Expr,
                             StringRef ReplacementString) {
    const auto *Callee = cast<DeclRefExpr>(Expr->getCallee()->IgnoreImplicit());
    const ValueDecl *FD = Callee->getDecl();
    return tooling::replaceNestedName(
        Callee->getQualifier(), Visitor.DeclStack.back()->getDeclContext(), FD,
        ReplacementString);
  };

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("bar", replaceCallExpr(Expr, "::bar"));
  };
  Visitor.runOver("namespace a { void foo(); }\n"
                  "namespace a { void f() { foo(); } }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("bar", replaceCallExpr(Expr, "::a::bar"));
  };
  Visitor.runOver("namespace a { void foo(); }\n"
                  "namespace a { void f() { foo(); } }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("a::bar", replaceCallExpr(Expr, "::a::bar"));
  };
  Visitor.runOver("namespace a { void foo(); }\n"
                  "namespace b { void f() { a::foo(); } }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("::a::bar", replaceCallExpr(Expr, "::a::bar"));
  };
  Visitor.runOver("namespace a { void foo(); }\n"
                  "namespace b { namespace a { void foo(); }\n"
                  "void f() { a::foo(); } }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("c::bar", replaceCallExpr(Expr, "::a::c::bar"));
  };
  Visitor.runOver("namespace a { namespace b { void foo(); }\n"
                  "void f() { b::foo(); } }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("bar", replaceCallExpr(Expr, "::a::bar"));
  };
  Visitor.runOver("namespace a { namespace b { void foo(); }\n"
                  "void f() { b::foo(); } }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("bar", replaceCallExpr(Expr, "::bar"));
  };
  Visitor.runOver("void foo(); void f() { foo(); }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("::bar", replaceCallExpr(Expr, "::bar"));
  };
  Visitor.runOver("void foo(); void f() { ::foo(); }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("a::bar", replaceCallExpr(Expr, "::a::bar"));
  };
  Visitor.runOver("namespace a { void foo(); }\nvoid f() { a::foo(); }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("a::bar", replaceCallExpr(Expr, "::a::bar"));
  };
  Visitor.runOver("namespace a { int foo(); }\nauto f = a::foo();\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("bar", replaceCallExpr(Expr, "::a::bar"));
  };
  Visitor.runOver(
      "namespace a { int foo(); }\nusing a::foo;\nauto f = foo();\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("c::bar", replaceCallExpr(Expr, "::a::c::bar"));
  };
  Visitor.runOver("namespace a { namespace b { void foo(); } }\n"
                  "namespace a { namespace b { namespace {"
                  "void f() { foo(); }"
                  "} } }\n");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("x::bar", replaceCallExpr(Expr, "::a::x::bar"));
  };
  Visitor.runOver("namespace a { namespace b { void foo(); } }\n"
                  "namespace a { namespace b { namespace c {"
                  "void f() { foo(); }"
                  "} } }\n");

  // If the shortest name is ambiguous, we need to add more qualifiers.
  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("::a::y::bar", replaceCallExpr(Expr, "::a::y::bar"));
  };
  Visitor.runOver(R"(
    namespace a {
    namespace b {
    namespace x { void foo() {} }
    namespace y { void foo() {} }
    }
    }

    namespace a {
    namespace b {
    void f() { x::foo(); }
    }
    })");

  Visitor.OnCall = [&](CallExpr *Expr) {
    EXPECT_EQ("y::bar", replaceCallExpr(Expr, "::y::bar"));
  };
  Visitor.runOver(R"(
    namespace a {
    namespace b {
    namespace x { void foo() {} }
    namespace y { void foo() {} }
    }
    }

    void f() { a::b::x::foo(); }
    )");
}

TEST(LookupTest, qualifyFunctionNameInContext) {
  GetDeclsVisitor Visitor;

  auto replaceCallExpr = [&](const CallExpr *Expr,
                             tooling::RenameOptions Opts) {
    assert(Expr->getCalleeDecl());
    const FunctionDecl *FD = dyn_cast<FunctionDecl>(Expr->getCalleeDecl());
    assert(FD);
    return tooling::qualifyFunctionNameInContext(
        Visitor.DeclStack.back()->getDeclContext(), FD, Opts);
  };

  const char *Header = R"cpp(
  namespace NS {
    struct T {};
    void f1(T);
    inline namespace IS {
      void f2(T);
    }
    namespace {
       void f3(T) {}
    }
  }
  namespace L1 { inline namespace L2 {
    namespace LL1 {
       struct T {};
       void f4(T);
    }
  }}
)cpp";

  using NamespaceList = std::vector<std::string>;

  std::string AdditionalHeader;

  auto MkCall = [&](StringRef S, const NamespaceList &Namespaces) {
    std::string R = Header;
    R += AdditionalHeader;
    R += "\n";
    for (StringRef NS : Namespaces) {
      if (NS.consume_front("inline ")) {
        R += "inline ";
      }
      R += "namespace ";
      R += NS;
      R += " {\n";
    }

    R += "void test() { \n";
    R += S;
    R += "\n}\n";

    for (unsigned I = 0; I < Namespaces.size(); ++I)
      R += "} \n";

    return R;
  };

  tooling::RenameOptions Opts;

#define TC(Expect, Code, Namespaces)                                           \
  do {                                                                         \
    auto OptsCp = Opts;                                                        \
    Visitor.OnCall = [&, OptsCp](CallExpr *Expr) {                             \
      EXPECT_EQ(Expect, replaceCallExpr(Expr, OptsCp));                        \
    };                                                                         \
    Visitor.runOver(MkCall(Code, Namespaces), GetDeclsVisitor::Lang_CXX14);    \
  } while (false)

  // auto TC = [&](const char *Expect, StringRef Code, const NamespaceList
  // &Namespaces) {
  //
  //};

  NamespaceList TopLevel;
  NamespaceList NS{"NS"};
  NamespaceList NS_IS{"NS", "inline IS"};
  NamespaceList NS_NS2{"NS", "Nested"};
  NamespaceList L1{"L1"};
  NamespaceList L1_L2{"L1", "inline L2"};
  NamespaceList L1_L2_L3{"L1", "inline L2", "L3"};

  TC("NS::f1", "NS::T t; f1(t);", TopLevel);

  Opts.OmitInlineNamespaces = true;
  TC("NS::f2", "NS::T t; f2(t);", TopLevel);
  TC("NS::f2", "NS::T t; f2(t);", NS);

  Opts.OmitInlineNamespaces = false;
  TC("NS::IS::f2", "NS::T t; f2(t);", TopLevel);
  TC("IS::f2", "NS::T t; f2(t);", NS);

  TC("NS::f3", "NS::T t; f3(t);", NS);

  TC("NS::f1", "T t; f1(t);", NS);
  TC("NS::f1", "T t; f1(t);", NS_IS);

  TC("LL1::f4", "LL1::T t; f4(t);", L1_L2_L3);

  AdditionalHeader = R"cpp(
namespace CXX {
  struct MyT {
      void foo() const;
      void bar() { foo(); }
  };
}
)cpp";
  TC("CXX::MyT::foo", "", TopLevel);
}

} // end anonymous namespace
