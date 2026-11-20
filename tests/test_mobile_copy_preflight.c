#include "ores_mobile_bridge.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static unsigned int called;
static ores_mobile_status recv_copy(void *ctx, const uint8_t *data, size_t length) {
    (void)ctx;
    ++called;
    assert(length == 3 && data[0] == 1 && data[2] == 3);
    return ORES_MOBILE_OK;
}

int main(void) {
    uint8_t source[5] = {1, 2, 3, 4, 5};
    uint8_t scratch[8] = {0};
    assert(ores_mobile_copy_preflight(NULL, 0, NULL, 0) == ORES_MOBILE_OK);
    assert(ores_mobile_copy_preflight(NULL, 1, scratch, 8) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_copy_preflight(source, 3, NULL, 8) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_copy_preflight(source, 3, scratch, 2) == ORES_MOBILE_BUFFER_TOO_SMALL);
    assert(ores_mobile_copy_preflight(source, 3, source + 2, 3) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_copy_preflight(source, 3, scratch, 8) == ORES_MOBILE_OK);
    assert(ores_mobile_copy_preflight(source, ORES_MOBILE_MAX_MESSAGE_BYTES + 1u,
                                      scratch, sizeof(scratch)) == ORES_MOBILE_MESSAGE_TOO_LARGE);
    assert(ores_mobile_dispatch_copy(recv_copy, NULL, source, 3, scratch, 8) == ORES_MOBILE_OK);
    assert(called == 1 && scratch[0] == 0 && scratch[1] == 0 && scratch[2] == 0);
    assert(ores_mobile_dispatch_copy(recv_copy, NULL, source, 3, source + 2, 3)
           == ORES_MOBILE_INVALID_ARGUMENT);
    assert(called == 1);
    return 0;
}
