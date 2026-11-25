#ifndef ATTO_MOBILE_UI_V1_H
#define ATTO_MOBILE_UI_V1_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define ATTO_MOBILE_UI_MAX_BYTES 65536u
#define ATTO_MOBILE_UI_MAX_OPS 256u

/* VALUE-ONLY descriptor. Never contains UIView/View/JNI/ObjC pointers. */
typedef struct atto_mobile_ui_intent_v1 {
    uint64_t app, scene, owner, generation, expected_revision;
    uint32_t encoded_bytes, operations;
} atto_mobile_ui_intent_v1;

/* Constructed by TRUSTED host at the moment of commit; not by a guest. */
typedef struct atto_mobile_ui_host_v1 {
    uint64_t app, scene, owner, generation, revision;
    uint8_t alive, foreground, on_ui_thread;
    uint8_t os_permission, entitlement, actor_grant;
} atto_mobile_ui_host_v1;

typedef enum atto_mobile_ui_decision_v1 {
    ATTO_MOBILE_UI_ADMISSIBLE = 0, /* preflight ONLY, not proof of UI commit */
    ATTO_MOBILE_UI_INVALID = 1,
    ATTO_MOBILE_UI_WRONG_THREAD = 2,
    ATTO_MOBILE_UI_STALE_OWNER = 3,
    ATTO_MOBILE_UI_INACTIVE = 4,
    ATTO_MOBILE_UI_PERMISSION_DENIED = 5,
    ATTO_MOBILE_UI_QUOTA = 6,
    ATTO_MOBILE_UI_STALE_REVISION = 7
} atto_mobile_ui_decision_v1;

atto_mobile_ui_decision_v1 atto_mobile_ui_preflight_v1(
    const atto_mobile_ui_intent_v1 *intent,
    const atto_mobile_ui_host_v1 *host);
#ifdef __cplusplus
}
#endif
#endif
