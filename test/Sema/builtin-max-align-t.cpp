// RUN: %clang_cc1 -fsyntax-only -verify %s -triple i386-pc-unknown
// RUN: %clang_cc1 -fsyntax-only -verify %s -triple x86_64-apple-darwin9
// RUN: %clang_cc1 -fsyntax-only -verify %s -triple x86_64-pc-linux-gnu

// expected-no-diagnostics

typedef max_align_t MAT;
#if defined(_MSC_VER)
_Static_assert(__is_same(MAT, double), "");
#elif defined(__APPLE__)
_Static_assert(__is_same(MAT, long double), "");
#else

#ifdef __SIZEOF_FLOAT128__
_Static_assert(__is_same(MAT, __float128), "");
#else
_Static_assert(__is_same(MAT, double), "");
#endif

#endif

