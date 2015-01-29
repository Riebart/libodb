# MPL2.0 HEADER START
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# MPL2.0 HEADER END
#
# Copyright 2010-2013 Michael Himbeault and Travis Friesen
#

include_directories("include" "../extralib/include" "../extralib/include/spinlock")

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

add_library(odb_static STATIC 
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

