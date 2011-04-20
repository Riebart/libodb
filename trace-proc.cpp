//conn_#,host_a,host_b,port_a,port_b,first_packet,last_packet,total_packets_a2b,total_packets_b2a,resets_sent_a2b,resets_sent_b2a,ack_pkts_sent_a2b,ack_pkts_sent_b2a,pure_acks_sent_a2b,pure_acks_sent_b2a,sack_pkts_sent_a2b,sack_pkts_sent_b2a,dsack_pkts_sent_a2b,dsack_pkts_sent_b2a,max_sack_blks/ack_a2b,max_sack_blks/ack_b2a,unique_bytes_sent_a2b,unique_bytes_sent_b2a,actual_data_pkts_a2b,actual_data_pkts_b2a,actual_data_bytes_a2b,actual_data_bytes_b2a,rexmt_data_pkts_a2b,rexmt_data_pkts_b2a,rexmt_data_bytes_a2b,rexmt_data_bytes_b2a,zwnd_probe_pkts_a2b,zwnd_probe_pkts_b2a,zwnd_probe_bytes_a2b,zwnd_probe_bytes_b2a,outoforder_pkts_a2b,outoforder_pkts_b2a,pushed_data_pkts_a2b,pushed_data_pkts_b2a,SYN/FIN_pkts_sent_a2b,SYN/FIN_pkts_sent_b2a,req_1323_ws/ts_a2b,req_1323_ws/ts_b2a,adv_wind_scale_a2b,adv_wind_scale_b2a,req_sack_a2b,req_sack_b2a,sacks_sent_a2b,sacks_sent_b2a,urgent_data_pkts_a2b,urgent_data_pkts_b2a,urgent_data_bytes_a2b,urgent_data_bytes_b2a,mss_requested_a2b,mss_requested_b2a,max_segm_size_a2b,max_segm_size_b2a,min_segm_size_a2b,min_segm_size_b2a,avg_segm_size_a2b,avg_segm_size_b2a,max_win_adv_a2b,max_win_adv_b2a,min_win_adv_a2b,min_win_adv_b2a,zero_win_adv_a2b,zero_win_adv_b2a,avg_win_adv_a2b,avg_win_adv_b2a,initial_window_bytes_a2b,initial_window_bytes_b2a,initial_window_pkts_a2b,initial_window_pkts_b2a,ttl_stream_length_a2b,ttl_stream_length_b2a,missed_data_a2b,missed_data_b2a,truncated_data_a2b,truncated_data_b2a,truncated_packets_a2b,truncated_packets_b2a,data_xmit_time_a2b,data_xmit_time_b2a,idletime_max_a2b,idletime_max_b2a,hardware_dups_a2b,hardware_dups_b2a,throughput_a2b,throughput_b2a,

//1,194.7.248.153,172.16.114.148,1028,21,896702544.267986 , 896702572.12615 ,             40,             31,              0,              0,             38,             31,             19,              5,              0,              0,              0,              0,              0,              0,            272,            903,             18,             24,            272,            903,              1,              0,              1,              0,              0,              0,              0,              0,              0,              0,             18,             24,            2/1,            1/1,            N/N,            N/N,              0,              0,              N,              N,              0,              0,              0,              0,              0,              0,           1460,           1460,             29,             96,              6,             14,             15,             37,          32120,          32736,            512,          32735,              0,              0,          31329,          32735,             16,             96,              1,              1,            272,            903,              0,              0,              0,              0,              0,              0,         24.260,         24.596,        10871.1,        10891.4,              0,              0,             10,             33,


//req_1323_ws/ts_a2b,req_1323_ws/ts_b2a,adv_wind_scale_a2b,adv_wind_scale_b2a,req_sack_a2b,req_sack_b2a,sacks_sent_a2b,sacks_sent_b2a,urgent_data_pkts_a2b,urgent_data_pkts_b2a,urgent_data_bytes_a2b,urgent_data_bytes_b2a,mss_requested_a2b,mss_requested_b2a,max_segm_size_a2b,max_segm_size_b2a,min_segm_size_a2b,min_segm_size_b2a,avg_segm_size_a2b,avg_segm_size_b2a,max_win_adv_a2b,max_win_adv_b2a,min_win_adv_a2b,min_win_adv_b2a,zero_win_adv_a2b,zero_win_adv_b2a,avg_win_adv_a2b,avg_win_adv_b2a,initial_window_bytes_a2b,initial_window_bytes_b2a,initial_window_pkts_a2b,initial_window_pkts_b2a,ttl_stream_length_a2b,ttl_stream_length_b2a,missed_data_a2b,missed_data_b2a,truncated_data_a2b,truncated_data_b2a,truncated_packets_a2b,truncated_packets_b2a,data_xmit_time_a2b,data_xmit_time_b2a,idletime_max_a2b,idletime_max_b2a,hardware_dups_a2b,hardware_dups_b2a,throughput_a2b,throughput_b2a,

#include "odb.hpp"
#include "index.hpp"
#include "buffer.hpp"
#include "iterator.hpp"
#include "utility.hpp"

#include <math.h>
#include <assert.h>
#include <sys/timeb.h>
#include <float.h>
#include <vector>
#include <arpa/inet.h>

//is this right?
#define HEADER_SIZE 1974+9 
#define BUF_SIZE 2048
#define K_NN 10
#define PERIOD 600



struct flow_stats
{
    double flow_start;
    double flow_end;
    uint32_t a_ip;
    uint32_t b_ip;
    uint16_t a_port;
    uint16_t b_port;
    uint32_t num_packets_a2b;
    uint32_t num_packets_b2a;
    uint32_t num_acks_a2b;
    uint32_t num_acks_b2a;
    uint32_t num_bytes_a2b;
    uint32_t num_bytes_b2a;
    uint32_t num_resent_a2b;
    uint32_t num_resent_b2a;
    uint32_t num_pushed_a2b;
    uint32_t num_pushed_b2a;
    uint16_t num_syns_a2b;
    uint16_t num_syns_b2a;
    uint16_t num_fins_a2b;
    uint16_t num_fins_b2a;
};

struct knn
{
    struct flow_stats * p;
    struct knn * neighbors [K_NN];
    double distances [K_NN];
    double k_distance;
    double lrd;
    double LOF;
};

uint32_t total;

bool null_prune (void* rawdata)
{
    return false;
}

int32_t compare_flow_stats_p(void* a, void* b)
{
    assert(a != NULL);
    assert(b != NULL);

    return ((reinterpret_cast<struct knn*>(a)->p - (reinterpret_cast<struct knn*>(b)->p)));
}

#define LAZY(x) sum += SQUARE( ((double)(a->x - b->x))/(uint16_t)(-1))
#define LAZYL(x) sum += SQUARE( ((double)(a->x - b->x))/(uint32_t)(-1))

double distance(struct flow_stats * a, struct flow_stats * b)
{

    double sum=0;

    assert (a != NULL);
    assert (b != NULL);
//     assert (a != b);

    LAZYL(a_ip);
    LAZYL(b_ip);
    LAZY(a_port);
    LAZY(b_port);
    LAZY(num_packets_a2b);
    LAZY(num_packets_b2a);
    LAZY(num_acks_a2b);
    LAZY(num_acks_b2a);
    LAZYL(num_bytes_a2b);
    LAZYL(num_bytes_b2a);
    LAZY(num_resent_a2b);
    LAZY(num_resent_b2a);
    LAZY(num_pushed_a2b);
    LAZY(num_pushed_b2a);
    LAZY(num_syns_a2b);
    LAZY(num_syns_b2a);
    LAZY(num_fins_a2b);
    LAZY(num_fins_b2a);
    
//     sum += SQUARE( (a->ip_struct.ip_len - b->ip_struct.ip_len)/1500);

//     if (sum != 0.0)
//     {
//         fprintf(stderr, "Distance: %f\n", sum);
//     }
    
//     fprintf(stderr, "IPs: %d, %d, %f\n", a->a_ip, b->a_ip, SQUARE( ((double)(a->a_ip - b->a_ip))/(uint32_t)(-1)));
    
    return sqrt(sum);
}

void knn_search(ODB * odb, struct knn * a, int k)
{
    Iterator * it = odb->it_first();

//     int cur_n = 0;
    double cur_dist = 0;
    double temp_dist;
    struct knn * temp_knn;
    DataObj * dobj;

    //the a-check should not be necessary. And yet...
    assert (a != NULL);
    assert (a->p != NULL);
//     if (it->data() != NULL)

    int i;
    for (i=0; i<k; i++)
    {
        a->distances[i] = DBL_MAX;
    }

    if ( (dobj=it->data()) != NULL && a->p != NULL)
    {
        do //for each point
        {
            struct knn * cur_knn = reinterpret_cast<struct knn *>(dobj->get_data());

            assert(cur_knn != NULL);
            assert(cur_knn->p != NULL);

            if (a->p != cur_knn->p)
            {
                cur_dist = distance( a->p, cur_knn->p );

                for (i=0; i<k; i++)
                {
                    //a wee little bubble sort.
                    if (cur_dist < a->distances[i])
                    {
                        temp_knn=a->neighbors[i];
                        temp_dist=a->distances[i];
                        a->distances[i] = cur_dist;
                        a->neighbors[i] = cur_knn;

                        cur_dist = temp_dist;
                        cur_knn = temp_knn;
                    }
                }

//                 cur_n++;
            }
        }
        while ( (dobj=it->next()) != NULL);

    }

    odb->it_release(it);

    a->k_distance = a->distances[K_NN-1];

}

double lof_calc(ODB * odb, IndexGroup * packets)
{
//     std::vector<Index*> indices = packets->flatten();
//     int num_indices = indices.size();
    double max_lof = 0;
    struct knn* max_knn = NULL;

    Iterator * it;

    struct ip * temp;

    struct knn * cur_knn = (struct knn *)malloc(sizeof(struct knn));
    //TODO: determine appropriate struct
    ODB * odb2 = new ODB(ODB::BANK_DS, null_prune, sizeof(struct knn));
    //For now, index by the pointer to data

    Index * knn_index = odb2->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_flow_stats_p, NULL);

    it = odb->it_first();

    //Step 0: populate the data structures
    if (it->data() != NULL)
    {
        do //for each point
        {
            //zero the struct
            memset(cur_knn, 0, sizeof(struct knn));

            cur_knn->p=reinterpret_cast<struct flow_stats*>(it->get_data());

            //this is run-time catch for null pointers
            assert(cur_knn->p != NULL);

            DataObj * dobj = odb2->add_data(cur_knn, false);
            knn_index->add_data(dobj);

        }
        while (it->next() != NULL);
    }

    odb->it_release(it);

    it = odb2->it_first();

    int i;
    int odb_size = odb2->size();

    //Step 1: determine k-nearest-neighbors of each point. O(nlogn), in theory.
    //Our implementation is O(n^2), hence, omp.

#pragma omp parallel for
    for (i=0; i< odb_size; i++)
//     for ( ; it->get_data() != NULL; )
    {
        struct knn * cur_knn;
#pragma omp critical
        {
            cur_knn = reinterpret_cast<struct knn*>(it->get_data());
            it->next();
        }

        knn_search(odb2, cur_knn, K_NN);
    }

    odb2->it_release(it);


    //Step 2: calculate the lrd of each point. This is O(n)

    it = odb2->it_first();

    if (it->data() != NULL)
    {
        double reach_sum = 0.0;
        Iterator * it_o;

        do //for each point p
        {
            struct knn * p = reinterpret_cast<struct knn *>(it->get_data());
            assert(p != NULL);
            //for each point o in p's k-neighborhood
            for (int i=0; i<K_NN; i++)
            {
                assert(p->distances[i] < DBL_MAX);
                assert(p->neighbors[i] != NULL);
//                 reach_sum += MAX(distance(p, p->neighbors[i]),
//                     (reinterpret_cast<struct knn *>(knn_index->it_lookup(p->neighbors[i])->get_data())->k_distance));
                reach_sum += MAX( distance(p->p, p->neighbors[i]->p), p->neighbors[i]->k_distance);
            }

            p->lrd=K_NN/reach_sum;

        }
        while (it->next() != NULL);
    }
    odb2->it_release(it);

    //Step 3: calculate the LOF of each point. O(n)
    it = odb2->it_first();

    if (it->data() != NULL)
    {
        do //for each point p
        {
            double lrd_sum = 0;
            struct knn * p = reinterpret_cast<struct knn *>(it->get_data());

            //for each neighbor o
            //sum of lrd(o)/lrd(p)
            for (int i=0; i<K_NN; i++)
            {
                lrd_sum += (p->neighbors[i]->lrd)/(p->lrd);
            }

            p->LOF = lrd_sum/K_NN;

            if (p->LOF > max_lof)
            {
                max_lof = MAX( max_lof, p->LOF );
                max_knn = p;
            }


        }
        while (it->next() != NULL);
    }

    odb2->it_release(it);
    odb2->purge();

    delete odb2;

//     if (max_knn != NULL && max_knn->p != NULL)
//     {
//         fprintf(stderr, "%d, %d\n", max_knn->p->tcp_struct.th_sport, max_knn->p->tcp_struct.th_dport);
//     }

    return max_lof;

}

bool process_data (struct flow_stats * cur_stats, char * data)
{
    int conn_num;
    int throwaway;
    char a_ip [16];
    char b_ip [16];
    int num_matched;
   
    
    num_matched = sscanf(data, "%d,%[^,],%[^,],%hu,%hu,%lf , %lf , %u, %u, %d, %d, %u, %u, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %u, %u, %u, %u, %u, %u, %d, %d, %d, %d, %d, %d, %u, %u, %hu/%hu, %hu/%hu",
           &conn_num,
           a_ip,
           b_ip,
           &(cur_stats->a_port),
           &(cur_stats->b_port),
           &(cur_stats->flow_start),
           &(cur_stats->flow_end),
           &(cur_stats->num_packets_a2b),
           &(cur_stats->num_packets_b2a),
           &throwaway, //resets_sent_a2b
           &throwaway, //resets_sent_b2a
           &(cur_stats->num_acks_a2b),
           &(cur_stats->num_acks_b2a),
           &throwaway, //pure_acks_a2b
           &throwaway, //pure_acks_b2
           &throwaway, //sack_packets_a2b
           &throwaway, //sack_packets_b2a
           &throwaway, //dsack
           &throwaway,
           &throwaway, //dsack
           &throwaway,
           &throwaway, //unique_bytes_a2b
           &throwaway, //unique_bytes_b2a
           &throwaway, //actual_data_packets
           &throwaway,
           &(cur_stats->num_bytes_a2b),
           &(cur_stats->num_bytes_b2a),
           &(cur_stats->num_resent_a2b),
           &(cur_stats->num_resent_b2a),
           &throwaway, //rxmt_data_bytes
           &throwaway,
           &throwaway,//zwnd? packets
           &throwaway,
           &throwaway,//zwnd? bytes
           &throwaway,
           &throwaway, //OOO pkts
           &throwaway,
           &(cur_stats->num_pushed_a2b),
           &(cur_stats->num_pushed_b2a),
           &(cur_stats->num_syns_a2b),
           &(cur_stats->num_fins_a2b),
           &(cur_stats->num_syns_b2a),
           &(cur_stats->num_fins_b2a)           
    );
    
    inet_pton(AF_INET, a_ip, &(cur_stats->a_ip));
    inet_pton(AF_INET, b_ip, &(cur_stats->b_ip));
    
//      fprintf(stderr, "%.30s %lf\n", data, (cur_stats->flow_start));
    
    if (num_matched < 17)
    {
        return false;
    }
    
    return true;
    
}


uint32_t read_data(ODB* odb, IndexGroup* packets, FILE *fp)
{
    uint32_t num_records = 0;
    int32_t nbytes;
    char data [BUF_SIZE];
    struct flow_stats cur_stats;
    double period_start =0;

    struct file_buffer* fb = fb_read_init(fp, 1048576);

    //throwaway the first 10 lines, and prep the first line
    int i;
    for (i=0; i<11; i++)
    {
        nbytes = fb_read_line(fb, data, BUF_SIZE);
//         printf("%30s", data);
    }

    while (nbytes > 0)
    {
        //         nbytes = read(fileno(fp), pheader, sizeof(pcaprec_hdr_t));

//         nbytes = fb_read(fb, pheader, sizeof(pcaprec_hdr_t));

        //         nbytes = read(fileno(fp), data, (pheader->incl_len));
//         nbytes = fb_read(fb, data, pheader->incl_len);
        
//         printf("%30s", data);
        
        if (process_data(&cur_stats, data) == false)
        {
            fprintf(stderr, "Error on flow: %.30s\n", data);
        }

        //set the start of the period
        if (period_start == 0)
        {
            period_start = cur_stats.flow_start;
        }
        
        if ((cur_stats.flow_start - period_start) > PERIOD && total > 0)
        {

//             do_it_calcs();
            if (total > K_NN)
            {
                printf("%.15f,", lof_calc(odb, packets));
            }
            else
            {
                printf("0,");
            }

            printf("%d\n", total);

//             printf("0\n");
            // Include the timestamp that marks the END of this interval
//             printf("TIMESTAMP %u\n", pheader->ts_sec);
            fflush(stdout);
//             fprintf(stderr, "\n");


            total = 0;

            // Reset the ODB object, and carry forward.
            odb->purge();

            // Change the start time of the preiod.
            period_start += (((int)(cur_stats.flow_start - period_start))/PERIOD)*PERIOD;
        } //if

/*        struct tcpip* rec = (struct tcpip*)malloc(sizeof(struct tcpip));
        get_data(rec, data, pheader->incl_len);*/
//         rec->timestamp = pheader->ts_sec;

        DataObj* dataObj = odb->add_data(&cur_stats, false);

        packets->add_data(dataObj);

        num_records++;
        total++;
        
        nbytes = fb_read_line(fb, data, BUF_SIZE);
//         printf("numbytes: %d\n", nbytes);
    } //while
    
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


