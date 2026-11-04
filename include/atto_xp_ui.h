#ifndef ATTO_XP_UI_H
#define ATTO_XP_UI_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* AttoXP scalar UI host-reference MODEL v1; host ONLY. No native widget creation.
 * A platform adapter must provide a trustworthy OS-main-thread check.
 * All calls on a state are serialized by that adapter.
 * IMPORTANT: NOT wire-compatible with the proposed 24-byte AttoXpUiHandleV1
 * / async submit_batch ABI in platform-conformance. Its 64-bit local handle,
 * 32-slot/64-command subset is only a fail-closed ownership/revision model. */
#define ATTO_XP_UI_ABI_VERSION 1u /* reference-model version, not native wire ABI */
#define ATTO_XP_UI_SLOTS 32u
#define ATTO_XP_UI_BATCH_LIMIT 64u
#define ATTO_XP_UI_COUNTER_LIMIT 1000000
#define ATTO_XP_UI_EPOCH_LIMIT 16777215u

typedef uint64_t atto_xp_ui_handle;
typedef struct atto_xp_ui_state atto_xp_ui_state;
typedef int (*atto_xp_ui_thread_check)(void *context);
typedef enum {
    ATTO_XP_UI_OK = 0,
    ATTO_XP_UI_INVALID = 1,
    ATTO_XP_UI_WRONG_THREAD = 2,
    ATTO_XP_UI_DENIED = 3,
    ATTO_XP_UI_STALE = 4,
    ATTO_XP_UI_REVISION = 5,
    ATTO_XP_UI_QUOTA = 6,
    ATTO_XP_UI_CLOSED = 7,
    ATTO_XP_UI_EXHAUSTED = 8,
    ATTO_XP_UI_REENTRANT = 9
} atto_xp_ui_status;
typedef enum {
    ATTO_XP_UI_CREATE = 1,
    ATTO_XP_UI_SET = 2,
    ATTO_XP_UI_REMOVE = 3
} atto_xp_ui_operation;
typedef struct {
    uint32_t operation;
    uint32_t slot; /* 0 .. ATTO_XP_UI_SLOTS - 1 */
    atto_xp_ui_handle expected_handle; /* zero only for CREATE */
    int64_t counter; /* nonnegative bounded value for CREATE/SET; zero for REMOVE */
} atto_xp_ui_command;

/* app, scene, owner and scene_epoch must be nonzero. scene_epoch is unique
 * and strictly increasing PROCESS-WIDE, across ALL scene identities and
 * after close/free. Reusing an epoch would alias a stale handle from another
 * scene, so the implementation rejects it; the host must handle exhaustion.
 * Handles are never durable across process restarts. Caller owns state. */
uint32_t atto_xp_ui_abi_version(void);
atto_xp_ui_status atto_xp_ui_open(
    atto_xp_ui_state **out, uint64_t app, uint64_t scene,
    uint64_t actor_owner, uint32_t scene_epoch,
    atto_xp_ui_thread_check main_thread_check, void *thread_context);
/* revision must match the committed state exactly; revision increments once
 * on success. Entire batch is validated on scratch before any commit.
 * A failed batch changes neither revision nor nodes. */
atto_xp_ui_status atto_xp_ui_apply(
    atto_xp_ui_state *state, uint64_t app, uint64_t scene, uint64_t actor_owner,
    uint64_t expected_revision, const atto_xp_ui_command *commands, size_t count);
atto_xp_ui_status atto_xp_ui_handle_for_slot(
    atto_xp_ui_state *state, uint64_t app, uint64_t scene, uint64_t actor_owner,
    uint32_t slot, atto_xp_ui_handle *out);
atto_xp_ui_status atto_xp_ui_read(
    atto_xp_ui_state *state, uint64_t app, uint64_t scene, uint64_t actor_owner,
    atto_xp_ui_handle handle, int64_t *out);
atto_xp_ui_status atto_xp_ui_revision(
    atto_xp_ui_state *state, uint64_t app, uint64_t scene, uint64_t actor_owner,
    uint64_t *out);
atto_xp_ui_status atto_xp_ui_close(
    atto_xp_ui_state *state, uint64_t app, uint64_t scene, uint64_t actor_owner);
/* Destruction requires the caller to quiesce all users of state. Never reuse
 * its pointer; calling after free is undefined, unlike stale handle checks. */
void atto_xp_ui_free(atto_xp_ui_state *state);
#ifdef __cplusplus
}
#endif
#endif
