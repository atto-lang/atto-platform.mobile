#include "ores_phone_policy.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #expr); \
    return 1; \
} } while (0)

static ores_phone_app_policy a(void) {
    ores_phone_app_policy p = {
        .app_id = "org.oreslang.camera", .uid = 200001u,
        .executable_path = "/opt/ores-phone/apps/org.oreslang.camera/bin/app",
        .private_data_path = "/var/lib/ores-phone/apps/org.oreslang.camera",
        .memory_max_bytes = 128ull * 1024ull * 1024ull,
        .cpu_quota_us = 50000u, .cpu_period_us = 100000u,
        .requested_capabilities = ORES_PHONE_CAP_CAMERA,
        .granted_capabilities = ORES_PHONE_CAP_CAMERA
    };
    return p;
}

static ores_phone_app_policy b(void) {
    ores_phone_app_policy p = {
        .app_id = "org.oreslang.messaging", .uid = 200002u,
        .executable_path = "/opt/ores-phone/apps/org.oreslang.messaging/bin/app",
        .private_data_path = "/var/lib/ores-phone/apps/org.oreslang.messaging",
        .memory_max_bytes = 64ull * 1024ull * 1024ull,
        .cpu_quota_us = 25000u, .cpu_period_us = 100000u,
        .requested_capabilities = 0u, .granted_capabilities = 0u
    };
    return p;
}

int main(void) {
    ores_phone_app_policy p = a();
    ores_phone_app_policy pair[2] = {a(), b()};
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_OK);
    CHECK(ores_phone_policy_validate_set(pair, 2) == ORES_PHONE_POLICY_OK);
    CHECK(ores_phone_policy_validate(NULL) == ORES_PHONE_POLICY_INVALID_ARGUMENT);
    CHECK(ores_phone_policy_validate_set(NULL, 1) == ORES_PHONE_POLICY_INVALID_ARGUMENT);
    CHECK(ores_phone_policy_validate_set(pair, 0) == ORES_PHONE_POLICY_INVALID_ARGUMENT);
    CHECK(ores_phone_policy_validate_set(pair, 1025) == ORES_PHONE_POLICY_INVALID_ARGUMENT);

    p = a(); p.app_id = "org..camera";
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_APP_ID);
    p = a(); p.app_id = "Org.oreslang.camera";
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_APP_ID);
    p = a(); p.app_id = "org.oreslang.camera/../../other";
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_APP_ID);
    p = a(); p.app_id = "org.camera-";
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_APP_ID);
    p = a(); p.app_id = "org.1camera";
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_APP_ID);
    p = a(); p.app_id = "org.camera.";
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_APP_ID);
    p = a(); p.uid = 0;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_UNSAFE_UID);
    p = a(); p.uid = 99999;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_UNSAFE_UID);
    p = a(); p.uid = 1000000;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_UNSAFE_UID);

    p = a(); p.executable_path = "/usr/bin/sh";
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_PATH);
    p = a(); p.executable_path = "/opt/ores-phone/apps/org.oreslang.camera/bin/../bin/app";
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_PATH);
    p = a(); p.private_data_path = "/var/lib/ores-phone/apps/org.oreslang.messaging";
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_PATH);
    p = a(); p.private_data_path = NULL;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_PATH);
    p = a(); p.memory_max_bytes = 0;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_BUDGET);
    p = a(); p.memory_max_bytes = 5ull * 1024ull * 1024ull * 1024ull;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_BUDGET);
    p = a(); p.cpu_period_us = 0;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_BUDGET);
    p = a(); p.cpu_quota_us = 100001;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_BUDGET);
    p = a(); p.cpu_quota_us = 0;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_INVALID_BUDGET);

    p = a(); p.granted_capabilities = 0;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_DENIED_CAPABILITY);
    p = a(); p.requested_capabilities = 1u << 31;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_DENIED_CAPABILITY);
    p = a(); p.granted_capabilities = 1u << 31;
    CHECK(ores_phone_policy_validate(&p) == ORES_PHONE_POLICY_DENIED_CAPABILITY);

    pair[1] = b(); pair[1].uid = pair[0].uid;
    CHECK(ores_phone_policy_validate_set(pair, 2) == ORES_PHONE_POLICY_DUPLICATE_SECURITY_DOMAIN);
    pair[1] = a(); pair[1].uid = 200002;
    CHECK(ores_phone_policy_validate_set(pair, 2) == ORES_PHONE_POLICY_DUPLICATE_SECURITY_DOMAIN);

    CHECK(strcmp(ores_phone_policy_status_name(ORES_PHONE_POLICY_OK), "ok") == 0);
    CHECK(strcmp(ores_phone_policy_status_name((ores_phone_policy_status)99), "unknown") == 0);
    puts("phone policy host contract: passed");
    return 0;
}
