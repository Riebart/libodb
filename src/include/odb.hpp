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

/// Header file for ODB.
/// @file odb.hpp

//! @todo Update the documentation with potentially changed function signatures.

#include "dll.hpp"

#ifndef ODB_HPP
#define ODB_HPP

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <vector>

#ifdef CPP11THREADS
#include <thread>
#include <mutex>
#include <atomic>

#define THREADP_T std::thread*
#define ATOMICP_T std::atomic<uint64_t>*
#else
#include <pthread.h>

#define THREADP_T pthread_t
#define ATOMICP_T volatile uint32_t
#endif

#include "lock.hpp"
#include "scheduler.hpp"

#ifndef LEN_V
#define LEN_V
inline uint32_t len_v(void* rawdata)
{
    return strlen((const char*)rawdata);
}
#endif

class DataStore;
class Archive;
class Index;
class IndexGroup;
class DataObj;
class Comparator;
class Merger;
class Keygen;
class Iterator;

class LIBODB_API ODB
{
    /// Allow IndexGroup objects to create a new specifically identified ODB
    ///instance.
    friend class IndexGroup;

    /// Allow Index objects to create a new specifically identified ODB
    ///instance.
    friend class Index;

    /// Allows the scheduled workload from ODB to access the private members.
    friend void* odb_sched_workload(void* argsV);

    /// Allows the scheduled workload on an Index Group from ODB to access the private members.
    friend void* ig_sched_workload(void* argsV);

    /// The memory checking thread needs access to private functions
    /// for time checking and updating.
    friend void * mem_checker(void * arg);

public:
    /// Enum defining the types of flags that an Index can have on creation.
    /// When an index is created, these are the options that can be specified
    ///at creation time. These options control the behaviour of the index
    ///table. DROP_DUPLICATES indicates that any values that compare as equal
    ///to one already in the tree should be simply freed and dropped.
    ///DO_NOT_ADD_TO_ALL indicates that the index table should not be added
    ///to the "all" index table of the parent ODB object. Care should be
    ///taken with this flag since it is then the responsibility of the user
    ///to clean up that Index table. DO_NOT_POPULATE indicates that, upon
    ///creation, to suppress the standard practice which is populating the index
    ///table with the data of all of the items in the ODB's DataStore.
    /// @todo Apparently this isn't the appropriate way to do this (flags)?
    typedef enum { NONE = 0, DROP_DUPLICATES = 1, DO_NOT_ADD_TO_ALL = 2, DO_NOT_POPULATE = 4 } IndexFlags;

    /// Enum defining the specific index implementations available.
    /// Index implementations starting with "Keyed" are key-value index tables, all
    ///others are value-only index tables. Note that a value-only index table has
    ///less storage overhead but must perform comparisons directly on the data
    ///and may result in more complicated compare functions. Key-value index
    ///tables however require a keygen function that generates a key from a piece
    ///of data.
    typedef enum { LINKED_LIST, RED_BLACK_TREE } IndexType;

    /// Enum defining the specific fixed-width DataStore timplementations available
    /// Fixed-width DataStore implementations require that a fixed value be
    ///specified at instantiation time, and all data is assumed to be of the
    ///the specified size.
    typedef enum { BANK_DS, LINKED_LIST_DS } FixedDatastoreType;

    /// Emum defining the specific indirect DataStore implementations available.
    /// Indirect DataStore types store pointers to data that is assumed to be
    ///allocated and managed externally to the ODB object. All other DataStore
    ///types are copy-on-insert and manage their own memory allocation and
    ///release. Indirect DataStore types only store pointers and transfer the
    ///onus of memory mamangement to the user.
    typedef enum { BANK_I_DS, LINKED_LIST_I_DS } IndirectDatastoreType;

    /// Enum defining the specific variable-width DataStore implementations available.
    /// Variable-width DataStores types allow for variable sizes of data,
    ///requiring a data-size option passed on insertion.
    typedef enum { LINKED_LIST_V_DS } VariableDatastoreType;

    ODB(FixedDatastoreType dt, uint64_t datalen, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t (*len)(void*) = len_v, uint32_t sleep_duration = 0, uint32_t flags = 0);

    ~ODB();

    Index* create_index(IndexType type, uint32_t flags, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL, void* (*keygen)(void*) = NULL, int32_t keylen = -1);

    Index* create_index(IndexType type, uint32_t flags, Comparator* compare, Merger* merge = NULL, Keygen* keygen = NULL, int32_t keylen = -1);

    bool delete_index(Index* index);

    IndexGroup* create_group();
    IndexGroup* get_indexes();

    void add_data(void* rawdata);
    void add_data(void* rawdata, uint32_t nbytes);
    DataObj* add_data(void* rawdata, bool add_to_all);
    DataObj* add_data(void* rawdata, uint32_t nbytes, bool add_to_all);
    void remove_sweep();
    void purge();
    void set_prune(bool (*prune)(void*));
    virtual bool (*get_prune())(void*);
    uint64_t size();
    void update_time(time_t t);
    time_t get_time();

    uint32_t start_scheduler(uint32_t num_threads);
    void block_until_done();

    Iterator* it_first();
    Iterator* it_last();
    void it_release(Iterator* it);

    /// The memory limit, in pages (usually 4k), that the memory sweeping
    ///thread uses as a maximum limit for this ODB to consume.
    uint64_t mem_limit;

    /// Used to determine if memory checker thread is running or not.
    bool is_running()
    {
        return running;
    };


private:
    ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), uint64_t ident, uint64_t datalen, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata), uint64_t ident, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), uint64_t ident, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t (*len)(void*) = len_v, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(DataStore* dt, uint64_t ident, uint64_t datalen);

    void init(DataStore* data, uint64_t ident, uint64_t datalen, Archive* archive, void (*freep)(void*), uint32_t sleep_duration);
    void update_tables(std::vector<void*>* old_addr, std::vector<void*>* new_addr);

    /// Static variable that indicates the number of unique ODB instances
    ///currently running in the context of the process. Atomic operations
    ///(using Sun's atomics.h and GCC's atomic intrinsics) are used to update
    ///this value.
    //! @bug Make sure that this is fetched atomically.
    static ATOMICP_T num_unique;

    /// Identity of this ODB insance in this process' context.
    uint64_t ident;

    /// The amount of user data, in bytes, stored per data item. This is passed
    ///to the Datastore which augments it based on the options (timestamps and
    ///query count).
    uint64_t datalen;

    /// Complete list of all Index tables associated with this ODB context.
    std::vector<Index*>* tables;

    /// Complete list of all IndexGroup objects associated with this ODB context.
    std::vector<IndexGroup*>* groups;

    /// DataStore object used as backend storage for this ODB context.
    DataStore* data;

    /// IndexGroup used for automatic insertion of data to index tables. The
    ///default behaviour is for new index tables to be added to this group
    ///however this can be overridden with the DO_NOT_ADD_TO_ALL.
    IndexGroup* all;

    /// Pointer to a DataOBj that is handed back to the user when the appropriate
    ///methods are called.
    DataObj* dataobj;

    /// Thread that the memory checker runs in.
    THREADP_T mem_thread;

    /// Duration that the memory checker thread sleeps between sweeps. If this
    ///value is set to zero at ODB creation time, the memory checker thread
    ///is not started.
    uint32_t sleep_duration;

    /// Archiving method used when objects are swept from the Datastore.
    Archive* archive;

    /// Function used to free portions of data that are user-managed and
    ///linked to ODB managed data.
    void (*freep)(void*);

    /// Scheduler that the ODB uses for multithreaded performance.
    Scheduler* scheduler;

    /// Whether or not the memory checker thread is running.
    bool running;

    /// Locking context.
    RWLOCK_T;
};

#endif

/// @mainpage
/// This is where the project, overall, will be described.
/// @todo Write up the main page documentation.

/// @class ODB
/// Overarching class that provides the functionality of libODB.
///
/// The ODB class joins all of the others structures in libODB together to
///provide the database-like functionality. It uses the DataStore classes
///to provide backend storage, which is indexed by the Index classes. The
///Scheduler class is used to provide multithreading and improved performance
///in concurrent applications.
/// @todo Extend this with some kind of dynamic querying, daemon something.
///Like what we tried to do with the Python interface

/// @fn ODB::~ODB()
/// Destructor is employed to orchestrate the cleanup of all of the ODB components.
/// The destruction of an ODB is complex, due to the Index tables, DataStore,
///Scheduler (and its threads), Archive and memory cleanup thread.

/// @fn ODB::ODB(FixedDatastoreType dt, uint32_t datalen, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0)
/// Standard public constructor when using a fixed-width DataStore.
/// @param[in] dt Specific implementation flag.
/// @param[in] datalen Length of the data that the user is inserting.
/// @param[in] prune The function called on each item in the datastore to
///determine when a piece of data should be cleaned up and removed.
/// @param[in] archive The Archive class to be used when a piece of data is
///removed from the ODB which determines what happens to it.
/// @param[in] freep The function to be called when a piece of data is freed
///from the datastore. If the data contains user-managed memory combined with
///ODB managed memory, then this should be a function that cleans up the user
///managed portion.
/// @param[in] sleep_duration The duration, in seconds, between when the memory
///cleanup thread wakes up to do its work. A value of 0 indicates that the
///cleanup thread will not be started.
/// @param[in] flags Flag options indicate which additional metadata should
///be included with the user data. Options are DataStore::TIME_STAMP and
///DataStore::QUERY_COUNT

/// @fn ODB::ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), uint64_t ident, uint32_t datalen, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0)
/// Private portion of the ODB creation chain.
/// @param[in] dt Specific implementation flag.
/// @param[in] prune The function called on each item in the datastore to
///determine when a piece of data should be cleaned up and removed.
/// @param[in] ident Identity of this ODB in this process' context.
/// @param[in] datalen Length of the data that the user is inserting.
/// @param[in] archive The Archive class to be used when a piece of data is
///removed from the ODB which determines what happens to it.
/// @param[in] freep The function to be called when a piece of data is freed
///from the datastore. If the data contains user-managed memory combined with
///ODB managed memory, then this should be a function that cleans up the user
///managed portion.
/// @param[in] sleep_duration The duration, in seconds, between when the memory
///cleanup thread wakes up to do its work. A value of 0 indicates that the
///cleanup thread will not be started.
/// @param[in] flags Flag options indicate which additional metadata should
///be included with the user data. Options are DataStore::TIME_STAMP and
///DataStore::QUERY_COUNT

/// @fn ODB::ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0)
/// Standard public constructor when using an indirect DataStore.
/// @param[in] dt Specific implementation flag.
/// @param[in] datalen Length of the data that the user is inserting.
/// @param[in] prune The function called on each item in the datastore to
///determine when a piece of data should be cleaned up and removed.
/// @param[in] archive The Archive class to be used when a piece of data is
///removed from the ODB which determines what happens to it.
/// @param[in] freep The function to be called when a piece of data is freed
///from the datastore. If the data contains user-managed memory combined with
///ODB managed memory, then this should be a function that cleans up the user
///managed portion.
/// @param[in] sleep_duration The duration, in seconds, between when the memory
///cleanup thread wakes up to do its work. A value of 0 indicates that the
///cleanup thread will not be started.
/// @param[in] flags Flag options indicate which additional metadata should
///be included with the user data. Options are DataStore::TIME_STAMP and
///DataStore::QUERY_COUNT

/// @fn ODB::ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata), uint64_t ident, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0)
/// Private portion of the ODB creation chain.
/// @param[in] dt Specific implementation flag.
/// @param[in] prune The function called on each item in the datastore to
///determine when a piece of data should be cleaned up and removed.
/// @param[in] ident Identity of this ODB in this process' context.
/// @param[in] archive The Archive class to be used when a piece of data is
///removed from the ODB which determines what happens to it.
/// @param[in] freep The function to be called when a piece of data is freed
///from the datastore. If the data contains user-managed memory combined with
///ODB managed memory, then this should be a function that cleans up the user
///managed portion.
/// @param[in] sleep_duration The duration, in seconds, between when the memory
///cleanup thread wakes up to do its work. A value of 0 indicates that the
///cleanup thread will not be started.
/// @param[in] flags Flag options indicate which additional metadata should
///be included with the user data. Options are DataStore::TIME_STAMP and
///DataStore::QUERY_COUNT

/// @fn ODB::ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t (*len)(void*) = len_v, uint32_t sleep_duration = 0, uint32_t flags = 0)
/// Standard public constructor when using a variable-width DataStore.
/// @param[in] dt Specific implementation flag.
/// @param[in] prune The function called on each item in the datastore to
///determine when a piece of data should be cleaned up and removed.
/// @param[in] archive The Archive class to be used when a piece of data is
///removed from the ODB which determines what happens to it.
/// @param[in] freep The function to be called when a piece of data is freed
///from the datastore. If the data contains user-managed memory combined with
///ODB managed memory, then this should be a function that cleans up the user
///managed portion.
/// @param[in] len_v The function called on the data to be inserted if no
///length is supplied. A data length can be explicitly specified, but if one
///is not specified, we still need to know how much memory to allocate for
///the item and this is done through this function.
/// @param[in] sleep_duration The duration, in seconds, between when the memory
///cleanup thread wakes up to do its work. A value of 0 indicates that the
///cleanup thread will not be started.
/// @param[in] flags Flag options indicate which additional metadata should
///be included with the user data. Options are DataStore::TIME_STAMP and
///DataStore::QUERY_COUNT

/// @fn ODB::ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), uint64_t ident, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t (*len)(void*) = len_v, uint32_t sleep_duration = 0, uint32_t flags = 0)
/// Standard public constructor when using an indirect DataStore.
/// @param[in] dt Specific implementation flag.
/// @param[in] datalen Length of the data that the user is inserting.
/// @param[in] prune The function called on each item in the datastore to
///determine when a piece of data should be cleaned up and removed.
/// @param[in] archive The Archive class to be used when a piece of data is
///removed from the ODB which determines what happens to it.
/// @param[in] freep The function to be called when a piece of data is freed
///from the datastore. If the data contains user-managed memory combined with
///ODB managed memory, then this should be a function that cleans up the user
///managed portion.
/// @param[in] len_v The function called on the data to be inserted if no
///length is supplied. A data length can be explicitly specified, but if one
///is not specified, we still need to know how much memory to allocate for
///the item and this is done through this function.
/// @param[in] sleep_duration The duration, in seconds, between when the memory
///cleanup thread wakes up to do its work. A value of 0 indicates that the
///cleanup thread will not be started.
/// @param[in] flags Flag options indicate which additional metadata should
///be included with the user data. Options are DataStore::TIME_STAMP and
///DataStore::QUERY_COUNT

/// @fn ODB::ODB(DataStore* dt, uint64_t ident, uint32_t datalen)
/// Work horse for ODB creation. Everything else just abstracts away the detals
///and this function does the common work.
/// @param[in] dt Datastore type.
/// @param[in] ident ODB identity in this process' context.
/// @param[in] datalen Length of the user data that will be inserted into this
///ODB.

/// @fn ODB::create_index(IndexType type, uint32_t flags, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL, void* (*keygen)(void*) = NULL, int32_t keylen = -1)
/// Create an Index table associated with this ODB object.
/// @param[in] type Index table type
/// @param[in] flags Flags (ODB::IndexFlags) used to control how the Index
///table behaves in various situations.
/// @param[in] compare Comparison function used to determine the ordering of
///elements in the index table. This comparison's return value follows the
///same conventions as the comparator used for qsort().
/// @param[in] merge Function called when two functions compare as equal. This
///function assumes that "something somewhere" convention of Unix in that
///it has the 'new' data as the first argument, and the data already in the
///tree (and hence the data that will be kept) as the second argument.
/// @param[in] keygen Function called to generate the key from the data.
/// @param[in] keylen Length, in bytes, of the key that is generated by the
///key generation function. A value of -1, the default, indicates that this
///is a value-only index table, and not a key-value store.
/// @return A pointer to the index table built given the specified parameters.
/// @attention Merging of nodes implies dropping duplicates post merge.
/// @see CompareCust
/// @see MergeCust
/// @see KeygenCust

/// @fn ODB::create_index(IndexType type, uint32_t flags, Comparator* compare, Merger* merge = NULL, Keygen* keygen = NULL, int32_t keylen = -1)
/// Create an Index table associated with this ODB object.
/// @param[in] type Index table type
/// @param[in] flags Flags (ODB::IndexFlags) used to control how the Index
///table behaves in various situations.
/// @param[in] compare Comparison object used to determine the ordering of
///elements in the index table.
/// @param[in] merge Object used when two functions compare as equal. This
///function assumes that "something somewhere" convention of Unix in that
///it has the 'new' data as the first argument, and the data already in the
///tree (and hence the data that will be kept) as the second argument.
/// @param[in] keygen Object used to generate the key from the data.
/// @param[in] keylen Length, in bytes, of the key that is generated by the
///key generation function. A value of -1, the default, indicates that this
///is a value-only index table, and not a key-value store.
/// @return A pointer to the index table built given the specified parameters.
/// @attention Merging of nodes implies dropping duplicates post merge.

/// @fn ODB::delete_index(Index* index)
/// Remove an index from this ODB context.
/// @param[in] index Index to be cleaned up.
/// @return Whether or not the deletion was successful. The deletion will fail if the index fiven is not part of the ODB's context.

/// @fn ODB::create_group()
/// Create an IndexGroup associated with this ODB context.
/// @return

/// @fn ODB::get_indexes()
/// Gets an IndexGroup of all of the index tables (module the DO_NOT_ADD_TO_ALL
///flag) associated with this context.
/// @return The all member of the ODB context.

/// @fn ODB::add_data(void* rawdata)
/// Add data to the DataStore and Index tables.
/// @param[in] rawdata Data to add to the ODB.

/// @fn ODB::add_data(void* rawdata, uint32_t nbytes)
/// Add data to the ODB, when using a variable-width DataStore with explicit size.
/// @param[in] rawdata Data to add to the ODB.
/// @param[in] nbytes Number of bytes of data to add. This parameter is only
///used when a variable-width DataStore is used.

/// @fn ODB::add_data(void* rawdata, bool add_to_all)
/// Add data to the ODB and pass the data back to the user for further use.
/// @param[in] rawdata Data to add to the ODB.
/// @param[in] add_to_all Whether or not to all to the 'all' IndexGroup of
///this ODB.
/// @param[in] nbytes Number of bytes of data to add. This parameter is only
///used when a variable-width DataStore is used.
/// @return A DataObj object that is tied to this ODB context that represents
///the data for adding to additional index tables.
/// @note The bool here cannot have a default value, even though the standard
///choice would be false. A default value makes the call ambiguous with the
///one above.

/// @fn ODB::add_data(void* rawdata, uint32_t nbytes, bool add_to_all)
/// Add data to the ODB and pass the data back to the user for further use - A variable-data size version.
/// @param[in] rawdata Data to add to the ODB.
/// @param[in] add_to_all Whether or not to all to the 'all' IndexGroup of
///this ODB.
/// @param[in] nbytes Number of bytes of data to add. This parameter is only
///used when a variable-width DataStore is used.
/// @return A DataObj object that is tied to this ODB context that represents
///the data for adding to additional index tables.
/// @note The bool here cannot have a default value, even though the standard
///choice would be false. A default value makes the call ambiguous with the
///one above.

/// @fn ODB::remove_sweep()
/// Perform a sweep of the ODB that applies its prune function to the elements
///in the datastore, then iterates through the index tables and removes references
///to the removed elements of the datastore. The sweep is performed in four
///phases:
///
///1) The DataStore is swept via DataStore::remove_sweep using the
///ODB's Archive object resulting in a list of pointers to data that are fit
///for removal. At this point, the data still exists in the DataStore.
///
///2) These pointers are removed from the index tables.
///
///3) An optional step includes calling ODB::update_tables with a list of
///old and new vectors that updates the pointers in the index tables. This
///step is only run if the DataStore returned the appropriate vectors.
///
///4) The datastore is called again via DataStore::remove_cleanup to perform
///the actual removal of the elements from the DataStore and cleanup of
///intermediate storage.
/// @see DataStore::remove_sweep
/// @see DataStore::remove_cleanup
/// @see ODB::update_tables
/// @see Index::remove_sweep
/// @attention This function locks the ODB object for the duration of the sweep.
///This means that no data can be added, or removed, from the ODB during the
///sweep. This is important since in real-time operations this will cause
///the inserting thread to block until the sweep completes.

/// @fn ODB::purge()
/// Purge the ODB and all of its associated Index tables and its DataStore
///of all data.

/// @fn ODB::set_prune(bool (*prune)(void*))
/// Set the pruning function.
/// @param[in] prune New pruning function.

/// @fn ODB::get_prune()
/// Return the current pruning function.
/// @return The current pruning function.

/// @fn ODB::size()
/// Return the 'size' of this ODB.
/// @return The number of elements in the DataStore.
/// @see DataStore::size()

/// @fn ODB::update_time(time_t t)
/// Set the time of the ODB object.
/// @param[in] t The new time to stamp on objects put into the ODB.

/// @fn ODB::get_time()
/// Get the time that is currently being stamped onto items put into the ODB.
/// @return The time structure representing the time the ODB is using.

/// @fn start_scheduler(uint32_t num_threads)
/// Start, or update the number of worker threads, in the ODB's scheduler.
/// @param[in] num_threads Number of threads to attempt to spool up in the
///scheduler.
/// @return Number of threads the scheduler is running at the point this call
///returns.
/// @see Scheduler::Scheduler
/// @see Scheduler::update_num_threads

/// @fn ODB::block_until_done()
/// Blocks the calling thread until all currently scheduled workloads in the
///scheduler are completed.
/// @see Scheduler::block_until_done

/// @fn ODB::it_first()
/// Pull an iterator for an unordered traversal of the data in the ODB's datastore.
/// @return Iterator to the first item in the ODB's datastore.

/// @fn ODB::it_last()
/// Pull an iterator for an unordered traversal of the data in the ODB's datastore.
/// @return Iterator to the last item in the ODB's datastore.

/// @fn ODB::it_release(Iterator* it)
/// Release an existing iterator.
/// @param[in] it Iterator to release.

