#ifndef DNS_HPP

// Reference: http://www.faqs.org/rfcs/rfc1035.html
// At this point, the packet structure is:

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

int16_t dns_query_str_len(const uint8_t* q, uint32_t q_offset, uint32_t packet_len)
{
    int16_t ret;
    int32_t len = 0;
    uint32_t pos;
    
    while (true)
    {
	pos = q_offset + len;
	
	// If we are at the end of the packet, and still not done the query string, return -1.
	if (pos >= packet_len)
	{
	    ret = -1;
	    break;
	}
	// If we found a position in the query string that marks the start of a pointer, and we have another byte to get from the packet, jump to the new location.
	else if ((q[pos] & 0xc0) == 0xc0)
	{
	    if (pos >= (packet_len - 1))
	    {
		ret = -1;
		break;
	    }
	    
	    len = (-1) * (len + 2);
	    q_offset = ((q[pos] & 0x3F) * 256) + q[pos + 1];
	}
	// If we hit the null terminator.
	else if (q[pos] == 0x00)
	{
	    if (len < -1)
		len *= -1;
	    else
		len++;
	    
	    ret = len;
	    break;
	}
	// Otherwise, we just hit another spacer.
	else
	{
	    if (len >= 0)
		len += q[pos] + 1;
	}
    }
    
    return ret;
}

bool verify_dns_packet(const uint8_t* dns_data, uint32_t packet_len)
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
	len = dns_query_str_len(dns_data, q_offset, packet_len);
	
	if (len < 0)
	    return false;
	else
	    q_offset += (len + sizeof(dns_query));
    }
    
    // Answers
    n = ntohs(hdr->num_answers);
    for (uint16_t i = 0 ; i < n ; i++)
    {
	len = dns_query_str_len(dns_data, q_offset, packet_len);
	
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
	len = dns_query_str_len(dns_data, q_offset, packet_len);
	
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
	len = dns_query_str_len(dns_data, q_offset, packet_len);
	
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

#endif
