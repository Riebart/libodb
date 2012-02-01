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

#include "iterator.hpp"
#include "index.hpp"
#include "datastore.hpp"
#include "common.hpp"

Iterator::Iterator()
{
    dataobj = new DataObj();
    it = NULL;
}

Iterator::Iterator(int ident, uint32_t _true_datalen, bool _time_stamp, bool _query_count)
{
    dataobj = new DataObj(ident);
    this->time_stamp = _time_stamp;
    this->query_count = _query_count;
    this->true_datalen = _true_datalen;
    it = NULL;
}

Iterator::~Iterator()
{
    delete dataobj;
}

DataObj* Iterator::next()
{
    NOT_IMPLEMENTED("Iterator::next()");
    return NULL;
}

DataObj* Iterator::prev()
{
    NOT_IMPLEMENTED("Iterator::prev()");
    return NULL;
}

DataObj* Iterator::data()
{
    if (dataobj->data == NULL)
    {
        return NULL;
    }
    else
    {
        return dataobj;
    }
}

void* Iterator::get_data()
{
    return dataobj->data;
}

time_t Iterator::get_time_stamp()
{
    return (time_stamp ? GET_TIME_STAMP(dataobj->data, true_datalen) : 0);
}

uint32_t Iterator::get_query_count()
{
    return (query_count ? GET_QUERY_COUNT(dataobj->data, true_datalen) : 0);
}

void Iterator::update_query_count()
{
    if (query_count)
    {
        UPDATE_QUERY_COUNT(dataobj->data, true_datalen);
    }
}

