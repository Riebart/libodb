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

/// Source file for implementations of Index, IndexGroup and DataObj objects.
/// @file index.cpp

#include "index.hpp"
#include "common.hpp"

#include <stdlib.h>
#include <stdio.h>

#include "odb.hpp"
#include "datastore.hpp"
#include "comparator.hpp"
#include "iterator.hpp"

#include "lock.hpp"

namespace libodb
{

    DataObj::~DataObj()
    {
    }

    DataObj::DataObj()
    {
    }

    DataObj::DataObj(uint64_t _ident)
    {
        this->ident = _ident;
    }

    // ============================================================================

    IndexGroup::IndexGroup()
    {
        indices = new std::vector<IndexGroup*>();
    }

    IndexGroup::~IndexGroup()
    {
        delete indices;
    }

    IndexGroup::IndexGroup(uint64_t _ident, DataStore* _parent)
    {
        this->ident = _ident;
        this->parent = _parent;
        scheduler = NULL;
        indices = new std::vector<IndexGroup*>();
    }

    bool IndexGroup::add_index(IndexGroup* ig)
    {
        // If it passes integrity checks, add it to the group.
        if (ident == ig->ident)
        {
            indices->push_back(ig);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool IndexGroup::delete_index(IndexGroup* ig)
    {
        if (ident == ig->ident)
        {
            for (uint32_t i = 0; i < indices->size(); i++)
            {
                // If we've got our match, then return true.
                if (indices->at(i) == ig)
                {
                    delete indices->at(i);
                    indices->erase(indices->begin() + i);
                    return true;
                }
                // Otherwise recursively check for it.
                else
                {
                    // Thus dynamic cast should try to cast to an Index.
                    // If we actually have an IndexGroup, this should bail and return NULL.
                    // Checking for that NULL, we can then test the IndexGroup for ig.
                    // If we find it, return true, otherwise carry on.
                    if ((dynamic_cast<Index*>(indices->at(i)) == NULL) && (indices->at(i)->delete_index(ig)))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    IndexGroup* IndexGroup::at(uint32_t i)
    {
        if (i < indices->size())
        {
            return indices->at(i);
        }
        else
        {
            return NULL;
        }
    }

    std::vector<Index*>* IndexGroup::flatten()
    {
        std::vector<Index*>* ret = new std::vector<Index*>();
        flatten(ret);
        return ret;
    }

    std::vector<Index*>* IndexGroup::flatten(std::vector<Index*>* list)
    {
        for (uint32_t i = 0; i < indices->size(); i++)
        {
            indices->at(i)->flatten(list);
        }

        return list;
    }

    struct sched_args
    {
        void* rawdata;
        IndexGroup* ig;
    };

    void* ig_sched_workload(void* argsV)
    {
        struct sched_args* args = (struct sched_args*)argsV;
        args->ig->add_data_v(args->rawdata);
        free(args);

        return NULL;
    }

    /// @attention Current threading is based on insertion, not per table. Changing that is done here.
    inline void IndexGroup::add_data(DataObj* data)
    {
        // If it passes integrity checks, add it to the group.
        if (data->ident == ident)
        {
            // Since any indices or index groups in here have passed integrity checks already, we can fall back to add_data_v.
            if (scheduler == NULL)
            {
                add_data_v(data->data);
            }
            else
            {
                struct sched_args* args;
                SAFE_MALLOC(struct sched_args*, args, sizeof(struct sched_args));
                args->rawdata = data->data;
                args->ig = this;
                scheduler->add_work(ig_sched_workload, args, NULL, Scheduler::NONE);
            }
        }
    }

    inline ODB* IndexGroup::query(bool(*condition)(void*))
    {
        ConditionCust* c = new ConditionCust(condition);
        ODB* ret = query(c);
        delete c;
        return ret;
    }

    inline ODB* IndexGroup::query(Condition* condition)
    {
        // Clone the parent.
        DataStore* ds = parent->clone_indirect();

        // Query
        query(condition, ds);

        // Wrap in ODB and return.
        ODB* odb = new ODB(ds, ident, parent->datalen);
        ds->update_parent(odb);
        return odb;
    }

    inline ODB* IndexGroup::query_eq(void* rawdata)
    {
        DataStore* ds = parent->clone_indirect();
        query_eq(rawdata, ds);

        ODB* odb = new ODB(ds, ident, parent->datalen);
        ds->update_parent(odb);
        return odb;
    }

    inline ODB* IndexGroup::query_lt(void* rawdata)
    {
        DataStore* ds = parent->clone_indirect();
        query_lt(rawdata, ds);

        ODB* odb = new ODB(ds, ident, parent->datalen);
        ds->update_parent(odb);
        return odb;
    }

    inline ODB* IndexGroup::query_gt(void* rawdata)
    {
        DataStore* ds = parent->clone_indirect();
        query_gt(rawdata, ds);

        ODB* odb = new ODB(ds, ident, parent->datalen);
        ds->update_parent(odb);
        return odb;
    }

    inline uint64_t IndexGroup::get_ident()
    {
        return ident;
    }

    inline void IndexGroup::add_data_v(void* data)
    {
        // Do it the silly way in order to preserve speed when compiled with zero optimizations.
        size_t n = indices->size();

        if (n == 0)
        {
            return;
        }

        for (size_t i = 0; i < n; i++)
        {
            indices->at(i)->add_data_v(data);
        }
    }

    //! @todo I don't think any of the read-only functions here are done right.
    ///What the hell do I mean by this?
    inline void IndexGroup::query(Condition* condition, DataStore* ds)
    {
        size_t n = indices->size();

        // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
        if (n > 0)
        {
            if (n == 1)
            {
                indices->at(0)->query(condition, ds);
            }
            else
            {
                for (size_t i = 0; i < n; i++)
                {
                    indices->at(i)->query(condition, ds);
                }
            }
        }
    }

    inline void IndexGroup::query_eq(void* rawdata, DataStore* ds)
    {
        size_t n = indices->size();

        // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
        if (n > 0)
        {
            if (n == 1)
            {
                indices->at(0)->query_eq(rawdata, ds);
            }
            else
            {
                for (size_t i = 0; i < n; i++)
                {
                    indices->at(i)->query_eq(rawdata, ds);
                }
            }
        }
    }

    inline void IndexGroup::query_lt(void* rawdata, DataStore* ds)
    {
        size_t n = indices->size();

        // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
        if (n > 0)
        {
            if (n == 1)
            {
                indices->at(0)->query_lt(rawdata, ds);
            }
            else
            {
                for (size_t i = 0; i < n; i++)
                {
                    indices->at(i)->query_lt(rawdata, ds);
                }
            }
        }
    }

    inline void IndexGroup::query_gt(void* rawdata, DataStore* ds)
    {
        size_t n = indices->size();

        // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
        if (n > 0)
        {
            if (n == 1)
            {
                indices->at(0)->query_gt(rawdata, ds);
            }
            else
            {
                for (size_t i = 0; i < n; i++)
                {
                    indices->at(i)->query_gt(rawdata, ds);
                }
            }
        }
    }

    uint64_t IndexGroup::size()
    {
        return indices->size();
    }

    // ============================================================================

    Index::Index() : IndexGroup()
    {
        // Implemented because something, somewhere, needs it when linking. Not sure where.
        // Also now for setting the LUID.
#ifdef WIN32
        luid_val = (uint64_t)this;
#else
        char* end;
        char buf[20];
        sprintf(buf, "%p", this);
        luid_val = strtoull(buf, &end, 16);
#endif
        scheduler = NULL;
    }

    inline void Index::add_data(DataObj* data)
    {
        if (data->ident == ident)
        {
            // If it passes integrity check, drop to add_data_v and succeed.
            add_data_v(data->data);
        }
    }

    uint64_t Index::size()
    {
        return count;
    }

    inline uint64_t Index::luid()
    {
        return luid_val;
    }

    std::vector<Index*>* Index::flatten(std::vector<Index*>* list)
    {
        list->push_back(this);
        return list;
    }

    void* add_data_v_wrapper(void* args)
    {
        void** args_a = (void**)args;
        void* rawdata = args_a[0];
        Index* obj = (Index*)(args_a[1]);
        void* ret = (void*)(obj->add_data_v2(rawdata));
        free(args);
        return ret;
    }

    inline void Index::add_data_v(void* rawdata)
    {
        //     if (scheduler == NULL)
        //     {
        add_data_v2(rawdata);
        //     }
        //     else
        //     {
        //         void** args;
        //         SAFE_MALLOC(void**, args, sizeof(void*) + sizeof(Index*));
        //         args[0] = rawdata;
        //         args[1] = this;
        //         scheduler->add_work(add_data_v_wrapper, args, NULL, luid_val, Scheduler::NONE);
        //     }
    }

    bool Index::add_data_v2(void* rawdata)
    {
        return false;
    }

    inline void Index::purge()
    {
    }

    inline ODB* Index::query(bool(*condition)(void*))
    {
        ConditionCust* c = new ConditionCust(condition);
        ODB* ret = query(c);
        delete c;
        return ret;
    }

    inline ODB* Index::query(Condition* condition)
    {
        // Clone the parent.
        DataStore* ds = parent->clone_indirect();

        // Query
        query(condition, ds);

        // Wrap in ODB and return.
        ODB* odb = new ODB(ds, ident, parent->datalen);
        ds->update_parent(odb);
        return odb;
    }

    inline ODB* Index::query_eq(void* rawdata)
    {
        DataStore* ds = parent->clone_indirect();
        query_eq(rawdata, ds);

        ODB* odb = new ODB(ds, ident, parent->datalen);
        ds->update_parent(odb);
        return odb;
    }

    inline ODB* Index::query_lt(void* rawdata)
    {
        DataStore* ds = parent->clone_indirect();
        query_lt(rawdata, ds);

        ODB* odb = new ODB(ds, ident, parent->datalen);
        ds->update_parent(odb);
        return odb;
    }

    inline ODB* Index::query_gt(void* rawdata)
    {
        DataStore* ds = parent->clone_indirect();
        query_gt(rawdata, ds);

        ODB* odb = new ODB(ds, ident, parent->datalen);
        ds->update_parent(odb);
        return odb;
    }

    inline void Index::query(Condition* condition, DataStore* ds)
    {
    }

    inline void Index::query_eq(void* rawdata, DataStore* ds)
    {
    }

    inline void Index::query_lt(void* rawdata, DataStore* ds)
    {
    }

    inline void Index::query_gt(void* rawdata, DataStore* ds)
    {
    }

    inline void Index::update(std::vector<void*>* old_addr, std::vector<void*>* new_addr, uint64_t datalen)
    {
    }

    inline bool Index::remove(DataObj* data)
    {
        return remove(data->data);
    }

    inline bool Index::remove(void* rawdata)
    {
        return false;
    }

    inline void Index::remove_sweep(std::vector<void*>* marked)
    {
    }

    inline Iterator* Index::it_first()
    {
        return NULL;
    }

    inline Iterator* Index::it_last()
    {
        return NULL;
    }

    inline Iterator* Index::it_lookup(void* rawdata, int8_t dir)
    {
        return NULL;
    }

    inline void Index::it_release(Iterator* it)
    {
        delete it;
        READ_UNLOCK(rwlock);
    }

}
