#include "atto_xp_ui.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>

static int allowed(void *p) { return *(int *)p; }
static void expect_value(atto_xp_ui_state *state, atto_xp_ui_handle h, int64_t v) {
    int64_t actual = -999;
    assert(atto_xp_ui_read(state,1,2,3,h,&actual) == ATTO_XP_UI_OK);
    assert(actual == v);
}
int main(void) {
    int main_thread = 1;
    atto_xp_ui_state *s = NULL;
    atto_xp_ui_handle handle = 0, old = 0;
    uint64_t revision = 0;
    int64_t expected = 0;
    uint32_t seed = 0x00c0ffee;
    size_t i;
    assert(atto_xp_ui_open(&s,1,2,3,14,allowed,&main_thread) == ATTO_XP_UI_OK);
    atto_xp_ui_command command = {ATTO_XP_UI_CREATE,0,0,0};
    assert(atto_xp_ui_apply(s,1,2,3,revision++,&command,1) == ATTO_XP_UI_OK);
    assert(atto_xp_ui_handle_for_slot(s,1,2,3,0,&handle) == ATTO_XP_UI_OK);
    for (i = 0; i < 5000; ++i) {
        atto_xp_ui_status status;
        atto_xp_ui_command batch[2];
        uint64_t actual_revision = UINT64_MAX;
        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        const int64_t next = (int64_t)(seed % (ATTO_XP_UI_COUNTER_LIMIT + 1u));
        batch[0] = (atto_xp_ui_command){ATTO_XP_UI_SET,0,handle,next};
        batch[1] = (atto_xp_ui_command){ATTO_XP_UI_SET,0,handle,next / 2};
        if (i % 3 == 0) batch[1].slot = ATTO_XP_UI_SLOTS;
        if (i % 5 == 0) batch[1].expected_handle = old ? old : (handle ^ UINT64_C(0x10000));
        status = atto_xp_ui_apply(s,1,2,3,revision,batch,2);
        if (i % 3 == 0 || i % 5 == 0) {
            assert(status == ATTO_XP_UI_INVALID || status == ATTO_XP_UI_STALE);
        } else {
            assert(status == ATTO_XP_UI_OK);
            expected = next / 2;
            ++revision;
        }
        expect_value(s,handle,expected);
        assert(atto_xp_ui_revision(s,1,2,3,&actual_revision) == ATTO_XP_UI_OK);
        assert(actual_revision == revision);
        if (i % 127 == 126) {
            old = handle;
            command = (atto_xp_ui_command){ATTO_XP_UI_REMOVE,0,handle,0};
            assert(atto_xp_ui_apply(s,1,2,3,revision++,&command,1) == ATTO_XP_UI_OK);
            command = (atto_xp_ui_command){ATTO_XP_UI_CREATE,0,0,expected};
            assert(atto_xp_ui_apply(s,1,2,3,revision++,&command,1) == ATTO_XP_UI_OK);
            assert(atto_xp_ui_handle_for_slot(s,1,2,3,0,&handle) == ATTO_XP_UI_OK);
            assert(handle != old);
            int64_t untouched = -1;
            assert(atto_xp_ui_read(s,1,2,3,old,&untouched) == ATTO_XP_UI_STALE);
            assert(untouched == -1);
            expect_value(s,handle,expected);
        }
    }
    /* A valid first command followed by a stale second handle is atomic:
       no speculative data or revision becomes visible on either slot. */
    atto_xp_ui_command create_other = {ATTO_XP_UI_CREATE,1,0,7};
    atto_xp_ui_handle other = 0;
    assert(atto_xp_ui_apply(s,1,2,3,revision,&create_other,1) == ATTO_XP_UI_OK);
    ++revision;
    assert(atto_xp_ui_handle_for_slot(s,1,2,3,1,&other) == ATTO_XP_UI_OK);
    atto_xp_ui_command atomic_batch[2] = {
        {ATTO_XP_UI_SET,0,handle,333},
        {ATTO_XP_UI_SET,1,other ^ UINT64_C(0x10000),9}
    };
    assert(atto_xp_ui_apply(s,1,2,3,revision,atomic_batch,2) == ATTO_XP_UI_STALE);
    expect_value(s,handle,expected);
    expect_value(s,other,7);
    uint64_t unchanged_revision = UINT64_MAX;
    assert(atto_xp_ui_revision(s,1,2,3,&unchanged_revision) == ATTO_XP_UI_OK);
    assert(unchanged_revision == revision);
    atomic_batch[1].expected_handle = other;
    assert(atto_xp_ui_apply(s,1,2,3,revision,atomic_batch,2) == ATTO_XP_UI_OK);
    ++revision;
    expected = 333;
    expect_value(s,handle,expected);
    expect_value(s,other,9);
    int64_t denied_value = -999;
    assert(atto_xp_ui_read(s,1,2,44,other,&denied_value) == ATTO_XP_UI_DENIED);
    assert(denied_value == -999);
    atto_xp_ui_command remove_other = {ATTO_XP_UI_REMOVE,1,other,0};
    assert(atto_xp_ui_apply(s,1,2,3,revision,&remove_other,1) == ATTO_XP_UI_OK);
    ++revision;
    assert(atto_xp_ui_read(s,1,2,3,other,&denied_value) == ATTO_XP_UI_STALE);
    assert(denied_value == -999);
    assert(atto_xp_ui_close(s,1,2,3) == ATTO_XP_UI_OK);
    atto_xp_ui_free(s);
    return 0;
}
