#include <assert.h>
#include "net_ndt7_tester_i.h"

int net_ndt7_tester_upload_start(net_ndt7_tester_t tester) {
    assert(tester->m_state == net_ndt7_tester_state_upload);
    return 0;
}
