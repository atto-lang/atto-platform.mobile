#include "atto_xp_ui.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t generation;
    int live;
    int64_t counter;
} ui_slot;
struct atto_xp_ui_state {
    uint64_t app, scene, owner, revision;
    uint32_t epoch;
    atto_xp_ui_thread_check check;
    void *context;
    int closed;
    ui_slot slots[ATTO_XP_UI_SLOTS];
};
static atto_xp_ui_handle make_handle(const atto_xp_ui_state *s, uint32_t slot, uint32_t generation) {
    return ((uint64_t)s->epoch << 40) | ((uint64_t)generation << 16) | ((uint64_t)slot + 1u);
}
static atto_xp_ui_status admit(atto_xp_ui_state *s, uint64_t app, uint64_t scene, uint64_t owner) {
    if (!s || !app || !scene || !owner) return ATTO_XP_UI_INVALID;
    if (s->app != app || s->scene != scene || s->owner != owner) return ATTO_XP_UI_DENIED;
    if (!s->check || !s->check(s->context)) return ATTO_XP_UI_WRONG_THREAD;
    if (s->closed) return ATTO_XP_UI_CLOSED;
    return ATTO_XP_UI_OK;
}
uint32_t atto_xp_ui_abi_version(void) { return ATTO_XP_UI_ABI_VERSION; }
atto_xp_ui_status atto_xp_ui_open(atto_xp_ui_state **out, uint64_t app, uint64_t scene,
                                  uint64_t owner, uint32_t epoch,
                                  atto_xp_ui_thread_check check, void *context) {
    atto_xp_ui_state *state;
    if (!out) return ATTO_XP_UI_INVALID;
    *out = NULL;
    if (!app || !scene || !owner || !epoch || epoch > ATTO_XP_UI_EPOCH_LIMIT || !check)
        return ATTO_XP_UI_INVALID;
    if (!check(context)) return ATTO_XP_UI_WRONG_THREAD;
    state = (atto_xp_ui_state *)calloc(1, sizeof *state);
    if (!state) return ATTO_XP_UI_QUOTA;
    state->app = app; state->scene = scene; state->owner = owner;
    state->epoch = epoch; state->check = check; state->context = context;
    *out = state;
    return ATTO_XP_UI_OK;
}
atto_xp_ui_status atto_xp_ui_apply(atto_xp_ui_state *s, uint64_t app, uint64_t scene,
    uint64_t owner, uint64_t revision, const atto_xp_ui_command *commands, size_t count) {
    ui_slot copy[ATTO_XP_UI_SLOTS];
    atto_xp_ui_status status = admit(s, app, scene, owner);
    size_t i;
    if (status != ATTO_XP_UI_OK) return status;
    if (revision != s->revision) return ATTO_XP_UI_REVISION;
    if (!commands || !count) return ATTO_XP_UI_INVALID;
    if (count > ATTO_XP_UI_BATCH_LIMIT) return ATTO_XP_UI_QUOTA;
    if (s->revision == UINT64_MAX) return ATTO_XP_UI_EXHAUSTED;
    memcpy(copy, s->slots, sizeof copy);
    for (i = 0; i < count; ++i) {
        const atto_xp_ui_command *cmd = commands + i;
        ui_slot *node;
        if (cmd->slot >= ATTO_XP_UI_SLOTS) return ATTO_XP_UI_INVALID;
        node = &copy[cmd->slot];
        if (cmd->operation == ATTO_XP_UI_CREATE) {
            if (node->live || cmd->expected_handle) return ATTO_XP_UI_STALE;
            if (cmd->counter < 0 || cmd->counter > ATTO_XP_UI_COUNTER_LIMIT) return ATTO_XP_UI_INVALID;
            if (node->generation == 0) node->generation = 1;
            node->live = 1; node->counter = cmd->counter;
        } else if (cmd->operation == ATTO_XP_UI_SET || cmd->operation == ATTO_XP_UI_REMOVE) {
            if (!node->live || cmd->expected_handle != make_handle(s, cmd->slot, node->generation))
                return ATTO_XP_UI_STALE;
            if (cmd->operation == ATTO_XP_UI_SET) {
                if (cmd->counter < 0 || cmd->counter > ATTO_XP_UI_COUNTER_LIMIT)
                    return ATTO_XP_UI_INVALID;
                node->counter = cmd->counter;
            } else {
                if (cmd->counter) return ATTO_XP_UI_INVALID;
                if (node->generation == ATTO_XP_UI_EPOCH_LIMIT) return ATTO_XP_UI_EXHAUSTED;
                node->live = 0; node->counter = 0; ++node->generation;
            }
        } else return ATTO_XP_UI_INVALID;
    }
    memcpy(s->slots, copy, sizeof copy);
    ++s->revision;
    return ATTO_XP_UI_OK;
}
atto_xp_ui_status atto_xp_ui_handle_for_slot(atto_xp_ui_state *s,
    uint64_t app, uint64_t scene, uint64_t owner, uint32_t slot, atto_xp_ui_handle *out) {
    atto_xp_ui_status status = admit(s, app, scene, owner);
    if (status != ATTO_XP_UI_OK) return status;
    if (!out || slot >= ATTO_XP_UI_SLOTS) return ATTO_XP_UI_INVALID;
    if (!s->slots[slot].live) return ATTO_XP_UI_STALE;
    *out = make_handle(s, slot, s->slots[slot].generation);
    return ATTO_XP_UI_OK;
}
atto_xp_ui_status atto_xp_ui_read(atto_xp_ui_state *s, uint64_t app, uint64_t scene,
    uint64_t owner, atto_xp_ui_handle h, int64_t *out) {
    uint32_t idx = (uint32_t)(h & 0xffffu);
    atto_xp_ui_status status = admit(s, app, scene, owner);
    if (status != ATTO_XP_UI_OK) return status;
    if (!out) return ATTO_XP_UI_INVALID;
    if (idx == 0 || idx > ATTO_XP_UI_SLOTS) return ATTO_XP_UI_STALE;
    --idx;
    if (!s->slots[idx].live || h != make_handle(s, idx, s->slots[idx].generation))
        return ATTO_XP_UI_STALE;
    *out = s->slots[idx].counter;
    return ATTO_XP_UI_OK;
}
atto_xp_ui_status atto_xp_ui_revision(atto_xp_ui_state *s,
    uint64_t app, uint64_t scene, uint64_t owner, uint64_t *out) {
    atto_xp_ui_status status = admit(s, app, scene, owner);
    if (status != ATTO_XP_UI_OK) return status;
    if (!out) return ATTO_XP_UI_INVALID;
    *out = s->revision;
    return ATTO_XP_UI_OK;
}
atto_xp_ui_status atto_xp_ui_close(atto_xp_ui_state *s, uint64_t app, uint64_t scene, uint64_t owner) {
    atto_xp_ui_status status = admit(s, app, scene, owner);
    if (status != ATTO_XP_UI_OK) return status;
    memset(s->slots, 0, sizeof s->slots);
    s->closed = 1;
    return ATTO_XP_UI_OK;
}
void atto_xp_ui_free(atto_xp_ui_state *s) {
    if (!s) return;
    memset(s, 0, sizeof *s);
    free(s);
}
