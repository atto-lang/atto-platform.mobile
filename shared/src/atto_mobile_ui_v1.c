#include "../include/atto_mobile_ui_v1.h"
#include <stdint.h>

atto_mobile_ui_decision_v1 atto_mobile_ui_preflight_v1(
    const atto_mobile_ui_intent_v1 *i,
    const atto_mobile_ui_host_v1 *h) {
    if (!i || !h || !i->app || !i->scene || !i->owner || !i->generation)
        return ATTO_MOBILE_UI_INVALID;
    if (!h->on_ui_thread) return ATTO_MOBILE_UI_WRONG_THREAD;
    if (i->app != h->app || i->scene != h->scene ||
        i->owner != h->owner || i->generation != h->generation)
        return ATTO_MOBILE_UI_STALE_OWNER;
    if (!h->alive || !h->foreground) return ATTO_MOBILE_UI_INACTIVE;
    if (!h->os_permission || !h->entitlement || !h->actor_grant)
        return ATTO_MOBILE_UI_PERMISSION_DENIED;
    if (!i->encoded_bytes || i->encoded_bytes > ATTO_MOBILE_UI_MAX_BYTES ||
        !i->operations || i->operations > ATTO_MOBILE_UI_MAX_OPS)
        return ATTO_MOBILE_UI_QUOTA;
    if (i->expected_revision != h->revision || h->revision == UINT64_MAX)
        return ATTO_MOBILE_UI_STALE_REVISION;
    return ATTO_MOBILE_UI_ADMISSIBLE;
}
