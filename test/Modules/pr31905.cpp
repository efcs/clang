// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: cp -R %S/Inputs/PR31905 %t/other-include
// RUN: %clang_cc1 -std=c++11 -I%S/Inputs/PR31905/ -I%t/other-include -fmodules -fmodules-local-submodule-visibility \
// RUN:    -fimplicit-module-maps -fmodules-cache-path=%t  -verify %s

#include "my-project/my-header.h"

int main() {} // expected-no-diagnostics
