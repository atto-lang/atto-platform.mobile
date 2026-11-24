#include "../include/atto_mobile_ui_v1.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
    atto_mobile_ui_intent_v1 i = {1, 2, 3, 4, 5, 128, 2};
    atto_mobile_ui_host_v1 h = {1, 2, 3, 4, 5, 1, 1, 1, 1, 1, 1};
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_ADMISSIBLE);
    assert(atto_mobile_ui_preflight_v1(NULL, &h) == ATTO_MOBILE_UI_INVALID);
    assert(atto_mobile_ui_preflight_v1(&i, NULL) == ATTO_MOBILE_UI_INVALID);
    h.on_ui_thread = 0;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_WRONG_THREAD);
    h.on_ui_thread = 1;
    i.owner = 6;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_STALE_OWNER);
    i.owner = 3;
    i.generation = 9;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_STALE_OWNER);
    i.generation = 4;
    h.foreground = 0;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_INACTIVE);
    h.foreground = 1;
    h.alive = 0;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_INACTIVE);
    h.alive = 1;
    h.os_permission = 0;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_PERMISSION_DENIED);
    h.os_permission = 1;
    h.entitlement = 0;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_PERMISSION_DENIED);
    h.entitlement = 1;
    h.actor_grant = 0;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_PERMISSION_DENIED);
    h.actor_grant = 1;
    i.encoded_bytes = 65537;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_QUOTA);
    i.encoded_bytes = 128;
    i.operations = 257;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_QUOTA);
    i.operations = 2;
    i.expected_revision = 4;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_STALE_REVISION);
    i.expected_revision = 5;
    h.revision = UINT64_MAX;
    assert(atto_mobile_ui_preflight_v1(&i, &h) == ATTO_MOBILE_UI_STALE_REVISION);
    puts("mobile host preflight: PASS (C host-only, not iOS/Android/Atto guest)");
    return 0;
}
