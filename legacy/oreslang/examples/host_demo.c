#include "ores_mobile_bridge.h"
#include <stdio.h>
#include <stdint.h>
static ores_mobile_status log_message(void *context, const uint8_t *data, size_t length) {
    (void)context;
    printf("OresMobile reference ABI v%u: %zu bytes, first=%u\n",
           ores_mobile_abi_version(), length, length ? (unsigned)data[0] : 0u);
    return ORES_MOBILE_OK;
}
int main(void) {
    const uint8_t message[] = {42u, 1u};
    return ores_mobile_dispatch(log_message, NULL, message, sizeof(message)) == ORES_MOBILE_OK ? 0 : 1;
}
