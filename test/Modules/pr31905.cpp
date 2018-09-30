// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: cp -R %S/Inputs/PR31905/my-project-root/ %t/other-include
// RUN: %clang_cc1 -std=c++11 -isystem %S/Inputs/PR31905/system-dir \
// RUN:    -I%S/Inputs/PR31905/my-project-root -I%t/other-include \
// RUN:    -fmodules -fmodules-local-submodule-visibility \
// RUN:    -fimplicit-module-maps -fmodules-cache-path=%t  -verify %s

#include "my-project/my-header.h"

int main() {} // expected-no-diagnostics
