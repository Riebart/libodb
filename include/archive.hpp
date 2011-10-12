#ifndef ARCHIVER_HPP
#define ARCHIVER_HPP

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "comparator.hpp"

class Archive
{
public:
    virtual bool write(void* rawdata, uint32_t datalen) = 0;
    virtual void set_constraint(Condition* _cond)
    {
        this->cond = _cond;
    };

protected:
    Condition* cond;
};

class AppendOnlyFile : public Archive
{
public:
    ~AppendOnlyFile();
    AppendOnlyFile(char* base_filename, bool append);
    virtual bool write(void* rawdata, uint32_t datalen);

private:
    char* data_name;
    char* index_name;
    FILE* data;
    FILE* index;
    uint64_t offset;
};

#endif
