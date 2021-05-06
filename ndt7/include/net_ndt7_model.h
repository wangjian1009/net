#ifndef NET_NDT7_MODEL_H_INCLEDED
#define NET_NDT7_MODEL_H_INCLEDED
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
    net_ndt7_test_type_t m_test_type;
};

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
    int64_t probes;
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
    int64_t m_pmtu;
    /* @SerializedName("RcvSsThresh") val rcvSsThresh: Long?, */
    /* @SerializedName("RTT") val rtt: Long?, */
    /* @SerializedName("RTTVar") val rttVar: Long?, */
    /* @SerializedName("SndSsThresh") val sndSsThresth: Long?, */
    /* @SerializedName("SndCwnd") val sndCwnd: Long?, */
    /* @SerializedName("AdvMSS") val advMss: Long?, */
    /* @SerializedName("Reordering") val reordering: Long?, */
    /* @SerializedName("RcvRTT") val rcvRtt: Long?, */
    /* @SerializedName("RcvSpace") val rcvSpace: Long?, */
    /* @SerializedName("TotalRetrans") val totalRetrans: Long?, */
    /* @SerializedName("PacingRate") val pacingRate: Long?, */
    /* @SerializedName("MaxPacingRate") val maxPacingRate: Long?, */
    /* @SerializedName("BytesAcked") val bytesAcked: Long?, */
    /* @SerializedName("BytesReceived") val bytesReceived: Long?, */
    /* @SerializedName("SegsOut") val segsOut: Long?, */
    /* @SerializedName("SegsIn") val segsIn: Long?, */
    /* @SerializedName("NotsentBytes") val notSentBytes: Long?, */
    /* @SerializedName("MinRTT") val minRtt: Long?, */
    /* @SerializedName("DataSegsIn") val dataSegsIn: Long?, */
    /* @SerializedName("DataSegsOut") val dataSegsOut: Long?, */
    /* @SerializedName("DeliveryRate") val deliveryRate: Long?, */
    /* @SerializedName("BusyTime") val busyTime: Long?, */
    /* @SerializedName("RWndLimited") val rWndLimited: Long?, */
    /* @SerializedName("SndBufLimited") val sndBufLimited: Long?, */
    /* @SerializedName("Delivered") val delivered: Long?, */
    /* @SerializedName("DeliveredCE") val deliveredCE: Long?, */
    /* @SerializedName("BytesSent") val bytesSent: Long?, */
    /* @SerializedName("BytesRetrans") val bytesRetrans: Long?, */
    /* @SerializedName("DSackDups") val dSackDups: Long?, */
    /* @SerializedName("ReordSeen") val reordSeen: Long?, */
    /* @SerializedName("ElapsedTime") val elapsedTime: Long? */
};

struct net_ndt7_measurement {
    struct net_ndt7_connection_info m_connection_info;
    struct net_ndt7_bbr_info m_bbr_info;
    struct net_ndt7_tcp_info m_tcp_info;
};

NET_END_DECL

#endif
