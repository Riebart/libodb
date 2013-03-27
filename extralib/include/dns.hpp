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

#ifndef DNS_HPP
#define DNS_HPP

#include "common.hpp"

#define DNS_QUERY_MAX_LEN 16384

// Reference: http://www.faqs.org/rfcs/rfc1035.html
#define DNS_FLAG_RESPONSE       0x8000
#define DNS_FLAG_OPCODE         0x7800
#define DNS_FLAG_AUTHORITATIVE  0x0400
#define DNS_FLAG_TRUNCATED      0x0200
#define DNS_FLAG_RECURSE        0x0100
#define DNS_FLAG_CAN_RECURSE    0x0080
#define DNS_FLAG_Z              0x0040 // flags & DNS_FLAG_Z == 0 must be true always for valid DNS packets.
#define DNS_FLAG_ISAUTH         0x0020 // This applies only to DNSSEC: Specifies whether or not the response is authenticated
#define DNS_FLAG_AUTHOK         0x0010 // This applies only to DNSSEC: Specifies whether or not unauthenticated reponses are allowed.
#define DNS_FLAG_REPLYCODE      0x000F

#define DNS_VERIFY_FLAG_SLACK_SPACE         0x0001
#define DNS_VERIFY_FLAG_FORWARD_POINTER     0x0002

#ifndef ABS
#define ABS(x) (((x) < 0) ? (-1) * (x) : (x))
#endif

#ifndef IS_UPPER
#define IS_UPPER(x) ((('A' <= x) && (x <= 'Z')) ? true : false)
#endif

#ifndef TO_LOWER
#define TO_LOWER(x) (x + ('a' - 'A'))
#endif

struct pre2str
{
    uint16_t len;
    uint8_t data;
};

struct dns_verify_result
{
    uint16_t len;
    uint16_t flags;
};

struct dns_header
{
    uint16_t transaction_id;
    uint16_t flags;
    uint16_t num_questions;
    uint16_t num_answers;
    uint16_t num_auth_ns;
    uint16_t num_other_answers;
    // num_questions queries appear now:
    // num_answers answers appear now;
    // num_auth_ns answers appear now;
    // num_other_answers answers appear now;
};

inline void ntoh_dns_hdr(const struct dns_header* hdr_c)
{
    struct dns_header* hdr = const_cast<struct dns_header*>(hdr_c);
    hdr->transaction_id = ntohs(hdr->transaction_id);
    hdr->flags = ntohs(hdr->flags);
    hdr->num_questions = ntohs(hdr->num_questions);
    hdr->num_answers = ntohs(hdr->num_answers);
    hdr->num_auth_ns = ntohs(hdr->num_auth_ns);
    hdr->num_other_answers = ntohs(hdr->num_other_answers);
}

// Recall that the 'spacers' in a DNS name field must be between 1 and 63
// inclusive, indicating that no one 'piece' can be longer than 63 characters,
// or shorter than 1. The latter is because a value of 0 indicates the end
// of a record.
//
// For this reason, if the top two bits of any spacer are on, then the rest
// of that spacer, and the following byte (total 14 bits) mark the offset from
// the end of the layer 4/UDP header to the first byte of the query. That is, a
// value of 0 marks the start of the ident field (that is 0 = first byte of
// dns data). This is a name pointer, and if it exists, marks the end of that
// name field in which it appears.

struct dns_query
{
    uint16_t type_code; // A, AAAA, NS, etc...
    uint16_t class_code; // IN (Internet address), etc...
};

#pragma pack(1)
struct dns_answer
{
    uint16_t type_code;
    uint16_t class_code;
    uint32_t ttl; // Number of seconds that this record is valid for.
    uint16_t num_bytes; // Number of bytes in the response address;
    //uint8_t addr_start; // Marks the first byte of the address
};
#pragma pack()

struct __dns_query_it
{
    const uint8_t* q;
    uint32_t pos;
    int32_t incl_len;
    uint32_t total_len;
    uint32_t packet_len;
    uint16_t flags; // Contains flags about suspicious, and likely malicious, behaviour.
    bool ptd; // Contains whether or not we've already encountered a pointer. Don't allow nested pointers.
};

const uint8_t* __dns_get_next_token(struct __dns_query_it* q)
{
    // If we are at the end of the packet, and still not done the query string,
    // then this packet is broken. Return NULL;
    if (q->pos >= q->packet_len)
    {
        return NULL;
    }
    // If we just hit another spacer that isn't the start of a pointer...
    else if ((q->q[q->pos] & 0xc0) != 0xc0)
    {
        // If the included length is still positive, then count the token we had.
        if (q->incl_len >= 0)
        {
            q->incl_len += q->q[q->pos] + 1;
        }

        q->total_len += q->q[q->pos] + 1;

        q->pos += q->q[q->pos] + 1;
    }

    // Disallowing chained pointers
    if (q->pos >= q->packet_len)
    {
        return NULL;
    }
    else if ((q->q[q->pos] & 0xc0) == 0xc0)
    {
        // If we already encountered a pointer, and we're encountering another
        // one, then this packet is broken.
        // Alternatively, it may not be broken, but may be malicious.
        if (q->ptd)
        {
            return NULL;
        }

        // If we don't have enough of the packet left to read the poitner, then
        // this packet is broken. Return NULL.
        if (q->pos >= (q->packet_len - 1))
        {
            return NULL;
        }

        // The included length gets incremented by 2 then negated to indicate
        // that the included length stops here.
        if (q->incl_len >= 0)
        {
            q->incl_len = (-1) * (q->incl_len + 2);
        }

        // Move the cursor to the new start of the token.
        uint16_t ptr_val = ((q->q[q->pos] & 0x3F) * 256) + q->q[q->pos + 1];

        // If the pointer takes us forward, then mark the appropriate flag
        if (ptr_val > q->pos)
        {
            q->flags &= DNS_VERIFY_FLAG_FORWARD_POINTER;
        }

        q->pos = ptr_val;
        q->ptd = true;
    }

    // If we hit the null terminator.
    if (q->pos >= q->packet_len)
    {
        return NULL;
    }
    else if (q->q[q->pos] == 0x00)
    {
        // Then just return a pointer to this location.
        // The idea is that if you call for the next token on an iteratore that
        // is already at the end, it just continues to return a pointer to the
        // end.

        if (q->incl_len >= 0)
        {
            q->incl_len = (-1) * (q->incl_len + 1);
        }

        q->total_len++;

        return q->q + q->pos;
    }

    return q->q + q->pos;
}

uint32_t __query_str_len(const uint8_t* p, uint32_t pos, uint32_t packet_len)
{
    int16_t ret;
    struct __dns_query_it q = { p, pos, 0, 0, packet_len, 0, false };

    while (true)
    {
        p = __dns_get_next_token(&q);

        if (p == NULL)
        {
            ret = -1;
            break;
        }
        else if (p[0] == 0)
        {
            ret = ABS(q.incl_len);
            break;
        }
    }

    return (q.flags << 16) + (uint16_t)ret;
}

struct dns_verify_result* dns_verify_packet(const uint8_t* dns_data, uint32_t packet_len)
{
    struct dns_verify_result* result;
    SAFE_CALLOC(struct dns_verify_result*, result, 1, sizeof(struct dns_verify_result));

    const struct dns_header* hdr = reinterpret_cast<const struct dns_header*>(dns_data);

    // First check the flags.
    uint16_t flags = ntohs(hdr->flags);
    if ((flags & DNS_FLAG_Z) != 0)
    {
        free(result);
        return NULL;
    }

    // Now check the query strings in all of the questions.
#define SPLIT_LEN_FLAGS(x) { len = (int16_t)(x & 0xFFFF); query_str_flags = (uint16_t)(x >> 16); }
    uint32_t mux;
    int16_t len;
    uint16_t query_str_flags;

    uint32_t q_offset = sizeof(struct dns_header);
    uint32_t n;
    uint32_t total_n = 0;

    // Queries
    n = ntohs(hdr->num_questions);
    total_n += n;
    for (uint16_t i = 0 ; i < n ; i++)
    {
        mux = __query_str_len(dns_data, q_offset, packet_len);
        SPLIT_LEN_FLAGS(mux);
        result->flags |= query_str_flags;

        if ((len < 0) || ((q_offset + len + sizeof(struct dns_query)) > packet_len))
        {
            free(result);
            return NULL;
        }
        else
        {
            q_offset += (len + sizeof(struct dns_query));
        }

        if (q_offset > packet_len)
        {
            free(result);
            return NULL;
        }
    }

    // Answers
    n = ntohs(hdr->num_answers);
    total_n += n;
    for (uint16_t i = 0 ; i < n ; i++)
    {
        mux = __query_str_len(dns_data, q_offset, packet_len);
        SPLIT_LEN_FLAGS(mux);
        result->flags |= query_str_flags;

        if ((len < 0) || ((q_offset + len + sizeof(struct dns_answer)) > packet_len))
        {
            free(result);
            return NULL;
        }
        else
        {
            struct dns_answer* a = (struct dns_answer*)(dns_data + q_offset + len);

            if ((q_offset + len + sizeof(struct dns_answer) - sizeof(uint16_t) + ntohs(a->num_bytes)) > packet_len)
            {
                free(result);
                return NULL;
            }

            q_offset += (len + sizeof(struct dns_answer) + ntohs(a->num_bytes));
        }

        if (q_offset > packet_len)
        {
            free(result);
            return NULL;
        }
    }

    // Authoritative Nameservers
    n = ntohs(hdr->num_auth_ns);
    total_n += n;
    for (uint16_t i = 0 ; i < n ; i++)
    {
        mux = __query_str_len(dns_data, q_offset, packet_len);
        SPLIT_LEN_FLAGS(mux);
        result->flags |= query_str_flags;

        if ((len < 0) || ((q_offset + len + sizeof(struct dns_answer)) > packet_len))
        {
            free(result);
            return NULL;
        }
        else
        {
            struct dns_answer* a = (struct dns_answer*)(dns_data + q_offset + len);

            if ((q_offset + len + sizeof(struct dns_answer) - sizeof(uint16_t) + ntohs(a->num_bytes)) > packet_len)
            {
                free(result);
                return NULL;
            }

            q_offset += (len + sizeof(struct dns_answer) + ntohs(a->num_bytes));
        }

        if (q_offset > packet_len)
        {
            free(result);
            return NULL;
        }
    }

    // Other Answers
    n = ntohs(hdr->num_other_answers);
    total_n += n;
    for (uint16_t i = 0 ; i < n ; i++)
    {
        mux = __query_str_len(dns_data, q_offset, packet_len);
        SPLIT_LEN_FLAGS(mux);
        result->flags |= query_str_flags;

        if ((len < 0) || ((q_offset + len + sizeof(struct dns_answer)) > packet_len))
        {
            free(result);
            return NULL;
        }
        else
        {
            struct dns_answer* a = (struct dns_answer*)(dns_data + q_offset + len);

            if ((q_offset + len + sizeof(struct dns_answer) - sizeof(uint16_t) + ntohs(a->num_bytes)) > packet_len)
            {
                free(result);
                return NULL;
            }

            q_offset += (len + sizeof(struct dns_answer) + ntohs(a->num_bytes));
        }

        if (q_offset > packet_len)
        {
            free(result);
            return NULL;
        }
    }

    result->len = q_offset;

    if (total_n == 0)
    {
        free(result);
        return NULL;
    }
    else
    {
        return result;
    }
}

void str_tolower(char* str)
{
    uint32_t i = 0;

    while (str[i] != 0)
    {
        if (IS_UPPER(str[i]))
        {
            str[i] = TO_LOWER(str[i]);
        }

        i++;
    }
}

// This is very similar to __query_str_len which could lend to turning the latter into
// doing this one's job. Tie this into protoparse, and you can output multiple queries
// per packet.
uint16_t dns_get_query_string(const uint8_t* p, char** strp, uint32_t packet_len)
{
    char str[DNS_QUERY_MAX_LEN];
    uint32_t len = DNS_QUERY_MAX_LEN - 1;
    str[len] = 0;

    struct __dns_query_it q = { p, sizeof(struct dns_header), 0, 0, packet_len, 0, false };

    if ((p[q.pos] & 0xc0) != 0xc0)
    {
        len -= (p[q.pos] + 1);
        memcpy(str + len, p + q.pos, p[q.pos] + 1);
    }

    while (p[0] > 0)
    {
        p = __dns_get_next_token(&q);

        if (p[0] > 0)
        {
            len -= (p[0] + 1);
            memcpy(str + len, p, p[0] + 1);
        }
    }

    SAFE_MALLOC(char*, *strp, (DNS_QUERY_MAX_LEN - len));
    if (*strp == NULL)
    {
        return 0;
    }

    memcpy(*strp, str + len, DNS_QUERY_MAX_LEN - len);
    str_tolower(*strp);

    return DNS_QUERY_MAX_LEN - len;
}

void dns_print(FILE* fp, char* q)
{
    uint32_t pos = 0;
    uint32_t jump1 = q[0] + 1;
    uint32_t jump2;

    while (true)
    {
        if (jump1 == 0)
        {
            break;
        }

        jump2 = q[pos + jump1];
        q[pos + jump1] = 0;
        fprintf(fp, "%s.", q + (pos == 0 ? 1 : pos));
        q[pos + jump1] = jump2;
        pos += jump1 + 1;
        jump1 = jump2;
    }
}

#endif
