#ifndef NET_NDT7_MODEL_H_INCLEDED
#define NET_NDT7_MODEL_H_INCLEDED
#include "cpe/utils/string_utils.h"
#include "net_ndt7_system.h"

NET_BEGIN_DECL

enum net_ndt7_test_type {
    net_ndt7_test_upload,
    net_ndt7_test_download,
    net_ndt7_test_download_and_upload,
};

/*sppeedtest*/
struct net_ndt7_app_info {
    int64_t m_elapsed_time_ms;
    double m_num_bytes;
};

enum net_ndt7_response_origin {
    net_ndt7_response_origin_client,
};

struct net_ndt7_response {
    struct net_ndt7_app_info m_app_info;
    net_ndt7_response_origin_t m_origin;
    net_ndt7_test_type_t m_test_type;
};

const char * net_ndt7_response_dump(mem_buffer_t buffer, net_ndt7_response_t response);

/**/
struct net_ndt7_connection_info {
    char m_client[64];
    char m_server[64];
    char m_uuid[64];
};

struct net_ndt7_bbr_info {
    int64_t m_bw;
    int64_t m_min_rtt;
    int64_t m_pacing_gain;
    int64_t m_cwnd_gain;
    int64_t m_elapsed_time_ms;
};

struct net_ndt7_tcp_info {
    int64_t m_state;
    int64_t m_ca_state;
    int64_t m_retransmits;
    int64_t m_probes;
    int64_t m_backoff;
    int64_t m_options;
    int64_t m_w_scale;
    int64_t m_app_limited;
    int64_t m_rto;
    int64_t m_ato;
    int64_t m_snd_mss;
    int64_t m_rcv_mss;
    int64_t m_unacked;    
    int64_t m_sacked;
    int64_t m_lost;
    int64_t m_retrans;
    int64_t m_fackets;
    int64_t m_last_data_sent;
    int64_t m_last_ack_sent;
    int64_t m_last_data_recv;
    int64_t m_last_ack_recv;
    int64_t m_pmtu;
    int64_t m_recv_ss_thresh;
    int64_t m_rtt;
    int64_t m_rtt_var;
    int64_t m_snd_ss_thresh;
    int64_t m_snd_cwnd;
    int64_t m_adv_mss;
    int64_t m_reordering;
    int64_t m_rcv_rtt;
    int64_t m_rcv_space;
    int64_t m_total_retrans;
    int64_t m_pacing_rate;
    int64_t m_max_pacing_rate;
    int64_t m_bytes_acked;
    int64_t m_bytes_received;
    int64_t m_segs_out;
    int64_t m_segs_in;
    int64_t m_notsent_bytes;
    int64_t m_min_rtt;
    int64_t m_data_segs_in;
    int64_t m_data_segs_out;
    int64_t m_delivery_rate;
    int64_t m_busy_time;
    int64_t m_r_wnd_limited;
    int64_t m_snd_buf_limited;
    int64_t m_delivered;
    int64_t m_delivered_ce;
    int64_t m_bytes_sent;
    int64_t m_bytes_retrans;
    int64_t m_d_sack_dups;
    int64_t m_reord_seen;
    int64_t m_rcv_ooo_pack;
    int64_t m_snd_wnd;
    int64_t m_elapsed_time;
};

struct net_ndt7_measurement {
    struct net_ndt7_connection_info m_connection_info;
    struct net_ndt7_bbr_info m_bbr_info;
    struct net_ndt7_tcp_info m_tcp_info;
};

const char * net_ndt7_measurement_dump(mem_buffer_t buffer, net_ndt7_measurement_t measurement);

NET_END_DECL

#endif
