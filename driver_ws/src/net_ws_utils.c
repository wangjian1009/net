#include "net_ws_utils.h"

const char * net_ws_wslay_err_str(int err) {
    switch(err) {
    case WSLAY_ERR_WANT_READ:
        return "wslay-want-read";
    case WSLAY_ERR_WANT_WRITE:
        return "wslay-want-write";
    case WSLAY_ERR_PROTO:
        return "wslay-proto";
    case WSLAY_ERR_INVALID_ARGUMENT:
        return "wslay-invalid-argument";
    case WSLAY_ERR_INVALID_CALLBACK:
        return "wslay-invalid-callback";
    case WSLAY_ERR_NO_MORE_MSG:
        return "wslay-no-more-msg";
    case WSLAY_ERR_CALLBACK_FAILURE:
        return "wslay-callback-failure";
    case WSLAY_ERR_WOULDBLOCK:
        return "wslay-wouldblock";
    case WSLAY_ERR_NOMEM:
        return "wslay-no-mem";
    default:
        return "wslay-unknown-error";
    }
}

const char * net_ws_wslay_op_code_str(int op_type) {
    switch(op_type) {
    case WSLAY_CONTINUATION_FRAME:
        return "continuation-frame";
    case WSLAY_TEXT_FRAME:
        return "text";
    case WSLAY_BINARY_FRAME:
        return "binary";
    case WSLAY_CONNECTION_CLOSE:
        return "close";
    case WSLAY_PING:
        return "ping";
    case WSLAY_PONG:
        return "pong";
    default:
        return "unknown-wslay-op";
    }
}

