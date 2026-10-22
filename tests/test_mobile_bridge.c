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
static ores_mobile_status invalid_status_handler(void *ctx, const uint8_t *data, size_t length) {
    (void)ctx; (void)data; (void)length;
    return (ores_mobile_status)99;
}
typedef struct {
    uint8_t *source;
    const uint8_t *scratch;
    size_t length;
    ores_mobile_status result;
    unsigned called;
} copy_observer;
static ores_mobile_status copied_handler(void *ctx, const uint8_t *data, size_t length) {
    copy_observer *out = ctx;
    size_t i;
    ++out->called;
    assert(length == out->length);
    assert(data == (length ? out->scratch : NULL));
    if (length) {
        assert(data != out->source && memcmp(data, out->source, length) == 0);
        memset(out->source, 0xff, length);
        for (i = 0; i < length; ++i) assert(data[i] == 0x5a);
    }
    return out->result;
}
static void test_dispatch_copy(void) {
    static uint8_t input[ORES_MOBILE_MAX_MESSAGE_BYTES + 1u];
    static uint8_t scratch[ORES_MOBILE_MAX_MESSAGE_BYTES + 1u];
    const ores_mobile_status results[] = {
        ORES_MOBILE_OK, ORES_MOBILE_INVALID_ARGUMENT,
        ORES_MOBILE_MESSAGE_TOO_LARGE, ORES_MOBILE_BUFFER_TOO_SMALL,
        (ores_mobile_status)99, (ores_mobile_status)-1
    };
    const size_t lengths[] = {0, 1, 3, ORES_MOBILE_MAX_MESSAGE_BYTES};
    copy_observer out = {input, scratch, 0, ORES_MOBILE_OK, 0};
    size_t i, j, k;
    for (i = 0; i < sizeof lengths / sizeof lengths[0]; ++i) {
        for (j = 0; j < sizeof results / sizeof results[0]; ++j) {
            out.length = lengths[i]; out.result = results[j];
            memset(input, 0x5a, sizeof input);
            memset(scratch, 0xa5, sizeof scratch);
            assert(ores_mobile_dispatch_copy(copied_handler, &out,
                out.length ? input : NULL, out.length,
                out.length ? scratch : NULL, out.length) ==
                (j < 4 ? results[j] : ORES_MOBILE_INVALID_ARGUMENT));
            for (k = 0; k < out.length; ++k) assert(scratch[k] == 0);
            assert(scratch[out.length] == 0xa5);
        }
    }
    out.called = 0;
    memset(scratch, 0xa5, sizeof scratch);
    assert(ores_mobile_dispatch_copy(NULL, &out, input, 3, scratch, 3) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_dispatch_copy(copied_handler, &out, NULL, 3, scratch, 3) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_dispatch_copy(copied_handler, &out, input, 3, NULL, 3) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_dispatch_copy(copied_handler, &out, input, 3, scratch, 2) == ORES_MOBILE_BUFFER_TOO_SMALL);
    assert(ores_mobile_dispatch_copy(copied_handler, &out, input, sizeof input, scratch, sizeof scratch) == ORES_MOBILE_MESSAGE_TOO_LARGE);
    /* Exact, forward-partial and reverse-partial aliases all fail before writes. */
    assert(ores_mobile_dispatch_copy(copied_handler, &out, scratch, 3, scratch, 3) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_dispatch_copy(copied_handler, &out, scratch, 3, scratch + 1, 3) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_dispatch_copy(copied_handler, &out, scratch + 1, 3, scratch, 3) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(out.called == 0);
    for (k = 0; k < sizeof scratch; ++k) assert(scratch[k] == 0xa5);
    /* Adjacent spans are allowed; only the consumed scratch span is erased. */
    memset(scratch, 0x5a, sizeof scratch);
    out.source = scratch; out.scratch = scratch + 3;
    out.length = 3; out.result = ORES_MOBILE_OK;
    assert(ores_mobile_dispatch_copy(copied_handler, &out, scratch, 3, scratch + 3, 3) == ORES_MOBILE_OK);
    assert(out.called == 1 && scratch[0] == 0xff && scratch[3] == 0 && scratch[6] == 0x5a);
}
int main(void) {
    test_dispatch_copy();
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
    {
        /* Give the declared range real storage. A fictitious 64 KiB range
         * starting at input[3] can cover written under ASan stack layout,
         * correctly triggering the earlier alias guard instead. */
        struct {
            uint8_t bytes[ORES_MOBILE_MAX_MESSAGE_BYTES + 1u];
            size_t count;
        } oversized = {{0}, 42};
        assert(ores_mobile_copy_message(oversized.bytes, sizeof oversized.bytes,
                    output, sizeof output, &oversized.count) == ORES_MOBILE_MESSAGE_TOO_LARGE);
        assert(oversized.count == 0);
    }
    assert(ores_mobile_copy_message(output, 2, output + 1, 2, &written) == ORES_MOBILE_OK);
    assert(output[1] == 11 && output[2] == 22);
    assert(ores_mobile_dispatch(ok_handler, &value, input, 3) == ORES_MOBILE_OK);
    assert(value.called == 1 && value.count == 3 && value.first == 11);
    assert(ores_mobile_dispatch(ok_handler, &value, NULL, 0) == ORES_MOBILE_OK);
    assert(value.called == 2 && value.count == 0);
    assert(ores_mobile_dispatch(NULL, NULL, input, 3) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_dispatch(ok_handler, &value, NULL, 1) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(ores_mobile_dispatch(fail_handler, NULL, input, 3) == ORES_MOBILE_BUFFER_TOO_SMALL);
    assert(ores_mobile_dispatch(invalid_status_handler, NULL, input, 3) == ORES_MOBILE_INVALID_ARGUMENT);
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
    {
        union { size_t alignment; uint8_t bytes[32]; } storage;
        uint8_t before[sizeof storage.bytes];
        memset(storage.bytes, 0x5a, sizeof storage.bytes);
        memcpy(before, storage.bytes, sizeof before);
        assert(ores_mobile_copy_message(storage.bytes, 3, output, sizeof output,
               (size_t *)(void *)storage.bytes) == ORES_MOBILE_INVALID_ARGUMENT);
        assert(memcmp(before, storage.bytes, sizeof before) == 0);
        assert(ores_mobile_copy_message(input, 3, storage.bytes, sizeof storage.bytes,
               (size_t *)(void *)storage.bytes) == ORES_MOBILE_INVALID_ARGUMENT);
        assert(memcmp(before, storage.bytes, sizeof before) == 0);
    }
    return 0;
}

