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

#include "archive.hpp"

#include "common.hpp"

AppendOnlyFile::~AppendOnlyFile()
{
    fclose(data);
    fclose(index);

    free(data_name);
    free(index_name);
}

AppendOnlyFile::AppendOnlyFile(char* base_filename, bool append)
{
    SAFE_MALLOC(char*, data_name, strlen(base_filename)+5);
    SAFE_MALLOC(char*, index_name, strlen(base_filename)+5);

    memcpy(data_name, base_filename, strlen(base_filename));
    strcat(data_name, ".dat");
    memcpy(index_name, base_filename, strlen(base_filename));
    strcat(index_name, ".ind");

    if (append)
    {
        data = fopen(data_name, "ab");
        index = fopen(index_name, "ab");
    }
    else
    {
        data = fopen(data_name, "wb");
        index = fopen(index_name, "wb");
    }

    cond = NULL;
    offset = 0;
}

inline bool AppendOnlyFile::write(void* rawdata, uint32_t datalen)
{
    if ((cond != NULL) && (cond->condition(rawdata)))
    {
        return false;
    }

    if (fwrite(rawdata, datalen, 1, data))
    {
        if (fwrite(&offset, sizeof(uint64_t), 1, index))
        {
            offset += datalen;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}
