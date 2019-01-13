#!/bin/bash

# TODO:
# - guard all relevant ast_id switch statement defaults
# - use TK_TYPEALIAS as resolved branch
# - use alias for error messages
# - allow finite recursive type aliases

make -f Makefile-lib-llvm config=debug lto=yes LTO_PLUGIN=/usr/lib/LLVMgold.so -j8 \
  && ./build/debug/libponyc.tests --gtest_filter=BadPonyTest.*
