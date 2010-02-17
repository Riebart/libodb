/// Header file for Index, IndexGroup and DataObj classes.
/// @file index.hpp
/// @Author Mike

#ifndef INDEX_HPP
#define INDEX_HPP

#include <vector>
#include <stdint.h>

class ODB;
class DataStore;
class Iterator;

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
class DataObj
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
    
    /// Requires ability to create and manipulate DataObj.
    friend class LLIterator;
    

private:
    /// Protected default constructor.
    /// By reserving the default constructor as protected the compiler cannot
    ///generate one on its own and allow the user to instantiate a DataStore
    ///instance.
    DataObj();

    /// Standard constructor
    /// This initializes the two data fields in the object to something useful.
    ///Since all accesses to this class are privileged, the rule here is that
    ///anything that does not change during the lifetime of the object is set by
    ///the constructor. Since the identifier never changes (but the data will
    ///change frequently) only ident is set by the constructor.
    /// @param [in] ident Unique identifier that specifies which instances of the
    ///ODB class this instance of DataObj can be used with.
    DataObj(int ident);

    /// Destructor for any DataObj objects.
    /// Currently this is an empty destructor, but it is around in case it is
    ///needed later.
    ~DataObj();

    /// Identifier
    /// This holds the unique identifier that matches the ODB objects that can
    ///work with it.
    int ident;

    /// Pointer to data.
    void* data;
};

/// Class that takes care of grouping index tables into more abstract logical
///structures.
/// By allowing for groups of index tables to be made more abstract structures
///and relations can be represented. Further, IndexGroups support nesting
///IndexGroups for further abstraction.
/// This means that a uniform interface can be presented to any machine learning
///or artificial intelligence systems placed on top of this backend.
class IndexGroup
{
    /// Allows ODB objects to call the IndexGroup::add_data_v method directly and
    ///skip the overhead of verifying data integrity since that is guaranteed
    ///in that usage scenario.
    friend class ODB;

public:
    /// Standard destructor.
    /// Provided explicitly in case it needs to be expanded later.
    virtual ~IndexGroup();

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
    bool add_index(IndexGroup* ig);

    /// Add a piece of data to this IndexGroup.
    /// A DataObj instance must be inserted as it contains the necessary
    ///information to ensure that data integrity is maintained through the insertion.
    /// @param data A pointer to an instance of DataObj that represents the piece
    ///of data to be added.
    /// @retval true The identifiers between this IndexGroup and the new data
    ///matched and the operation succeeded.
    /// @retval false The identifiers did not match and the operation failed.
    ///No changes were made to any of the involved objects.
    virtual bool add_data(DataObj* data);

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
    /// @warning This opertation is O(N), where N is the number of entries in all
    ///index tables contained in this IndexGroup tree.
    virtual ODB* query(bool (*condition)(void*));

    /// Obtain the identifier of this IndexGroup.
    /// It is possible that one would want to verify the indentifier of an
    ///IndexGroup and this function allows the user to obtain the necessary
    ///information.
    virtual int get_ident();

    /// Get the size of this IndexGroup.
    /// Note that this does not recurse at all and returns a number equal to the
    ///number of times this instance's IndexGroup::add_index function was called.
    /// @return The number of IndexGroups in this IndexGroup.
    /// @todo Add a recursive flavour. It will have to return the number of items
    ///in all contained indices though. Is there a way to stop before indices at
    ///the last layer of IndexGroups?
    virtual uint64_t size();

protected:
    /// Protected default constructor.
    /// By reserving the default constructor as protected the compiler cannot
    ///generate one on its own and allow the user to instantiate a DataStore
    ///instance.
    IndexGroup();

    /// Standard constructor.
    /// The only constructur explicitly called as both items need to be set at
    ///creation (or eventually, anyway).
    /// @param [in] ident Set the identifier to ensure data integrity and restrict
    ///data additions to only compatible data types.
    /// @param [in] parent Set the datastore parent of this IndexGroup. This is
    ///necessary for when the parent is cloned for query returns.
    IndexGroup(int ident, DataStore* parent);

    /// Identifier
    int ident;

    /// Pointer to the parent datastore for use when running a query.
    DataStore* parent;

    /// Add raw data to this IndexGroup, bypassing integrity checks.
    /// This is called by the IndexGroup::add_data function after integrity
    ///checks have passed. It is also called by the ODB class when adding data
    ///to its ODB::all group as data verification is unnecessary (it is guaranteed
    ///to pass).
    /// @param [in] data Pointer to the data in memory.
    virtual void add_data_v(void* data);

    /// Perform a general query and insert the results.
    /// This is called by IndexGroup::query(bool (*)(void*)) after the creation
    ///of the resulting DataStore is complete. A pointer to that DataStore object
    ///is then passed around so the results from each query can be inserted into it.
    /// @param [in] condition A condition function that returns true if the piece
    ///of data 'passes' (and should be added to the query results) and false if
    ///the data fails (and will not be returned in the results).
    /// @param [in] ds A pointer to a datastore that will be filled with the
    ///results of the query.
    virtual void query(bool (*condition)(void*), DataStore* ds);

private:
    /// An STL implementation of a vector that stores pointers to the IndexGroups.
    /// An expanding list of pointers to the IndexGroups contained in this group.
    ///These are iterated over whenever data is added or a query is run.
    /// @todo Move to something homegrown?
    std::vector<IndexGroup*> indices;
};

/// Superclass for all specific implementations of indices (linked lists, redblack
///trees, tries, etc).
/// This provides the superclass for all indices and inherits publicly from IndexGroup
///allowing it to be used in all of the same situations (and thus allow it to sit
///next to nested IndexGroups in an IndexGroup).
///
/// Since each DataStore implementation will implement DataStore::populate
///differently, each specific DataStore implementation requires privileged access
///to Index::add_data_v.
class Index : public IndexGroup
{
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

public:
    /// Add a piece of data to this Index.
    /// A DataObj instance must be inserted as it contains the necessary
    ///information to ensure that data integrity is maintained through the insertion.
    /// @param data A pointer to an instance of DataObj that represents the piece
    ///of data to be added.
    /// @retval true The identifiers between this Index and the new data matched
    ///and the operation succeeded.
    /// @retval false The identifiers did not match and the operation failed.
    ///No changes were made to any of the involved objects.
    virtual bool add_data(DataObj* data);

    /// Perform a general query on this Index
    /// This performs a general query on the index table and stores the results
    ///in a datastore cloned from this instance's parent datastore.
    /// The datastore used is an indirect copy of the datastore that is used in
    ///this Index's parent (Index::parent).
    /// @param condition A condition function that returns true if the piece of
    ///data 'passes' (and should be added to the query results) and false if the
    ///data fails (and will not be returned in the results).
    /// @return A pointer to an ODB object that represents the query results.
    /// @warning This operation is O(N), where N is the number of items in this
    ///index table.
    virtual ODB* query(bool (*condition)(void*));

    /// Get the size of this Index.
    /// @return The number of items in this datastore.
    virtual uint64_t size();
    
    virtual Iterator* it_first();
    virtual Iterator* it_last();
    virtual Iterator* it_middle(DataObj* data);

protected:
    /// Protected default constructor.
    /// By reserving the default constructor as protected the compiler cannot
    ///generate one on its own and allow the user to instantiate a DataStore
    ///instance.
    Index();

    /// Add raw data to this Index, bypassing integrity checks.
    /// This is called by the Index::add_data function after integrity checks
    ///have passed. It is also called by the specific implementations of the
    ///datastores (BankDS,...) in their version of DataStore::populate.
    /// @param [in] data Pointer to the data in memory.
    virtual void add_data_v(void*);

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
    virtual void query(bool (*condition)(void*), DataStore* ds);

    /// Comparator function.
    int (*compare)(void*, void*);

    /// Merge function.
    void* (*merge)(void*, void*);

    /// Number of elements in the tree.
    uint64_t count;

    /// Indicator on whether or not to drop duplicates.
    bool drop_duplicates;
};

class Iterator
{
    public:
        virtual ~Iterator();
        virtual DataObj* next();
        virtual DataObj* data();
        virtual void* data_v();
        
    protected:
        Iterator();
        Iterator(int ident);
        
        DataObj* dataobj;
};

#endif
