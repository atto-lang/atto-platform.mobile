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

/* Copies bytes into caller-owned memory. Zero-length payloads are allowed.
 * Source/destination may overlap. The written count must not overlap either
 * byte range; conflicting count pointers are rejected without any writes. */
ores_mobile_status ores_mobile_copy_message(
    const uint8_t *source, size_t length, uint8_t *destination,
    size_t capacity, size_t *written);

/* CPU-side non-elidable wipe of a temporary, caller-owned message buffer.
 * Does not free storage, erase copies (e.g. Java heap), or guarantee physical
 * erasure by a device. Rejects oversized/NULL nonempty spans without writes. */
ores_mobile_status ores_mobile_clear_message(uint8_t *buffer, size_t length);

/* Transfer a mutable source buffer into disjoint caller-owned destination
 * storage, then CPU-scrub exactly length source bytes on success.
 * Source and destination may NOT overlap; the written count must not alias
 * source or destination storage. Rejections never wipe source/destination.
 * Does not erase other copies or guarantee device-level erasure. */
ores_mobile_status ores_mobile_handoff_message(
    uint8_t *source, size_t length, uint8_t *destination,
    size_t capacity, size_t *written);

/* Calls handler synchronously; does not enqueue, spawn or schedule actors.
 * Callback results outside the declared status enum fail closed as INVALID_ARGUMENT. */
ores_mobile_status ores_mobile_dispatch(
    ores_mobile_handler handler, void *context,
    const uint8_t *data, size_t length);

/* Copy-isolated synchronous dispatch using caller-owned scratch; no allocation.
 * Scratch must not overlap the input byte range. The callback borrows the copy
 * only for its invocation and must not retain it or alter scratch ownership.
 * Exactly length scratch bytes are scrubbed after every callback result,
 * including invalid statuses. Rejected calls do not modify scratch or invoke
 * the handler. Zero-length messages allow NULL data/scratch. Access must be
 * serialized; this is not an actor mailbox or a hardware isolation boundary. */
ores_mobile_status ores_mobile_dispatch_copy(
    ores_mobile_handler handler, void *context,
    const uint8_t *data, size_t length,
    uint8_t *scratch, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif

