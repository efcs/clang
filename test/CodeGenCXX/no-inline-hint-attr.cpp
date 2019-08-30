// RUN: %clang_cc1 %s -std=c++11 -triple=x86_64-linux -O1 -finline-functions -emit-llvm -disable-llvm-passes -o - | FileCheck -allow-deprecated-dag-overlap %s

void normal_func() {}

// CHECK: @_Z11inline_funcv() [[INLINE_ATTR:#[0-9]+]]
inline void inline_func() {}
// CHECK: @_ZL13noinline_funcv() [[NOINLINE_ATTR:#[0-9]+]]
__attribute__((noinline))
static void noinline_func() {}

struct A {
  void start_pos() {}

  void implicit_inline() {  }
  [[clang::no_inline_hint]]
  void implicit_inline_no_hint() { }

  inline void explicit_inline();
  [[clang::no_inline_hint]]
  inline void explicit_inline_no_hint();
};

void A::explicit_inline() {  }
void A::explicit_inline_no_hint() {}

struct Extern;
struct Implicit;
template <class T>
struct B {
  void implicit_inline() {}
  [[clang::no_inline_hint]]
  void implicit_inline_no_hint() {}

  inline void explicit_inline();
  [[clang::no_inline_hint]]
  inline void explicit_inline_no_hint();
};
template <class T>
void B<T>::explicit_inline() {}
template <class T>
void B<T>::explicit_inline_no_hint() {}
extern template struct B<Extern>;

void test()
{
  inline_func();
  noinline_func();
  normal_func();
  {
  A a;
  // CHECK: define{{.*}}@_ZN1A9start_posEv
  a.start_pos();

  // CHECK: define{{.*}}implicit_inline{{.*}} [[NO_ATTR:#[0-9]+]]
  a.implicit_inline();
  // CHECK: define{{.*}}implicit_inline_no_hint{{.*}} [[NO_ATTR]]
  a.implicit_inline_no_hint();
  // CHECK: define{{.*}}explicit_inline{{.*}} [[INLINE_ATTR]]
  a.explicit_inline();
  // CHECK: define{{.*}}explicit_inline_no_hint{{.*}} [[NO_ATTR]]
  a.explicit_inline_no_hint();
  }
  {
  B<Extern> a;
  // CHECK: define available_externally{{.*}}implicit_inline{{.*}} [[NO_ATTR]]
  a.implicit_inline();
  // CHECK: define available_externally{{.*}}implicit_inline_no_hint{{.*}} [[NO_ATTR]]
  a.implicit_inline_no_hint();
  // CHECK: define available_externally{{.*}}explicit_inline{{.*}} [[INLINE_ATTR]]
  a.explicit_inline();
  // CHECK: define available_externally{{.*}}explicit_inline_no_hint{{.*}} [[NO_ATTR]]
  a.explicit_inline_no_hint();
  }
  {
  B<Implicit> a;
  // CHECK: define{{.*}}Implicit{{.*}}implicit_inline{{.*}} [[NO_ATTR]]
  a.implicit_inline();
  // CHECK: define{{.*}}Implicit{{.*}}implicit_inline_no_hint{{.*}} [[NO_ATTR]]
  a.implicit_inline_no_hint();
  // CHECK: define{{.*}}Implicit{{.*}}explicit_inline{{.*}} [[INLINE_ATTR]]
  a.explicit_inline();
  // CHECK: define{{.*}}Implicit{{.*}}explicit_inline_no_hint{{.*}} [[NO_ATTR]]
  a.explicit_inline_no_hint();
  }
}


// CHECK-NOT: attributes
// CHECK-DAG: attributes [[NO_ATTR]] = {
// CHECK-NOT:inline
// CHECK-SAME:}
// CHECK-DAG: attributes [[NOINLINE_ATTR]] =  { {{.*}}noinline{{.*}} }
// CHECK-DAG: attributes [[INLINE_ATTR]] = { {{.*}}inlinehint{{.*}} }



