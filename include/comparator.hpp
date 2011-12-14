#ifndef COMPARATOR_HPP
#define COMPARATOR_HPP

#include <stdint.h>
#include <string.h>

#warning "TODO: Do something like the comparators for the free() function we're passing around."
class Condition
{
public:
    virtual bool condition(void* a) = 0;
};

class ConditionCust : public Condition
{
public:
    ConditionCust(bool (*_c)(void*))
    {
        this->c = _c;
    }

    virtual inline bool condition(void* a)
    {
        return c(a);
    }

private:
    bool (*c)(void*);
};

class Pruner
{
public:
    virtual bool prune(void* a) = 0;
};

class PruneCust : public Pruner
{
public:
    PruneCust(bool (*_p)(void*))
    {
        this->p = _p;
    }

    virtual inline bool prune(void* a)
    {
        return p(a);
    }

private:
    bool (*p)(void*);
};

class Keygen
{
public:
    virtual void* keygen(void* a) = 0;
};

class KeygenCust : public Keygen
{
public:
    KeygenCust(void* (*_k)(void*))
    {
        this->k = _k;
    }

    virtual inline void* keygen(void* a)
    {
        return k(a);
    }

private:
    void* (*k)(void*);
};

class Merger
{
public:
    virtual void* merge(void* a, void* b) = 0;
};

class MergeCust : public Merger
{
public:
    MergeCust(void* (*_m)(void*, void*))
    {
        this->m = _m;
    }

    virtual inline void* merge(void* a, void* b)
    {
        return m(a, b);
    }

private:
    void* (*m)(void*, void*);
};

class Comparator
{
public:
    virtual int32_t compare(void* a, void* b) = 0;
};

class Modifier
{
public:
    virtual void* mod(void* a) = 0;
};

class ModCompare : public Comparator
{
public:
    ModCompare(Modifier* _m, Comparator* _c)
    {
        this->m = _m;
        this->c = _c;
    }

    virtual inline int32_t compare(void* a, void* b)
    {
        return c->compare(m->mod(a), m->mod(b));
    }

private:
    Comparator* c;
    Modifier* m;
};

class ModOffset : public Modifier
{
public:
    ModOffset(uint32_t _offset)
    {
        this->offset = _offset;
    }

    virtual inline void* mod(void* a)
    {
        return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(a) + offset);
    }

private:
    uint32_t offset;
};

class ModAddrof : public Modifier
{
public:
    virtual inline void* mod(void* a)
    {
        addr = a;
        return reinterpret_cast<void*>(&addr);
    }

private:
    void* addr;
};

class ModDeref : public Modifier
{
public:
    virtual inline void* mod(void* a)
    {
        return *reinterpret_cast<void**>(a);
    }
};

class CompareCust : public Comparator
{
public:
    CompareCust(int32_t (*_c)(void*, void*))
    {
        this->c = _c;
    }

    virtual inline int32_t compare(void* a, void* b)
    {
        return c(a, b);
    }

private:
    int32_t (*c)(void*, void*);
};

class CompareUInt64 : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return *reinterpret_cast<uint64_t*>(a) - *reinterpret_cast<uint64_t*>(b);
    }
};

class CompareUInt32 : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return *reinterpret_cast<uint32_t*>(a) - *reinterpret_cast<uint32_t*>(b);
    }
};

class CompareUInt16 : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return *reinterpret_cast<uint16_t*>(a) - *reinterpret_cast<uint16_t*>(b);
    }
};

class CompareUInt8 : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return *reinterpret_cast<uint8_t*>(a) - *reinterpret_cast<uint8_t*>(b);
    }
};

class CompareInt64 : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return (int32_t)(*reinterpret_cast<int64_t*>(a) - *reinterpret_cast<int64_t*>(b));
    }
};

class CompareInt32 : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return *reinterpret_cast<int32_t*>(a) - *reinterpret_cast<int32_t*>(b);
    }
};

class CompareInt16 : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return *reinterpret_cast<int16_t*>(a) - *reinterpret_cast<int16_t*>(b);
    }
};

class CompareInt8 : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return *reinterpret_cast<int8_t*>(a) - *reinterpret_cast<int8_t*>(b);
    }
};

class CompareFloat : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return static_cast<int32_t>(*reinterpret_cast<float*>(a) - *reinterpret_cast<float*>(b));
    }
};

class CompareDouble : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return static_cast<int32_t>(*reinterpret_cast<double*>(a) - *reinterpret_cast<double*>(b));
    }
};

class CompareLongDouble : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return static_cast<int32_t>(*reinterpret_cast<long double*>(a) - *reinterpret_cast<long double*>(b));
    }
};

class CompareString : public Comparator
{
public:
    virtual inline int32_t compare(void* a, void* b)
    {
        return strcmp(reinterpret_cast<const char*>(a), reinterpret_cast<const char*>(b));
    }
};

#endif
