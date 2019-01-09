// RUN: %clang_cc1 -std=c++17 -fsyntax-only -Wall -verify %s

#define BEGIN_DISABLE_ADL _Pragma("clang attribute push ([[clang::no_adl]], apply_to = function(is_non_member))")
#define END_DISABLE_ADL _Pragma("clang attribute pop")

namespace test_manual_decl {
  struct T {};
  [[clang::no_adl]] void foo(T); // OK
  [[clang::no_adl]] void foo(T); // OK
  [[clang::no_adl]] void foo(T) {} // OK

  void on_redecl(T);
  [[clang::allow_adl]] void on_redecl(T); // expected-error {{'allow_adl' attribute does not appear on the first declaration of 'on_redecl'}}

  [[clang::no_adl]] void bad_redecl(T);
  [[clang::allow_adl]] void bad_redecl(T); // expected-error {{ADL attribute does not match previous declaration}}

  struct CXXClass {
    [[clang::no_adl]]  // expected-warning {{'no_adl' attribute only applies to non-member functions}}
    static void foo();

    [[clang::allow_adl]]  // expected-warning {{'allow_adl' attribute only applies to non-member functions}}
    void bar() const {}

    [[clang::no_adl]]  // expected-warning {{'no_adl' attribute only applies to non-member functions}}
    void operator()();
  };
} // namespace

void do_test_manual_decl() {
  test_manual_decl::T t;
  foo(t);
}

BEGIN_DISABLE_ADL
namespace test_pragma {

struct T {};
[[clang::allow_adl]] void t1(T);
[[clang::no_adl]] void t2(T);
void t3(T);
[[clang::no_adl]] void t3(T);

}
END_DISABLE_ADL


void do_test_pragma() {
  test_pragma::T t;
  t1(t);
  t2(t);
  t3(t);
}

BEGIN_DISABLE_ADL

namespace NS {
struct T {};
void f(T); // expected-note 1 {{'f' declared here}}
[[clang::allow_adl]] inline void g(T);
inline void g(T) {}
} // namespace NS
END_DISABLE_ADL

namespace NS2 {
struct T {};
[[clang::no_adl]] void
f(T) {} // expected-note 1 {{declared here}}
} // namespace NS2

void test() {
  NS::T t;
  f(t); // expected-warning {{call to function using ADL is not allowed}}
  g(t);

  NS2::T t2;
  f(t2); // expected-warning {{call to function using ADL is not allowed}}
}
