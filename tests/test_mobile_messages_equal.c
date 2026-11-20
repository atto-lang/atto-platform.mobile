#include "ores_mobile_bridge.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>
int main(void) {
    const uint8_t x[4] = {1u,2u,3u,4u};
    uint8_t y[4] = {1u,2u,3u,4u};
    assert(ores_mobile_messages_equal(x,y,4u)==1);
    y[3]=9u;
    assert(ores_mobile_messages_equal(x,y,4u)==0);
    assert(ores_mobile_messages_equal(x,x,4u)==1);
    assert(ores_mobile_messages_equal(NULL,NULL,0u)==1);
    assert(ores_mobile_messages_equal(NULL,y,4u)==-1);
    assert(ores_mobile_messages_equal(x,NULL,4u)==-1);
    assert(ores_mobile_messages_equal(x,y,ORES_MOBILE_MAX_MESSAGE_BYTES+1u)==-1);
    assert(memcmp(x,(const uint8_t[]){1u,2u,3u,4u},sizeof x)==0);
    assert(y[3]==9u);
    return 0;
}
