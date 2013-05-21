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

#ifndef TRIEI_HPP
#define TRIEI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class TrieI : public Index
{
public:
    TrieI(int keylen);
    virtual void add(void*, void*);

private:
    struct inode
    {
        struct node** next;
    };

    struct dnode
    {
        void* data;
        char key;
    };

    struct inode* root;
    int keylen;
};

TrieI::TrieI()
{
}

#endif
