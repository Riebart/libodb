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

void handler(void* context, uint32_t length, void* data)
{
    struct hc* ctxt = (struct hc*)context;
    while (len(g_i) == 0)
    {
        g_i++;
    }
    
    while (len(ctxt->i) == 0)
    {
        ctxt->i++;
    }
    
    assert(data != NULL);
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

TEST_BEGIN(10)
{
    DataCollator* data = new DataCollator();
    char c[256];
    
    data->add_data(0, 128, c);
    data->add_data(256, 128, c);
    
    try
    {
        data->add_data(64, 128, c);
    }
    catch (int e)
    {
        assert(e == -2);
        delete data;
        return EXIT_SUCCESS;
    }
    
    delete data;
    return EXIT_FAILURE;
}

TEST_BEGIN(11)
{
    DataCollator* data = new DataCollator();
    char c[256];
    
    data->add_data(0, 128, c);
    data->add_data(256, 128, c);
    
    try
    {
        data->add_data(320, 128, c);
    }
    catch (int e)
    {
        assert(e == -2);
        delete data;
        return EXIT_SUCCESS;
    }
    
    delete data;
    return EXIT_FAILURE;
}

TEST_BEGIN(12)
{
    DataCollator* data = new DataCollator();
    char c[256];
    
    data->add_data(0, 128, c);
    data->add_data(256, 128, c);
    
    try
    {
        data->add_data(64, 256, c);
    }
    catch (int e)
    {
        assert(e == -2);
        delete data;
        return EXIT_SUCCESS;
    }
    
    delete data;
    return EXIT_FAILURE;
}

TEST_BEGIN(13)
{
    DataCollator* data = new DataCollator();
    char c[256];
    
    data->add_data(0, 128, c);
    data->add_data(256, 128, c);
    
    try
    {
        data->add_data(-1, 129, c);
    }
    catch (int e)
    {
        assert(e == -2);
        delete data;
        return EXIT_SUCCESS;
    }
    
    delete data;
    return EXIT_FAILURE;
}

TEST_BEGIN(14)
{
    DataCollator* data = new DataCollator();
    char c[256];
    
    data->add_data(0, 128, c);
    data->add_data(256, 128, c);
    
    try
    {
        data->add_data(64, 32, c);
    }
    catch (int e)
    {
        assert(e == -2);
        delete data;
        return EXIT_SUCCESS;
    }
    
    delete data;
    return EXIT_FAILURE;
}

TEST_CASES_END()
