#ifndef ORES_MOBILE_BRIDGE_H
#define ORES_MOBILE_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Portable C reference ABI, NOT an Oreslang actor runtime. */
#define ORES_MOBILE_ABI_VERSION 1u
#define ORES_MOBILE_MAX_MESSAGE_BYTES 65536u

typedef enum {
    ORES_MOBILE_OK = 0,
    ORES_MOBILE_INVALID_ARGUMENT = 1,
    ORES_MOBILE_MESSAGE_TOO_LARGE = 2,
    ORES_MOBILE_BUFFER_TOO_SMALL = 3
} ores_mobile_status;

/* Callback is synchronous; data may only be borrowed for the invocation. */
typedef ores_mobile_status (*ores_mobile_handler)(
    void *context, const uint8_t *data, size_t length);

uint32_t ores_mobile_abi_version(void);

/* Copies bytes into caller-owned memory. Zero-length payloads are allowed. */
ores_mobile_status ores_mobile_copy_message(
    const uint8_t *source, size_t length, uint8_t *destination,
    size_t capacity, size_t *written);

/* Calls handler synchronously; does not enqueue, spawn or schedule actors. */
ores_mobile_status ores_mobile_dispatch(
    ores_mobile_handler handler, void *context,
    const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif
#endif
