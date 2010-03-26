#ifndef COMPARATOR_HPP
#define COMPARATOR_HPP

#include <stdint.h>
#include <string.h>

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
        ModCompare(Modifier* m, Comparator* c)
        {
            this->m = m;
            this->c = c;
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
        ModOffset(uint32_t offset)
        {
            this->offset = offset;
        }
        
        virtual inline void* mod(void* a)
        {
            return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(a) + offset);
        }
    
    private:
        uint32_t offset;
};

// class ModAddrof : public Modifier
// {
//     public:
//         virtual inline void* mod(void* a)
//         {
//             return reinterpret_cast<void*>(&a);
//         }
// };

class ModDeref : public Modifier
{
    public:
        virtual inline void* mod(void* a)
        {
            return *reinterpret_cast<void**>(a);
        }
};

class CompareUInt64 : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<uint64_t*>(b) - *reinterpret_cast<uint64_t*>(a);
        }
};

class CompareUInt32 : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<uint32_t*>(b) - *reinterpret_cast<uint32_t*>(a);
        }
};

class CompareUInt16 : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<uint16_t*>(b) - *reinterpret_cast<uint16_t*>(a);
        }
};

class CompareUInt8 : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<uint8_t*>(b) - *reinterpret_cast<uint8_t*>(a);
        }
};

class CompareInt64 : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<int64_t*>(b) - *reinterpret_cast<int64_t*>(a);
        }
};

class CompareInt32 : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<int32_t*>(b) - *reinterpret_cast<int32_t*>(a);
        }
};

class CompareInt16 : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<int16_t*>(b) - *reinterpret_cast<int16_t*>(a);
        }
};

class CompareInt8 : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<int8_t*>(b) - *reinterpret_cast<int8_t*>(a);
        }
};

class CompareFloat : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return static_cast<int32_t>(*reinterpret_cast<float*>(b) - *reinterpret_cast<float*>(a));
        }
};

class CompareDouble : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return static_cast<int32_t>(*reinterpret_cast<double*>(b) - *reinterpret_cast<double*>(a));
        }
};

class CompareLongDouble : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return static_cast<int32_t>(*reinterpret_cast<long double*>(b) - *reinterpret_cast<long double*>(a));
        }
};

class CompareString : public Comparator
{
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return strcmp(reinterpret_cast<const char*>(b), reinterpret_cast<const char*>(a));
        }
};

#endif
