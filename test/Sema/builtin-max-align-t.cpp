// RUN: %clang_cc1 -fsyntax-only -verify %s -triple i386-pc-unknown
// RUN: %clang_cc1 -fsyntax-only -verify %s -triple x86_64-apple-darwin9
// RUN: %clang_cc1 -fsyntax-only -verify %s -triple x86_64-pc-linux-gnu

// expected-no-diagnostics

#include <stddef.h>

typedef max_align_t MAT;
#if defined(_MSC_VER)
_Static_assert(__is_same(MAT, double), "");
#elif defined(__APPLE__)
_Static_assert(__is_same(MAT, long double), "");
#else

// Define 'max_align_t' to match the GCC definition.
typedef struct {
    long long __clang_max_align_nonce1
        __attribute__((__aligned__(__alignof__(long long))));
    long double __clang_max_align_nonce2
        __attribute__((__aligned__(__alignof__(long double))));
#if defined(__SIZEOF_FLOAT128__)
  __float128 __clang_max_align_nonce3
      __attribute__((__aligned__(__alignof__(__float128))));
#endif
} gnu_max_align_t;

template <class T>
struct Printer;
template <unsigned, unsigned> struct SPrinter;
SPrinter<sizeof(max_align_t), alignof(max_align_t)> p1;

SPrinter<sizeof(gnu_max_align_t), alignof(gnu_max_align_t)> p2;

Printer<__builtin_max_align_t> p;

_Static_assert(sizeof(max_align_t) == sizeof(gnu_max_align_t), "");
_Static_assert(_Alignof(max_align_t) == _Alignof(gnu_max_align_t), "");
_Static_assert(__alignof(max_align_t) == __alignof(gnu_max_align_t), "");

#endif

