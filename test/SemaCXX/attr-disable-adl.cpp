// RUN: %clang_cc1 -std=c++17 -fsyntax-only -Wall -verify %s

#define MY_PUSH _Pragma("clang attribute push ([[clang::disable_adl]], apply_to = any(function))")
#define MY_POP _Pragma("clang attribute pop")

// FIXME(EricWF): Improve this diagnostic
MY_PUSH // expected-note 1 {{disable_if declared here}}

    namespace NS {
  struct T {};
  void f(T); // expected-note 1 {{'f' declared here}}
  [[clang::enable_adl]] inline void g(T);
  inline void g(T) {}
}
MY_POP

namespace NS2 {
struct T {};
[[clang::disable_adl]] // expected-note 1 {{declared here}}
void
f(T) {} // expected-note 1 {{declared here}}
} // namespace NS2

void test() {
  NS::T t;
  f(t); // expected-warning {{call to function using ADL is not allowed}}
  g(t);

  NS2::T t2;
  f(t2); // expected-warning {{call to function using ADL is not allowed}}
}
