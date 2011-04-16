//conn_#,host_a,host_b,port_a,port_b,first_packet,last_packet,total_packets_a2b,total_packets_b2a,resets_sent_a2b,resets_sent_b2a,ack_pkts_sent_a2b,ack_pkts_sent_b2a,pure_acks_sent_a2b,pure_acks_sent_b2a,sack_pkts_sent_a2b,sack_pkts_sent_b2a,dsack_pkts_sent_a2b,dsack_pkts_sent_b2a,max_sack_blks/ack_a2b,max_sack_blks/ack_b2a,unique_bytes_sent_a2b,unique_bytes_sent_b2a,actual_data_pkts_a2b,actual_data_pkts_b2a,actual_data_bytes_a2b,actual_data_bytes_b2a,rexmt_data_pkts_a2b,rexmt_data_pkts_b2a,rexmt_data_bytes_a2b,rexmt_data_bytes_b2a,zwnd_probe_pkts_a2b,zwnd_probe_pkts_b2a,zwnd_probe_bytes_a2b,zwnd_probe_bytes_b2a,outoforder_pkts_a2b,outoforder_pkts_b2a,pushed_data_pkts_a2b,pushed_data_pkts_b2a,SYN/FIN_pkts_sent_a2b,SYN/FIN_pkts_sent_b2a,req_1323_ws/ts_a2b,req_1323_ws/ts_b2a,adv_wind_scale_a2b,adv_wind_scale_b2a,req_sack_a2b,req_sack_b2a,sacks_sent_a2b,sacks_sent_b2a,urgent_data_pkts_a2b,urgent_data_pkts_b2a,urgent_data_bytes_a2b,urgent_data_bytes_b2a,mss_requested_a2b,mss_requested_b2a,max_segm_size_a2b,max_segm_size_b2a,min_segm_size_a2b,min_segm_size_b2a,avg_segm_size_a2b,avg_segm_size_b2a,max_win_adv_a2b,max_win_adv_b2a,min_win_adv_a2b,min_win_adv_b2a,zero_win_adv_a2b,zero_win_adv_b2a,avg_win_adv_a2b,avg_win_adv_b2a,initial_window_bytes_a2b,initial_window_bytes_b2a,initial_window_pkts_a2b,initial_window_pkts_b2a,ttl_stream_length_a2b,ttl_stream_length_b2a,missed_data_a2b,missed_data_b2a,truncated_data_a2b,truncated_data_b2a,truncated_packets_a2b,truncated_packets_b2a,data_xmit_time_a2b,data_xmit_time_b2a,idletime_max_a2b,idletime_max_b2a,hardware_dups_a2b,hardware_dups_b2a,throughput_a2b,throughput_b2a,

//1,194.7.248.153,172.16.114.148,1028,21,896702544.267986 , 896702572.12615 ,             40,             31,              0,              0,             38,             31,             19,              5,              0,              0,              0,              0,              0,              0,            272,            903,             18,             24,            272,            903,              1,              0,              1,              0,              0,              0,              0,              0,              0,              0,             18,             24,            2/1,            1/1,            N/N,            N/N,              0,              0,              N,              N,              0,              0,              0,              0,              0,              0,           1460,           1460,             29,             96,              6,             14,             15,             37,          32120,          32736,            512,          32735,              0,              0,          31329,          32735,             16,             96,              1,              1,            272,            903,              0,              0,              0,              0,              0,              0,         24.260,         24.596,        10871.1,        10891.4,              0,              0,             10,             33,


#include "odb.hpp"
#include "index.hpp"
#include "buffer.hpp"
#include "iterator.hpp"
#include "utility.hpp"

#include <math.h>
#include <assert.h>
#include <sys/timeb.h>


#define HEADER_SIZE 200
#define BUF_SIZE 1024
#define K_NN 10
#define PERIOD 600

struct flow_stats
{
    uint32_t a_ip;
    uint32_t b_ip;
    uint16_t a_port;
    uint16_t b_port;
    double flow_start;
    double flow_end;
    uint16_t num_packets_a2b;
    uint16_t num_packets_b2a;
    uint16_t num_acks_a2b;
    uint16_t num_acks_b2a;
    uint32_t num_bytes_a2b;
    uint32_t num_bytes_b2a;
    uint16_t num_resent_a2b;
    uint16_t num_resent_b2a;
    uint16_t num_pushed_a2b;
    uint16_t num_pushed_b2a;
    uint16_t num_syns_a2b;
    uint16_t num_syns_b2a;
    uint16_t num_fins_a2b;
    uint16_t num_fins_b2a;
};

uint32_t total;

bool null_prune (void* rawdata)
{
    return false;
}

double distance(struct flow_stats * a, struct flow_stats * b)
{

    double sum=0;

    assert (a != NULL);
    assert (b != NULL);

    sum += SQUARE( (a->a_ip - b->a_ip)/(uint32_t)(-1));
    sum += SQUARE( (a->b_ip - b->b_ip)/(uint32_t)(-1));
    sum += SQUARE( (a->a_port - b->a_port)/(uint16_t)(-1) );
    sum += SQUARE( (a->b_port - b->b_port)/(uint16_t)(-1) );
//     sum += SQUARE( (a->ip_struct.ip_len - b->ip_struct.ip_len)/1500);

    return sqrt(sum);
}


bool process_data (struct flow_stats * cur_stats, char * data)
{
    int throwaway;
    char a_ip [16];
    char b_ip [16];
    int num_matched;
    
    num_matched = sscanf(data, "%u,%[^,],%[^,],%hu,%hu,%lf , %lf",
           &throwaway,
           a_ip,
           b_ip,
           &cur_stats->a_port,
           &cur_stats->b_port,
           &cur_stats->flow_start,
           &cur_stats->flow_end
    );
    
    
    
    if (num_matched < 5)
    {
        return false;
    }
    
    return true;
    
}


uint32_t read_data(ODB* odb, IndexGroup* packets, FILE *fp)
{
    uint32_t num_records = 0;
    uint32_t nbytes;
    char data [BUF_SIZE];
    struct flow_stats cur_stats;
    double period_start;

    struct file_buffer* fb = fb_read_init(fp, 1048576);

    nbytes = fb_read(fb, data, HEADER_SIZE);

    while (nbytes > 0)
    {
        //         nbytes = read(fileno(fp), pheader, sizeof(pcaprec_hdr_t));

//         nbytes = fb_read(fb, pheader, sizeof(pcaprec_hdr_t));

        //         nbytes = read(fileno(fp), data, (pheader->incl_len));
//         nbytes = fb_read(fb, data, pheader->incl_len);

        nbytes = fb_read_line(fb, data, BUF_SIZE);

        
        process_data(&cur_stats, data);
        
        if (period_start == 0)
        {
            period_start = cur_stats.flow_start;
        }
        
        if ((cur_stats.flow_start - period_start) > PERIOD && total > 0)
        {


//             do_it_calcs();
            if (total > K_NN)
            {
//                 printf("%.15f,", lof_calc(odb, packets));
            }
            else
            {
                printf("0,");
            }

//     printf("%d\n", total);

            printf("0\n");
            // Include the timestamp that marks the END of this interval
//             printf("TIMESTAMP %u\n", pheader->ts_sec);
            fflush(stdout);
//             fprintf(stderr, "\n");


            total = 0;

            // Reset the ODB object, and carry forward.
            odb->purge();

            // Change the start time of the preiod.
            period_start += PERIOD;
        }

/*        struct tcpip* rec = (struct tcpip*)malloc(sizeof(struct tcpip));
        get_data(rec, data, pheader->incl_len);*/
//         rec->timestamp = pheader->ts_sec;

        DataObj* dataObj = odb->add_data(&cur_stats, false);

        packets->add_data(dataObj);

        num_records++;
    }
    
    return num_records;
}




int main(int argc, char *argv[])
{
    struct timeb start, end;
    FILE *fp;
    uint64_t num, total_num;
    double dur;
    double totaldur;
    int num_files;
    int i;
    ODB* odb;

    odb = new ODB(ODB::BANK_DS, null_prune, sizeof(struct flow_stats));

    IndexGroup* flows = odb->create_group();

//     src_addr_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_src_addr, merge_src_addr);
//     dst_addr_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_dst_addr, merge_dst_addr);
//     src_port_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_src_port, merge_src_port);
//     dst_port_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_dst_port, merge_dst_port);
//     payload_len_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_payload_len, merge_payload_len);
// 
// 
//     packets->add_index(src_addr_index);
//     packets->add_index(dst_addr_index);
//     packets->add_index(src_port_index);
//     packets->add_index(dst_port_index);
//     packets->add_index(payload_len_index);


    if (argc < 2)
    {
        printf("Alternatively: trace-proc <Number of files> <file name>+\n");
        return EXIT_FAILURE;
    }

    totaldur = 0;
    dur = 0;
    total_num = 0;

    sscanf(argv[1], "%d", &num_files);

    for (i = 0 ; i < num_files ; i++)
    {
        fprintf(stderr, ".");
        fflush(stderr);

        if (((argv[i+2])[0] == '-') && ((argv[i+2])[1] == '\0'))
        {
            fp = stdin;
        }
        else
        {
            fp = fopen(argv[i+2], "rb");
        }

//         printf("%s (%d/%d): \n", argv[i+2], i+1, num_files);

        if (fp == NULL)
        {
            fprintf(stderr, "\n");
            break;
        }

        ftime(&start);

        num = read_data(odb, flows, fp);

//         printf("(");
        fflush(stdout);


        total_num += num;
//         printf("%lu) ", total_num - odb->size());
        fflush(stdout);

        ftime(&end);

        dur = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

        fprintf(stderr, "%f\n", (num / dur));

        totaldur += dur;

        fclose(fp);
        fflush(stdout);
    }

    delete odb;
//     delete packets;
//     delete src_addr_index;
//     delete dst_addr_index;
//     delete src_port_index;
//     delete dst_port_index;
//     delete payload_len_index;

    return EXIT_SUCCESS;
}


