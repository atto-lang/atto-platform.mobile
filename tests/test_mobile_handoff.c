#include "ores_mobile_bridge.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

int main(void) {
    uint8_t source[4] = {1, 2, 3, 4};
    uint8_t destination[4] = {9, 9, 9, 9};
    const uint8_t expected[4] = {1, 2, 3, 4};
    const uint8_t cleared[4] = {0, 0, 0, 0};
    size_t written = 999;
    assert(ores_mobile_handoff_message(source, 4, destination, 4, &written)
           == ORES_MOBILE_OK);
    assert(written == 4 && memcmp(destination, expected, 4) == 0);
    assert(memcmp(source, cleared, 4) == 0);

    source[0] = 42;
    destination[0] = 99;
    written = 999;
    assert(ores_mobile_handoff_message(source, 4, destination, 3, &written)
           == ORES_MOBILE_BUFFER_TOO_SMALL);
    assert(written == 0 && source[0] == 42 && destination[0] == 99);

    written = 999;
    assert(ores_mobile_handoff_message(source, 3, source + 1, 3, &written)
           == ORES_MOBILE_INVALID_ARGUMENT);
    assert(written == 0 && source[0] == 42 && source[1] == 0);

    written = 999;
    assert(ores_mobile_handoff_message(source, 4, NULL, 4, &written)
           == ORES_MOBILE_INVALID_ARGUMENT);
    assert(written == 0 && source[0] == 42);
    assert(ores_mobile_handoff_message(NULL, 0, NULL, 0, &written)
           == ORES_MOBILE_OK && written == 0);
    assert(ores_mobile_handoff_message(source, 1, destination, 4, NULL)
           == ORES_MOBILE_INVALID_ARGUMENT && source[0] == 42);
    assert(ores_mobile_handoff_message(source, 1, destination, 4,
           (size_t *)(void *)source) == ORES_MOBILE_INVALID_ARGUMENT);
    assert(source[0] == 42 && destination[0] == 99);
    return 0;
}
