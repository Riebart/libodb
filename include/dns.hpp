#ifndef DNS_HPP
#define DNS_HPP

#define DNS_QUERY_MAX_LEN 2048

// Reference: http://www.faqs.org/rfcs/rfc1035.html
#define DNS_FLAG_RESPONSE 	0x8000
#define DNS_FLAG_OPCODE 	0x7800
#define DNS_FLAG_AUTHORITATIVE 	0x0400
#define DNS_FLAG_TRUNCATED 	0x0200
#define DNS_FLAG_RECURSE 	0x0100
#define DNS_FLAG_CAN_RECURSE 	0x0080
#define DNS_FLAG_Z 		0x0040 // flags & DNS_FLAG_Z == 0 must be true always for valid DNS packets.
#define DNS_FLAG_ISAUTH		0x0020 // This applies only to DNSSEC: Specifies whether or not the response is authenticated
#define DNS_FLAG_AUTHOK		0x0010 // This applies only to DNSSEC: Specifies whether or not unauthenticated reponses are allowed.
#define DNS_FLAG_REPLYCODE	0x000F

#ifndef ABS
#define ABS(x) (((x) < 0) ? (-1) * (x) : (x))
#endif

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
// the end of the layer 3 header to the first byte of the query. That is, a
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
};

const uint8_t* __dns_get_next_token(struct __dns_query_it* q)
{
    // If we are at the end of the packet, and still not done the query string,
    // then this packet is broken. Return NULL;
    if (q->pos >= q->packet_len)
        return NULL;
    // If we just hit another spacer that isn't the start of a pointer...
    else if ((q->q[q->pos] & 0xc0) != 0xc0)
    {
        // If the included length is still positive, then count the token we had.
        if (q->incl_len >= 0)
            q->incl_len += q->q[q->pos] + 1;

        q->total_len += q->q[q->pos] + 1;

        q->pos += q->q[q->pos] + 1;
    }

    // We might have chained pointers, so this handles that.
    while ((q->q[q->pos] & 0xc0) == 0xc0)
    {
        // If we don't have enough of the packet left to read the poitner, then
        // this packet is broken. Return NULL.
        if (q->pos >= (q->packet_len - 1))
            return NULL;

        // The included length gets incremented by 2 then negated to indicate
        // that the included length stops here.
        if (q->incl_len >= 0)
            q->incl_len = (-1) * (q->incl_len + 2);

        // Move the cursor to the new start of the token.
        q->pos = ((q->q[q->pos] & 0x3F) * 256) + q->q[q->pos + 1];
    }

    // If we hit the null terminator.
    if (q->q[q->pos] == 0x00)
    {
        // Then just return a pointer to this location.
        // The idea is that if you call for the next token on an iteratore that
        // is already at the end, it just continues to return a pointer to the
        // end.

        if (q->incl_len >= 0)
            q->incl_len = (-1) * (q->incl_len + 1);

        q->total_len++;

        return q->q + q->pos;
    }

    return q->q + q->pos;
}

int16_t __query_str_len(const uint8_t* p, uint32_t pos, uint32_t packet_len)
{
    struct __dns_query_it q = { p, pos, 0, 0, packet_len };

    while (true)
    {
        p = __dns_get_next_token(&q);

        if (p == NULL)
        {
            return -1;
        }
        else if (p[0] == 0)
        {
            return ABS(q.incl_len);
        }
    }

    return -1;
}

bool dns_verify_packet(const uint8_t* dns_data, uint32_t packet_len)
{
    const struct dns_header* hdr = reinterpret_cast<const struct dns_header*>(dns_data);

    // First check the flags.
    uint16_t flags = ntohs(hdr->flags);
    if ((flags & DNS_FLAG_Z) != 0)
        return false;

    // Now check the query strings in all of the questions.
    int16_t len;
    uint32_t q_offset = sizeof(struct dns_header);
    uint16_t n;

    // Queries
    n = ntohs(hdr->num_questions);
    for (uint16_t i = 0 ; i < n ; i++)
    {
        len = __query_str_len(dns_data, q_offset, packet_len);

        if (len < 0)
            return false;
        else
            q_offset += (len + sizeof(dns_query));
    }

    // Answers
    n = ntohs(hdr->num_answers);
    for (uint16_t i = 0 ; i < n ; i++)
    {
        len = __query_str_len(dns_data, q_offset, packet_len);

        if (len < 0)
            return false;
        else
        {
            struct dns_answer* a = (struct dns_answer*)(dns_data + q_offset + len);
            q_offset += (len + sizeof(dns_answer) + ntohs(a->num_bytes));
        }
    }

    // Authoritative Nameservers
    n = ntohs(hdr->num_auth_ns);
    for (uint16_t i = 0 ; i < n ; i++)
    {
        len = __query_str_len(dns_data, q_offset, packet_len);

        if (len < 0)
            return false;
        else
        {
            struct dns_answer* a = (struct dns_answer*)(dns_data + q_offset + len);
            q_offset += (len + sizeof(dns_answer) + ntohs(a->num_bytes));
        }
    }

    // Other Answers
    n = ntohs(hdr->num_other_answers);
    for (uint16_t i = 0 ; i < n ; i++)
    {
        len = __query_str_len(dns_data, q_offset, packet_len);

        if (len < 0)
            return false;
        else
        {
            struct dns_answer* a = (struct dns_answer*)(dns_data + q_offset + len);
            q_offset += (len + sizeof(dns_answer) + ntohs(a->num_bytes));
        }
    }

    return true;
}

uint16_t dns_get_query_string(const uint8_t* p, char** strp)
{
    char str[DNS_QUERY_MAX_LEN];
    uint32_t len = DNS_QUERY_MAX_LEN - 1;
    str[len] = 0;

    struct __dns_query_it q = { p, sizeof(struct dns_header), 0, 0, (uint32_t)(-1) };

    len -= (p[sizeof(struct dns_header)] + 1);
    memcpy(str + len, p + sizeof(dns_header), p[sizeof(struct dns_header)] + 1);

    while (true)
    {
        p = __dns_get_next_token(&q);

        if (p[0] == 0)
            break;

        len -= (p[0] + 1);
        memcpy(str + len, p, p[0] + 1);
    }

    *strp = (char*)malloc(DNS_QUERY_MAX_LEN - len);
    if (*strp == NULL)
        return 0;

    memcpy(*strp, str + len, DNS_QUERY_MAX_LEN - len);

    return DNS_QUERY_MAX_LEN - len;
}

void dns_print(char* q)
{
    uint32_t pos = 0;
    uint32_t jump1 = q[0] + 1;
    uint32_t jump2;

    while (true)
    {
        if (jump1 == 0)
            break;

        jump2 = q[pos + jump1];
        q[pos + jump1] = 0;
        printf("%s.", q + pos);
        q[pos + jump1] = jump2;
        pos += jump1 + 1;
        jump1 = jump2;
    }
}

#endif
