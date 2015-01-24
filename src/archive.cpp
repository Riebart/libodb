/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2013 Michael Himbeault and Travis Friesen
 *
 */

/// Source file for implementation of AppendOnlyFile.
/// @file archive.cpp

#include "archive.hpp"
#include "common.hpp"

namespace libodb
{

    Archive::~Archive()
    {
    }

    AppendOnlyFile::~AppendOnlyFile()
    {
        fclose(data);
        fclose(index);

        free(data_name);
        free(index_name);
    }

    //! @todo Make this use a biffered writer for improved throughput
    AppendOnlyFile::AppendOnlyFile(char* base_filename, bool append)
    {
        size_t base_fname_len = strlen(base_filename);

        // The strcat call incurs a conditional jump on unitialized value from here
        // according to valgrind. So calloc() it.
        SAFE_CALLOC(char*, data_name, 1, base_fname_len + 5);
        SAFE_CALLOC(char*, index_name, 1, base_fname_len + 5);

        memcpy(data_name, base_filename, base_fname_len);
        memcpy(index_name, base_filename, base_fname_len);

#ifdef WIN32
        strncat_s(data_name, base_fname_len + 5, (char*)".dat", 4);
        strncat_s(index_name, base_fname_len + 5, (char*)".ind", 4);
#else
        strcat(data_name, ".dat");
        strcat(index_name, ".ind");
#endif

        if (append)
        {
#ifdef WIN32
            int r;
            r = fopen_s(&data, data_name, "ab");
            if (r != 0)
            {
                fprintf(stderr, "UNABLE TO OPEN FILE \"%s\"\n", index_name);
                return;
            }

            r = fopen_s(&index, index_name, "ab");
            if (r != 0)
            {
                fprintf(stderr, "UNABLE TO OPEN FILE \"%s\"\n", index_name);
                return;
            }
#else
            data = fopen(data_name, "ab");
            index = fopen(index_name, "ab");
#endif

            // Since we're appending, seek to the end, get the position, and then
            // rewind back to the start.
            fseek(data, 0, SEEK_END);
            offset = ftell(data);
            rewind(data);
        }
        else
        {
#ifdef WIN32
            int r;
            r = fopen_s(&data, data_name, "wb");
            if (r != 0)
            {
                fprintf(stderr, "UNABLE TO OPEN FILE \"%s\"\n", index_name);
                return;
            }

            r = fopen_s(&index, index_name, "wb");
            if (r != 0)
            {
                fprintf(stderr, "UNABLE TO OPEN FILE \"%s\"\n", index_name);
                return;
            }
#else
            data = fopen(data_name, "wb");
            index = fopen(index_name, "wb");
#endif

            offset = 0;
        }

        cond = NULL;
    }

    inline bool AppendOnlyFile::write(void* rawdata, uint64_t datalen)
    {
        if ((cond != NULL) && (cond->condition(rawdata)))
        {
            return false;
        }

        if (fwrite(rawdata, (size_t)datalen, 1, data))
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

}
