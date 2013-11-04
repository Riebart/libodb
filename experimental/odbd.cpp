/*
- Not thread-safe with the ODB-wide DataObj instance.
- Is the "populate the newly added index table with current DS contents" scheduled and thus non-blocking?
- Verify that index tables/etc... do check the ODB ID to make sure they aren't being misused.
 > If this is working as intended than a process-unique ID for every new reference object passed back from an ODB/index/etc... makes sense
  + Errors are shoved back indicating you are a fuckwit.
- Key-value stores? How done are they, and how did I do them?
- DOCUMENTATION!
 > And of course, all of the ToDo's in there.
- Add an "experimental" folder that contains the odbd and dynamic loading stuff

- Single process-wide (static?) ODBWrapper class that wraps the entirety of the public ODB (and Index/Iterator/etc...?) interfaces.
 > Wraps the creation of ODBs !!!
 > Maintains a mapping of ODB/Index/Iterator/etc... from pointers/object references to IDs (32-bit?) which are returned for reference outside
 > When used with a partially 'native' ODB, can it offer native access to the ODB object? What are the detriments of that?
  + Should the ODB be marked as 'wrapped' and thus before returning from index creation, add them to the wrapper's mappings?
*/

std::vector<ODB*> odb_vec;
std::vector<DataObj*> dataobj_vec;
std::vector<Index*> index_vec;
std::vector<IndexGroup*> indexgroup_vec;
std::vector<Iterator*> iterator_vec;

c4bf ~ODB();
1f30 bool delete_index(Index* index);
dbf9 DataObj* add_data(void* rawdata, bool add_to_all);
4c35 DataObj* add_data(void* rawdata, uint32_t nbytes, bool add_to_all);
5446 Index* create_index(IndexType type, int flags, Comparator* compare, Merger* merge = NULL, Keygen* keygen = NULL, int32_t keylen = -1);
47f5 Index* create_index(IndexType type, int flags, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL, void* (*keygen)(void*) = NULL, int32_t keylen = -1);
f573 IndexGroup* create_group();
84b0 IndexGroup* get_indexes();
1cd9 Iterator* it_first();
24f5 Iterator* it_last();
8b2d ODB(FixedDatastoreType dt, uint32_t datalen, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
48dc ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
a022 ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t (*len)(void*) = len_v, uint32_t sleep_duration = 0, uint32_t flags = 0);
ab40 time_t get_time();
7926 uint32_t start_scheduler(uint32_t num_threads);
ec21 uint64_t size();
37ae virtual bool (*get_prune())(void*);
956a void add_data(void* rawdata);
628c void add_data(void* rawdata, uint32_t nbytes);
4451 void block_until_done();
b041 void it_release(Iterator* it);
bf16 void purge();
ed43 void remove_sweep();
b428 void set_prune(bool (*prune)(void*));
956e void update_time(time_t t);