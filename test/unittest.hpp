/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2012 Michael Himbeault and Travis Friesen
 *
 */

#ifndef UNITTEST_HPP
#define UNITTEST_HPP

#include <stdint.h>

#define TEST_OPT_BEGIN() int main(int argc, char** argv) { if(argc < 2) { ____unit_test_test_arg_count = 0;
#define TEST_OPT_PREAMBLE(bin_name) TEST_OPT_BEGIN(); printf("Usage: " bin_name " <arg>\n");
#define TEST_OPT(desc) printf("    %d: " desc "\n", ____unit_test_test_arg_count++);
#define TEST_OPT_END() return 1; }

#define TEST_CASES_BEGIN() int32_t test_arg; sscanf(argv[1], "%d", &test_arg); switch(test_arg) { case -1: return 1;
#define TEST_BEGIN(n)  break; case n:
#define TEST_CASES_END() break; default: return 1; break; } return 0; }

#define TEST_ARRAY() (*tests)(void)[2] = { test_0, test_1 };

int ____unit_test_test_arg_count = 0;

class UnitTest
{
public:
    static bool test(bool (*test)(void))
    {
        return test();
    }
};

#endif
