#include "ores_mobile_bridge.h"

#include <string.h>

/* Flat-address platform ABI: do not dereference conflicting output pointers. */
static int overlaps_count(const void *data, size_t length, const size_t *written) {
    uintptr_t a, b;
    if (data == NULL || length == 0) return 0;
    a = (uintptr_t)data;
    b = (uintptr_t)written;
    return a >= b ? a - b < sizeof(*written) : b - a < length;
}

uint32_t ores_mobile_abi_version(void) { return ORES_MOBILE_ABI_VERSION; }

ores_mobile_status ores_mobile_copy_message(
    const uint8_t *source, size_t length, uint8_t *destination,
    size_t capacity, size_t *written) {
    if (written == NULL) return ORES_MOBILE_INVALID_ARGUMENT;
    if (overlaps_count(source, length, written) ||
        overlaps_count(destination, capacity, written))
        return ORES_MOBILE_INVALID_ARGUMENT;
    *written = 0u;
    if (length > ORES_MOBILE_MAX_MESSAGE_BYTES) return ORES_MOBILE_MESSAGE_TOO_LARGE;
    if (length != 0u && source == NULL) return ORES_MOBILE_INVALID_ARGUMENT;
    if (length > capacity) return ORES_MOBILE_BUFFER_TOO_SMALL;
    if (length != 0u && destination == NULL) return ORES_MOBILE_INVALID_ARGUMENT;
    if (length != 0u) memmove(destination, source, length);
    *written = length;
    return ORES_MOBILE_OK;
}

ores_mobile_status ores_mobile_dispatch(
    ores_mobile_handler handler, void *context,
    const uint8_t *data, size_t length) {
    if (handler == NULL || (length != 0u && data == NULL))
        return ORES_MOBILE_INVALID_ARGUMENT;
    if (length > ORES_MOBILE_MAX_MESSAGE_BYTES) return ORES_MOBILE_MESSAGE_TOO_LARGE;
    return handler(context, data, length);
}
