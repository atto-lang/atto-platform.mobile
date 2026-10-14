#include "ores_mobile_bridge.h"
#ifdef NDEBUG
#undef NDEBUG /* evaluate test calls in Release builds */
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct { int called; size_t count; uint8_t first; } observed;
static ores_mobile_status ok_handler(void *ctx, const uint8_t *data, size_t length) {
    observed *out = (observed *)ctx;
    assert(out != NULL);
    out->called++;
    out->count = length;
    out->first = length ? data[0] : 0;
    return ORES_MOBILE_OK;
}
static ores_mobile_status fail_handler(void *ctx, const uint8_t *data, size_t length) {
    (void)ctx; (void)data; (void)length;
    return ORES_MOBILE_BUFFER_TOO_SMALL;
}
int main(void) {
    uint8_t input[] = {11, 22, 33};
    uint8_t output[] = {0, 0, 0};
    size_t written = 42;
    observed value = {0, 0, 0};
    assert(ores_mobile_abi_version() == 1);
    assert(ores_mobile_copy_message(input, 3, output, sizeof output, &written) == ORES_MOBILE_OK);
    assert(written == 3 && memcmp(input, output, 3) == 0);
    assert(ores_mobile_copy_message(input, 3, output, 2, &written) == ORES_MOBILE_BUFFER_TOO_SMALL);
    assert(written == 0);
    assert(ores_mobile_copy_message(NULL, 0, NULL, 0, &written) == ORES_MOBILE_OK && written == 0);
    assert(ores_mobile_copy_message(input, 3, output, 3, NULL) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_copy_message(NULL, 3, output, 3, &written) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_copy_message(input, 3, NULL, 3, &written) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_copy_message(input, ORES_MOBILE_MAX_MESSAGE_BYTES + 1u,
                                    output, 3, &written) == ORES_MOBILE_MESSAGE_TOO_LARGE);
    assert(ores_mobile_copy_message(output, 2, output + 1, 2, &written) == ORES_MOBILE_OK);
    assert(output[1] == 11 && output[2] == 22);
    assert(ores_mobile_dispatch(ok_handler, &value, input, 3) == ORES_MOBILE_OK);
    assert(value.called == 1 && value.count == 3 && value.first == 11);
    assert(ores_mobile_dispatch(ok_handler, &value, NULL, 0) == ORES_MOBILE_OK);
    assert(value.called == 2 && value.count == 0);
    assert(ores_mobile_dispatch(NULL, NULL, input, 3) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_dispatch(ok_handler, &value, NULL, 1) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_dispatch(fail_handler, NULL, input, 3) == ORES_MOBILE_BUFFER_TOO_SMALL);
    assert(ores_mobile_dispatch(ok_handler, &value, input,
                                ORES_MOBILE_MAX_MESSAGE_BYTES + 1u) == ORES_MOBILE_MESSAGE_TOO_LARGE);
    {
        static uint8_t large[ORES_MOBILE_MAX_MESSAGE_BYTES];
        static uint8_t copied[ORES_MOBILE_MAX_MESSAGE_BYTES];
        large[0] = 0x5a;
        large[sizeof large - 1u] = 0xa5;
        assert(ores_mobile_copy_message(large, sizeof large, copied, sizeof copied, &written) == ORES_MOBILE_OK);
        assert(written == sizeof large && copied[0] == 0x5a && copied[sizeof copied - 1u] == 0xa5);
        assert(ores_mobile_copy_message(large, sizeof large, copied, sizeof copied - 1u, &written) == ORES_MOBILE_BUFFER_TOO_SMALL);
        assert(written == 0);
    }
    return 0;
}
