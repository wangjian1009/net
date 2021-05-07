#include "yajl/yajl_gen.h"
#include "yajl/yajl_tree.h"
#include "net_ndt7_json_i.h"
#include "net_ndt7_model_i.h"

/*response*/
void net_ndt7_response_to_json(yajl_gen gen, net_ndt7_response_t response) {
    yajl_gen_map_open(gen);

    yajl_gen_string2(gen, "AppInfo");
    yajl_gen_map_open(gen);

    yajl_gen_string2(gen, "ElapsedTime");
    yajl_gen_integer(gen, response->m_app_info.m_elapsed_time_ms);
    
    yajl_gen_string2(gen, "NumBytes");
    yajl_gen_double(gen, response->m_app_info.m_num_bytes);

    yajl_gen_map_close(gen);

    yajl_gen_string2(gen, "Origin");
    switch(response->m_origin) {
    case net_ndt7_response_origin_client:
        yajl_gen_string2(gen, "Client");
        break;
    }

    yajl_gen_string2(gen, "Test");
    switch(response->m_test_type) {
    case net_ndt7_test_upload:
        yajl_gen_string2(gen, "upload");
        break;
    case net_ndt7_test_download:
        yajl_gen_string2(gen, "download");
        break;
    case net_ndt7_test_download_and_upload:
        yajl_gen_string2(gen, "DownloadAndUpload");
        break;
    }
    
    yajl_gen_map_close(gen);
}

void net_ndt7_response_from_json(net_ndt7_response_t response, yajl_val val, error_monitor_t em) {
}

/*connection info*/
void net_ndt7_connection_info_from_json(net_ndt7_connection_info_t connection_info, yajl_val val, error_monitor_t em) {
    const char * str_value;

    if ((str_value = yajl_get_string(yajl_tree_get_2(val, "Client", yajl_t_string)))) {
        cpe_str_dup(connection_info->m_client, sizeof(connection_info->m_client), str_value);
    }
    else {
        connection_info->m_client[0] = 0;
    }

    if ((str_value = yajl_get_string(yajl_tree_get_2(val, "Server", yajl_t_string)))) {
        cpe_str_dup(connection_info->m_server, sizeof(connection_info->m_server), str_value);
    }
    else {
        connection_info->m_server[0] = 0;
    }

    if ((str_value = yajl_get_string(yajl_tree_get_2(val, "UUID", yajl_t_string)))) {
        cpe_str_dup(connection_info->m_uuid, sizeof(connection_info->m_uuid), str_value);
    }
    else {
        connection_info->m_uuid[0] = 0;
    }
}

void net_ndt7_connection_info_to_json(yajl_gen gen, net_ndt7_connection_info_t connection_info) {
    yajl_gen_map_open(gen);

    yajl_gen_string2(gen, "Client");
    yajl_gen_string2(gen, connection_info->m_client);

    yajl_gen_string2(gen, "Server");
    yajl_gen_string2(gen, connection_info->m_server);

    yajl_gen_string2(gen, "UUID");
    yajl_gen_string2(gen, connection_info->m_uuid);

    yajl_gen_map_close(gen);
}

/*bbr info*/
void net_ndt7_bbr_info_from_json(net_ndt7_bbr_info_t bbr_info, yajl_val val, error_monitor_t em) {
    bbr_info->m_bw = yajl_get_integer(yajl_tree_get_2(val, "BW", yajl_t_number));
    bbr_info->m_min_rtt = yajl_get_integer(yajl_tree_get_2(val, "MinRTT", yajl_t_number));
    bbr_info->m_pacing_gain = yajl_get_integer(yajl_tree_get_2(val, "PacingGain", yajl_t_number));
    bbr_info->m_cwnd_gain = yajl_get_integer(yajl_tree_get_2(val, "CwndGain", yajl_t_number));
    bbr_info->m_elapsed_time_ms = yajl_get_integer(yajl_tree_get_2(val, "ElapsedTime", yajl_t_number));
}

void net_ndt7_bbr_info_to_json(yajl_gen gen, net_ndt7_bbr_info_t bbr_info) {
    yajl_gen_map_open(gen);

    yajl_gen_string2(gen, "BW");
    yajl_gen_integer(gen, bbr_info->m_bw);

    yajl_gen_string2(gen, "MinRTT");
    yajl_gen_integer(gen, bbr_info->m_min_rtt);

    yajl_gen_string2(gen, "PacingGain");
    yajl_gen_integer(gen, bbr_info->m_pacing_gain);

    yajl_gen_string2(gen, "CwndGain");
    yajl_gen_integer(gen, bbr_info->m_cwnd_gain);

    yajl_gen_string2(gen, "ElapsedTime");
    yajl_gen_integer(gen, bbr_info->m_elapsed_time_ms);
    
    yajl_gen_map_close(gen);
}

/*tcp info*/
void net_ndt7_tcp_info_from_json(net_ndt7_tcp_info_t tcp_info, yajl_val val, error_monitor_t em) {
    tcp_info->m_state = yajl_get_integer(yajl_tree_get_2(val, "State", yajl_t_number));
    tcp_info->m_ca_state = yajl_get_integer(yajl_tree_get_2(val, "CAState", yajl_t_number));
    tcp_info->m_retransmits = yajl_get_integer(yajl_tree_get_2(val, "Retransmits", yajl_t_number));
    tcp_info->m_probes = yajl_get_integer(yajl_tree_get_2(val, "Probes", yajl_t_number));
    tcp_info->m_backoff = yajl_get_integer(yajl_tree_get_2(val, "Backoff", yajl_t_number));
    tcp_info->m_options = yajl_get_integer(yajl_tree_get_2(val, "Options", yajl_t_number));
    tcp_info->m_w_scale = yajl_get_integer(yajl_tree_get_2(val, "WScale", yajl_t_number));
    tcp_info->m_app_limited = yajl_get_integer(yajl_tree_get_2(val, "AppLimited", yajl_t_number));
    tcp_info->m_rto = yajl_get_integer(yajl_tree_get_2(val, "RTO", yajl_t_number));
    tcp_info->m_ato = yajl_get_integer(yajl_tree_get_2(val, "ATO", yajl_t_number));
    tcp_info->m_snd_mss = yajl_get_integer(yajl_tree_get_2(val, "SndMSS", yajl_t_number));
    tcp_info->m_rcv_mss = yajl_get_integer(yajl_tree_get_2(val, "RcvMSS", yajl_t_number));
    tcp_info->m_unacked = yajl_get_integer(yajl_tree_get_2(val, "Unacked", yajl_t_number));
    tcp_info->m_sacked = yajl_get_integer(yajl_tree_get_2(val, "Sacked", yajl_t_number));
    tcp_info->m_lost = yajl_get_integer(yajl_tree_get_2(val, "Lost", yajl_t_number));
    tcp_info->m_retrans = yajl_get_integer(yajl_tree_get_2(val, "Retrans", yajl_t_number));
    tcp_info->m_fackets = yajl_get_integer(yajl_tree_get_2(val, "Fackets", yajl_t_number));
    tcp_info->m_last_data_sent = yajl_get_integer(yajl_tree_get_2(val, "LastDataSent", yajl_t_number));
    tcp_info->m_last_ack_sent = yajl_get_integer(yajl_tree_get_2(val, "LastAckSent", yajl_t_number));
    tcp_info->m_last_data_recv = yajl_get_integer(yajl_tree_get_2(val, "LastDataRecv", yajl_t_number));
    tcp_info->m_last_ack_recv = yajl_get_integer(yajl_tree_get_2(val, "LastAckRecv", yajl_t_number));
    tcp_info->m_pmtu = yajl_get_integer(yajl_tree_get_2(val, "PMTU", yajl_t_number));
    tcp_info->m_recv_ss_thresh = yajl_get_integer(yajl_tree_get_2(val, "RcvSsThresh", yajl_t_number));
    tcp_info->m_rtt = yajl_get_integer(yajl_tree_get_2(val, "RTT", yajl_t_number));
    tcp_info->m_rtt_var = yajl_get_integer(yajl_tree_get_2(val, "RTTVar", yajl_t_number));
    tcp_info->m_snd_ss_thresh = yajl_get_integer(yajl_tree_get_2(val, "SndSsThresh", yajl_t_number));
    tcp_info->m_snd_cwnd = yajl_get_integer(yajl_tree_get_2(val, "SndCwnd", yajl_t_number));
    tcp_info->m_adv_mss = yajl_get_integer(yajl_tree_get_2(val, "AdvMSS", yajl_t_number));
    tcp_info->m_reordering = yajl_get_integer(yajl_tree_get_2(val, "Reordering", yajl_t_number));
    tcp_info->m_rcv_rtt = yajl_get_integer(yajl_tree_get_2(val, "RcvRTT", yajl_t_number));
    tcp_info->m_rcv_space = yajl_get_integer(yajl_tree_get_2(val, "RcvSpace", yajl_t_number));
    tcp_info->m_total_retrans = yajl_get_integer(yajl_tree_get_2(val, "TotalRetrans", yajl_t_number));
    tcp_info->m_pacing_rate = yajl_get_integer(yajl_tree_get_2(val, "PacingRate", yajl_t_number));
    tcp_info->m_max_pacing_rate = yajl_get_integer(yajl_tree_get_2(val, "MaxPacingRate", yajl_t_number));
    tcp_info->m_bytes_acked = yajl_get_integer(yajl_tree_get_2(val, "BytesAcked", yajl_t_number));
    tcp_info->m_bytes_received = yajl_get_integer(yajl_tree_get_2(val, "BytesReceived", yajl_t_number));
    tcp_info->m_segs_out = yajl_get_integer(yajl_tree_get_2(val, "SegsOut", yajl_t_number));
    tcp_info->m_segs_in = yajl_get_integer(yajl_tree_get_2(val, "SegsIn", yajl_t_number));
    tcp_info->m_notsent_bytes = yajl_get_integer(yajl_tree_get_2(val, "NotsentBytes", yajl_t_number));
    tcp_info->m_min_rtt = yajl_get_integer(yajl_tree_get_2(val, "MinRTT", yajl_t_number));
    tcp_info->m_data_segs_in = yajl_get_integer(yajl_tree_get_2(val, "DataSegsIn", yajl_t_number));
    tcp_info->m_data_segs_out = yajl_get_integer(yajl_tree_get_2(val, "DataSegsOut", yajl_t_number));
    tcp_info->m_delivery_rate = yajl_get_integer(yajl_tree_get_2(val, "DeliveryRate", yajl_t_number));
    tcp_info->m_busy_time = yajl_get_integer(yajl_tree_get_2(val, "BusyTime", yajl_t_number));
    tcp_info->m_r_wnd_limited = yajl_get_integer(yajl_tree_get_2(val, "RWndLimited", yajl_t_number));
    tcp_info->m_snd_buf_limited = yajl_get_integer(yajl_tree_get_2(val, "SndBufLimited", yajl_t_number));
    tcp_info->m_delivered = yajl_get_integer(yajl_tree_get_2(val, "Delivered", yajl_t_number));
    tcp_info->m_delivered_ce = yajl_get_integer(yajl_tree_get_2(val, "DeliveredCE", yajl_t_number));
    tcp_info->m_bytes_sent = yajl_get_integer(yajl_tree_get_2(val, "BytesSent", yajl_t_number));
    tcp_info->m_bytes_retrans = yajl_get_integer(yajl_tree_get_2(val, "BytesRetrans", yajl_t_number));
    tcp_info->m_d_sack_dups = yajl_get_integer(yajl_tree_get_2(val, "DSackDups", yajl_t_number));
    tcp_info->m_reord_seen = yajl_get_integer(yajl_tree_get_2(val, "ReordSeen", yajl_t_number));
    tcp_info->m_rcv_ooo_pack = yajl_get_integer(yajl_tree_get_2(val, "RcvOooPack", yajl_t_number));
    tcp_info->m_snd_wnd = yajl_get_integer(yajl_tree_get_2(val, "SndWnd", yajl_t_number));
    tcp_info->m_elapsed_time = yajl_get_integer(yajl_tree_get_2(val, "ElapsedTime", yajl_t_number));
}

void net_ndt7_tcp_info_to_json(yajl_gen gen, net_ndt7_tcp_info_t tcp_info) {
    yajl_gen_map_open(gen);

    yajl_gen_string2(gen, "State");
    yajl_gen_integer(gen, tcp_info->m_state);

    yajl_gen_string2(gen, "CAState");
    yajl_gen_integer(gen, tcp_info->m_ca_state);

    yajl_gen_string2(gen, "Retransmits");
    yajl_gen_integer(gen, tcp_info->m_retransmits);

    yajl_gen_string2(gen, "Probes");
    yajl_gen_integer(gen, tcp_info->m_probes);

    yajl_gen_string2(gen, "Backoff");
    yajl_gen_integer(gen, tcp_info->m_backoff);

    yajl_gen_string2(gen, "Options");
    yajl_gen_integer(gen, tcp_info->m_options);

    yajl_gen_string2(gen, "WScale");
    yajl_gen_integer(gen, tcp_info->m_w_scale);

    yajl_gen_string2(gen, "AppLimited");
    yajl_gen_integer(gen, tcp_info->m_app_limited);

    yajl_gen_string2(gen, "RTO");
    yajl_gen_integer(gen, tcp_info->m_rto);

    yajl_gen_string2(gen, "ATO");
    yajl_gen_integer(gen, tcp_info->m_ato);

    yajl_gen_string2(gen, "SndMSS");
    yajl_gen_integer(gen, tcp_info->m_snd_mss);

    yajl_gen_string2(gen, "RcvMSS");
    yajl_gen_integer(gen, tcp_info->m_rcv_mss);

    yajl_gen_string2(gen, "Unacked");
    yajl_gen_integer(gen, tcp_info->m_unacked);

    yajl_gen_string2(gen, "Sacked");
    yajl_gen_integer(gen, tcp_info->m_sacked);

    yajl_gen_string2(gen, "Lost");
    yajl_gen_integer(gen, tcp_info->m_lost);

    yajl_gen_string2(gen, "Retrans");
    yajl_gen_integer(gen, tcp_info->m_retrans);

    yajl_gen_string2(gen, "Fackets");
    yajl_gen_integer(gen, tcp_info->m_fackets);

    yajl_gen_string2(gen, "LastDataSent");
    yajl_gen_integer(gen, tcp_info->m_last_data_sent);

    yajl_gen_string2(gen, "LastAckSent");
    yajl_gen_integer(gen, tcp_info->m_last_ack_sent);

    yajl_gen_string2(gen, "LastDataRecv");
    yajl_gen_integer(gen, tcp_info->m_last_data_recv);

    yajl_gen_string2(gen, "LastAckRecv");
    yajl_gen_integer(gen, tcp_info->m_last_ack_recv);

    yajl_gen_string2(gen, "PMTU");
    yajl_gen_integer(gen, tcp_info->m_pmtu);

    yajl_gen_string2(gen, "RcvSsThresh");
    yajl_gen_integer(gen, tcp_info->m_recv_ss_thresh);

    yajl_gen_string2(gen, "RTT");
    yajl_gen_integer(gen, tcp_info->m_rtt);

    yajl_gen_string2(gen, "RTTVar");
    yajl_gen_integer(gen, tcp_info->m_rtt_var);

    yajl_gen_string2(gen, "SndSsThresh");
    yajl_gen_integer(gen, tcp_info->m_snd_ss_thresh);

    yajl_gen_string2(gen, "SndCwnd");
    yajl_gen_integer(gen, tcp_info->m_snd_cwnd);

    yajl_gen_string2(gen, "AdvMSS");
    yajl_gen_integer(gen, tcp_info->m_adv_mss);

    yajl_gen_string2(gen, "Reordering");
    yajl_gen_integer(gen, tcp_info->m_reordering);

    yajl_gen_string2(gen, "RcvRTT");
    yajl_gen_integer(gen, tcp_info->m_rcv_rtt);

    yajl_gen_string2(gen, "RcvSpace");
    yajl_gen_integer(gen, tcp_info->m_rcv_space);

    yajl_gen_string2(gen, "TotalRetrans");
    yajl_gen_integer(gen, tcp_info->m_total_retrans);

    yajl_gen_string2(gen, "PacingRate");
    yajl_gen_integer(gen, tcp_info->m_pacing_rate);

    yajl_gen_string2(gen, "MaxPacingRate");
    yajl_gen_integer(gen, tcp_info->m_max_pacing_rate);

    yajl_gen_string2(gen, "BytesAcked");
    yajl_gen_integer(gen, tcp_info->m_bytes_acked);

    yajl_gen_string2(gen, "BytesReceived");
    yajl_gen_integer(gen, tcp_info->m_bytes_received);

    yajl_gen_string2(gen, "SegsOut");
    yajl_gen_integer(gen, tcp_info->m_segs_out);

    yajl_gen_string2(gen, "SegsIn");
    yajl_gen_integer(gen, tcp_info->m_segs_in);

    yajl_gen_string2(gen, "NotsentBytes");
    yajl_gen_integer(gen, tcp_info->m_notsent_bytes);

    yajl_gen_string2(gen, "MinRTT");
    yajl_gen_integer(gen, tcp_info->m_min_rtt);

    yajl_gen_string2(gen, "DataSegsIn");
    yajl_gen_integer(gen, tcp_info->m_data_segs_in);

    yajl_gen_string2(gen, "DataSegsOut");
    yajl_gen_integer(gen, tcp_info->m_data_segs_out);

    yajl_gen_string2(gen, "DeliveryRate");
    yajl_gen_integer(gen, tcp_info->m_delivery_rate);

    yajl_gen_string2(gen, "BusyTime");
    yajl_gen_integer(gen, tcp_info->m_busy_time);

    yajl_gen_string2(gen, "RWndLimited");
    yajl_gen_integer(gen, tcp_info->m_r_wnd_limited);

    yajl_gen_string2(gen, "SndBufLimited");
    yajl_gen_integer(gen, tcp_info->m_snd_buf_limited);

    yajl_gen_string2(gen, "Delivered");
    yajl_gen_integer(gen, tcp_info->m_delivered);

    yajl_gen_string2(gen, "DeliveredCE");
    yajl_gen_integer(gen, tcp_info->m_delivered_ce);

    yajl_gen_string2(gen, "BytesSent");
    yajl_gen_integer(gen, tcp_info->m_bytes_sent);

    yajl_gen_string2(gen, "BytesRetrans");
    yajl_gen_integer(gen, tcp_info->m_bytes_retrans);

    yajl_gen_string2(gen, "DSackDups");
    yajl_gen_integer(gen, tcp_info->m_d_sack_dups);

    yajl_gen_string2(gen, "ReordSeen");
    yajl_gen_integer(gen, tcp_info->m_reord_seen);

    yajl_gen_string2(gen, "RcvOooPack");
    yajl_gen_integer(gen, tcp_info->m_rcv_ooo_pack);

    yajl_gen_string2(gen, "SndWnd");
    yajl_gen_integer(gen, tcp_info->m_snd_wnd);

    yajl_gen_string2(gen, "ElapsedTime");
    yajl_gen_integer(gen, tcp_info->m_elapsed_time);
    
    yajl_gen_map_close(gen);
}

/*measurement*/
void net_ndt7_measurement_to_json(yajl_gen gen, net_ndt7_measurement_t measurement) {
    yajl_gen_map_open(gen);

    yajl_gen_string2(gen, "ConnectionInfo");
    net_ndt7_connection_info_to_json(gen, &measurement->m_connection_info);
    
    yajl_gen_string2(gen, "BBRInfo");
    net_ndt7_bbr_info_to_json(gen, &measurement->m_bbr_info);

    yajl_gen_string2(gen, "TCPInfo");
    net_ndt7_tcp_info_to_json(gen, &measurement->m_tcp_info);

    yajl_gen_map_close(gen);
}

void net_ndt7_measurement_from_json(net_ndt7_measurement_t measurement, yajl_val val, error_monitor_t em) {
    net_ndt7_connection_info_from_json(&measurement->m_connection_info, yajl_tree_get_2(val, "ConnectionInfo", yajl_t_object), em);
    net_ndt7_bbr_info_from_json(&measurement->m_bbr_info, yajl_tree_get_2(val, "BBRInfo", yajl_t_object), em);
    net_ndt7_tcp_info_from_json(&measurement->m_tcp_info, yajl_tree_get_2(val, "TCPInfo", yajl_t_object), em);
}
