include_directories("include")

add_library(odb SHARED 
            datastore.cpp 
            index.cpp 
            odb.cpp 
            linkedlistds.cpp 
            linkedlisti.cpp 
            bankds.cpp 
            redblacktreei.cpp 
            archive.cpp 
            iterator.cpp
            scheduler.cpp 
            lfqueue.cpp)

