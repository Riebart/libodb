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
