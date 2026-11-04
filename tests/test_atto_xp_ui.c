#include "atto_xp_ui.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

static int ui_allowed(void *ctx) { return *(int *)ctx; }
#define APPLY(s,rev,cmd,n) atto_xp_ui_apply((s),1,2,3,(rev),(cmd),(n))
#define READ(s,h,out) atto_xp_ui_read((s),1,2,3,(h),(out))
#define HANDLE(s,idx,out) atto_xp_ui_handle_for_slot((s),1,2,3,(idx),(out))
static void test_main(void) {
    int allowed = 1;
    atto_xp_ui_state *s = NULL;
    atto_xp_ui_handle a = 0, b = 0, previous = 0;
    int64_t val = -1;
    uint64_t rev = 99;
    atto_xp_ui_command cmds[ATTO_XP_UI_BATCH_LIMIT + 1u] = {{0}};
    assert(atto_xp_ui_abi_version() == 1);
    assert(atto_xp_ui_open(NULL,1,2,3,1,ui_allowed,&allowed) == ATTO_XP_UI_INVALID);
    assert(atto_xp_ui_open(&s,1,2,3,0,ui_allowed,&allowed) == ATTO_XP_UI_INVALID && s == NULL);
    assert(atto_xp_ui_open(&s,1,2,3,ATTO_XP_UI_EPOCH_LIMIT+1u,ui_allowed,&allowed) == ATTO_XP_UI_INVALID);
    assert(atto_xp_ui_open(&s,1,2,3,1,NULL,&allowed) == ATTO_XP_UI_INVALID);
    allowed = 0;
    assert(atto_xp_ui_open(&s,1,2,3,1,ui_allowed,&allowed) == ATTO_XP_UI_WRONG_THREAD);
    allowed = 1;
    assert(atto_xp_ui_open(&s,1,2,3,1,ui_allowed,&allowed) == ATTO_XP_UI_OK && s);
    assert(atto_xp_ui_revision(s,1,2,3,&rev) == ATTO_XP_UI_OK && rev == 0);
    assert(atto_xp_ui_revision(s,1,2,5,&rev) == ATTO_XP_UI_DENIED && rev == 0);
    assert(HANDLE(s,0,&a) == ATTO_XP_UI_STALE && a == 0);
    cmds[0] = (atto_xp_ui_command){ATTO_XP_UI_CREATE,0,0,40};
    cmds[1] = (atto_xp_ui_command){ATTO_XP_UI_CREATE,1,0,2};
    assert(APPLY(s,0,cmds,2) == ATTO_XP_UI_OK);
    assert(HANDLE(s,0,&a) == ATTO_XP_UI_OK && a != 0);
    assert(HANDLE(s,1,&b) == ATTO_XP_UI_OK && b != 0 && b != a);
    assert(READ(s,a,&val) == ATTO_XP_UI_OK && val == 40);
    assert(READ(s,b,&val) == ATTO_XP_UI_OK && val == 2);
    assert(APPLY(s,0,cmds,2) == ATTO_XP_UI_REVISION);
    assert(atto_xp_ui_apply(s,1,2,4,1,cmds,2) == ATTO_XP_UI_DENIED);
    assert(atto_xp_ui_apply(s,10,2,3,1,cmds,2) == ATTO_XP_UI_DENIED);
    assert(atto_xp_ui_apply(s,1,10,3,1,cmds,2) == ATTO_XP_UI_DENIED);
    allowed=0;
    assert(APPLY(s,1,cmds,2) == ATTO_XP_UI_WRONG_THREAD);
    assert(HANDLE(s,0,&previous) == ATTO_XP_UI_WRONG_THREAD);
    allowed=1;
    /* Failed second op must not leak the first SET. */
    cmds[0] = (atto_xp_ui_command){ATTO_XP_UI_SET,0,a,42};
    cmds[1] = (atto_xp_ui_command){ATTO_XP_UI_SET,1,a,43};
    assert(APPLY(s,1,cmds,2) == ATTO_XP_UI_STALE);
    assert(READ(s,a,&val) == ATTO_XP_UI_OK && val == 40);
    assert(atto_xp_ui_revision(s,1,2,3,&rev) == ATTO_XP_UI_OK && rev == 1);
    cmds[1] = (atto_xp_ui_command){ATTO_XP_UI_SET,1,b,2};
    assert(APPLY(s,1,cmds,2) == ATTO_XP_UI_OK);
    assert(READ(s,a,&val) == ATTO_XP_UI_OK && val == 42);
    cmds[0] = (atto_xp_ui_command){ATTO_XP_UI_REMOVE,0,a,0};
    assert(APPLY(s,2,cmds,1) == ATTO_XP_UI_OK);
    assert(READ(s,a,&val) == ATTO_XP_UI_STALE && val == 42);
    cmds[0] = (atto_xp_ui_command){ATTO_XP_UI_CREATE,0,0,17};
    assert(APPLY(s,3,cmds,1) == ATTO_XP_UI_OK);
    assert(HANDLE(s,0,&previous) == ATTO_XP_UI_OK && previous != a);
    assert(READ(s,a,&val) == ATTO_XP_UI_STALE);
    assert(READ(s,previous,&val) == ATTO_XP_UI_OK && val == 17);
    cmds[0] = (atto_xp_ui_command){ATTO_XP_UI_SET,0,previous,ATTO_XP_UI_COUNTER_LIMIT+1};
    assert(APPLY(s,4,cmds,1) == ATTO_XP_UI_INVALID);
    cmds[0].counter = -1;
    assert(APPLY(s,4,cmds,1) == ATTO_XP_UI_INVALID);
    cmds[0].counter = 41;
    assert(APPLY(s,4,cmds,ATTO_XP_UI_BATCH_LIMIT+1u) == ATTO_XP_UI_QUOTA);
    cmds[0].slot = ATTO_XP_UI_SLOTS;
    assert(APPLY(s,4,cmds,1) == ATTO_XP_UI_INVALID);
    cmds[0].slot = 0;
    cmds[0].operation = 99;
    assert(APPLY(s,4,cmds,1) == ATTO_XP_UI_INVALID);
    cmds[0].operation = ATTO_XP_UI_SET;
    assert(APPLY(s,4,cmds,0) == ATTO_XP_UI_INVALID);
    assert(APPLY(s,4,NULL,1) == ATTO_XP_UI_INVALID);
    assert(READ(s,previous,NULL) == ATTO_XP_UI_INVALID);
    assert(READ(s,previous ^ ((uint64_t)1 << 40),&val) == ATTO_XP_UI_STALE);
    assert(READ(s,UINT64_MAX,&val) == ATTO_XP_UI_STALE);
    assert(atto_xp_ui_close(s,1,2,3) == ATTO_XP_UI_OK);
    assert(READ(s,previous,&val) == ATTO_XP_UI_CLOSED);
    assert(APPLY(s,4,cmds,1) == ATTO_XP_UI_CLOSED);
    assert(atto_xp_ui_close(s,1,2,3) == ATTO_XP_UI_CLOSED);
    atto_xp_ui_free(s);
    s = NULL;
    assert(atto_xp_ui_open(&s,1,2,3,2,ui_allowed,&allowed) == ATTO_XP_UI_OK);
    cmds[0] = (atto_xp_ui_command){ATTO_XP_UI_CREATE,0,0,5};
    assert(APPLY(s,0,cmds,1) == ATTO_XP_UI_OK);
    assert(READ(s,previous,&val) == ATTO_XP_UI_STALE);
    atto_xp_ui_free(s);
}
static void test_deterministic_model(void) {
    int allowed=1;
    atto_xp_ui_state *s=NULL;
    atto_xp_ui_handle h;
    uint64_t rev=0;
    int64_t value=-1;
    atto_xp_ui_command batch[2] = {{ATTO_XP_UI_CREATE,0,0,40},{ATTO_XP_UI_CREATE,1,0,2}};
    assert(atto_xp_ui_open(&s,1,2,3,11,ui_allowed,&allowed)==ATTO_XP_UI_OK);
    assert(APPLY(s,rev++,batch,2)==ATTO_XP_UI_OK);
    assert(HANDLE(s,0,&h)==ATTO_XP_UI_OK);
    batch[0]=(atto_xp_ui_command){ATTO_XP_UI_SET,0,h,42};
    assert(APPLY(s,rev++,batch,1)==ATTO_XP_UI_OK);
    assert(READ(s,h,&value)==ATTO_XP_UI_OK && value==42);
    batch[0]=(atto_xp_ui_command){ATTO_XP_UI_SET,0,h,41};
    assert(APPLY(s,rev++,batch,1)==ATTO_XP_UI_OK);
    assert(READ(s,h,&value)==ATTO_XP_UI_OK && value==41);
    batch[0]=(atto_xp_ui_command){ATTO_XP_UI_SET,0,h,0};
    assert(APPLY(s,rev++,batch,1)==ATTO_XP_UI_OK);
    assert(READ(s,h,&value)==ATTO_XP_UI_OK && value==0);
    assert(atto_xp_ui_revision(s,1,2,3,&rev)==ATTO_XP_UI_OK && rev==4);
    atto_xp_ui_free(s);
}
int main(void) { test_main(); test_deterministic_model(); return 0; }
