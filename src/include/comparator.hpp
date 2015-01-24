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

/// Header file for Comparator and child objects.
/// @file comparator.hpp

#ifndef COMPARATOR_HPP
#define COMPARATOR_HPP

#include "dll.hpp"

#include <stdint.h>
#include <string.h>

namespace libodb
{

    /// @class Condition

    //! @todo Do something like the comparators for the free() function we're
    ///passing around. That is, turn them into an object-wrapped thing?
    class LIBODB_API Condition
    {
    public:
        virtual bool condition(void* a) = 0;
    };

    class LIBODB_API ConditionCust : public Condition
    {
    public:
        ConditionCust(bool(*_c)(void*))
        {
            this->c = _c;
        }

        virtual inline bool condition(void* a)
        {
            return c(a);
        }

    private:
        bool(*c)(void*);
    };

    class LIBODB_API Pruner
    {
    public:
        virtual bool prune(void* a) = 0;
    };

    class LIBODB_API PruneCust : public Pruner
    {
    public:
        PruneCust(bool(*_p)(void*))
        {
            this->p = _p;
        }

        virtual inline bool prune(void* a)
        {
            return p(a);
        }

    private:
        bool(*p)(void*);
    };

    class LIBODB_API Keygen
    {
    public:
        virtual void* keygen(void* a) = 0;
    };

    class LIBODB_API KeygenCust : public Keygen
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

    class LIBODB_API Merger
    {
    public:
        virtual void* merge(void* a, void* b) = 0;
    };

    class LIBODB_API MergeCust : public Merger
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

    class LIBODB_API Comparator
    {
    public:
        virtual int32_t compare(void* a, void* b) = 0;
    };

    class LIBODB_API Modifier
    {
    public:
        virtual void* mod(void* a) = 0;
    };

    class LIBODB_API ModCompare : public Comparator
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

    class LIBODB_API ModOffset : public Modifier
    {
    public:
        ModOffset(uint32_t _offset)
        {
            this->offset = _offset;
        }

        virtual inline void* mod(void* a)
        {
            return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(a)+offset);
        }

    private:
        uint32_t offset;
    };

    class LIBODB_API ModAddrof : public Modifier
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

    class LIBODB_API ModDeref : public Modifier
    {
    public:
        virtual inline void* mod(void* a)
        {
            return *reinterpret_cast<void**>(a);
        }
    };

    class LIBODB_API CompareCust : public Comparator
    {
    public:
        CompareCust(int32_t(*_c)(void*, void*))
        {
            this->c = _c;
        }

        virtual inline int32_t compare(void* a, void* b)
        {
            return c(a, b);
        }

    private:
        int32_t(*c)(void*, void*);
    };

    class LIBODB_API CompareUInt64 : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            if (*reinterpret_cast<uint64_t*>(a) < *reinterpret_cast<uint64_t*>(b))
            {
                return -1;
            }
            else if (*reinterpret_cast<uint64_t*>(a) > *reinterpret_cast<uint64_t*>(b))
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
    };

    class LIBODB_API CompareUInt32 : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<uint32_t*>(a)-*reinterpret_cast<uint32_t*>(b);
        }
    };

    class LIBODB_API CompareUInt16 : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<uint16_t*>(a)-*reinterpret_cast<uint16_t*>(b);
        }
    };

    class LIBODB_API CompareUInt8 : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<uint8_t*>(a)-*reinterpret_cast<uint8_t*>(b);
        }
    };

    class LIBODB_API CompareInt64 : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return (int32_t)(*reinterpret_cast<int64_t*>(a)-*reinterpret_cast<int64_t*>(b));
        }
    };

    class LIBODB_API CompareInt32 : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<int32_t*>(a)-*reinterpret_cast<int32_t*>(b);
        }
    };

    class LIBODB_API CompareInt16 : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<int16_t*>(a)-*reinterpret_cast<int16_t*>(b);
        }
    };

    class LIBODB_API CompareInt8 : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return *reinterpret_cast<int8_t*>(a)-*reinterpret_cast<int8_t*>(b);
        }
    };

    class LIBODB_API CompareFloat : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return static_cast<int32_t>(*reinterpret_cast<float*>(a)-*reinterpret_cast<float*>(b));
        }
    };

    class LIBODB_API CompareDouble : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return static_cast<int32_t>(*reinterpret_cast<double*>(a)-*reinterpret_cast<double*>(b));
        }
    };

    class LIBODB_API CompareLongDouble : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return static_cast<int32_t>(*reinterpret_cast<long double*>(a)-*reinterpret_cast<long double*>(b));
        }
    };

    class LIBODB_API CompareString : public Comparator
    {
    public:
        virtual inline int32_t compare(void* a, void* b)
        {
            return strcmp(reinterpret_cast<const char*>(a), reinterpret_cast<const char*>(b));
        }
    };

}

#endif
