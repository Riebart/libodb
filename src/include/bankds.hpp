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

#ifndef BANKDS_HPP
#define BANKDS_HPP

#include <stack>

#include "datastore.hpp"
#include "lock.hpp"
#include "iterator.hpp"

class BankDS : public DataStore
{
    using DataStore::add_data;
    using DataStore::get_addr;

    /// Since the constructors are protected, ODB needs to be able to create new
    ///datastores.
    friend class ODB;

    friend class BankDSIterator;

public:
    virtual ~BankDS();

protected:
    BankDS();
    BankDS(DataStore* parent, bool (*prune)(void* rawdata), uint64_t data_size, uint32_t flags = 0, uint64_t cap = 102400);
    virtual void init(DataStore* parent, bool (*prune)(void* rawdata), uint64_t data_size, uint64_t cap);
    virtual void* add_data(void* rawdata);
    virtual void* get_addr();
    virtual void* get_at(uint64_t index);
    virtual bool remove_at(uint64_t index);
    virtual bool remove_addr(void* addr);
    virtual std::vector<void*>** remove_sweep(Archive* archive);
    virtual void remove_cleanup(std::vector<void*>** marked);
    virtual void purge(void (*freep)(void*));
    virtual void populate(Index* index);
    virtual DataStore* clone();
    virtual DataStore* clone_indirect();

    Iterator* it_first();
    Iterator* it_last();

    char** data;
    uint64_t posA;
    uint64_t posB;
    uint64_t list_size;
    uint64_t cap;
    uint64_t cap_size;
    uint64_t datalen;
    std::stack<void*> deleted;
};

class BankIDS : public BankDS
{
    using DataStore::add_data;

    /// Since the constructors are protected, ODB needs to be able to create new
    ///datastores.
    friend class ODB;

    /// Allows BankDS to create an indirect copy of itself.
    friend class BankDS;

protected:
    BankIDS();
#warning "TODO: Make proper use of the parent pointers where necessary. (See comment)"
    //(In: Cloning, getting idents, checking data integrity, **multiple levels of indirect datastores**).
    //Applies to: BankDS, BankIDS, BankVDS, LinkedListDS, LinkedListIDS, LinkedListVDS.
    BankIDS(DataStore* parent, bool (*prune)(void* rawdata), uint32_t flags = 0, uint64_t cap = 102400);
    virtual void* add_data(void* rawdata);
    virtual void* get_at(uint64_t index);
    virtual std::vector<void*>** remove_sweep(Archive* archive);
    virtual void remove_cleanup(std::vector<void*>** marked);
    virtual void populate(Index* index);
};

class BankDSIterator : public Iterator
{
    friend class BankDS;

protected:
    BankDSIterator();
    DataObj* next();
    DataObj* prev();

    BankDS* dstore;
    uint64_t posA;
    uint64_t posB;
};

#endif

/// Header file for BankDS datastore type and any children (BankIDS).
/// @file bankds.hpp
/// @Author Mike

/// @class BankDS
/// A datastore backend that works based on large buckets of memory.
/// The BankDS datastore works around the principle of approximately contiguous
///placement of objects in memory.
///
/// Unlike a linked list (such as LinkedListDS) where data can be scattered across
/// multiple memory pages, BankDS attempts maintain locality by allocating large
///blocks of contiguous memory and copying new items into those blocks.
/// While this is typically the job of malloc (or some other memory allocation
///method), malloc incurs both time and space overhead for each alloc. By reducing
///the number of allocs required and handling the pointer arithmetic internally
///these overheads are efficiently culled from the operations required.
///
/// Note that since BankDS is locally contiguous it is possible to index into
///it, much like an array. This incurs some overhead, but it is possible and
///is provided through the BankDS::get_at function.
/// Since it is not possible to free only a piece of each block deletions are
///handled by BankDS by pushing the memory locations of 'deleted' items onto a
///stack.
/// At the beginning of each insertion (more accurately: addition since most of
///the time items are being added to the end of the datastore) this stack is
///checked for contents. If the stack is empty then the new item is appended
///to the end, however if the stack contains any memory locations, they new data
///is copied to the top memory location. The stack is then popped to 'unmark'
///that location.

/// @fn BankDS::~BankDS()
/// Destructor for a BankDS object.
/// Since BankDS performs most of the memory handling itself there is a
///non-trivial destruction process that is required to free all of that
///memory. This destructor takes care of that.

/// @fn BankDS::BankDS()
/// Protected default constructor.
/// By reserving the default constructor as protected the compiler cannot
///generate one on its own and allow the user to instantiate a DataStore
///instance.

/// @fn BankDS::BankDS(DataStore* parent, bool (*prune)(void* rawdata), uint64_t data_size, uint32_t flags, uint64_t cap)
/// Complete constructor for a BankDS object
/// Since BankDS requires only three parameters, two of which have default
///values and the third is required in all situations, a single constructor
///is all that is needed. This protected version is differentiated from the
///public version by placing the parent argument first.
/// @param [in] parent A pointer to the DataStore that spawned this one
///through a BankDS::clone operation. This parameter will rarely be supplied
///by the user.
/// @param [in] data_size The length (in bytes) of each unit of data.
/// @param [in] cap The number of items to store in each block. (The default
///value is 102400 = 4096*25 as it has shown to have very good performance
///and guarantees memory alignment)

/// @fn void BankDS::init(DataStore* parent, bool (*prune)(void* rawdata), uint64_t data_size, uint64_t cap)
/// Initializer to save on code repetition.
/// Takes care of initializing everything, otherwise this code would appear
///in several different constructors.

/// @fn void* BankDS::add_data(void* rawdata)
/// Add a piece of raw data to the datastore.
/// @param [in] rawdata A pointer to the the location of the data to be added.
/// @return A pointer to the location of the added data in the datastore.
///By returning a pointer this reduces the lookup overhead to a minimal level.

/// @fn void* BankDS::get_addr()
/// Get the address of the space occupied by the next element (drop in replacement
///for malloc).
/// This is almost identical to add_element, except it does not perform a memcpy.
/// @return A pointer to the location of the next empty 'slot'.

/// @fn void* BankDS::get_at(uint64_t index)
/// Index into the datastore to retrieve a pointer to the data at the requested
///location.
/// Since BankDS is a locally contiguous data structure, it is possible to
///index into it.
/// @param [in] index A value indicating where to look into the datastore.
/// @return Returns a pointer to the desired data.

/// @fn bool BankDS::remove_at(uint64_t index)
/// Mark the location at the specified index as free.
/// @param [in] index A value indicating where in the datastore to mark as
///free.
/// @retval true The deletion succeeded and the corresponding location has
///been marked as free for the next addition.
/// @retval false The deletion failed and the location was not marked as
///free. This is most likely due to the case that index was out of bounds.

/// @fn bool BankDS::remove_addr(void* addr)
/// Mark the specified location as free (drop in replacement for free).
/// @param [in] addr Address to mark as free for data.
/// @retval true The specified location is a part of this datastore and was
///marked as free.
/// @retval false The specified location is not a part of this datastore and
///was left untouched.

/// @fn std::vector<void*>** BankDS::remove_sweep()
/// Sweep the datastore and mark data that satisfies the criterion for pruning.
/// @return A vector of pointers to the marked locations. It is important to
///note that the returned vector is sorted into ascending order for fast searching.

/// @fn void BankDS::remove_cleanup(std::vector<void*>** marked)
/// Clean up the pruned data by marking it available for reallocation.
/// @param [in] marked A vector of locations to memory that has been pruned
///from all associated index tables and is ready to be reallocated.

/// @fn void BankDS::populate(Index* index)
/// Populate a given index table with all items in this datastore.
/// It is conceivable that, when a new index table is created
///(by ODB::create_index) on top of a datastore that already contains data,
///the new index table be populated with the existing data. That job is
///performed by this function.
/// @param [in] index A pointer to the index table to be populated.

/// @fn DataStore* BankDS::clone()
/// Clone the datastore and return the same type but with no data.
/// @return A pointer to an indirect datastore (an instance of BankDS) that
///references back to this instance of BankDS as its parent.

/// @fn DataStore* BankDS::clone_indirect()
/// Clone the datastore and return an indirect version.
/// @return A pointer to an indirect datastore (an instance of BankIDS) that
///references back to this instance of BankDS as its parent.

/// @var char** BankDS::data
/// List of pointers to the buckets.
/// This contains a series of char* pointers to the various buckets. It
///starts with enough space for one bucket and double each time it needs
///to grow in size (i.e: as buckets are added).

/// @var uint64_t BankDS::posA
/// The position indicating which bucket the 'cursor' is in.
/// This actually holds a byte-offset to avoid unnecessary arithmetic.

/// @var uint64_t BankDS::posB
/// The position indicating where in the bucket (indicated by posA) the
///'cursor' is.
/// This actually holds a byte-offset to avoid unnecessary arithmetic.

/// @var uint64_t BankDS::list_size
/// The number of buckets that can be created before BankDS::data needs to
///grow.
/// This actually stores sizeof(char*)*number_of_buckets, to save on
///unnecessary arithmetic.

/// @var uint64_t BankDS::cap
/// The number of elements stored per bucket.

/// @var uint64_t BankDS::cap_size
/// The number of bytes per bucket. Equal to cap*data_size.

/// @var uint64_t BankDS::datalen
/// The size (in bytes) of each piece of data.

/// @var std::stack<void*> BankDS::deleted
/// Stack containing the memory locations of any deleted items.

/// @class BankIDS
/// A datastore backend that does not contain data, but rather pointers to data
///contained in a direct datastore.
///
/// BankIDS uses the same structure as BankDS but, instead of storing the actual
///data, it stores the pointers to data that resides elsewhere in memory. By
///overriding the BankDS::add_element and BankDS::get_at to add a single operation
///before the base versions the indirection is achieved with minimal code.

/// @fn BankIDS::BankIDS()
/// Protected default constructor.
/// By reserving the default constructor as protected the compiler cannot
///generate one on its own and allow the user to instantiate a DataStore
///instance.

/// @fn BankIDS::BankIDS(DataStore* parent, bool (*prune)(void* rawdata), uint32_t flags, uint64_t cap)
/// Unique constructor for a BankIDS object.
/// Since BankIDS requires only two parameters (since data_size is automatically
///set to sizeof(char*)), both of which can be default, only one constructor
///is needed. This protected version is differentiated from the
///public version by placing the parent argument first.
/// @param [in] parent A pointer to the DataStore that spawned this one
///through a BankDS::clone operation. This parameter will rarely be supplied
///by the user.
/// @param [in] cap The number of items to store in each block. (The default
///value is 102400 = 4096*25 as it has shown to have very good performance
///and guarantees memory alignment)

/// @fn void* BankIDS::add_data(void* rawdata)
/// Add a piece of raw data to the datastore.
/// This function adds a single address-of (&) operation to perform the
///necessary indirection on the input.
/// @param [in] rawdata A pointer to the the location of the data to be
///added.
/// @return A pointer to the location of the added data in the datastore.
///By returning a pointer this reduces the lookup overhead to a minimal level.

/// @fn void* BankIDS::get_at(uint64_t index)
/// Index into the datastore to retrieve a pointer to the data at the
///requested location.
/// Since BankDS is a locally contiguous data structure, it is possible to
///index into it. This function adds a single dereference (*) operation to
///perform the necessary indirection on the output.
/// @param [in] index A value indicating where to look into the datastore.
/// @return Returns a pointer to the desired data.

/// @fn std::vector<void*>** BankIDS::remove_sweep()
/// Sweep the datastore and mark data that satisfies the criterion for pruning.
/// @return A vector of pointers to the marked locations. It is important to
///note that the returned vector is sorted into ascending order for fast searching.

/// @fn void BankIDS::populate(Index* index)
/// Populate a given index table with all items in this datastore.
/// It is conceivable that, when a new index table is created
///(by ODB::create_index) on top of a datastore that already contains data,
///the new index table be populated with the existing data. That job is
///performed by this function.
/// @param [in] index A pointer to the index table to be populated.
