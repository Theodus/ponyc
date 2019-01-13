#!/bin/bash

# https://github.com/ponylang/ponyc/issues/267

# TODO:
# - guard all relevant ast_id switch statement defaults
# - use TK_TYPEALIAS as resolved branch
# - use alias for error messages
# - remove `case TK_TYPEALIAS: pony_assert(0)`
# - use alias for docgen
# - allow finite recursive type aliases

make -f Makefile-lib-llvm config=debug lto=yes LTO_PLUGIN=/usr/lib/LLVMgold.so -j8

if [[ $? == 0 ]]; then
  # ./build/debug/libponyc.tests --gtest_filter=BadPonyTest.*

  # name,flatten,traits,docs,refer,expr,verify
  # ./build/debug/ponyc -r refer packages/stdlib
  # ./build/debug/ponyc -r expr packages/builtin

  ./build/debug/libponyc.tests -j8 \
    --gtest_filter=-BadPonyTest.CoerceUninferredNumericLiteralThroughTypeArgWithViewpointCodegenTest.BoxBoolAsUnionIntoTuple:BadPonyTest.CoerceUninferredNumericLiteralThroughTypeArgWithViewpoint:CodegenTest.BoxBoolAsUnionIntoTuple:CodegenTest.StringSerialization:CodegenTest.CustomSerialization:CodegenTest.CycleDetector:SignatureTest.SerialiseSignature:LiteralTest.*Generic*:CompilerSerialisationTest.SerialiseAfterSugar:CompilerSerialisationTest.*:ReachTest.Determinism

fi
