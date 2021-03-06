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

/// Header file for Index, IndexGroup and DataObj objects.
/// @file index.hpp

#ifndef INDEX_HPP
#define INDEX_HPP

#include "dll.hpp"

#include <vector>
#include <stdint.h>

namespace libodb
{

    // Forward declarations.
    class ODB;
    class DataStore;
    class Scheduler;
    class Comparator;
    class Condition;
    class Merger;
    class Iterator;
    class Index;

    class LIBODB_API DataObj
    {
        /// Requires ability to set the data field.
        friend class ODB;

        /// Requires ability to read the ident field.
        friend class IndexGroup;

        /// Requires ability to read the ident field.
        friend class Index;

        /// Requires ability to create and manipulate DataObj.
        friend class RedBlackTreeI;

        /// Requires ability to create and manipulate DataObj.
        friend class LinkedListI;

        /// Requires ability to create and manipulate DataObj.
        friend class Iterator;

        /// Requires ability to create and manipulate DataObj.
        friend class RBTIterator;
        friend class ERBTIterator;

        /// Requires ability to create and manipulate DataObj.
        friend class LLIterator;

        friend class BankDS;
        friend class BankDSIterator;

        friend class LinkedListDS;
        friend class LinkedListDSIterator;

    public:
        inline void* get_data()
        {
            return data;
        };


    private:
        DataObj();
        DataObj(uint64_t ident);
        ~DataObj();

        uint64_t ident;
        void* data;
    };

    class LIBODB_API IndexGroup
    {
        /// Allows ODB objects to call the IndexGroup::add_data_v method directly and
        ///skip the overhead of verifying data integrity since that is guaranteed
        ///in that usage scenario.
        friend class ODB;

        /// Allow the ODB scheduled workload to access the add_data_v function.
        friend void* odb_sched_workload(void* argsV);
        friend void* ig_sched_workload(void* argsV);

    public:
        virtual ~IndexGroup();

        bool add_index(IndexGroup* ig);
        bool delete_index(IndexGroup* ig);
        IndexGroup* at(uint32_t i);
        std::vector<Index*>* flatten();
        virtual void add_data(DataObj* data);
        virtual ODB* query(bool(*condition)(void*));
        virtual ODB* query(Condition* condition);
        virtual ODB* query_eq(void* rawdata);
        virtual ODB* query_lt(void* rawdata);
        virtual ODB* query_gt(void* rawdata);
        virtual uint64_t get_ident();
        //! @todo Add a recursive flavour of size()
        ///It will have to return the number of items
        ///in all contained indices though. Is there a way to stop before indices at
        ///the last layer of IndexGroups?
        virtual uint64_t size();

    protected:
        IndexGroup();
        IndexGroup(uint64_t _ident, DataStore* _parent);

        uint64_t ident;
        DataStore* parent;

        Scheduler* scheduler;

        virtual void add_data_v(void* data);
        virtual void query(Condition* condition, DataStore* ds);
        virtual void query_eq(void* rawdata, DataStore* ds);
        virtual void query_lt(void* rawdata, DataStore* ds);
        virtual void query_gt(void* rawdata, DataStore* ds);
        virtual std::vector<Index*>* flatten(std::vector<Index*>* list);

        void* rwlock;

    private:
        std::vector<IndexGroup*>* indices;
    };

    //! @todo An index table built on a vector, behaving like the LinkedListI.
    //! @todo Batch insertions for index tables. At least LL.
    class LIBODB_API Index : public IndexGroup
    {
        /// Allows ODB to call remove_sweep.
        friend class ODB;

        /// Allows BankDS to access the Index::add_data_v function in BankDS::populate
        ///to bypass integrity checking.
        friend class BankDS;

        /// Allows BankIDS to access the Index::add_data_v function in BankIDS::populate
        ///to bypass integrity checking.
        friend class BankIDS;

        /// Allows LinkedListDS to access the Index::add_data_v function in
        ///LinkedListDS::populate to bypass integrity checking.
        friend class LinkedListDS;

        /// Allows LinkedListIDS to access the Index::add_data_v function in
        ///LinkedListIDS::populate to bypass integrity checking.
        friend class LinkedListIDS;

        /// Needed in order to wrap the actual work function which is a fat pointer
        /// and won't fit into a normal function pointer.
        friend void* add_data_v_wrapper(void* args);

    public:
        virtual void add_data(DataObj* data);
        virtual uint64_t size();
        virtual uint64_t luid();
        virtual bool remove(DataObj* data);
        virtual ODB* query(bool(*condition)(void*));
        virtual ODB* query(Condition* condition);
        virtual ODB* query_eq(void* rawdata);
        virtual ODB* query_lt(void* rawdata);
        virtual ODB* query_gt(void* rawdata);
        virtual Iterator* it_first();
        virtual Iterator* it_last();
        virtual Iterator* it_lookup(void* rawdata, int8_t dir = 0);
        virtual void it_release(Iterator* it);

    protected:
        Index();
        void add_data_v(void* rawdata);
        //     void* add_data_v_wrapper(void* args);
        virtual bool add_data_v2(void* rawdata);
        virtual void purge();
        virtual void query(Condition* condition, DataStore* ds);
        virtual void query_eq(void* rawdata, DataStore* ds);
        virtual void query_lt(void* rawdata, DataStore* ds);
        virtual void query_gt(void* rawdata, DataStore* ds);
        virtual std::vector<Index*>* flatten(std::vector<Index*>* list);
        //! @bug Another setting of -1 as a the defauly value to a uint...
        virtual void update(std::vector<void*>* old_addr, std::vector<void*>* new_addr, uint64_t datalen = -1);
        virtual bool remove(void* rawdata);
        virtual void remove_sweep(std::vector<void*>* marked);

        Comparator* compare;
        Merger* merge;
        uint64_t count;
        bool drop_duplicates;
        uint64_t luid_val;
    };

}

#endif

/// @class DataObj
/// Class container for giving the user access to a secure piece of data to
///manually add to index tables and index groups.
/// By keeping all members private it is able to give the user the ability
///to pass around a piece of data that cannot be tampered with but still allows
///for manual insertion into index tables or index groups. Because anything
///that is intended to be able to access a DataObj instance is declared as a
///friend (ODB, Index, IndexGroup), and thus privileged, everything is private
///and no accessors or settors are provided.
///
/// Since all accesses must be privileged, any class that uses this object
///must be a friend.

/// @fn DataObj::DataObj()
/// Protected default constructor.
/// By reserving the default constructor as protected the compiler cannot
///generate one on its own and allow the user to instantiate a DataStore
///instance.

/// @fn DataObj::DataObj(int ident)
/// Standard constructor
/// This initializes the two data fields in the object to something useful.
///Since all accesses to this class are privileged, the rule here is that
///anything that does not change during the lifetime of the object is set by
///the constructor. Since the identifier never changes (but the data will
///change frequently) only ident is set by the constructor.
/// @param [in] ident Unique identifier that specifies which instances of the
///ODB class this instance of DataObj can be used with.

/// @fn DataObj::~DataObj()
/// Destructor for any DataObj objects.
/// Currently this is an empty destructor, but it is around in case it is
///needed later.

/// @var DataObj::ident
/// Identifier
/// This holds the unique identifier that matches the ODB objects that can
///work with it.

/// @var DataObj::data
/// Pointer to data.

/// @class IndexGroup
/// Class that takes care of grouping index tables into more abstract logical
///structures.
/// By allowing for groups of index tables to be made more abstract structures
///and relations can be represented. Further, IndexGroups support nesting
///IndexGroups for further abstraction.
/// This means that a uniform interface can be presented to any machine learning
///or artificial intelligence systems placed on top of this backend.

/// @fn IndexGroup::~IndexGroup()
/// Standard destructor.
/// Provided explicitly in case it needs to be expanded later.

/// @fn bool IndexGroup::add_index(IndexGroup* ig)
/// Add another IndexGroup (or any derived class, such as Index, or a
///specific index type) to this IndexGroup.
/// By allowing the insertion of an IndexGroup there is native support
///for nesting any sort of index (IndexGroup, Index, LinkedListI, ...).
///This is where the support for an arbitrary amount of abstraction stems
///from.
/// @param [in] ig IndexGroup to add.
/// @retval true The identifiers between this IndexGroup and the new one
///matched and the operation succeeded.
/// @retval false The identifiers did not match and the operation failed.
///No changes were made to any of the involved objects.

/// @fn bool IndexGroup::add_data(DataObj* data)
/// Add a piece of data to this IndexGroup.
/// A DataObj instance must be inserted as it contains the necessary
///information to ensure that data integrity is maintained through the insertion.
/// @param data A pointer to an instance of DataObj that represents the piece
///of data to be added.
/// @retval true The identifiers between this IndexGroup and the new data
///matched and the operation succeeded.
/// @retval false The identifiers did not match and the operation failed.
///No changes were made to any of the involved objects.

/// @fn ODB* IndexGroup::query(Condition* condition)
/// Perform a general query on the entire IndexGroup
/// This performs a general query on the entire IndexGroup structure. Every
///index table 'contained' in this IndexGroup will contribute to the results.
///No effort is made to avoid duplicates and so it is recommended that this
///only be used if duplicates are acceptable or if the index tables contain
///disjoint information.
/// The datastore used is an indirect copy of the datastore that is used in
///this IndexGroup's parent (IndexGroup::parent).
/// @param condition A condition function that returns true if the piece of
///data 'passes' (and should be added to the query results) and false if the
///data fails (and will not be returned in the results).
/// @return A pointer to an ODB object that represents the query results.
/// @attention This opertation is O(N), where N is the number of entries in all
///index tables contained in this IndexGroup tree.

/// @fn uint64_t IndexGroup::get_ident()
/// Obtain the identifier of this IndexGroup.
/// It is possible that one would want to verify the indentifier of an
///IndexGroup and this function allows the user to obtain the necessary
///information.

/// @fn uint64_t IndexGroup::size()
/// Get the size of this IndexGroup.
/// Note that this does not recurse at all and returns a number equal to the
///number of times this instance's IndexGroup::add_index function was called.
/// @return The number of IndexGroups in this IndexGroup.

/// @fn IndexGroup::IndexGroup()
/// Protected default constructor.
/// By reserving the default constructor as protected the compiler cannot
///generate one on its own and allow the user to instantiate a DataStore
///instance.

/// @fn IndexGroup::IndexGroup(int ident, DataStore* parent)
/// Standard constructor.
/// The only constructur explicitly called as both items need to be set at
///creation (or eventually, anyway).
/// @param [in] ident Set the identifier to ensure data integrity and restrict
///data additions to only compatible data types.
/// @param [in] parent Set the datastore parent of this IndexGroup. This is
///necessary for when the parent is cloned for query returns.

/// @fn IndexGroup::add_data_v(void* data)
/// Add raw data to this IndexGroup, bypassing integrity checks.
/// This is called by the IndexGroup::add_data function after integrity
///checks have passed. It is also called by the ODB class when adding data
///to its ODB::all group as data verification is unnecessary (it is guaranteed
///to pass).
/// @param [in] data Pointer to the data in memory.

/// @fn ODB* IndexGroup::query(Condition* condition, DataStore* ds)
/// Perform a general query and insert the results.
/// This is called by IndexGroup::query(bool (*)(void*)) after the creation
///of the resulting DataStore is complete. A pointer to that DataStore object
///is then passed around so the results from each query can be inserted into it.
/// @param [in] condition A condition function that returns true if the piece
///of data 'passes' (and should be added to the query results) and false if
///the data fails (and will not be returned in the results).
/// @param [in] ds A pointer to a datastore that will be filled with the
///results of the query.

/// @var int IndexGroup::ident
/// Identifier

/// @var DataStore* IndexGroup::parent
/// Pointer to the parent datastore for use when running a query.

/// @var std::vector<IndexGroup*> IndexGroup::indices
/// An STL implementation of a vector that stores pointers to the IndexGroups.
/// An expanding list of pointers to the IndexGroups contained in this group.
///These are iterated over whenever data is added or a query is run.

/// @class Index
/// Superclass for all specific implementations of indices (linked lists, redblack
///trees, tries, etc).
/// This provides the superclass for all indices and inherits publicly from IndexGroup
///allowing it to be used in all of the same situations (and thus allow it to sit
///next to nested IndexGroups in an IndexGroup).
///
/// Since each DataStore implementation will implement DataStore::populate
///differently, each specific DataStore implementation requires privileged access
///to Index::add_data_v.

/// @fn bool Index::add_data(DataObj* data)
/// Add a piece of data to this Index.
/// A DataObj instance must be inserted as it contains the necessary
///information to ensure that data integrity is maintained through the insertion.
/// @param data A pointer to an instance of DataObj that represents the piece
///of data to be added.
/// @retval true The identifiers between this Index and the new data matched
///and the operation succeeded.
/// @retval false The identifiers did not match and the operation failed.
///No changes were made to any of the involved objects.

/// @fn ODB* Index::query(Condition* condition)
/// Perform a general query on this Index
/// This performs a general query on the index table and stores the results
///in a datastore cloned from this instance's parent datastore.
/// The datastore used is an indirect copy of the datastore that is used in
///this Index's parent (Index::parent).
/// @param condition A condition function that returns true if the piece of
///data 'passes' (and should be added to the query results) and false if the
///data fails (and will not be returned in the results).
/// @return A pointer to an ODB object that represents the query results.
/// @attention This operation is O(N), where N is the number of items in this
///index table.

/// @fn uint64_t Index::size()
/// Get the number of elements in the table.
/// @return The number of elements added to the table. This includes the items
///in embedded lists if duplicates are allowed. If duplicates are allowed
///then this does not necessarily return the number of items in the table,
///but rather the number of items in the table as well as in all embedded lists.

/// @fn bool Index::remove(DataObj* data)
/// Remove an item from the index table. This matches memory location exactly.
/// @param [in] data A piece of data representing what is to be removed.
/// @retval true The removal succeeded and the index table was updated accordingly.
/// @retval false The data was not found in the index and no changes were made.

/// @fn Iterator* Index::it_first()
/// Iterator initialization method.
/// @return A pointer to an iterator that starts at the 'first' item in the index table.

/// @fn Iterator* Index::it_last()
/// Iterator initialization method.
/// @return A pointer to an iterator that starts at the 'last' item in the index table.

/// @fn Iterator* Index::it_lookup(void* rawdata, int8_t dir)
/// Iterator initialization method.
/// @return A pointer to an iterator that starts at the specified data.

/// @fn void Index::it_release(Iterator* it)
/// Release the specified iterator.
/// This releases the memory held by the iterator and takes care of releasing
///the read lock that was acquired then the iterator was created.
/// @param [in] it Iterator to release.

/// @fn Index::Index()
/// Protected default constructor.
/// By reserving the default constructor as protected the compiler cannot
///generate one on its own and allow the user to instantiate a DataStore
///instance.

/// @fn void Index::add_data_v(void* rawdata)
/// Add raw data to this Index, bypassing integrity checks.
/// This is called by the Index::add_data function after integrity checks
///have passed. It is also called by the specific implementations of the
///datastores (BankDS,...) in their version of DataStore::populate.
/// @param [in] data Pointer to the data in memory.

/// @fn void Index::query(Condition* condition, DataStore* ds)
/// Perform a general query and insert the results.
/// This is called by Index::query(bool (*)(void*)) after the creation of
///the resulting DataStore is complete. A pointer to that DataStore object
///is then passed around so the results from each query can be inserted into
///it.
/// @param [in] condition A condition function that returns true if the
///piece of data 'passes' (and should be added to the query results) and
///false if the data fails (and will not be returned in the results).
/// @param [in] ds A pointer to a datastore that will be filled with the
///results of the query.

/// @fn void Index::update(std::vector<void*> old_addr, std::vector<void*> new_addr, uint32_t datalen)
/// Update the data pointers of this index table.
/// This is done under the assumption that the new and old addresses compare as
///equal. This is done in the best time of the underlying index structure.
/// @param [in] old_addr A (not necessarily sorted) vector of pointers to look for.
/// @param [in] new_addr A list of pointers that dictates what the old pointers
///should be changed to.

/// @fn bool Index::remove(void* rawdata)
/// Remove an item from the index table. This matches memory location exactly.
/// @param [in] data A piece of data representing what is to be removed.
/// @retval true The removal succeeded and the index table was updated accordingly.
/// @retval false The data was not found in the index and no changes were made.

/// @fn void Index::remove_sweep(std::vector<void*>* marked)
/// Sweep the datastore and perform batch deletions from the specified list.
/// @param [in] marked List of locations to remove from this index table.

/// @fn int32_t (*Index::compare)(void*, void*)
/// Comparator function.

/// @fn void* (*Index::merge)(void*, void*)
/// Merge function.

/// @fn uint64_t Index::count
/// Number of elements in the tree.

/// @fn bool Index::drop_duplicates
/// Indicator on whether or not to drop duplicates.
