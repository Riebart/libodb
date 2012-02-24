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

#include "unittest.hpp"
#include "collator.hpp"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define N 1000
#define len(x) (((x) * 125) % 256)

int g_i = 0;

struct hc
{
    int i;
};

void handler(void* context, uint64_t length, void* data)
{
    assert(((data != NULL) && (length != 0)) || ((data == NULL) && (length == 0)));

    if ((length == 0) && (data == NULL))
    {
        return;
    }

    struct hc* ctxt = (struct hc*)context;
    while (len(g_i) == 0)
    {
        g_i++;
    }

    while (len(ctxt->i) == 0)
    {
        ctxt->i++;
    }

    assert(g_i == ctxt->i);
    assert(length == len(ctxt->i));

    uint8_t* block = (uint8_t*)data;

    for (int j = 0 ; j < len(ctxt->i) ; j++)
    {
        assert(block[j] == (ctxt->i % 256));
    }

    uint32_t num_out = fwrite(block, length, 1, stdout);

    if (len(ctxt->i) > 0)
    {
        assert(num_out == 1);
    }

    ctxt->i++;
    g_i++;
}

char* add_data(DataCollator* data, int i, int32_t offset)
{
    uint32_t len = len(i);
    char* block = (char*)malloc(len);

    if (block == NULL)
    {
        throw -1;
    }

    memset(block, i, len);

    try
    {
        data->add_data(offset, len, block);
        free(block);
    }
    catch (int e)
    {
        switch (e)
        {
        case -1:
            fprintf(stderr, "OOM\n");
            break;

        case -2:
            fprintf(stderr, "OVERLAP\n");
            break;

        default:
            fprintf(stderr, "UNKNOWN\n");
            break;
        }
    }

    return block;
}

void manual_print_verify(DataCollator* data)
{
    for (int i = 0 ; i < N ; i++)
    {
        if (len(i) == 0)
        {
            continue;
        }

        struct DataCollator::row* r = data->get_data();
        uint8_t* block = (uint8_t*)&(r->data);

        assert((r->length) == len(i));
        assert(block != NULL);

        for (int j = 0 ; j < len(i) ; j++)
        {
            assert(block[j] == (i % 256));
        }

        uint32_t num_out = fwrite(block, len(i), 1, stdout);

        if (len(i) > 0)
        {
            assert(num_out == 1);
        }

        free(r);
    }
}

TEST_OPT_PREAMBLE("unit-collator")
TEST_OPT("Destructor, no gaps, no output")
TEST_OPT("Destructor, NULL gaps, no output")
printf("\n  All tests after here should have a stdout sha256 hash of \n      25d407c31b675b62d15d8f14177a67f67f61533e72dd61106f13e3b0cb88daf2\n");
TEST_OPT("Sequential, no gaps, manual output")
TEST_OPT("Sequential, no gaps, file descriptor output")
TEST_OPT("Sequential, no gaps, handler output")
TEST_OPT("2,1 stepping, manual output")
TEST_OPT("Odd/even, manual output")
TEST_OPT("4,2,1,3 gap-introducing, manual output")
TEST_OPT("5,2,3,1,4 gap-filling, manual output")
TEST_OPT("5,2,4,1,3 back gap-filling, manual output")
TEST_OPT("back-to-front, manual output")
TEST_OPT("Overlap test, front only")
TEST_OPT("Overlap test, back only")
TEST_OPT("Overlap test, front and back")
TEST_OPT("Overlap test, superset")
TEST_OPT("Overlap test, subset")
TEST_OPT("Overlap test, hanging off both ends, and overlapping everything")
TEST_OPT("Overlap test, ending before the start offset")
TEST_OPT_END()

TEST_CASES_BEGIN()
TEST_BEGIN(0)
{
    DataCollator* data = new DataCollator();
    uint32_t offset = 0;

    for (int i = 0 ; i < N ; i++)
    {
        add_data(data, i, offset);
        offset += len(i);
    }

    delete data;
    return EXIT_SUCCESS;
}

TEST_BEGIN(1)
{
    DataCollator* data = new DataCollator();
    uint32_t offset = 0;

    for (int i = 0 ; i < N ; i++)
    {
        add_data(data, i, offset);
        offset += 2 * len(i);
    }

    delete data;
    return EXIT_SUCCESS;
}

TEST_BEGIN(2)
{
    DataCollator* data = new DataCollator();
    uint32_t offset = 0;

    for (int i = 0 ; i < N ; i++)
    {
        add_data(data, i, offset);
        offset += len(i);
    }

    manual_print_verify(data);

    delete data;
    return EXIT_SUCCESS;
}

TEST_BEGIN(3)
{
    DataCollator* data = new DataCollator(stdout);
    uint32_t offset = 0;

    for (int i = 0 ; i < N ; i++)
    {
        assert(data->get_data() == NULL);
        add_data(data, i, offset);
        offset += len(i);
    }

    delete data;
    return EXIT_SUCCESS;
}

TEST_BEGIN(4)
{
    g_i = 0;
    struct hc ctxt;
    ctxt.i = 0;

    DataCollator* data = new DataCollator(handler, &ctxt);
    uint32_t offset = 0;

    for (int i = 0 ; i < N ; i++)
    {
        add_data(data, i, offset);
        offset += len(i);
    }

    delete data;
    return EXIT_SUCCESS;
}

TEST_BEGIN(5)
{
    DataCollator* data = new DataCollator();
    uint32_t offset = 0;

    for (int i = 0 ; i < N ; i += 2)
    {
        add_data(data, i+1, offset + len(i));
        add_data(data, i, offset);
        offset += len(i) + len(i+1);
    }

    manual_print_verify(data);

    delete data;
    return EXIT_SUCCESS;
}

TEST_BEGIN(6)
{
    DataCollator* data = new DataCollator();
    uint32_t offset = 0;

    for (int i = 0 ; i < N ; i += 2)
    {
        add_data(data, i, offset);
        offset += len(i) + len(i+1);
    }

    offset = 0;
    for (int i = 1 ; i < N ; i += 2)
    {
        add_data(data, i, offset + len(i-1));
        offset += len(i) + len(i-1);
    }

    manual_print_verify(data);

    delete data;
    return EXIT_SUCCESS;
}

TEST_BEGIN(7)
{
    DataCollator* data = new DataCollator();
    uint32_t offset = 0;

    for (int i = 0 ; i < N ; i += 4)
    {
        add_data(data, i+3, offset + len(i) + len(i+1) + len(i+2));
        add_data(data, i+1, offset + len(i));
        add_data(data, i, offset);
        add_data(data, i+2, offset + len(i) + len(i+1));
        offset += len(i) + len(i+1) + len(i+2) + len(i+3);
    }

    manual_print_verify(data);

    delete data;
    return EXIT_SUCCESS;
}

TEST_BEGIN(8)
{
    DataCollator* data = new DataCollator();
    uint32_t offset = 0;

    for (int i = 0 ; i < N ; i += 5)
    {
        add_data(data, i+4, offset + len(i) + len(i+1) + len(i+2) + len(i+3));
        add_data(data, i+1, offset + len(i));
        add_data(data, i+2, offset + len(i) + len(i+1));
        add_data(data, i, offset);
        add_data(data, i+3, offset + len(i) + len(i+1) + len(i+2));
        offset += len(i) + len(i+1) + len(i+2) + len(i+3) + len(i+4);
    }

    manual_print_verify(data);

    delete data;
    return EXIT_SUCCESS;
}

TEST_BEGIN(9)
{
    DataCollator* data = new DataCollator();
    uint32_t offset = 0;

    for (int i  = 0 ; i < N-1 ; i++)
    {
        offset += len(i);
    }

    for (int i = N-1 ; i >= 0 ; i--)
    {
        add_data(data, i, offset);
        if (i > 0)
        {
            offset -= len(i-1);
        }
    }

    manual_print_verify(data);

    delete data;
    return EXIT_SUCCESS;
}

#define TEST_OVERLAP(n, s, a, g, b, i, l) TEST_BEGIN(n)\
{\
    DataCollator* data = new DataCollator();\
    char c[512];\
\
    data->add_data(s, a, c);\
    data->add_data(s+a+g, b, c);\
\
    data->add_data(i, l, c);\
\
    delete data;\
    return EXIT_SUCCESS;\
}

// ="TEST_OVERLAP(" & C2 & "," & D2 & "," & E2 & "," & F2 & "," & G2 & "," & I2 & "," & H2 & ");"
// ="add_test(unit-collator." & C2 & " test-output """" """" ""./unit-collator"" """ & C2 & """)"

TEST_OVERLAP(10,0,10,10,10,-11,10);
TEST_OVERLAP(11,0,10,10,10,-10,10);
TEST_OVERLAP(12,0,10,10,10,-1,10);
TEST_OVERLAP(13,0,10,10,10,0,10);
TEST_OVERLAP(14,0,10,10,10,1,10);
TEST_OVERLAP(15,0,10,10,10,10,10);
TEST_OVERLAP(16,0,10,10,10,11,10);
TEST_OVERLAP(17,0,10,10,10,20,10);
TEST_OVERLAP(18,0,10,10,10,21,10);
TEST_OVERLAP(19,0,10,10,10,30,10);
TEST_OVERLAP(20,0,10,10,10,31,10);
TEST_OVERLAP(21,0,10,15,10,-11,10);
TEST_OVERLAP(22,0,10,15,10,-10,10);
TEST_OVERLAP(23,0,10,15,10,-1,10);
TEST_OVERLAP(24,0,10,15,10,0,10);
TEST_OVERLAP(25,0,10,15,10,1,10);
TEST_OVERLAP(26,0,10,15,10,10,10);
TEST_OVERLAP(27,0,10,15,10,11,10);
TEST_OVERLAP(28,0,10,15,10,15,10);
TEST_OVERLAP(29,0,10,15,10,16,10);
TEST_OVERLAP(30,0,10,15,10,25,10);
TEST_OVERLAP(31,0,10,15,10,26,10);
TEST_OVERLAP(32,0,10,15,10,35,10);
TEST_OVERLAP(33,0,10,15,10,36,10);
TEST_OVERLAP(34,0,10,5,10,-11,10);
TEST_OVERLAP(35,0,10,5,10,-10,10);
TEST_OVERLAP(36,0,10,5,10,-1,10);
TEST_OVERLAP(37,0,10,5,10,0,10);
TEST_OVERLAP(38,0,10,5,10,1,10);
TEST_OVERLAP(39,0,10,5,10,5,10);
TEST_OVERLAP(40,0,10,5,10,6,10);
TEST_OVERLAP(41,0,10,5,10,10,10);
TEST_OVERLAP(42,0,10,5,10,11,10);
TEST_OVERLAP(43,0,10,5,10,15,10);
TEST_OVERLAP(44,0,10,5,10,16,10);
TEST_OVERLAP(45,0,10,5,10,25,10);
TEST_OVERLAP(46,0,10,5,10,26,10);
TEST_OVERLAP(47,0,10,10,10,-6,5);
TEST_OVERLAP(48,0,10,10,10,-5,5);
TEST_OVERLAP(49,0,10,10,10,-1,5);
TEST_OVERLAP(50,0,10,10,10,0,5);
TEST_OVERLAP(51,0,10,10,10,1,5);
TEST_OVERLAP(52,0,10,10,10,5,5);
TEST_OVERLAP(53,0,10,10,10,6,5);
TEST_OVERLAP(54,0,10,10,10,10,5);
TEST_OVERLAP(55,0,10,10,10,11,5);
TEST_OVERLAP(56,0,10,10,10,15,5);
TEST_OVERLAP(57,0,10,10,10,16,5);
TEST_OVERLAP(58,0,10,10,10,20,5);
TEST_OVERLAP(59,0,10,10,10,21,5);
TEST_OVERLAP(60,0,10,10,10,25,5);
TEST_OVERLAP(61,0,10,10,10,26,5);
TEST_OVERLAP(62,0,10,10,10,30,5);
TEST_OVERLAP(63,0,10,10,10,31,5);
TEST_OVERLAP(64,0,10,10,10,-16,15);
TEST_OVERLAP(65,0,10,10,10,-15,15);
TEST_OVERLAP(66,0,10,10,10,-5,15);
TEST_OVERLAP(67,0,10,10,10,-4,15);
TEST_OVERLAP(68,0,10,10,10,-1,15);
TEST_OVERLAP(69,0,10,10,10,0,15);
TEST_OVERLAP(70,0,10,10,10,5,15);
TEST_OVERLAP(71,0,10,10,10,6,15);
TEST_OVERLAP(72,0,10,10,10,10,15);
TEST_OVERLAP(73,0,10,10,10,11,15);
TEST_OVERLAP(74,0,10,10,10,15,15);
TEST_OVERLAP(75,0,10,10,10,16,15);
TEST_OVERLAP(76,0,10,10,10,20,15);
TEST_OVERLAP(77,0,10,10,10,21,15);
TEST_OVERLAP(78,0,10,10,10,30,15);
TEST_OVERLAP(79,0,10,10,10,31,15);
TEST_OVERLAP(80,0,10,15,10,-16,15);
TEST_OVERLAP(81,0,10,15,10,-15,15);
TEST_OVERLAP(82,0,10,15,10,-5,15);
TEST_OVERLAP(83,0,10,15,10,-4,15);
TEST_OVERLAP(84,0,10,15,10,-1,15);
TEST_OVERLAP(85,0,10,15,10,0,15);
TEST_OVERLAP(86,0,10,15,10,1,15);
TEST_OVERLAP(87,0,10,15,10,10,15);
TEST_OVERLAP(88,0,10,15,10,11,15);
TEST_OVERLAP(89,0,10,15,10,25,15);
TEST_OVERLAP(90,0,10,15,10,26,15);
TEST_OVERLAP(91,0,10,15,10,35,15);
TEST_OVERLAP(92,0,10,15,10,36,15);
TEST_OVERLAP(93,0,10,5,10,-6,5);
TEST_OVERLAP(94,0,10,5,10,-5,5);
TEST_OVERLAP(95,0,10,5,10,-4,5);
TEST_OVERLAP(96,0,10,5,10,-1,5);
TEST_OVERLAP(97,0,10,5,10,0,5);
TEST_OVERLAP(98,0,10,5,10,1,5);
TEST_OVERLAP(99,0,10,5,10,5,5);
TEST_OVERLAP(100,0,10,5,10,6,5);
TEST_OVERLAP(101,0,10,5,10,10,5);
TEST_OVERLAP(102,0,10,5,10,11,5);
TEST_OVERLAP(103,0,10,5,10,15,5);
TEST_OVERLAP(104,0,10,5,10,16,5);
TEST_OVERLAP(105,0,10,5,10,25,5);
TEST_OVERLAP(106,0,10,5,10,26,5);
TEST_OVERLAP(107,0,10,8,10,-5,4);
TEST_OVERLAP(108,0,10,8,10,-4,4);
TEST_OVERLAP(109,0,10,8,10,-1,4);
TEST_OVERLAP(110,0,10,8,10,0,4);
TEST_OVERLAP(111,0,10,8,10,1,4);
TEST_OVERLAP(112,0,10,8,10,6,4);
TEST_OVERLAP(113,0,10,8,10,7,4);
TEST_OVERLAP(114,0,10,8,10,10,4);
TEST_OVERLAP(115,0,10,8,10,11,4);
TEST_OVERLAP(116,0,10,8,10,14,4);
TEST_OVERLAP(117,0,10,8,10,15,4);
TEST_OVERLAP(118,0,10,8,10,18,4);
TEST_OVERLAP(119,0,10,8,10,19,4);
TEST_OVERLAP(120,0,10,8,10,24,4);
TEST_OVERLAP(121,0,10,8,10,25,4);
TEST_OVERLAP(122,0,10,8,10,28,4);
TEST_OVERLAP(123,0,10,8,10,29,4);
TEST_OVERLAP(124,0,10,4,10,-9,8);
TEST_OVERLAP(125,0,10,4,10,-8,8);
TEST_OVERLAP(126,0,10,4,10,-1,8);
TEST_OVERLAP(127,0,10,4,10,0,8);
TEST_OVERLAP(128,0,10,4,10,1,8);
TEST_OVERLAP(129,0,10,4,10,2,8);
TEST_OVERLAP(130,0,10,4,10,3,8);
TEST_OVERLAP(131,0,10,4,10,6,8);
TEST_OVERLAP(132,0,10,4,10,7,8);
TEST_OVERLAP(133,0,10,4,10,10,8);
TEST_OVERLAP(134,0,10,4,10,11,8);
TEST_OVERLAP(135,0,10,4,10,14,8);
TEST_OVERLAP(136,0,10,4,10,15,8);
TEST_OVERLAP(137,0,10,4,10,16,8);
TEST_OVERLAP(138,0,10,4,10,17,8);
TEST_OVERLAP(139,0,10,4,10,24,8);
TEST_OVERLAP(140,0,10,4,10,25,8);
TEST_OVERLAP(141,10,10,10,10,-1,10);
TEST_OVERLAP(142,10,10,10,10,0,10);
TEST_OVERLAP(143,10,10,10,10,9,10);
TEST_OVERLAP(144,10,10,10,10,10,10);
TEST_OVERLAP(145,10,10,10,10,11,10);
TEST_OVERLAP(146,10,10,10,10,20,10);
TEST_OVERLAP(147,10,10,10,10,21,10);
TEST_OVERLAP(148,10,10,10,10,30,10);
TEST_OVERLAP(149,10,10,10,10,31,10);
TEST_OVERLAP(150,10,10,10,10,40,10);
TEST_OVERLAP(151,10,10,10,10,41,10);
TEST_OVERLAP(152,10,10,10,10,-11,10);
TEST_OVERLAP(153,10,10,10,10,-10,10);
TEST_OVERLAP(154,5,10,15,10,-6,10);
TEST_OVERLAP(155,5,10,15,10,-5,10);
TEST_OVERLAP(156,5,10,15,10,4,10);
TEST_OVERLAP(157,5,10,15,10,5,10);
TEST_OVERLAP(158,5,10,15,10,6,10);
TEST_OVERLAP(159,5,10,15,10,15,10);
TEST_OVERLAP(160,5,10,15,10,16,10);
TEST_OVERLAP(161,5,10,15,10,20,10);
TEST_OVERLAP(162,5,10,15,10,21,10);
TEST_OVERLAP(163,5,10,15,10,30,10);
TEST_OVERLAP(164,5,10,15,10,31,10);
TEST_OVERLAP(165,5,10,15,10,40,10);
TEST_OVERLAP(166,5,10,15,10,41,10);
TEST_OVERLAP(167,5,10,5,10,-11,10);
TEST_OVERLAP(168,5,10,5,10,-10,10);
TEST_OVERLAP(169,15,10,5,10,4,10);
TEST_OVERLAP(170,15,10,5,10,5,10);
TEST_OVERLAP(171,15,10,5,10,14,10);
TEST_OVERLAP(172,15,10,5,10,15,10);
TEST_OVERLAP(173,15,10,5,10,16,10);
TEST_OVERLAP(174,15,10,5,10,20,10);
TEST_OVERLAP(175,15,10,5,10,21,10);
TEST_OVERLAP(176,15,10,5,10,25,10);
TEST_OVERLAP(177,15,10,5,10,26,10);
TEST_OVERLAP(178,15,10,5,10,30,10);
TEST_OVERLAP(179,15,10,5,10,31,10);
TEST_OVERLAP(180,15,10,5,10,40,10);
TEST_OVERLAP(181,15,10,5,10,41,10);
TEST_OVERLAP(182,15,15,10,5,-11,10);
TEST_OVERLAP(183,15,15,10,5,-10,10);
TEST_OVERLAP(184,5,10,10,10,-11,15);
TEST_OVERLAP(185,5,10,10,10,-10,15);
TEST_OVERLAP(186,5,10,10,10,0,15);
TEST_OVERLAP(187,5,10,10,10,1,15);
TEST_OVERLAP(188,5,10,10,10,4,15);
TEST_OVERLAP(189,5,10,10,10,5,15);
TEST_OVERLAP(190,5,10,10,10,10,15);
TEST_OVERLAP(191,5,10,10,10,11,15);
TEST_OVERLAP(192,5,10,10,10,15,15);
TEST_OVERLAP(193,5,10,10,10,16,15);
TEST_OVERLAP(194,5,10,10,10,20,15);
TEST_OVERLAP(195,5,10,10,10,21,15);
TEST_OVERLAP(196,5,10,10,10,25,15);
TEST_OVERLAP(197,5,10,10,10,26,15);
TEST_OVERLAP(198,5,10,10,10,35,15);
TEST_OVERLAP(199,5,10,10,10,36,15);
TEST_OVERLAP(200,5,10,10,10,-16,15);
TEST_OVERLAP(201,5,10,10,10,-15,15);
TEST_OVERLAP(202,25,10,15,10,9,15);
TEST_OVERLAP(203,25,10,15,10,10,15);
TEST_OVERLAP(204,25,10,15,10,20,15);
TEST_OVERLAP(205,25,10,15,10,21,15);
TEST_OVERLAP(206,25,10,15,10,24,15);
TEST_OVERLAP(207,25,10,15,10,25,15);
TEST_OVERLAP(208,25,10,15,10,26,15);
TEST_OVERLAP(209,25,10,15,10,35,15);
TEST_OVERLAP(210,25,10,15,10,36,15);
TEST_OVERLAP(211,25,10,15,10,50,15);
TEST_OVERLAP(212,25,10,15,10,51,15);
TEST_OVERLAP(213,25,10,15,10,60,15);
TEST_OVERLAP(214,25,10,15,10,61,15);
TEST_OVERLAP(215,25,10,15,10,-16,15);
TEST_OVERLAP(216,25,10,15,10,-15,15);
TEST_OVERLAP(217,5,10,10,10,-36,35);
TEST_OVERLAP(218,5,10,10,10,-35,35);
TEST_OVERLAP(219,5,10,10,10,-34,35);
TEST_OVERLAP(220,5,10,10,10,-30,35);
TEST_OVERLAP(221,5,10,10,10,-31,35);
TEST_OVERLAP(222,5,10,10,10,-20,35);
TEST_OVERLAP(223,5,10,10,10,-21,35);
TEST_OVERLAP(224,5,10,10,10,-10,35);
TEST_OVERLAP(225,5,10,10,10,-9,35);
TEST_OVERLAP(226,5,10,10,10,0,35);
TEST_OVERLAP(227,5,10,10,10,1,35);
TEST_OVERLAP(228,5,10,10,10,5,35);
TEST_OVERLAP(229,5,10,10,10,6,35);
TEST_OVERLAP(230,5,10,10,10,10,35);
TEST_OVERLAP(231,5,10,10,10,11,35);
TEST_OVERLAP(232,5,10,10,10,20,35);
TEST_OVERLAP(233,5,10,10,10,21,35);
TEST_OVERLAP(234,5,10,10,10,30,35);
TEST_OVERLAP(235,5,10,10,10,31,35);

TEST_CASES_END()
