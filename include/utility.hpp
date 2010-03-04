#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string.h>

#ifndef LEN_V
#define LEN_V
inline uint32_t len_v(void* rawdata)
{
    return strlen((const char*)rawdata);
}
#endif

inline bool search(std::vector<void*>* marked, void* addr)
{
    int32_t start = 0, end = marked->size() - 1, midpoint = (start + end) / 2;;
    int64_t c;

    while (start <= end)
    {
        c = reinterpret_cast<int64_t>(addr) - reinterpret_cast<int64_t>(marked->at(midpoint));

        if (c == 0)
            return true;
        else if (c < 0)
            end = midpoint - 1;
        else
            start = midpoint + 1;

        midpoint = (start + end) / 2;
    }

    return false;
}

inline bool prune_false(void* rawdata)
{
    return false;
}

inline bool prune_true(void* rawdata)
{
    return true;
}

#endif