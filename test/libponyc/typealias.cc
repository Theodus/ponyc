#include <gtest/gtest.h>
#include <platform.h>

#include "util.h"

#define TEST_COMPILE(src) DO(test_compile(src, "expr"))

class TypealiasTest : public PassTest
{};

TEST_F(TypealiasTest, TODO)
{
   const char* src =
    "primitive A\n"
    "primitive B\n"
    "type AA is A\n"
    "type AB is (AA | B)\n"

    "actor Main\n"
    "  new create(env: Env) =>\n"
    "    None\n"
    ;

  TEST_COMPILE(src);
}
