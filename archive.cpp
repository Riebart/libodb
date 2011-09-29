#include "archive.hpp"

#include "common.hpp"

AppendOnlyFile::~AppendOnlyFile()
{
    fclose(data);
    fclose(index);

    free(data_name);
    free(index_name);
}

AppendOnlyFile::AppendOnlyFile(char* base_filename)
{
    SAFE_MALLOC(char*, data_name, strlen(base_filename)+5);
    SAFE_MALLOC(char*, index_name, strlen(base_filename)+5);

    memcpy(data_name, base_filename, strlen(base_filename));
    strcat(data_name, ".dat");
    memcpy(index_name, base_filename, strlen(base_filename));
    strcat(index_name, ".ind");

    data = fopen(data_name, "ab");
    index = fopen(index_name, "ab");

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
